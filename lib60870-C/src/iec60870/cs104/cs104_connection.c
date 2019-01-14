/*
 *  Copyright 2016 MZ Automation GmbH
 *
 *  This file is part of lib60870-C
 *
 *  lib60870-C is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  lib60870-C is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with lib60870-C.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include "cs104_connection.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include <QDebug>
#else
#include <syslog.h>
#endif
#include <errno.h>

#include "cs104_frame.h"
#include "hal_thread.h"
#include "hal_socket.h"
#include "hal_time.h"
#include "lib_memory.h"

#include "apl_types_internal.h"
#include "information_objects_internal.h"
#include "lib60870_internal.h"
#include "cs101_asdu_internal.h"

struct sCS104_APCIParameters defaultAPCIParameters = {
		/* .k = */ 12,
        /* .w = */ 1,
		/* .t0 = */ 10,
		/* .t1 = */ 15,
		/* .t2 = */ 10,
        /* .t3 = */ 10
};//old default value: w=8; t3=20

static struct sCS101_AppLayerParameters defaultAppLayerParameters = {
    /* .sizeOfTypeId =  */ 1,
    /* .sizeOfVSQ = */ 1,
    /* .sizeOfCOT = */ 2,
    /* .originatorAddress = */ 0,
    /* .sizeOfCA = */ 2,
    /* .sizeOfIOA = */ 3,
};

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

typedef struct {
    uint64_t sentTime; /* required for T1 timeout */
    int seqNo;
} SentASDU;

struct sCS104_Connection {
    char hostname[HOST_NAME_MAX + 1];
    int tcpPort;

    struct sCS104_APCIParameters parameters;
    struct sCS101_AppLayerParameters alParameters;

    int connectTimeoutInMs;
    uint8_t sMessage[6];

    SentASDU* sentASDUs; /* the k-buffer */
    int maxSentASDUs;    /* maximum number of ASDU to be sent without confirmation - parameter k */
    int oldestSentASDU;  /* index of oldest entry in k-buffer */
    int newestSentASDU;  /* index of newest entry in k-buffer */
#if (CONFIG_USE_THREADS == 1)
    Semaphore sentASDUsLock;
    Thread connectionHandlingThread;
#endif

    int receiveCount;
    int sendCount;

    bool firstIMessageReceived;

    int unconfirmedReceivedIMessages;
    uint64_t lastConfirmationTime;

    uint64_t nextT3Timeout;
    int outstandingTestFCConMessages;

    uint64_t uMessageTimeout;

    Socket socket;
    bool running;
    bool failure;
    bool close;

#if (CONFIG_CS104_SUPPORT_TLS == 1)
    TLSConfiguration tlsConfig;
    TLSSocket tlsSocket;
#endif

    CS101_ASDUReceivedHandler receivedHandler;
    void* receivedHandlerParameter;

    CS104_ConnectionHandler connectionHandler;
    void* connectionHandlerParameter;

    //报文收发
    CS104_MSGReceivedHandler msgreceivedHandler;
    void* msgreceivedHandlerParameter;

    CS104_MSGSendHandler msgsendHandler;
    void* msgsendHandlerParameter;

    //报文收发（带解析版本）
    CS104_MSGReceivedHandler_withExplain msgreceivedHandler_withExplain;
    void* msgreceivedHandlerParameter_withExplain;

    CS104_MSGSendHandler_withExplain msgsendHandler_withExplain;
    void* msgsendHandlerParameter_withExplain;

    _FRAMESTRUCT104 cs104_frame;//104帧结构

    CS104_ImportantInfoHandler importantInfoHandler;
    void* importantInfoHandlerParameter;

}connectionHandler;


static uint8_t STARTDT_ACT_MSG[] = { 0x68, 0x04, 0x07, 0x00, 0x00, 0x00 };
#define STARTDT_ACT_MSG_SIZE 6

static uint8_t TESTFR_ACT_MSG[] = { 0x68, 0x04, 0x43, 0x00, 0x00, 0x00 };
#define TESTFR_ACT_MSG_SIZE 6

static uint8_t TESTFR_CON_MSG[] = { 0x68, 0x04, 0x83, 0x00, 0x00, 0x00 };
#define TESTFR_CON_MSG_SIZE 6

static uint8_t STOPDT_ACT_MSG[] = { 0x68, 0x04, 0x13, 0x00, 0x00, 0x00 };
#define STOPDT_ACT_MSG_SIZE 6

static uint8_t STARTDT_CON_MSG[] = { 0x68, 0x04, 0x0b, 0x00, 0x00, 0x00 };
#define STARTDT_CON_MSG_SIZE 6

static inline int
writeToSocket(CS104_Connection self, uint8_t* buf, int size)
{
    if (size > 0) {
        if (self->msgsendHandler != NULL)
            self->msgsendHandler(self->msgsendHandlerParameter, buf, size);

    }
#if (CONFIG_CS104_SUPPORT_TLS == 1)
    if (self->tlsSocket)
        return TLSSocket_write(self->tlsSocket, buf, size);
    else
        return Socket_write(self->socket, buf, size);
#else
    return Socket_write(self->socket, buf, size);
#endif
}

static void
prepareSMessage(uint8_t* msg)
{
    msg[0] = 0x68;
    msg[1] = 0x04;
    msg[2] = 0x01;
    msg[3] = 0x00;
}

static void
sendSMessage(CS104_Connection self)
{
    uint8_t* msg = self->sMessage;

    msg [4] = (uint8_t) ((self->receiveCount % 128) * 2);
    msg [5] = (uint8_t) (self->receiveCount / 128);

    writeToSocket(self, msg, 6);

    if(self->msgsendHandler_withExplain != NULL)
    {
        //按规约类型分别赋值解析描述
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = self->receiveCount;
        self->cs104_frame.NS = self->sendCount;
        self->cs104_frame.FT = 3;//frame type帧类型：1-I帧 2-U帧 3-S帧

        self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, msg, 6,"S_frame_explain",self->cs104_frame,NULL);//
    }
}

static int
sendIMessage(CS104_Connection self, Frame frame)
{
    T104Frame_prepareToSend((T104Frame) frame, self->sendCount, self->receiveCount);

    writeToSocket(self, T104Frame_getBuffer(frame), T104Frame_getMsgSize(frame));

    if(self->msgsendHandler_withExplain != NULL)
    {
        CS101_ASDU asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), T104Frame_getBuffer(frame) + 6, T104Frame_getMsgSize(frame) - 6);
        //按规约类型分别赋值解析描述
        if (asdu != NULL)
        {
            self->cs104_frame.TI = CS101_ASDU_getTypeID(asdu);
            self->cs104_frame.COT = CS101_ASDU_getCOT(asdu);
            self->cs104_frame.COT += (CS101_ASDU_isTest(asdu)==true)?0x80:0x00;
            self->cs104_frame.COT += (CS101_ASDU_isNegative(asdu)==true)?0x40:0x00;
            self->cs104_frame.EC = CS101_ASDU_getNumberOfElements(asdu);
            self->cs104_frame.NR = self->receiveCount;
            self->cs104_frame.NS = self->sendCount;
            self->cs104_frame.FT = 1;//frame type帧类型：1-I帧 2-U帧 3-S帧

            self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, T104Frame_getBuffer(frame), T104Frame_getMsgSize(frame),"S_frame_explain",self->cs104_frame,asdu);//
            CS101_ASDU_destroy(asdu);
        }

    }


    self->sendCount = (self->sendCount + 1) % 32768;

    return self->sendCount;
}

static CS104_Connection
createConnection(const char* hostname, int tcpPort)
{
    CS104_Connection self = (CS104_Connection) GLOBAL_MALLOC(sizeof(struct sCS104_Connection));

    if (self != NULL) {
        strncpy(self->hostname, hostname, HOST_NAME_MAX);
        self->tcpPort = tcpPort;
        self->parameters = defaultAPCIParameters;
        self->alParameters = defaultAppLayerParameters;

        self->receivedHandler = NULL;
        self->receivedHandlerParameter = NULL;

        self->connectionHandler = NULL;
        self->connectionHandlerParameter = NULL;

        self->msgreceivedHandler = NULL;
        self->msgreceivedHandlerParameter = NULL;
        self->msgsendHandler = NULL;
        self->msgsendHandlerParameter = NULL;

        //_withExplain
        self->msgreceivedHandler_withExplain = NULL;
        self->msgreceivedHandlerParameter_withExplain = NULL;
        self->msgsendHandler_withExplain = NULL;
        self->msgsendHandlerParameter_withExplain = NULL;
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.VSQ = 0;
        self->cs104_frame.NS = 0;
        self->cs104_frame.NR = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.FT = 0;

        self->importantInfoHandler = NULL;
        self->importantInfoHandlerParameter = NULL;
#if (CONFIG_USE_THREADS == 1)
        self->sentASDUsLock = Semaphore_create(1);
        self->connectionHandlingThread = NULL;
#endif

#if (CONFIG_CS104_SUPPORT_TLS == 1)
        self->tlsConfig = NULL;
        self->tlsSocket = NULL;
#endif

        self->sentASDUs = NULL;

        prepareSMessage(self->sMessage);
    }

    return self;
}

CS104_Connection
CS104_Connection_create(const char* hostname, int tcpPort)
{
    if (tcpPort == -1)
        tcpPort = IEC_60870_5_104_DEFAULT_PORT;

    return createConnection(hostname, tcpPort);
}

#if (CONFIG_CS104_SUPPORT_TLS == 1)
CS104_Connection
CS104_Connection_createSecure(const char* hostname, int tcpPort, TLSConfiguration tlsConfig)
{
    if (tcpPort == -1)
        tcpPort = IEC_60870_5_104_DEFAULT_TLS_PORT;

    CS104_Connection self = createConnection(hostname, tcpPort);

    if (self != NULL) {
        self->tlsConfig = tlsConfig;
        TLSConfiguration_setClientMode(tlsConfig);
    }

    return self;
}
#endif /* (CONFIG_CS104_SUPPORT_TLS == 1) */

static void
resetConnection(CS104_Connection self)
{
    self->connectTimeoutInMs = self->parameters.t0 * 1000;

    self->running = false;
    self->failure = false;
    self->close = false;

    self->receiveCount = 0;
    self->sendCount = 0;

    self->unconfirmedReceivedIMessages = 0;
    self->lastConfirmationTime = 0xffffffffffffffff;
    self->firstIMessageReceived = false;

    self->oldestSentASDU = -1;
    self->newestSentASDU = -1;

    if (self->sentASDUs == NULL) {
        self->maxSentASDUs = self->parameters.k;
        self->sentASDUs = (SentASDU*) GLOBAL_MALLOC(sizeof(SentASDU) * self->maxSentASDUs);
    }

    self->outstandingTestFCConMessages = 0;
    self->uMessageTimeout = 0;
}

static void
resetT3Timeout(CS104_Connection self) {
    self->nextT3Timeout = Hal_getTimeInMs() + (self->parameters.t3 * 1000);
}

static bool
checkSequenceNumber(CS104_Connection self, int seqNo)
{
#if (CONFIG_USE_THREADS == 1)
    Semaphore_wait(self->sentASDUsLock);
#endif

    /* check if received sequence number is valid */

    bool seqNoIsValid = false;
    bool counterOverflowDetected = false;

    if (self->oldestSentASDU == -1) { /* if k-Buffer is empty */
        if (seqNo == self->sendCount)
            seqNoIsValid = true;
    }
    else {
        /* Two cases are required to reflect sequence number overflow */
        if (self->sentASDUs[self->oldestSentASDU].seqNo <= self->sentASDUs[self->newestSentASDU].seqNo) {
            if ((seqNo >= self->sentASDUs[self->oldestSentASDU].seqNo) &&
                (seqNo <= self->sentASDUs[self->newestSentASDU].seqNo))
                seqNoIsValid = true;
        }
        else {
            if ((seqNo >= self->sentASDUs[self->oldestSentASDU].seqNo) ||
                (seqNo <= self->sentASDUs[self->newestSentASDU].seqNo))
                seqNoIsValid = true;

            counterOverflowDetected = true;
        }

        int latestValidSeqNo = (self->sentASDUs[self->oldestSentASDU].seqNo - 1) % 32768;

        if (latestValidSeqNo == seqNo)
            seqNoIsValid = true;
    }

    if (seqNoIsValid) {

        if (self->oldestSentASDU != -1) {

            do {
                if (counterOverflowDetected == false) {
                    if (seqNo < self->sentASDUs [self->oldestSentASDU].seqNo)
                        break;
                }
                else {
                    if (seqNo == ((self->sentASDUs [self->oldestSentASDU].seqNo - 1) % 32768))
                        break;
                }

                self->oldestSentASDU = (self->oldestSentASDU + 1) % self->maxSentASDUs;

                int checkIndex = (self->newestSentASDU + 1) % self->maxSentASDUs;

                if (self->oldestSentASDU == checkIndex) {
                    self->oldestSentASDU = -1;
                    break;
                }

                if (self->sentASDUs [self->oldestSentASDU].seqNo == seqNo)
                    break;
            } while (true);

        }
    }

#if (CONFIG_USE_THREADS == 1)
    Semaphore_post(self->sentASDUsLock);
#endif

    return seqNoIsValid;
}


static bool
isSentBufferFull(CS104_Connection self)
{
    if (self->oldestSentASDU == -1)
        return false;

    int newIndex = (self->newestSentASDU + 1) % self->maxSentASDUs;

    if (newIndex == self->oldestSentASDU)
        return true;
    else
        return false;
}

void
CS104_Connection_close(CS104_Connection self)
{
    self->close = true;
#if (CONFIG_USE_THREADS == 1)
    if (self->connectionHandlingThread)
    {
        Thread_destroy(self->connectionHandlingThread);
        self->connectionHandlingThread = NULL;
    }
#endif
}

void
CS104_Connection_destroy(CS104_Connection self)
{
    CS104_Connection_close(self);

    if (self->sentASDUs != NULL)
        GLOBAL_FREEMEM(self->sentASDUs);

#if (CONFIG_USE_THREADS == 1)
    Semaphore_destroy(self->sentASDUsLock);
#endif

    GLOBAL_FREEMEM(self);
}

void
CS104_Connection_setAPCIParameters(CS104_Connection self, CS104_APCIParameters parameters)
{
    self->parameters = *parameters;

    self->connectTimeoutInMs = self->parameters.t0 * 1000;
}

void
CS104_Connection_setAppLayerParameters(CS104_Connection self, CS101_AppLayerParameters parameters)
{
    self->alParameters = *parameters;
}

CS101_AppLayerParameters
CS104_Connection_getAppLayerParameters(CS104_Connection self)
{
    return &(self->alParameters);
}

void
CS104_Connection_setConnectTimeout(CS104_Connection self, int millies)
{
    self->connectTimeoutInMs = millies;
}

CS104_APCIParameters
CS104_Connection_getAPCIParameters(CS104_Connection self)
{
    return &(self->parameters);
}

#if (CONFIG_CS104_SUPPORT_TLS == 1)
static inline int
receiveMessageTlsSocket(TLSSocket socket, uint8_t* buffer)
{
    int readFirst = TLSSocket_read(socket, buffer, 1);

    if (readFirst < 1)
        return readFirst;

    if (buffer[0] != 0x68)
        return -1; /* message error */

    if (TLSSocket_read(socket, buffer + 1, 1) != 1)
        return -1;

    int length = buffer[1];

    /* read remaining frame */
    if (TLSSocket_read(socket, buffer + 2, length) != length)
        return -1;

    return length + 2;
}
#endif /*  (CONFIG_CS104_SUPPORT_TLS == 1) */

static inline int
receiveMessageSocket(Socket socket, uint8_t* buffer)
{
    int readFirst = Socket_read(socket, buffer, 1);

    if (readFirst < 1)
    {
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"Socket_read(socket, buffer, 1) return value < 1,value is "<<readFirst;
#else
        syslog(LOG_WARNING,"LIB60870:Socket_read(socket, buffer, 1) return value < 1,value is %d",readFirst);
#endif
        return readFirst;
    }

    if (buffer[0] != 0x68)
    {
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"message error,buffer[0] != 0x68,buffer[0] =="<<buffer[0];
#endif

        return -1; /* message error */
    }

    if (Socket_read(socket, buffer + 1, 1) != 1)
    {
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"Socket_read(socket, buffer + 1, 1) != 1";
#endif
        return -1;
    }

    int length = buffer[1];

    /* read remaining frame */
    if (Socket_read(socket, buffer + 2, length) != length)
    {
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"message error,Socket_read(socket, buffer + 2, length) != buffer[1]";
#endif
        return -1;
    }

    return length + 2;
}

static int
receiveMessage(CS104_Connection self, uint8_t* buffer)
{
#if (CONFIG_CS104_SUPPORT_TLS == 1)
    if (self->tlsSocket != NULL)
        return receiveMessageTlsSocket(self->tlsSocket, buffer);
    else
        return receiveMessageSocket(self->socket, buffer);
#else
    return receiveMessageSocket(self->socket, buffer);
#endif
}


static bool
checkConfirmTimeout(CS104_Connection self, uint64_t currentTime)//long
{
    if ((currentTime - self->lastConfirmationTime) >= (uint32_t) (self->parameters.t2 * 1000))
        return true;
    else
        return false;
}

static bool
checkMessage(CS104_Connection self, uint8_t* buffer, int msgSize)
{

    if (msgSize > 0) {
        if (self->msgreceivedHandler != NULL)
            self->msgreceivedHandler(self->msgreceivedHandlerParameter, buffer, msgSize);

    }

    if ((buffer[2] & 1) == 0) { /* I format frame */

        if (self->firstIMessageReceived == false) {
            self->firstIMessageReceived = true;
            self->lastConfirmationTime = Hal_getTimeInMs(); /* start timeout T2 */
        }

        if (msgSize < 7) {
            DEBUG_PRINT("checkMessage return false,I msg too small!");//\n
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "checkMessage return false,I msg too small!");
            return false;
        }

        int frameSendSequenceNumber = ((buffer [3] * 0x100) + (buffer [2] & 0xfe)) / 2;
        int frameRecvSequenceNumber = ((buffer [5] * 0x100) + (buffer [4] & 0xfe)) / 2;

        //DEBUG_PRINT("Received I frame: N(S) = %i N(R) = %i\n", frameSendSequenceNumber, frameRecvSequenceNumber);
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870: "<<"Received I frame: N(S) = "<<frameSendSequenceNumber<<" N(R) = "<<frameRecvSequenceNumber;
#endif
        /* check the receive sequence number N(R) - connection will be closed on an unexpected value */
        if (frameSendSequenceNumber != self->receiveCount) {
            DEBUG_PRINT("checkMessage return false(I format frame),frameSendSequenceNumber error: (Shoud) Close connection!");
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "checkMessage return false(I format frame),Sequence error: (Shoud) Close Connection!");//
            //return false;
        }

        if (checkSequenceNumber(self, frameRecvSequenceNumber) == false)
        {
            DEBUG_PRINT("checkSequenceNumber(self, frameRecvSequenceNumber) (Shoud) return false!");
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "checkSequenceNumber(self, frameRecvSequenceNumber) (Shoud) return false!");
            //return false;
        }

        self->receiveCount = (self->receiveCount + 1) % 32768;
        self->unconfirmedReceivedIMessages++;

        CS101_ASDU asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), buffer + 6, msgSize - 6);

        if (asdu != NULL) {
            if (self->receivedHandler != NULL)
                self->receivedHandler(self->receivedHandlerParameter, -1, asdu);


            self->cs104_frame.TI = CS101_ASDU_getTypeID(asdu);
            self->cs104_frame.COT = CS101_ASDU_getCOT(asdu);
            self->cs104_frame.COT += (CS101_ASDU_isTest(asdu)==true)?0x80:0x00;
            self->cs104_frame.COT += (CS101_ASDU_isNegative(asdu)==true)?0x40:0x00;
            self->cs104_frame.EC = CS101_ASDU_getNumberOfElements(asdu);
            self->cs104_frame.NR = frameRecvSequenceNumber;
            self->cs104_frame.NS = frameSendSequenceNumber;
            self->cs104_frame.FT = 1;//frame type帧类型：1-I帧 2-U帧 3-S帧
            if (self->msgreceivedHandler_withExplain != NULL)
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"I_frame_explain",self->cs104_frame,asdu);

            CS101_ASDU_destroy(asdu);
        }
        else
        {
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "CS101_ASDU == NULL,return false!");
            return false;
        }

    }
    else if ((buffer[2] & 0x03) == 0x03) { /* U format frame */

        //DEBUG_PRINT("Received U frame");//\n
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = 0;
        self->cs104_frame.NS = 0;
        self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧


        self->uMessageTimeout = 0;

        if (buffer[2] == 0x43) { /* Check for TESTFR_ACT message */
//            DEBUG_PRINT("Send TESTFR_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Send TESTFR_CON",self->cs104_frame,NULL);
//            //qDebug()<<"DEBUG_LIB60870:"<<"(for printtest)Send TESTFR_CON\n";
//            if (self->importantInfoHandler != NULL)
//                self->importantInfoHandler(self->importantInfoHandlerParameter, "Send TESTFR_CON!");//
            writeToSocket(self, TESTFR_CON_MSG, TESTFR_CON_MSG_SIZE);

            if(self->msgsendHandler_withExplain != NULL)
            {
                self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, buffer, 6,"Send TESTFR_CON",self->cs104_frame,NULL);//
            }
        }
        else if (buffer[2] == 0x83) { /* TESTFR_CON */
//            DEBUG_PRINT("Rcvd TESTFR_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Rcvd TESTFR_CON",self->cs104_frame,NULL);
            //qDebug()<<"DEBUG_LIB60870:"<<"(for printtest)Rcvd TESTFR_CON\n";

            self->outstandingTestFCConMessages = 0;
        }
        else if (buffer[2] == 0x07) { /* STARTDT_ACT */
            //DEBUG_PRINT("Send STARTDT_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Send STARTDT_CON",self->cs104_frame,NULL);
            self->receiveCount = 0;//clear seqnum
            self->sendCount = 0;
            self->unconfirmedReceivedIMessages = 0;
            writeToSocket(self, STARTDT_CON_MSG, STARTDT_CON_MSG_SIZE);

            if(self->msgsendHandler_withExplain != NULL)
            {
                self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, buffer, 6,"Send STARTDT_CON",self->cs104_frame,NULL);//
            }
        }
        else if (buffer[2] == 0x0b) { /* STARTDT_CON */
            //DEBUG_PRINT("Received STARTDT_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Rcvd STARTDT_CON",self->cs104_frame,NULL);
            self->receiveCount = 0;//clear seqnum
            self->sendCount = 0;
            self->unconfirmedReceivedIMessages = 0;
//            if (self->importantInfoHandler != NULL)
//                self->importantInfoHandler(self->importantInfoHandlerParameter, "Received STARTDT_CON(68040b000000)!");//
            if (self->connectionHandler != NULL)
                self->connectionHandler(self->connectionHandlerParameter, self, CS104_CONNECTION_STARTDT_CON_RECEIVED);
        }
        else if (buffer[2] == 0x23) { /* STOPDT_CON */
            //DEBUG_PRINT("Received STOPDT_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Rcvd STOPDT_CON",self->cs104_frame,NULL);

            if (self->connectionHandler != NULL)
                self->connectionHandler(self->connectionHandlerParameter, self, CS104_CONNECTION_STOPDT_CON_RECEIVED);
        }

    }
    else if (buffer [2] == 0x01) { /* S-message */
        int seqNo = (buffer[4] + buffer[5] * 0x100) / 2;
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = seqNo;
        self->cs104_frame.NS = self->sendCount;
        self->cs104_frame.FT = 3;//frame type帧类型：1-I帧 2-U帧 3-S帧
        if (self->msgreceivedHandler_withExplain != NULL)
            self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"S_frame_explain",self->cs104_frame,NULL);

        //DEBUG_PRINT("Rcvd S(%i) (own sendcounter = %i)\n", seqNo, self->sendCount);
        //qDebug()<<"DEBUG_LIB60870: "<<"Rcvd S("<<seqNo<<") (own sendcounter = "<<self->sendCount<<")";

        if (checkSequenceNumber(self, seqNo) == false)
        {
            //DEBUG_PRINT("checkMessage return false(S-message),Sequence error: Close connection!");
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "checkMessage return false(S-message),Sequence error: Close connection!");
            return false;
        }
    }


    resetT3Timeout(self);

    return true;
}

static bool
handleTimeouts(CS104_Connection self)
{
    bool retVal = true;

    uint64_t currentTime = Hal_getTimeInMs();

    if (currentTime > self->nextT3Timeout) {

        if (self->outstandingTestFCConMessages > 2) {//测试帧重发两次未回复，断开链路
            //DEBUG_PRINT("Timeout for TESTFR_CON message");//\n

            /* close connection */
            retVal = false;
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "(handleTimeouts)Timeout for TESTFR_CON message: Close connection!");
            goto exit_function;
        }
        else {
            //DEBUG_PRINT("U message T3 timeout");//\n 长期空闲状态下发送测试帧的超时

            writeToSocket(self, TESTFR_ACT_MSG, TESTFR_ACT_MSG_SIZE);
            if(self->msgsendHandler_withExplain != NULL)
            {
                self->cs104_frame.TI = 0;
                self->cs104_frame.COT = 0;
                self->cs104_frame.EC = 0;
                self->cs104_frame.NR = 0;
                self->cs104_frame.NS = 0;
                self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧
                self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, TESTFR_ACT_MSG, 6,"Send TESTFR_ASK(U message T3 timeout)",self->cs104_frame,NULL);//
            }
            self->uMessageTimeout = currentTime + (self->parameters.t1 * 1000);
            self->outstandingTestFCConMessages++;

            resetT3Timeout(self);
        }
    }

    //t2  10s  无数据报文时确认的超时，t2<t1  ==========
//    if (self->unconfirmedReceivedIMessages > 0) {

//        if (checkConfirmTimeout(self, currentTime)) {

//            self->lastConfirmationTime = currentTime;
//            self->unconfirmedReceivedIMessages = 0;

//            sendSMessage(self); /* send confirmation message */
//        }
//    }


    if (self->uMessageTimeout != 0) {
        if (currentTime > self->uMessageTimeout) {
            DEBUG_PRINT("U message T1 timeout");//\n
            //retVal = false;
            //goto exit_function;
        }
    }

    /* check if counterpart confirmed I messages */
    //t1  15s  发送或测试 APDU 的超时  ==========
#if (CONFIG_USE_THREADS == 1)
    Semaphore_wait(self->sentASDUsLock);
#endif
    if (self->oldestSentASDU != -1) {
        if ((currentTime - self->sentASDUs[self->oldestSentASDU].sentTime) >= (uint64_t) (self->parameters.t1 * 1000)) {
            //DEBUG_PRINT("I message timeout,self->parameters.t1 = %d (currentTime - self->sentASDUs[%d].sentTime = %d)\n",self->parameters.t1,self->oldestSentASDU,(currentTime - self->sentASDUs[self->oldestSentASDU].sentTime));//\n
            //retVal = false;
        }
    }
#if (CONFIG_USE_THREADS == 1)
    Semaphore_post(self->sentASDUsLock);
#endif


exit_function:

    return retVal;
}

static void*
handleConnection(void* parameter)
{
    CS104_Connection self = (CS104_Connection) parameter;

    resetConnection(self);

    self->socket = TcpSocket_create();

    Socket_setConnectTimeout(self->socket, self->connectTimeoutInMs);

    if (Socket_connect(self->socket, self->hostname, self->tcpPort)) {

#if (CONFIG_CS104_SUPPORT_TLS == 1)
        if (self->tlsConfig != NULL) {
            self->tlsSocket = TLSSocket_create(self->socket, self->tlsConfig);

            if (self->tlsSocket)
                self->running = true;
            else
                self->failure = true;
        }
        else
            self->running = true;
#else
        self->running = true;
#endif

        if (self->running) {

            /* Call connection handler */
            if (self->connectionHandler != NULL)
                self->connectionHandler(self->connectionHandlerParameter, self, CS104_CONNECTION_OPENED);

            HandleSet handleSet = Handleset_new();

            bool loopRunning = true;

            while (loopRunning) {

                uint8_t buffer[300];

                Handleset_reset(handleSet);
                Handleset_addSocket(handleSet, self->socket);

                if (Handleset_waitReady(handleSet, 100)) {
                    int bytesRec = receiveMessage(self, buffer);

                    if (bytesRec == -1) {
                        loopRunning = false;
                        self->failure = true;//perror();strerror(errno);
                        if (self->importantInfoHandler != NULL)
                        {
                            self->importantInfoHandler(self->importantInfoHandlerParameter, "loopRunning is set false,receiveMessage bytesRec == -1,strerror(errno):");
                            self->importantInfoHandler(self->importantInfoHandlerParameter, strerror(errno));
                        }
                    }

                    if (bytesRec > 0) {

                        if (checkMessage(self, buffer, bytesRec) == false) {
                            /* close connection on error */
                            loopRunning= false;
                            DEBUG_PRINT("loopRunning is set false,checkMessage(self, buffer, bytesRec) is false");//\n
                            if (self->importantInfoHandler != NULL)
                                self->importantInfoHandler(self->importantInfoHandlerParameter, "loopRunning is set false,checkMessage(self, buffer, bytesRec) return false!");
                            self->failure = true;
                        }
                    }

                    if (self->unconfirmedReceivedIMessages >= self->parameters.w) {
                        self->lastConfirmationTime = Hal_getTimeInMs();
                        self->unconfirmedReceivedIMessages = 0;
                        sendSMessage(self);
                    }
                }

                if (handleTimeouts(self) == false)
                {
                    loopRunning = false;
                    DEBUG_PRINT("loopRunning is set false,handleTimeouts(self) is false");//\n
                    if (self->importantInfoHandler != NULL)
                        self->importantInfoHandler(self->importantInfoHandlerParameter, "loopRunning is set false,handleTimeouts(self) return false!");
                }

                if (self->close)
                {
                    loopRunning = false;
                    DEBUG_PRINT("loopRunning is set false,self->close is true");//\n
                    if (self->importantInfoHandler != NULL)
                        self->importantInfoHandler(self->importantInfoHandlerParameter, "loopRunning is set false,self->close is true!");
                }
            }

            Handleset_destroy(handleSet);

            /* Call connection handler */
            if (self->connectionHandler != NULL)
            {
                self->connectionHandler(self->connectionHandlerParameter, self, CS104_CONNECTION_CLOSED);
            }

        }
    }
    else {
        self->failure = true;
    }

#if (CONFIG_CS104_SUPPORT_TLS == 1)
    if (self->tlsSocket)
        TLSSocket_close(self->tlsSocket);
#endif

    Socket_destroy(self->socket);

    DEBUG_PRINT("EXIT CONNECTION HANDLING THREAD!!!");//\n
    if (self->importantInfoHandler != NULL)
        self->importantInfoHandler(self->importantInfoHandlerParameter, "EXIT CONNECTION HANDLING THREAD!");

    self->running = false;

    return NULL;
}

void
CS104_Connection_connectAsync(CS104_Connection self)
{
    resetConnection(self);

    resetT3Timeout(self);

    self->connectionHandlingThread = Thread_create(handleConnection, (void*) self, false);

    if (self->connectionHandlingThread)
        Thread_start(self->connectionHandlingThread);
}

bool
CS104_Connection_connect(CS104_Connection self)
{
    CS104_Connection_connectAsync(self);

    while ((self->running == false) && (self->failure == false))
        Thread_sleep(1);

    return self->running;
}

bool
CS104_Connection_station(CS104_Connection self)
{
    return self->running;
}

void
CS104_Connection_setASDUReceivedHandler(CS104_Connection self, CS101_ASDUReceivedHandler handler, void* parameter)
{
    self->receivedHandler = handler;
    self->receivedHandlerParameter = parameter;
}

void
CS104_Connection_setMSGReceivedHandler(CS104_Connection self, CS104_MSGReceivedHandler handler, void* parameter)
{
    self->msgreceivedHandler = handler;
    self->msgreceivedHandlerParameter = parameter;
}

void
CS104_Connection_setMSGSendHandler(CS104_Connection self, CS104_MSGSendHandler handler, void* parameter)
{
    self->msgsendHandler = handler;
    self->msgsendHandlerParameter = parameter;
}

void
CS104_Connection_setMSGReceivedHandler_withExplain(CS104_Connection self, CS104_MSGReceivedHandler_withExplain handler, void* parameter)
{
    self->msgreceivedHandler_withExplain = handler;
    self->msgreceivedHandlerParameter_withExplain = parameter;
}

void
CS104_Connection_setMSGSendHandler_withExplain(CS104_Connection self, CS104_MSGSendHandler_withExplain handler, void* parameter)
{
    self->msgsendHandler_withExplain = handler;
    self->msgsendHandlerParameter_withExplain = parameter;
}

void
CS104_Connection_setImportantInfoHandler(CS104_Connection self, CS104_ImportantInfoHandler handler, void* parameter)
{
    self->importantInfoHandler = handler;
    self->importantInfoHandlerParameter = parameter;
}

void
CS104_Connection_setConnectionHandler(CS104_Connection self, CS104_ConnectionHandler handler, void* parameter)
{
    self->connectionHandler = handler;
    self->connectionHandlerParameter = parameter;
}

static void
encodeIdentificationField(CS104_Connection self, Frame frame, TypeID typeId,
        int vsq, CS101_CauseOfTransmission cot, int ca)
{
    T104Frame_setNextByte(frame, typeId);
    T104Frame_setNextByte(frame, (uint8_t) vsq);

    /* encode COT */
    T104Frame_setNextByte(frame, (uint8_t) cot);
    if (self->alParameters.sizeOfCOT == 2)
        T104Frame_setNextByte(frame, (uint8_t) self->alParameters.originatorAddress);

    /* encode CA */
    T104Frame_setNextByte(frame, (uint8_t)(ca & 0xff));
    if (self->alParameters.sizeOfCA == 2)
        T104Frame_setNextByte(frame, (uint8_t) ((ca & 0xff00) >> 8));
}

static void
encodeIOA(CS104_Connection self, Frame frame, int ioa)
{
    T104Frame_setNextByte(frame, (uint8_t) (ioa & 0xff));

    if (self->alParameters.sizeOfIOA > 1)
        T104Frame_setNextByte(frame, (uint8_t) ((ioa / 0x100) & 0xff));

    if (self->alParameters.sizeOfIOA > 1)
        T104Frame_setNextByte(frame, (uint8_t) ((ioa / 0x10000) & 0xff));
}

void
CS104_Connection_sendStartDT(CS104_Connection self)
{
    writeToSocket(self, STARTDT_ACT_MSG, STARTDT_ACT_MSG_SIZE);
    if(self->msgsendHandler_withExplain != NULL)
    {
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = 0;
        self->cs104_frame.NS = 0;
        self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧
        self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, STARTDT_ACT_MSG, 6,"Send STARTDT_ACT",self->cs104_frame,NULL);//
    }
}

void
CS104_Connection_sendStopDT(CS104_Connection self)
{
    writeToSocket(self, STOPDT_ACT_MSG, STOPDT_ACT_MSG_SIZE);
    if(self->msgsendHandler_withExplain != NULL)
    {
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = 0;
        self->cs104_frame.NS = 0;
        self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧
        self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, STOPDT_ACT_MSG, 6,"Send STOPDT_ACT",self->cs104_frame,NULL);//
    }
}

static void
sendIMessageAndUpdateSentASDUs(CS104_Connection self, Frame frame)
{
#if (CONFIG_USE_THREADS == 1)
    Semaphore_wait(self->sentASDUsLock);
#endif

    int currentIndex = 0;

    if (self->oldestSentASDU == -1) {
        self->oldestSentASDU = 0;
        self->newestSentASDU = 0;

    } else {
        currentIndex = (self->newestSentASDU + 1) % self->maxSentASDUs;
    }

    self->sentASDUs [currentIndex].seqNo = sendIMessage (self, frame);
    self->sentASDUs [currentIndex].sentTime = Hal_getTimeInMs();

    self->newestSentASDU = currentIndex;

#if (CONFIG_USE_THREADS == 1)
    Semaphore_post(self->sentASDUsLock);
#endif
}

static bool
sendASDUInternal(CS104_Connection self, Frame frame)
{
    bool retVal = false;

    if (self->running) {
        if (isSentBufferFull(self) == false) {
            sendIMessageAndUpdateSentASDUs(self, frame);
            retVal = true;
        }
    }

    T104Frame_destroy(frame);

    return retVal;
}

//站总召唤   C_CI_NA_1=100
/*
68H              1 字节
报文长度 L         1 字节
控制域 C          4 字节
类型标识符 TI      1 字节
可变帧长限定词 VSQ  1 字节
传送原因 COT       2 字节
ASDU 公共地址      2 字节
信息对象地址（=0）   3 字节
召唤限定词 QOI     1 字节
*/
bool
CS104_Connection_sendInterrogationCommand(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, QualifierOfInterrogation qoi)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_IC_NA_1, 1, cot, ca);

    encodeIOA(self, frame, 0);

    /* encode QOI (7.2.6.22) */
    T104Frame_setNextByte(frame, qoi); /* 20 = station interrogation *///召唤限定词 QOI:<20>  总召唤

    return sendASDUInternal(self, frame);
}

//<101>∶＝电能量召唤命令   C_CI_NA_1=101
bool
CS104_Connection_sendCounterInterrogationCommand(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, uint8_t qcc)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_CI_NA_1, 1, cot, ca);

    encodeIOA(self, frame, 0);

    /* encode QCC */
    T104Frame_setNextByte(frame, qcc);

    return sendASDUInternal(self, frame);
}

//   C_RD_NA_1=102
bool
CS104_Connection_sendReadCommand(CS104_Connection self, int ca, int ioa)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_RD_NA_1, 1, CS101_COT_REQUEST, ca);

    encodeIOA(self, frame, ioa);

    return sendASDUInternal(self, frame);
}

//时钟命令  C_CS_NA_1=103
/*
68H  1 字节
报文长度 L  1 字节
控制域 C  4 字节
类型标识符 TI  1 字节
可变帧长限定词 VSQ  1 字节
传送原因 COT  2 字节
ASDU 公共地址  2 字节
信息对象地址（=0）  3 字节
时标 CP56Time2a  7 字节
 */
bool
CS104_Connection_sendClockSyncCommand(CS104_Connection self, int ca, CP56Time2a time)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_CS_NA_1, 1, CS101_COT_ACTIVATION, ca);

    encodeIOA(self, frame, 0);

    T104Frame_appendBytes(frame, CP56Time2a_getEncodedValue(time), 7);

    return sendASDUInternal(self, frame);
}

bool //selfdefined
CS104_Connection_sendClockSyncCommand_SetandRead(CS104_Connection self, int ca, int cot,unsigned char* time)
{
    Frame frame = (Frame) T104Frame_create();

    if(cot==5)
        encodeIdentificationField(self, frame, C_CS_NA_1, 1, CS101_COT_REQUEST, ca);//5  CS101_COT_REQUEST
    else if(cot==6)
        encodeIdentificationField(self, frame, C_CS_NA_1, 1, CS101_COT_ACTIVATION, ca);//6  CS101_COT_ACTIVATION


    encodeIOA(self, frame, 0);

    T104Frame_appendBytes(frame, (uint8_t*)time, 7);
    //T104Frame_setNextByte(time[0]);

    return sendASDUInternal(self, frame);
}

//测试命令   C_TS_NA_1=104

bool
CS104_Connection_sendTestCommand(CS104_Connection self, int ca)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_TS_NA_1, 1, CS101_COT_ACTIVATION, ca);

    encodeIOA(self, frame, 0);

    T104Frame_setNextByte(frame, 0xcc);
    T104Frame_setNextByte(frame, 0x55);

    return sendASDUInternal(self, frame);
}

//复位进程命令  C_RP_NA_1 typeID: 105
/*
68H  1 字节
报文长度 L  1 字节
控制域 C  4 字节
类型标识符 TI  1 字节
可变帧长限定词 VSQ  1 字节
传送原因 COT  2 字节
ASDU 公共地址  2 字节
信息对象地址（=0）  3 字节
复位进程命令限定词 QRP  1 字节*/
bool
CS104_Connection_sendResetprocessCommand(CS104_Connection self, int ca, int qrp)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_TS_NA_1, 1, CS101_COT_ACTIVATION, ca);

    encodeIOA(self, frame, 0);

    T104Frame_setNextByte(frame, qrp);

    return sendASDUInternal(self, frame);
}

//控制命令   //<45>∶＝单点命令C_SC_NA_1,//<46>∶＝双点命令C_DC_NA_1
/*
 * 当信息对象序列（SQ=0）：
68H  1 字节
报文长度 L  1 字节
控制域 C  4 字节
类型标识符 TI  1 字节
可变帧长限定词 VSQ  1 字节
传输原因 COT  2 字节
ASDU 公共地址  2 字节
遥控信息对象地址  3 字节
单命令 SCO/双命令 DCO  1 字节
*/
bool
CS104_Connection_sendProcessCommand(CS104_Connection self, TypeID typeId, CS101_CauseOfTransmission cot, int ca, InformationObject sc)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField (self, frame, typeId, 1 /* SQ:false; NumIX:1 */, cot, ca);

    InformationObject_encode(sc, frame, (CS101_AppLayerParameters) &(self->alParameters), false);

    return sendASDUInternal(self, frame);
}

//200-203 远程参数读写
bool
CS104_Connection_sendParamValueRead(CS104_Connection self, CS101_CauseOfTransmission cot, int vsq,int ca, int sn,InformationObject sc)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_RS_NA_1, vsq, cot, ca);

    //定值区号SN
    Frame_setNextByte(frame, (uint8_t) (sn & 0xff));
    Frame_setNextByte(frame, (uint8_t) ((sn / 0x100) & 0xff));

    if(vsq != 0)//读全部时，vsq=0;读多个时，vsq=对应信息元素个数
        InformationObject_encode(sc, frame, (CS101_AppLayerParameters) &(self->alParameters), false);

    return sendASDUInternal(self, frame);
}

bool
CS104_Connection_sendParamValueWrite(CS104_Connection self, CS101_CauseOfTransmission cot, int vsq,int ca, int sn,int paramti, InformationObject sc)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_WS_NA_1, vsq, cot, ca);

    //定值区号SN
    Frame_setNextByte(frame, (uint8_t) (sn & 0xff));
    Frame_setNextByte(frame, (uint8_t) ((sn / 0x100) & 0xff));

    //参数特征标识  1 字节
    Frame_setNextByte(frame, (uint8_t)paramti);

    InformationObject_encode(sc, frame, (CS101_AppLayerParameters) &(self->alParameters), false);

    return sendASDUInternal(self, frame);
}


//========== <210>	：文件服务 ==========//
//Fileserving文件服务   F_FR_NA_1=210
//===7.6.3.1 召唤文目录服务报文;7.6.3.2 读文件服务报文;7.6.3.3 写文件服务报文
//文件目录召唤--->确认
//读文件激活--->确认-->传输
//读文件传输确认
//
//
bool
CS104_Connection_sendFileservingCommand(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, InformationObject sc)
{
    Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, F_FR_NA_1, 1, cot, ca);

    InformationObject_encode(sc, frame, (CS101_AppLayerParameters) &(self->alParameters), false);

    return sendASDUInternal(self, frame);
}
//========== <210>	：文件服务 ==========//

//========== 自定义报文组包 ==========//
bool CS104_Connection_sendSelfdefinedFrame(CS104_Connection self,unsigned char* buf,int len)
{
    if(len <= 0)
    {
        return false;
    }

    //writeToSocket(CS104_Connection self, uint8_t* buf, int size)
    int rtn = writeToSocket(self, (uint8_t*)buf, len);

    if(rtn < len)
    {
        return false;
    }

    return true;
}
//========== 自定义报文组包 ==========//

bool
CS104_Connection_sendASDU(CS104_Connection self, CS101_ASDU asdu)
{
    Frame frame = (Frame) T104Frame_create();

    CS101_ASDU_encode(asdu, frame);

    return sendASDUInternal(self, frame);
}

bool
CS104_Connection_isTransmitBufferFull(CS104_Connection self)
{
    return isSentBufferFull(self);
}


