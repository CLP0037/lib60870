﻿/*
 *  Copyright 2016-2018 MZ Automation GmbH
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

#ifndef SRC_INC_CS104_CONNECTION_H_
#define SRC_INC_CS104_CONNECTION_H_

#include <stdbool.h>
#include <stdint.h>
#include "tls_api.h"
#include "iec60870_master.h"
#include "iec60870_slave.h"//
#include "frame.h"
#include "buffer_frame.h"

#include "hal_serial.h"  // for serialport

#include "lib60870_config.h"
#if (CONFIG_USE_THREADS == 0)//0//
#include <uv.h>
//#else
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file cs104_connection.h
 * \brief CS 104 master side definitions
 */

/**
 * @addtogroup MASTER Master related functions
 *
 * @{
 */

/**
 * @defgroup CS104_MASTER CS 104 master related functions
 *
 * @{
 */

typedef struct sCS104_Connection* CS104_Connection;

typedef struct sCS104_Connection_mStation* CS104_Connection_mStation;

/**
 * \brief Connection request handler is called when a client tries to connect to the server.
 *
 * \param parameter user provided parameter
 * \param ipAddress string containing IP address and TCP port number (e.g. "192.168.1.1:34521")
 *
 * \return true to accept the connection request, false to deny
 */
typedef bool (*CS104_ConnectionRequestHandler_mStation) (void* parameter, const char* ipAddress, int port);

typedef bool (*CS104_ConnectionBrokenHandler_mStation) (void* parameter, const char* ipAddress, int port);

/**
 * \brief Create a new connection object
 *
 * \param hostname host name of IP address of the server to connect
 * \param tcpPort tcp port of the server to connect. If set to -1 use default port (2404)
 *
 * \return the new connection object
 */
CS104_Connection
CS104_Connection_create(const char* hostname, int tcpPort);

//SerialPort commPort
bool CS104_Connection_getCommState(CS104_Connection self);

CS104_Connection
CS104_Connection_create_comm(SerialPort serialPort);

CS104_Connection
CS104_Connection_create_mStation(CS104_Connection_mStation slave, Socket socket);

/**
 * \brief Create a new secure connection object (uses TLS)
 *
 * \param hostname host name of IP address of the server to connect
 * \param tcpPort tcp port of the server to connect. If set to -1 use default port (19998)
 * \param tlcConfig the TLS configuration (certificates, keys, and parameters)
 *
 * \return the new connection object
 */
CS104_Connection
CS104_Connection_createSecure(const char* hostname, int tcpPort, TLSConfiguration tlsConfig);

void
CS104_Connection_setProtocalType(CS104_Connection self, unsigned char baseProtocalType);

unsigned char
CS104_Connection_getProtocalType(CS104_Connection self);

int
CS104_Connection_getunconfirmedSendIMessages(CS104_Connection self);

/**
 * \brief Set the CS104 specific APCI parameters.
 *
 * If not set the default parameters are used. This function must be called before the
 * CS104_Connection_connect function is called! If the function is called after the connect
 * the behavior is undefined.
 *
 * \param self CS104_Connection instance
 */
void
CS104_Connection_setAPCIParameters(CS104_Connection self, CS104_APCIParameters parameters);

/**
 * \brief Get the currently used CS104 specific APCI parameters
 */
CS104_APCIParameters
CS104_Connection_getAPCIParameters(CS104_Connection self);

/**
 * \brief Set the CS101 application layer parameters
 *
 * If not set the default parameters are used. This function must be called before the
 * CS104_Connection_connect function is called! If the function is called after the connect
 * the behavior is undefined.
 *
 * \param self CS104_Connection instance
 * \param parameters the application layer parameters
 */
void
CS104_Connection_setAppLayerParameters(CS104_Connection self, CS101_AppLayerParameters parameters);

/**
 * \brief Return the currently used application layer parameter
 *
 * NOTE: The application layer parameters are required to create CS101_ASDU objects.
 *
 * \param self CS104_Connection instance
 *
 * \return the currently used CS101_AppLayerParameters object
 */
CS101_AppLayerParameters
CS104_Connection_getAppLayerParameters(CS104_Connection self);

/**
 * \brief Sets the timeout for connecting to the server (in ms)
 *
 * \param self
 * \param millies timeout value in ms
 */
void
CS104_Connection_setConnectTimeout(CS104_Connection self, int millies);

/**
 * \brief non-blocking connect.
 *
 * Invokes a connection establishment to the server and returns immediately.
 *
 * \param self CS104_Connection instance
 */
void
CS104_Connection_connectAsync(CS104_Connection self);

/**
 * \brief blocking connect
 *
 * Establishes a connection to a server. This function is blocking and will return
 * after the connection is established or the connect timeout elapsed.
 *
 * \param self CS104_Connection instance
 * \return true when connected, false otherwise
 */
bool
CS104_Connection_connect(CS104_Connection self);

bool
CS104_Connection_connect_comm(CS104_Connection self);

bool
CS104_Connection_connect_mStation(CS104_Connection self);

bool
CS104_Connection_station(CS104_Connection self);//获取当前连接状态

/**
 * \brief start data transmission on this connection
 *
 * After issuing this command the client (master) will receive spontaneous
 * (unsolicited) messages from the server (slave).
 */
void
CS104_Connection_sendStartDT(CS104_Connection self);

/**
 * \brief stop data transmission on this connection
 */
void
CS104_Connection_sendStopDT(CS104_Connection self);

/**
 * \brief Check if the transmit (send) buffer is full. If true the next send command will fail.
 *
 * The transmit buffer is full when the slave/server didn't confirm the last k sent messages.
 * In this case the next message can only be sent after the next confirmation (by I or S messages)
 * that frees part of the sent messages buffer.
 */
bool
CS104_Connection_isTransmitBufferFull(CS104_Connection self);

/**
 * \brief send an interrogation command
 *
 * \param cot cause of transmission
 * \param ca Common address of the slave/server
 * \param qoi qualifier of interrogation (20 for station interrogation)
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendInterrogationCommand(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, QualifierOfInterrogation qoi);

bool
CS104_Connection_sendSoftwareUpgrade(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, uint8_t ctype);

/**
 * \brief send a counter interrogation command
 *
 * \param cot cause of transmission
 * \param ca Common address of the slave/server
 * \param qcc
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendCounterInterrogationCommand(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, uint8_t qcc);

/**
 * \brief  Sends a read command (C_RD_NA_1 typeID: 102)
 *
 * This will send a read command C_RC_NA_1 (102) to the slave/outstation. The COT is always REQUEST (5).
 * It is used to implement the cyclical polling of data application function.
 *
 * \param ca Common address of the slave/server
 * \param ioa Information object address of the data point to read
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendReadCommand(CS104_Connection self, int ca, int ioa);

/**
 * \brief Sends a clock synchronization command (C_CS_NA_1 typeID: 103)
 *
 * \param ca Common address of the slave/server
 * \param time new system time for the slave/server
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendClockSyncCommand(CS104_Connection self, int ca, CP56Time2a time);
bool
CS104_Connection_sendClockSyncCommand_SetandRead(CS104_Connection self, int ca, int cot,unsigned char* time);
/**
 * \brief Send a test command (C_TS_NA_1 typeID: 104)
 *
 * Note: This command is not supported by IEC 60870-5-104
 *
 * \param ca Common address of the slave/server
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendTestCommand(CS104_Connection self, int ca);


/**
 * \brief Send a reset process command (C_RP_NA_1 typeID: 105)
 *
 * \param ca Common address of the slave/server
 * \param qrp 复位进程命令限定词 QRP
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendResetprocessCommand(CS104_Connection self, int ca, int qrp);//复位进程







/**
 * \brief Send a process command to the controlled (or other) station
 *
 * \param cot the cause of transmission (should be ACTIVATION to select/execute or ACT_TERM to cancel the command)
 * \param ca the common address of the information object
 * \param command the command information object (e.g. SingleCommand or DoubleCommand)
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendProcessCommand(CS104_Connection self, TypeID typeId, CS101_CauseOfTransmission cot,
        int ca, InformationObject command);

//===遥测置数
bool
CS104_Connection_sendSetpointCommand(CS104_Connection self, int ca, int ti,int ioa,char* value);


//远程参数和数据读写
bool
CS104_Connection_sendParamValueRead(CS104_Connection self, CS101_CauseOfTransmission cot, int vsq,int ca, int sn,InformationObject sc);

bool
CS104_Connection_sendParamValueWrite(CS104_Connection self, CS101_CauseOfTransmission cot, int vsq,int ca, int sn,int paramti, InformationObject sc);

//文件服务====
bool
CS104_Connection_sendFileservingCommand(CS104_Connection self, CS101_CauseOfTransmission cot, int ca, InformationObject sc);

//自定义报文组包====
bool CS104_Connection_sendSelfdefinedFrame(CS104_Connection self,unsigned char* buf,int len);

/**
 * \brief Send a user specified ASDU
 *
 * \param asdu the ASDU to send
 *
 * \return true if message was sent, false otherwise
 */
bool
CS104_Connection_sendASDU(CS104_Connection self, CS101_ASDU asdu);

/**
 * \brief Register a callback handler for received ASDUs
 *
 * \param handler user provided callback handler function
 * \param parameter user provided parameter that is passed to the callback handler
 */
void
CS104_Connection_setASDUReceivedHandler(CS104_Connection self, CS101_ASDUReceivedHandler handler, void* parameter);

void
CS104_Connection_setMSGReceivedHandler(CS104_Connection self, CS104_MSGReceivedHandler handler, void* parameter);
void
CS104_Connection_setMSGSendHandler(CS104_Connection self, CS104_MSGSendHandler handler, void* parameter);
void
CS104_Connection_setImportantInfoHandler(CS104_Connection self, CS104_ImportantInfoHandler handler, void* parameter);

void
CS104_Connection_setMSGReceivedHandler_withExplain(CS104_Connection self, CS104_MSGReceivedHandler_withExplain handler, void* parameter);
void
CS104_Connection_setMSGSendHandler_withExplain(CS104_Connection self, CS104_MSGSendHandler_withExplain handler, void* parameter);

typedef enum {
    CS104_CONNECTION_OPENED = 0,
    CS104_CONNECTION_CLOSED = 1,
    CS104_CONNECTION_STARTDT_CON_RECEIVED = 2,
    CS104_CONNECTION_STOPDT_CON_RECEIVED = 3
} CS104_ConnectionEvent;

/**
 * \brief Handler that is called when the connection is established or closed
 *
 * \param parameter user provided parameter
 * \param connection the connection object
 * \param event event type
 */
typedef void (*CS104_ConnectionHandler) (void* parameter, CS104_Connection connection, CS104_ConnectionEvent event);

void
CS104_Connection_setConnectionHandler(CS104_Connection self, CS104_ConnectionHandler handler, void* parameter);

/**
 * \brief Close the connection
 */
void
CS104_Connection_close(CS104_Connection self);

void
CS104_Connection_close_mStation(CS104_Connection self);

/**
 * \brief Close the connection and free all related resources
 */
void
CS104_Connection_destroy(CS104_Connection self);

void
CS104_Connection_destroy_comm(CS104_Connection self);

void
CS104_Connection_destroy_mStation_one(CS104_Connection self);

//===============  104 connection : as server and as master station ==============//

CS104_Connection_mStation
sCS104_Connection_mStation_create(int maxLowPrioQueueSize, int maxHighPrioQueueSize);

/**
 * \brief Set the local IP address to bind the server
 * use "0.0.0.0" to bind to all interfaces
 *
 * \param self the slave instance
 * \param ipAddress the IP address string or hostname
 */
void
CS104_Connection_setLocalAddress(CS104_Connection_mStation self, const char* ipAddress);

/**
 * \brief Set the local TCP port to bind the server
 *
 * \param self the slave instance
 * \param tcpPort the TCP port to use (default is 2404)
 */
void
CS104_Connection_setLocalPort(CS104_Connection_mStation self, int tcpPort);

int
CS104_Connection_getOpenConnections(CS104_Connection_mStation self);

void
CS104_Connection_setConnectionRequestHandler(CS104_Connection_mStation self, CS104_ConnectionRequestHandler_mStation handler, void* parameter);

void
CS104_Connection_setConnectionBrokenHandler(CS104_Connection_mStation self, CS104_ConnectionBrokenHandler_mStation handler, void* parameter);

//void
//CS104_Connection_setASDUReceivedHandler_mStation0(CS104_Connection self, CS101_ASDUReceivedHandler handler, void* parameter);


/**
 * \brief State the CS 104 slave. The slave (server) will listen on the configured TCP/IP port
 *
 * \param self CS104_Slave instance
 */
void
CS104_Connection_start_mStation(CS104_Connection_mStation self);

bool
CS104_Connection_isRunning_mStation(CS104_Connection_mStation self);

void
CS104_Connection_stop_mStation(CS104_Connection_mStation self);

void
CS104_Connection_destroy_mStation(CS104_Connection_mStation self);

void
CS104_Connection_deactivate(CS104_Connection self);

int get_connectionIndex_mStation(CS104_Connection_mStation self,char* ip_temp,int port_temp);

void
CS104_Connection_setMSGReceivedHandler_m(CS104_Connection self, CS104_MSGReceivedHandler_mStation handler, void* parameter);
void
CS104_Connection_setMSGSendHandler_m(CS104_Connection self, CS104_MSGSendHandler_mStation handler, void* parameter);

void
CS104_Connection_setASDUReceivedHandler_mStation(CS104_Connection_mStation self, CS101_ASDUReceivedHandler handler, void* parameter,int con_index);

void
CS104_Connection_setMSGReceivedHandler_mStation(CS104_Connection_mStation self, CS104_MSGReceivedHandler_mStation handler, void* parameter,int con_index);

void
CS104_Connection_setMSGSendHandler_mStation(CS104_Connection_mStation self, CS104_MSGSendHandler_mStation handler, void* parameter,int con_index);

CS104_Connection
get_connection_from_mStation(CS104_Connection_mStation self,int con_index);

char*
CS104_Connection_getConnections_IP(CS104_Connection self);

int
CS104_Connection_getConnections_Port(CS104_Connection self);



int //启动链路
CS104_Connection_sendStartDT_mStation(CS104_Connection_mStation self,char* ip,int port);

int //总召
CS104_Connection_sendInterrogationCommand_mStation(CS104_Connection_mStation self,char* ip,int port, int ca);

int // 测试帧
CS104_Connection_sendTestCommand_mStation(CS104_Connection_mStation self,char* ip,int port, int ca);

int //对时
CS104_Connection_sendClockSyncCommand_SetandRead_mStation(CS104_Connection_mStation self,char* ip,int port, int ca, int cot,unsigned char* time);

void //移除连接
CS104_Connection_removeConnection_mStation(CS104_Connection_mStation self, CS104_Connection connection);
//===============  104 connection : as server and as master station ==============//


//===============  104 connection : as client and as slave device(simulation equipment) ==============//
CS104_Connection
CS104_Connection_create_sDEV(const char* hostname, int tcpPort);

void
CS104_Connection_setAsduHandler_sDEV(CS104_Connection self, CS101_ASDUHandler handler, void* parameter);

void
CS104_Connection_InterrogationHandler_sDEV(CS104_Connection self, CS101_InterrogationHandler handler, void* parameter);

void
CS104_Connection_CounterInterrogationHandler_sDEV(CS104_Connection self, CS101_CounterInterrogationHandler handler, void* parameter);

void
CS104_Connection_ReadHandler_sDEV(CS104_Connection self, CS101_ReadHandler handler, void* parameter);

void
CS104_Connection_ClockSynchronizationHandler_sDEV(CS104_Connection self, CS101_ClockSynchronizationHandler handler, void* parameter);

void
CS104_Connection_ResetProcessHandler_sDEV(CS104_Connection self, CS101_ResetProcessHandler handler, void* parameter);

void
CS104_Connection_DelayAcquisitionHandler_sDEV(CS104_Connection self, CS101_DelayAcquisitionHandler handler, void* parameter);


bool
CS104_Connection_connect_sDEV(CS104_Connection self);







//===============  104 connection : as client and as slave device(simulation equipment) ==============//


//===============  uv connection (connection based on libuv)==============//
#if (CONFIG_USE_THREADS == 0)//

//=============参数设置与获取==============//
int CS104_Connection_get_tcpmallocsize(CS104_Connection self);
int CS104_Connection_get_tcpPort(CS104_Connection self);
char* CS104_Connection_get_hostname(CS104_Connection self);
bool CS104_Connection_get_running(CS104_Connection self);
void CS104_Connection_set_running(CS104_Connection self,bool flag);

int CS104_Connection_getDevlinkfd(CS104_Connection self);
void CS104_Connection_setDevlinkfd(CS104_Connection self,int dev_fd);

int CS104_Connection_getDevlinkindex(CS104_Connection self);
void CS104_Connection_setDevlinkindex(CS104_Connection self,int dev_index);

int CS104_Connection_getDEV_addr(CS104_Connection self);
void CS104_Connection_setDEV_addr(CS104_Connection self,int dev_addr);

int CS104_Connection_getDEV_sn(CS104_Connection self);
void CS104_Connection_setDEV_sn(CS104_Connection self,int dev_sn);

uv_tcp_t* CS104_Connection_get_uv_client(CS104_Connection self);

unsigned int CS104_Connection_getSummonTime(CS104_Connection self);
void CS104_Connection_setSummonTime(CS104_Connection self,unsigned int _count);
void CS104_Connection_setSummonTime_plus(CS104_Connection self);

unsigned int CS104_Connection_getTimeSynTime(CS104_Connection self);
void CS104_Connection_setTimeSynTime(CS104_Connection self,unsigned int _count);
void CS104_Connection_setTimeSynTime_plus(CS104_Connection self);

unsigned int CS104_Connection_getLinkBeatTime(CS104_Connection self);
void CS104_Connection_setLinkBeatTime(CS104_Connection self,unsigned int _count);
void CS104_Connection_setLinkBeatTime_plus(CS104_Connection self);

bool CS104_Connection_getfirsttime_link(CS104_Connection self);
void CS104_Connection_setfirsttime_link(CS104_Connection self,bool flag);
bool CS104_Connection_getfirsttime_DataCall(CS104_Connection self);
void CS104_Connection_setfirsttime_DataCall(CS104_Connection self,bool flag);
 bool CS104_Connection_getfirsttime_ClockSync(CS104_Connection self);
void CS104_Connection_setfirsttime_ClockSync(CS104_Connection self,bool flag);

bool CS104_Connection_getflag_cmdfromhmi_call(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_call(CS104_Connection self,bool flag);
bool CS104_Connection_getflag_cmdfromhmi_time(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_time(CS104_Connection self,bool flag);
bool CS104_Connection_getflag_cmdfromhmi_test(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_test(CS104_Connection self,bool flag);
bool CS104_Connection_getflag_cmdfromhmi_control_select(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_control_select(CS104_Connection self,bool flag);
bool CS104_Connection_getflag_cmdfromhmi_control_exc(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_control_exc(CS104_Connection self,bool flag);
bool CS104_Connection_getflag_cmdfromhmi_control_quit(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_control_quit(CS104_Connection self,bool flag);
bool CS104_Connection_getflag_cmdfromhmi_menu(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_menu(CS104_Connection self,bool flag);
bool CS104_Connection_getflag_cmdfromhmi_file(CS104_Connection self);
void CS104_Connection_setflag_cmdfromhmi_file(CS104_Connection self,bool flag);
//=============参数设置与获取==============//



//连接设备
bool
CS104_Connection_connectAsync_uv(CS104_Connection self ,uv_loop_t* loop);

#endif


/*! @} */

/*! @} */

#ifdef __cplusplus
}
#endif

#endif /* SRC_INC_CS104_CONNECTION_H_ */
