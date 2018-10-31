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

#ifndef SRC_IEC60870_MASTER_H_
#define SRC_IEC60870_MASTER_H_

#include <stdint.h>
#include <stdbool.h>

#include "iec60870_common.h"

/**
 * \file iec60870_master.h
 * \brief Common master side definitions for IEC 60870-5-101/104
 * These types are used by CS101/CS104 master
 */

/**
 * \brief Callback handler for received ASDUs
 *
 * This callback handler will be called for each received ASDU.
 * The CS101_ASDU object that is passed is only valid in the context
 * of the callback function.
 *
 * \param parameter user provided parameter
 * \param address address of the sender (slave/other station) - undefined for CS 104
 * \param asdu object representing the received ASDU
 *
 * \return true if the ASDU has been handled by the callback, false otherwise
 */
typedef bool (*CS101_ASDUReceivedHandler) (void* parameter, int address, CS101_ASDU asdu);

//CS104_MSGReceivedHandler
typedef bool (*CS104_MSGReceivedHandler) (void* parameter, uint8_t* buffer, int msgSize);//, CS101_ASDU asdu

//CS104_MSGSendHandler
typedef bool (*CS104_MSGSendHandler) (void* parameter, uint8_t* buffer, int msgSize);//, CS101_ASDU asdu

//_withExplain
typedef struct
{
    unsigned char 		TI;   //类型标识
    unsigned char 		COT;  //传送原因
    unsigned char 		VSQ;
    unsigned int 		NS;   //发送序号
    unsigned int 		NR;   //接收序号
    unsigned char 		EC;   //element count
    unsigned char 		FT;   //frame type帧类型：1-I帧 2-U帧 3-S帧
}_FRAMESTRUCT104;//规约结构
//CS104_MSGReceivedHandler
typedef bool (*CS104_MSGReceivedHandler_withExplain) (void* parameter, uint8_t* buffer, int msgSize,char* msg_explain,_FRAMESTRUCT104 sframe, CS101_ASDU asdu);

//CS104_MSGSendHandler
typedef bool (*CS104_MSGSendHandler_withExplain) (void* parameter, uint8_t* buffer, int msgSize,char* msg_explain,_FRAMESTRUCT104 sframe, CS101_ASDU asdu);//,_FRAMESTRUCT104 sframe



typedef void (*CS104_ImportantInfoHandler)(void* parameter, char* msg);

#endif /* SRC_IEC60870_MASTER_H_ */
