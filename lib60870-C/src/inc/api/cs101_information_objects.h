﻿/*
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

#ifndef SRC_INC_INFORMATION_OBJECTS_H_
#define SRC_INC_INFORMATION_OBJECTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file cs101_information_objects.h
 * \brief Functions for CS101/CS104 information objects
 * These are the implementation of the different data types and message types
 */

/**
 * @defgroup COMMON Common API functions
 *
 * @{
 */


typedef uint8_t QualityDescriptor;

/**
 * \brief QDP - Quality descriptor for events of protection equipment according to IEC 60870-5-101:2003 7.2.6.4
 */
typedef uint8_t QualityDescriptorP;

#define IEC60870_QUALITY_GOOD                 0
#define IEC60870_QUALITY_OVERFLOW             0x01 /* QualityDescriptor */
#define IEC60870_QUALITY_RESERVED             0x04 /* QualityDescriptorP */
#define IEC60870_QUALITY_ELAPSED_TIME_INVALID 0x08 /* QualityDescriptorP */
#define IEC60870_QUALITY_BLOCKED              0x10 /* QualityDescriptor, QualityDescriptorP */
#define IEC60870_QUALITY_SUBSTITUTED          0x20 /* QualityDescriptor, QualityDescriptorP */
#define IEC60870_QUALITY_NON_TOPICAL          0x40 /* QualityDescriptor, QualityDescriptorP */
#define IEC60870_QUALITY_INVALID              0x80 /* QualityDescriptor, QualityDescriptorP */

/**
 * \brief SPE - Start events of protection equipment according to IEC 60870-5-101:2003 7.2.6.11
 */
typedef uint8_t StartEvent;

#define IEC60870_START_EVENT_NONE   0
#define IEC60870_START_EVENT_GS     0x01 /* General start of operation */
#define IEC60870_START_EVENT_SL1    0x02 /* Start of operation phase L1 */
#define IEC60870_START_EVENT_SL2    0x04 /* Start of operation phase L2 */
#define IEC60870_START_EVENT_SL3    0x08 /* Start of operation phase L3 */
#define IEC60870_START_EVENT_SIE    0x10 /* Start of operation IE (earth current) */
#define IEC60870_START_EVENT_SRD    0x20 /* Start of operation in reverse direction */
#define IEC60870_START_EVENT_RES1   0x40 /* Reserved bit */
#define IEC60870_START_EVENT_RES2   0x80 /* Reserved bit */

/**
 * \brief Output circuit information (OCI) of protection equipment according to IEC 60870-5-101:2003 7.2.6.12
 */
typedef uint8_t OutputCircuitInfo;

#define IEC60870_OUTPUT_CI_GC   0x01 /* General command to output circuit */
#define IEC60870_OUTPUT_CI_CL1  0x02 /* Command to output circuit phase L1 */
#define IEC60870_OUTPUT_CI_CL2  0x04 /* Command to output circuit phase L2 */
#define IEC60870_OUTPUT_CI_CL3  0x08 /* Command to output circuit phase L3 */

/**
 *  \brief Qualifier of parameter of measured values (QPM) according to IEC 60870-5-101:2003 7.2.6.24
 */
typedef uint8_t QualifierOfParameterMV;

#define IEC60870_QPM_NOT_USED 0
#define IEC60870_QPM_THRESHOLD_VALUE 1
#define IEC60870_QPM_SMOOTHING_FACTOR 2
#define IEC60870_QPM_LOW_LIMIT_FOR_TRANSMISSION 3
#define IEC60870_QPM_HIGH_LIMIT_FOR_TRANSMISSION 4

/**
 * \brief Cause of Initialization (COI) according to IEC 60870-5-101:2003 7.2.6.21
 */

typedef uint8_t CauseOfInitialization;

#define IEC60870_COI_LOCAL_SWITCH_ON 0
#define IEC60870_COI_LOCAL_MANUAL_RESET 1
#define IEC60870_COI_REMOTE_RESET 2

/**
 * \brief Qualifier Of Command (QOC) according to IEC 60870-5-101:2003 7.2.6.26
 */
typedef uint8_t QualifierOfCommand;

#define IEC60870_QOC_NO_ADDITIONAL_DEFINITION 0
#define IEC60870_QOC_SHORT_PULSE_DURATION 1
#define IEC60870_QOC_LONG_PULSE_DURATION 2
#define IEC60870_QOC_PERSISTANT_OUTPUT 3

/**
 * \brief Select And Call Qualifier (SCQ) according to IEC 60870-5-101:2003 7.2.6.30
 */

typedef uint8_t SelectAndCallQualifier;

#define IEC60870_SCQ_DEFAULT 0
#define IEC60870_SCQ_SELECT_FILE 1
#define IEC60870_SCQ_REQUEST_FILE 2
#define IEC60870_SCQ_DEACTIVATE_FILE 3
#define IEC60870_SCQ_DELETE_FILE 4
#define IEC60870_SCQ_SELECT_SECTION 5
#define IEC60870_SCQ_REQUEST_SECTION 6
#define IEC60870_SCQ_DEACTIVATE_SECTION 7

/**
 * \brief Qualifier of interrogation (QUI) according to IEC 60870-5-101:2003 7.2.6.22
 */

typedef uint8_t QualifierOfInterrogation;

#define IEC60870_QOI_STATION 20
#define IEC60870_QOI_GROUP_1 21
#define IEC60870_QOI_GROUP_2 22
#define IEC60870_QOI_GROUP_3 23
#define IEC60870_QOI_GROUP_4 24
#define IEC60870_QOI_GROUP_5 25
#define IEC60870_QOI_GROUP_6 26
#define IEC60870_QOI_GROUP_7 27
#define IEC60870_QOI_GROUP_8 28
#define IEC60870_QOI_GROUP_9 29
#define IEC60870_QOI_GROUP_10 30
#define IEC60870_QOI_GROUP_11 31
#define IEC60870_QOI_GROUP_12 32
#define IEC60870_QOI_GROUP_13 33
#define IEC60870_QOI_GROUP_14 34
#define IEC60870_QOI_GROUP_15 35
#define IEC60870_QOI_GROUP_16 36


/**
 * \brief QCC (Qualifier of counter interrogation command) according to IEC 60870-5-101:2003 7.2.6.23
 *
 * The QCC is composed by the RQT(request) and the FRZ(Freeze) part
 *
 * QCC = RQT + FRZ
 *
 * E.g.
 *
 * to read the the values from counter group one use:
 *
 *   QCC = IEC60870_QCC_RQT_GROUP_1 + IEC60870_QCC_FRZ_READ
 *
 * to reset all counters use:
 *
 *   QCC = IEC60870_QCC_RQT_GENERAL + IEC60870_QCC_FRZ_COUNTER_RESET
 *
 */
typedef uint8_t QualifierOfCIC;

#define IEC60870_QCC_RQT_GROUP_1 1
#define IEC60870_QCC_RQT_GROUP_2 2
#define IEC60870_QCC_RQT_GROUP_3 3
#define IEC60870_QCC_RQT_GROUP_4 4
#define IEC60870_QCC_RQT_GENERAL 5

#define IEC60870_QCC_FRZ_READ                 0x00
#define IEC60870_QCC_FRZ_FREEZE_WITHOUT_RESET 0x40
#define IEC60870_QCC_FRZ_FREEZE_WITH_RESET    0x80
#define IEC60870_QCC_FRZ_COUNTER_RESET

/**
 * \brief QRP (Qualifier of reset process command) according to IEC 60870-5-101:2003 7.2.6.27
 */
typedef uint8_t QualifierOfRPC;

#define IEC60870_QRP_NOT_USED 0
#define IEC60870_QRP_GENERAL_RESET 1
#define IEC60870_QRP_RESET_PENDING_INFO_WITH_TIME_TAG 2


/**
 * \brief Qualifier of parameter activation (QPA) according to IEC 60870-5-101:2003 7.2.6.25
 */
typedef uint8_t QualifierOfParameterActivation;

#define IEC60870_QPA_NOT_USED 0
#define IEC60870_QPA_DE_ACT_PREV_LOADED_PARAMETER 1
#define IEC60870_QPA_DE_ACT_OBJECT_PARAMETER 2
#define IEC60870_QPA_DE_ACT_OBJECT_TRANSMISSION 4


typedef uint8_t SetpointCommandQualifier;

typedef enum  {
    IEC60870_DOUBLE_POINT_INTERMEDIATE = 0,
    IEC60870_DOUBLE_POINT_OFF = 1,
    IEC60870_DOUBLE_POINT_ON = 2,
    IEC60870_DOUBLE_POINT_INDETERMINATE = 3
} DoublePointValue;

typedef enum {
    IEC60870_EVENTSTATE_INDETERMINATE_0 = 0,
    IEC60870_EVENTSTATE_OFF = 1,
    IEC60870_EVENTSTATE_ON = 2,
    IEC60870_EVENTSTATE_INDETERMINATE_3 = 3
} EventState;

/**
 * \brief Regulating step command state (RCS) according to IEC 60870-5-101:2003 7.2.6.17
 */
typedef enum {
    IEC60870_STEP_INVALID_0 = 0,
    IEC60870_STEP_LOWER = 1,
    IEC60870_STEP_HIGHER = 2,
    IEC60870_STEP_INVALID_3 = 3
} StepCommandValue;

typedef uint8_t tSingleEvent;

typedef tSingleEvent* SingleEvent;

void
SingleEvent_setEventState(SingleEvent self, EventState eventState);

EventState
SingleEvent_getEventState(SingleEvent self);

void
SingleEvent_setQDP(SingleEvent self, QualityDescriptorP qdp);

QualityDescriptorP
SingleEvent_getQDP(SingleEvent self);


typedef struct sStatusAndStatusChangeDetection tStatusAndStatusChangeDetection;

typedef tStatusAndStatusChangeDetection* StatusAndStatusChangeDetection;

struct sStatusAndStatusChangeDetection {
    uint8_t encodedValue[4];
};

uint16_t
StatusAndStatusChangeDetection_getSTn(StatusAndStatusChangeDetection self);

uint16_t
StatusAndStatusChangeDetection_getCDn(StatusAndStatusChangeDetection self);

void
StatusAndStatusChangeDetection_setSTn(StatusAndStatusChangeDetection self, uint16_t value);

bool
StatusAndStatusChangeDetection_getST(StatusAndStatusChangeDetection self, int index);

bool
StatusAndStatusChangeDetection_getCD(StatusAndStatusChangeDetection self, int index);


/************************************************
 * InformationObject
 ************************************************/

typedef struct sInformationObject* InformationObject;

/**
 * \brief return the size in memory of a generic InformationObject instance
 *
 * This function can be used to determine the required memory for malloc
 */
int
InformationObject_getMaxSizeInMemory(void);

int
InformationObject_getObjectAddress(InformationObject self);

/**
 * \brief Destroy object - free all related resources
 *
 * This is a virtual function that calls the destructor from the implementation class
 *
 * \self the InformationObject instance
 */
void
InformationObject_destroy(InformationObject self);

/************************************************
 * SinglePointInformation (:InformationObject)
 ************************************************/

typedef struct sSinglePointInformation* SinglePointInformation;

SinglePointInformation
SinglePointInformation_create(SinglePointInformation self, int ioa, bool value,
        QualityDescriptor quality);

bool
SinglePointInformation_getValue(SinglePointInformation self);

QualityDescriptor
SinglePointInformation_getQuality(SinglePointInformation self);

void
SinglePointInformation_destroy(SinglePointInformation self);

/********************************************************
 *  SinglePointWithCP24Time2a (:SinglePointInformation)
 ********************************************************/

typedef struct sSinglePointWithCP24Time2a* SinglePointWithCP24Time2a;

SinglePointWithCP24Time2a
SinglePointWithCP24Time2a_create(SinglePointWithCP24Time2a self, int ioa, bool value,
        QualityDescriptor quality, CP24Time2a timestamp);

void
SinglePointWithCP24Time2a_destroy(SinglePointWithCP24Time2a self);

CP24Time2a
SinglePointWithCP24Time2a_getTimestamp(SinglePointWithCP24Time2a self);

/********************************************************
 *  SinglePointWithCP56Time2a (:SinglePointInformation)
 ********************************************************/

typedef struct sSinglePointWithCP56Time2a* SinglePointWithCP56Time2a;

SinglePointWithCP56Time2a
SinglePointWithCP56Time2a_create(SinglePointWithCP56Time2a self, int ioa, bool value,
        QualityDescriptor quality, CP56Time2a timestamp);

void
SinglePointWithCP56Time2a_destroy(SinglePointWithCP56Time2a self);

CP56Time2a
SinglePointWithCP56Time2a_getTimestamp(SinglePointWithCP56Time2a self);

//42==故障事件信息 FaultEventWithCP56Time2a
/********************************************************
 *  FaultEventWithCP56Time2a (:SinglePointInformation + YCvalue)
 ********************************************************/

typedef struct sFaultEventWithCP56Time2a* FaultEventWithCP56Time2a;

//FaultEventWithCP56Time2a
//FaultEventWithCP56Time2a_create(FaultEventWithCP56Time2a self, int ioa, bool value_yx, float value_yc,
//                                int type,int num,bool isEncodeYXelseYC,bool isEncodefirstframe,CP56Time2a timestamp);
int FaultEventWithCP56Time2a_getYXnum(FaultEventWithCP56Time2a self);
int FaultEventWithCP56Time2a_getYXtype(FaultEventWithCP56Time2a self);
int FaultEventWithCP56Time2a_getYXaddr(FaultEventWithCP56Time2a self,int index);
int FaultEventWithCP56Time2a_getYXdata(FaultEventWithCP56Time2a self,int index);
CP56Time2a FaultEventWithCP56Time2a_getTimestamp(FaultEventWithCP56Time2a self,int index);

int FaultEventWithCP56Time2a_getYCnum(FaultEventWithCP56Time2a self);
int FaultEventWithCP56Time2a_getYCtype(FaultEventWithCP56Time2a self);
int FaultEventWithCP56Time2a_getYCaddr(FaultEventWithCP56Time2a self,int index);
float FaultEventWithCP56Time2a_getYCdata(FaultEventWithCP56Time2a self,int index);

void FaultEventWithCP56Time2a_destroy(FaultEventWithCP56Time2a self);




/************************************************
 * DoublePointInformation (:InformationObject)
 ************************************************/

typedef struct sDoublePointInformation* DoublePointInformation;

void
DoublePointInformation_destroy(DoublePointInformation self);

DoublePointInformation
DoublePointInformation_create(DoublePointInformation self, int ioa, DoublePointValue value,
        QualityDescriptor quality);

DoublePointValue
DoublePointInformation_getValue(DoublePointInformation self);

QualityDescriptor
DoublePointInformation_getQuality(DoublePointInformation self);

/********************************************************
 *  DoublePointWithCP24Time2a (:DoublePointInformation)
 ********************************************************/

typedef struct sDoublePointWithCP24Time2a* DoublePointWithCP24Time2a;

void
DoublePointWithCP24Time2a_destroy(DoublePointWithCP24Time2a self);

DoublePointWithCP24Time2a
DoublePointWithCP24Time2a_create(DoublePointWithCP24Time2a self, int ioa, DoublePointValue value,
        QualityDescriptor quality, CP24Time2a timestamp);

CP24Time2a
DoublePointWithCP24Time2a_getTimestamp(DoublePointWithCP24Time2a self);

/********************************************************
 *  DoublePointWithCP56Time2a (:DoublePointInformation)
 ********************************************************/

typedef struct sDoublePointWithCP56Time2a* DoublePointWithCP56Time2a;

DoublePointWithCP56Time2a
DoublePointWithCP56Time2a_create(DoublePointWithCP56Time2a self, int ioa, DoublePointValue value,
        QualityDescriptor quality, CP56Time2a timestamp);

void
DoublePointWithCP56Time2a_destroy(DoublePointWithCP56Time2a self);

CP56Time2a
DoublePointWithCP56Time2a_getTimestamp(DoublePointWithCP56Time2a self);

/************************************************
 * StepPositionInformation (:InformationObject)
 ************************************************/

typedef struct sStepPositionInformation* StepPositionInformation;

/**
* \brief Create a new instance of StepPositionInformation information object
*
* \param self Reference to an existing instance to reuse, if NULL a new instance will we dynamically allocated
* \param ioa Information object address
* \param value Step position (range -64 ... +63)
* \param isTransient true if position is transient, false otherwise
* \param quality quality descriptor (according to IEC 60870-5-101:2003 7.2.6.3)
*
* \return Reference to the new instance
*/
StepPositionInformation
StepPositionInformation_create(StepPositionInformation self, int ioa, int value, bool isTransient,
        QualityDescriptor quality);

void
StepPositionInformation_destroy(StepPositionInformation self);

int
StepPositionInformation_getObjectAddress(StepPositionInformation self);

/**
 * \brief Step position (range -64 ... +63)
 */
int
StepPositionInformation_getValue(StepPositionInformation self);

bool
StepPositionInformation_isTransient(StepPositionInformation self);

QualityDescriptor
StepPositionInformation_getQuality(StepPositionInformation self);

/*********************************************************
 * StepPositionWithCP24Time2a (:StepPositionInformation)
 *********************************************************/

typedef struct sStepPositionWithCP24Time2a* StepPositionWithCP24Time2a;

void
StepPositionWithCP24Time2a_destroy(StepPositionWithCP24Time2a self);

StepPositionWithCP24Time2a
StepPositionWithCP24Time2a_create(StepPositionWithCP24Time2a self, int ioa, int value, bool isTransient,
        QualityDescriptor quality, CP24Time2a timestamp);

CP24Time2a
StepPositionWithCP24Time2a_getTimestamp(StepPositionWithCP24Time2a self);


/*********************************************************
 * StepPositionWithCP56Time2a (:StepPositionInformation)
 *********************************************************/

typedef struct sStepPositionWithCP56Time2a* StepPositionWithCP56Time2a;

void
StepPositionWithCP56Time2a_destroy(StepPositionWithCP56Time2a self);

StepPositionWithCP56Time2a
StepPositionWithCP56Time2a_create(StepPositionWithCP56Time2a self, int ioa, int value, bool isTransient,
        QualityDescriptor quality, CP56Time2a timestamp);

CP56Time2a
StepPositionWithCP56Time2a_getTimestamp(StepPositionWithCP56Time2a self);

/**********************************************
 * BitString32 (:InformationObject)
 **********************************************/

typedef struct sBitString32* BitString32;

void
BitString32_destroy(BitString32 self);

BitString32
BitString32_create(BitString32 self, int ioa, uint32_t value);

uint32_t
BitString32_getValue(BitString32 self);

QualityDescriptor
BitString32_getQuality(BitString32 self);

/**********************************************
 * Bitstring32WithCP24Time2a (:BitString32)
 **********************************************/

typedef struct sBitstring32WithCP24Time2a* Bitstring32WithCP24Time2a;

void
Bitstring32WithCP24Time2a_destroy(Bitstring32WithCP24Time2a self);

Bitstring32WithCP24Time2a
Bitstring32WithCP24Time2a_create(Bitstring32WithCP24Time2a self, int ioa, uint32_t value, CP24Time2a timestamp);

CP24Time2a
Bitstring32WithCP24Time2a_getTimestamp(Bitstring32WithCP24Time2a self);

/**********************************************
 * Bitstring32WithCP56Time2a (:BitString32)
 **********************************************/

typedef struct sBitstring32WithCP56Time2a* Bitstring32WithCP56Time2a;

void
Bitstring32WithCP56Time2a_destroy(Bitstring32WithCP56Time2a self);

Bitstring32WithCP56Time2a
Bitstring32WithCP56Time2a_create(Bitstring32WithCP56Time2a self, int ioa, uint32_t value, CP56Time2a timestamp);

CP56Time2a
Bitstring32WithCP56Time2a_getTimestamp(Bitstring32WithCP56Time2a self);

/*************************************************************
 * MeasuredValueNormalizedWithoutQuality : InformationObject
 *************************************************************/

typedef struct sMeasuredValueNormalizedWithoutQuality* MeasuredValueNormalizedWithoutQuality;

void
MeasuredValueNormalizedWithoutQuality_destroy(MeasuredValueNormalizedWithoutQuality self);

MeasuredValueNormalizedWithoutQuality
MeasuredValueNormalizedWithoutQuality_create(MeasuredValueNormalizedWithoutQuality self, int ioa, float value);

float
MeasuredValueNormalizedWithoutQuality_getValue(MeasuredValueNormalizedWithoutQuality self);

void
MeasuredValueNormalizedWithoutQuality_setValue(MeasuredValueNormalizedWithoutQuality self, float value);

/**********************************************
 * MeasuredValueNormalized
 **********************************************/

typedef struct sMeasuredValueNormalized* MeasuredValueNormalized;

void
MeasuredValueNormalized_destroy(MeasuredValueNormalized self);

MeasuredValueNormalized
MeasuredValueNormalized_create(MeasuredValueNormalized self, int ioa, float value, QualityDescriptor quality);

float
MeasuredValueNormalized_getValue(MeasuredValueNormalized self);

void
MeasuredValueNormalized_setValue(MeasuredValueNormalized self, float value);

QualityDescriptor
MeasuredValueNormalized_getQuality(MeasuredValueNormalized self);

/***********************************************************************
 * MeasuredValueNormalizedWithCP24Time2a : MeasuredValueNormalized
 ***********************************************************************/

typedef struct sMeasuredValueNormalizedWithCP24Time2a* MeasuredValueNormalizedWithCP24Time2a;

void
MeasuredValueNormalizedWithCP24Time2a_destroy(MeasuredValueNormalizedWithCP24Time2a self);

MeasuredValueNormalizedWithCP24Time2a
MeasuredValueNormalizedWithCP24Time2a_create(MeasuredValueNormalizedWithCP24Time2a self, int ioa,
            float value, QualityDescriptor quality, CP24Time2a timestamp);

CP24Time2a
MeasuredValueNormalizedWithCP24Time2a_getTimestamp(MeasuredValueNormalizedWithCP24Time2a self);

void
MeasuredValueNormalizedWithCP24Time2a_setTimestamp(MeasuredValueNormalizedWithCP24Time2a self, CP24Time2a value);

/***********************************************************************
 * MeasuredValueNormalizedWithCP56Time2a : MeasuredValueNormalized
 ***********************************************************************/

typedef struct sMeasuredValueNormalizedWithCP56Time2a* MeasuredValueNormalizedWithCP56Time2a;

void
MeasuredValueNormalizedWithCP56Time2a_destroy(MeasuredValueNormalizedWithCP56Time2a self);

MeasuredValueNormalizedWithCP56Time2a
MeasuredValueNormalizedWithCP56Time2a_create(MeasuredValueNormalizedWithCP56Time2a self, int ioa,
            float value, QualityDescriptor quality, CP56Time2a timestamp);

CP56Time2a
MeasuredValueNormalizedWithCP56Time2a_getTimestamp(MeasuredValueNormalizedWithCP56Time2a self);

void
MeasuredValueNormalizedWithCP56Time2a_setTimestamp(MeasuredValueNormalizedWithCP56Time2a self, CP56Time2a value);


/*******************************************
 * MeasuredValueScaled : InformationObject
 *******************************************/

typedef struct sMeasuredValueScaled* MeasuredValueScaled;

/**
 * \brief Create a new instance of MeasuredValueScaled information object
 *
 * \param self Reference to an existing instance to reuse, if NULL a new instance will we dynamically allocated
 * \param ioa Information object address
 * \param value scaled value (range -32768 - 32767)
 * \param quality quality descriptor (according to IEC 60870-5-101:2003 7.2.6.3)
 *
 * \return Reference to the new instance
 */
MeasuredValueScaled
MeasuredValueScaled_create(MeasuredValueScaled self, int ioa, int value, QualityDescriptor quality);

void
MeasuredValueScaled_destroy(MeasuredValueScaled self);

int
MeasuredValueScaled_getValue(MeasuredValueScaled self);

void
MeasuredValueScaled_setValue(MeasuredValueScaled self, int value);

QualityDescriptor
MeasuredValueScaled_getQuality(MeasuredValueScaled self);

void
MeasuredValueScaled_setQuality(MeasuredValueScaled self, QualityDescriptor quality);

/***********************************************************************
 * MeasuredValueScaledWithCP24Time2a : MeasuredValueScaled
 ***********************************************************************/

typedef struct sMeasuredValueScaledWithCP24Time2a* MeasuredValueScaledWithCP24Time2a;

void
MeasuredValueScaledWithCP24Time2a_destroy(MeasuredValueScaledWithCP24Time2a self);

MeasuredValueScaledWithCP24Time2a
MeasuredValueScaledWithCP24Time2a_create(MeasuredValueScaledWithCP24Time2a self, int ioa,
        int value, QualityDescriptor quality, CP24Time2a timestamp);

CP24Time2a
MeasuredValueScaledWithCP24Time2a_getTimestamp(MeasuredValueScaledWithCP24Time2a self);

void
MeasuredValueScaledWithCP24Time2a_setTimestamp(MeasuredValueScaledWithCP24Time2a self, CP24Time2a value);

/***********************************************************************
 * MeasuredValueScaledWithCP56Time2a : MeasuredValueScaled
 ***********************************************************************/

typedef struct sMeasuredValueScaledWithCP56Time2a* MeasuredValueScaledWithCP56Time2a;

void
MeasuredValueScaledWithCP56Time2a_destroy(MeasuredValueScaledWithCP56Time2a self);

MeasuredValueScaledWithCP56Time2a
MeasuredValueScaledWithCP56Time2a_create(MeasuredValueScaledWithCP56Time2a self, int ioa,
        int value, QualityDescriptor quality, CP56Time2a timestamp);

CP56Time2a
MeasuredValueScaledWithCP56Time2a_getTimestamp(MeasuredValueScaledWithCP56Time2a self);

void
MeasuredValueScaledWithCP56Time2a_setTimestamp(MeasuredValueScaledWithCP56Time2a self, CP56Time2a value);

/*******************************************
 * MeasuredValueShort : InformationObject
 *******************************************/

typedef struct sMeasuredValueShort* MeasuredValueShort;

void
MeasuredValueShort_destroy(MeasuredValueShort self);

MeasuredValueShort
MeasuredValueShort_create(MeasuredValueShort self, int ioa, float value, QualityDescriptor quality);

float
MeasuredValueShort_getValue(MeasuredValueShort self);

void
MeasuredValueShort_setValue(MeasuredValueShort self, float value);

QualityDescriptor
MeasuredValueShort_getQuality(MeasuredValueShort self);

/***********************************************************************
 * MeasuredValueShortWithCP24Time2a : MeasuredValueShort
 ***********************************************************************/

typedef struct sMeasuredValueShortWithCP24Time2a* MeasuredValueShortWithCP24Time2a;

void
MeasuredValueShortWithCP24Time2a_destroy(MeasuredValueShortWithCP24Time2a self);

MeasuredValueShortWithCP24Time2a
MeasuredValueShortWithCP24Time2a_create(MeasuredValueShortWithCP24Time2a self, int ioa,
        float value, QualityDescriptor quality, CP24Time2a timestamp);

CP24Time2a
MeasuredValueShortWithCP24Time2a_getTimestamp(MeasuredValueShortWithCP24Time2a self);

void
MeasuredValueShortWithCP24Time2a_setTimestamp(MeasuredValueShortWithCP24Time2a self,
        CP24Time2a value);

/***********************************************************************
 * MeasuredValueShortWithCP56Time2a : MeasuredValueShort
 ***********************************************************************/

typedef struct sMeasuredValueShortWithCP56Time2a* MeasuredValueShortWithCP56Time2a;

void
MeasuredValueShortWithCP56Time2a_destroy(MeasuredValueShortWithCP56Time2a self);

MeasuredValueShortWithCP56Time2a
MeasuredValueShortWithCP56Time2a_create(MeasuredValueShortWithCP56Time2a self, int ioa,
        float value, QualityDescriptor quality, CP56Time2a timestamp);

CP56Time2a
MeasuredValueShortWithCP56Time2a_getTimestamp(MeasuredValueShortWithCP56Time2a self);

void
MeasuredValueShortWithCP56Time2a_setTimestamp(MeasuredValueShortWithCP56Time2a self,
        CP56Time2a value);

/*******************************************
 * IntegratedTotals : InformationObject
 *******************************************/

typedef struct sIntegratedTotals* IntegratedTotals;

void
IntegratedTotals_destroy(IntegratedTotals self);

/**
 * \brief Create a new instance of IntegratedTotals information object
 *
 * For message type: M_IT_NA_1 (15)
 *
 * \param self Reference to an existing instance to reuse, if NULL a new instance will we dynamically allocated
 * \param ioa Information object address
 * \param value binary counter reading value and state
 *
 * \return Reference to the new instance
 */
IntegratedTotals
IntegratedTotals_create(IntegratedTotals self, int ioa, BinaryCounterReading value);

BinaryCounterReading
IntegratedTotals_getBCR(IntegratedTotals self);

void
IntegratedTotals_setBCR(IntegratedTotals self, BinaryCounterReading value);

/***********************************************************************
 * IntegratedTotalsWithCP24Time2a : IntegratedTotals
 ***********************************************************************/

typedef struct sIntegratedTotalsWithCP24Time2a* IntegratedTotalsWithCP24Time2a;

/**
 * \brief Create a new instance of IntegratedTotalsWithCP24Time2a information object
 *
 * For message type: M_IT_TA_1 (16)
 *
 * \param self Reference to an existing instance to reuse, if NULL a new instance will we dynamically allocated
 * \param ioa Information object address
 * \param value binary counter reading value and state
 * \param timestamp timestamp of the reading
 *
 * \return Reference to the new instance
 */
IntegratedTotalsWithCP24Time2a
IntegratedTotalsWithCP24Time2a_create(IntegratedTotalsWithCP24Time2a self, int ioa,
        BinaryCounterReading value, CP24Time2a timestamp);

void
IntegratedTotalsWithCP24Time2a_destroy(IntegratedTotalsWithCP24Time2a self);

CP24Time2a
IntegratedTotalsWithCP24Time2a_getTimestamp(IntegratedTotalsWithCP24Time2a self);

void
IntegratedTotalsWithCP24Time2a_setTimestamp(IntegratedTotalsWithCP24Time2a self,
        CP24Time2a value);

/***********************************************************************
 * IntegratedTotalsWithCP56Time2a : IntegratedTotals
 ***********************************************************************/

typedef struct sIntegratedTotalsWithCP56Time2a* IntegratedTotalsWithCP56Time2a;

/**
 * \brief Create a new instance of IntegratedTotalsWithCP56Time2a information object
 *
 * For message type: M_IT_TB_1 (37)
 *
 * \param self Reference to an existing instance to reuse, if NULL a new instance will we dynamically allocated
 * \param ioa Information object address
 * \param value binary counter reading value and state
 * \param timestamp timestamp of the reading
 *
 * \return Reference to the new instance
 */
IntegratedTotalsWithCP56Time2a
IntegratedTotalsWithCP56Time2a_create(IntegratedTotalsWithCP56Time2a self, int ioa,
        BinaryCounterReading value, CP56Time2a timestamp);

void
IntegratedTotalsWithCP56Time2a_destroy(IntegratedTotalsWithCP56Time2a self);

CP56Time2a
IntegratedTotalsWithCP56Time2a_getTimestamp(IntegratedTotalsWithCP56Time2a self);

void
IntegratedTotalsWithCP56Time2a_setTimestamp(IntegratedTotalsWithCP56Time2a self,
        CP56Time2a value);

/***********************************************************************
 * EventOfProtectionEquipment : InformationObject
 ***********************************************************************/

typedef struct sEventOfProtectionEquipment* EventOfProtectionEquipment;

void
EventOfProtectionEquipment_destroy(EventOfProtectionEquipment self);

EventOfProtectionEquipment
EventOfProtectionEquipment_create(EventOfProtectionEquipment self, int ioa,
        SingleEvent event, CP16Time2a elapsedTime, CP24Time2a timestamp);

SingleEvent
EventOfProtectionEquipment_getEvent(EventOfProtectionEquipment self);

CP16Time2a
EventOfProtectionEquipment_getElapsedTime(EventOfProtectionEquipment self);

CP24Time2a
EventOfProtectionEquipment_getTimestamp(EventOfProtectionEquipment self);

/***********************************************************************
 * PackedStartEventsOfProtectionEquipment : InformationObject
 ***********************************************************************/

typedef struct sPackedStartEventsOfProtectionEquipment* PackedStartEventsOfProtectionEquipment;

PackedStartEventsOfProtectionEquipment
PackedStartEventsOfProtectionEquipment_create(PackedStartEventsOfProtectionEquipment self, int ioa,
        StartEvent event, QualityDescriptorP qdp, CP16Time2a elapsedTime, CP24Time2a timestamp);

void
PackedStartEventsOfProtectionEquipment_destroy(PackedStartEventsOfProtectionEquipment self);

StartEvent
PackedStartEventsOfProtectionEquipment_getEvent(PackedStartEventsOfProtectionEquipment self);

QualityDescriptorP
PackedStartEventsOfProtectionEquipment_getQuality(PackedStartEventsOfProtectionEquipment self);

CP16Time2a
PackedStartEventsOfProtectionEquipment_getElapsedTime(PackedStartEventsOfProtectionEquipment self);

CP24Time2a
PackedStartEventsOfProtectionEquipment_getTimestamp(PackedStartEventsOfProtectionEquipment self);

/***********************************************************************
 * PacketOutputCircuitInfo : InformationObject
 ***********************************************************************/

typedef struct sPackedOutputCircuitInfo* PackedOutputCircuitInfo;

void
PackedOutputCircuitInfo_destroy(PackedOutputCircuitInfo self);

PackedOutputCircuitInfo
PackedOutputCircuitInfo_create(PackedOutputCircuitInfo self, int ioa,
        OutputCircuitInfo oci, QualityDescriptorP qdp, CP16Time2a operatingTime, CP24Time2a timestamp);

OutputCircuitInfo
PackedOutputCircuitInfo_getOCI(PackedOutputCircuitInfo self);

QualityDescriptorP
PackedOutputCircuitInfo_getQuality(PackedOutputCircuitInfo self);

CP16Time2a
PackedOutputCircuitInfo_getOperatingTime(PackedOutputCircuitInfo self);

CP24Time2a
PackedOutputCircuitInfo_getTimestamp(PackedOutputCircuitInfo self);

/***********************************************************************
 * PackedSinglePointWithSCD : InformationObject
 ***********************************************************************/

typedef struct sPackedSinglePointWithSCD* PackedSinglePointWithSCD;

void
PackedSinglePointWithSCD_destroy(PackedSinglePointWithSCD self);

PackedSinglePointWithSCD
PackedSinglePointWithSCD_create(PackedSinglePointWithSCD self, int ioa,
        StatusAndStatusChangeDetection scd, QualityDescriptor qds);

QualityDescriptor
PackedSinglePointWithSCD_getQuality(PackedSinglePointWithSCD self);

StatusAndStatusChangeDetection
PackedSinglePointWithSCD_getSCD(PackedSinglePointWithSCD self);


/*******************************************
 * SingleCommand
 *******************************************/

typedef struct sSingleCommand* SingleCommand;

SingleCommand
SingleCommand_create(SingleCommand self, int ioa, bool command, bool selectCommand, int qu);

void
SingleCommand_destroy(SingleCommand self);

int
SingleCommand_getQU(SingleCommand self);

bool
SingleCommand_getState(SingleCommand self);

bool
SingleCommand_isSelect(SingleCommand self);

/***********************************************************************
 * SingleCommandWithCP56Time2a : SingleCommand
 ***********************************************************************/

typedef struct sSingleCommandWithCP56Time2a* SingleCommandWithCP56Time2a;

void
SingleCommandWithCP56Time2a_destroy(SingleCommandWithCP56Time2a self);

SingleCommandWithCP56Time2a
SingleCommandWithCP56Time2a_create(SingleCommandWithCP56Time2a self, int ioa, bool command, bool selectCommand, int qu, CP56Time2a timestamp);

CP56Time2a
SingleCommandWithCP56Time2a_getTimestamp(SingleCommandWithCP56Time2a self);

/*******************************************
 * DoubleCommand : InformationObject
 *******************************************/

typedef struct sDoubleCommand* DoubleCommand;

void
DoubleCommand_destroy(DoubleCommand self);

DoubleCommand
DoubleCommand_create(DoubleCommand self, int ioa, int command, bool selectCommand, int qu);

int
DoubleCommand_getQU(DoubleCommand self);

int
DoubleCommand_getState(DoubleCommand self);

bool
DoubleCommand_isSelect(DoubleCommand self);

//CommandParamSet  开关参数设置（遥控扩展）
/*******************************************
 * CommandParamSet : InformationObject
 *******************************************/
typedef struct sCommandParamSet* CommandParamSet;

void
CommandParamSet_destroy(CommandParamSet self);

CommandParamSet
CommandParamSet_create(CommandParamSet self, int ioa, int param1, int param2, int param3 ,int param4);

int
CommandParamSet_getParam1(CommandParamSet self);

int
CommandParamSet_getParam2(CommandParamSet self);

int
CommandParamSet_getParam3(CommandParamSet self);

/*******************************************
 * StepCommand : InformationObject
 *******************************************/

typedef struct sStepCommand* StepCommand;

void
StepCommand_destroy(StepCommand self);

StepCommand
StepCommand_create(StepCommand self, int ioa, StepCommandValue command, bool selectCommand, int qu);

int
StepCommand_getQU(StepCommand self);

StepCommandValue
StepCommand_getState(StepCommand self);

bool
StepCommand_isSelect(StepCommand self);

/*************************************************
 * SetpointCommandNormalized : InformationObject
 ************************************************/

typedef struct sSetpointCommandNormalized* SetpointCommandNormalized;

void
SetpointCommandNormalized_destroy(SetpointCommandNormalized self);

SetpointCommandNormalized
SetpointCommandNormalized_create(SetpointCommandNormalized self, int ioa, float value, bool selectCommand, int ql);

float
SetpointCommandNormalized_getValue(SetpointCommandNormalized self);

int
SetpointCommandNormalized_getQL(SetpointCommandNormalized self);

bool
SetpointCommandNormalized_isSelect(SetpointCommandNormalized self);

/*************************************************
 * SetpointCommandScaled : InformationObject
 ************************************************/

typedef struct sSetpointCommandScaled* SetpointCommandScaled;

void
SetpointCommandScaled_destroy(SetpointCommandScaled self);

SetpointCommandScaled
SetpointCommandScaled_create(SetpointCommandScaled self, int ioa, int value, bool selectCommand, int ql);

int
SetpointCommandScaled_getValue(SetpointCommandScaled self);

int
SetpointCommandScaled_getQL(SetpointCommandScaled self);

bool
SetpointCommandScaled_isSelect(SetpointCommandScaled self);

/*************************************************
 * SetpointCommandShort: InformationObject
 ************************************************/

typedef struct sSetpointCommandShort* SetpointCommandShort;

void
SetpointCommandShort_destroy(SetpointCommandShort self);

SetpointCommandShort
SetpointCommandShort_create(SetpointCommandShort self, int ioa, float value, bool selectCommand, int ql);

float
SetpointCommandShort_getValue(SetpointCommandShort self);

int
SetpointCommandShort_getQL(SetpointCommandShort self);

bool
SetpointCommandShort_isSelect(SetpointCommandShort self);

/*************************************************
 * Bitstring32Command : InformationObject
 ************************************************/

typedef struct sBitstring32Command* Bitstring32Command;

Bitstring32Command
Bitstring32Command_create(Bitstring32Command self, int ioa, uint32_t value);

void
Bitstring32Command_destroy(Bitstring32Command self);

uint32_t
Bitstring32Command_getValue(Bitstring32Command self);

/*************************************************
 * InterrogationCommand : InformationObject
 ************************************************/

typedef struct sInterrogationCommand* InterrogationCommand;

InterrogationCommand
InterrogationCommand_create(InterrogationCommand self, int ioa, uint8_t qoi);

void
InterrogationCommand_destroy(InterrogationCommand self);

uint8_t
InterrogationCommand_getQOI(InterrogationCommand self);

/*************************************************
 * ReadCommand : InformationObject
 ************************************************/

typedef struct sReadCommand* ReadCommand;

ReadCommand
ReadCommand_create(ReadCommand self, int ioa);

void
ReadCommand_destroy(ReadCommand self);

/***************************************************
 * ClockSynchronizationCommand : InformationObject
 **************************************************/

typedef struct sClockSynchronizationCommand* ClockSynchronizationCommand;

ClockSynchronizationCommand
ClockSynchronizationCommand_create(ClockSynchronizationCommand self, int ioa, CP56Time2a timestamp);

void
ClockSynchronizationCommand_destroy(ClockSynchronizationCommand self);

CP56Time2a
ClockSynchronizationCommand_getTime(ClockSynchronizationCommand self);

/******************************************************
 * ParameterNormalizedValue : MeasuredValueNormalized
 *****************************************************/

typedef struct sMeasuredValueNormalized* ParameterNormalizedValue;

void
ParameterNormalizedValue_destroy(ParameterNormalizedValue self);

ParameterNormalizedValue
ParameterNormalizedValue_create(ParameterNormalizedValue self, int ioa, float value, QualifierOfParameterMV qpm);

float
ParameterNormalizedValue_getValue(ParameterNormalizedValue self);

void
ParameterNormalizedValue_setValue(ParameterNormalizedValue self, float value);

QualifierOfParameterMV
ParameterNormalizedValue_getQPM(ParameterNormalizedValue self);

/******************************************************
 * ParameterScaledValue : MeasuredValueScaled
 *****************************************************/

typedef struct sMeasuredValueScaled* ParameterScaledValue;

void
ParameterScaledValue_destroy(ParameterScaledValue self);

ParameterScaledValue
ParameterScaledValue_create(ParameterScaledValue self, int ioa, int value, QualifierOfParameterMV qpm);

int
ParameterScaledValue_getValue(ParameterScaledValue self);

void
ParameterScaledValue_setValue(ParameterScaledValue self, int value);

QualifierOfParameterMV
ParameterScaledValue_getQPM(ParameterScaledValue self);

/******************************************************
 * ParameterFloatValue : MeasuredValueShort
 *****************************************************/

typedef struct sMeasuredValueShort* ParameterFloatValue;

void
ParameterFloatValue_destroy(ParameterFloatValue self);

ParameterFloatValue
ParameterFloatValue_create(ParameterFloatValue self, int ioa, float value, QualifierOfParameterMV qpm);

float
ParameterFloatValue_getValue(ParameterFloatValue self);

void
ParameterFloatValue_setValue(ParameterFloatValue self, float value);

QualifierOfParameterMV
ParameterFloatValue_getQPM(ParameterFloatValue self);

/*******************************************
 * ParameterActivation : InformationObject
 *******************************************/

typedef struct sParameterActivation* ParameterActivation;

void
ParameterActivation_destroy(ParameterActivation self);

ParameterActivation
ParameterActivation_create(ParameterActivation self, int ioa, QualifierOfParameterActivation qpa);

QualifierOfParameterActivation
ParameterActivation_getQuality(ParameterActivation self);

/***********************************************************************
 * EventOfProtectionEquipmentWithCP56Time2a : InformationObject
 ***********************************************************************/

typedef struct sEventOfProtectionEquipmentWithCP56Time2a* EventOfProtectionEquipmentWithCP56Time2a;

void
EventOfProtectionEquipmentWithCP56Time2a_destroy(EventOfProtectionEquipmentWithCP56Time2a self);

EventOfProtectionEquipmentWithCP56Time2a
EventOfProtectionEquipmentWithCP56Time2a_create(EventOfProtectionEquipmentWithCP56Time2a self, int ioa,
        SingleEvent event, CP16Time2a elapsedTime, CP56Time2a timestamp);

SingleEvent
EventOfProtectionEquipmentWithCP56Time2a_getEvent(EventOfProtectionEquipmentWithCP56Time2a self);

CP16Time2a
EventOfProtectionEquipmentWithCP56Time2a_getElapsedTime(EventOfProtectionEquipmentWithCP56Time2a self);

CP56Time2a
EventOfProtectionEquipmentWithCP56Time2a_getTimestamp(EventOfProtectionEquipmentWithCP56Time2a self);

/***************************************************************************
 * PackedStartEventsOfProtectionEquipmentWithCP56Time2a : InformationObject
 ***************************************************************************/

typedef struct sPackedStartEventsOfProtectionEquipmentWithCP56Time2a* PackedStartEventsOfProtectionEquipmentWithCP56Time2a;

void
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_destroy(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self);
PackedStartEventsOfProtectionEquipmentWithCP56Time2a

PackedStartEventsOfProtectionEquipmentWithCP56Time2a_create(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self, int ioa,
        StartEvent event, QualityDescriptorP qdp, CP16Time2a elapsedTime, CP56Time2a timestamp);

StartEvent
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getEvent(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self);

QualityDescriptorP
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getQuality(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self);

CP16Time2a
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getElapsedTime(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self);

CP56Time2a
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getTimestamp(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self);

/***********************************************************************
 * PackedOutputCircuitInfoWithCP56Time2a : InformationObject
 ***********************************************************************/

typedef struct sPackedOutputCircuitInfoWithCP56Time2a* PackedOutputCircuitInfoWithCP56Time2a;

void
PackedOutputCircuitInfoWithCP56Time2a_destroy(PackedOutputCircuitInfoWithCP56Time2a self);

PackedOutputCircuitInfoWithCP56Time2a
PackedOutputCircuitInfoWithCP56Time2a_create(PackedOutputCircuitInfoWithCP56Time2a self, int ioa,
        OutputCircuitInfo oci, QualityDescriptorP qdp, CP16Time2a operatingTime, CP56Time2a timestamp);

OutputCircuitInfo
PackedOutputCircuitInfoWithCP56Time2a_getOCI(PackedOutputCircuitInfoWithCP56Time2a self);

QualityDescriptorP
PackedOutputCircuitInfoWithCP56Time2a_getQuality(PackedOutputCircuitInfoWithCP56Time2a self);

CP16Time2a
PackedOutputCircuitInfoWithCP56Time2a_getOperatingTime(PackedOutputCircuitInfoWithCP56Time2a self);

CP56Time2a
PackedOutputCircuitInfoWithCP56Time2a_getTimestamp(PackedOutputCircuitInfoWithCP56Time2a self);

/**********************************************
 * DoubleCommandWithCP56Time2a : DoubleCommand
 **********************************************/

typedef struct sDoubleCommandWithCP56Time2a* DoubleCommandWithCP56Time2a;

void
DoubleCommandWithCP56Time2a_destroy(DoubleCommandWithCP56Time2a self);

DoubleCommandWithCP56Time2a
DoubleCommandWithCP56Time2a_create(DoubleCommandWithCP56Time2a self, int ioa, int command, bool selectCommand, int qu, CP56Time2a timestamp);

int
DoubleCommandWithCP56Time2a_getQU(DoubleCommandWithCP56Time2a self);

int
DoubleCommandWithCP56Time2a_getState(DoubleCommandWithCP56Time2a self);

bool
DoubleCommandWithCP56Time2a_isSelect(DoubleCommandWithCP56Time2a self);

/*************************************************
 * StepCommandWithCP56Time2a : InformationObject
 *************************************************/

typedef struct sStepCommandWithCP56Time2a* StepCommandWithCP56Time2a;

void
StepCommandWithCP56Time2a_destroy(StepCommand self);

StepCommandWithCP56Time2a
StepCommandWithCP56Time2a_create(StepCommandWithCP56Time2a self, int ioa, StepCommandValue command, bool selectCommand, int qu, CP56Time2a timestamp);

int
StepCommandWithCP56Time2a_getQU(StepCommandWithCP56Time2a self);

StepCommandValue
StepCommandWithCP56Time2a_getState(StepCommandWithCP56Time2a self);

bool
StepCommandWithCP56Time2a_isSelect(StepCommandWithCP56Time2a self);

/**********************************************************************
 * SetpointCommandNormalizedWithCP56Time2a : SetpointCommandNormalized
 **********************************************************************/

typedef struct sSetpointCommandNormalizedWithCP56Time2a* SetpointCommandNormalizedWithCP56Time2a;

void
SetpointCommandNormalizedWithCP56Time2a_destroy(SetpointCommandNormalizedWithCP56Time2a self);

SetpointCommandNormalizedWithCP56Time2a
SetpointCommandNormalizedWithCP56Time2a_create(SetpointCommandNormalizedWithCP56Time2a self, int ioa, float value, bool selectCommand, int ql, CP56Time2a timestamp);

float
SetpointCommandNormalizedWithCP56Time2a_getValue(SetpointCommandNormalizedWithCP56Time2a self);

int
SetpointCommandNormalizedWithCP56Time2a_getQL(SetpointCommandNormalizedWithCP56Time2a self);

bool
SetpointCommandNormalizedWithCP56Time2a_isSelect(SetpointCommandNormalizedWithCP56Time2a self);

/**********************************************************************
 * SetpointCommandScaledWithCP56Time2a : SetpointCommandScaled
 **********************************************************************/

typedef struct sSetpointCommandScaledWithCP56Time2a* SetpointCommandScaledWithCP56Time2a;

void
SetpointCommandScaledWithCP56Time2a_destroy(SetpointCommandScaledWithCP56Time2a self);

SetpointCommandScaledWithCP56Time2a
SetpointCommandScaledWithCP56Time2a_create(SetpointCommandScaledWithCP56Time2a self, int ioa, int value, bool selectCommand, int ql, CP56Time2a timestamp);

int
SetpointCommandScaledWithCP56Time2a_getValue(SetpointCommandScaledWithCP56Time2a self);

int
SetpointCommandScaledWithCP56Time2a_getQL(SetpointCommandScaledWithCP56Time2a self);

bool
SetpointCommandScaledWithCP56Time2a_isSelect(SetpointCommandScaledWithCP56Time2a self);

/**********************************************************************
 * SetpointCommandShortWithCP56Time2a : SetpointCommandShort
 **********************************************************************/

typedef struct sSetpointCommandShortWithCP56Time2a* SetpointCommandShortWithCP56Time2a;

void
SetpointCommandShortWithCP56Time2a_destroy(SetpointCommandShortWithCP56Time2a self);

SetpointCommandShortWithCP56Time2a
SetpointCommandShortWithCP56Time2a_create(SetpointCommandShortWithCP56Time2a self, int ioa, float value, bool selectCommand, int ql, CP56Time2a timestamp);

float
SetpointCommandShortWithCP56Time2a_getValue(SetpointCommandShortWithCP56Time2a self);

int
SetpointCommandShortWithCP56Time2a_getQL(SetpointCommandShortWithCP56Time2a self);

bool
SetpointCommandShortWithCP56Time2a_isSelect(SetpointCommandShortWithCP56Time2a self);

/*******************************************************
 * Bitstring32CommandWithCP56Time2a: Bitstring32Command
 *******************************************************/

typedef struct sBitstring32CommandWithCP56Time2a* Bitstring32CommandWithCP56Time2a;

Bitstring32CommandWithCP56Time2a
Bitstring32CommandWithCP56Time2a_create(Bitstring32CommandWithCP56Time2a self, int ioa, uint32_t value, CP56Time2a timestamp);

void
Bitstring32CommandWithCP56Time2a_destroy(Bitstring32CommandWithCP56Time2a self);

uint32_t
Bitstring32CommandWithCP56Time2a_getValue(Bitstring32CommandWithCP56Time2a self);

CP56Time2a
Bitstring32CommandWithCP56Time2a_getTimestamp(Bitstring32CommandWithCP56Time2a self);


/**************************************************
 * CounterInterrogationCommand : InformationObject
 **************************************************/

typedef struct sCounterInterrogationCommand* CounterInterrogationCommand;

CounterInterrogationCommand
CounterInterrogationCommand_create(CounterInterrogationCommand self, int ioa, QualifierOfCIC qcc);

void
CounterInterrogationCommand_destroy(CounterInterrogationCommand self);

QualifierOfCIC
CounterInterrogationCommand_getQCC(CounterInterrogationCommand self);

/*************************************************
 * TestCommand : InformationObject
 ************************************************/

typedef struct sTestCommand* TestCommand;

TestCommand
TestCommand_create(TestCommand self);

void
TestCommand_destroy(TestCommand self);

bool
TestCommand_isValid(TestCommand self);

/*************************************************
 * ResetProcessCommand : InformationObject
 ************************************************/

typedef struct sResetProcessCommand* ResetProcessCommand;

ResetProcessCommand
ResetProcessCommand_create(ResetProcessCommand self, int ioa, QualifierOfRPC qrp);

void
ResetProcessCommand_destroy(ResetProcessCommand self);

QualifierOfRPC
ResetProcessCommand_getQRP(ResetProcessCommand self);

/*************************************************
 * DelayAcquisitionCommand : InformationObject
 ************************************************/

typedef struct sDelayAcquisitionCommand* DelayAcquisitionCommand;

DelayAcquisitionCommand
DelayAcquisitionCommand_create(DelayAcquisitionCommand self, int ioa,  CP16Time2a delay);

void
DelayAcquisitionCommand_destroy(DelayAcquisitionCommand self);

CP16Time2a
DelayAcquisitionCommand_getDelay(DelayAcquisitionCommand self);

/*******************************************
 * EndOfInitialization : InformationObject
 *******************************************/

typedef struct sEndOfInitialization* EndOfInitialization;

EndOfInitialization
EndOfInitialization_create(EndOfInitialization self, uint8_t coi);

void
EndOfInitialization_destroy(EndOfInitialization self);

uint8_t
EndOfInitialization_getCOI(EndOfInitialization self);

/*******************************************
 * FileReady : InformationObject
 *******************************************/

/**
 * \name CS101_NOF
 *
 * \brief NOF (Name of file) values
 *
 * @{
 */

#define CS101_NOF_TRANSPARENT_FILE 1
#define CS101_NOF_DISTURBANCE_DATA 2
#define CS101_NOF_SEQUENCES_OF_EVENTS 3
#define CS101_NOF_SEQUENCES_OF_ANALOGUE_VALUES 4

/** @} */

/**
 * \name CS101_SCQ
 *
 * \brief SCQ (select and call qualifier) values
 *
 * @{
 */

#define CS101_SCQ_DEFAULT 0
#define CS101_SCQ_SELECT_FILE 1
#define CS101_SCQ_REQUEST_FILE 2
#define CS101_SCQ_DEACTIVATE_FILE 3
#define CS101_SCQ_DELETE_FILE 4
#define CS101_SCQ_SELECT_SECTION 5
#define CS101_SCQ_REQUEST_SECTION 6
#define CS101_SCQ_DEACTIVATE_SECTION 7

/** @} */

/**
 * \name CS101_LSQ
 *
 * \brief LSQ (last section or segment qualifier) values
 *
 * @{
 */

#define CS101_LSQ_NOT_USED 0,
#define CS101_LSQ_FILE_TRANSFER_WITHOUT_DEACT 1
#define CS101_LSQ_FILE_TRANSFER_WITH_DEACT 2
#define CS101_LSQ_SECTION_TRANSFER_WITHOUT_DEACT 3
#define CS101_LSQ_SECTION_TRANSFER_WITH_DEACT 4

/** @} */

/**
 * \name CS101_AFQ
 *
 * \brief AFQ (Acknowledge file or section qualifier) values
 *
 * @{
 */

/** \brief AFQ not used */
#define CS101_AFQ_NOT_USED 0

/** \brief acknowledge file positively */
#define CS101_AFQ_POS_ACK_FILE 1

/** \brief acknowledge file negatively */
#define CS101_AFQ_NEG_ACK_FILE 2

/** \brief acknowledge section positively */
#define CS101_AFQ_POS_ACK_SECTION 3

/** \brief acknowledge section negatively */
#define CS101_AFQ_NEG_ACK_SECTION 4

/** @} */

/**
 * \name CS101_FILE_ERROR
 *
 * \brief Error code values used by FileACK
 *
 * @{
 */

/** \brief no error */
#define CS101_FILE_ERROR_DEFAULT 0

/** \brief requested memory not available (not enough memory) */
#define CS101_FILE_ERROR_REQ_MEMORY_NOT_AVAILABLE 1

/** \brief checksum test failed */
#define CS101_FILE_ERROR_CHECKSUM_FAILED 2

/** \brief unexpected communication service */
#define CS101_FILE_ERROR_UNEXPECTED_COMM_SERVICE 3

/** \brief unexpected name of file */
#define CS101_FILE_ERROR_UNEXPECTED_NAME_OF_FILE 4

/** \brief unexpected name of section */
#define CS101_FILE_ERROR_UNEXPECTED_NAME_OF_SECTION 5

/** @} */

/**
 * \name CS101_SOF
 *
 * \brief Status of file (SOF) definitions - IEC 60870-5-101:2003 7.2.6.38
 *
 * @{
 */

/** \brief bit mask value for STATUS part of SOF */
#define CS101_SOF_STATUS 0x1f

/** \brief bit mask value for LFD (last file of the directory) flag */
#define CS101_SOF_LFD 0x20

/** \brief bit mask value for FOR (name defines subdirectory) flag */
#define CS101_SOF_FOR 0x40

/** \brief bit mask value for FA (file transfer of this file is active) flag */
#define CS101_SOF_FA 0x80

/** @} */

typedef struct sFileReady* FileReady;

/**
 * \brief Create a new instance of FileReady information object
 *
 * For message type: F_FR_NA_1 (120)
 *
 * \param self
 * \param ioa
 * \param nof name of file (1 for transparent file)
 * \param lengthOfFile
 * \param positive when true file is ready to transmit
 */
FileReady
FileReady_create(FileReady self, int ioa, uint16_t nof, uint32_t lengthOfFile, bool positive);

void
FileReady_destroy(FileReady self);

uint8_t
FileReady_getFRQ(FileReady self);

void
FileReady_setFRQ(FileReady self, uint8_t frq);

bool
FileReady_isPositive(FileReady self);

uint16_t
FileReady_getNOF(FileReady self);

uint32_t
FileReady_getLengthOfFile(FileReady self);

void
FileReady_destroy(FileReady self);

/*******************************************
 * SectionReady : InformationObject
 *******************************************/

typedef struct sSectionReady* SectionReady;

SectionReady
SectionReady_create(SectionReady self, int ioa, uint16_t nof, uint8_t nos, uint32_t lengthOfSection, bool notReady);


bool
SectionReady_isNotReady(SectionReady self);

uint8_t
SectionReady_getSRQ(SectionReady self);

void
SectionReady_setSRQ(SectionReady self, uint8_t srq);

uint16_t
SectionReady_getNOF(SectionReady self);

uint8_t
SectionReady_getNameOfSection(SectionReady self);

uint32_t
SectionReady_getLengthOfSection(SectionReady self);

void
SectionReady_destroy(SectionReady self);

/*******************************************
 * FileCallOrSelect : InformationObject
 *******************************************/


typedef struct sFileCallOrSelect* FileCallOrSelect;

FileCallOrSelect
FileCallOrSelect_create(FileCallOrSelect self, int ioa, uint16_t nof, uint8_t nos, uint8_t scq);

uint16_t
FileCallOrSelect_getNOF(FileCallOrSelect self);

uint8_t
FileCallOrSelect_getNameOfSection(FileCallOrSelect self);

uint8_t
FileCallOrSelect_getSCQ(FileCallOrSelect self);

void
FileCallOrSelect_destroy(FileCallOrSelect self);

/*************************************************
 * FileLastSegmentOrSection : InformationObject
 *************************************************/

typedef struct sFileLastSegmentOrSection* FileLastSegmentOrSection;

FileLastSegmentOrSection
FileLastSegmentOrSection_create(FileLastSegmentOrSection self, int ioa, uint16_t nof, uint8_t nos, uint8_t lsq, uint8_t chs);

uint16_t
FileLastSegmentOrSection_getNOF(FileLastSegmentOrSection self);

uint8_t
FileLastSegmentOrSection_getNameOfSection(FileLastSegmentOrSection self);

uint8_t
FileLastSegmentOrSection_getLSQ(FileLastSegmentOrSection self);

uint8_t
FileLastSegmentOrSection_getCHS(FileLastSegmentOrSection self);

void
FileLastSegmentOrSection_destroy(FileLastSegmentOrSection self);

/*************************************************
 * FileACK : InformationObject
 *************************************************/

typedef struct sFileACK* FileACK;

FileACK
FileACK_create(FileACK self, int ioa, uint16_t nof, uint8_t nos, uint8_t afq);

uint16_t
FileACK_getNOF(FileACK self);

uint8_t
FileACK_getNameOfSection(FileACK self);

uint8_t
FileACK_getAFQ(FileACK self);

void
FileACK_destroy(FileACK self);

/*************************************************
 * FileSegment : InformationObject
 *************************************************/

typedef struct sFileSegment* FileSegment;

FileSegment
FileSegment_create(FileSegment self, int ioa, uint16_t nof, uint8_t nos, uint8_t* data, uint8_t los);

uint16_t
FileSegment_getNOF(FileSegment self);

uint8_t
FileSegment_getNameOfSection(FileSegment self);

uint8_t
FileSegment_getLengthOfSegment(FileSegment self);

uint8_t*
FileSegment_getSegmentData(FileSegment self);

int
FileSegment_GetMaxDataSize(CS101_AppLayerParameters parameters);

void
FileSegment_destroy(FileSegment self);

/*************************************************
 * FileDirectory: InformationObject
 *************************************************/

typedef struct sFileDirectory* FileDirectory;

FileDirectory
FileDirectory_create(FileDirectory self, int ioa, uint16_t nof, int lengthOfFile, uint8_t sof, CP56Time2a creationTime);

uint16_t
FileDirectory_getNOF(FileDirectory self);

uint8_t
FileDirectory_getSOF(FileDirectory self);

int
FileDirectory_getSTATUS(FileDirectory self);

bool
FileDirectory_getLFD(FileDirectory self);

bool
FileDirectory_getFOR(FileDirectory self);

bool
FileDirectory_getFA(FileDirectory self);

uint8_t
FileDirectory_getLengthOfFile(FileDirectory self);

CP56Time2a
FileDirectory_getCreationTime(FileDirectory self);

void
FileDirectory_destroy(FileDirectory self);




//RemoteReposition
typedef struct sRemoteReposition* RemoteReposition;

RemoteReposition
RemoteReposition_create(RemoteReposition self, int ioa, uint16_t sn);//切换定值区

void
RemoteReposition_destroy(RemoteReposition self);

//RemoteReadSN===201
typedef struct sRemoteReadSN* RemoteReadSN;

RemoteReadSN
RemoteReadSN_create(RemoteReadSN self, int ioa);//读当前定值区号

void
RemoteReadSN_destroy(RemoteReadSN self);

int16_t//int
RemoteReadSN_getSN1(RemoteReadSN self);
int16_t//int
RemoteReadSN_getSN2(RemoteReadSN self);
int64_t//int
RemoteReadSN_getSN3(RemoteReadSN self);

//ParamValue_Read===202
typedef struct sParamValue_Read* ParamValue_Read;
//typedef struct sParamRead* ParamRead;

//ParamValue_Read
//ParamValue_Read_create(ParamValue_Read self, int ioa, uint16_t sn);//可变帧长限定词 VSQ有效
ParamValue_Read
ParamValue_Read_create(ParamValue_Read self, int ioa_index);//int sn, ParamRead

void
ParamValue_Read_destroy(ParamValue_Read self);

int8_t//int
ParamValue_Read_getSN(ParamValue_Read self);
int
ParamValue_Read_getParamReadnum(ParamValue_Read self);

//ParamRead
//ParamValue_Read_getParamReadstruct(ParamValue_Read self);
int
ParamValue_Read_getParamReadinfoaddr(ParamValue_Read self,int index);
int
ParamValue_Read_getParamReadtagtype(ParamValue_Read self,int index);
int
ParamValue_Read_getParamReaddatalen(ParamValue_Read self,int index);
char*
ParamValue_Read_getParamReaddatavalue(ParamValue_Read self,int index);

//ParamValue_Write===203
typedef struct sParamValue_Write* ParamValue_Write;

ParamValue_Write
ParamValue_Write_create(ParamValue_Write self, int ioa_index,int tag_index,int datalen,unsigned char* datavalue);//int sn, ParamRead paramRead ParamRead读参数和定值int ioa
//ParamValue_Write_create(ParamValue_Write self, int ioa, uint16_t sn,uint8_t param_TI,uint8_t Tag_type,uint8_t data_len,char* data_value);//可变帧长限定词 VSQ有效

void
ParamValue_Write_destroy(ParamValue_Write self);

unsigned char
ParamValue_Write_getParamTI(ParamValue_Write self);
unsigned char
ParamValue_Write_getSN(ParamValue_Write self);
unsigned char
ParamValue_Write_getParamWritenum(ParamValue_Write self);
int
ParamValue_Write_getParamWriteinfoaddr(ParamValue_Write self,int index);
unsigned char
ParamValue_Write_getParamWritetagtype(ParamValue_Write self,int index);
unsigned char
ParamValue_Write_getParamWritedatalen(ParamValue_Write self,int index);
char*
ParamValue_Write_getParamWritedatavalue(ParamValue_Write self,int index);

/*************************************************
 * FileServer(selfdefine) : InformationObject
 *************************************************/
//========== <210>	：文件服务 ==========//
typedef struct  sFileCallMenu* FileCallMenu;//文件目录召唤
FileCallMenu FileCallMenu_create(FileCallMenu self, int ioa, uint8_t operateType, uint32_t catalogueID,
                                 uint8_t catalogueNamelength, char* catalogueName, uint8_t callflag, CP56Time2a startTime, CP56Time2a endTime);
void FileCallMenu_destroy(FileCallMenu self);

typedef struct sFileCallMenuAffirm* FileCallMenuAffirm;
//FileCallMenuAffirm FileCallMenuAffirm_create(FileCallMenuAffirm self);
void FileCallMenuAffirm_destroy(FileCallMenuAffirm self);
int FileCallMenuAffirm_getOperateType(FileCallMenuAffirm self);
int FileCallMenuAffirm_getResultDescribe(FileCallMenuAffirm self);
int FileCallMenuAffirm_getCatalogueID(FileCallMenuAffirm self);
int FileCallMenuAffirm_getFilenum(FileCallMenuAffirm self);
int FileCallMenuAffirm_getFollowupFlag(FileCallMenuAffirm self);
int FileCallMenuAffirm_getFilenamelen(FileCallMenuAffirm self,int index);
int FileCallMenuAffirm_getFileProperty(FileCallMenuAffirm self,int index);
int FileCallMenuAffirm_getFileSize(FileCallMenuAffirm self,int index);
CP56Time2a FileCallMenuAffirm_getFileTime(FileCallMenuAffirm self,int index);
char* FileCallMenuAffirm_getFilename(FileCallMenuAffirm self,int index);


typedef struct  sFileActivate* FileActivate;//读文件激活
FileActivate FileActivate_create(FileActivate self, int ioa, uint8_t operateType, uint8_t fileNamelength, char* fileName);
void FileActivate_destroy(FileActivate self);


typedef struct  sFileActivateAffirm* FileActivateAffirm;//读文件激活确认
int FileActivateAffirm_getOperateType(FileActivateAffirm self);
int FileActivateAffirm_getResultDescribe(FileActivateAffirm self);
int FileActivateAffirm_getFilenamelen(FileActivateAffirm self);
char* FileActivateAffirm_getFilename(FileActivateAffirm self);
int FileActivateAffirm_getFileID(FileActivateAffirm self);
int FileActivateAffirm_getFilesize(FileActivateAffirm self);
void FileActivateAffirm_destroy(FileActivateAffirm self);

typedef struct  sFileTransfer* FileTransfer;//读文件数据传输

int FileTransfer_getOperateType(FileTransfer self);
int FileTransfer_getCurrentfiledataLen(FileTransfer self);
int FileTransfer_getFileID(FileTransfer self);
int FileTransfer_getSegmentnumber(FileTransfer self);
int FileTransfer_getFollowupFlag(FileTransfer self);
char* FileTransfer_getFileData(FileTransfer self);
int FileTransfer_getFileCheckSum(FileTransfer self);
void FileTransfer_destroy(FileTransfer self);

typedef struct  sFileTransferAffirm* FileTransferAffirm;//读文件数据传输确认
FileTransferAffirm FileTransferAffirm_create(FileTransferAffirm self, int ioa, uint8_t operateType, uint32_t fileID, uint32_t segmentnumber, uint8_t followupFlag);
void FileTransferAffirm_destroy(FileTransferAffirm self);


//Write
typedef struct  sFileActivateWrite* FileActivateWrite;//写文件激活
FileActivateWrite FileActivateWrite_create(FileActivateWrite self, int ioa, uint8_t operateType, uint8_t fileNamelength, char* fileName,uint32_t fileID, uint32_t fileSize);
void FileActivateWrite_destroy(FileActivateWrite self);

typedef struct  sFileActivateAffirmWrite* FileActivateAffirmWrite;//写文件激活确认(接收)
int FileActivateAffirmWrite_getOperateType(FileActivateAffirmWrite self);
int FileActivateAffirmWrite_getResultDescribe(FileActivateAffirmWrite self);
int FileActivateAffirmWrite_getFilenamelen(FileActivateAffirmWrite self);
char* FileActivateAffirmWrite_getFilename(FileActivateAffirmWrite self);
unsigned int FileActivateAffirmWrite_getFileID(FileActivateAffirmWrite self);
unsigned int FileActivateAffirmWrite_getFilesize(FileActivateAffirmWrite self);
void FileActivateAffirmWrite_destroy(FileActivateAffirmWrite self);

typedef struct  sFileTransferWrite* FileTransferWrite;//写文件数据传输
FileTransferWrite FileTransferWrite_create(FileTransferWrite self, int ioa, uint8_t operateType, uint32_t fileID, uint32_t segmentnumber, uint8_t followupFlag,char* fileData,uint8_t fileCheckSum);
void FileTransferWrite_destroy(FileTransferWrite self);

typedef struct  sFileTransferAffirmWrite* FileTransferAffirmWrite;//写文件数据传输确认(接收)
//FileTransferAffirmWrite FileTransferAffirmWrite_create(FileTransferAffirmWrite self, int ioa, uint8_t operateType, uint32_t fileID, uint32_t segmentnumber, uint8_t resultDescribe);
int FileTransferAffirmWrite_getOperateType(FileTransferAffirmWrite self);
unsigned char FileTransferAffirmWrite_getResultDescribe(FileTransferAffirmWrite self);
unsigned int FileTransferAffirmWrite_getFileID(FileTransferAffirmWrite self);
unsigned int FileTransferAffirmWrite_getFilesegmentnumber(FileTransferAffirmWrite self);

void FileTransferAffirmWrite_destroy(FileTransferAffirmWrite self);

//========== <210>	：文件服务 ==========//

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SRC_INC_INFORMATION_OBJECTS_H_ */
