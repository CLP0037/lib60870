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

#include "linked_list.h"

#define CS104_DEFAULT_PORT 2404
#define CS104_DEFAULT_PORT_104P 2402  //104+ default port:2402

struct sCS104_APCIParameters defaultAPCIParameters = {
		/* .k = */ 12,
        /* .w = */ 1,
		/* .t0 = */ 10,
		/* .t1 = */ 15,
		/* .t2 = */ 10,
        /* .t3 = */ 10
};//old default value: w=8; t3=20

struct sCS104_APCIParameters defaultAPCIParameters_sDEV = {
        /* .k = */ 12,
        /* .w = */ 8,
        /* .t0 = */ 30,
        /* .t1 = */ 15,
        /* .t2 = */ 10,
        /* .t3 = */ 20
};

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
    int unconfirmedSendIMessages;//记录设备I帧未确认帧数

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

    CS104_Connection_mStation server_mStation;
    bool isActive;

    //===============  104 connection : as server and as master station ==============//
    int connection_index;//
    //报文收发
    CS104_MSGReceivedHandler_mStation msgreceivedHandler_mStation;
    void* msgreceivedHandlerParameter_mStation;

    CS104_MSGSendHandler_mStation msgsendHandler_mStation;
    void* msgsendHandlerParameter_mStation;
    //===============  104 connection : as server and as master station ==============//


    //===============  104 connection : as client and as slave device(simulation equipment) ==============//
    CS101_InterrogationHandler interrogationHandler;
    void* interrogationHandlerParameter;

    CS101_CounterInterrogationHandler counterInterrogationHandler;
    void* counterInterrogationHandlerParameter;

    CS101_ReadHandler readHandler;
    void* readHandlerParameter;

    CS101_ClockSynchronizationHandler clockSyncHandler;
    void* clockSyncHandlerParameter;

    CS101_ResetProcessHandler resetProcessHandler;
    void* resetProcessHandlerParameter;

    CS101_DelayAcquisitionHandler delayAcquisitionHandler;
    void* delayAcquisitionHandlerParameter;

    CS101_ASDUHandler asduHandler;
    void* asduHandlerParameter;

#if (CONFIG_USE_THREADS == 1)
    Semaphore sentASDUsLock_sDEV;
    //Thread connectionHandlingThread_sDEV;
#endif

    struct sIMasterConnection iMasterConnection;

    //===============  104 connection : as client and as slave device(simulation equipment) ==============//


    unsigned char connectionType;//通讯方式：1-网络通讯（客户端） 2-网络通讯（服务端的子连接）  3-串口通讯
    //===============  104 connection :  SerialPort ==============//
    SerialPort commPort;
    bool isCommOpend;
    int messageTimeout;
    int characterTimeout;
    //Thread commThread;

    //===============  104 connection :  SerialPort ==============//

    //===============  104+ connection  ==============//
    unsigned char baseProtocalType;//基本规约类型：1-104   2-104+
    uint8_t sMessage_104P[7];
    int shortframesize;//短帧长度/长帧基准长度
    //===============  104+ connection  ==============//

}connectionHandler;

//===============  104 connection : as server and as master station ==============//
typedef enum {
    CS104_MODE_SINGLE_REDUNDANCY_GROUP,
    CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP
} CS104_Connection_ServerMode;

struct sCS104_Connection_mStation {

    CS104_ConnectionRequestHandler_mStation connectionRequestHandler_mStation;
    void* connectionRequestHandlerParameter_mStation;

    CS104_ConnectionBrokenHandler_mStation connectionBrokenHandler_mStation;
    void* connectionBrokenHandlerParameter_mStation;

#if (CONFIG_CS104_SUPPORT_TLS == 1)
    TLSConfiguration tlsConfig;
#endif

//#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP)
//    MessageQueue asduQueue; /**< low priority ASDU queue and buffer */
//    HighPriorityASDUQueue connectionAsduQueue; /**< high priority ASDU queue */
//#endif

#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP)
    int maxLowPrioQueueSize;
    int maxHighPrioQueueSize;
#endif

    int openConnections; /**< number of connected clients */   //客户端连接数
    //LinkedList masterConnections; /**< references to all MasterConnection objects */
    LinkedList masterConnections_mStation;
#if (CONFIG_USE_THREADS == 1)
    Semaphore openConnectionsLock;
#endif

    int maxOpenConnections; /**< maximum accepted open client connections */   //最大客户端连接数

    CS104_Connection_ServerMode serverMode;

    bool isStarting;
    bool isRunning;
    bool stopRunning;

    int tcpPort;

//    CS104_ServerMode serverMode;

    char* localAddress;
    Thread listeningThread;

    CS104_Connection connections_mStation[1024];
    int history_index; // history index total
    char* connections_ip[1024];
    int connections_port[1024];

};



//===============  104 connection : as server and as master station ==============//

//===============  104 connection : as client and as slave device(simulation equipment) ==============//
struct sCS104_Connection_sDEV{



};

static uint8_t STOPDT_CON_MSG[] = { 0x68, 0x04, 0x23, 0x00, 0x00, 0x00 };
#define STOPDT_CON_MSG_SIZE 6
//===============  104 connection : as client and as slave device(simulation equipment) ==============//

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

//===============  104+(104P: 104 plus) connection ==============//
static uint8_t STARTDT_ACT_MSG_104P[] = { 0x68, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00 };
#define STARTDT_ACT_MSG_SIZE_104P 7

static uint8_t TESTFR_ACT_MSG_104P[] = { 0x68, 0x04, 0x00, 0x43, 0x00, 0x00, 0x00 };
#define TESTFR_ACT_MSG_SIZE_104P 7

static uint8_t TESTFR_CON_MSG_104P[] = { 0x68, 0x04, 0x00, 0x83, 0x00, 0x00, 0x00 };
#define TESTFR_CON_MSG_SIZE_104P 7

static uint8_t STOPDT_ACT_MSG_104P[] = { 0x68, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00 };
#define STOPDT_ACT_MSG_SIZE_104P 7

static uint8_t STARTDT_CON_MSG_104P[] = { 0x68, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00 };
#define STARTDT_CON_MSG_SIZE_104P 7

static uint8_t STOPDT_CON_MSG_104P[] = { 0x68, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00 };
#define STOPDT_CON_MSG_SIZE_104P 7
//===============  104+ connection ==============//

static inline int
writeToSocket(CS104_Connection self, uint8_t* buf, int size)
{
    if (size > 0) {
        if (self->msgsendHandler != NULL)
            self->msgsendHandler(self->msgsendHandlerParameter, buf, size);
        if (self->msgsendHandler_mStation != NULL)
            self->msgsendHandler_mStation(self->msgsendHandlerParameter_mStation, buf, size,self->hostname,self->tcpPort);
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
prepareSMessage_104P(uint8_t* msg)
{
    msg[0] = 0x68;
    msg[1] = 0x04;
    msg[2] = 0x00;
    msg[3] = 0x01;
    msg[4] = 0x00;
}

static void
sendSMessage(CS104_Connection self)
{
    uint8_t* msg;
    int msgsize;
    if(self->baseProtocalType == 2)
    {
        msg = self->sMessage_104P;
        msgsize = 7;
        msg [5] = (uint8_t) ((self->receiveCount % 128) * 2);
        msg [6] = (uint8_t) (self->receiveCount / 128);
    }
    else
    {
        msg = self->sMessage;
        msgsize = 6;
        msg [4] = (uint8_t) ((self->receiveCount % 128) * 2);
        msg [5] = (uint8_t) (self->receiveCount / 128);
    }

//    uint8_t* msg = self->sMessage;

//    msg [4] = (uint8_t) ((self->receiveCount % 128) * 2);
//    msg [5] = (uint8_t) (self->receiveCount / 128);

    if(self->connectionType == 3)//串口
    {
        SerialPort_write(self->commPort, msg, 0, msgsize);//6
    }
    else
        writeToSocket(self, msg, msgsize);//6

    if(self->msgsendHandler_withExplain != NULL)//,Hal_getTimeInMs()
    {
        //按规约类型分别赋值解析描述
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = self->receiveCount;
        self->cs104_frame.NS = self->sendCount;
        self->cs104_frame.FT = 3;//frame type帧类型：1-I帧 2-U帧 3-S帧
        //6
        self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, msg, msgsize,"S_frame_explain",self->cs104_frame,NULL);//,Hal_getTimeInMs()
    }
}

static int
sendIMessage(CS104_Connection self, Frame frame)
{
    self->unconfirmedSendIMessages ++;
    //T104Frame_prepareToSend((T104Frame) frame, self->sendCount, self->receiveCount);
    T104Frame_prepareToSend_104P((T104Frame) frame, self->sendCount, self->receiveCount, self->baseProtocalType);


    if(self->connectionType == 3)//串口
    {
        SerialPort_write(self->commPort, T104Frame_getBuffer(frame), 0, T104Frame_getMsgSize(frame));
    }
    else
        writeToSocket(self, T104Frame_getBuffer(frame), T104Frame_getMsgSize(frame));
//    unsigned long long timestamp = Hal_getTimeInMs();
    if(self->msgsendHandler_withExplain != NULL)//,Hal_getTimeInMs()   ,timestamp
    {
        //CS101_ASDU asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), T104Frame_getBuffer(frame) + 6, T104Frame_getMsgSize(frame) - 6);
        CS101_ASDU asdu;
        if(self->baseProtocalType == 2)//104+
        {
            asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), T104Frame_getBuffer(frame) + 7, T104Frame_getMsgSize(frame) - 7);
        }
        else
        {
            asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), T104Frame_getBuffer(frame) + 6, T104Frame_getMsgSize(frame) - 6);
        }
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

            self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, T104Frame_getBuffer(frame), T104Frame_getMsgSize(frame),"I_frame_explain",self->cs104_frame,asdu);//Hal_getTimeInMs()

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
        //
        self->msgsendHandler_mStation = NULL;
        self->msgsendHandlerParameter_mStation = NULL;
        self->msgreceivedHandler_mStation = NULL;
        self->msgreceivedHandlerParameter_mStation = NULL;

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
        self->receiveCount = 0;//clear seqnum
        self->sendCount = 0;

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
        self->isCommOpend = false;

        self->sentASDUs = NULL;
        //
        if(tcpPort == CS104_DEFAULT_PORT_104P)//
        {
            self->baseProtocalType = 2;
            self->shortframesize = 7;
            prepareSMessage_104P(self->sMessage_104P);

        }
        else
        {
            self->baseProtocalType = 1;//基本规约类型：1-104   2-104+
            self->shortframesize = 6;
            prepareSMessage(self->sMessage);

        }
    }

    return self;
}

CS104_Connection
CS104_Connection_create(const char* hostname, int tcpPort)
{
//    if (tcpPort == -1)
//        tcpPort = IEC_60870_5_104_DEFAULT_PORT;

//    return createConnection(hostname, tcpPort);

    CS104_Connection self = createConnection(hostname, tcpPort);
    if (self != NULL) {
        self->commPort = NULL;
        self->connectionType = 1;
    }
    return self;
}

bool CS104_Connection_getCommState(CS104_Connection self)
{
    return self->isCommOpend;
}

CS104_Connection
CS104_Connection_create_comm(SerialPort serialPort)
{
    CS104_Connection self = createConnection("0.0.0.0", IEC_60870_5_104_DEFAULT_PORT);
    if (self != NULL) {
        self->commPort = serialPort;
        self->connectionType = 3;
    }


    return self;
}

CS104_Connection
CS104_Connection_create_mStation(CS104_Connection_mStation slave, Socket socket)
{
    CS104_Connection self = (CS104_Connection) GLOBAL_MALLOC(sizeof(struct sCS104_Connection));

    if (self != NULL) {
        self->server_mStation = slave;
        self->socket = socket;

        //strncpy(self->hostname, "127.0.0.1", HOST_NAME_MAX);//hostname
        //self->tcpPort = 123456;//tcpPort

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
        //
        self->msgsendHandler_mStation = NULL;
        self->msgsendHandlerParameter_mStation = NULL;
        self->msgreceivedHandler_mStation = NULL;
        self->msgreceivedHandlerParameter_mStation = NULL;

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


    }
    return self;
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
    self->unconfirmedSendIMessages = 0;//
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
//k 表示在某一特定的时间内未被 DTE 确认（即不被承认）的连续编号的 I 格式 APDU的最大数目
//k 值的最大范围：推荐值为 12，精确到一个 APDU
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
CS104_Connection_close_mStation(CS104_Connection self)
{
    self->running = false;//self->close = true;
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
CS104_Connection_destroy_comm(CS104_Connection self)
{
    if(self)
    {
        SerialPort_close(self->commPort);
        SerialPort_destroy(self->commPort);
        self->isCommOpend = false;//self->running = false;

    #if (CONFIG_USE_THREADS == 1)
        if (self->connectionHandlingThread)
        {
            Thread_destroy(self->connectionHandlingThread);
            self->connectionHandlingThread = NULL;
        }
    #endif
    }


    GLOBAL_FREEMEM(self);
}

void
CS104_Connection_destroy_mStation_one(CS104_Connection self)
{
    CS104_Connection_close_mStation(self);

    if (self->sentASDUs != NULL)
        GLOBAL_FREEMEM(self->sentASDUs);

#if (CONFIG_USE_THREADS == 1)
    Semaphore_destroy(self->sentASDUsLock);
#endif

    GLOBAL_FREEMEM(self);
}

void
CS104_Connection_setProtocalType(CS104_Connection self, unsigned char baseProtocalType)
{
    if(self)
    {
        self->baseProtocalType = baseProtocalType;

        if(baseProtocalType == 2)//改变规约类型，修改相关参数
        {
            self->shortframesize = 7;
            prepareSMessage_104P(self->sMessage_104P);
        }
        else
        {
            self->shortframesize = 6;
            prepareSMessage(self->sMessage);
        }
    }
}

unsigned char
CS104_Connection_getProtocalType(CS104_Connection self)
{
    if(self)
    {
        return self->baseProtocalType;
    }
    return 0;
}

int
CS104_Connection_getunconfirmedSendIMessages(CS104_Connection self)
{
    if(self)
    {
        return self->unconfirmedSendIMessages;
    }
    return -1;
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
receiveMessageSocket_104P(Socket socket, uint8_t* buffer)
{
    int readFirst = Socket_read(socket, buffer, 1);

    if (readFirst < 1)
    {
        #ifdef WIN32
                qDebug()<<"DEBUG_LIB60870:(warnning)"<<"Socket_read(socket, buffer, 1) return value < 1,value is"<<readFirst;
        #else
                syslog(LOG_WARNING,"Socket_read(socket, buffer, 1) return value < 1,value is  = %i", readFirst);
        #endif
        //DEBUG_PRINT("DEBUG_LIB60870:(warnning)Socket_read(socket, buffer, 1) return value < 1,value is  = %i\n", readFirst);
        return readFirst;
    }

    if (buffer[0] != 0x68)//读取首字节失败
    {
        #ifdef WIN32
                qDebug()<<"DEBUG_LIB60870:(warnning)"<<"message error,buffer[0] != 0x68,buffer[0] == "<<buffer[0];
        #else
                syslog(LOG_WARNING,"message error,buffer[0] != 0x68,buffer[0] == ", buffer[0]);
        #endif
        //DEBUG_PRINT("DEBUG_LIB60870:(warnning)message error,buffer[0] != 0x68,buffer[0] == %i\n", buffer[0]);
        return -2; /* message error */
    }

    if (Socket_read(socket, buffer + 1, 1) != 1)//读取长度失败
    {
        DEBUG_PRINT("DEBUG_LIB60870:(warnning)Socket_read(socket, buffer + 1, 1) != 1\n");
        return -3;
    }
    if (Socket_read(socket, buffer + 2, 1) != 1)//读取长度失败
    {
        DEBUG_PRINT("DEBUG_LIB60870:(warnning)Socket_read(socket, buffer + 2, 1) != 1\n");
        return -3;
    }

    int length = buffer[1] + buffer[2]*256;
    int datasize = Socket_read(socket, buffer + 3, length);
    /* read remaining frame */
    if(datasize < length)   //TCP通讯大小超过1500进行分片，目前帧大小<2400,最多分两片
    {
        Thread_sleep(1);//SOCKET ERROR10035 立马读取会报错（Service temporarily unavailable）
        datasize += Socket_read(socket, buffer + 3 + datasize, (length-datasize));
    }
    if (datasize != length)//if (Socket_read(socket, buffer + 3, length) != length)
    {
        #ifdef WIN32
                qDebug()<<"DEBUG_LIB60870:(warnning)"<<"Socket_read(socket, buffer + 3, length) != buffer[1]+ buffer[2]*256,"<<datasize<<","<<length;
        #else
                syslog(LOG_WARNING,"Socket_read(socket, buffer + 3, length) != buffer[1]+ buffer[2]*256,%d,%d",datasize,length);
        #endif
        //DEBUG_PRINT("DEBUG_LIB60870:(warnning)message error,Socket_read(socket, buffer + 2, length) != (%1)buffer[1]+ buffer[2]*256\n",length);

        return -4;
    }

    return length + 3;
}

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
        //DEBUG_PRINT("DEBUG_LIB60870:(warnning)Socket_read(socket, buffer, 1) return value < 1,value is  = %i\n", readFirst);
        return readFirst;
    }

    if (buffer[0] != 0x68)
    {
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"message error,buffer[0] != 0x68,buffer[0] =="<<buffer[0];
#else
        syslog(LOG_WARNING,"message error,buffer[0] != 0x68,buffer[0] == %i", buffer[0]);
#endif
        //DEBUG_PRINT("DEBUG_LIB60870:(warnning)message error,buffer[0] != 0x68,buffer[0] == %i\n", buffer[0]);
        return -1; /* message error */
    }

    if (Socket_read(socket, buffer + 1, 1) != 1)
    {
        DEBUG_PRINT("DEBUG_LIB60870:(warnning)Socket_read(socket, buffer + 1, 1) != 1\n");
        return -1;
    }

    int length = buffer[1];

    /* read remaining frame */
    if (Socket_read(socket, buffer + 2, length) != length)
    {
        DEBUG_PRINT("DEBUG_LIB60870:(warnning)message error,Socket_read(socket, buffer + 2, length) != buffer[1]\n");
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
    if(self->baseProtocalType == 2)//104+
        return receiveMessageSocket_104P(self->socket, buffer);
    else
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
    //unsigned long long timestamp = Hal_getTimeInMs();

    if (msgSize > 0) {
        if (self->msgreceivedHandler != NULL)
            self->msgreceivedHandler(self->msgreceivedHandlerParameter, buffer, msgSize);

    }

    if(buffer[0] == 0x69)//0x69=====private
    {
        //传给dtu调用私有规约解包
        if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()  ,Hal_getTimeInMs()
            self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Recieve Private protocal",self->cs104_frame,NULL);
    }
    else  //0x68=====104
    {
        unsigned char apduStartBuf;
        if(self->baseProtocalType == 2)
            apduStartBuf = buffer[3];
        else
            apduStartBuf = buffer[2];

    if ((apduStartBuf & 1) == 0) {//buffer[2] /* I format frame */
//#ifdef WIN32
//        qDebug()<<"ReceivedIMessage: "<<Hal_getTimeInMs();
//#endif

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

//        int frameSendSequenceNumber = ((buffer [3] * 0x100) + (buffer [2] & 0xfe)) / 2;
//        int frameRecvSequenceNumber = ((buffer [5] * 0x100) + (buffer [4] & 0xfe)) / 2;
        int frameSendSequenceNumber = 0;
        int frameRecvSequenceNumber = 0;
        if(self->baseProtocalType == 2)//104+
        {
            frameSendSequenceNumber = ((buffer [4] * 0x100) + (buffer [3] & 0xfe)) / 2;
            frameRecvSequenceNumber = ((buffer [6] * 0x100) + (buffer [5] & 0xfe)) / 2;

        }
        else
        {
            frameSendSequenceNumber = ((buffer [3] * 0x100) + (buffer [2] & 0xfe)) / 2;
            frameRecvSequenceNumber = ((buffer [5] * 0x100) + (buffer [4] & 0xfe)) / 2;
        }

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

        //CS101_ASDU asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), buffer + 6, msgSize - 6);
        CS101_ASDU asdu;
        if(self->baseProtocalType == 2)//104+
        {
            asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), buffer + 7, msgSize - 7);
        }
        else
        {
            asdu = CS101_ASDU_createFromBuffer((CS101_AppLayerParameters)&(self->alParameters), buffer + 6, msgSize - 6);
        }

        if (asdu != NULL) {
//            if (self->receivedHandler != NULL)
//                self->receivedHandler(self->receivedHandlerParameter, -1, asdu);

            self->cs104_frame.TI = CS101_ASDU_getTypeID(asdu);
            self->cs104_frame.COT = CS101_ASDU_getCOT(asdu);
            self->cs104_frame.COT += (CS101_ASDU_isTest(asdu)==true)?0x80:0x00;
            self->cs104_frame.COT += (CS101_ASDU_isNegative(asdu)==true)?0x40:0x00;
            self->cs104_frame.EC = CS101_ASDU_getNumberOfElements(asdu);
            self->cs104_frame.NR = frameRecvSequenceNumber;
            self->cs104_frame.NS = frameSendSequenceNumber;
            self->cs104_frame.FT = 1;//frame type帧类型：1-I帧 2-U帧 3-S帧

//#ifdef WIN32
//        qDebug()<<"ReceivedIMessage(before msgreceivedHandler): "<<Hal_getTimeInMs();
//#endif
            if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()  获取当前时间时间戳   ,timestamp
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"I_frame_explain",self->cs104_frame,asdu);// Hal_getTimeInMs()
//            DEBUG_PRINT("checkMessage(Received I frame): time %ld\n", timestamp);//
//#ifdef WIN32
//        qDebug()<<"ReceivedIMessage(after msgreceivedHandler): "<<Hal_getTimeInMs();
//#endif

        if (self->receivedHandler != NULL)
            self->receivedHandler(self->receivedHandlerParameter, -1, asdu);

            CS101_ASDU_destroy(asdu);
        }
        else
        {
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "CS101_ASDU == NULL,return false!");
            return false;
        }

    }
    else if ((apduStartBuf & 0x03) == 0x03) {//buffer[2] /* U format frame */

        //DEBUG_PRINT("Received U frame");//\n
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = 0;
        self->cs104_frame.NS = 0;
        self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧


        self->uMessageTimeout = 0;

        if (apduStartBuf == 0x43) {//buffer[2] /* Check for TESTFR_ACT message */
//            DEBUG_PRINT("Send TESTFR_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()  ,Hal_getTimeInMs()
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Send TESTFR_CON",self->cs104_frame,NULL);
//            //qDebug()<<"DEBUG_LIB60870:"<<"(for printtest)Send TESTFR_CON\n";
//            if (self->importantInfoHandler != NULL)
//                self->importantInfoHandler(self->importantInfoHandlerParameter, "Send TESTFR_CON!");//
            if(self->connectionType == 3)//串口
            {
                if(self->baseProtocalType == 2)
                {
                    SerialPort_write(self->commPort, TESTFR_CON_MSG_104P, 0, TESTFR_CON_MSG_SIZE_104P);
                }
                else
                    SerialPort_write(self->commPort, TESTFR_CON_MSG, 0, TESTFR_CON_MSG_SIZE);
            }
            else
            {
                if(self->baseProtocalType == 2)
                {
                    writeToSocket(self, TESTFR_CON_MSG_104P, TESTFR_CON_MSG_SIZE_104P);
                }
                else
                    writeToSocket(self, TESTFR_CON_MSG, TESTFR_CON_MSG_SIZE);
            }

//            int tempsize_shortframe = 6;//tempsize_shortframe/*6*/
//            if(self->baseProtocalType == 2)
//            {
//                tempsize_shortframe = 7;
//            }


            if(self->msgsendHandler_withExplain != NULL)//,Hal_getTimeInMs()   ,Hal_getTimeInMs()
            {
                self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, buffer, self->shortframesize/*6*/,"Send TESTFR_CON",self->cs104_frame,NULL);//
            }
        }
        else if (apduStartBuf == 0x83) {//buffer[2] /* TESTFR_CON */
//            DEBUG_PRINT("Rcvd TESTFR_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()    ,Hal_getTimeInMs()
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Rcvd TESTFR_CON",self->cs104_frame,NULL);
            //qDebug()<<"DEBUG_LIB60870:"<<"(for printtest)Rcvd TESTFR_CON\n";

            self->outstandingTestFCConMessages = 0;
        }
        else if (apduStartBuf == 0x07) {//buffer[2] /* STARTDT_ACT */
            //DEBUG_PRINT("Send STARTDT_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()    ,Hal_getTimeInMs()
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Send STARTDT_CON",self->cs104_frame,NULL);
            self->receiveCount = 0;//clear seqnum
            self->sendCount = 0;
            self->unconfirmedReceivedIMessages = 0;
            self->unconfirmedSendIMessages = 0;
            if(self->connectionType == 3)//串口
            {
                if(self->baseProtocalType == 2)
                {
                    SerialPort_write(self->commPort, STARTDT_CON_MSG_104P, 0, STARTDT_CON_MSG_SIZE_104P);
                }
                else
                    SerialPort_write(self->commPort, STARTDT_CON_MSG, 0, STARTDT_CON_MSG_SIZE);
            }
            else
            {
                if(self->baseProtocalType == 2)
                {
                    writeToSocket(self, STARTDT_CON_MSG_104P, STARTDT_CON_MSG_SIZE_104P);
                }
                else
                    writeToSocket(self, STARTDT_CON_MSG, STARTDT_CON_MSG_SIZE);
            }

            if(self->msgsendHandler_withExplain != NULL)//,Hal_getTimeInMs()  ,Hal_getTimeInMs()
            {
                self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, buffer, self->shortframesize/*6*/,"Send STARTDT_CON",self->cs104_frame,NULL);//
            }
        }
        else if (apduStartBuf == 0x0b) {//buffer[2] /* STARTDT_CON */
            //DEBUG_PRINT("Received STARTDT_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()   ,Hal_getTimeInMs()
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Rcvd STARTDT_CON",self->cs104_frame,NULL);
            self->receiveCount = 0;//clear seqnum
            self->sendCount = 0;
            self->unconfirmedReceivedIMessages = 0;
            self->unconfirmedSendIMessages = 0;
//            if (self->importantInfoHandler != NULL)
//                self->importantInfoHandler(self->importantInfoHandlerParameter, "Received STARTDT_CON(68040b000000)!");//
            if (self->connectionHandler != NULL)
                self->connectionHandler(self->connectionHandlerParameter, self, CS104_CONNECTION_STARTDT_CON_RECEIVED);
        }
        else if (apduStartBuf == 0x23) {//buffer[2] /* STOPDT_CON */
            //DEBUG_PRINT("Received STOPDT_CON");//\n
            if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()    ,Hal_getTimeInMs()
                self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"Rcvd STOPDT_CON",self->cs104_frame,NULL);

            if (self->connectionHandler != NULL)
                self->connectionHandler(self->connectionHandlerParameter, self, CS104_CONNECTION_STOPDT_CON_RECEIVED);
        }

    }
    else if (apduStartBuf == 0x01) {//buffer [2] /* S-message */
        //int seqNo = (buffer[4] + buffer[5] * 0x100) / 2;
        int seqNo;
        if(self->baseProtocalType == 2)//104+
        {
            seqNo = (buffer[5] + buffer[6] * 0x100) / 2;
        }
        else
        {
           seqNo = (buffer[4] + buffer[5] * 0x100) / 2;
        }
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = seqNo;
        self->cs104_frame.NS = self->sendCount;
        self->cs104_frame.FT = 3;//frame type帧类型：1-I帧 2-U帧 3-S帧
        if (self->msgreceivedHandler_withExplain != NULL)//,Hal_getTimeInMs()    ,Hal_getTimeInMs()
            self->msgreceivedHandler_withExplain(self->msgreceivedHandlerParameter_withExplain, buffer, msgSize,"S_frame_explain",self->cs104_frame,NULL);

        //DEBUG_PRINT("Rcvd S(%i) (own sendcounter = %i)\n", seqNo, self->sendCount);
        //qDebug()<<"DEBUG_LIB60870: "<<"Rcvd S("<<seqNo<<") (own sendcounter = "<<self->sendCount<<")";
        self->unconfirmedSendIMessages = 0;//收到S帧，I帧未回复帧计数清零

        if (checkSequenceNumber(self, seqNo) == false)
        {
            //DEBUG_PRINT("checkMessage return false(S-message),Sequence error: Close connection!");
            if (self->importantInfoHandler != NULL)
                self->importantInfoHandler(self->importantInfoHandlerParameter, "checkMessage return false(S-message),Sequence error: Close connection!");
            return false;
        }
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

            if(self->connectionType == 3)//串口
            {
                if(self->baseProtocalType == 2)
                {
                    SerialPort_write(self->commPort, TESTFR_ACT_MSG_104P, 0, TESTFR_ACT_MSG_SIZE_104P);
                }
                else
                    SerialPort_write(self->commPort, TESTFR_ACT_MSG, 0, TESTFR_ACT_MSG_SIZE);
            }
            else
            {
                if(self->baseProtocalType == 2)
                {
                    writeToSocket(self, TESTFR_ACT_MSG_104P, TESTFR_ACT_MSG_SIZE_104P);
                }
                else
                    writeToSocket(self, TESTFR_ACT_MSG, TESTFR_ACT_MSG_SIZE);
            }
            if(self->msgsendHandler_withExplain != NULL)//,Hal_getTimeInMs()  ,Hal_getTimeInMs()
            {
                self->cs104_frame.TI = 0;
                self->cs104_frame.COT = 0;
                self->cs104_frame.EC = 0;
                self->cs104_frame.NR = 0;
                self->cs104_frame.NS = 0;
                self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧
                if(self->baseProtocalType == 2)
                    self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, TESTFR_ACT_MSG_104P, self->shortframesize/*6*/,"Send TESTFR_ASK(U message T3 timeout)",self->cs104_frame,NULL);//
                else
                    self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, TESTFR_ACT_MSG, self->shortframesize/*6*/,"Send TESTFR_ASK(U message T3 timeout)",self->cs104_frame,NULL);
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
            DEBUG_PRINT("U message T1 timeout\n");//
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

                uint8_t buffer[300*4*2];

                Handleset_reset(handleSet);
                Handleset_addSocket(handleSet, self->socket);

                if (Handleset_waitReady(handleSet, 100)) {
                    int bytesRec = receiveMessage(self, buffer);

                    if (bytesRec <= -1) {
                        loopRunning = false;
                        self->failure = true;//perror();strerror(errno);
                        if (self->importantInfoHandler != NULL)
                        {
                            char bytesRecstr[5];
                            itoa(bytesRec, bytesRecstr, 10);
                            printf("Error reading from socket,loopRunning is set false,receiveMessage bytesRec == %d,strerror(errno): %s\n",bytesRec,strerror(errno));
                            //DEBUG_PRINT("Error reading from socket,loopRunning is set false,receiveMessage bytesRec == %d,strerror(errno): %s\n",bytesRec,strerror(errno));
                            self->importantInfoHandler(self->importantInfoHandlerParameter, "loopRunning is set false,receiveMessage bytesRec <0, bytesRec/strerror(errno):");
                            self->importantInfoHandler(self->importantInfoHandlerParameter, bytesRecstr);
                            self->importantInfoHandler(self->importantInfoHandlerParameter, strerror(errno));
                        }
                    }

                    if (bytesRec > 0) {

                        if (checkMessage(self, buffer, bytesRec) == false) {
                            /* close connection on error */
                            loopRunning= false;
                            DEBUG_PRINT("loopRunning is set false,checkMessage(self, buffer, bytesRec) is false\n");//
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
                    DEBUG_PRINT("loopRunning is set false,handleTimeouts(self) is false\n");//
                    if (self->importantInfoHandler != NULL)
                        self->importantInfoHandler(self->importantInfoHandlerParameter, "loopRunning is set false,handleTimeouts(self) return false!");
                }

                if (self->close)
                {
                    loopRunning = false;
                    DEBUG_PRINT("loopRunning is set false,self->close is true\n");//
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

    DEBUG_PRINT("EXIT CONNECTION HANDLING THREAD!!!\n");//
    if (self->importantInfoHandler != NULL)
        self->importantInfoHandler(self->importantInfoHandlerParameter, "EXIT CONNECTION HANDLING THREAD!");

    self->running = false;

    return NULL;
}

static int
readBytesWithTimeout(CS104_Connection self, uint8_t* buffer, int startIndex, int count)
{
    int readByte;
    int readBytes = 0;

    while ((readByte = SerialPort_readByte(self->commPort)) != -1) {
        buffer[startIndex++] = (uint8_t) readByte;

        readBytes++;

        if (readBytes >= count)
            break;
    }

    return readBytes;
}

static void*
handleConnection_comm(void* parameter)
{
    CS104_Connection self = (CS104_Connection) parameter;

    resetConnection(self);

    self->isCommOpend = true;
    //self->running = true;
    uint8_t buffer[520];

    while(self->isCommOpend){
        SerialPort_setTimeout(self->commPort, self->messageTimeout);

        int read = SerialPort_readByte(self->commPort);//起始帧

        if (read != -1) {

            if (read == 0x68||read == 0x69) {//帧头：兼容私有规约和标准104规约

                SerialPort_setTimeout(self->commPort, self->characterTimeout);

                int msgSize = SerialPort_readByte(self->commPort);//报文长度

                if (msgSize == -1)
                    goto sync_error;

                //buffer[0] = (uint8_t) 0x68;
                buffer[0] = (uint8_t) (read);
                buffer[1] = (uint8_t) msgSize;

                int readBytes;
                if(read == 0x69)
                {
                    readBytes= readBytesWithTimeout(self, buffer, 2, msgSize+4);
                    if (readBytes == (msgSize+4)) {//读取到完整帧，调用解帧函数
                        if (checkMessage(self, buffer, msgSize+6) == false) {

                        }

                    }
                    else {
                        DEBUG_PRINT("RECV[69]: Timeout reading variable length frame size = %i (expected = %i)\n", readBytes, msgSize);
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"RECV[69]: Timeout reading variable length frame size = "<<readBytes<<" (expected = "<<msgSize<<")";
#else
        syslog(LOG_WARNING,"RECV[69]: Timeout reading variable length frame size = %i (expected = %i)", readBytes, msgSize);
#endif
                    }
                }
                else //if(read == 0x68)
                {
                    readBytes= readBytesWithTimeout(self, buffer, 2, msgSize);
                    if (readBytes == msgSize) {//读取到完整帧，调用解帧函数
                        if (checkMessage(self, buffer, msgSize+2) == false) {

                        }

                    }
                    else {
                        DEBUG_PRINT("RECV[68]: Timeout reading variable length frame size = %i (expected = %i)\n", readBytes, msgSize);
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"RECV[68]: Timeout reading variable length frame size = "<<readBytes<<" (expected = "<<msgSize<<")";
#else
        syslog(LOG_WARNING,"RECV[68]: Timeout reading variable length frame size = %i (expected = %i)", , readBytes, msgSize);
#endif
                    }
                }




            }

            else {
                goto sync_error;
            }

        }
        else
        {
            //self->isCommOpend = false;
            //self->running = false;
            return NULL;
        }



    sync_error:

        DEBUG_PRINT("RECV: SYNC ERROR\n");

        SerialPort_discardInBuffer(self->commPort);

        return NULL;
    }

    /* Call connection handler */
    if (self->connectionHandler != NULL)
    {
        self->connectionHandler(self->connectionHandlerParameter, self, CS104_CONNECTION_CLOSED);
    }

}



static void*
handleConnection_mStation(void* parameter)//
{
    CS104_Connection self = (CS104_Connection) parameter;
    resetConnection(self);
    resetT3Timeout(self);

    uint8_t buffer[260];

    self->running = true;//

    HandleSet handleSet = Handleset_new();

    bool isAsduWaiting = false;
    //bool loopRunning = true;

    while (self->running) {//loopRunning

        Handleset_reset(handleSet);//DEBUG_PRINT("after Handleset_reset\n");//for debug
        Handleset_addSocket(handleSet, self->socket);//DEBUG_PRINT("after Handleset_addSocket\n");//for debug

        int socketTimeout;

        /*
         * When an ASDU is waiting only have a short look to see if a client request
         * was received. Otherwise wait to save CPU time.
         */
        if (isAsduWaiting)
            socketTimeout = 1;
        else
            socketTimeout = 100; /* TODO replace by configurable parameter */

        if (Handleset_waitReady(handleSet, socketTimeout)) {

            int bytesRec = receiveMessage(self, buffer);

            if (bytesRec == -1) {
#ifdef WIN32
        qDebug()<<"DEBUG_LIB60870:(warnning)"<<"Error reading from socket, strerror(errno) is "<<strerror(errno);
#else
        syslog(LOG_WARNING,"Error reading from socket, strerror(errno) is %s", strerror(errno));
#endif
                //DEBUG_PRINT("Error reading from socket, strerror(errno) is %s\n", strerror(errno));
                self->running = false;// loopRunning = false;
                self->failure = true;
                break;
            }

            if (bytesRec > 0) {
                //DEBUG_PRINT("Connection: rcvd msg(%i bytes)\n", bytesRec);

                if (self->msgreceivedHandler_mStation != NULL)
                    self->msgreceivedHandler_mStation(self->msgreceivedHandlerParameter_mStation, buffer, bytesRec,self->hostname,self->tcpPort);
                if (checkMessage(self, buffer, bytesRec) == false) {
                    /* close connection on error */
                    self->running = false;//loopRunning= false;
                    DEBUG_PRINT("loopRunning is set false,checkMessage(self, buffer, bytesRec) is false");//\n
                    self->failure = true;
                }

//                if (handleMessage(self, buffer, bytesRec) == false)
//                    self->running = false;

//                if (self->unconfirmedReceivedIMessages >= self->slave->conParameters.w) {

//                    self->lastConfirmationTime = Hal_getTimeInMs();

//                    self->unconfirmedReceivedIMessages = 0;

//                    sendSMessage(self);
//                }
            }
        }//DEBUG_PRINT("after Handleset_waitReady\n");//for debug

        if (handleTimeouts(self) == false)
        {
            self->running = false;//loopRunning = false;
            DEBUG_PRINT("loopRunning is set false,handleTimeouts(self) is false\n");//
        }

        if (self->close)
        {
            self->running = false;//loopRunning = false;
            DEBUG_PRINT("loopRunning is set false,self->close is true\n");//
        }

//        if (self->running)//
//            if (self->isActive)
//                isAsduWaiting = sendWaitingASDUs(self);
    }

    DEBUG_PRINT("Connection closed\n");

    Handleset_destroy(handleSet);

//    self->running = false;

//#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
//    if (self->slave->serverMode == CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP) {
//        MessageQueue_destroy(self->lowPrioQueue);
//        HighPriorityASDUQueue_destroy(self->highPrioQueue);
//    }
//#endif

//    CS104_Slave_removeConnection(self->slave, self);

//    return NULL;

#if (CONFIG_CS104_SUPPORT_TLS == 1)
    if (self->tlsSocket)
        TLSSocket_close(self->tlsSocket);
#endif

    Socket_destroy(self->socket);

    DEBUG_PRINT("EXIT CONNECTION HANDLING THREAD!!!\n");//
    if (self->importantInfoHandler != NULL)
        self->importantInfoHandler(self->importantInfoHandlerParameter, "EXIT CONNECTION HANDLING THREAD!");

    self->running = false;

    self->server_mStation->connectionBrokenHandler_mStation(self->server_mStation->connectionBrokenHandlerParameter_mStation,(char*)(self->hostname),self->tcpPort);
    //CS104_Connection_removeConnection(self->server_mStation, self);
    CS104_Connection_removeConnection_mStation(self->server_mStation, self);

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
CS104_Connection_connect_comm(CS104_Connection self)
{
    self->isCommOpend = SerialPort_open(self->commPort);

    if(self->isCommOpend)//成功打开串口，启动串口接收线程
    {
        //self->running = true;
        self->connectionHandlingThread = Thread_create(handleConnection_comm, (void*) self, false);

        if (self->connectionHandlingThread)
            Thread_start(self->connectionHandlingThread);
    }

    return self->isCommOpend ;

}

bool
CS104_Connection_connect_mStation(CS104_Connection self)
{
    //resetConnection(self);
    //resetT3Timeout(self);
    //DEBUG_PRINT("CS104_Connection_connect_mStation\n");//for debug
    self->connectionHandlingThread = Thread_create(handleConnection_mStation, (void*) self, false);

    if (self->connectionHandlingThread)
        Thread_start(self->connectionHandlingThread);

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
    DEBUG_PRINT("CS104_Connection_setASDUReceivedHandler\n");//for debug
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
    if(self->connectionType == 3)//串口
    {
        if(self->baseProtocalType == 2)
        {
            SerialPort_write(self->commPort, STARTDT_ACT_MSG_104P, 0, STARTDT_ACT_MSG_SIZE_104P);
        }
        else
            SerialPort_write(self->commPort, STARTDT_ACT_MSG, 0, STARTDT_ACT_MSG_SIZE);
    }
    else
    {
        if(self->baseProtocalType == 2)
        {
            writeToSocket(self, STARTDT_ACT_MSG_104P, STARTDT_ACT_MSG_SIZE_104P);
        }
        else
            writeToSocket(self, STARTDT_ACT_MSG, STARTDT_ACT_MSG_SIZE);
    }

    if(self->msgsendHandler_withExplain != NULL)//,Hal_getTimeInMs()   ,Hal_getTimeInMs()
    {
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = 0;
        self->cs104_frame.NS = 0;
        self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧
        if(self->baseProtocalType == 2)
            self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, STARTDT_ACT_MSG_104P, self->shortframesize/*6*/,"Send STARTDT_ACT",self->cs104_frame,NULL);//
        else
            self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, STARTDT_ACT_MSG, self->shortframesize/*6*/,"Send STARTDT_ACT",self->cs104_frame,NULL);
    }
}

void
CS104_Connection_sendStopDT(CS104_Connection self)
{
    if(self->connectionType == 3)//串口
    {
        if(self->baseProtocalType == 2)
        {
            SerialPort_write(self->commPort, STOPDT_ACT_MSG_104P, 0, STOPDT_ACT_MSG_SIZE_104P);
        }
        else
            SerialPort_write(self->commPort, STOPDT_ACT_MSG, 0, STOPDT_ACT_MSG_SIZE);
    }
    else
    {
        if(self->baseProtocalType == 2)
        {
            writeToSocket(self, STOPDT_ACT_MSG_104P, STOPDT_ACT_MSG_SIZE_104P);
        }
        else
            writeToSocket(self, STOPDT_ACT_MSG, STOPDT_ACT_MSG_SIZE);
    }
    if(self->msgsendHandler_withExplain != NULL)//,Hal_getTimeInMs()   ,Hal_getTimeInMs()
    {
        self->cs104_frame.TI = 0;
        self->cs104_frame.COT = 0;
        self->cs104_frame.EC = 0;
        self->cs104_frame.NR = 0;
        self->cs104_frame.NS = 0;
        self->cs104_frame.FT = 2;//frame type帧类型：1-I帧 2-U帧 3-S帧
        if(self->baseProtocalType == 2)
            self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, STOPDT_ACT_MSG_104P, self->shortframesize/*6*/,"Send STOPDT_ACT",self->cs104_frame,NULL);//
        else
            self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, STOPDT_ACT_MSG, self->shortframesize/*6*/,"Send STOPDT_ACT",self->cs104_frame,NULL);
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

    if (self->running||self->isCommOpend) {
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
    //Frame frame = (Frame) T104Frame_create();
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);

    encodeIdentificationField(self, frame, C_IC_NA_1, 1, cot, ca);

    encodeIOA(self, frame, 0);

    /* encode QOI (7.2.6.22) */
    T104Frame_setNextByte(frame, qoi); /* 20 = station interrogation *///召唤限定词 QOI:<20>  总召唤

    return sendASDUInternal(self, frame);
}

bool
CS104_Connection_sendSoftwareUpgrade(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, uint8_t ctype)
{
    //Frame frame = (Frame) T104Frame_create();
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //F_SR_NA_1 = 211 //<211>：= 软件升级
    encodeIdentificationField(self, frame, F_SR_NA_1, 1, cot, ca);

    encodeIOA(self, frame, 0);

    T104Frame_setNextByte(frame, ctype); //ctype     命令类型  (&0x80)=1，软件升级启动；=0，软件升级结束；

    return sendASDUInternal(self, frame);
}

//<101>∶＝电能量召唤命令   C_CI_NA_1=101
bool
CS104_Connection_sendCounterInterrogationCommand(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, uint8_t qcc)
{
    //Frame frame = (Frame) T104Frame_create();
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);

    //C_CI_NA_1=101    类型标识符 TI 1 字节
    //VSQ = 1          可变帧长限定词 VSQ 1 字节
    //COT = 6/7/10     传送原因 COT 2 字节
    //ca               ASDU 公共地址 2
    encodeIdentificationField(self, frame, C_CI_NA_1, 1, cot, ca);

    encodeIOA(self, frame, 0);//信息对象地址（=0） 3 字节

    /* encode QCC */  //计量信息命令限定词 QCC 1 字节
    T104Frame_setNextByte(frame, qcc);

    return sendASDUInternal(self, frame);
}

//   C_RD_NA_1=102
bool
CS104_Connection_sendReadCommand(CS104_Connection self, int ca, int ioa)
{
    //Frame frame = (Frame) T104Frame_create();
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);

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
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_CS_NA_1, 1, CS101_COT_ACTIVATION, ca);

    encodeIOA(self, frame, 0);

    T104Frame_appendBytes(frame, CP56Time2a_getEncodedValue(time), 7);

    return sendASDUInternal(self, frame);
}

bool //selfdefined
CS104_Connection_sendClockSyncCommand_SetandRead(CS104_Connection self, int ca, int cot,unsigned char* time)
{
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

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
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField(self, frame, C_TS_NA_1, 1, CS101_COT_ACTIVATION, ca);

    encodeIOA(self, frame, 0);

    T104Frame_setNextByte(frame, 0xaa);//0xcc
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
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

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
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

    encodeIdentificationField (self, frame, typeId, 1 /* SQ:false; NumIX:1 */, cot, ca);

    InformationObject_encode(sc, frame, (CS101_AppLayerParameters) &(self->alParameters), false);

    return sendASDUInternal(self, frame);
}

//200-203 远程参数读写
bool
CS104_Connection_sendParamValueRead(CS104_Connection self, CS101_CauseOfTransmission cot, int vsq,int ca, int sn,InformationObject sc)
{
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

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
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

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
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

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
    int rtn;
    if(self->connectionType == 3)//串口
    {
        rtn = SerialPort_write(self->commPort, (uint8_t*)buf, 0, len);
    }
    else
        rtn = writeToSocket(self, (uint8_t*)buf, len);

    if(self->msgsendHandler_withExplain != NULL)//私有规约包发送
    {
        self->msgsendHandler_withExplain(self->msgsendHandlerParameter_withExplain, buf, len,"Send Private Protocal",self->cs104_frame,NULL);//
    }

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
    Frame frame = (Frame) T104Frame_create_104P(self->shortframesize);
    //Frame frame = (Frame) T104Frame_create();

    CS101_ASDU_encode(asdu, frame);

    return sendASDUInternal(self, frame);
}

bool
CS104_Connection_isTransmitBufferFull(CS104_Connection self)
{
    return isSentBufferFull(self);
}

//===============  104 connection : as server and as master station ==============//

CS104_Connection_mStation
sCS104_Connection_mStation_create(int maxLowPrioQueueSize, int maxHighPrioQueueSize)
{
    printf("sCS104_Connection_mStation_create!\n");
    CS104_Connection_mStation self = (CS104_Connection_mStation) GLOBAL_MALLOC(sizeof(struct sCS104_Connection_mStation));

    if(self != NULL)
    {
        self->connectionRequestHandler_mStation = NULL;
        self->connectionRequestHandlerParameter_mStation = NULL;
        self->connectionBrokenHandler_mStation = NULL;
        self->connectionBrokenHandlerParameter_mStation = NULL;
#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
        self->maxLowPrioQueueSize = maxLowPrioQueueSize;
        self->maxHighPrioQueueSize = maxHighPrioQueueSize;
#endif

        self->isRunning = false;
        self->stopRunning = false;

        self->localAddress = NULL;
        self->tcpPort = CS104_DEFAULT_PORT;
        self->openConnections = 0;  //初始化，当前连接数=0
        self->listeningThread = NULL;
        self->history_index = 0;//remenber the connection ip:port link with index_ID

         self->masterConnections_mStation = LinkedList_create();

#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
        self->serverMode = CS104_MODE_SINGLE_REDUNDANCY_GROUP;
#else
#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
        self->serverMode = CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP;
#endif
#endif

    }

    return self;
}

void
CS104_Connection_setLocalAddress(CS104_Connection_mStation self, const char* ipAddress)
{
    printf("CS104_Connection_setLocalAddress 127.0.0.1\n");
    if (self->localAddress)
        GLOBAL_FREEMEM(self->localAddress);

    self->localAddress = (char*) GLOBAL_MALLOC(strlen(ipAddress) + 1);

    if (self->localAddress)
        strcpy(self->localAddress, ipAddress);
}

void
CS104_Connection_setLocalPort(CS104_Connection_mStation self, int tcpPort)
{
    printf("CS104_Connection_setLocalPort 4001\n");
    self->tcpPort = tcpPort;
}

int
CS104_Connection_getOpenConnections(CS104_Connection_mStation self)
{
    int openConnections;

#if (CONFIG_USE_THREADS == 1)
    Semaphore_wait(self->openConnectionsLock);
#endif

    openConnections = self->openConnections;

#if (CONFIG_USE_THREADS == 1)
    Semaphore_post(self->openConnectionsLock);
#endif

    return openConnections;
}

void
CS104_Connection_setConnectionRequestHandler(CS104_Connection_mStation self, CS104_ConnectionRequestHandler_mStation handler, void* parameter)
{
    printf("CS104_Connection_setConnectionRequestHandler\n");
    self->connectionRequestHandler_mStation = handler;
    self->connectionRequestHandlerParameter_mStation = parameter;
}

void
CS104_Connection_setConnectionBrokenHandler(CS104_Connection_mStation self, CS104_ConnectionBrokenHandler_mStation handler, void* parameter)
{
    self->connectionBrokenHandler_mStation = handler;
    self->connectionBrokenHandlerParameter_mStation = parameter;
}

//void
//CS104_Connection_setASDUReceivedHandler_mStation0(CS104_Connection self, CS101_ASDUReceivedHandler handler, void* parameter)
//{
//    DEBUG_PRINT("CS104_Connection_setASDUReceivedHandler\n");//for debug
//    self->asduHandler_mStation = handler;
//    self->asduHandlerParameter_mStation = parameter;
//}

/**
 * Activate connection and deactivate existing active connections if required
 */
static void
CS104_Connection_activate(CS104_Connection_mStation self, CS104_Connection connectionToActivate)
{
#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
    if (self->serverMode == CS104_MODE_SINGLE_REDUNDANCY_GROUP) {

        /* Deactivate all other connections */
#if (CONFIG_USE_THREADS == 1)
        Semaphore_wait(self->openConnectionsLock);
#endif

        LinkedList element;

        for (element = LinkedList_getNext(self->masterConnections_mStation);
             element != NULL;
             element = LinkedList_getNext(element))
        {
            CS104_Connection connection = (CS104_Connection) LinkedList_getData(element);

            if (connection != connectionToActivate)
                CS104_Connection_deactivate(connection);
        }

#if (CONFIG_USE_THREADS == 1)
        Semaphore_post(self->openConnectionsLock);
#endif

    }
#endif /* (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1) */
}

static void*
serverThread_mStation (void* parameter)
{
    CS104_Connection_mStation self = (CS104_Connection_mStation) parameter;

    ServerSocket serverSocket;

    if (self->localAddress)
        serverSocket = TcpServerSocket_create(self->localAddress, self->tcpPort);
    else
        serverSocket = TcpServerSocket_create("0.0.0.0", self->tcpPort);

    if (serverSocket == NULL) {
        DEBUG_PRINT("Cannot create server socket\n");
        self->isStarting = false;
        goto exit_function;
    }

    ServerSocket_listen(serverSocket);

    self->isRunning = true;
    self->isStarting = false;

    while (self->stopRunning == false) {
        Socket newSocket = ServerSocket_accept(serverSocket);

        if (newSocket != NULL) {

            bool acceptConnection = true;

            /* check if maximum number of open connections is reached */  //超过最大连接数，不再接受连接
            if (self->maxOpenConnections > 0) {
                if (self->openConnections >= self->maxOpenConnections)
                    acceptConnection = false;
            }

            char ipAddress[60];
            int port_new;
            if (acceptConnection && (self->connectionRequestHandler_mStation != NULL)) {
                //get ip and port info of the new connection
                Socket_getPeerAddressStatic(newSocket, ipAddress);
                char* p1;
                char* p2;

                p1 = strtok(ipAddress, ":");
                if(p1)
                {
                    strcpy(ipAddress, p1);
                    p2 = strtok(NULL, ":");
                    if(p2)
                        port_new = atoi(p2);
                }
                else
                {
                    //port_new = 2404;//2404

                }

                DEBUG_PRINT("new Connection of ipAddress = %s;p1 = %s;p2 = %s!;port = %d\n",ipAddress,p1,p2,port_new);


//                /* remove TCP port part */
//                char* separator = strchr(ipAddress, ':');
//                if (separator != NULL)
//                    *separator = 0;

//                //接收新的连接
//                acceptConnection = self->connectionRequestHandler_mStation(self->connectionRequestHandlerParameter_mStation,ipAddress,port_new);
//                //DEBUG_PRINT("new Connection of %s!(after strchr)\n",ipAddress);//for debug

                if(acceptConnection)
                {
                    /*
                    CS104_Connection connection = CS104_Connection_create_mStation(self, newSocket);
                    if (connection) {

    #if (CONFIG_USE_THREADS)
                        Semaphore_wait(self->openConnectionsLock);
    #endif

                        //self->openConnections++;
                        //LinkedList_add(self->masterConnections, connection);
                        //================
                        strncpy(connection->hostname, ipAddress, HOST_NAME_MAX);
                        connection->tcpPort = port_new;
                        bool rtn = CS104_Connection_connect_mStation(connection);
                       if(rtn)
                       {
                           CS104_Connection_activate(connection->server_mStation, connection);
                           connection->isActive = true;
                           self->openConnections++;
                           LinkedList_add(self->masterConnections_mStation, connection);

                           CS104_Connection_sendStartDT(connection);
                       }
                        //================





    #if (CONFIG_USE_THREADS)
                        Semaphore_post(self->openConnectionsLock);
    #endif
                    }
                    else
                        DEBUG_PRINT("Connection attempt failed!");
                    */

//                    //self->socket, self->hostname, self->tcpPort
                    int cur_index = 0;
                    if(self->history_index <= 0 || self->history_index > 1024)//start from 1
                        self->history_index = 1;
                    else
                    {
                        //judge weather the ip:port info is in the history list
                        cur_index = get_connectionIndex_mStation(self ,ipAddress,port_new);

                        if(cur_index < 0)// || cur_index >= 1024
                        {
                            self->history_index++;
                            cur_index = self->history_index -1;
                        }
                    }
                    self->connections_mStation[cur_index] = CS104_Connection_create_mStation(self, newSocket);

                    if (self->connections_mStation[cur_index])
                    {
#if (CONFIG_USE_THREADS)
                    Semaphore_wait(self->openConnectionsLock);
#endif

                        strncpy(self->connections_mStation[cur_index]->hostname, ipAddress, HOST_NAME_MAX);
                        self->connections_mStation[cur_index]->tcpPort = port_new;
                        self->connections_mStation[cur_index]->connection_index = cur_index;//start from 0

                       bool rtn = CS104_Connection_connect_mStation(self->connections_mStation[cur_index]);
                       if(rtn)
                       {
                           CS104_Connection_activate(self->connections_mStation[cur_index]->server_mStation, self->connections_mStation[cur_index]);
                           self->connections_mStation[cur_index]->isActive = true;
                           self->openConnections++;
                           LinkedList_add(self->masterConnections_mStation, self->connections_mStation[cur_index]);

                           CS104_Connection_sendStartDT(self->connections_mStation[cur_index]);
                       }
    #if (CONFIG_USE_THREADS)
                        Semaphore_post(self->openConnectionsLock);
    #endif
                }
                else
                    DEBUG_PRINT("Connection attempt failed!");

                }
            }

            //接收新的连接
            acceptConnection = self->connectionRequestHandler_mStation(self->connectionRequestHandlerParameter_mStation,ipAddress,port_new);
            //DEBUG_PRINT("new Connection of %s!(after strchr)\n",ipAddress);//for debug

            if (acceptConnection) {



//                MessageQueue lowPrioQueue = NULL;
//                HighPriorityASDUQueue highPrioQueue = NULL;

//#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
//                if (self->serverMode == CS104_MODE_SINGLE_REDUNDANCY_GROUP) {
//                    lowPrioQueue = self->asduQueue;
//                    highPrioQueue = self->connectionAsduQueue;
//                }
//#endif

//#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
//                if (self->serverMode == CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP) {
//                    lowPrioQueue = MessageQueue_create(self->maxLowPrioQueueSize);
//                    highPrioQueue = HighPriorityASDUQueue_create(self->maxHighPrioQueueSize);
//                }
//#endif

//                MasterConnection connection =
//                        MasterConnection_create(self, newSocket, lowPrioQueue, highPrioQueue);

//                if (connection) {

//#if (CONFIG_USE_THREADS)
//                    Semaphore_wait(self->openConnectionsLock);
//#endif

//                    self->openConnections++;
//                    LinkedList_add(self->masterConnections, connection);

//#if (CONFIG_USE_THREADS)
//                    Semaphore_post(self->openConnectionsLock);
//#endif
//                }
//                else
//                    DEBUG_PRINT("Connection attempt failed!");

            }
            else {
                Socket_destroy(newSocket);
            }
        }
        else
            Thread_sleep(10);
    }

    if (serverSocket)
        Socket_destroy((Socket) serverSocket);

    self->isRunning = false;
    self->stopRunning = false;

exit_function:
    return NULL;
}

void
CS104_Connection_start_mStation(CS104_Connection_mStation self)//启动监听
{
    printf("CS104_Connection_start_mStation\n");
    if (self->isRunning == false) {
    printf("self->isRunning == false\n");
        self->isStarting = true;
        self->isRunning = false;
        self->stopRunning = false;

//#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
//        if (self->serverMode == CS104_MODE_SINGLE_REDUNDANCY_GROUP)
//            initializeMessageQueues(self, self->maxLowPrioQueueSize, self->maxHighPrioQueueSize);
//#endif
        self->listeningThread = Thread_create(serverThread_mStation, (void*) self, false);//serverThread

        Thread_start(self->listeningThread);

        while (self->isStarting)
            Thread_sleep(1);
    }
}

bool
CS104_Connection_isRunning_mStation(CS104_Connection_mStation self)
{
    return self->isRunning;
}

void
CS104_Connection_stop_mStation(CS104_Connection_mStation self)
{
    //qDebug()<<"before stopRunning";
    if (self->isRunning) {
        self->stopRunning = true;

        while (self->isRunning)
            Thread_sleep(1);
    }

    //qDebug()<<"after stopRunning";

    if (self->listeningThread) {
        Thread_destroy(self->listeningThread);
    }

    self->listeningThread = NULL;
}

void
CS104_Connection_destroy_mStation(CS104_Connection_mStation self)
{
    if (self->isRunning)
        CS104_Connection_stop_mStation(self);

//#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
//    if (self->serverMode == CS104_MODE_SINGLE_REDUNDANCY_GROUP)
//        MessageQueue_releaseAllQueuedASDUs(self->asduQueue);
//#endif

 //   if (self->localAddress != NULL)
 //       GLOBAL_FREEMEM(self->localAddress);

    /*
     * Stop all connections
     * */
//#if (CONFIG_USE_THREADS == 1)
//    Semaphore_wait(self->openConnectionsLock);
//#endif

//    if(self->openConnections > 0 && self->openConnections <= 1024)
//    {
//        for(int i=0;i<self->openConnections;i++)
//        {
//            if(self->connections_mStation[i] != NULL)
//                CS104_Connection_destroy(self->connections_mStation[i]);
//        }
//    }

    LinkedList element;

    for (element = LinkedList_getNext(self->masterConnections_mStation);
         element != NULL;
         element = LinkedList_getNext(element))
    {
        CS104_Connection connection = (CS104_Connection) LinkedList_getData(element);

        //CS104_Connection_close(connection);
        CS104_Connection_close_mStation(connection);
    }


//#if (CONFIG_USE_THREADS == 1)
//    Semaphore_post(self->openConnectionsLock);
//#endif

    /* Wait until all connections are closed */
    while (CS104_Connection_getOpenConnections(self) > 0)
        Thread_sleep(10);

    LinkedList_destroyStatic(self->masterConnections_mStation);

//#if (CONFIG_USE_THREADS == 1)
//    Semaphore_destroy(self->openConnectionsLock);
//#endif

#if (CONFIG_SLAVE_WITH_STATIC_MESSAGE_QUEUE == 0)

//#if (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1)
//    if (self->serverMode == CS104_MODE_SINGLE_REDUNDANCY_GROUP) {
//        MessageQueue_destroy(self->asduQueue);
//        HighPriorityASDUQueue_destroy(self->connectionAsduQueue);
//    }
//#endif /* (CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP == 1) */
  //  GLOBAL_FREEMEM(self);

#endif /* (CONFIG_SLAVE_WITH_STATIC_MESSAGE_QUEUE == 0) */
}

void
CS104_Connection_deactivate(CS104_Connection self)
{
    self->isActive = false;
}

int get_connectionIndex_mStation(CS104_Connection_mStation self,char* ip_temp,int port_temp)
{
    if(self->history_index <= 0 || self->history_index > 1024)
        return -1;
    //int strcmp(const char* s1,const char* s2)
    for(int i=0;i<self->history_index;i++)
    {
        if(self->connections_mStation[i] == NULL)
            return -1;
        if(strcmp(self->connections_mStation[i]->hostname,ip_temp)==0 && self->connections_mStation[i]->tcpPort == port_temp)
        {
            return i;
        }
    }
    return -1;
}

void
CS104_Connection_setMSGReceivedHandler_m(CS104_Connection self, CS104_MSGReceivedHandler_mStation handler, void* parameter)
{
    self->msgreceivedHandler_mStation = handler;
    self->msgreceivedHandlerParameter_mStation = parameter;
}

void
CS104_Connection_setMSGSendHandler_m(CS104_Connection self, CS104_MSGSendHandler_mStation handler, void* parameter)
{
    self->msgsendHandler_mStation = handler;
    self->msgsendHandlerParameter_mStation = parameter;
}

void
CS104_Connection_setASDUReceivedHandler_mStation(CS104_Connection_mStation self, CS101_ASDUReceivedHandler handler, void* parameter,int con_index)
{
//    self->receivedHandler = handler;
//    self->receivedHandlerParameter = parameter;

//    for(int i=0;i<1024;i++)
//    {
//        if(self->connections_mStation[i] != NULL)
//            CS104_Connection_setASDUReceivedHandler(self->connections_mStation[i],handler,parameter);
//    }
    DEBUG_PRINT("CS104_Connection_setASDUReceivedHandler for the %d connection",con_index);
    if(con_index < 0 || con_index > 1024)
        return ;
    if(self->connections_mStation[con_index] != NULL)
    {
        CS104_Connection_setASDUReceivedHandler(self->connections_mStation[con_index],handler,parameter);
        //CS104_Connection_setASDUReceivedHandler_mStation0(self->connections_mStation[con_index],handler,parameter);
    }
}

void
CS104_Connection_setMSGReceivedHandler_mStation(CS104_Connection_mStation self, CS104_MSGReceivedHandler_mStation handler, void* parameter,int con_index)
{
    DEBUG_PRINT("CS104_Connection_setMSGReceivedHandler_mStation for the %d connection",con_index);
    if(con_index < 0 || con_index > 1024)
        return ;
    if(self->connections_mStation[con_index] != NULL)
    {
        CS104_Connection_setMSGReceivedHandler_m(self->connections_mStation[con_index],handler,parameter);
        //CS104_Connection_setMSGReceivedHandler(self->connections_mStation[con_index],handler,parameter);
    }
}

void
CS104_Connection_setMSGSendHandler_mStation(CS104_Connection_mStation self, CS104_MSGSendHandler_mStation handler, void* parameter,int con_index)
{
    DEBUG_PRINT("CS104_Connection_setMSGSendHandler_mStation for the %d connection",con_index);
    if(con_index < 0 || con_index > 1024)
        return ;
    if(self->connections_mStation[con_index] != NULL)
    {
        CS104_Connection_setMSGSendHandler_m(self->connections_mStation[con_index],handler,parameter);
        //CS104_Connection_setMSGSendHandler(self->connections_mStation[con_index],handler,parameter);
    }
}

CS104_Connection
get_connection_from_mStation(CS104_Connection_mStation self,int con_index)
{
    if(self == NULL || con_index<0 || con_index > 1024)
        return NULL;

    //CS104_Connection con = self->connections_mStation[con_index];
    return self->connections_mStation[con_index];
}

char*
CS104_Connection_getConnections_IP(CS104_Connection self)
{
    if(self == NULL)
        return "";
    return (char*)(self->hostname);
}

int
CS104_Connection_getConnections_Port(CS104_Connection self)
{
    if(self == NULL)
        return 0;
    return self->tcpPort;
}

int //启动链路
CS104_Connection_sendStartDT_mStation(CS104_Connection_mStation self,char* ip,int port)
{
    int con_index;
    con_index = get_connectionIndex_mStation(self,ip,port);

    if(con_index < 0)
    {
       return -1;//没有当前IP:Port对应的设备建立过连接
    }

    if(self == NULL || self->connections_mStation[con_index] == NULL)
    {
        return -1;
    }

    if(CS104_Connection_station(self->connections_mStation[con_index])==false)
    {
        return -1;//设备不在线
    }

    CS104_Connection_sendStartDT(self->connections_mStation[con_index]);
    return 0;
}

int //总召
CS104_Connection_sendInterrogationCommand_mStation(CS104_Connection_mStation self,char* ip,int port, int ca)
{
    int con_index;
    con_index = get_connectionIndex_mStation(self,ip,port);

    if(con_index < 0)
    {
       return -1;//没有当前IP:Port对应的设备建立过连接
    }

    if(self == NULL || self->connections_mStation[con_index] == NULL)
    {
        return -1;
    }

    if(CS104_Connection_station(self->connections_mStation[con_index])==false)
    {
        return -1;//设备不在线
    }

    CS104_Connection_sendInterrogationCommand(self->connections_mStation[con_index], CS101_COT_ACTIVATION, ca, IEC60870_QOI_STATION);

    return 0;
}

int // 测试帧
CS104_Connection_sendTestCommand_mStation(CS104_Connection_mStation self,char* ip,int port, int ca)
{
    int con_index;
    con_index = get_connectionIndex_mStation(self,ip,port);

    if(con_index < 0)
    {
       return -1;//没有当前IP:Port对应的设备建立过连接
    }

    if(self == NULL || self->connections_mStation[con_index] == NULL)
    {
        return -1;
    }

    if(CS104_Connection_station(self->connections_mStation[con_index])==false)
    {
        return -1;//设备不在线
    }

    CS104_Connection_sendTestCommand(self->connections_mStation[con_index],ca);

    return 0;
}

int //对时
CS104_Connection_sendClockSyncCommand_SetandRead_mStation(CS104_Connection_mStation self,char* ip,int port, int ca, int cot,unsigned char* time)
{
    int con_index;
    con_index = get_connectionIndex_mStation(self,ip,port);

    if(con_index < 0)
    {
       return -1;//没有当前IP:Port对应的设备建立过连接
    }

    if(self == NULL || self->connections_mStation[con_index] == NULL)
    {
        return -1;
    }

    if(CS104_Connection_station(self->connections_mStation[con_index])==false)
    {
        return -1;//设备不在线
    }

    CS104_Connection_sendClockSyncCommand_SetandRead(self->connections_mStation[con_index],ca,cot,time);

    return 0;

}




static void
CS104_Connection_destroy_mStation_con(CS104_Connection self)
{
    if(self)
    {
        if(self->socket)
        {
            Socket_destroy(self->socket);
        }
        CS104_Connection_close(self);

        if (self->sentASDUs != NULL)
            GLOBAL_FREEMEM(self->sentASDUs);

    #if (CONFIG_USE_THREADS == 1)
        Semaphore_destroy(self->sentASDUsLock);
    #endif

        GLOBAL_FREEMEM(self);
    }

}

static void
CS104_Connection_removeConnection(CS104_Connection_mStation self, CS104_Connection connection)
{
    if(!(self||connection))
        return;
#if (CONFIG_USE_THREADS)
    Semaphore_wait(self->openConnectionsLock);
#endif

    //CS104_Connection_destroy_mStation_one(connection);
    //CS104_Connection_destroy_mStation_con(connection);
    CS104_Connection_close_mStation(connection);

    if(self->masterConnections_mStation)
        LinkedList_remove(self->masterConnections_mStation, (void*) connection);
    self->openConnections--;

#if (CONFIG_USE_THREADS)
    Semaphore_post(self->openConnectionsLock);
#endif


}

void
CS104_Connection_removeConnection_mStation(CS104_Connection_mStation self, CS104_Connection connection)
{
    if(self != NULL && connection != NULL)
    {
        CS104_Connection_removeConnection(self, connection);
        //connection = NULL ;
    }

}

//static void
//CS104_Connection_destroy(CS104_Connection self)
//{
//    if (self) {

//#if (CONFIG_CS104_SUPPORT_TLS == 1)
//      if (self->tlsSocket != NULL)
//          TLSSocket_close(self->tlsSocket);
//#endif

//        Socket_destroy(self->socket);

//        GLOBAL_FREEMEM(self->sentASDUs);

//#if (CONFIG_USE_THREADS == 1)
//        Semaphore_destroy(self->sentASDUsLock);
//#endif

//        GLOBAL_FREEMEM(self);
//    }
//}



//===============  104 connection : as server and as master station ==============//


//===============  104 connection : as client and as slave device(simulation equipment) ==============//
typedef struct {
    uint8_t msg[256];
    int msgSize;
} FrameBuffer;
/********************************************
 * IMasterConnection(from cs104_slave)
 *******************************************/
static int
sendIMessage_sDEV(CS104_Connection self, uint8_t* buffer, int msgSize)
{
//    buffer[0] = (uint8_t) 0x68;
//    buffer[1] = (uint8_t) (msgSize - 2);

//    buffer[2] = (uint8_t) ((self->sendCount % 128) * 2);
//    buffer[3] = (uint8_t) (self->sendCount / 128);

//    buffer[4] = (uint8_t) ((self->receiveCount % 128) * 2);
//    buffer[5] = (uint8_t) (self->receiveCount / 128);

    int len=0;

    buffer[len++] = (uint8_t) 0x68;
    //buffer[len++] = (uint8_t) (msgSize - 2);
    if(self->baseProtocalType == 2)//104+
    {
        buffer[len++] = (uint8_t) ((msgSize - 3)%256);
        buffer[len++] = (uint8_t) ((msgSize - 3)/256);
    }
    else
    {
        buffer[len++] = (uint8_t) (msgSize - 2);
    }

    buffer[len++] = (uint8_t) ((self->sendCount % 128) * 2);
    buffer[len++] = (uint8_t) (self->sendCount / 128);

    buffer[len++] = (uint8_t) ((self->receiveCount % 128) * 2);
    buffer[len++] = (uint8_t) (self->receiveCount / 128);



    if(self->connectionType == 3)//串口
    {
        SerialPort_write(self->commPort, buffer, 0, msgSize);
    }
    else
    {
        if (writeToSocket(self, buffer, msgSize) != -1) {
            DEBUG_PRINT("SEND I (size = %i) N(S) = %i N(R) = %i\n", msgSize, self->sendCount, self->receiveCount);
            self->sendCount = (self->sendCount + 1) % 32768;
            self->unconfirmedReceivedIMessages = 0;
        }
        else
            self->running = false;
    }


    self->unconfirmedReceivedIMessages = 0;

    return self->sendCount;
}

static void
sendASDU_sDEV(CS104_Connection self, FrameBuffer* asdu, uint64_t timestamp, int index)
{
    int currentIndex = 0;

    if (self->oldestSentASDU == -1) {
        self->oldestSentASDU = 0;
        self->newestSentASDU = 0;
    }
    else {
        currentIndex = (self->newestSentASDU + 1) % self->maxSentASDUs;
    }

    //self->sentASDUs[currentIndex].entryTime = timestamp;
    //self->sentASDUs[currentIndex].queueIndex = index;
    self->sentASDUs[currentIndex].seqNo = sendIMessage_sDEV(self, asdu->msg, asdu->msgSize);
    self->sentASDUs[currentIndex].sentTime = Hal_getTimeInMs();

    self->newestSentASDU = currentIndex;

    //printSendBuffer(self);
}

static bool
sendASDUInternal_sDEV(CS104_Connection self, CS101_ASDU asdu)
{
    bool asduSent;

    if (self->isActive) {

#if (CONFIG_USE_THREADS == 1)
        Semaphore_wait(self->sentASDUsLock);
#endif

        if (isSentBufferFull(self) == false) {

            FrameBuffer frameBuffer;

            struct sBufferFrame bufferFrame;

            Frame frame = BufferFrame_initialize(&bufferFrame, frameBuffer.msg, IEC60870_5_104_APCI_LENGTH);
            CS101_ASDU_encode(asdu, frame);

            frameBuffer.msgSize = Frame_getMsgSize(frame);

            sendASDU_sDEV(self, &frameBuffer, 0, -1);

#if (CONFIG_USE_THREADS == 1)
            Semaphore_post(self->sentASDUsLock);
#endif

            asduSent = true;
        }
        else {
#if (CONFIG_USE_THREADS == 1)
            Semaphore_post(self->sentASDUsLock);
#endif
            //asduSent = HighPriorityASDUQueue_enqueue(self->highPrioQueue, asdu);
            asduSent = false;
        }

    }
    else
        asduSent = false;

    if (asduSent == false)
        DEBUG_PRINT("unable to send response (isActive=%i)\n", self->isActive);

    return asduSent;
}

static void
_sendASDU(IMasterConnection self, CS101_ASDU asdu)
{
    CS104_Connection con = (CS104_Connection) self->object;

    sendASDUInternal_sDEV(con, asdu);
}

static void
_sendACT_CON(IMasterConnection self, CS101_ASDU asdu, bool negative)
{
    CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);
    CS101_ASDU_setNegative(asdu, negative);

    _sendASDU(self, asdu);
}

static void
_sendACT_TERM(IMasterConnection self, CS101_ASDU asdu)
{
    CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_TERMINATION);
    CS101_ASDU_setNegative(asdu, false);

    _sendASDU(self, asdu);
}

static CS101_AppLayerParameters
_getApplicationLayerParameters(IMasterConnection self)
{
    CS104_Connection con = (CS104_Connection) self->object;

    return &(con->alParameters);
}

/********************************************
 * END IMasterConnection
 *******************************************/

CS104_Connection
CS104_Connection_create_sDEV(const char* hostname, int tcpPort)
{
    CS104_Connection self = (CS104_Connection) GLOBAL_MALLOC(sizeof(struct sCS104_Connection));

    if (self != NULL) {
        strncpy(self->hostname, hostname, HOST_NAME_MAX);
        self->tcpPort = tcpPort;
        self->parameters = defaultAPCIParameters_sDEV;//defaultAPCIParameters
        self->alParameters = defaultAppLayerParameters;

        self->receivedHandler = NULL;
        self->receivedHandlerParameter = NULL;

        self->connectionHandler = NULL;
        self->connectionHandlerParameter = NULL;

        self->msgreceivedHandler = NULL;
        self->msgreceivedHandlerParameter = NULL;
        self->msgsendHandler = NULL;
        self->msgsendHandlerParameter = NULL;
        //
        self->msgsendHandler_mStation = NULL;
        self->msgsendHandlerParameter_mStation = NULL;
        self->msgreceivedHandler_mStation = NULL;
        self->msgreceivedHandlerParameter_mStation = NULL;

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

        //slave DEV
        self->asduHandler = NULL;
        self->interrogationHandler = NULL;
        self->counterInterrogationHandler = NULL;
        self->readHandler = NULL;
        self->clockSyncHandler = NULL;
        self->resetProcessHandler = NULL;
        self->delayAcquisitionHandler = NULL;
        //self->connectionRequestHandler = NULL;
#if (CONFIG_USE_THREADS == 1)
        self->sentASDUsLock_sDEV = Semaphore_create(1);
        //self->connectionHandlingThread_sDEV = NULL;
#endif

        self->iMasterConnection.object = self;
        self->iMasterConnection.getApplicationLayerParameters = _getApplicationLayerParameters;
        self->iMasterConnection.sendASDU = _sendASDU;
        self->iMasterConnection.sendACT_CON = _sendACT_CON;
        self->iMasterConnection.sendACT_TERM = _sendACT_TERM;

        self->baseProtocalType = 1;//基本规约类型：1-104   2-104+
        self->shortframesize = 6;
        prepareSMessage(self->sMessage);
    }

    return self;
}

void
CS104_Connection_setAsduHandler_sDEV(CS104_Connection self, CS101_ASDUHandler handler, void* parameter)
{
    self->asduHandler = handler;
    self->asduHandlerParameter = parameter;
}

void
CS104_Connection_InterrogationHandler_sDEV(CS104_Connection self, CS101_InterrogationHandler handler, void* parameter)
{
    self->interrogationHandler = handler;
    self->interrogationHandlerParameter = parameter;
}

void
CS104_Connection_CounterInterrogationHandler_sDEV(CS104_Connection self, CS101_CounterInterrogationHandler handler, void* parameter)
{
    self->counterInterrogationHandler = handler;
    self->counterInterrogationHandlerParameter = parameter;
}

void
CS104_Connection_ReadHandler_sDEV(CS104_Connection self, CS101_ReadHandler handler, void* parameter)
{
    self->readHandler = handler;
    self->readHandlerParameter = parameter;
}

void
CS104_Connection_ClockSynchronizationHandler_sDEV(CS104_Connection self, CS101_ClockSynchronizationHandler handler, void* parameter)
{
    self->clockSyncHandler = handler;
    self->clockSyncHandlerParameter = parameter;
}

void
CS104_Connection_ResetProcessHandler_sDEV(CS104_Connection self, CS101_ResetProcessHandler handler, void* parameter)
{
    self->resetProcessHandler = handler;
    self->resetProcessHandlerParameter = parameter;
}

void
CS104_Connection_DelayAcquisitionHandler_sDEV(CS104_Connection self, CS101_DelayAcquisitionHandler handler, void* parameter)
{
    self->delayAcquisitionHandler = handler;
    self->delayAcquisitionHandlerParameter = parameter;
}

static void responseCOTUnknown(CS101_ASDU asdu, CS104_Connection self)
{
    DEBUG_PRINT("  with unknown COT\n");
    CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);
    sendASDUInternal_sDEV(self, asdu);
}

static void
handleASDU_sDEV(CS104_Connection self, CS101_ASDU asdu)
{
    bool messageHandled = false;
    uint8_t cot = CS101_ASDU_getCOT(asdu);
    switch (CS101_ASDU_getTypeID(asdu))
    {
        case C_IC_NA_1: /* 100 - interrogation command */

            DEBUG_PRINT("Rcvd interrogation command C_IC_NA_1\n");

            if ((cot == CS101_COT_ACTIVATION) || (cot == CS101_COT_DEACTIVATION)) {
                if (self->interrogationHandler != NULL) {

                    InterrogationCommand irc = (InterrogationCommand) CS101_ASDU_getElement(asdu, 0);

                    if (self->interrogationHandler(self->interrogationHandlerParameter,
                            &(self->iMasterConnection), asdu, InterrogationCommand_getQOI(irc)))
                        messageHandled = true;

                    InterrogationCommand_destroy(irc);
                }
            }
            else
                responseCOTUnknown(asdu, self);

            break;

        case C_CI_NA_1: /* 101 - counter interrogation command */

            DEBUG_PRINT("Rcvd counter interrogation command C_CI_NA_1\n");

            if ((cot == CS101_COT_ACTIVATION) || (cot == CS101_COT_DEACTIVATION)) {

                if (self->counterInterrogationHandler != NULL) {

                    CounterInterrogationCommand cic = (CounterInterrogationCommand) CS101_ASDU_getElement(asdu, 0);


                    if (self->counterInterrogationHandler(self->counterInterrogationHandlerParameter,
                            &(self->iMasterConnection), asdu, CounterInterrogationCommand_getQCC(cic)))
                        messageHandled = true;

                    CounterInterrogationCommand_destroy(cic);
                }
            }
            else
                responseCOTUnknown(asdu, self);

            break;

        case C_RD_NA_1: /* 102 - read command */

            DEBUG_PRINT("Rcvd read command C_RD_NA_1\n");

            if (cot == CS101_COT_REQUEST) {
                if (self->readHandler != NULL) {
                    ReadCommand rc = (ReadCommand) CS101_ASDU_getElement(asdu, 0);

                    if (self->readHandler(self->readHandlerParameter,
                            &(self->iMasterConnection), asdu, InformationObject_getObjectAddress((InformationObject) rc)))
                        messageHandled = true;

                    ReadCommand_destroy(rc);
                }
            }
            else
                responseCOTUnknown(asdu, self);

            break;

        case C_CS_NA_1: /* 103 - Clock synchronization command */

            DEBUG_PRINT("Rcvd clock sync command C_CS_NA_1\n");

            if (cot == CS101_COT_ACTIVATION || cot == CS101_COT_REQUEST) {//set or read time

                if (self->clockSyncHandler != NULL) {

                    ClockSynchronizationCommand csc = (ClockSynchronizationCommand) CS101_ASDU_getElement(asdu, 0);

                    if (self->clockSyncHandler(self->clockSyncHandlerParameter,
                            &(self->iMasterConnection), asdu, ClockSynchronizationCommand_getTime(csc)))
                        messageHandled = true;

                    ClockSynchronizationCommand_destroy(csc);
                }
            }
            else
                responseCOTUnknown(asdu, self);

            break;

        case C_TS_NA_1: /* 104 - test command */

            DEBUG_PRINT("Rcvd test command C_TS_NA_1\n");

            if (cot != CS101_COT_ACTIVATION)
                CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);
            else
                CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);

            sendASDUInternal_sDEV(self, asdu);

            messageHandled = true;

            break;

        case C_RP_NA_1: /* 105 - Reset process command */

            DEBUG_PRINT("Rcvd reset process command C_RP_NA_1\n");

            if (cot == CS101_COT_ACTIVATION) {

                if (self->resetProcessHandler != NULL) {
                    ResetProcessCommand rpc = (ResetProcessCommand) CS101_ASDU_getElement(asdu, 0);

                    if (self->resetProcessHandler(self->resetProcessHandlerParameter,
                            &(self->iMasterConnection), asdu, ResetProcessCommand_getQRP(rpc)))
                        messageHandled = true;

                    ResetProcessCommand_destroy(rpc);
                }

            }
            else
                responseCOTUnknown(asdu, self);

            break;

        case C_CD_NA_1: /* 106 - Delay acquisition command */

            DEBUG_PRINT("Rcvd delay acquisition command C_CD_NA_1\n");

            if ((cot == CS101_COT_ACTIVATION) || (cot == CS101_COT_SPONTANEOUS)) {

                if (self->delayAcquisitionHandler != NULL) {
                    DelayAcquisitionCommand dac = (DelayAcquisitionCommand) CS101_ASDU_getElement(asdu, 0);

                    if (self->delayAcquisitionHandler(self->delayAcquisitionHandlerParameter,
                            &(self->iMasterConnection), asdu, DelayAcquisitionCommand_getDelay(dac)))
                        messageHandled = true;

                    DelayAcquisitionCommand_destroy(dac);
                }
            }
            else
                responseCOTUnknown(asdu, self);

            break;

        default: /* no special handler available -> use default handler */
            break;
    }

    if ((messageHandled == false) && (self->asduHandler != NULL))
        if (self->asduHandler(self->asduHandlerParameter, &(self->iMasterConnection), asdu))
            messageHandled = true;

    if (messageHandled == false) {
        /* send error response */
        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_TYPE_ID);
        sendASDUInternal_sDEV(self, asdu);
    }
}

static bool
handleMessage_sDEV(CS104_Connection self, uint8_t* buffer, int msgSize)
{
    if (msgSize > 0) {
        if (self->msgreceivedHandler != NULL)
            self->msgreceivedHandler(self->msgreceivedHandlerParameter, buffer, msgSize);

    }

    uint64_t currentTime = Hal_getTimeInMs();

    unsigned char apduStartBuf;
    if(self->baseProtocalType == 2)
        apduStartBuf = buffer[3];
    else
        apduStartBuf = buffer[2];

    if ((apduStartBuf & 1) == 0) {//buffer[2]

        if (msgSize < 7) {
            DEBUG_PRINT("Received I msg too small!\n");
            return false;
        }

        if (self->firstIMessageReceived == false) {
            self->firstIMessageReceived = true;
            self->lastConfirmationTime = currentTime; /* start timeout T2 */
        }

//        int frameSendSequenceNumber = ((buffer [3] * 0x100) + (buffer [2] & 0xfe)) / 2;
//        int frameRecvSequenceNumber = ((buffer [5] * 0x100) + (buffer [4] & 0xfe)) / 2;
        int frameSendSequenceNumber = 0;
        int frameRecvSequenceNumber = 0;
        if(self->baseProtocalType == 2)//104+
        {
            frameSendSequenceNumber = ((buffer [4] * 0x100) + (buffer [3] & 0xfe)) / 2;
            frameRecvSequenceNumber = ((buffer [6] * 0x100) + (buffer [5] & 0xfe)) / 2;

        }
        else
        {
            frameSendSequenceNumber = ((buffer [3] * 0x100) + (buffer [2] & 0xfe)) / 2;
            frameRecvSequenceNumber = ((buffer [5] * 0x100) + (buffer [4] & 0xfe)) / 2;
        }

        DEBUG_PRINT("Received I frame: N(S) = %i N(R) = %i\n", frameSendSequenceNumber, frameRecvSequenceNumber);

        if (frameSendSequenceNumber != self->receiveCount) {
            DEBUG_PRINT("Sequence error: Close connection!");
            //return false;
        }

        if (checkSequenceNumber (self, frameRecvSequenceNumber) == false) {
            DEBUG_PRINT("Sequence number check failed");
            //return false;
        }

        self->receiveCount = (self->receiveCount + 1) % 32768;
        self->unconfirmedReceivedIMessages++;

        if (self->isActive) {

            CS101_ASDU asdu = CS101_ASDU_createFromBuffer(&(self->alParameters), buffer + 6, msgSize - 6);

            handleASDU_sDEV(self, asdu);

            CS101_ASDU_destroy(asdu);
        }
        else
            DEBUG_PRINT("Connection not activated. Skip I message");

    }

    /* Check for TESTFR_ACT message */
    else if ((apduStartBuf & 0x43) == 0x43) {//buffer[2]
        DEBUG_PRINT("Send TESTFR_CON\n");

        if(self->connectionType == 3)//串口
        {
            if(self->baseProtocalType == 2)
            {
                SerialPort_write(self->commPort, TESTFR_CON_MSG_104P, 0, TESTFR_CON_MSG_SIZE_104P);
            }
            else
                SerialPort_write(self->commPort, TESTFR_CON_MSG, 0, TESTFR_CON_MSG_SIZE);
        }
        else
        {
            if(self->baseProtocalType == 2)
            {
                writeToSocket(self, TESTFR_CON_MSG_104P, TESTFR_CON_MSG_SIZE_104P);
            }
            else
                writeToSocket(self, TESTFR_CON_MSG, TESTFR_CON_MSG_SIZE);
        }
    }

    /* Check for STARTDT_ACT message */
    else if ((apduStartBuf & 0x07) == 0x07) {//buffer [2]
        DEBUG_PRINT("Send STARTDT_CON\n");
        self->isActive = true;
        self->receiveCount = 0;//clear seqnum
        self->sendCount = 0;
        self->unconfirmedReceivedIMessages = 0;

        if(self->connectionType == 3)//串口
        {
            if(self->baseProtocalType == 2)
            {
                SerialPort_write(self->commPort, STARTDT_CON_MSG_104P, 0, STARTDT_CON_MSG_SIZE_104P);
            }
            else
                SerialPort_write(self->commPort, STARTDT_CON_MSG, 0, STARTDT_CON_MSG_SIZE);
        }
        else
        {
            if(self->baseProtocalType == 2)
            {
                writeToSocket(self, STARTDT_CON_MSG_104P, STARTDT_CON_MSG_SIZE_104P);
            }
            else
                writeToSocket(self, STARTDT_CON_MSG, STARTDT_CON_MSG_SIZE);
        }
    }

    /* Check for STOPDT_ACT message */
    else if ((apduStartBuf & 0x13) == 0x13) {//buffer [2]
        DEBUG_PRINT("Send STOPDT_CON\n");

        self->isActive = false;

        if(self->connectionType == 3)//串口
        {
            if(self->baseProtocalType == 2)
            {
                SerialPort_write(self->commPort, STOPDT_CON_MSG_104P, 0, STOPDT_CON_MSG_SIZE_104P);
            }
            else
                SerialPort_write(self->commPort, STOPDT_CON_MSG, 0, STOPDT_CON_MSG_SIZE);
        }
        else
        {
            if(self->baseProtocalType == 2)
            {
                writeToSocket(self, STOPDT_CON_MSG_104P, STOPDT_CON_MSG_SIZE_104P);
            }
            else
                writeToSocket(self, STOPDT_CON_MSG, STOPDT_CON_MSG_SIZE);
        }
    }

    /* Check for TESTFR_CON message */
    else if ((apduStartBuf & 0x83) == 0x83) {//buffer[2]
        DEBUG_PRINT("Recv TESTFR_CON\n");

        //self->outstandingTestFRConMessages = 0;
        self->outstandingTestFCConMessages = 0;
    }

    else if (apduStartBuf == 0x01) {//buffer [2] /* S-message */
        //int seqNo = (buffer[4] + buffer[5] * 0x100) / 2;
        int seqNo;
        if(self->baseProtocalType == 2)//104+
        {
            seqNo = (buffer[5] + buffer[6] * 0x100) / 2;
        }
        else
        {
           seqNo = (buffer[4] + buffer[5] * 0x100) / 2;
        }

        DEBUG_PRINT("Rcvd S(%i) (own sendcounter = %i)\n", seqNo, self->sendCount);

        if (checkSequenceNumber(self, seqNo) == false)
        {
            DEBUG_PRINT("checkMessage return false(S-message),Sequence error: Close connection!\n");
            return false;
        }
    }
    else {
        DEBUG_PRINT("unknown message - IGNORE\n");
        return true;
    }

    resetT3Timeout(self);

    return true;
}

static bool
handleTimeouts_sDEV(CS104_Connection self)
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
            //\n 长期空闲状态下发送测试帧的超时(设备端主动发出)
            if(self->connectionType == 3)//串口
            {
                if(self->baseProtocalType == 2)
                {
                    SerialPort_write(self->commPort, TESTFR_ACT_MSG_104P, 0, TESTFR_ACT_MSG_SIZE_104P);
                }
                else
                    SerialPort_write(self->commPort, TESTFR_ACT_MSG, 0, TESTFR_ACT_MSG_SIZE);
            }
            else
            {
                if(self->baseProtocalType == 2)
                {
                    writeToSocket(self, TESTFR_ACT_MSG_104P, TESTFR_ACT_MSG_SIZE_104P);
                }
                else
                    writeToSocket(self, TESTFR_ACT_MSG, TESTFR_ACT_MSG_SIZE);
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
            DEBUG_PRINT("U message T1 timeout\n");//
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
handleConnection_sDEV(void* parameter)
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

                    if (bytesRec <= -1) {
                        loopRunning = false;
                        self->failure = true;//perror();strerror(errno);
                        DEBUG_PRINT("Error reading from socket,loopRunning is set false,receiveMessage bytesRec == %d,strerror(errno): %s\n",bytesRec,strerror(errno));//
                    }

                    if (bytesRec > 0) {
                        DEBUG_PRINT("Connection: rcvd msg(%i bytes)\n", bytesRec);

                        if (handleMessage_sDEV(self, buffer, bytesRec) == false) {
                            /* close connection on error */
                            loopRunning= false;
                            self->failure = true;
                            DEBUG_PRINT("loopRunning is set false,checkMessage(self, buffer, bytesRec) is false\n");//
                        }
                    }

                    if (self->unconfirmedReceivedIMessages >= self->parameters.w) {
                        self->lastConfirmationTime = Hal_getTimeInMs();
                        self->unconfirmedReceivedIMessages = 0;
                        sendSMessage(self);
                    }
                }

                if (handleTimeouts_sDEV(self) == false)
                {
                    self->running = false;
                    loopRunning = false;
                    DEBUG_PRINT("loopRunning is set false,handleTimeouts(self) is false\n");//
                }

                if (self->close)
                {
                    self->running = false;
                    loopRunning = false;
                    DEBUG_PRINT("loopRunning is set false,self->close is true\n");//
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

    DEBUG_PRINT("EXIT CONNECTION HANDLING THREAD!!!\n");//
    if (self->importantInfoHandler != NULL)
        self->importantInfoHandler(self->importantInfoHandlerParameter, "EXIT CONNECTION HANDLING THREAD!");

    self->running = false;

    return NULL;
}

bool
CS104_Connection_connect_sDEV(CS104_Connection self)
{
    resetConnection(self);

    resetT3Timeout(self);

    //self->connectionHandlingThread_sDEV = Thread_create(handleConnection_sDEV, (void*) self, false);
    self->connectionHandlingThread = Thread_create(handleConnection_sDEV, (void*) self, false);
    if (self->connectionHandlingThread)
        Thread_start(self->connectionHandlingThread);

    while ((self->running == false) && (self->failure == false))
        Thread_sleep(1);

    return self->running;
}


//===============  104 connection : as client and as slave device(simulation equipment) ==============//
