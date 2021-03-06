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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
//#include <QString>
//#include <qcstring.h>
//#include <QByteArray>

#include "iec60870_common.h"
#include "apl_types_internal.h"
#include "cs101_information_objects.h"
#include "information_objects_internal.h"
#include "lib_memory.h"
#include "frame.h"
#include "platform_endian.h"
#include "lib60870_internal.h"

typedef bool (*EncodeFunction)(InformationObject self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence);
typedef void (*DestroyFunction)(InformationObject self);

struct sInformationObjectVFT {
    EncodeFunction encode;
    DestroyFunction destroy;
#if 0
    const char* (*toString)(InformationObject self);
#endif
};


/*****************************************
 * Basic data types
 ****************************************/

void
SingleEvent_setEventState(SingleEvent self, EventState eventState)
{
    uint8_t value = *self;

    value &= 0xfc;

    value += eventState;

    *self = value;
}

EventState
SingleEvent_getEventState(SingleEvent self)
{
    return (EventState) (*self & 0x3);
}

void
SingleEvent_setQDP(SingleEvent self, QualityDescriptorP qdp)
{
    uint8_t value = *self;

    value &= 0x03;

    value += qdp;

    *self = value;
}

QualityDescriptorP
SingleEvent_getQDP(SingleEvent self)
{
    return (QualityDescriptor) (*self & 0xfc);
}

uint16_t
StatusAndStatusChangeDetection_getSTn(StatusAndStatusChangeDetection self)
{
    return (uint16_t) (self->encodedValue[0] + 256 * self->encodedValue[1]);
}

uint16_t
StatusAndStatusChangeDetection_getCDn(StatusAndStatusChangeDetection self)
{
    return (uint16_t) (self->encodedValue[2] + 256 * self->encodedValue[3]);
}

void
StatusAndStatusChangeDetection_setSTn(StatusAndStatusChangeDetection self, uint16_t value)
{
    self->encodedValue[0] = (uint8_t) (value % 256);
    self->encodedValue[1] = (uint8_t) (value / 256);
}

bool
StatusAndStatusChangeDetection_getST(StatusAndStatusChangeDetection self, int index)
{
    if ((index >= 0) && (index < 16))
        return ((int) (StatusAndStatusChangeDetection_getSTn(self) & (2^index)) != 0);
    else
        return false;
}

bool
StatusAndStatusChangeDetection_getCD(StatusAndStatusChangeDetection self, int index)
{
    if ((index >= 0) && (index < 16))
        return ((int) (StatusAndStatusChangeDetection_getCDn(self) & (2^index)) != 0);
    else
        return false;
}


/*****************************************
 * Information object hierarchy
 *****************************************/

/*****************************************
 * InformationObject (base class)
 *****************************************/

struct sInformationObject {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;
};

bool
InformationObject_encode(InformationObject self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    return self->virtualFunctionTable->encode(self, frame, parameters, isSequence);
}

void
InformationObject_destroy(InformationObject self)
{
    self->virtualFunctionTable->destroy(self);
}

int
InformationObject_getObjectAddress(InformationObject self)
{
    return self->objectAddress;
}

void
InformationObject_setObjectAddress(InformationObject self, int ioa)
{
    self->objectAddress = ioa;
}

TypeID
InformationObject_getType(InformationObject self)
{
    return self->type;
}

static void
InformationObject_encodeBase(InformationObject self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    if (!isSequence) {
        Frame_setNextByte(frame, (uint8_t)(self->objectAddress & 0xff));

        if (parameters->sizeOfIOA > 1)
            Frame_setNextByte(frame, (uint8_t)((self->objectAddress / 0x100) & 0xff));

        if (parameters->sizeOfIOA > 2)
            Frame_setNextByte(frame, (uint8_t)((self->objectAddress / 0x10000) & 0xff));
    }
}

int
InformationObject_ParseObjectAddress(CS101_AppLayerParameters parameters, uint8_t* msg, int startIndex)
{
    /* parse information object address */
    int ioa = msg [startIndex];

    if (parameters->sizeOfIOA > 1)
        ioa += (msg [startIndex + 1] * 0x100);

    if (parameters->sizeOfIOA > 2)
        ioa += (msg [startIndex + 2] * 0x10000);

    return ioa;
}

static void
InformationObject_getFromBuffer(InformationObject self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int startIndex)
{
    /* parse information object address */
    self->objectAddress = InformationObject_ParseObjectAddress(parameters, msg, startIndex);
}

int
InformationObject_getMaxSizeInMemory()
{
    return sizeof(union uInformationObject);
}

/**********************************************
 * SinglePointInformation  M_SP_NA_1=1  单点遥信
 **********************************************/

static bool
SinglePointInformation_encode(SinglePointInformation self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t val = (uint8_t) self->quality;

    if (self->value)
        val++;

    Frame_setNextByte(frame, val);

    return true;
}

struct sInformationObjectVFT singlePointInformationVFT = {
        (EncodeFunction) SinglePointInformation_encode,
        (DestroyFunction) SinglePointInformation_destroy
};

static void
SinglePointInformation_initialize(SinglePointInformation self)
{
    self->virtualFunctionTable = &(singlePointInformationVFT);
    self->type = M_SP_NA_1;
}

SinglePointInformation
SinglePointInformation_create(SinglePointInformation self, int ioa, bool value,
        QualityDescriptor quality)
{
    if (self == NULL)
        self = (SinglePointInformation) GLOBAL_CALLOC(1, sizeof(struct sSinglePointInformation));

    if (self != NULL) {
        SinglePointInformation_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
    }

    return self;
}

void
SinglePointInformation_destroy(SinglePointInformation self)
{
    GLOBAL_FREEMEM(self);
}

SinglePointInformation
SinglePointInformation_getFromBuffer(SinglePointInformation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (SinglePointInformation) GLOBAL_MALLOC(sizeof(struct sSinglePointInformation));

    if (self != NULL) {
        SinglePointInformation_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse SIQ (single point information with qualitiy) */
        uint8_t siq = msg [startIndex];

        self->value = ((siq & 0x01) == 0x01);

        self->quality = (QualityDescriptor) (siq & 0xf0);
    }

    return self;
}

bool
SinglePointInformation_getValue(SinglePointInformation self)
{
    return self->value;
}

QualityDescriptor
SinglePointInformation_getQuality(SinglePointInformation self)
{
    return self->quality;
}


/**********************************************
 * StepPositionInformation  M_ST_NA_1=5  步位置信息
 **********************************************/

static bool
StepPositionInformation_encode(StepPositionInformation self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 2 : (parameters->sizeOfIOA + 2);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->vti);

    Frame_setNextByte(frame, (uint8_t) self->quality);

    return true;
}

struct sInformationObjectVFT stepPositionInformationVFT = {
        (EncodeFunction) StepPositionInformation_encode,
        (DestroyFunction) StepPositionInformation_destroy
};

static void
StepPositionInformation_initialize(StepPositionInformation self)
{
    self->virtualFunctionTable = &(stepPositionInformationVFT);
    self->type = M_ST_NA_1;
}

StepPositionInformation
StepPositionInformation_create(StepPositionInformation self, int ioa, int value, bool isTransient,
        QualityDescriptor quality)
{
    if (self == NULL)
        self = (StepPositionInformation) GLOBAL_CALLOC(1, sizeof(struct sStepPositionInformation));

    if (self != NULL) {
        StepPositionInformation_initialize(self);

        self->objectAddress = ioa;

        if (value > 63)
            value = 63;
        else if (value < -64)
            value = -64;

        if (value < 0)
            value = value + 128;

        self->vti = value;

        if (isTransient)
            self->vti |= 0x80;

        self->quality = quality;
    }

    return self;
}

void
StepPositionInformation_destroy(StepPositionInformation self)
{
    GLOBAL_FREEMEM(self);
}

int
StepPositionInformation_getObjectAddress(StepPositionInformation self)
{
    return self->objectAddress;
}

int
StepPositionInformation_getValue(StepPositionInformation self)
{
    int value = (self->vti & 0x7f);

    if (value > 63)
        value = value - 128;

    return value;
}

bool
StepPositionInformation_isTransient(StepPositionInformation self)
{
    return ((self->vti & 0x80) == 0x80);
}

QualityDescriptor
StepPositionInformation_getQuality(StepPositionInformation self)
{
    return self->quality;
}


StepPositionInformation
StepPositionInformation_getFromBuffer(StepPositionInformation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (StepPositionInformation) GLOBAL_MALLOC(sizeof(struct sStepPositionInformation));

    if (self != NULL) {
        StepPositionInformation_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse VTI (value with transient state indication) */
        self->vti = msg [startIndex++];

        self->quality = (QualityDescriptor) msg [startIndex];
    }

    return self;
}


/**********************************************
 * StepPositionWithCP56Time2a  M_ST_TB_1=32  带时标CP56Time2a的步位置信息
 **********************************************/

static bool
StepPositionWithCP56Time2a_encode(StepPositionWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 9 : (parameters->sizeOfIOA + 9);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->vti);

    Frame_setNextByte(frame, (uint8_t) self->quality);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return false;
}

struct sInformationObjectVFT stepPositionWithCP56Time2aVFT = {
        (EncodeFunction) StepPositionWithCP56Time2a_encode,
        (DestroyFunction) StepPositionWithCP56Time2a_destroy
};

static void
StepPositionWithCP56Time2a_initialize(StepPositionWithCP56Time2a self)
{
    self->virtualFunctionTable = &(stepPositionWithCP56Time2aVFT);
    self->type = M_ST_TB_1;
}

void
StepPositionWithCP56Time2a_destroy(StepPositionWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

StepPositionWithCP56Time2a
StepPositionWithCP56Time2a_create(StepPositionWithCP56Time2a self, int ioa, int value, bool isTransient,
        QualityDescriptor quality, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (StepPositionWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sStepPositionWithCP56Time2a));

    if (self != NULL) {
        StepPositionWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        if (value > 63)
            value = 63;
        else if (value < -64)
            value = -64;

        if (value < 0)
            value = value + 128;

        self->vti = value;

        if (isTransient)
            self->vti |= 0x80;

        self->quality = quality;

        self->timestamp = *timestamp;
    }

    return self;
}

CP56Time2a
StepPositionWithCP56Time2a_getTimestamp(StepPositionWithCP56Time2a self)
{
    return &(self->timestamp);
}

StepPositionWithCP56Time2a
StepPositionWithCP56Time2a_getFromBuffer(StepPositionWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (StepPositionWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sStepPositionWithCP56Time2a));

    if (self != NULL) {
        StepPositionWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse VTI (value with transient state indication) */
        self->vti = msg [startIndex++];

        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/**********************************************
 * StepPositionWithCP24Time2a  M_ST_TA_1=6  带时标CP24Time2a的步位置信息
 **********************************************/

static bool
StepPositionWithCP24Time2a_encode(StepPositionWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 5 : (parameters->sizeOfIOA + 5);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->vti);

    Frame_setNextByte(frame, (uint8_t) self->quality);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT stepPositionWithCP24Time2aVFT = {
        (EncodeFunction) StepPositionWithCP24Time2a_encode,
        (DestroyFunction) StepPositionWithCP24Time2a_destroy
};

static void
StepPositionWithCP24Time2a_initialize(StepPositionWithCP24Time2a self)
{
    self->virtualFunctionTable = &(stepPositionWithCP24Time2aVFT);
    self->type = M_ST_TA_1;
}

void
StepPositionWithCP24Time2a_destroy(StepPositionWithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

StepPositionWithCP24Time2a
StepPositionWithCP24Time2a_create(StepPositionWithCP24Time2a self, int ioa, int value, bool isTransient,
        QualityDescriptor quality, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (StepPositionWithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sStepPositionWithCP24Time2a));

    if (self != NULL) {
        StepPositionWithCP24Time2a_initialize(self);

        self->objectAddress = ioa;

        if (value > 63)
            value = 63;
        else if (value < -64)
            value = -64;

        if (value < 0)
            value = value + 128;

        self->vti = value;

        if (isTransient)
            self->vti |= 0x80;

        self->quality = quality;

        self->timestamp = *timestamp;
    }

    return self;
}


CP24Time2a
StepPositionWithCP24Time2a_getTimestamp(StepPositionWithCP24Time2a self)
{
    return &(self->timestamp);
}

StepPositionWithCP24Time2a
StepPositionWithCP24Time2a_getFromBuffer(StepPositionWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (StepPositionWithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sStepPositionWithCP24Time2a));

    if (self != NULL) {
        StepPositionWithCP24Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse VTI (value with transient state indication) */
        self->vti = msg [startIndex++];

        self->quality = (QualityDescriptor) msg [startIndex];

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/**********************************************
 * DoublePointInformation  M_DP_NA_1=3  双点遥信
 **********************************************/

static bool
DoublePointInformation_encode(DoublePointInformation self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t val = (uint8_t) self->quality;

    val += (int) self->value;

    Frame_setNextByte(frame, val);

    return true;
}

struct sInformationObjectVFT doublePointInformationVFT = {
        (EncodeFunction) DoublePointInformation_encode,
        (DestroyFunction) DoublePointInformation_destroy
};

void
DoublePointInformation_destroy(DoublePointInformation self)
{
    GLOBAL_FREEMEM(self);
}

static void
DoublePointInformation_initialize(DoublePointInformation self)
{
    self->virtualFunctionTable = &(doublePointInformationVFT);
    self->type = M_DP_NA_1;
}

DoublePointInformation
DoublePointInformation_create(DoublePointInformation self, int ioa, DoublePointValue value,
        QualityDescriptor quality)
{
    if (self == NULL)
        self = (DoublePointInformation) GLOBAL_CALLOC(1, sizeof(struct sDoublePointInformation));

    if (self != NULL)
        DoublePointInformation_initialize(self);

    self->objectAddress = ioa;
    self->value = value;
    self->quality = quality;

    return self;
}

DoublePointValue
DoublePointInformation_getValue(DoublePointInformation self)
{
    return self->value;
}

QualityDescriptor
DoublePointInformation_getQuality(DoublePointInformation self)
{
    return self->quality;
}

DoublePointInformation
DoublePointInformation_getFromBuffer(DoublePointInformation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (DoublePointInformation) GLOBAL_MALLOC(sizeof(struct sDoublePointInformation));

    if (self != NULL) {
        DoublePointInformation_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse DIQ (double point information with quality) */
        uint8_t diq = msg [startIndex++];

        self->value = (DoublePointValue) (diq & 0x03);

        self->quality = (QualityDescriptor) (diq & 0xf0);
    }

    return self;
}


/*******************************************
 * DoublePointWithCP24Time2a  M_DP_TA_1=4  带时标CP24Time2a的双点遥信
 *******************************************/

static bool
DoublePointWithCP24Time2a_encode(DoublePointWithCP24Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 4 : (parameters->sizeOfIOA + 4);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t val = (uint8_t) self->quality;

    val += (int) self->value;

    Frame_setNextByte(frame, val);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}


struct sInformationObjectVFT doublePointWithCP24Time2aVFT = {
        (EncodeFunction) DoublePointWithCP24Time2a_encode,
        (DestroyFunction) DoublePointWithCP24Time2a_destroy
};

void
DoublePointWithCP24Time2a_destroy(DoublePointWithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

static void
DoublePointWithCP24Time2a_initialize(DoublePointWithCP24Time2a self)
{
    self->virtualFunctionTable = &(doublePointWithCP24Time2aVFT);
    self->type = M_DP_TA_1;
}

DoublePointWithCP24Time2a
DoublePointWithCP24Time2a_create(DoublePointWithCP24Time2a self, int ioa, DoublePointValue value,
        QualityDescriptor quality, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (DoublePointWithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sDoublePointWithCP24Time2a));

    if (self != NULL) {
        DoublePointWithCP24Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}

CP24Time2a
DoublePointWithCP24Time2a_getTimestamp(DoublePointWithCP24Time2a self)
{
    return &(self->timestamp);
}

DoublePointWithCP24Time2a
DoublePointWithCP24Time2a_getFromBuffer(DoublePointWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (DoublePointWithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sDoublePointWithCP24Time2a));

    if (self != NULL) {
        DoublePointWithCP24Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse DIQ (double point information with quality) */
        uint8_t diq = msg [startIndex++];

        self->value = (DoublePointValue) (diq & 0x03);

        self->quality = (QualityDescriptor) (diq & 0xf0);

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*******************************************
 * DoublePointWithCP56Time2a  M_DP_TB_1=31 带时标CP56Time2a的双点遥信
 *******************************************/

static bool
DoublePointWithCP56Time2a_encode(DoublePointWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t val = (uint8_t) self->quality;

    val += (int) self->value;

    Frame_setNextByte(frame, val);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}



struct sInformationObjectVFT doublePointWithCP56Time2aVFT = {
        (EncodeFunction) DoublePointWithCP56Time2a_encode,
        (DestroyFunction) DoublePointWithCP56Time2a_destroy
};

void
DoublePointWithCP56Time2a_destroy(DoublePointWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

static void
DoublePointWithCP56Time2a_initialize(DoublePointWithCP56Time2a self)
{
    self->virtualFunctionTable = &(doublePointWithCP56Time2aVFT);
    self->type = M_DP_TB_1;
}

DoublePointWithCP56Time2a
DoublePointWithCP56Time2a_create(DoublePointWithCP56Time2a self, int ioa, DoublePointValue value,
        QualityDescriptor quality, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (DoublePointWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sDoublePointWithCP56Time2a));

    if (self != NULL) {
        DoublePointWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}


CP56Time2a
DoublePointWithCP56Time2a_getTimestamp(DoublePointWithCP56Time2a self)
{
    return &(self->timestamp);
}

DoublePointWithCP56Time2a
DoublePointWithCP56Time2a_getFromBuffer(DoublePointWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (DoublePointWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sDoublePointWithCP56Time2a));

    if (self != NULL) {
        DoublePointWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse DIQ (double point information with quality) */
        uint8_t diq = msg [startIndex++];

        self->value = (DoublePointValue) (diq & 0x03);

        self->quality = (QualityDescriptor) (diq & 0xf0);

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*******************************************
 * SinglePointWithCP24Time2a  M_SP_TA_1=2  带时标CP24Time2a的单点遥信
 *******************************************/

static bool
SinglePointWithCP24Time2a_encode(SinglePointWithCP24Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 4 : (parameters->sizeOfIOA + 4);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t val = (uint8_t) self->quality;

    if (self->value)
        val++;

    Frame_setNextByte(frame, val);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT singlePointWithCP24Time2aVFT = {
        (EncodeFunction) SinglePointWithCP24Time2a_encode,
        (DestroyFunction) SinglePointWithCP24Time2a_destroy
};

void
SinglePointWithCP24Time2a_destroy(SinglePointWithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

static void
SinglePointWithCP24Time2a_initialize(SinglePointWithCP24Time2a self)
{
    self->virtualFunctionTable = &(singlePointWithCP24Time2aVFT);
    self->type = M_SP_TA_1;
}

SinglePointWithCP24Time2a
SinglePointWithCP24Time2a_create(SinglePointWithCP24Time2a self, int ioa, bool value,
        QualityDescriptor quality, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (SinglePointWithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sSinglePointWithCP24Time2a));

    if (self != NULL) {
        SinglePointWithCP24Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}

CP24Time2a
SinglePointWithCP24Time2a_getTimestamp(SinglePointWithCP24Time2a self)
{
    return &(self->timestamp);
}

SinglePointWithCP24Time2a
SinglePointWithCP24Time2a_getFromBuffer(SinglePointWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (SinglePointWithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sSinglePointWithCP24Time2a));

    if (self != NULL) {
        SinglePointWithCP24Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse SIQ (single point information with qualitiy) */
        uint8_t siq = msg [startIndex++];

        self->value = ((siq & 0x01) == 0x01);

        self->quality = (QualityDescriptor) (siq & 0xf0);

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}



/*******************************************
 * SinglePointWithCP56Time2a  M_SP_TB_1=30  带时标CP56Time2a的单点遥信
 *******************************************/

static bool
SinglePointWithCP56Time2a_encode(SinglePointWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t val = (uint8_t) self->quality;

    if (self->value)
        val++;

    Frame_setNextByte(frame, val);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}


struct sInformationObjectVFT singlePointWithCP56Time2aVFT = {
        (EncodeFunction) SinglePointWithCP56Time2a_encode,
        (DestroyFunction) SinglePointWithCP56Time2a_destroy
};

static void
SinglePointWithCP56Time2a_initialize(SinglePointWithCP56Time2a self)
{
    self->virtualFunctionTable = &(singlePointWithCP56Time2aVFT);
    self->type = M_SP_TB_1;
}

SinglePointWithCP56Time2a
SinglePointWithCP56Time2a_create(SinglePointWithCP56Time2a self, int ioa, bool value,
        QualityDescriptor quality, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (SinglePointWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sSinglePointWithCP56Time2a));

    if (self != NULL) {
        SinglePointWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}

void
SinglePointWithCP56Time2a_destroy(SinglePointWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

CP56Time2a
SinglePointWithCP56Time2a_getTimestamp(SinglePointWithCP56Time2a self)
{
    return &(self->timestamp);
}


SinglePointWithCP56Time2a
SinglePointWithCP56Time2a_getFromBuffer(SinglePointWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (SinglePointWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSinglePointWithCP56Time2a));

    if (self != NULL) {
        SinglePointWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* parse SIQ (single point information with qualitiy) */
        uint8_t siq = msg [startIndex++];

        self->value = ((siq & 0x01) == 0x01);

        self->quality = (QualityDescriptor) (siq & 0xf0);

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

//新增42===故障事件
/*******************************************
 * FaultEventWithCP56Time2a  M_FT_NA_1=42  故障事件信息
 *******************************************/
static bool
FaultEventWithCP56Time2a_encode(FaultEventWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{

    //    if(self->isEncodeYXelseYC && (self->objectAddress >0 && self->objectAddress<0x4001))//状态量信息  1-4000
    //    {
    //        if(self->isEncodefirstframe)//首次组遥信包
    //        {
    //            self->isEncodefirstframe=false;
    //            Frame_setNextByte(frame, self->num_YX);
    //            Frame_setNextByte(frame, self->type_YX);
    //        }
    //        int size_yx = isSequence ? 8 : (parameters->sizeOfIOA + 8);
    //        InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);
    //        Frame_setNextByte(frame, self->value_YX);
    //        //timestamp
    //        Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    //    }
    //    else if(self->objectAddress >=0x4001 && self->objectAddress<0x6001)//模拟量信息  4001-6000
    //    {
    //        if(self->isEncodefirstframe)//首次组遥测包
    //        {
    //            self->isEncodefirstframe=false;
    //            Frame_setNextByte(frame, self->num_YC);
    //            Frame_setNextByte(frame, self->type_YC);
    //        }

    //        int size_yc = 0;
    //        if(self->type_YC ==9||self->type_YC ==34)//归一化值
    //        {
    //            size_yc = isSequence ? 2 : (parameters->sizeOfIOA + 2);
    //            InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    //            int valueToEncode;

    //            if (self->value_YC < 0)
    //                valueToEncode = self->value_YC + 65536;
    //            else
    //                valueToEncode = self->value_YC;

    //            //encodedValue[0] = (uint8_t) (valueToEncode % 256);
    //            //encodedValue[1] = (uint8_t) (valueToEncode / 256);

    //            Frame_setNextByte(frame, (uint8_t) (valueToEncode % 256));
    //            Frame_setNextByte(frame, (uint8_t) (valueToEncode / 256));
    //        }
    //        else if(self->type_YC ==13||self->type_YC ==36)//浮点型
    //        {
    //            size_yc = isSequence ? 4 : (parameters->sizeOfIOA + 4);
    //            InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);
    //            uint8_t* valueBytes = (uint8_t*) &(self->value_YC);

    //        #if (ORDER_LITTLE_ENDIAN == 1)
    //            Frame_appendBytes(frame, valueBytes, 4);
    //        #else
    //            Frame_setNextByte(frame, valueBytes[3]);
    //            Frame_setNextByte(frame, valueBytes[2]);
    //            Frame_setNextByte(frame, valueBytes[1]);
    //            Frame_setNextByte(frame, valueBytes[0]);
    //        #endif
    //        }

    //        if(size_yc == 0)
    //        {
    //            //error log print
    //            return false;
    //        }


    //    }
    //    else
    //    {
    //        //error log print
    //        return false;
    //    }


        return true;
}


struct sInformationObjectVFT FaultEventWithCP56Time2aVFT ={
       (EncodeFunction) FaultEventWithCP56Time2a_encode,
       (DestroyFunction) FaultEventWithCP56Time2a_destroy
};

static void
FaultEventWithCP56Time2a_initialize(FaultEventWithCP56Time2a self)
{
    self->virtualFunctionTable = &(FaultEventWithCP56Time2aVFT);
    self->type = M_FT_NA_1;
}

//FaultEventWithCP56Time2a
//FaultEventWithCP56Time2a_create(FaultEventWithCP56Time2a self, int ioa, bool value_yx, float value_yc,
//                                int type,int num,bool isEncodeYXelseYC,bool isEncodefirstframe,CP56Time2a timestamp)
//{
//    if (self == NULL)
//        self = (FaultEventWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sFaultEventWithCP56Time2a));

//    if (self != NULL) {
//        FaultEventWithCP56Time2a_initialize(self);

//        //self->objectAddress = ioa;

//        if(isEncodeYXelseYC)
//        {
//            self->isEncodeYXelseYC = true;
//            self->objectAddress = ioa;
//            self->value_YX = value_yx;
//            self->timestamp = *timestamp;
//            if(isEncodefirstframe)
//            {
//                self->isEncodefirstframe = true;
//                self->num_YX = num;
//                self->type_YX = type;
//            }
//            else
//            {
//                //self->num_YX = 0;
//                self->isEncodefirstframe = false;
//            }
//        }
//        else
//        {
//            self->isEncodeYXelseYC = false;
//            self->objectAddress = ioa;
//            self->value_YC = value_yc;
//            //self->timestamp_YC = *timestamp;
//            if(isEncodefirstframe)
//            {
//                self->num_YC = num;
//                self->type_YC = type;
//            }
//            else
//            {
//                //self->num_YC = 0;
//                self->isEncodefirstframe = false;
//            }
//        }


//        //self->quality = quality;


//    }

//    return self;
//}




FaultEventWithCP56Time2a
FaultEventWithCP56Time2a_getFromBuffer(FaultEventWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence,int bytelenYXdot)
{
    //TODO check message size

    if (self == NULL)
        self = (FaultEventWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sFaultEventWithCP56Time2a));

    if (self != NULL) {
        FaultEventWithCP56Time2a_initialize(self);

//        if (!isSequence) {
//            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);
//            startIndex += parameters->sizeOfIOA; /* skip IOA */
//        }
        //对故障事件而言:遥测信息体地址字节数为3位，遥信信息体地址字节数可为2位也可为3位(默认2位)

        //YX
        self->num_YX = msg [startIndex++];//带时标遥信个数
        self->type_YX = msg [startIndex++];//遥信类型======单点=1 双点=3
        //int bytelen_YX = 2;//====debug
        int i=0;
        for(i=0;i<self->num_YX;i++)
        {
            //故障遥信点号  2 字节(change to 3 bytes for YX_infomation_addr)
            if(bytelenYXdot == 3)
            {
                self->structFaultEven_YX[i].addr = msg [startIndex++];
                self->structFaultEven_YX[i].addr += (msg [startIndex++] * 0x100);
                self->structFaultEven_YX[i].addr += (msg [startIndex++] * 0x10000);
            }
            else //默认2位
            {
                self->structFaultEven_YX[i].addr = msg [startIndex++];
                self->structFaultEven_YX[i].addr += (msg [startIndex++] * 0x100);
            }


            //遥信值  1 字节
            self->structFaultEven_YX[i].State = msg [startIndex++];
            //故障时刻时标 CP56Time2a  7 字节
            CP56Time2a_getFromBuffer(&(self->structFaultEven_YX[i].timestamp), msg, msgSize, startIndex);
            startIndex += 7;
        }

        //YC
        self->num_YC = msg [startIndex++];//遥测个数
        self->type_YC = msg [startIndex++];//遥测类型
        for(i=0;i<self->num_YC;i++)
        {
            //遥测信息体地址 1  3 字节
            self->structFaultEven_YC[i].addr = msg [startIndex++];
            self->structFaultEven_YC[i].addr += (msg [startIndex++] * 0x100);
            self->structFaultEven_YC[i].addr += (msg [startIndex++] * 0x10000);
            //故障时刻数据 1 归一化值（或 IEEE STD745 短浮点数）  2 字节（4 字节）
            if(self->type_YC == 13)//浮点型
            {
                uint8_t* valueBytes = (uint8_t*) &(self->structFaultEven_YC[i].Yc_Value);

                #if (ORDER_LITTLE_ENDIAN == 1)
                        valueBytes[0] = msg [startIndex++];
                        valueBytes[1] = msg [startIndex++];
                        valueBytes[2] = msg [startIndex++];
                        valueBytes[3] = msg [startIndex++];
                #else
                        valueBytes[3] = msg [startIndex++];
                        valueBytes[2] = msg [startIndex++];
                        valueBytes[1] = msg [startIndex++];
                        valueBytes[0] = msg [startIndex++];
                #endif
            }
            else if(self->type_YC == 9||self->type_YC == 11)//归一化
            {
                self->structFaultEven_YC[i].Yc_Value = msg [startIndex++];
                self->structFaultEven_YC[i].Yc_Value += (msg [startIndex++] * 0x100);
            }

        }

    }

    return self;
}

int FaultEventWithCP56Time2a_getYXnum(FaultEventWithCP56Time2a self)
{
    return (self->num_YX);
}

int FaultEventWithCP56Time2a_getYXtype(FaultEventWithCP56Time2a self)
{
    return (self->type_YX);
}

int FaultEventWithCP56Time2a_getYXaddr(FaultEventWithCP56Time2a self,int index)
{
    return (self->structFaultEven_YX[index].addr);
}

int FaultEventWithCP56Time2a_getYXdata(FaultEventWithCP56Time2a self,int index)
{
    return (self->structFaultEven_YX[index].State);
}

CP56Time2a
FaultEventWithCP56Time2a_getTimestamp(FaultEventWithCP56Time2a self,int index)
{
    return &(self->structFaultEven_YX[index].timestamp);
}

int FaultEventWithCP56Time2a_getYCnum(FaultEventWithCP56Time2a self)
{
    return (self->num_YC);
}

int FaultEventWithCP56Time2a_getYCtype(FaultEventWithCP56Time2a self)
{
    return (self->type_YC);
}

int FaultEventWithCP56Time2a_getYCaddr(FaultEventWithCP56Time2a self,int index)
{
    return (self->structFaultEven_YC[index].addr);
}

float FaultEventWithCP56Time2a_getYCdata(FaultEventWithCP56Time2a self,int index)
{
    return (self->structFaultEven_YC[index].Yc_Value);
}

void
FaultEventWithCP56Time2a_destroy(FaultEventWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}


/**********************************************
 * BitString32  M_BO_NA_1=7  32比特串
 **********************************************/

static bool
BitString32_encode(BitString32 self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 5 : (parameters->sizeOfIOA + 5);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    int value = self->value;

    Frame_setNextByte(frame, (uint8_t) (value % 0x100));
    Frame_setNextByte(frame, (uint8_t) ((value / 0x100) % 0x100));
    Frame_setNextByte(frame, (uint8_t) ((value / 0x10000) % 0x100));
    Frame_setNextByte(frame, (uint8_t) (value / 0x1000000));

    Frame_setNextByte(frame, (uint8_t) self->quality);

    return true;
}

struct sInformationObjectVFT bitString32VFT = {
        (EncodeFunction) BitString32_encode,
        (DestroyFunction) BitString32_destroy
};

static void
BitString32_initialize(BitString32 self)
{
    self->virtualFunctionTable = &(bitString32VFT);
    self->type = M_BO_NA_1;
}

void
BitString32_destroy(BitString32 self)
{
    GLOBAL_FREEMEM(self);
}

BitString32
BitString32_create(BitString32 self, int ioa, uint32_t value)
{
    if (self == NULL)
         self = (BitString32) GLOBAL_CALLOC(1, sizeof(struct sBitString32));

    if (self != NULL) {
        BitString32_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
    }

    return self;
}

uint32_t
BitString32_getValue(BitString32 self)
{
    return self->value;
}

QualityDescriptor
BitString32_getQuality(BitString32 self)
{
    return self->quality;
}

BitString32
BitString32_getFromBuffer(BitString32 self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (BitString32) GLOBAL_MALLOC(sizeof(struct sBitString32));

    if (self != NULL) {
        BitString32_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        uint32_t value;

        value = msg [startIndex++];
        value += ((uint32_t)msg [startIndex++] * 0x100);
        value += ((uint32_t)msg [startIndex++] * 0x10000);
        value += ((uint32_t)msg [startIndex++] * 0x1000000);
        self->value = value;

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];
    }

    return self;
}

/**********************************************
 * Bitstring32WithCP24Time2a  M_BO_TA_1=8 带时标CP24Time2a的32比特串
 **********************************************/

static bool
Bitstring32WithCP24Time2a_encode(Bitstring32WithCP24Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    int value = self->value;

    Frame_setNextByte(frame, (uint8_t) (value % 0x100));
    Frame_setNextByte(frame, (uint8_t) ((value / 0x100) % 0x100));
    Frame_setNextByte(frame, (uint8_t) ((value / 0x10000) % 0x100));
    Frame_setNextByte(frame, (uint8_t) (value / 0x1000000));

    Frame_setNextByte(frame, (uint8_t) self->quality);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT bitstring32WithCP24Time2aVFT = {
        (EncodeFunction) Bitstring32WithCP24Time2a_encode,
        (DestroyFunction) Bitstring32WithCP24Time2a_destroy
};

static void
Bitstring32WithCP24Time2a_initialize(Bitstring32WithCP24Time2a self)
{
    self->virtualFunctionTable = &(bitstring32WithCP24Time2aVFT);
    self->type = M_BO_TA_1;
}

void
Bitstring32WithCP24Time2a_destroy(Bitstring32WithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

Bitstring32WithCP24Time2a
Bitstring32WithCP24Time2a_create(Bitstring32WithCP24Time2a self, int ioa, uint32_t value, CP24Time2a timestamp)
{
    if (self == NULL)
         self = (Bitstring32WithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sBitstring32WithCP24Time2a));

    if (self != NULL) {
        Bitstring32WithCP24Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->timestamp = *timestamp;
    }

    return self;
}

CP24Time2a
Bitstring32WithCP24Time2a_getTimestamp(Bitstring32WithCP24Time2a self)
{
    return &(self->timestamp);
}

Bitstring32WithCP24Time2a
Bitstring32WithCP24Time2a_getFromBuffer(Bitstring32WithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (Bitstring32WithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sBitstring32WithCP24Time2a));

    if (self != NULL) {
        Bitstring32WithCP24Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        uint32_t value;

        value = msg [startIndex++];
        value += ((uint32_t)msg [startIndex++] * 0x100);
        value += ((uint32_t)msg [startIndex++] * 0x10000);
        value += ((uint32_t)msg [startIndex++] * 0x1000000);
        self->value = value;

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/**********************************************
 * Bitstring32WithCP56Time2a  M_BO_TB_1=33  带时标CP56Time2a的32比特串
 **********************************************/

static bool
Bitstring32WithCP56Time2a_encode(Bitstring32WithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 12 : (parameters->sizeOfIOA + 12);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    int value = self->value;

    Frame_setNextByte(frame, (uint8_t) (value % 0x100));
    Frame_setNextByte(frame, (uint8_t) ((value / 0x100) % 0x100));
    Frame_setNextByte(frame, (uint8_t) ((value / 0x10000) % 0x100));
    Frame_setNextByte(frame, (uint8_t) (value / 0x1000000));

    Frame_setNextByte(frame, (uint8_t) self->quality);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT bitstring32WithCP56Time2aVFT = {
        (EncodeFunction) Bitstring32WithCP56Time2a_encode,
        (DestroyFunction) Bitstring32WithCP56Time2a_destroy
};

static void
Bitstring32WithCP56Time2a_initialize(Bitstring32WithCP56Time2a self)
{
    self->virtualFunctionTable = &(bitstring32WithCP56Time2aVFT);
    self->type = M_BO_TB_1;
}

void
Bitstring32WithCP56Time2a_destroy(Bitstring32WithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

Bitstring32WithCP56Time2a
Bitstring32WithCP56Time2a_create(Bitstring32WithCP56Time2a self, int ioa, uint32_t value, CP56Time2a timestamp)
{
    if (self == NULL)
         self = (Bitstring32WithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sBitstring32WithCP56Time2a));

    if (self != NULL) {
        Bitstring32WithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->timestamp = *timestamp;
    }

    return self;
}


CP56Time2a
Bitstring32WithCP56Time2a_getTimestamp(Bitstring32WithCP56Time2a self)
{
    return &(self->timestamp);
}

Bitstring32WithCP56Time2a
Bitstring32WithCP56Time2a_getFromBuffer(Bitstring32WithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (Bitstring32WithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sBitstring32WithCP56Time2a));

    if (self != NULL) {
        Bitstring32WithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        uint32_t value;

        value = msg [startIndex++];
        value += ((uint32_t)msg [startIndex++] * 0x100);
        value += ((uint32_t)msg [startIndex++] * 0x10000);
        value += ((uint32_t)msg [startIndex++] * 0x1000000);
        self->value = value;

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/**********************************************
 * MeasuredValueNormalized  M_ME_NA_1=9  测量值，归一化值
 **********************************************/

static int
getScaledValue(uint8_t* encodedValue)
{
    int value;

    value = encodedValue[0];
    value += (encodedValue[1] * 0x100);

    if (value > 32767)
        value = value - 65536;

    return value;
}

static void
setScaledValue(uint8_t* encodedValue, int value)
{
    int valueToEncode;

    if (value < 0)
        valueToEncode = value + 65536;
    else
        valueToEncode = value;

    encodedValue[0] = (uint8_t) (valueToEncode % 256);
    encodedValue[1] = (uint8_t) (valueToEncode / 256);
}

static bool
MeasuredValueNormalized_encode(MeasuredValueNormalized self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 3 : (parameters->sizeOfIOA + 3);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->encodedValue[0]);
    Frame_setNextByte(frame, self->encodedValue[1]);

    Frame_setNextByte(frame, (uint8_t) self->quality);

    return true;
}

struct sInformationObjectVFT measuredValueNormalizedVFT = {
        (EncodeFunction) MeasuredValueNormalized_encode,
        (DestroyFunction) MeasuredValueNormalized_destroy
};

static void
MeasuredValueNormalized_initialize(MeasuredValueNormalized self)
{
    self->virtualFunctionTable = &(measuredValueNormalizedVFT);
    self->type = M_ME_NA_1;
}

void
MeasuredValueNormalized_destroy(MeasuredValueNormalized self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueNormalized
MeasuredValueNormalized_create(MeasuredValueNormalized self, int ioa, float value, QualityDescriptor quality)
{
    if (self == NULL)
        self = (MeasuredValueNormalized) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueNormalized));

    if (self != NULL) {
        MeasuredValueNormalized_initialize(self);

        self->objectAddress = ioa;

        MeasuredValueNormalized_setValue(self, value);

        self->quality = quality;
    }

    return self;
}

float
MeasuredValueNormalized_getValue(MeasuredValueNormalized self)
{
    float nv = (float) (getScaledValue(self->encodedValue)) / 32767.f;

    return nv;
}

void
MeasuredValueNormalized_setValue(MeasuredValueNormalized self, float value)
{
    if (value > 1.0f)
        value = 1.0f;
    else if (value < -1.0f)
        value = -1.0f;

    int scaledValue = (int)(value * 32767.f);

    setScaledValue(self->encodedValue, scaledValue);
}

QualityDescriptor
MeasuredValueNormalized_getQuality(MeasuredValueNormalized self)
{
    return self->quality;
}

MeasuredValueNormalized
MeasuredValueNormalized_getFromBuffer(MeasuredValueNormalized self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueNormalized) GLOBAL_MALLOC(sizeof(struct sMeasuredValueNormalized));

    if (self != NULL) {
        MeasuredValueNormalized_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        self->encodedValue[0] = msg [startIndex++];
        self->encodedValue[1] = msg [startIndex++];

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];
    }

    return self;
}

/**********************************************
 * ParameterNormalizedValue  P_ME_NA_1=110  测量值参数，归一化值
 **********************************************/

void
ParameterNormalizedValue_destroy(ParameterNormalizedValue self)
{
    GLOBAL_FREEMEM(self);
}

ParameterNormalizedValue
ParameterNormalizedValue_create(ParameterNormalizedValue self, int ioa, float value, QualifierOfParameterMV quality)
{
    ParameterNormalizedValue pvn =
            MeasuredValueNormalized_create(self, ioa, value, (QualityDescriptor) quality);

    pvn->type = P_ME_NA_1;

    return pvn;
}

float
ParameterNormalizedValue_getValue(ParameterNormalizedValue self)
{
    return MeasuredValueNormalized_getValue(self);
}

void
ParameterNormalizedValue_setValue(ParameterNormalizedValue self, float value)
{
    MeasuredValueNormalized_setValue(self, value);
}

QualifierOfParameterMV
ParameterNormalizedValue_getQPM(ParameterNormalizedValue self)
{
    return self->quality;
}

ParameterNormalizedValue
ParameterNormalizedValue_getFromBuffer(ParameterNormalizedValue self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    MeasuredValueNormalized pvn =
            MeasuredValueNormalized_getFromBuffer(self, parameters, msg, msgSize, startIndex, false);

    pvn->type = P_ME_NA_1;

    return (ParameterNormalizedValue) pvn;
}


/*************************************************************
 * MeasuredValueNormalizedWithoutQuality : InformationObject  M_ME_ND_1=21  不带品质描述的归一化值
 *************************************************************/

static bool
MeasuredValueNormalizedWithoutQuality_encode(MeasuredValueNormalizedWithoutQuality self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 2 : (parameters->sizeOfIOA + 2);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->encodedValue[0]);
    Frame_setNextByte(frame, self->encodedValue[1]);

    return true;
}

struct sInformationObjectVFT measuredValueNormalizedWithoutQualityVFT = {
        (EncodeFunction) MeasuredValueNormalizedWithoutQuality_encode,
        (DestroyFunction) MeasuredValueNormalizedWithoutQuality_destroy
};

static void
MeasuredValueNormalizedWithoutQuality_initialize(MeasuredValueNormalizedWithoutQuality self)
{
    self->virtualFunctionTable = &(measuredValueNormalizedWithoutQualityVFT);
    self->type = M_ME_ND_1;
}

void
MeasuredValueNormalizedWithoutQuality_destroy(MeasuredValueNormalizedWithoutQuality self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueNormalizedWithoutQuality
MeasuredValueNormalizedWithoutQuality_create(MeasuredValueNormalizedWithoutQuality self, int ioa, float value)
{
    if (self == NULL)
        self = (MeasuredValueNormalizedWithoutQuality) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueNormalizedWithoutQuality));

    if (self != NULL) {
        MeasuredValueNormalizedWithoutQuality_initialize(self);

        self->objectAddress = ioa;

        MeasuredValueNormalizedWithoutQuality_setValue(self, value);
    }

    return self;
}

float
MeasuredValueNormalizedWithoutQuality_getValue(MeasuredValueNormalizedWithoutQuality self)
{
    float nv = ((float) (getScaledValue(self->encodedValue) + 0.5) / 32767.5f);

    return nv;
}

void
MeasuredValueNormalizedWithoutQuality_setValue(MeasuredValueNormalizedWithoutQuality self, float value)
{
    if (value > 1.0f)
        value = 1.0f;
    else if (value < -1.0f)
        value = -1.0f;

    int scaledValue = (int) ((value * 32767.5f) - 0.5);

    setScaledValue(self->encodedValue, scaledValue);
}

MeasuredValueNormalizedWithoutQuality
MeasuredValueNormalizedWithoutQuality_getFromBuffer(MeasuredValueNormalizedWithoutQuality self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueNormalizedWithoutQuality) GLOBAL_MALLOC(sizeof(struct sMeasuredValueNormalizedWithoutQuality));

    if (self != NULL) {
        MeasuredValueNormalizedWithoutQuality_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        self->encodedValue[0] = msg [startIndex++];
        self->encodedValue[1] = msg [startIndex++];
    }

    return self;
}

/***********************************************************************
 * MeasuredValueNormalizedWithCP24Time2a : MeasuredValueNormalized  M_ME_TA_1=10  带时标CP24Time2a的归一化值
 ***********************************************************************/

static bool
MeasuredValueNormalizedWithCP24Time2a_encode(MeasuredValueNormalizedWithCP24Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 6 : (parameters->sizeOfIOA + 6);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    MeasuredValueNormalized_encode((MeasuredValueNormalized) self, frame, parameters, isSequence);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT measuredValueNormalizedWithCP24Time2aVFT = {
        (EncodeFunction) MeasuredValueNormalizedWithCP24Time2a_encode,
        (DestroyFunction) MeasuredValueNormalizedWithCP24Time2a_destroy
};

static void
MeasuredValueNormalizedWithCP24Time2a_initialize(MeasuredValueNormalizedWithCP24Time2a self)
{
    self->virtualFunctionTable = &(measuredValueNormalizedWithCP24Time2aVFT);
    self->type = M_ME_TA_1;
}

void
MeasuredValueNormalizedWithCP24Time2a_destroy(MeasuredValueNormalizedWithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueNormalizedWithCP24Time2a
MeasuredValueNormalizedWithCP24Time2a_create(MeasuredValueNormalizedWithCP24Time2a self, int ioa,
            float value, QualityDescriptor quality, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (MeasuredValueNormalizedWithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueNormalizedWithCP24Time2a));

    if (self != NULL) {
        MeasuredValueNormalizedWithCP24Time2a_initialize(self);

        self->objectAddress = ioa;

        MeasuredValueNormalized_setValue((MeasuredValueNormalized) self, value);

        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}


CP24Time2a
MeasuredValueNormalizedWithCP24Time2a_getTimestamp(MeasuredValueNormalizedWithCP24Time2a self)
{
    return &(self->timestamp);
}

void
MeasuredValueNormalizedWithCP24Time2a_setTimestamp(MeasuredValueNormalizedWithCP24Time2a self,
        CP24Time2a value)
{
    int i;
    for (i = 0; i < 3; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}

MeasuredValueNormalizedWithCP24Time2a
MeasuredValueNormalizedWithCP24Time2a_getFromBuffer(MeasuredValueNormalizedWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueNormalizedWithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sMeasuredValueNormalizedWithCP24Time2a));

    if (self != NULL) {
        MeasuredValueNormalizedWithCP24Time2a_initialize(self);

        if (!isSequence) {
             InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

             startIndex += parameters->sizeOfIOA; /* skip IOA */
         }

        self->encodedValue[0] = msg [startIndex++];
        self->encodedValue[1] = msg [startIndex++];

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/***********************************************************************
 * MeasuredValueNormalizedWithCP56Time2a : MeasuredValueNormalized  M_ME_TD_1  带时标CP56Time2a的归一化值
 ***********************************************************************/

static bool
MeasuredValueNormalizedWithCP56Time2a_encode(MeasuredValueNormalizedWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 10 : (parameters->sizeOfIOA + 10);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    MeasuredValueNormalized_encode((MeasuredValueNormalized) self, frame, parameters, isSequence);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT measuredValueNormalizedWithCP56Time2aVFT = {
        (EncodeFunction) MeasuredValueNormalizedWithCP56Time2a_encode,
        (DestroyFunction) MeasuredValueNormalizedWithCP56Time2a_destroy
};

static void
MeasuredValueNormalizedWithCP56Time2a_initialize(MeasuredValueNormalizedWithCP56Time2a self)
{
    self->virtualFunctionTable = &(measuredValueNormalizedWithCP56Time2aVFT);
    self->type = M_ME_TD_1;
}

void
MeasuredValueNormalizedWithCP56Time2a_destroy(MeasuredValueNormalizedWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueNormalizedWithCP56Time2a
MeasuredValueNormalizedWithCP56Time2a_create(MeasuredValueNormalizedWithCP56Time2a self, int ioa,
            float value, QualityDescriptor quality, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (MeasuredValueNormalizedWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueNormalizedWithCP56Time2a));

    if (self != NULL) {
        MeasuredValueNormalizedWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        MeasuredValueNormalized_setValue((MeasuredValueNormalized) self, value);

        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}


CP56Time2a
MeasuredValueNormalizedWithCP56Time2a_getTimestamp(MeasuredValueNormalizedWithCP56Time2a self)
{
    return &(self->timestamp);
}

void
MeasuredValueNormalizedWithCP56Time2a_setTimestamp(MeasuredValueNormalizedWithCP56Time2a self,
        CP56Time2a value)
{
    int i;
    for (i = 0; i < 7; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}

MeasuredValueNormalizedWithCP56Time2a
MeasuredValueNormalizedWithCP56Time2a_getFromBuffer(MeasuredValueNormalizedWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueNormalizedWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sMeasuredValueNormalizedWithCP56Time2a));

    if (self != NULL) {
        MeasuredValueNormalizedWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        self->encodedValue[0] = msg [startIndex++];
        self->encodedValue[1] = msg [startIndex++];

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}



/*******************************************
 * MeasuredValueScaled  M_ME_NB_1=11  测量值，标度化值
 *******************************************/

static bool
MeasuredValueScaled_encode(MeasuredValueScaled self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    return MeasuredValueNormalized_encode((MeasuredValueNormalized) self, frame, parameters, isSequence);
}

struct sInformationObjectVFT measuredValueScaledVFT = {
        (EncodeFunction) MeasuredValueScaled_encode,
        (DestroyFunction) MeasuredValueScaled_destroy
};

static void
MeasuredValueScaled_initialize(MeasuredValueScaled self)
{
    self->virtualFunctionTable = &(measuredValueScaledVFT);
    self->type = M_ME_NB_1;
}

MeasuredValueScaled
MeasuredValueScaled_create(MeasuredValueScaled self, int ioa, int value, QualityDescriptor quality)
{
    if (self == NULL)
        self = (MeasuredValueScaled) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueScaled));

    if (self != NULL) {
        MeasuredValueScaled_initialize(self);

        self->objectAddress = ioa;
        setScaledValue(self->encodedValue, value);
        self->quality = quality;
    }

    return self;
}


void
MeasuredValueScaled_destroy(MeasuredValueScaled self)
{
    GLOBAL_FREEMEM(self);
}


int
MeasuredValueScaled_getValue(MeasuredValueScaled self)
{
    return getScaledValue(self->encodedValue);
}

void
MeasuredValueScaled_setValue(MeasuredValueScaled self, int value)
{
    setScaledValue(self->encodedValue, value);
}

QualityDescriptor
MeasuredValueScaled_getQuality(MeasuredValueScaled self)
{
    return self->quality;
}

void
MeasuredValueScaled_setQuality(MeasuredValueScaled self, QualityDescriptor quality)
{
    self->quality = quality;
}

MeasuredValueScaled
MeasuredValueScaled_getFromBuffer(MeasuredValueScaled self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueScaled) GLOBAL_MALLOC(sizeof(struct sMeasuredValueScaled));

    if (self != NULL) {
        MeasuredValueScaled_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        self->encodedValue[0] = msg [startIndex++];
        self->encodedValue[1] = msg [startIndex++];

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];
    }

    return self;
}

/******************************************************
 * ParameterScaledValue : MeasuredValueScaled  P_ME_NB_1=111  测量值参数，标度化值
 *****************************************************/

void
ParameterScaledValue_destroy(ParameterScaledValue self)
{
    GLOBAL_FREEMEM(self);
}

ParameterScaledValue
ParameterScaledValue_create(ParameterScaledValue self, int ioa, int value, QualifierOfParameterMV qpm)
{
    ParameterScaledValue pvn =
            MeasuredValueScaled_create(self, ioa, value, qpm);

    pvn->type = P_ME_NB_1;

    return pvn;
}

int
ParameterScaledValue_getValue(ParameterScaledValue self)
{
    return getScaledValue(self->encodedValue);
}

void
ParameterScaledValue_setValue(ParameterScaledValue self, int value)
{
    setScaledValue(self->encodedValue, value);
}

QualifierOfParameterMV
ParameterScaledValue_getQPM(ParameterScaledValue self)
{
    return self->quality;
}

ParameterScaledValue
ParameterScaledValue_getFromBuffer(ParameterScaledValue self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    MeasuredValueScaled psv =
            MeasuredValueScaled_getFromBuffer(self, parameters, msg, msgSize, startIndex, false);

    psv->type = P_ME_NB_1;

    return (ParameterScaledValue) psv;
}


/*******************************************
 * MeasuredValueScaledWithCP24Time2a  M_ME_TB_1=12  带时标CP24Time2a的标度化值
 *******************************************/

static bool
MeasuredValueScaledWithCP24Time2a_encode(MeasuredValueScaledWithCP24Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 6 : (parameters->sizeOfIOA + 6);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    MeasuredValueNormalized_encode((MeasuredValueNormalized) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT measuredValueScaledWithCP24Time2aVFT = {
        (EncodeFunction) MeasuredValueScaledWithCP24Time2a_encode,
        (DestroyFunction) MeasuredValueScaled_destroy
};

static void
MeasuredValueScaledWithCP24Time2a_initialize(MeasuredValueScaledWithCP24Time2a self)
{
    self->virtualFunctionTable = &(measuredValueScaledWithCP24Time2aVFT);
    self->type = M_ME_TB_1;
}

void
MeasuredValueScaledWithCP24Time2a_destroy(MeasuredValueScaledWithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueScaledWithCP24Time2a
MeasuredValueScaledWithCP24Time2a_create(MeasuredValueScaledWithCP24Time2a self, int ioa,
        int value, QualityDescriptor quality, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (MeasuredValueScaledWithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueScaledWithCP24Time2a));

    if (self != NULL) {
        MeasuredValueScaledWithCP24Time2a_initialize(self);

        self->objectAddress = ioa;
        setScaledValue(self->encodedValue, value);
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}

CP24Time2a
MeasuredValueScaledWithCP24Time2a_getTimestamp(MeasuredValueScaledWithCP24Time2a self)
{
    return &(self->timestamp);
}

void
MeasuredValueScaledWithCP24Time2a_setTimestamp(MeasuredValueScaledWithCP24Time2a self,
        CP24Time2a value)
{
    int i;
    for (i = 0; i < 3; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}

MeasuredValueScaledWithCP24Time2a
MeasuredValueScaledWithCP24Time2a_getFromBuffer(MeasuredValueScaledWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueScaledWithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sMeasuredValueScaledWithCP24Time2a));

    if (self != NULL) {
        MeasuredValueScaledWithCP24Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        self->encodedValue[0] = msg [startIndex++];
        self->encodedValue[1] = msg [startIndex++];

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/*******************************************
 * MeasuredValueScaledWithCP56Time2a  M_ME_TE_1=35  带时标CP56Time2a的标度化值
 *******************************************/

static bool
MeasuredValueScaledWithCP56Time2a_encode(MeasuredValueScaledWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 10 : (parameters->sizeOfIOA + 10);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    MeasuredValueNormalized_encode((MeasuredValueNormalized) self, frame, parameters, isSequence);

    /* timestamp */
    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT measuredValueScaledWithCP56Time2aVFT = {
        (EncodeFunction) MeasuredValueScaledWithCP56Time2a_encode,
        (DestroyFunction) MeasuredValueScaledWithCP56Time2a_destroy
};

static void
MeasuredValueScaledWithCP56Time2a_initialize(MeasuredValueScaledWithCP56Time2a self)
{
    self->virtualFunctionTable = &measuredValueScaledWithCP56Time2aVFT;
    self->type = M_ME_TE_1;
}

void
MeasuredValueScaledWithCP56Time2a_destroy(MeasuredValueScaledWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueScaledWithCP56Time2a
MeasuredValueScaledWithCP56Time2a_create(MeasuredValueScaledWithCP56Time2a self, int ioa,
        int value, QualityDescriptor quality, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (MeasuredValueScaledWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueScaledWithCP56Time2a));

    if (self != NULL) {
        MeasuredValueScaledWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        setScaledValue(self->encodedValue, value);
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}

CP56Time2a
MeasuredValueScaledWithCP56Time2a_getTimestamp(MeasuredValueScaledWithCP56Time2a self)
{
    return &(self->timestamp);
}

void
MeasuredValueScaledWithCP56Time2a_setTimestamp(MeasuredValueScaledWithCP56Time2a self,
        CP56Time2a value)
{
    int i;
    for (i = 0; i < 7; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}


MeasuredValueScaledWithCP56Time2a
MeasuredValueScaledWithCP56Time2a_getFromBuffer(MeasuredValueScaledWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueScaledWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sMeasuredValueScaledWithCP56Time2a));

    if (self != NULL) {
        MeasuredValueScaledWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* scaled value */
        self->encodedValue[0] = msg [startIndex++];
        self->encodedValue[1] = msg [startIndex++];

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/*******************************************
 * MeasuredValueShort  M_ME_NC_1=13 测量值，短浮点数
 *******************************************/

static bool
MeasuredValueShort_encode(MeasuredValueShort self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 5 : (parameters->sizeOfIOA + 5);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
    Frame_appendBytes(frame, valueBytes, 4);
#else
    Frame_setNextByte(frame, valueBytes[3]);
    Frame_setNextByte(frame, valueBytes[2]);
    Frame_setNextByte(frame, valueBytes[1]);
    Frame_setNextByte(frame, valueBytes[0]);
#endif

    Frame_setNextByte(frame, (uint8_t) self->quality);

    return true;
}

struct sInformationObjectVFT measuredValueShortVFT = {
        (EncodeFunction) MeasuredValueShort_encode,
        (DestroyFunction) MeasuredValueShort_destroy
};

static void
MeasuredValueShort_initialize(MeasuredValueShort self)
{
    self->virtualFunctionTable = &(measuredValueShortVFT);
    self->type = M_ME_NC_1;
}

void
MeasuredValueShort_destroy(MeasuredValueShort self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueShort
MeasuredValueShort_create(MeasuredValueShort self, int ioa, float value, QualityDescriptor quality)
{
    if (self == NULL)
        self = (MeasuredValueShort) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueShort));

    if (self != NULL) {
        MeasuredValueShort_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
    }

    return self;
}

float
MeasuredValueShort_getValue(MeasuredValueShort self)
{
    return self->value;
}

void
MeasuredValueShort_setValue(MeasuredValueShort self, float value)
{
    self->value = value;
}

QualityDescriptor
MeasuredValueShort_getQuality(MeasuredValueShort self)
{
    return self->quality;
}

MeasuredValueShort
MeasuredValueShort_getFromBuffer(MeasuredValueShort self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueShort) GLOBAL_MALLOC(sizeof(struct sMeasuredValueShort));

    if (self != NULL) {
        MeasuredValueShort_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
        valueBytes[0] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[3] = msg [startIndex++];
#else
        valueBytes[3] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[0] = msg [startIndex++];
#endif

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];
    }

    return self;
}

/******************************************************
 * ParameterFloatValue : MeasuredValueShort  P_ME_NC_1=112  测量值参数，短浮点数
 *****************************************************/

void
ParameterFloatValue_destroy(ParameterFloatValue self)
{
    GLOBAL_FREEMEM(self);
}

ParameterFloatValue
ParameterFloatValue_create(ParameterFloatValue self, int ioa, float value, QualifierOfParameterMV qpm)
{
    ParameterFloatValue pvf =
            MeasuredValueShort_create(self, ioa, value, (QualityDescriptor) qpm);

    pvf->type = P_ME_NC_1;

    return pvf;
}

float
ParameterFloatValue_getValue(ParameterFloatValue self)
{
    return self->value;
}

void
ParameterFloatValue_setValue(ParameterFloatValue self, float value)
{
    self->value = value;
}

QualifierOfParameterMV
ParameterFloatValue_getQPM(ParameterFloatValue self)
{
    return self->quality;
}

ParameterFloatValue
ParameterFloatValue_getFromBuffer(ParameterFloatValue self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    ParameterFloatValue psv =
            MeasuredValueShort_getFromBuffer(self, parameters, msg, msgSize, startIndex, false);

    psv->type = P_ME_NC_1;

    return (ParameterFloatValue) psv;
}

/*******************************************
 * MeasuredValueFloatWithCP24Time2a  M_ME_TC_1=14  带时标CP24Time2a的短浮点数
 *******************************************/

static bool
MeasuredValueShortWithCP24Time2a_encode(MeasuredValueShortWithCP24Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    MeasuredValueShort_encode((MeasuredValueShort) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT measuredValueShortWithCP24Time2aVFT = {
        (EncodeFunction) MeasuredValueShortWithCP24Time2a_encode,
        (DestroyFunction) MeasuredValueShortWithCP24Time2a_destroy
};

static void
MeasuredValueShortWithCP24Time2a_initialize(MeasuredValueShortWithCP24Time2a self)
{
    self->virtualFunctionTable = &(measuredValueShortWithCP24Time2aVFT);
    self->type = M_ME_TC_1;
}

void
MeasuredValueShortWithCP24Time2a_destroy(MeasuredValueShortWithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueShortWithCP24Time2a
MeasuredValueShortWithCP24Time2a_create(MeasuredValueShortWithCP24Time2a self, int ioa,
        float value, QualityDescriptor quality, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (MeasuredValueShortWithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueShortWithCP24Time2a));

    if (self != NULL) {
        MeasuredValueShortWithCP24Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}

CP24Time2a
MeasuredValueShortWithCP24Time2a_getTimestamp(MeasuredValueShortWithCP24Time2a self)
{
    return &(self->timestamp);
}

void
MeasuredValueShortWithCP24Time2a_setTimestamp(MeasuredValueShortWithCP24Time2a self,
        CP24Time2a value)
{
    int i;
    for (i = 0; i < 3; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}

MeasuredValueShortWithCP24Time2a
MeasuredValueShortWithCP24Time2a_getFromBuffer(MeasuredValueShortWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueShortWithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sMeasuredValueShortWithCP24Time2a));

    if (self != NULL) {
        MeasuredValueShortWithCP24Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
        valueBytes[0] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[3] = msg [startIndex++];
#else
        valueBytes[3] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[0] = msg [startIndex++];
#endif

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/*******************************************
 * MeasuredValueFloatWithCP56Time2a  M_ME_TF_1=36  带时标CP56Time2a的短浮点数
 *******************************************/

static bool
MeasuredValueShortWithCP56Time2a_encode(MeasuredValueShortWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 12 : (parameters->sizeOfIOA + 12);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    MeasuredValueShort_encode((MeasuredValueShort) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT measuredValueShortWithCP56Time2aVFT = {
        (EncodeFunction) MeasuredValueShortWithCP56Time2a_encode,
        (DestroyFunction) MeasuredValueShortWithCP56Time2a_destroy
};

static void
MeasuredValueShortWithCP56Time2a_initialize(MeasuredValueShortWithCP56Time2a self)
{
    self->virtualFunctionTable = &(measuredValueShortWithCP56Time2aVFT);
    self->type = M_ME_TF_1;
}

void
MeasuredValueShortWithCP56Time2a_destroy(MeasuredValueShortWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

MeasuredValueShortWithCP56Time2a
MeasuredValueShortWithCP56Time2a_create(MeasuredValueShortWithCP56Time2a self, int ioa,
        float value, QualityDescriptor quality, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (MeasuredValueShortWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sMeasuredValueShortWithCP56Time2a));

    if (self != NULL) {
        MeasuredValueShortWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->value = value;
        self->quality = quality;
        self->timestamp = *timestamp;
    }

    return self;
}

CP56Time2a
MeasuredValueShortWithCP56Time2a_getTimestamp(MeasuredValueShortWithCP56Time2a self)
{
    return &(self->timestamp);
}

void
MeasuredValueShortWithCP56Time2a_setTimestamp(MeasuredValueShortWithCP56Time2a self,
        CP56Time2a value)
{
    int i;
    for (i = 0; i < 7; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}

MeasuredValueShortWithCP56Time2a
MeasuredValueShortWithCP56Time2a_getFromBuffer(MeasuredValueShortWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (MeasuredValueShortWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sMeasuredValueShortWithCP56Time2a));

    if (self != NULL) {
        MeasuredValueShortWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
        valueBytes[0] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[3] = msg [startIndex++];
#else
        valueBytes[3] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[0] = msg [startIndex++];
#endif

        /* quality */
        self->quality = (QualityDescriptor) msg [startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*******************************************
 * IntegratedTotals  M_IT_NA_1=15  累积量
 *******************************************/

static bool
IntegratedTotals_encode(IntegratedTotals self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 5 : (parameters->sizeOfIOA + 5);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->totals.encodedValue, 5);

    return true;
}

struct sInformationObjectVFT integratedTotalsVFT = {
        (EncodeFunction) IntegratedTotals_encode,
        (DestroyFunction) IntegratedTotals_destroy
};

static void
IntegratedTotals_initialize(IntegratedTotals self)
{
    self->virtualFunctionTable = &(integratedTotalsVFT);
    self->type = M_IT_NA_1;
}

void
IntegratedTotals_destroy(IntegratedTotals self)
{
    GLOBAL_FREEMEM(self);
}

IntegratedTotals
IntegratedTotals_create(IntegratedTotals self, int ioa, BinaryCounterReading value)
{
    if (self == NULL)
        self = (IntegratedTotals) GLOBAL_CALLOC(1, sizeof(struct sIntegratedTotals));

    if (self != NULL) {
        IntegratedTotals_initialize(self);

        self->objectAddress = ioa;
        self->totals = *value;
    }

    return self;
}

BinaryCounterReading
IntegratedTotals_getBCR(IntegratedTotals self)
{
    return &(self->totals);
}

void
IntegratedTotals_setBCR(IntegratedTotals self, BinaryCounterReading value)
{
    int i;

    for (i = 0; i < 5; i++)
        self->totals.encodedValue[i] = value->encodedValue[i];
}

IntegratedTotals
IntegratedTotals_getFromBuffer(IntegratedTotals self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (IntegratedTotals) GLOBAL_MALLOC(sizeof(struct sIntegratedTotals));

    if (self != NULL) {
        IntegratedTotals_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* BCR */
        int i = 0;

        for (i = 0; i < 5; i++)
            self->totals.encodedValue[i] = msg [startIndex++];
    }

    return self;
}

/***********************************************************************
 * IntegratedTotalsWithCP24Time2a : IntegratedTotals  M_IT_TA_1=16  带时标CP24Time2a的累积量
 ***********************************************************************/

static bool
IntegratedTotalsWithCP24Time2a_encode(IntegratedTotalsWithCP24Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    IntegratedTotals_encode((IntegratedTotals) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT integratedTotalsWithCP24Time2aVFT = {
        (EncodeFunction) IntegratedTotalsWithCP24Time2a_encode,
        (DestroyFunction) IntegratedTotalsWithCP24Time2a_destroy
};

static void
IntegratedTotalsWithCP24Time2a_initialize(IntegratedTotalsWithCP24Time2a self)
{
    self->virtualFunctionTable = &(integratedTotalsWithCP24Time2aVFT);
    self->type = M_IT_TA_1;
}

void
IntegratedTotalsWithCP24Time2a_destroy(IntegratedTotalsWithCP24Time2a self)
{
    GLOBAL_FREEMEM(self);
}

IntegratedTotalsWithCP24Time2a
IntegratedTotalsWithCP24Time2a_create(IntegratedTotalsWithCP24Time2a self, int ioa,
        BinaryCounterReading value, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (IntegratedTotalsWithCP24Time2a) GLOBAL_CALLOC(1, sizeof(struct sIntegratedTotalsWithCP24Time2a));

    if (self != NULL) {
        IntegratedTotalsWithCP24Time2a_initialize(self);

        self->objectAddress = ioa;
        self->totals = *value;
        self->timestamp = *timestamp;
    }

    return self;
}


CP24Time2a
IntegratedTotalsWithCP24Time2a_getTimestamp(IntegratedTotalsWithCP24Time2a self)
{
    return &(self->timestamp);
}

void
IntegratedTotalsWithCP24Time2a_setTimestamp(IntegratedTotalsWithCP24Time2a self,
        CP24Time2a value)
{
    int i;
    for (i = 0; i < 3; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}


IntegratedTotalsWithCP24Time2a
IntegratedTotalsWithCP24Time2a_getFromBuffer(IntegratedTotalsWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (IntegratedTotalsWithCP24Time2a) GLOBAL_MALLOC(sizeof(struct sIntegratedTotalsWithCP24Time2a));

    if (self != NULL) {
        IntegratedTotalsWithCP24Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* BCR */
        int i = 0;

        for (i = 0; i < 5; i++)
            self->totals.encodedValue[i] = msg [startIndex++];

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/***********************************************************************
 * IntegratedTotalsWithCP56Time2a : IntegratedTotals  M_IT_TB_1=37  M_IT_TA_1=16  带时标CP56Time2a的累积量
 ***********************************************************************/


static bool
IntegratedTotalsWithCP56Time2a_encode(IntegratedTotalsWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 12 : (parameters->sizeOfIOA + 12);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    IntegratedTotals_encode((IntegratedTotals) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT integratedTotalsWithCP56Time2aVFT = {
        (EncodeFunction) IntegratedTotalsWithCP56Time2a_encode,
        (DestroyFunction) IntegratedTotalsWithCP56Time2a_destroy
};

static void
IntegratedTotalsWithCP56Time2a_initialize(IntegratedTotalsWithCP56Time2a self)
{
    self->virtualFunctionTable = &(integratedTotalsWithCP56Time2aVFT);
    self->type = M_IT_TB_1;
}

void
IntegratedTotalsWithCP56Time2a_destroy(IntegratedTotalsWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

IntegratedTotalsWithCP56Time2a
IntegratedTotalsWithCP56Time2a_create(IntegratedTotalsWithCP56Time2a self, int ioa,
        BinaryCounterReading value, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (IntegratedTotalsWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sIntegratedTotalsWithCP56Time2a));

    if (self != NULL) {
        IntegratedTotalsWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->totals = *value;
        self->timestamp = *timestamp;
    }

    return self;
}

CP56Time2a
IntegratedTotalsWithCP56Time2a_getTimestamp(IntegratedTotalsWithCP56Time2a self)
{
    return &(self->timestamp);
}

void
IntegratedTotalsWithCP56Time2a_setTimestamp(IntegratedTotalsWithCP56Time2a self,
        CP56Time2a value)
{
    int i;
    for (i = 0; i < 7; i++) {
        self->timestamp.encodedValue[i] = value->encodedValue[i];
    }
}


IntegratedTotalsWithCP56Time2a
IntegratedTotalsWithCP56Time2a_getFromBuffer(IntegratedTotalsWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    //TODO check message size

    if (self == NULL)
        self = (IntegratedTotalsWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sIntegratedTotalsWithCP56Time2a));

    if (self != NULL) {
        IntegratedTotalsWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* BCR */
        int i = 0;

        for (i = 0; i < 5; i++)
            self->totals.encodedValue[i] = msg [startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/***********************************************************************
 * EventOfProtectionEquipment : InformationObject  M_EP_TA_1=17  继电保护装置事件
 ***********************************************************************/

static bool
EventOfProtectionEquipment_encode(EventOfProtectionEquipment self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 6 : (parameters->sizeOfIOA + 6);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, (uint8_t) self->event);

    Frame_appendBytes(frame, self->elapsedTime.encodedValue, 2);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT eventOfProtectionEquipmentVFT = {
        (EncodeFunction) EventOfProtectionEquipment_encode,
        (DestroyFunction) EventOfProtectionEquipment_destroy
};

static void
EventOfProtectionEquipment_initialize(EventOfProtectionEquipment self)
{
    self->virtualFunctionTable = &(eventOfProtectionEquipmentVFT);
    self->type = M_EP_TA_1;
}

void
EventOfProtectionEquipment_destroy(EventOfProtectionEquipment self)
{
    GLOBAL_FREEMEM(self);
}

EventOfProtectionEquipment
EventOfProtectionEquipment_create(EventOfProtectionEquipment self, int ioa,
        SingleEvent event, CP16Time2a elapsedTime, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (EventOfProtectionEquipment) GLOBAL_CALLOC(1, sizeof(struct sEventOfProtectionEquipment));

    if (self != NULL) {
        EventOfProtectionEquipment_initialize(self);

        self->objectAddress = ioa;
        self->event = *event;
        self->elapsedTime = *elapsedTime;
        self->timestamp = *timestamp;
    }

    return self;
}

EventOfProtectionEquipment
EventOfProtectionEquipment_getFromBuffer(EventOfProtectionEquipment self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 6))
        return NULL;

    if (self == NULL)
        self = (EventOfProtectionEquipment) GLOBAL_MALLOC(sizeof(struct sEventOfProtectionEquipment));

    if (self != NULL) {
        EventOfProtectionEquipment_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* event */
        self->event = msg[startIndex++];

        /* elapsed time */
        CP16Time2a_getFromBuffer(&(self->elapsedTime), msg, msgSize, startIndex);
        startIndex += 2;

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


SingleEvent
EventOfProtectionEquipment_getEvent(EventOfProtectionEquipment self)
{
    return &(self->event);
}

CP16Time2a
EventOfProtectionEquipment_getElapsedTime(EventOfProtectionEquipment self)
{
    return &(self->elapsedTime);
}

CP24Time2a
EventOfProtectionEquipment_getTimestamp(EventOfProtectionEquipment self)
{
    return &(self->timestamp);
}

/***********************************************************************
 * EventOfProtectionEquipmentWithCP56Time2a : InformationObject  M_EP_TD_1=38  带时标CP56Time2a的继电保护装置事件
 ***********************************************************************/

static bool
EventOfProtectionEquipmentWithCP56Time2a_encode(EventOfProtectionEquipmentWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 10 : (parameters->sizeOfIOA + 10);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, (uint8_t) self->event);

    Frame_appendBytes(frame, self->elapsedTime.encodedValue, 2);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT eventOfProtectionEquipmentWithCP56Time2aVFT = {
        (EncodeFunction) EventOfProtectionEquipmentWithCP56Time2a_encode,
        (DestroyFunction) EventOfProtectionEquipmentWithCP56Time2a_destroy
};

static void
EventOfProtectionEquipmentWithCP56Time2a_initialize(EventOfProtectionEquipmentWithCP56Time2a self)
{
    self->virtualFunctionTable = &(eventOfProtectionEquipmentWithCP56Time2aVFT);
    self->type = M_EP_TD_1;
}

void
EventOfProtectionEquipmentWithCP56Time2a_destroy(EventOfProtectionEquipmentWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

EventOfProtectionEquipmentWithCP56Time2a
EventOfProtectionEquipmentWithCP56Time2a_create(EventOfProtectionEquipmentWithCP56Time2a self, int ioa,
        SingleEvent event, CP16Time2a elapsedTime, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (EventOfProtectionEquipmentWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sEventOfProtectionEquipmentWithCP56Time2a));

    if (self != NULL) {
        EventOfProtectionEquipmentWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->event = *event;
        self->elapsedTime = *elapsedTime;
        self->timestamp = *timestamp;
    }

    return self;
}

SingleEvent
EventOfProtectionEquipmentWithCP56Time2a_getEvent(EventOfProtectionEquipmentWithCP56Time2a self)
{
    return &(self->event);
}

CP16Time2a
EventOfProtectionEquipmentWithCP56Time2a_getElapsedTime(EventOfProtectionEquipmentWithCP56Time2a self)
{
    return &(self->elapsedTime);
}

CP56Time2a
EventOfProtectionEquipmentWithCP56Time2a_getTimestamp(EventOfProtectionEquipmentWithCP56Time2a self)
{
    return &(self->timestamp);
}

EventOfProtectionEquipmentWithCP56Time2a
EventOfProtectionEquipmentWithCP56Time2a_getFromBuffer(EventOfProtectionEquipmentWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 6))
        return NULL;

    if (self == NULL)
        self = (EventOfProtectionEquipmentWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sEventOfProtectionEquipmentWithCP56Time2a));

    if (self != NULL) {
        EventOfProtectionEquipmentWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* event */
        self->event = msg[startIndex++];

        /* elapsed time */
        CP16Time2a_getFromBuffer(&(self->elapsedTime), msg, msgSize, startIndex);
        startIndex += 2;

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/***********************************************************************
 * PackedStartEventsOfProtectionEquipment : InformationObject  M_EP_TB_1=18  继电保护装置成组启动事件
 ***********************************************************************/

static bool
PackedStartEventsOfProtectionEquipment_encode(PackedStartEventsOfProtectionEquipment self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 7 : (parameters->sizeOfIOA + 7);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, (uint8_t) self->event);

    Frame_setNextByte(frame, (uint8_t) self->qdp);

    Frame_appendBytes(frame, self->elapsedTime.encodedValue, 2);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT packedStartEventsOfProtectionEquipmentVFT = {
        (EncodeFunction) PackedStartEventsOfProtectionEquipment_encode,
        (DestroyFunction) PackedStartEventsOfProtectionEquipment_destroy
};

static void
PackedStartEventsOfProtectionEquipment_initialize(PackedStartEventsOfProtectionEquipment self)
{
    self->virtualFunctionTable = &(packedStartEventsOfProtectionEquipmentVFT);
    self->type = M_EP_TB_1;
}

void
PackedStartEventsOfProtectionEquipment_destroy(PackedStartEventsOfProtectionEquipment self)
{
    GLOBAL_FREEMEM(self);
}

PackedStartEventsOfProtectionEquipment
PackedStartEventsOfProtectionEquipment_create(PackedStartEventsOfProtectionEquipment self, int ioa,
        StartEvent event, QualityDescriptorP qdp, CP16Time2a elapsedTime, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (PackedStartEventsOfProtectionEquipment) GLOBAL_CALLOC(1, sizeof(struct sPackedStartEventsOfProtectionEquipment));

    if (self != NULL) {
        PackedStartEventsOfProtectionEquipment_initialize(self);

        self->objectAddress = ioa;
        self->event = event;
        self->qdp = qdp;
        self->elapsedTime = *elapsedTime;
        self->timestamp = *timestamp;
    }

    return self;
}

StartEvent
PackedStartEventsOfProtectionEquipment_getEvent(PackedStartEventsOfProtectionEquipment self)
{
    return self->event;
}

QualityDescriptorP
PackedStartEventsOfProtectionEquipment_getQuality(PackedStartEventsOfProtectionEquipment self)
{
    return self->qdp;
}

CP16Time2a
PackedStartEventsOfProtectionEquipment_getElapsedTime(PackedStartEventsOfProtectionEquipment self)
{
    return &(self->elapsedTime);
}

CP24Time2a
PackedStartEventsOfProtectionEquipment_getTimestamp(PackedStartEventsOfProtectionEquipment self)
{
    return &(self->timestamp);
}

PackedStartEventsOfProtectionEquipment
PackedStartEventsOfProtectionEquipment_getFromBuffer(PackedStartEventsOfProtectionEquipment self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 7))
        return NULL;

    if (self == NULL)
        self = (PackedStartEventsOfProtectionEquipment) GLOBAL_MALLOC(sizeof(struct sPackedStartEventsOfProtectionEquipment));

    if (self != NULL) {
        PackedStartEventsOfProtectionEquipment_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* event */
        self->event = msg[startIndex++];

        /* qdp */
        self->qdp = msg[startIndex++];

        /* elapsed time */
        CP16Time2a_getFromBuffer(&(self->elapsedTime), msg, msgSize, startIndex);
        startIndex += 2;

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/***************************************************************************
 * PackedStartEventsOfProtectionEquipmentWithCP56Time2a : InformationObject  M_EP_TE_1=39  带时标CP56Time2a的继电保护装置成组启动事件
 ***************************************************************************/

static bool
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_encode(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 11 : (parameters->sizeOfIOA + 11);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, (uint8_t) self->event);

    Frame_setNextByte(frame, (uint8_t) self->qdp);

    Frame_appendBytes(frame, self->elapsedTime.encodedValue, 2);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT packedStartEventsOfProtectionEquipmentWithCP56Time2aVFT = {
        (EncodeFunction) PackedStartEventsOfProtectionEquipmentWithCP56Time2a_encode,
        (DestroyFunction) PackedStartEventsOfProtectionEquipmentWithCP56Time2a_destroy
};

static void
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_initialize(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self)
{
    self->virtualFunctionTable = &(packedStartEventsOfProtectionEquipmentWithCP56Time2aVFT);
    self->type = M_EP_TE_1;
}

void
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_destroy(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

PackedStartEventsOfProtectionEquipmentWithCP56Time2a
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_create(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self, int ioa,
        StartEvent event, QualityDescriptorP qdp, CP16Time2a elapsedTime, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (PackedStartEventsOfProtectionEquipmentWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sPackedStartEventsOfProtectionEquipmentWithCP56Time2a));

    if (self != NULL) {
        PackedStartEventsOfProtectionEquipmentWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->event = event;
        self->qdp = qdp;
        self->elapsedTime = *elapsedTime;
        self->timestamp = *timestamp;
    }

    return self;
}

StartEvent
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getEvent(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self)
{
    return self->event;
}

QualityDescriptorP
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getQuality(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self)
{
    return self->qdp;
}

CP16Time2a
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getElapsedTime(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self)
{
    return &(self->elapsedTime);
}

CP56Time2a
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getTimestamp(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self)
{
    return &(self->timestamp);
}

PackedStartEventsOfProtectionEquipmentWithCP56Time2a
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getFromBuffer(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 7))
        return NULL;

    if (self == NULL)
        self = (PackedStartEventsOfProtectionEquipmentWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sPackedStartEventsOfProtectionEquipmentWithCP56Time2a));

    if (self != NULL) {
        PackedStartEventsOfProtectionEquipmentWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* event */
        self->event = msg[startIndex++];

        /* qdp */
        self->qdp = msg[startIndex++];

        /* elapsed time */
        CP16Time2a_getFromBuffer(&(self->elapsedTime), msg, msgSize, startIndex);
        startIndex += 2;

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/***********************************************************************
 * PacketOutputCircuitInfo : InformationObject  M_EP_TC_1=19  继电保护装置成组输出电路信息
 ***********************************************************************/

static bool
PacketOutputCircuitInfo_encode(PackedOutputCircuitInfo self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 7 : (parameters->sizeOfIOA + 7);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, (uint8_t) self->oci);

    Frame_setNextByte(frame, (uint8_t) self->qdp);

    Frame_appendBytes(frame, self->operatingTime.encodedValue, 2);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 3);

    return true;
}

struct sInformationObjectVFT packedOutputCircuitInfoVFT = {
        (EncodeFunction) PacketOutputCircuitInfo_encode,
        (DestroyFunction) PackedOutputCircuitInfo_destroy
};

static void
PacketOutputCircuitInfo_initialize(PackedOutputCircuitInfo self)
{
    self->virtualFunctionTable = &(packedOutputCircuitInfoVFT);
    self->type = M_EP_TC_1;
}

void
PackedOutputCircuitInfo_destroy(PackedOutputCircuitInfo self)
{
    GLOBAL_FREEMEM(self);
}

PackedOutputCircuitInfo
PackedOutputCircuitInfo_create(PackedOutputCircuitInfo self, int ioa,
        OutputCircuitInfo oci, QualityDescriptorP qdp, CP16Time2a operatingTime, CP24Time2a timestamp)
{
    if (self == NULL)
        self = (PackedOutputCircuitInfo) GLOBAL_CALLOC(1, sizeof(struct sPackedOutputCircuitInfo));

    if (self) {
        PacketOutputCircuitInfo_initialize(self);

        self->objectAddress = ioa;
        self->oci = oci;
        self->qdp = qdp;
        self->operatingTime = *operatingTime;
        self->timestamp = *timestamp;
    }

    return self;
}

OutputCircuitInfo
PackedOutputCircuitInfo_getOCI(PackedOutputCircuitInfo self)
{
    return self->oci;
}

QualityDescriptorP
PackedOutputCircuitInfo_getQuality(PackedOutputCircuitInfo self)
{
    return self->qdp;
}

CP16Time2a
PackedOutputCircuitInfo_getOperatingTime(PackedOutputCircuitInfo self)
{
    return &(self->operatingTime);
}

CP24Time2a
PackedOutputCircuitInfo_getTimestamp(PackedOutputCircuitInfo self)
{
    return &(self->timestamp);
}

PackedOutputCircuitInfo
PackedOutputCircuitInfo_getFromBuffer(PackedOutputCircuitInfo self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 7))
        return NULL;

    if (self == NULL)
        self = (PackedOutputCircuitInfo) GLOBAL_MALLOC(sizeof(struct sPackedOutputCircuitInfo));

    if (self != NULL) {
        PacketOutputCircuitInfo_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* OCI - output circuit information */
        self->oci = msg[startIndex++];

        /* qdp */
        self->qdp = msg[startIndex++];

        /* operating time */
        CP16Time2a_getFromBuffer(&(self->operatingTime), msg, msgSize, startIndex);
        startIndex += 2;

        /* timestamp */
        CP24Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/***********************************************************************
 * PackedOutputCircuitInfoWithCP56Time2a : InformationObject  M_EP_TF_1=40  带时标CP56Time2a的继电保护装置成组输出电路信息
 ***********************************************************************/

static bool
PackedOutputCircuitInfoWithCP56Time2a_encode(PackedOutputCircuitInfoWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 11 : (parameters->sizeOfIOA + 11);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, (uint8_t) self->oci);

    Frame_setNextByte(frame, (uint8_t) self->qdp);

    Frame_appendBytes(frame, self->operatingTime.encodedValue, 2);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT packedOutputCircuitInfoWithCP56Time2aVFT = {
        (EncodeFunction) PackedOutputCircuitInfoWithCP56Time2a_encode,
        (DestroyFunction) PackedOutputCircuitInfoWithCP56Time2a_destroy
};

static void
PackedOutputCircuitInfoWithCP56Time2a_initialize(PackedOutputCircuitInfoWithCP56Time2a self)
{
    self->virtualFunctionTable = &(packedOutputCircuitInfoWithCP56Time2aVFT);
    self->type = M_EP_TF_1;
}

void
PackedOutputCircuitInfoWithCP56Time2a_destroy(PackedOutputCircuitInfoWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

PackedOutputCircuitInfoWithCP56Time2a
PackedOutputCircuitInfoWithCP56Time2a_create(PackedOutputCircuitInfoWithCP56Time2a self, int ioa,
        OutputCircuitInfo oci, QualityDescriptorP qdp, CP16Time2a operatingTime, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (PackedOutputCircuitInfoWithCP56Time2a) GLOBAL_CALLOC(1, sizeof(struct sPackedOutputCircuitInfoWithCP56Time2a));

    if (self) {
        PackedOutputCircuitInfoWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;
        self->oci = oci;
        self->qdp = qdp;
        self->operatingTime = *operatingTime;
        self->timestamp = *timestamp;
    }

    return self;
}

OutputCircuitInfo
PackedOutputCircuitInfoWithCP56Time2a_getOCI(PackedOutputCircuitInfoWithCP56Time2a self)
{
    return self->oci;
}

QualityDescriptorP
PackedOutputCircuitInfoWithCP56Time2a_getQuality(PackedOutputCircuitInfoWithCP56Time2a self)
{
    return self->qdp;
}

CP16Time2a
PackedOutputCircuitInfoWithCP56Time2a_getOperatingTime(PackedOutputCircuitInfoWithCP56Time2a self)
{
    return &(self->operatingTime);
}

CP56Time2a
PackedOutputCircuitInfoWithCP56Time2a_getTimestamp(PackedOutputCircuitInfoWithCP56Time2a self)
{
    return &(self->timestamp);
}

PackedOutputCircuitInfoWithCP56Time2a
PackedOutputCircuitInfoWithCP56Time2a_getFromBuffer(PackedOutputCircuitInfoWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 7))
        return NULL;

    if (self == NULL)
        self = (PackedOutputCircuitInfoWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sPackedOutputCircuitInfoWithCP56Time2a));

    if (self != NULL) {
        PackedOutputCircuitInfoWithCP56Time2a_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* OCI - output circuit information */
        self->oci = msg[startIndex++];

        /* qdp */
        self->qdp = msg[startIndex++];

        /* operating time */
        CP16Time2a_getFromBuffer(&(self->operatingTime), msg, msgSize, startIndex);
        startIndex += 2;

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/***********************************************************************
 * PackedSinglePointWithSCD : InformationObject  M_PS_NA_1=20  带状态输出的成组单点信息
 ***********************************************************************/

static bool
PackedSinglePointWithSCD_encode(PackedSinglePointWithSCD self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 5 : (parameters->sizeOfIOA + 5);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->scd.encodedValue, 4);

    Frame_setNextByte(frame, (uint8_t) self->qds);

    return true;
}

struct sInformationObjectVFT packedSinglePointWithSCDVFT = {
        (EncodeFunction) PackedSinglePointWithSCD_encode,
        (DestroyFunction) PackedSinglePointWithSCD_destroy
};

static void
PackedSinglePointWithSCD_initialize(PackedSinglePointWithSCD self)
{
    self->virtualFunctionTable = &(packedSinglePointWithSCDVFT);
    self->type = M_PS_NA_1;
}

void
PackedSinglePointWithSCD_destroy(PackedSinglePointWithSCD self)
{
    GLOBAL_FREEMEM(self);
}

PackedSinglePointWithSCD
PackedSinglePointWithSCD_create(PackedSinglePointWithSCD self, int ioa,
        StatusAndStatusChangeDetection scd, QualityDescriptor qds)
{
    if (self == NULL)
        self = (PackedSinglePointWithSCD) GLOBAL_CALLOC(1, sizeof(struct sPackedSinglePointWithSCD));

    if (self) {
        PackedSinglePointWithSCD_initialize(self);

        self->objectAddress = ioa;
        self->scd = *scd;
        self->qds = qds;
    }

    return self;
}

QualityDescriptor
PackedSinglePointWithSCD_getQuality(PackedSinglePointWithSCD self)
{
    return self->qds;
}

StatusAndStatusChangeDetection
PackedSinglePointWithSCD_getSCD(PackedSinglePointWithSCD self)
{
    return &(self->scd);
}

PackedSinglePointWithSCD
PackedSinglePointWithSCD_getFromBuffer(PackedSinglePointWithSCD self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 5))
        return NULL;

    if (self == NULL)
        self = (PackedSinglePointWithSCD) GLOBAL_MALLOC(sizeof(struct sPackedSinglePointWithSCD));

    if (self != NULL) {
        PackedSinglePointWithSCD_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        /* SCD */
        self->scd.encodedValue[0] = msg[startIndex++];
        self->scd.encodedValue[1] = msg[startIndex++];
        self->scd.encodedValue[2] = msg[startIndex++];
        self->scd.encodedValue[3] = msg[startIndex++];

        /* QDS */
        self->qds = msg[startIndex];
    }

    return self;
}


/*******************************************
 * SingleCommand  C_SC_NA_1=45  单命令
 *******************************************/

static bool
SingleCommand_encode(SingleCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->sco);

    return true;
}

struct sInformationObjectVFT singleCommandVFT = {
        (EncodeFunction) SingleCommand_encode,
        (DestroyFunction) SingleCommand_destroy
};

static void
SingleCommand_initialize(SingleCommand self)
{
    self->virtualFunctionTable = &(singleCommandVFT);
    self->type = C_SC_NA_1;
}

void
SingleCommand_destroy(SingleCommand self)
{
    GLOBAL_FREEMEM(self);
}

SingleCommand
SingleCommand_create(SingleCommand self, int ioa, bool command, bool selectCommand, int qu)
{
    if (self == NULL)
        self = (SingleCommand) GLOBAL_MALLOC(sizeof(struct sSingleCommand));

    if (self) {
        SingleCommand_initialize(self);

        self->objectAddress = ioa;

        uint8_t sco = ((qu & 0x1f) * 4);

        if (command) sco |= 0x01;

        if (selectCommand) sco |= 0x80;

        self->sco = sco;
    }

    return self;
}

int
SingleCommand_getQU(SingleCommand self)
{
    return ((self->sco & 0x7c) / 4);
}

bool
SingleCommand_getState(SingleCommand self)
{
    return ((self->sco & 0x01) == 0x01);
}

bool
SingleCommand_isSelect(SingleCommand self)
{
    return ((self->sco & 0x80) == 0x80);
}

SingleCommand
SingleCommand_getFromBuffer(SingleCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 1))
        return NULL;

    if (self == NULL)
        self = (SingleCommand) GLOBAL_MALLOC(sizeof(struct sSingleCommand));

    if (self != NULL) {
        SingleCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* SCO */
        self->sco = msg[startIndex];
    }

    return self;
}


/***********************************************************************
 * SingleCommandWithCP56Time2a : SingleCommand  C_SC_TA_1=58  带时标CP56Time2a的单命令
 ***********************************************************************/

static bool
SingleCommandWithCP56Time2a_encode(SingleCommandWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    SingleCommand_encode((SingleCommand) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT singleCommandWithCP56Time2aVFT = {
        (EncodeFunction) SingleCommandWithCP56Time2a_encode,
        (DestroyFunction) SingleCommandWithCP56Time2a_destroy
};

static void
SingleCommandWithCP56Time2a_initialize(SingleCommandWithCP56Time2a self)
{
    self->virtualFunctionTable = &(singleCommandWithCP56Time2aVFT);
    self->type = C_SC_TA_1;
}

void
SingleCommandWithCP56Time2a_destroy(SingleCommandWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

SingleCommandWithCP56Time2a
SingleCommandWithCP56Time2a_create(SingleCommandWithCP56Time2a self, int ioa, bool command, bool selectCommand, int qu, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (SingleCommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSingleCommandWithCP56Time2a));

    if (self) {
        SingleCommandWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        uint8_t sco = ((qu & 0x1f) * 4);

        if (command) sco |= 0x01;

        if (selectCommand) sco |= 0x80;

        self->sco = sco;

        self->timestamp = *timestamp;
    }

    return self;
}

CP56Time2a
SingleCommandWithCP56Time2a_getTimestamp(SingleCommandWithCP56Time2a self)
{
    return &(self->timestamp);
}

SingleCommandWithCP56Time2a
SingleCommandWithCP56Time2a_getFromBuffer(SingleCommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 1))
        return NULL;

    if (self == NULL)
        self = (SingleCommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSingleCommandWithCP56Time2a));

    if (self != NULL) {
        SingleCommandWithCP56Time2a_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* SCO */
        self->sco = msg[startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*******************************************
 * DoubleCommand : InformationObject  C_DC_NA_1=46  双命令
 *******************************************/

static bool
DoubleCommand_encode(DoubleCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->dcq);

    return true;
}

struct sInformationObjectVFT doubleCommandVFT = {
        (EncodeFunction) DoubleCommand_encode,
        (DestroyFunction) DoubleCommand_destroy
};

static void
DoubleCommand_initialize(DoubleCommand self)
{
    self->virtualFunctionTable = &(doubleCommandVFT);
    self->type = C_DC_NA_1;
}

void
DoubleCommand_destroy(DoubleCommand self)
{
    GLOBAL_FREEMEM(self);
}

DoubleCommand
DoubleCommand_create(DoubleCommand self, int ioa, int command, bool selectCommand, int qu)
{
    if (self == NULL)
        self = (DoubleCommand) GLOBAL_MALLOC(sizeof(struct sDoubleCommand));

    if (self) {
        DoubleCommand_initialize(self);

        self->objectAddress = ioa;

        uint8_t dcq = ((qu & 0x1f) * 4);

        dcq += (uint8_t) (command & 0x03);

        if (selectCommand) dcq |= 0x80;

        self->dcq = dcq;
    }

    return self;
}

int
DoubleCommand_getQU(DoubleCommand self)
{
    return ((self->dcq & 0x7c) / 4);
}

int
DoubleCommand_getState(DoubleCommand self)
{
    return (self->dcq & 0x03);
}

bool
DoubleCommand_isSelect(DoubleCommand self)
{
    return ((self->dcq & 0x80) == 0x80);
}

DoubleCommand
DoubleCommand_getFromBuffer(DoubleCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 1))
        return NULL;

    if (self == NULL)
        self = (DoubleCommand) GLOBAL_MALLOC(sizeof(struct sDoubleCommand));

    if (self != NULL) {
        DoubleCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* SCO */
        self->dcq = msg[startIndex];
    }

    return self;
}

//CommandParamSet  开关参数设置（遥控扩展）
/*******************************************
 * CommandParamSet : InformationObject
 *******************************************/
//void
//CommandParamSet_destroy(CommandParamSet self);
//DoubleCommand
//CommandParamSet_create(CommandParamSet self, int ioa, int param1, int param2, int param3);
//int
//CommandParamSet_getParam1(CommandParamSet self);
//int
//CommandParamSet_getParam2(CommandParamSet self);
//int
//CommandParamSet_getParam3(CommandParamSet self);

static bool
CommandParamSet_encode(CommandParamSet self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, (self->param4)&0xff);
    Frame_setNextByte(frame, (self->param1)&0xff);
    Frame_setNextByte(frame, 0x00);
    Frame_setNextByte(frame, (self->param2)&0xff);
    Frame_setNextByte(frame, (self->param3)&0xff);
    Frame_setNextByte(frame, ((self->param3)&0xff00)>>8);

    return true;
}

struct sInformationObjectVFT commandParamSetVFT = {
        (EncodeFunction) CommandParamSet_encode,
        (DestroyFunction) CommandParamSet_destroy
};

static void
CommandParamSet_initialize(CommandParamSet self)
{
    self->virtualFunctionTable = &(commandParamSetVFT);
    self->type = C_SC_NA_1;//默认单点  45
}

void
CommandParamSet_destroy(CommandParamSet self)
{
    GLOBAL_FREEMEM(self);
}

CommandParamSet
CommandParamSet_create(CommandParamSet self, int ioa, int param1, int param2, int param3 ,int param4)
{
    if (self == NULL)
        self = (CommandParamSet) GLOBAL_MALLOC(sizeof(struct sCommandParamSet));

    if (self) {
        CommandParamSet_initialize(self);

        self->objectAddress = ioa;

        self->param1 = param1;
        self->param2 = param2;
        self->param3 = param3;
        self->param4 = param4;
    }

    return self;
}

int
CommandParamSet_getParam1(CommandParamSet self)
{
    return self->param1;
}

int
CommandParamSet_getParam2(CommandParamSet self)
{
   return self->param2;
}

int
CommandParamSet_getParam3(CommandParamSet self)
{
    return self->param3;
}

CommandParamSet
CommandParamSet_getFromBuffer(CommandParamSet self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 1))
        return NULL;

    if (self == NULL)
        self = (CommandParamSet) GLOBAL_MALLOC(sizeof(struct sCommandParamSet));

    if (self != NULL) {
        CommandParamSet_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* param */
        self->param4 = msg[startIndex];
        startIndex++;

        self->param1 = msg[startIndex];
        startIndex++;
        self->param2 = msg[startIndex];
        startIndex++;
        self->param3 = msg[startIndex];
        startIndex++;
        self->param3 += msg[startIndex]*256;
    }

    return self;
}

/**********************************************
 * DoubleCommandWithCP56Time2a : DoubleCommand  C_DC_TA_1=59  带时标CP56Time2a的双命令
 **********************************************/


static bool
DoubleCommandWithCP56Time2a_encode(DoubleCommandWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    DoubleCommand_encode((DoubleCommand) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT doubleCommandWithCP56Time2aVFT = {
        (EncodeFunction) DoubleCommandWithCP56Time2a_encode,
        (DestroyFunction) DoubleCommandWithCP56Time2a_destroy
};

static void
DoubleCommandWithCP56Time2a_initialize(DoubleCommandWithCP56Time2a self)
{
    self->virtualFunctionTable = &(doubleCommandWithCP56Time2aVFT);
    self->type = C_DC_TA_1;
}

void
DoubleCommandWithCP56Time2a_destroy(DoubleCommandWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

DoubleCommandWithCP56Time2a
DoubleCommandWithCP56Time2a_create(DoubleCommandWithCP56Time2a self, int ioa, int command, bool selectCommand, int qu, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (DoubleCommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sDoubleCommandWithCP56Time2a));

    if (self) {
        DoubleCommandWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        uint8_t dcq = ((qu & 0x1f) * 4);

        dcq += (uint8_t) (command & 0x03);

        if (selectCommand) dcq |= 0x80;

        self->dcq = dcq;

        self->timestamp = *timestamp;
    }

    return self;
}

int
DoubleCommandWithCP56Time2a_getQU(DoubleCommandWithCP56Time2a self)
{
    return DoubleCommand_getQU((DoubleCommand) self);
}

int
DoubleCommandWithCP56Time2a_getState(DoubleCommandWithCP56Time2a self)
{
    return DoubleCommand_getState((DoubleCommand) self);
}

bool
DoubleCommandWithCP56Time2a_isSelect(DoubleCommandWithCP56Time2a self)
{
    return DoubleCommand_isSelect((DoubleCommand) self);
}

DoubleCommandWithCP56Time2a
DoubleCommandWithCP56Time2a_getFromBuffer(DoubleCommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 1))
        return NULL;

    if (self == NULL)
        self = (DoubleCommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sDoubleCommandWithCP56Time2a));

    if (self != NULL) {
        DoubleCommandWithCP56Time2a_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* DCQ */
        self->dcq = msg[startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/*******************************************
 * StepCommand : InformationObject  C_RC_NA_1=47  步调节命令
 *******************************************/

static bool
StepCommand_encode(StepCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->dcq);

    return true;
}

struct sInformationObjectVFT stepCommandVFT = {
        (EncodeFunction) StepCommand_encode,
        (DestroyFunction) StepCommand_destroy
};

static void
StepCommand_initialize(StepCommand self)
{
    self->virtualFunctionTable = &(stepCommandVFT);
    self->type = C_RC_NA_1;
}

void
StepCommand_destroy(StepCommand self)
{
    GLOBAL_FREEMEM(self);
}

StepCommand
StepCommand_create(StepCommand self, int ioa, StepCommandValue command, bool selectCommand, int qu)
{
    if (self == NULL) {
        self = (StepCommand) GLOBAL_MALLOC(sizeof(struct sStepCommand));

        if (self == NULL)
            return NULL;
        else
            StepCommand_initialize(self);
    }

    self->objectAddress = ioa;

    uint8_t dcq = ((qu & 0x1f) * 4);

    dcq += (uint8_t) (command & 0x03);

    if (selectCommand) dcq |= 0x80;

    self->dcq = dcq;

    return self;
}

int
StepCommand_getQU(StepCommand self)
{
    return ((self->dcq & 0x7c) / 4);
}

StepCommandValue
StepCommand_getState(StepCommand self)
{
    return (StepCommandValue) (self->dcq & 0x03);
}

bool
StepCommand_isSelect(StepCommand self)
{
    return ((self->dcq & 0x80) == 0x80);
}

StepCommand
StepCommand_getFromBuffer(StepCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 1))
        return NULL;

    if (self == NULL)
        self = (StepCommand) GLOBAL_MALLOC(sizeof(struct sStepCommand));

    if (self != NULL) {
        StepCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* SCO */
        self->dcq = msg[startIndex];
    }

    return self;
}


/*************************************************
 * StepCommandWithCP56Time2a : InformationObject  C_RC_TA_1=60  带时标CP56Time2a的步调节命令
 *************************************************/

static bool
StepCommandWithCP56Time2a_encode(StepCommandWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 8 : (parameters->sizeOfIOA + 8);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    StepCommand_encode((StepCommand) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT stepCommandWithCP56Time2aVFT = {
        (EncodeFunction) StepCommandWithCP56Time2a_encode,
        (DestroyFunction) StepCommandWithCP56Time2a_destroy
};

static void
StepCommandWithCP56Time2a_initialize(StepCommandWithCP56Time2a self)
{
    self->virtualFunctionTable = &(stepCommandWithCP56Time2aVFT);
    self->type = C_RC_TA_1;
}

void
StepCommandWithCP56Time2a_destroy(StepCommand self)
{
    GLOBAL_FREEMEM(self);
}

StepCommandWithCP56Time2a
StepCommandWithCP56Time2a_create(StepCommandWithCP56Time2a self, int ioa, StepCommandValue command, bool selectCommand, int qu, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (StepCommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sStepCommandWithCP56Time2a));

    if (self) {
        StepCommandWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        uint8_t dcq = ((qu & 0x1f) * 4);

        dcq += (uint8_t) (command & 0x03);

        if (selectCommand) dcq |= 0x80;

        self->dcq = dcq;

        self->timestamp = *timestamp;
    }

    return self;
}

int
StepCommandWithCP56Time2a_getQU(StepCommandWithCP56Time2a self)
{
    return StepCommand_getQU((StepCommand) self);
}

StepCommandValue
StepCommandWithCP56Time2a_getState(StepCommandWithCP56Time2a self)
{
    return StepCommand_getState((StepCommand) self);
}

bool
StepCommandWithCP56Time2a_isSelect(StepCommandWithCP56Time2a self)
{
    return StepCommand_isSelect((StepCommand) self);
}

StepCommandWithCP56Time2a
StepCommandWithCP56Time2a_getFromBuffer(StepCommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 8))
        return NULL;

    if (self == NULL)
        self = (StepCommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sStepCommandWithCP56Time2a));

    if (self != NULL) {
        StepCommandWithCP56Time2a_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* SCO */
        self->dcq = msg[startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*************************************************
 * SetpointCommandNormalized : InformationObject  C_SE_NA_1=48  设点命令，归一化值
 ************************************************/

static bool
SetpointCommandNormalized_encode(SetpointCommandNormalized self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 3 : (parameters->sizeOfIOA + 3);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->encodedValue, 2);
    Frame_setNextByte(frame, self->qos);

    return true;
}

struct sInformationObjectVFT setpointCommandNormalizedVFT = {
        (EncodeFunction) SetpointCommandNormalized_encode,
        (DestroyFunction) SetpointCommandNormalized_destroy
};

static void
SetpointCommandNormalized_initialize(SetpointCommandNormalized self)
{
    self->virtualFunctionTable = &(setpointCommandNormalizedVFT);
    self->type = C_SE_NA_1;
}

void
SetpointCommandNormalized_destroy(SetpointCommandNormalized self)
{
    GLOBAL_FREEMEM(self);
}

SetpointCommandNormalized
SetpointCommandNormalized_create(SetpointCommandNormalized self, int ioa, float value, bool selectCommand, int ql)
{
    if (self == NULL)
        self = (SetpointCommandNormalized) GLOBAL_MALLOC(sizeof(struct sSetpointCommandNormalized));

    if (self) {
        SetpointCommandNormalized_initialize(self);

        self->objectAddress = ioa;

        int scaledValue = (int)((value * 32767.5) - 0.5);

        setScaledValue(self->encodedValue, scaledValue);

        uint8_t qos = ql;

        if (selectCommand) qos |= 0x80;

        self->qos = qos;
    }

    return self;
}

float
SetpointCommandNormalized_getValue(SetpointCommandNormalized self)
{
    float nv = ((float) getScaledValue(self->encodedValue) + 0.5) / 32767.5;

    return nv;
}

int
SetpointCommandNormalized_getQL(SetpointCommandNormalized self)
{
    return (int) (self->qos & 0x7f);
}

bool
SetpointCommandNormalized_isSelect(SetpointCommandNormalized self)
{
    return ((self->qos & 0x80) == 0x80);
}

SetpointCommandNormalized
SetpointCommandNormalized_getFromBuffer(SetpointCommandNormalized self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 3))
        return NULL;

    if (self == NULL)
        self = (SetpointCommandNormalized) GLOBAL_MALLOC(sizeof(struct sSetpointCommandNormalized));

    if (self != NULL) {
        SetpointCommandNormalized_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->encodedValue[0] = msg[startIndex++];
        self->encodedValue[1] = msg[startIndex++];

        /* QOS - qualifier of setpoint command */
        self->qos = msg[startIndex];
    }

    return self;
}

/**********************************************************************
 * SetpointCommandNormalizedWithCP56Time2a : SetpointCommandNormalized  C_SE_TA_1=61  带时标CP56Time2a的设点命令，归一化值
 **********************************************************************/

static bool
SetpointCommandNormalizedWithCP56Time2a_encode(SetpointCommandNormalizedWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 10 : (parameters->sizeOfIOA + 10);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    SetpointCommandNormalized_encode((SetpointCommandNormalized) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT setpointCommandNormalizedWithCP56Time2aVFT = {
        (EncodeFunction) SetpointCommandNormalizedWithCP56Time2a_encode,
        (DestroyFunction) SetpointCommandNormalizedWithCP56Time2a_destroy
};

static void
SetpointCommandNormalizedWithCP56Time2a_initialize(SetpointCommandNormalizedWithCP56Time2a self)
{
    self->virtualFunctionTable = &(setpointCommandNormalizedWithCP56Time2aVFT);
    self->type = C_SE_TA_1;
}

void
SetpointCommandNormalizedWithCP56Time2a_destroy(SetpointCommandNormalizedWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

SetpointCommandNormalizedWithCP56Time2a
SetpointCommandNormalizedWithCP56Time2a_create(SetpointCommandNormalizedWithCP56Time2a self, int ioa, float value, bool selectCommand, int ql, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (SetpointCommandNormalizedWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSetpointCommandNormalizedWithCP56Time2a));

    if (self) {
        SetpointCommandNormalizedWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        int scaledValue = (int)(value * 32767.f);

        setScaledValue(self->encodedValue, scaledValue);

        uint8_t qos = ql;

        if (selectCommand) qos |= 0x80;

        self->qos = qos;

        self->timestamp = *timestamp;
    }

    return self;
}

float
SetpointCommandNormalizedWithCP56Time2a_getValue(SetpointCommandNormalizedWithCP56Time2a self)
{
    return SetpointCommandNormalized_getValue((SetpointCommandNormalized) self);
}

int
SetpointCommandNormalizedWithCP56Time2a_getQL(SetpointCommandNormalizedWithCP56Time2a self)
{
    return SetpointCommandNormalized_getQL((SetpointCommandNormalized) self);
}

bool
SetpointCommandNormalizedWithCP56Time2a_isSelect(SetpointCommandNormalizedWithCP56Time2a self)
{
    return SetpointCommandNormalized_isSelect((SetpointCommandNormalized) self);
}

SetpointCommandNormalizedWithCP56Time2a
SetpointCommandNormalizedWithCP56Time2a_getFromBuffer(SetpointCommandNormalizedWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 10))
        return NULL;

    if (self == NULL)
        self = (SetpointCommandNormalizedWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSetpointCommandNormalizedWithCP56Time2a));

    if (self != NULL) {
        SetpointCommandNormalizedWithCP56Time2a_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->encodedValue[0] = msg[startIndex++];
        self->encodedValue[1] = msg[startIndex++];

        /* QOS - qualifier of setpoint command */
        self->qos = msg[startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*************************************************
 * SetpointCommandScaled: InformationObject  C_SE_NB_1=49  设点命令，标度化值
 ************************************************/

static bool
SetpointCommandScaled_encode(SetpointCommandScaled self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 3 : (parameters->sizeOfIOA + 3);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->encodedValue, 2);
    Frame_setNextByte(frame, self->qos);

    return true;
}

struct sInformationObjectVFT setpointCommandScaledVFT = {
        (EncodeFunction) SetpointCommandScaled_encode,
        (DestroyFunction) SetpointCommandScaled_destroy
};

static void
SetpointCommandScaled_initialize(SetpointCommandScaled self)
{
    self->virtualFunctionTable = &(setpointCommandScaledVFT);
    self->type = C_SE_NB_1;
}

void
SetpointCommandScaled_destroy(SetpointCommandScaled self)
{
    GLOBAL_FREEMEM(self);
}

SetpointCommandScaled
SetpointCommandScaled_create(SetpointCommandScaled self, int ioa, int value, bool selectCommand, int ql)
{
    if (self == NULL)
        self = (SetpointCommandScaled) GLOBAL_MALLOC(sizeof(struct sSetpointCommandScaled));

    if (self) {
        SetpointCommandScaled_initialize(self);

        self->objectAddress = ioa;

        setScaledValue(self->encodedValue, value);

        uint8_t qos = ql;

        if (selectCommand) qos |= 0x80;

        self->qos = qos;
    }

    return self;
}

int
SetpointCommandScaled_getValue(SetpointCommandScaled self)
{
    return getScaledValue(self->encodedValue);
}

int
SetpointCommandScaled_getQL(SetpointCommandScaled self)
{
    return (int) (self->qos & 0x7f);
}

bool
SetpointCommandScaled_isSelect(SetpointCommandScaled self)
{
    return ((self->qos & 0x80) == 0x80);
}

SetpointCommandScaled
SetpointCommandScaled_getFromBuffer(SetpointCommandScaled self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 3))
        return NULL;

    if (self == NULL)
        self = (SetpointCommandScaled) GLOBAL_MALLOC(sizeof(struct sSetpointCommandScaled));

    if (self != NULL) {
        SetpointCommandScaled_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->encodedValue[0] = msg[startIndex++];
        self->encodedValue[1] = msg[startIndex++];

        /* QOS - qualifier of setpoint command */
        self->qos = msg[startIndex];
    }

    return self;
}

/**********************************************************************
 * SetpointCommandScaledWithCP56Time2a : SetpointCommandScaled  C_SE_TB_1=62  带时标CP56Time2a的设点命令，标度化值
 **********************************************************************/

static bool
SetpointCommandScaledWithCP56Time2a_encode(SetpointCommandScaledWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 10 : (parameters->sizeOfIOA + 10);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    SetpointCommandScaled_encode((SetpointCommandScaled) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT setpointCommandScaledWithCP56Time2aVFT = {
        (EncodeFunction) SetpointCommandScaledWithCP56Time2a_encode,
        (DestroyFunction) SetpointCommandScaledWithCP56Time2a_destroy
};

static void
SetpointCommandScaledWithCP56Time2a_initialize(SetpointCommandScaledWithCP56Time2a self)
{
    self->virtualFunctionTable = &(setpointCommandScaledWithCP56Time2aVFT);
    self->type = C_SE_TB_1;
}

void
SetpointCommandScaledWithCP56Time2a_destroy(SetpointCommandScaledWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

SetpointCommandScaledWithCP56Time2a
SetpointCommandScaledWithCP56Time2a_create(SetpointCommandScaledWithCP56Time2a self, int ioa, int value, bool selectCommand, int ql, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (SetpointCommandScaledWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSetpointCommandScaledWithCP56Time2a));

    if (self) {
        SetpointCommandScaledWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        setScaledValue(self->encodedValue, value);

        uint8_t qos = ql;

        if (selectCommand) qos |= 0x80;

        self->qos = qos;

        self->timestamp = *timestamp;
    }

    return self;
}

int
SetpointCommandScaledWithCP56Time2a_getValue(SetpointCommandScaledWithCP56Time2a self)
{
    return SetpointCommandScaled_getValue((SetpointCommandScaled) self);
}

int
SetpointCommandScaledWithCP56Time2a_getQL(SetpointCommandScaledWithCP56Time2a self)
{
    return SetpointCommandScaled_getQL((SetpointCommandScaled) self);
}

bool
SetpointCommandScaledWithCP56Time2a_isSelect(SetpointCommandScaledWithCP56Time2a self)
{
    return SetpointCommandScaled_isSelect((SetpointCommandScaled) self);
}

SetpointCommandScaledWithCP56Time2a
SetpointCommandScaledWithCP56Time2a_getFromBuffer(SetpointCommandScaledWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 10))
        return NULL;

    if (self == NULL)
        self = (SetpointCommandScaledWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSetpointCommandScaledWithCP56Time2a));

    if (self != NULL) {
        SetpointCommandScaledWithCP56Time2a_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->encodedValue[0] = msg[startIndex++];
        self->encodedValue[1] = msg[startIndex++];

        /* QOS - qualifier of setpoint command */
        self->qos = msg[startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*************************************************
 * SetpointCommandShort: InformationObject  C_SE_NC_1=50  设点命令，短浮点数
 ************************************************/

static bool
SetpointCommandShort_encode(SetpointCommandShort self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 5 : (parameters->sizeOfIOA + 5);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
    Frame_appendBytes(frame, valueBytes, 4);
#else
    Frame_setNextByte(frame, valueBytes[3]);
    Frame_setNextByte(frame, valueBytes[2]);
    Frame_setNextByte(frame, valueBytes[1]);
    Frame_setNextByte(frame, valueBytes[0]);
#endif

    Frame_setNextByte(frame, self->qos);

    return true;
}

struct sInformationObjectVFT setpointCommandShortVFT = {
        (EncodeFunction) SetpointCommandShort_encode,
        (DestroyFunction) SetpointCommandShort_destroy
};

static void
SetpointCommandShort_initialize(SetpointCommandShort self)
{
    self->virtualFunctionTable = &(setpointCommandShortVFT);
    self->type = C_SE_NC_1;
}

void
SetpointCommandShort_destroy(SetpointCommandShort self)
{
    GLOBAL_FREEMEM(self);
}

SetpointCommandShort
SetpointCommandShort_create(SetpointCommandShort self, int ioa, float value, bool selectCommand, int ql)
{
    if (self == NULL)
        self = (SetpointCommandShort) GLOBAL_MALLOC(sizeof(struct sSetpointCommandShort));

    if (self) {
        SetpointCommandShort_initialize(self);

        self->objectAddress = ioa;

        self->value = value;

        uint8_t qos = ql & 0x7f;

        if (selectCommand) qos |= 0x80;

        self->qos = qos;
    }

    return self;
}

float
SetpointCommandShort_getValue(SetpointCommandShort self)
{
    return self->value;
}

int
SetpointCommandShort_getQL(SetpointCommandShort self)
{
    return (int) (self->qos & 0x7f);
}

bool
SetpointCommandShort_isSelect(SetpointCommandShort self)
{
    return ((self->qos & 0x80) == 0x80);
}

SetpointCommandShort
SetpointCommandShort_getFromBuffer(SetpointCommandShort self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 5))
        return NULL;

    if (self == NULL)
        self = (SetpointCommandShort) GLOBAL_MALLOC(sizeof(struct sSetpointCommandShort));

    if (self != NULL) {
        SetpointCommandShort_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
        valueBytes[0] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[3] = msg [startIndex++];
#else
        valueBytes[3] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[0] = msg [startIndex++];
#endif


        /* QOS - qualifier of setpoint command */
        self->qos = msg[startIndex];
    }

    return self;
}


/**********************************************************************
 * SetpointCommandShortWithCP56Time2a : SetpointCommandShort  C_SE_TC_1=63  带时标CP56Time2a的设点命令，短浮点数
 **********************************************************************/

static bool
SetpointCommandShortWithCP56Time2a_encode(SetpointCommandShortWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 12 : (parameters->sizeOfIOA + 12);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    SetpointCommandShort_encode((SetpointCommandShort) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT setpointCommandShortWithCP56Time2aVFT = {
        (EncodeFunction) SetpointCommandShortWithCP56Time2a_encode,
        (DestroyFunction) SetpointCommandShortWithCP56Time2a_destroy
};

static void
SetpointCommandShortWithCP56Time2a_initialize(SetpointCommandShortWithCP56Time2a self)
{
    self->virtualFunctionTable = &(setpointCommandShortWithCP56Time2aVFT);
    self->type = C_SE_TC_1;
}

void
SetpointCommandShortWithCP56Time2a_destroy(SetpointCommandShortWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

SetpointCommandShortWithCP56Time2a
SetpointCommandShortWithCP56Time2a_create(SetpointCommandShortWithCP56Time2a self, int ioa, float value, bool selectCommand, int ql, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (SetpointCommandShortWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSetpointCommandShortWithCP56Time2a));

    if (self) {
        SetpointCommandShortWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        self->value = value;

        uint8_t qos = ql;

        if (selectCommand) qos |= 0x80;

        self->qos = qos;

        self->timestamp = *timestamp;
    }

    return self;
}

float
SetpointCommandShortWithCP56Time2a_getValue(SetpointCommandShortWithCP56Time2a self)
{
    return SetpointCommandShort_getValue((SetpointCommandShort) self);
}

int
SetpointCommandShortWithCP56Time2a_getQL(SetpointCommandShortWithCP56Time2a self)
{
    return SetpointCommandShort_getQL((SetpointCommandShort) self);
}

bool
SetpointCommandShortWithCP56Time2a_isSelect(SetpointCommandShortWithCP56Time2a self)
{
    return SetpointCommandShort_isSelect((SetpointCommandShort) self);
}

SetpointCommandShortWithCP56Time2a
SetpointCommandShortWithCP56Time2a_getFromBuffer(SetpointCommandShortWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 10))
        return NULL;

    if (self == NULL)
        self = (SetpointCommandShortWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sSetpointCommandShortWithCP56Time2a));

    if (self != NULL) {
        SetpointCommandShortWithCP56Time2a_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
        valueBytes[0] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[3] = msg [startIndex++];
#else
        valueBytes[3] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[0] = msg [startIndex++];
#endif

        /* QOS - qualifier of setpoint command */
        self->qos = msg[startIndex++];

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*************************************************
 * Bitstring32Command : InformationObject  C_BO_NA_1=51  32比特串
 ************************************************/

static bool
Bitstring32Command_encode(Bitstring32Command self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 5 : (parameters->sizeOfIOA + 5);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
    Frame_appendBytes(frame, valueBytes, 4);
#else
    Frame_setNextByte(frame, valueBytes[3]);
    Frame_setNextByte(frame, valueBytes[2]);
    Frame_setNextByte(frame, valueBytes[1]);
    Frame_setNextByte(frame, valueBytes[0]);
#endif

    return true;
}

struct sInformationObjectVFT bitstring32CommandVFT = {
        (EncodeFunction) Bitstring32Command_encode,
        (DestroyFunction) Bitstring32Command_destroy
};

static void
Bitstring32Command_initialize(Bitstring32Command self)
{
    self->virtualFunctionTable = &(bitstring32CommandVFT);
    self->type = C_BO_NA_1;
}

Bitstring32Command
Bitstring32Command_create(Bitstring32Command self, int ioa, uint32_t value)
{
    if (self == NULL)
        self = (Bitstring32Command) GLOBAL_MALLOC(sizeof(struct sBitstring32Command));

    if (self) {
        Bitstring32Command_initialize(self);

        self->objectAddress = ioa;

        self->value = value;
    }

    return self;
}

void
Bitstring32Command_destroy(Bitstring32Command self)
{
    GLOBAL_FREEMEM(self);
}

uint32_t
Bitstring32Command_getValue(Bitstring32Command self)
{
    return self->value;
}

Bitstring32Command
Bitstring32Command_getFromBuffer(Bitstring32Command self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 4))
        return NULL;

    if (self == NULL)
        self = (Bitstring32Command) GLOBAL_MALLOC(sizeof(struct sBitstring32Command));

    if (self != NULL) {
        Bitstring32Command_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
        valueBytes[0] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[3] = msg [startIndex++];
#else
        valueBytes[3] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[0] = msg [startIndex++];
#endif
    }

    return self;
}


/*******************************************************
 * Bitstring32CommandWithCP56Time2a: Bitstring32Command  C_BO_TA_1=64  带时标CP56Time2a的32比特串
 *******************************************************/

static bool
Bitstring32CommandWithCP56Time2a_encode(Bitstring32CommandWithCP56Time2a self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 12 : (parameters->sizeOfIOA + 12);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    Bitstring32Command_encode((Bitstring32Command) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT bitstring32CommandWithCP56Time2aVFT = {
        (EncodeFunction) Bitstring32CommandWithCP56Time2a_encode,
        (DestroyFunction) Bitstring32CommandWithCP56Time2a_destroy
};

static void
Bitstring32CommandWithCP56Time2a_initialize(Bitstring32CommandWithCP56Time2a self)
{
    self->virtualFunctionTable = &(bitstring32CommandWithCP56Time2aVFT);
    self->type = C_BO_TA_1;
}

Bitstring32CommandWithCP56Time2a
Bitstring32CommandWithCP56Time2a_create(Bitstring32CommandWithCP56Time2a self, int ioa, uint32_t value, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (Bitstring32CommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sBitstring32CommandWithCP56Time2a));

    if (self) {
        Bitstring32CommandWithCP56Time2a_initialize(self);

        self->objectAddress = ioa;

        self->value = value;

        self->timestamp = *timestamp;
    }

    return self;
}

void
Bitstring32CommandWithCP56Time2a_destroy(Bitstring32CommandWithCP56Time2a self)
{
    GLOBAL_FREEMEM(self);
}

uint32_t
Bitstring32CommandWithCP56Time2a_getValue(Bitstring32CommandWithCP56Time2a self)
{
    return self->value;
}

CP56Time2a
Bitstring32CommandWithCP56Time2a_getTimestamp(Bitstring32CommandWithCP56Time2a self)
{
    return &(self->timestamp);
}

Bitstring32CommandWithCP56Time2a
Bitstring32CommandWithCP56Time2a_getFromBuffer(Bitstring32CommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA + 11))
        return NULL;

    if (self == NULL)
        self = (Bitstring32CommandWithCP56Time2a) GLOBAL_MALLOC(sizeof(struct sBitstring32CommandWithCP56Time2a));

    if (self != NULL) {
        Bitstring32CommandWithCP56Time2a_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        uint8_t* valueBytes = (uint8_t*) &(self->value);

#if (ORDER_LITTLE_ENDIAN == 1)
        valueBytes[0] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[3] = msg [startIndex++];
#else
        valueBytes[3] = msg [startIndex++];
        valueBytes[2] = msg [startIndex++];
        valueBytes[1] = msg [startIndex++];
        valueBytes[0] = msg [startIndex++];
#endif

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}


/*************************************************
 * ReadCommand : InformationObject  C_RD_NA_1=102  读命令
 ************************************************/

static bool
ReadCommand_encode(ReadCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 0 : (parameters->sizeOfIOA + 0);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    return true;
}

struct sInformationObjectVFT readCommandVFT = {
        (EncodeFunction) ReadCommand_encode,
        (DestroyFunction) ReadCommand_destroy
};

static void
ReadCommand_initialize(ReadCommand self)
{
    self->virtualFunctionTable = &(readCommandVFT);
    self->type = C_RD_NA_1;
}

ReadCommand
ReadCommand_create(ReadCommand self, int ioa)
{
    if (self == NULL)
        self = (ReadCommand) GLOBAL_MALLOC(sizeof(struct sReadCommand));

    if (self) {
        ReadCommand_initialize(self);

        self->objectAddress = ioa;
    }

    return self;
}

void
ReadCommand_destroy(ReadCommand self)
{
    GLOBAL_FREEMEM(self);
}


ReadCommand
ReadCommand_getFromBuffer(ReadCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA))
        return NULL;

    if (self == NULL)
        self = (ReadCommand) GLOBAL_MALLOC(sizeof(struct sReadCommand));

    if (self != NULL) {
        ReadCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);
    }

    return self;
}

/***************************************************
 * ClockSynchronizationCommand : InformationObject  C_CS_NA_1=103  时钟同步命令
 **************************************************/

static bool
ClockSynchronizationCommand_encode(ClockSynchronizationCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 7 : (parameters->sizeOfIOA + 7);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->timestamp.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT clockSynchronizationCommandVFT = {
        (EncodeFunction) ClockSynchronizationCommand_encode,
        (DestroyFunction) ClockSynchronizationCommand_destroy
};

static void
ClockSynchronizationCommand_initialize(ClockSynchronizationCommand self)
{
    self->virtualFunctionTable = &(clockSynchronizationCommandVFT);
    self->type = C_CS_NA_1;
}

ClockSynchronizationCommand
ClockSynchronizationCommand_create(ClockSynchronizationCommand self, int ioa, CP56Time2a timestamp)
{
    if (self == NULL)
        self = (ClockSynchronizationCommand) GLOBAL_MALLOC(sizeof(struct sClockSynchronizationCommand));

    if (self) {
        ClockSynchronizationCommand_initialize(self);

        self->objectAddress = ioa;
        self->timestamp = *timestamp;
    }

    return self;
}

void
ClockSynchronizationCommand_destroy(ClockSynchronizationCommand self)
{
    GLOBAL_FREEMEM(self);
}

CP56Time2a
ClockSynchronizationCommand_getTime(ClockSynchronizationCommand self)
{
    return &(self->timestamp);
}

ClockSynchronizationCommand
ClockSynchronizationCommand_getFromBuffer(ClockSynchronizationCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 7)
        return NULL;

    if (self == NULL)
        self = (ClockSynchronizationCommand) GLOBAL_MALLOC(sizeof(struct sClockSynchronizationCommand));

    if (self != NULL) {
        ClockSynchronizationCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* timestamp */
        CP56Time2a_getFromBuffer(&(self->timestamp), msg, msgSize, startIndex);
    }

    return self;
}

/*************************************************
 * InterrogationCommand : InformationObject  C_IC_NA_1=100  总召唤命令
 ************************************************/

static bool
InterrogationCommand_encode(InterrogationCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->qoi);

    return true;
}

struct sInformationObjectVFT interrogationCommandVFT = {
        (EncodeFunction) InterrogationCommand_encode,
        (DestroyFunction) InterrogationCommand_destroy
};

static void
InterrogationCommand_initialize(InterrogationCommand self)
{
    self->virtualFunctionTable = &(interrogationCommandVFT);
    self->type = C_IC_NA_1;
}

InterrogationCommand
InterrogationCommand_create(InterrogationCommand self, int ioa, uint8_t qoi)
{
    if (self == NULL)
        self = (InterrogationCommand) GLOBAL_MALLOC(sizeof(struct sInterrogationCommand));

    if (self) {
        InterrogationCommand_initialize(self);

        self->objectAddress = ioa;

        self->qoi = qoi;
    }

    return self;
}

void
InterrogationCommand_destroy(InterrogationCommand self)
{
    GLOBAL_FREEMEM(self);
}

uint8_t
InterrogationCommand_getQOI(InterrogationCommand self)
{
    return self->qoi;
}

InterrogationCommand
InterrogationCommand_getFromBuffer(InterrogationCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 1)
        return NULL;

    if (self == NULL)
        self = (InterrogationCommand) GLOBAL_MALLOC(sizeof(struct sInterrogationCommand));

    if (self != NULL) {
        InterrogationCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* QUI */
        self->qoi = msg[startIndex];
    }

    return self;
}

/**************************************************
 * CounterInterrogationCommand : InformationObject  C_CI_NA_1=101  电能量脉冲命令
 **************************************************/

static bool
CounterInterrogationCommand_encode(CounterInterrogationCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->qcc);

    return true;
}

struct sInformationObjectVFT counterInterrogationCommandVFT = {
        (EncodeFunction) CounterInterrogationCommand_encode,
        (DestroyFunction) CounterInterrogationCommand_destroy
};

static void
CounterInterrogationCommand_initialize(CounterInterrogationCommand self)
{
    self->virtualFunctionTable = &(counterInterrogationCommandVFT);
    self->type = C_CI_NA_1;
}

CounterInterrogationCommand
CounterInterrogationCommand_create(CounterInterrogationCommand self, int ioa, QualifierOfCIC qcc)
{
    if (self == NULL)
        self = (CounterInterrogationCommand) GLOBAL_MALLOC(sizeof(struct sCounterInterrogationCommand));

    if (self) {
        CounterInterrogationCommand_initialize(self);

        self->objectAddress = ioa;

        self->qcc = qcc;
    }

    return self;
}

void
CounterInterrogationCommand_destroy(CounterInterrogationCommand self)
{
    GLOBAL_FREEMEM(self);
}

QualifierOfCIC
CounterInterrogationCommand_getQCC(CounterInterrogationCommand self)
{
    return self->qcc;
}

CounterInterrogationCommand
CounterInterrogationCommand_getFromBuffer(CounterInterrogationCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 1)
        return NULL;

    if (self == NULL)
        self = (CounterInterrogationCommand) GLOBAL_MALLOC(sizeof(struct sCounterInterrogationCommand));

    if (self != NULL) {
        CounterInterrogationCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* QCC */
        self->qcc = msg[startIndex];
    }

    return self;
}

/*************************************************
 * TestCommand : InformationObject  C_TS_NA_1=104  测试命令
 ************************************************/

static bool
TestCommand_encode(TestCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 2 : (parameters->sizeOfIOA + 2);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->byte1);
    Frame_setNextByte(frame, self->byte2);

    return true;
}

struct sInformationObjectVFT testCommandVFT = {
        (EncodeFunction) TestCommand_encode,
        (DestroyFunction) TestCommand_destroy
};

static void
TestCommand_initialize(TestCommand self)
{
    self->virtualFunctionTable = &(testCommandVFT);
    self->type = C_TS_NA_1;
}

TestCommand
TestCommand_create(TestCommand self)
{
    if (self == NULL)
        self = (TestCommand) GLOBAL_MALLOC(sizeof(struct sTestCommand));

    if (self) {
        TestCommand_initialize(self);

        self->objectAddress = 0;

        self->byte1 = 0xcc;
        self->byte2 = 0x55;
    }

    return self;
}

void
TestCommand_destroy(TestCommand self)
{
    GLOBAL_FREEMEM(self);
}

bool
TestCommand_isValid(TestCommand self)
{
    if ((self->byte1 == 0xcc) && (self->byte2 == 0x55))
        return true;
    else
        return false;
}

TestCommand
TestCommand_getFromBuffer(TestCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 1)
        return NULL;

    if (self == NULL)
        self = (TestCommand) GLOBAL_MALLOC(sizeof(struct sTestCommand));

    if (self != NULL) {
        TestCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* test bytes */
        self->byte1 = msg[startIndex++];
        self->byte2 = msg[startIndex];
    }

    return self;
}

/*************************************************
 * ResetProcessCommand : InformationObject  C_RP_NA_1=105  复位进程命令
 ************************************************/

static bool
ResetProcessCommand_encode(ResetProcessCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->qrp);

    return true;
}

struct sInformationObjectVFT resetProcessCommandVFT = {
        (EncodeFunction) ResetProcessCommand_encode,
        (DestroyFunction) ResetProcessCommand_destroy
};

static void
ResetProcessCommand_initialize(ResetProcessCommand self)
{
    self->virtualFunctionTable = &(resetProcessCommandVFT);
    self->type = C_RP_NA_1;
}

ResetProcessCommand
ResetProcessCommand_create(ResetProcessCommand self, int ioa,  QualifierOfRPC qrp)
{
    if (self == NULL)
        self = (ResetProcessCommand) GLOBAL_MALLOC(sizeof(struct sResetProcessCommand));

    if (self) {
        ResetProcessCommand_initialize(self);

        self->objectAddress = ioa;

        self->qrp = qrp;
    }

    return self;
}

void
ResetProcessCommand_destroy(ResetProcessCommand self)
{
    GLOBAL_FREEMEM(self);
}

QualifierOfRPC
ResetProcessCommand_getQRP(ResetProcessCommand self)
{
    return self->qrp;
}

ResetProcessCommand
ResetProcessCommand_getFromBuffer(ResetProcessCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 1)
        return NULL;

    if (self == NULL)
        self = (ResetProcessCommand) GLOBAL_MALLOC(sizeof(struct sResetProcessCommand));

    if (self != NULL) {
        ResetProcessCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* QUI */
        self->qrp = msg[startIndex];
    }

    return self;
}

/*************************************************
 * DelayAcquisitionCommand : InformationObject  C_CD_NA_1=106  延时获得命令
 ************************************************/

static bool
DelayAcquisitionCommand_encode(DelayAcquisitionCommand self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 2 : (parameters->sizeOfIOA + 2);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_appendBytes(frame, self->delay.encodedValue, 2);

    return true;
}

struct sInformationObjectVFT DelayAcquisitionCommandVFT = {
        (EncodeFunction) DelayAcquisitionCommand_encode,
        (DestroyFunction) DelayAcquisitionCommand_destroy
};

static void
DelayAcquisitionCommand_initialize(DelayAcquisitionCommand self)
{
    self->virtualFunctionTable = &(DelayAcquisitionCommandVFT);
    self->type = C_CD_NA_1;
}

DelayAcquisitionCommand
DelayAcquisitionCommand_create(DelayAcquisitionCommand self, int ioa,  CP16Time2a delay)
{
    if (self == NULL)
        self = (DelayAcquisitionCommand) GLOBAL_MALLOC(sizeof(struct sDelayAcquisitionCommand));

    if (self) {
        DelayAcquisitionCommand_initialize(self);

        self->objectAddress = ioa;

        self->delay = *delay;
    }

    return self;
}

void
DelayAcquisitionCommand_destroy(DelayAcquisitionCommand self)
{
    GLOBAL_FREEMEM(self);
}

CP16Time2a
DelayAcquisitionCommand_getDelay(DelayAcquisitionCommand self)
{
    return &(self->delay);
}

DelayAcquisitionCommand
DelayAcquisitionCommand_getFromBuffer(DelayAcquisitionCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 1)
        return NULL;

    if (self == NULL)
        self = (DelayAcquisitionCommand) GLOBAL_MALLOC(sizeof(struct sDelayAcquisitionCommand));

    if (self != NULL) {
        DelayAcquisitionCommand_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* delay */
        CP16Time2a_getFromBuffer(&(self->delay), msg, msgSize, startIndex);
    }

    return self;
}


/*******************************************
 * ParameterActivation : InformationObject  P_AC_NA_1=113  参数激活
 *******************************************/

static bool
ParameterActivation_encode(ParameterActivation self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->qpa);

    return true;
}

struct sInformationObjectVFT parameterActivationVFT = {
        (EncodeFunction) ParameterActivation_encode,
        (DestroyFunction) ParameterActivation_destroy
};

static void
ParameterActivation_initialize(ParameterActivation self)
{
    self->virtualFunctionTable = &(parameterActivationVFT);
    self->type = P_AC_NA_1;
}

void
ParameterActivation_destroy(ParameterActivation self)
{
    GLOBAL_FREEMEM(self);
}


ParameterActivation
ParameterActivation_create(ParameterActivation self, int ioa, QualifierOfParameterActivation qpa)
{
    if (self == NULL)
        self = (ParameterActivation) GLOBAL_CALLOC(1, sizeof(struct sParameterActivation));

    if (self) {
        ParameterActivation_initialize(self);

        self->objectAddress = ioa;
        self->qpa = qpa;
    }

    return self;
}


QualifierOfParameterActivation
ParameterActivation_getQuality(ParameterActivation self)
{
    return self->qpa;
}

ParameterActivation
ParameterActivation_getFromBuffer(ParameterActivation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    //TODO check message size

    if (self == NULL)
        self = (ParameterActivation) GLOBAL_MALLOC(sizeof(struct sParameterActivation));

    if (self) {
        ParameterActivation_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* QPA */
        self->qpa = (QualifierOfParameterActivation) msg [startIndex++];
    }

    return self;
}

/*******************************************
 * EndOfInitialization : InformationObject  M_EI_NA_1=70  初始化结束
 *******************************************/

static bool
EndOfInitialization_encode(EndOfInitialization self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte(frame, self->coi);

    return true;
}

struct sInformationObjectVFT EndOfInitializationVFT = {
        (EncodeFunction) EndOfInitialization_encode,
        (DestroyFunction) EndOfInitialization_destroy
};

static void
EndOfInitialization_initialize(EndOfInitialization self)
{
    self->virtualFunctionTable = &(EndOfInitializationVFT);
    self->type = M_EI_NA_1;
}

EndOfInitialization
EndOfInitialization_create(EndOfInitialization self, uint8_t coi)
{
    if (self == NULL)
       self = (EndOfInitialization) GLOBAL_MALLOC(sizeof(struct sEndOfInitialization));

    if (self) {
        EndOfInitialization_initialize(self);

        self->objectAddress = 0;

        self->coi = coi;
    }

    return self;
};

void
EndOfInitialization_destroy(EndOfInitialization self)
{
    GLOBAL_FREEMEM(self);
}

uint8_t
EndOfInitialization_getCOI(EndOfInitialization self)
{
    return self->coi;
}

EndOfInitialization
EndOfInitialization_getFromBuffer(EndOfInitialization self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 1)
        return NULL;

    if (self == NULL)
       self = (EndOfInitialization) GLOBAL_MALLOC(sizeof(struct sEndOfInitialization));

    if (self) {
        EndOfInitialization_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        /* COI */
        self->coi = msg[startIndex];
    }

    return self;
};

/*******************************************
 * FileReady : InformationObject  F_FR_NA_1（原-120）
 *******************************************/

static bool
FileReady_encode(FileReady self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)((int) self->nof % 256));
    Frame_setNextByte (frame, (uint8_t)((int) self->nof / 256));

    Frame_setNextByte (frame, (uint8_t)(self->lengthOfFile % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->lengthOfFile / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->lengthOfFile / 0x10000) % 0x100));

    Frame_setNextByte (frame, self->frq);

    return true;
}

struct sInformationObjectVFT FileReadyVFT = {
        (EncodeFunction) FileReady_encode,
        (DestroyFunction) FileReady_destroy
};

static void
FileReady_initialize(FileReady self)
{
    self->virtualFunctionTable = &(FileReadyVFT);
    self->type = F_FR_NA_1;
}

FileReady
FileReady_create(FileReady self, int ioa, uint16_t nof, uint32_t lengthOfFile, bool positive)
{
    if (self == NULL)
       self = (FileReady) GLOBAL_MALLOC(sizeof(struct sFileReady));

    if (self) {
        FileReady_initialize(self);

        self->objectAddress = ioa;
        self->nof = nof;
        self->lengthOfFile = lengthOfFile;

        if (positive)
            self->frq = 0x80;
        else
            self->frq = 0;
    }

    return self;
}

uint8_t
FileReady_getFRQ(FileReady self)
{
    return self->frq;
}

void
FileReady_setFRQ(FileReady self, uint8_t frq)
{
    self->frq = frq;
}

bool
FileReady_isPositive(FileReady self)
{
    return ((self->frq & 0x80) == 0x80);
}

uint16_t
FileReady_getNOF(FileReady self)
{
    return self->nof;
}

uint32_t
FileReady_getLengthOfFile(FileReady self)
{
    return self->lengthOfFile;
}

void
FileReady_destroy(FileReady self)
{
    GLOBAL_FREEMEM(self);
}

FileReady
FileReady_getFromBuffer(FileReady self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 6)
        return NULL;

    if (self == NULL)
       self = (FileReady) GLOBAL_MALLOC(sizeof(struct sFileReady));

    if (self != NULL) {
        FileReady_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->nof = msg[startIndex++];
        self->nof += (msg[startIndex++] * 0x100);

        self->lengthOfFile = msg [startIndex++];
        self->lengthOfFile += (msg [startIndex++] * 0x100);
        self->lengthOfFile += (msg [startIndex++] * 0x10000);

        /* FRQ */
        self->frq = msg[startIndex];
    }

    return self;
};

/*******************************************
 * SectionReady : InformationObject  F_SR_NA_1（原121）
 *******************************************/

static bool
SectionReady_encode(SectionReady self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)((int) self->nof % 256));
    Frame_setNextByte (frame, (uint8_t)((int) self->nof / 256));

    Frame_setNextByte (frame, self->nameOfSection);

    Frame_setNextByte (frame, (uint8_t)(self->lengthOfSection % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->lengthOfSection / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->lengthOfSection / 0x10000) % 0x100));

    Frame_setNextByte (frame, self->srq);

    return true;
}

struct sInformationObjectVFT SectionReadyVFT = {
        (EncodeFunction) SectionReady_encode,
        (DestroyFunction) SectionReady_destroy
};

static void
SectionReady_initialize(SectionReady self)
{
    self->virtualFunctionTable = &(SectionReadyVFT);
    self->type = F_SR_NA_1;
}

SectionReady
SectionReady_create(SectionReady self, int ioa, uint16_t nof, uint8_t nos, uint32_t lengthOfSection, bool notReady)
{
    if (self == NULL)
       self = (SectionReady) GLOBAL_MALLOC(sizeof(struct sSectionReady));

    if (self != NULL) {
        SectionReady_initialize(self);

        self->objectAddress = ioa;
        self->nof = nof;
        self->nameOfSection = nos;
        self->lengthOfSection = lengthOfSection;

        if (notReady)
            self->srq = 0x80;
        else
            self->srq = 0;
    }

    return self;
}


bool
SectionReady_isNotReady(SectionReady self)
{
    return ((self->srq & 0x80) == 0x80);
}

uint8_t
SectionReady_getSRQ(SectionReady self)
{
    return self->srq;
}

void
SectionReady_setSRQ(SectionReady self, uint8_t srq)
{
    self->srq = srq;
}

uint16_t
SectionReady_getNOF(SectionReady self)
{
    return self->nof;
}

uint8_t
SectionReady_getNameOfSection(SectionReady self)
{
    return self->nameOfSection;
}

uint32_t
SectionReady_getLengthOfSection(SectionReady self)
{
    return self->lengthOfSection;
}

void
SectionReady_destroy(SectionReady self)
{
    GLOBAL_FREEMEM(self);
}


SectionReady
SectionReady_getFromBuffer(SectionReady self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 7)
        return NULL;

    if (self == NULL)
       self = (SectionReady) GLOBAL_MALLOC(sizeof(struct sSectionReady));

    if (self != NULL) {
        SectionReady_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->nof = msg[startIndex++];
        self->nof += (msg[startIndex++] * 0x100);

        self->nameOfSection = msg[startIndex++];

        self->lengthOfSection = msg [startIndex++];
        self->lengthOfSection += (msg [startIndex++] * 0x100);
        self->lengthOfSection += (msg [startIndex++] * 0x10000);

        self->srq = msg[startIndex];
    }

    return self;
};


/*******************************************
 * FileCallOrSelect : InformationObject  F_SC_NA_1=122
 *******************************************/

static bool
FileCallOrSelect_encode(FileCallOrSelect self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)((int) self->nof % 256));
    Frame_setNextByte (frame, (uint8_t)((int) self->nof / 256));

    Frame_setNextByte (frame, self->nameOfSection);

    Frame_setNextByte (frame, self->scq);

    return true;
}

struct sInformationObjectVFT FileCallOrSelectVFT = {
        (EncodeFunction) FileCallOrSelect_encode,
        (DestroyFunction) FileCallOrSelect_destroy
};

static void
FileCallOrSelect_initialize(FileCallOrSelect self)
{
    self->virtualFunctionTable = &(FileCallOrSelectVFT);
    self->type = F_SC_NA_1;
}

FileCallOrSelect
FileCallOrSelect_create(FileCallOrSelect self, int ioa, uint16_t nof, uint8_t nos, uint8_t scq)
{
    if (self == NULL)
       self = (FileCallOrSelect) GLOBAL_MALLOC(sizeof(struct sFileCallOrSelect));

    if (self != NULL) {
        FileCallOrSelect_initialize(self);

        self->objectAddress = ioa;
        self->nof = nof;
        self->nameOfSection = nos;
        self->scq = scq;
    }

    return self;
}

uint16_t
FileCallOrSelect_getNOF(FileCallOrSelect self)
{
    return self->nof;
}

uint8_t
FileCallOrSelect_getNameOfSection(FileCallOrSelect self)
{
    return self->nameOfSection;
}

uint8_t
FileCallOrSelect_getSCQ(FileCallOrSelect self)
{
    return self->scq;
}


void
FileCallOrSelect_destroy(FileCallOrSelect self)
{
    GLOBAL_FREEMEM(self);
}


FileCallOrSelect
FileCallOrSelect_getFromBuffer(FileCallOrSelect self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 4)
        return NULL;

    if (self == NULL)
       self = (FileCallOrSelect) GLOBAL_MALLOC(sizeof(struct sFileCallOrSelect));

    if (self != NULL) {
        FileCallOrSelect_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->nof = msg[startIndex++];
        self->nof += (msg[startIndex++] * 0x100);

        self->nameOfSection = msg[startIndex++];

        self->scq = msg[startIndex];
    }

    return self;
};

/*************************************************
 * FileLastSegmentOrSection : InformationObject  F_LS_NA_1=123
 *************************************************/

static bool
FileLastSegmentOrSection_encode(FileLastSegmentOrSection self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)((int) self->nof % 256));
    Frame_setNextByte (frame, (uint8_t)((int) self->nof / 256));

    Frame_setNextByte (frame, self->nameOfSection);

    Frame_setNextByte (frame, self->lsq);

    Frame_setNextByte (frame, self->chs);

    return true;
}

struct sInformationObjectVFT FileLastSegmentOrSectionVFT = {
        (EncodeFunction) FileLastSegmentOrSection_encode,
        (DestroyFunction) FileLastSegmentOrSection_destroy
};

static void
FileLastSegmentOrSection_initialize(FileLastSegmentOrSection self)
{
    self->virtualFunctionTable = &(FileLastSegmentOrSectionVFT);
    self->type = F_LS_NA_1;
}

FileLastSegmentOrSection
FileLastSegmentOrSection_create(FileLastSegmentOrSection self, int ioa, uint16_t nof, uint8_t nos, uint8_t lsq, uint8_t chs)
{
    if (self == NULL)
       self = (FileLastSegmentOrSection) GLOBAL_MALLOC(sizeof(struct sFileLastSegmentOrSection));

    if (self != NULL) {
        FileLastSegmentOrSection_initialize(self);

        self->objectAddress = ioa;
        self->nof = nof;
        self->nameOfSection = nos;
        self->lsq = lsq;
        self->chs = chs;
    }

    return self;
}

unsigned short//uint16_t
FileLastSegmentOrSection_getNOF(FileLastSegmentOrSection self)
{
    return self->nof;
}

unsigned char//uint8_t
FileLastSegmentOrSection_getNameOfSection(FileLastSegmentOrSection self)
{
    return self->nameOfSection;
}

unsigned char//uint8_t
FileLastSegmentOrSection_getLSQ(FileLastSegmentOrSection self)
{
    return self->lsq;
}

unsigned char//uint8_t
FileLastSegmentOrSection_getCHS(FileLastSegmentOrSection self)
{
    return self->chs;
}

void
FileLastSegmentOrSection_destroy(FileLastSegmentOrSection self)
{
    GLOBAL_FREEMEM(self);
}

FileLastSegmentOrSection
FileLastSegmentOrSection_getFromBuffer(FileLastSegmentOrSection self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 5)
        return NULL;

    if (self == NULL)
       self = (FileLastSegmentOrSection) GLOBAL_MALLOC(sizeof(struct sFileLastSegmentOrSection));

    if (self != NULL) {
        FileLastSegmentOrSection_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->nof = msg[startIndex++];
        self->nof += (msg[startIndex++] * 0x100);

        self->nameOfSection = msg[startIndex++];

        self->lsq = msg[startIndex];

        self->chs = msg[startIndex];
    }

    return self;
};

/*************************************************
 * FileACK : InformationObject  F_AF_NA_1=124
 *************************************************/

static bool
FileACK_encode(FileACK self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)((int) self->nof % 256));
    Frame_setNextByte (frame, (uint8_t)((int) self->nof / 256));

    Frame_setNextByte (frame, self->nameOfSection);

    Frame_setNextByte (frame, self->afq);

    return true;
}

struct sInformationObjectVFT FileACKVFT = {
        (EncodeFunction) FileACK_encode,
        (DestroyFunction) FileACK_destroy
};

static void
FileACK_initialize(FileACK self)
{
    self->virtualFunctionTable = &(FileACKVFT);
    self->type = F_AF_NA_1;
}


FileACK
FileACK_create(FileACK self, int ioa, uint16_t nof, uint8_t nos, uint8_t afq)
{
    if (self == NULL)
       self = (FileACK) GLOBAL_MALLOC(sizeof(struct sFileACK));

    if (self != NULL) {
        FileACK_initialize(self);

        self->objectAddress = ioa;
        self->nof = nof;
        self->nameOfSection = nos;
        self->afq = afq;
    }

    return self;
}

uint16_t
FileACK_getNOF(FileACK self)
{
    return self->nof;
}

uint8_t
FileACK_getNameOfSection(FileACK self)
{
    return self->nameOfSection;
}

uint8_t
FileACK_getAFQ(FileACK self)
{
    return self->afq;
}

void
FileACK_destroy(FileACK self)
{
    GLOBAL_FREEMEM(self);
}

FileACK
FileACK_getFromBuffer(FileACK self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 4)
        return NULL;

    if (self == NULL)
       self = (FileACK) GLOBAL_MALLOC(sizeof(struct sFileACK));

    if (self != NULL) {
        FileACK_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->nof = msg[startIndex++];
        self->nof += (msg[startIndex++] * 0x100);

        self->nameOfSection = msg[startIndex++];

        self->afq = msg[startIndex];
    }

    return self;
};


/*************************************************
 * FileSegment : InformationObject  F_SG_NA_1=125
 *************************************************/

static bool
FileSegment_encode(FileSegment self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    if (self->los > FileSegment_GetMaxDataSize(parameters))
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)((int) self->nof % 256));
    Frame_setNextByte (frame, (uint8_t)((int) self->nof / 256));

    Frame_setNextByte (frame, self->nameOfSection);

    Frame_setNextByte (frame, self->los);

    Frame_appendBytes (frame, self->data, self->los);

    return true;
}

struct sInformationObjectVFT FileSegmentVFT = {
        (EncodeFunction) FileSegment_encode,
        (DestroyFunction) FileSegment_destroy
};

static void
FileSegment_initialize(FileSegment self)
{
    self->virtualFunctionTable = &(FileSegmentVFT);
    self->type = F_SG_NA_1;
}

FileSegment
FileSegment_create(FileSegment self, int ioa, uint16_t nof, uint8_t nos, uint8_t* data, uint8_t los)
{
    if (self == NULL)
       self = (FileSegment) GLOBAL_MALLOC(sizeof(struct sFileSegment));

    if (self != NULL) {
        FileSegment_initialize(self);

        self->objectAddress = ioa;
        self->nof = nof;
        self->nameOfSection = nos;
        self->data = data;
        self->los = los;
    }

    return self;
}

uint16_t
FileSegment_getNOF(FileSegment self)
{
    return self->nof;
}

uint8_t
FileSegment_getNameOfSection(FileSegment self)
{
    return self->nameOfSection;
}

uint8_t
FileSegment_getLengthOfSegment(FileSegment self)
{
    return self->los;
}

uint8_t*
FileSegment_getSegmentData(FileSegment self)
{
    return self->data;
}


int
FileSegment_GetMaxDataSize(CS101_AppLayerParameters parameters)
{
    int maxSize = 249 -
        parameters->sizeOfTypeId - parameters->sizeOfVSQ - parameters->sizeOfCA - parameters->sizeOfCOT;

    return maxSize;
}

void
FileSegment_destroy(FileSegment self)
{
    GLOBAL_FREEMEM(self);
}

FileSegment
FileSegment_getFromBuffer(FileSegment self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex)
{
    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 4)
        return NULL;

    uint8_t los = msg[startIndex + 3 + parameters->sizeOfIOA];

    if ((msgSize - startIndex) < (parameters->sizeOfIOA) + 4 + los)
        return NULL;

    if (self == NULL)
       self = (FileSegment) GLOBAL_MALLOC(sizeof(struct sFileSegment));

    if (self != NULL) {
        FileSegment_initialize(self);

        InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

        startIndex += parameters->sizeOfIOA; /* skip IOA */

        self->nof = msg[startIndex++];
        self->nof += (msg[startIndex++] * 0x100);

        self->nameOfSection = msg[startIndex++];

        self->los = msg[startIndex++];

        self->data = msg + startIndex;
    }

    return self;
};

/*************************************************
 * FileDirectory: InformationObject  F_DR_TA_1=126
 *************************************************/


static bool
FileDirectory_encode(FileDirectory self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 13 : (parameters->sizeOfIOA + 13);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)((int) self->nof % 256));
    Frame_setNextByte (frame, (uint8_t)((int) self->nof / 256));

    Frame_setNextByte (frame, (uint8_t)(self->lengthOfFile % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->lengthOfFile / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->lengthOfFile / 0x10000) % 0x100));

    Frame_setNextByte (frame, self->sof);

    Frame_appendBytes(frame, self->creationTime.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT FileDirectoryVFT = {
        (EncodeFunction) FileDirectory_encode,
        (DestroyFunction) FileDirectory_destroy
};

static void
FileDirectory_initialize(FileDirectory self)
{
    self->virtualFunctionTable = &(FileDirectoryVFT);
    self->type = F_DR_TA_1;
}

FileDirectory
FileDirectory_create(FileDirectory self, int ioa, uint16_t nof, int lengthOfFile, uint8_t sof, CP56Time2a creationTime)
{
    if (self == NULL)
        self = (FileDirectory) GLOBAL_MALLOC(sizeof(struct sFileDirectory));

    if (self != NULL) {
        FileDirectory_initialize(self);

        self->objectAddress = ioa;
        self->nof = nof;
        self->sof = sof;
        self->creationTime = *creationTime;
    }

    return self;
}

uint16_t
FileDirectory_getNOF(FileDirectory self)
{
    return self->nof;
}

uint8_t
FileDirectory_getSOF(FileDirectory self)
{
    return self->sof;
}

int
FileDirectory_getSTATUS(FileDirectory self)
{
    return (int) (self->sof & CS101_SOF_STATUS);
}

bool
FileDirectory_getLFD(FileDirectory self)
{
    return ((self->sof & CS101_SOF_LFD) == CS101_SOF_LFD);
}

bool
FileDirectory_getFOR(FileDirectory self)
{
    return ((self->sof & CS101_SOF_FOR) == CS101_SOF_FOR);
}

bool
FileDirectory_getFA(FileDirectory self)
{
    return ((self->sof & CS101_SOF_FA) == CS101_SOF_FA);
}


uint8_t
FileDirectory_getLengthOfFile(FileDirectory self)
{
    return self->lengthOfFile;
}

CP56Time2a
FileDirectory_getCreationTime(FileDirectory self)
{
    return &(self->creationTime);
}

void
FileDirectory_destroy(FileDirectory self)
{
    GLOBAL_FREEMEM(self);
}

FileDirectory
FileDirectory_getFromBuffer(FileDirectory self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{

    if (self == NULL)
       self = (FileDirectory) GLOBAL_MALLOC(sizeof(struct sFileDirectory));

    if (self != NULL) {

        FileDirectory_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        self->nof = msg[startIndex++];
        self->nof += (msg[startIndex++] * 0x100);

        self->lengthOfFile = msg [startIndex++];
        self->lengthOfFile += (msg [startIndex++] * 0x100);
        self->lengthOfFile += (msg [startIndex++] * 0x10000);


        self->sof = msg[startIndex++];

        CP56Time2a_getFromBuffer(&(self->creationTime), msg, msgSize, startIndex);
    }

    return self;
}

//远程参数读写
//<200>：= 切换定值区 C_SR_NA_1
//<201>：= 读定值区号 C_RR_NA_1
//<202>：= 读参数和定值 C_RS_NA_1
//<203>：= 写参数和定值 C_WS_NA_1

/*************************************************
 * RemoteReposition: InformationObject  C_SR_NA_1=200
 * 远方切换定值区 signal remote reposition
 *************************************************/
static bool
RemoteReposition_encode(RemoteReposition self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, (uint8_t)(self->SN % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->SN / 0x100) % 0x100));

    return true;
}

struct sInformationObjectVFT RemoteRepositionVFT = {
        (EncodeFunction) RemoteReposition_encode,
        (DestroyFunction) RemoteReposition_destroy
};

static void
RemoteReposition_initialize(RemoteReposition self)
{
    self->virtualFunctionTable = &(RemoteRepositionVFT);
    self->type = C_SR_NA_1;
}

RemoteReposition
RemoteReposition_create(RemoteReposition self, int ioa, uint16_t sn)//切换定值区
{
    if (self == NULL)
        self = (RemoteReposition) GLOBAL_MALLOC(sizeof(struct sRemoteReposition));

    if (self != NULL) {
        RemoteReposition_initialize(self);

        self->objectAddress = ioa;
        self->SN = sn;

    }

    return self;
}

void
RemoteReposition_destroy(RemoteReposition self)
{
    GLOBAL_FREEMEM(self);
}

RemoteReposition
RemoteReposition_getFromBuffer(RemoteReposition self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if (self == NULL)
       self = (RemoteReposition) GLOBAL_MALLOC(sizeof(struct sRemoteReposition));

    if (self != NULL) {

        RemoteReposition_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

        self->SN = msg[startIndex++];
        self->SN += (msg[startIndex++] * 0x100);
    }

    return self;
}

/*************************************************
 * RemoteReadSN: InformationObject  C_RR_NA_1=201
 * 读定值区号 SN
 *************************************************/
static bool
RemoteReadSN_encode(RemoteReposition self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);



    return true;
}

struct sInformationObjectVFT RemoteReadSNVFT = {
        (EncodeFunction) RemoteReadSN_encode,
        (DestroyFunction) RemoteReadSN_destroy
};

static void
RemoteReadSN_initialize(RemoteReadSN self)
{
    self->virtualFunctionTable = &(RemoteReadSNVFT);
    self->type = C_RR_NA_1;
}

RemoteReadSN
RemoteReadSN_create(RemoteReadSN self, int ioa)//读当前定值区号
{
    if (self == NULL)
        self = (RemoteReadSN) GLOBAL_MALLOC(sizeof(struct sRemoteReadSN));

    if (self != NULL) {
        RemoteReadSN_initialize(self);

        self->objectAddress = ioa;

    }

    return self;
}

void
RemoteReadSN_destroy(RemoteReadSN self)
{
    GLOBAL_FREEMEM(self);
}

RemoteReadSN
RemoteReadSN_getFromBuffer(RemoteReadSN self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if (self == NULL)
       self = (RemoteReadSN) GLOBAL_MALLOC(sizeof(struct sRemoteReadSN));

    if (self != NULL) {

        RemoteReadSN_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }

//        信息体地址（0）  3 字节
//        当前定值区区号 SN1  2 字节
//        终端支持的最小定值区号 SN2  2 字节
//        终端支持的最大定值区号 SN3  7 字节(101)    2字节(104/104+)
        self->SN1 = msg[startIndex++];
        self->SN1 += (msg[startIndex++] * 0x100);

        self->SN2 = msg[startIndex++];
        self->SN2 += (msg[startIndex++] * 0x100);

        self->SN3 = msg[startIndex++];//7字节？？
        self->SN3 += (msg[startIndex++] * 0x100);
//        self->SN3 += (msg [startIndex++] * 0x10000);
//        self->SN3 += (msg [startIndex++] * 0x1000000);

    }

    return self;
}

int16_t//int
RemoteReadSN_getSN1(RemoteReadSN self)
{
    return self->SN1;
}

int16_t//int
RemoteReadSN_getSN2(RemoteReadSN self)
{
    return self->SN2;
}

int64_t//long
RemoteReadSN_getSN3(RemoteReadSN self)
{
    return self->SN3;
}

/*************************************************
 * ParamValue_Read: InformationObject  C_RS_NA_1=202
 * 读参数和定值
 *************************************************/
static bool
ParamValue_Read_encode(ParamValue_Read self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;

    //InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

//        Frame_setNextByte(frame, (uint8_t)(self->infoaddr & 0xff));
//        Frame_setNextByte(frame, (uint8_t)((self->infoaddr / 0x100) & 0xff));
//        Frame_setNextByte(frame, (uint8_t)((self->infoaddr / 0x10000) & 0xff));




//    //定值区号SN
//    Frame_setNextByte(frame, (uint8_t) ((self->SN) & 0xff));
//    Frame_setNextByte(frame, (uint8_t) (((self->SN) / 0x100) & 0xff));

    if(self->__sParamRead.paramNums>0&&self->__sParamRead.paramNums<=1024)
    {
        int i=0;
        for(i=0;i<self->__sParamRead.paramNums;i++)
        {
            //固有参数信息体地址从0x8001开始编码至0x801F
            //运行参数信息体地址定义为0x8020至0x821F
            //动作参数信息体地址定义为0x8220至0x85EF
            if(self->infoaddr[i]>=0x8001 && self->infoaddr[i] <= 0xFFFF)//0x85EF
            {
                Frame_setNextByte(frame, (uint8_t)(self->infoaddr[i] & 0xff));
                Frame_setNextByte(frame, (uint8_t)((self->infoaddr[i] / 0x100) & 0xff));
                Frame_setNextByte(frame, (uint8_t)((self->infoaddr[i] / 0x10000) & 0xff));
            }

        }
    }

    return true;
}

struct sInformationObjectVFT ParamValue_ReadVFT = {
        (EncodeFunction) ParamValue_Read_encode,
        (DestroyFunction) ParamValue_Read_destroy
};

static void
ParamValue_Read_initialize(ParamValue_Read self)
{
    self->virtualFunctionTable = &(ParamValue_ReadVFT);
    self->type = C_RS_NA_1;
}

ParamValue_Read
ParamValue_Read_create(ParamValue_Read self, int ioa_index)//int sn, ParamRead paramRead ParamRead读参数和定值int ioa
{
    if (self == NULL)
    {
        self = (ParamValue_Read) GLOBAL_MALLOC(sizeof(struct sParamValue_Read));
        self->__sParamRead.paramNums = 0;
    }

    if (self != NULL) {
        ParamValue_Read_initialize(self);

        //self->objectAddress = ioa_index;
        //self->infoaddr = ioa_index;

        self->infoaddr[self->__sParamRead.paramNums] = ioa_index;
        self->__sParamRead.structParamRead[self->__sParamRead.paramNums].informationAddr = 0;
        self->__sParamRead.structParamRead[self->__sParamRead.paramNums].tagType =0;
        self->__sParamRead.structParamRead[self->__sParamRead.paramNums].datalen=0;

        self->__sParamRead.paramNums++;


//        int i=0;
//        for(i=0;i<1024;i++)//self->__sParamRead->paramNums
//        {
//            self->__sParamRead.structParamRead[i].informationAddr = 0;
//            self->__sParamRead.structParamRead[i].tagType =0;
//            self->__sParamRead.structParamRead[i].datalen=0;
//            //self->__sParamRead->structParamRead[i].datavalue=0;
//        }
//        //self->structParamRead[index].informationAddr = ioa_index;

    }

    return self;
}

void
ParamValue_Read_destroy(ParamValue_Read self)
{
    GLOBAL_FREEMEM(self);
}

ParamValue_Read
ParamValue_Read_getFromBuffer(ParamValue_Read self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence,int nums)
{
    if (self == NULL)
       self = (ParamValue_Read) GLOBAL_MALLOC(sizeof(struct sParamValue_Read));

    if (self != NULL) {

        ParamValue_Read_initialize(self);

//        if (!isSequence) {
//            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

//            startIndex += parameters->sizeOfIOA; /* skip IOA */
//        }

//        定值区号 SN  2 字节
//        参数特征标识  1 字节
//        信息体地址 1  3 字节
//        Tag 类型  1 字节
//        数据长度  1 字节
//        值  由数据长度决定
//        …
        self->SN = msg[startIndex++];
        self->SN += (msg[startIndex++] * 0x100);

        self->paramTI = msg[startIndex++];  //

        self->__sParamRead.paramNums = nums;

        int i=0;
        for(i=0;i<nums;i++)
        {
            self->__sParamRead.structParamRead[i].informationAddr = msg[startIndex++];//3字节
            self->__sParamRead.structParamRead[i].informationAddr += (msg[startIndex++] * 0x100);
            self->__sParamRead.structParamRead[i].informationAddr += (msg [startIndex++] * 0x10000);

            self->__sParamRead.structParamRead[i].tagType = msg[startIndex++];//1字节

            self->__sParamRead.structParamRead[i].datalen = msg[startIndex++];//1字节
            for(int k=0;k<self->__sParamRead.structParamRead[i].datalen;k++)
            {
                self->__sParamRead.structParamRead[i].datavalue[k] = msg[startIndex++];

//                if(self->__sParamRead.structParamRead[i].tagType == 4)//针对字符串类型    String  4  可变
//                {
//                    if(self->__sParamRead.structParamRead[i].datavalue[k] >=128)//若
//                    {
//                        self->__sParamRead.structParamRead[i].datavalue[k]=0;
//                    }
//                }

            }


        }

    }

    return self;
}

int8_t//int
ParamValue_Read_getSN(ParamValue_Read self)
{
    return self->SN;
}

int
ParamValue_Read_getParamReadnum(ParamValue_Read self)
{
    return self->__sParamRead.paramNums;
}
//struct _sParamRead{
//    int informationAddr;//信息体地址  3 字节
//    uint8_t tagType;//Tag 类型  1 字节
//    uint8_t datalen;//数据长度  1 字节
//    char datavalue[128];//数据值  由数据长度决定

//};
int
ParamValue_Read_getParamReadinfoaddr(ParamValue_Read self,int index)
{
    if(index<self->__sParamRead.paramNums)
        return self->__sParamRead.structParamRead[index].informationAddr;
    else
        return -1;
}

int
ParamValue_Read_getParamReadtagtype(ParamValue_Read self,int index)
{
    if(index<self->__sParamRead.paramNums)
        return self->__sParamRead.structParamRead[index].tagType;
    else
        return -1;
}

int
ParamValue_Read_getParamReaddatalen(ParamValue_Read self,int index)
{
    if(index<self->__sParamRead.paramNums)
        return self->__sParamRead.structParamRead[index].datalen;
    else
        return -1;
}

char*
ParamValue_Read_getParamReaddatavalue(ParamValue_Read self,int index)
{
    if(index<self->__sParamRead.paramNums)
        return (char*)(self->__sParamRead.structParamRead[index].datavalue);
    else
        return NULL;
}

/*************************************************
 * ParamValue_Write: InformationObject  C_WS_NA_1=203
 * 写参数和定值
 *************************************************/
static bool
ParamValue_Write_encode(ParamValue_Write self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    int size = isSequence ? 1 : (parameters->sizeOfIOA + 1);

    if (Frame_getSpaceLeft(frame) < size)
        return false;


    if(self->paramNums>0&&self->paramNums<=1024)
    {
        int i=0;
        for(i=0;i<self->paramNums;i++)
        {
            //固有参数信息体地址从0x8001开始编码至0x801F
            //运行参数信息体地址定义为0x8020至0x821F
            //动作参数信息体地址定义为0x8220至0x85EF
            if(self->structParamWrite[i].informationAddr>=0x8001 && self->structParamWrite[i].informationAddr <= 0xFFFF)//0x85EF
            {
                //信息体地址 1  3 字节
                Frame_setNextByte(frame, (uint8_t)(self->structParamWrite[i].informationAddr & 0xff));
                Frame_setNextByte(frame, (uint8_t)((self->structParamWrite[i].informationAddr / 0x100) & 0xff));
                Frame_setNextByte(frame, (uint8_t)((self->structParamWrite[i].informationAddr / 0x10000) & 0xff));
                //Tag 类型  1 字节
                Frame_setNextByte(frame,(uint8_t)(self->structParamWrite[i].tagType));
                //数据长度  1 字节
                //值  由数据长度决定
                int temp_datalen = (uint8_t)(self->structParamWrite[i].datalen);
                Frame_setNextByte(frame,temp_datalen);
                for(int j=0;j<temp_datalen;j++)
                {
                    Frame_setNextByte(frame,(uint8_t)(self->structParamWrite[i].datavalue[j]));
                }
            }

        }
    }

    return true;
}

struct sInformationObjectVFT ParamValue_WriteVFT = {
        (EncodeFunction) ParamValue_Write_encode,
        (DestroyFunction) ParamValue_Write_destroy
};

static void
ParamValue_Write_initialize(ParamValue_Write self)
{
    self->virtualFunctionTable = &(ParamValue_WriteVFT);
    self->type = C_WS_NA_1;
}

ParamValue_Write
ParamValue_Write_create(ParamValue_Write self, int ioa_index,int tag_index,int datalen,unsigned char* datavalue)//int sn, ParamRead paramRead ParamRead读参数和定值int ioa
{
    if (self == NULL)
    {
        self = (ParamValue_Write) GLOBAL_MALLOC(sizeof(struct sParamValue_Write));
        self->paramNums = 0;
    }

    if (self != NULL) {
        ParamValue_Write_initialize(self);

        //self->objectAddress = ioa_index;
        //self->infoaddr = ioa_index;

        if(ioa_index>=0x8001 && ioa_index<=0xFFFF)//0x85EF
        {
            self->structParamWrite[self->paramNums].informationAddr = ioa_index;
            self->structParamWrite[self->paramNums].tagType =tag_index;
            self->structParamWrite[self->paramNums].datalen=datalen;

            for(int k=0;k<datalen;k++)
            {
                self->structParamWrite[self->paramNums].datavalue[k]=datavalue[k];
            }

            self->paramNums++;
        }

    }

    return self;
}

void
ParamValue_Write_destroy(ParamValue_Write self)
{
    GLOBAL_FREEMEM(self);
}

ParamValue_Write
ParamValue_Write_getFromBuffer(ParamValue_Write self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence,int nums)
{
    if (self == NULL)
       self = (ParamValue_Write) GLOBAL_MALLOC(sizeof(struct sParamValue_Write));

    if (self != NULL) {

        ParamValue_Write_initialize(self);

//        if (!isSequence) {
//            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

//            startIndex += parameters->sizeOfIOA; /* skip IOA */
//        }

//        定值区号 SN  2 字节
//        参数特征标识  1 字节
//        信息体地址 1  3 字节
//        Tag 类型  1 字节
//        数据长度  1 字节
//        值  由数据长度决定
//        …
        self->SN = msg[startIndex++];
        self->SN += (msg[startIndex++] * 0x100);

        self->paramTI = msg[startIndex++];  //

        self->paramNums = nums;

        int i=0;
        for(i=0;i<nums;i++)
        {
            self->structParamWrite[i].informationAddr = msg[startIndex++];//3字节
            self->structParamWrite[i].informationAddr += (msg[startIndex++] * 0x100);
            self->structParamWrite[i].informationAddr += (msg [startIndex++] * 0x10000);

            self->structParamWrite[i].tagType = msg[startIndex++];//1字节

            self->structParamWrite[i].datalen = msg[startIndex++];//1字节
            for(int k=0;k<self->structParamWrite[i].datalen;k++)
            {
                self->structParamWrite[i].datavalue[k] = msg[startIndex++];
            }


        }

    }

    return self;
}

unsigned char
ParamValue_Write_getParamTI(ParamValue_Write self)
{
    return self->paramTI;
}

unsigned char
ParamValue_Write_getSN(ParamValue_Write self)
{
    return self->SN;
}

unsigned char
ParamValue_Write_getParamWritenum(ParamValue_Write self)
{
    return self->paramNums;
}

int
ParamValue_Write_getParamWriteinfoaddr(ParamValue_Write self,int index)
{
    if(index<self->paramNums)
        return self->structParamWrite[index].informationAddr;
    else
        return -1;
}

unsigned char
ParamValue_Write_getParamWritetagtype(ParamValue_Write self,int index)
{
    if(index<self->paramNums)
        return self->structParamWrite[index].tagType;
    else
        return -1;
}

unsigned char
ParamValue_Write_getParamWritedatalen(ParamValue_Write self,int index)
{
    if(index<self->paramNums)
        return self->structParamWrite[index].datalen;
    else
        return -1;
}

char*
ParamValue_Write_getParamWritedatavalue(ParamValue_Write self,int index)
{
    if(index<self->paramNums)
        return (char*)(self->structParamWrite[index].datavalue);
    else
        return NULL;
}


//TI=210:文件服务
/*************************************************
 * FileServer(selfdefine) : InformationObject
 *************************************************/

//========== <210>	：文件服务 ==========//
/*************************************************
 * FileCallMenu : InformationObject 召目录
 *************************************************/
static bool
FileCallMenu_encode(FileCallMenu self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

    //uint8_t operateType;           //1字节:操作标识  1：读目录
    //uint32_t catalogueID;          //4字节：目录ID
    //uint8_t catalogueNamelength;   //1字节：目录名长度
    //uint8_t* catalogueName;        //x字节：目录名
    //uint8_t callflag;              //1字节：召唤标志(0：目录下所有文件；1：目录下满足搜索时间段的文件)
    //struct sCP56Time2a startTime;  //7字节：查询起始时间
    //struct sCP56Time2a endTime;    //7字节：查询终止时间
    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, (uint8_t)(self->catalogueID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->catalogueID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->catalogueID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->catalogueID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, self->catalogueNamelength);

    //Frame_appendBytes(frame, self->catalogueName, self->catalogueNamelength);
    for(int i=0;i<self->catalogueNamelength;i++)
    {
       Frame_setNextByte (frame, (uint8_t)(self->catalogueName[i]));
    }

    Frame_setNextByte (frame, self->callflag);

    Frame_appendBytes(frame, self->startTime.encodedValue, 7);
    Frame_appendBytes(frame, self->endTime.encodedValue, 7);

    return true;
}

struct sInformationObjectVFT FileCallMenuVFT = {
        (EncodeFunction) FileCallMenu_encode,
        (DestroyFunction) FileCallMenu_destroy
};

static void
FileCallMenu_initialize(FileCallMenu self)
{
    self->virtualFunctionTable = &(FileCallMenuVFT);
    self->type = F_FR_NA_1;
}

FileCallMenu FileCallMenu_create(FileCallMenu self, int ioa, uint8_t operateType, uint32_t catalogueID, uint8_t catalogueNamelength, char* catalogueName, uint8_t callflag, CP56Time2a startTime, CP56Time2a endTime)
{
    if (self == NULL)
        self = (FileCallMenu) GLOBAL_MALLOC(sizeof(struct sFileCallMenu));

    if (self != NULL) {
        FileCallMenu_initialize(self);

        self->objectAddress = ioa;
        self->operateType = operateType;
        self->catalogueID = catalogueID;
        self->catalogueNamelength = catalogueNamelength;
        self->catalogueName = catalogueName;
        self->callflag = callflag;
        self->startTime = *startTime;
        self->endTime = *endTime;
    }

    return self;
}

FileCallMenu
FileCallMenu_getFromBuffer(FileCallMenu self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{

    if (self == NULL)
       self = (FileCallMenu) GLOBAL_MALLOC(sizeof(struct sFileCallMenu));

    if (self != NULL) {

        FileCallMenu_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }


        self->operateType = msg[startIndex++];
        //
        self->catalogueID = msg [startIndex++];
        self->catalogueID += (msg [startIndex++] * 0x100);
        self->catalogueID += (msg [startIndex++] * 0x10000);
        self->catalogueID += (msg [startIndex++] * 0x1000000);
        //
        self->catalogueNamelength = msg[startIndex++];
        //
        for(int i=0;i<self->catalogueNamelength;i++)
        {
            self->catalogueName[i]=msg[startIndex++];
        }
        //
        self->callflag = msg[startIndex++];
        //
        CP56Time2a_getFromBuffer(&(self->startTime), msg, msgSize, startIndex);
        CP56Time2a_getFromBuffer(&(self->endTime), msg, msgSize, startIndex);
    }

    return self;
}

void FileCallMenu_destroy(FileCallMenu self)
{
    GLOBAL_FREEMEM(self);
}












/*************************************************
 * FileCallMenuAffirm : InformationObject 召目录激活确认
 *************************************************/
//FileCallMenuAffirm
static bool
FileCallMenuAffirm_encode(FileCallMenuAffirm self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

//    uint8_t operateType;           //1字节:操作标识  2：读目录(目录召唤确认)
//    uint8_t resultDescribe;        //1字节:结果描述字 0-成功 1-失败
//    uint32_t catalogueID;          //4字节：目录ID
//    uint8_t followupFlag;          //1字节:后续标志,0：无后续,1：有后续
//    uint8_t file_num;              //1字节：本帧文件数量(通常1帧数据5个目录名称)
    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, self->resultDescribe);

    Frame_setNextByte (frame, (uint8_t)(self->catalogueID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->catalogueID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->catalogueID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->catalogueID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, self->followupFlag);

    Frame_setNextByte (frame, self->file_num);

    for(int i=0;i<self->file_num;i++)//目录数据组包
    {
        Frame_setNextByte (frame, self->structMenu[i].fileNamelength);
        Frame_appendBytes(frame, (uint8_t*)(&(self->structMenu[i].fileName)), self->structMenu[i].fileNamelength);
        Frame_setNextByte (frame, self->structMenu[i].fileProperty);
        Frame_setNextByte (frame, (uint8_t)(self->structMenu[i].fileSize % 0x100));
        Frame_setNextByte (frame, (uint8_t)((self->structMenu[i].fileSize / 0x100) % 0x100));
        Frame_setNextByte (frame, (uint8_t)((self->structMenu[i].fileSize / 0x10000) % 0x100));
        Frame_setNextByte (frame, (uint8_t)((self->structMenu[i].fileSize / 0x1000000) % 0x100));
        Frame_appendBytes(frame, self->structMenu[i].fileTime.encodedValue, 7);
    }

    return true;
}

struct sInformationObjectVFT FileCallMenuAffirmVFT = {
        (EncodeFunction) FileCallMenuAffirm_encode,
        (DestroyFunction) FileCallMenuAffirm_destroy
};

static void
FileCallMenuAffirm_initialize(FileCallMenuAffirm self)
{
    self->virtualFunctionTable = &(FileCallMenuAffirmVFT);
    self->type = F_FR_NA_1;
}



FileCallMenuAffirm
FileCallMenuAffirm_getFromBuffer(FileCallMenuAffirm self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if (self == NULL)
       self = (FileCallMenuAffirm) GLOBAL_MALLOC(sizeof(struct sFileCallMenuAffirm));

    if (self != NULL) {

        FileCallMenuAffirm_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }
        startIndex++;//附加数据包类型 2：文件传输；
        //操作标识   2：读目录确认
        self->operateType = msg[startIndex++];
        //结果描述字
        self->resultDescribe = msg[startIndex++];
        //目录 ID
        self->catalogueID = msg [startIndex++];
        self->catalogueID += (msg [startIndex++] * 0x100);
        self->catalogueID += (msg [startIndex++] * 0x10000);
        self->catalogueID += (msg [startIndex++] * 0x1000000);
        //后续标志  0：无后续,1：有后续
        self->followupFlag = msg [startIndex++];
        //本帧文件数量
        self->file_num = msg [startIndex++];

        for(int i=0;i<self->file_num;i++)//目录数据
        {

            //文件 i 名称长度
            self->structMenu[i].fileNamelength = msg [startIndex++];
            //文件 i 名称
            //self->structMenu[i].fileName={0};//先清空
            //memset(&(self->structMenu[i].fileName),0,256);
            //self->structMenu[i].fileName[0]='\0';

//            for(int len=0;len<self->structMenu[i].fileNamelength;len++)
//            {
//                self->structMenu[i].fileName[len]=msg [startIndex++];
//            }
            for(int len=0;len<256;len++)
            {
                if(len<self->structMenu[i].fileNamelength)
                    self->structMenu[i].fileName[len]=msg [startIndex++];
                else
                    self->structMenu[i].fileName[len]='\0';
            }


            //文件 i 属性
            self->structMenu[i].fileProperty=msg [startIndex++];
            //文件 i 大小
            self->structMenu[i].fileSize = msg [startIndex++];
            self->structMenu[i].fileSize += (msg [startIndex++] * 0x100);
            self->structMenu[i].fileSize += (msg [startIndex++] * 0x10000);
            self->structMenu[i].fileSize += (msg [startIndex++] * 0x1000000);
            //文件 i 时间
            CP56Time2a_getFromBuffer(&(self->structMenu[i].fileTime), msg, msgSize, startIndex);
            startIndex +=7;
        }

    }

    return self;
}


int
FileCallMenuAffirm_getOperateType(FileCallMenuAffirm self)
{
    return self->operateType;
}


int
FileCallMenuAffirm_getResultDescribe(FileCallMenuAffirm self)
{
    return self->resultDescribe;
}

//catalogueID
int
FileCallMenuAffirm_getCatalogueID(FileCallMenuAffirm self)
{
    return self->catalogueID;
}

int
FileCallMenuAffirm_getFilenum(FileCallMenuAffirm self)
{
    return self->file_num;
}

int
FileCallMenuAffirm_getFollowupFlag(FileCallMenuAffirm self)
{
    return self->followupFlag;
}

int
FileCallMenuAffirm_getFilenamelen(FileCallMenuAffirm self,int index)
{
    return self->structMenu[index].fileNamelength;
}

//fileProperty
int
FileCallMenuAffirm_getFileProperty(FileCallMenuAffirm self,int index)
{
    return self->structMenu[index].fileProperty;
}

//fileSize
int
FileCallMenuAffirm_getFileSize(FileCallMenuAffirm self,int index)
{
    return self->structMenu[index].fileSize;
}

//fileTime
CP56Time2a
FileCallMenuAffirm_getFileTime(FileCallMenuAffirm self,int index)
{
    return &(self->structMenu[index].fileTime);
}

char* FileCallMenuAffirm_getFilename(FileCallMenuAffirm self,int index)
{
    if(index >= self->file_num)
        return NULL;

    return (char*)(self->structMenu[index].fileName);
}


void FileCallMenuAffirm_destroy(FileCallMenuAffirm self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}

/*************************************************
 * FileActivate : InformationObject 读文件激活
 *************************************************/
static bool
FileActivate_encode(FileActivate self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

//    uint8_t operateType;           //1字节:操作标识  3：读文件激活
//    uint8_t fileNamelength;        //1字节：文件长度
//    char* fileName;             //x字节：文件名

    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, self->fileNamelength);

    //Frame_appendBytes(frame, self->catalogueName, self->catalogueNamelength);
    for(int i=0;i<self->fileNamelength;i++)
    {
       Frame_setNextByte (frame, (uint8_t)(self->fileName[i]));
    }


    return true;
}

struct sInformationObjectVFT FileActivateVFT = {
        (EncodeFunction) FileActivate_encode,
        (DestroyFunction) FileActivate_destroy
};

static void
FileActivate_initialize(FileActivate self)
{
    self->virtualFunctionTable = &(FileActivateVFT);
    self->type = F_FR_NA_1;
}

FileActivate FileActivate_create(FileActivate self, int ioa, uint8_t operateType, uint8_t fileNamelength, char* fileName)
{
    if (self == NULL)
        self = (FileActivate) GLOBAL_MALLOC(sizeof(struct sFileActivate));

    if (self != NULL) {
        FileActivate_initialize(self);

        self->objectAddress = ioa;
        self->operateType = operateType;

        self->fileNamelength = fileNamelength;
        self->fileName = fileName;
    }

    return self;
}

void FileActivate_destroy(FileActivate self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}



/*************************************************
 * FileActivateAffirm : InformationObject 读文件激活确认
 *************************************************/
//FileActivateAffirm
static bool
FileActivateAffirm_encode(FileActivateAffirm self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

//    uint8_t operateType;           //1字节:操作标识  4：读文件激活确认
//    uint8_t resultDescribe;        //1字节:结果描述字 0-成功 1-失败
//    uint8_t fileNamelength;        //1字节：文件长度
//    char* fileName;             //x字节：文件名
//    uint32_t fileID;               //4字节：文件ID
//    uint32_t fileSize;             //4字节：文件大小
    Frame_setNextByte (frame, self->operateType);//4：读文件激活确认

    Frame_setNextByte (frame, self->resultDescribe);

    Frame_setNextByte (frame, self->fileNamelength);
    Frame_appendBytes(frame, (uint8_t*)(&(self->fileName)), self->fileNamelength);

    Frame_setNextByte (frame, (uint8_t)(self->fileID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, (uint8_t)(self->fileSize % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x1000000) % 0x100));



    return true;
}

struct sInformationObjectVFT FileActivateAffirmVFT = {
        (EncodeFunction) FileActivateAffirm_encode,
        (DestroyFunction) FileActivateAffirm_destroy
};

static void
FileActivateAffirm_initialize(FileActivateAffirm self)
{
    self->virtualFunctionTable = &(FileActivateAffirmVFT);
    self->type = F_FR_NA_1;
}



FileActivateAffirm
FileActivateAffirm_getFromBuffer(FileActivateAffirm self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if (self == NULL)
       self = (FileActivateAffirm) GLOBAL_MALLOC(sizeof(struct sFileActivateAffirm));

    if (self != NULL) {

        FileActivateAffirm_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }
        startIndex++;//附加数据包类型 2：文件传输；
        //    uint8_t operateType;           //1字节:操作标识  4：读文件激活确认
        //    uint8_t resultDescribe;        //1字节:结果描述字 0-成功 1-失败
        //    uint8_t fileNamelength;        //1字节：文件长度
        //    char* fileName;             //x字节：文件名
        //    uint32_t fileID;               //4字节：文件ID
        //    uint32_t fileSize;             //4字节：文件大小
        //操作标识   2：读目录确认
        self->operateType = msg[startIndex++];
        //结果描述字
        self->resultDescribe = msg[startIndex++];
        //文件名长度
        self->fileNamelength = msg [startIndex++];
        //文件名称
        for(int len=0;len<256;len++)
        {
            if(len<self->fileNamelength)
                self->fileName[len]=msg [startIndex++];
            else
                self->fileName[len]='\0';
        }
        //文件ID
        self->fileID = msg [startIndex++];
        self->fileID += (msg [startIndex++] * 0x100);
        self->fileID += (msg [startIndex++] * 0x10000);
        self->fileID += (msg [startIndex++] * 0x1000000);
        //文件大小
        self->fileSize = msg [startIndex++];
        self->fileSize += (msg [startIndex++] * 0x100);
        self->fileSize += (msg [startIndex++] * 0x10000);
        self->fileSize += (msg [startIndex++] * 0x1000000);

    }



    return self;
}

int
FileActivateAffirm_getOperateType(FileActivateAffirm self)
{
    return self->operateType;
}

int
FileActivateAffirm_getResultDescribe(FileActivateAffirm self)
{
    return self->resultDescribe;
}

int
FileActivateAffirm_getFilenamelen(FileActivateAffirm self)
{
    return self->fileNamelength;
}

char* FileActivateAffirm_getFilename(FileActivateAffirm self)
{
    return (char*)(self->fileName);
}

int
FileActivateAffirm_getFileID(FileActivateAffirm self)
{
    return self->fileID;
}

int
FileActivateAffirm_getFilesize(FileActivateAffirm self)
{
    return self->fileSize;
}

void FileActivateAffirm_destroy(FileActivateAffirm self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}

/*************************************************
 * FileTransfer : InformationObject 读文件数据传输
 *************************************************/
//FileTransfer
static bool
FileTransfer_encode(FileTransfer self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, (uint8_t)(self->fileID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, (uint8_t)(self->segmentnumber % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x1000000) % 0x100));

    Frame_setNextByte (frame, self->followupFlag);

    //fileData
    for(int len=0;len<256;len++)
    {
        if(self->fileData[len]!='\0'&&Frame_getMsgSize(frame)<255)
        {
            Frame_setNextByte (frame, self->fileData[len]);
        }
        else
            break;

    }

    Frame_setNextByte (frame, self->fileCheckSum);

    return true;
}

struct sInformationObjectVFT FileTransferVFT = {
        (EncodeFunction) FileTransfer_encode,
        (DestroyFunction) FileTransfer_destroy
};

static void
FileTransfer_initialize(FileTransfer self)
{
    self->virtualFunctionTable = &(FileTransferVFT);
    self->type = F_FR_NA_1;
}



FileTransfer
FileTransfer_getFromBuffer(FileTransfer self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{

    if (self == NULL)
       self = (FileTransfer) GLOBAL_MALLOC(sizeof(struct sFileTransfer));

    if (self != NULL) {

        FileTransfer_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }
        startIndex++;//附加数据包类型 2：文件传输；
//        uint8_t operateType;           //1字节:操作标识  5：读文件数据
//        uint32_t fileID;               //4字节：文件ID
//        uint32_t segmentnumber;        //4字节：数据段号,可以使用文件内容的偏移指针值
//        uint8_t followupFlag;          //1字节:后续标志,0：无后续,1：有后续
//        //char* fileData;                //x字节：文件数据
//        char fileData[256];
//        uint8_t fileCheckSum;          //1字节:校验码(校验范围：文件数据 校验算法：单字节模和运算)
        //操作标识   5 读文件数据传输
        self->operateType = msg[startIndex++];

        //文件ID
        self->fileID = msg [startIndex++];
        self->fileID += (msg [startIndex++] * 0x100);
        self->fileID += (msg [startIndex++] * 0x10000);
        self->fileID += (msg [startIndex++] * 0x1000000);
        //数据段号
        self->segmentnumber = msg [startIndex++];
        self->segmentnumber += (msg [startIndex++] * 0x100);
        self->segmentnumber += (msg [startIndex++] * 0x10000);
        self->segmentnumber += (msg [startIndex++] * 0x1000000);

        //后续标志
        self->followupFlag = msg[startIndex++];
        //x字节：文件数据
        //1字节:校验码
        //if (msgSize < startIndex + 7)
        int temp_filesize = msgSize - startIndex-1;
        for(int len=0;len<256*4*2;len++)
        {
            if(len<temp_filesize)
                self->fileData[len]=msg [startIndex++];
            else
                self->fileData[len]='\0';
        }
        self->fileCheckSum = msg[startIndex++];
        self->file_currentlen = temp_filesize;

    }

    return self;
}

int
FileTransfer_getOperateType(FileTransfer self)
{
    return self->operateType;
}

int
FileTransfer_getCurrentfiledataLen(FileTransfer self)
{
    return self->file_currentlen;
}

int
FileTransfer_getFileID(FileTransfer self)
{
    return self->fileID;
}

int
FileTransfer_getSegmentnumber(FileTransfer self)
{
    return self->segmentnumber;
}

int
FileTransfer_getFollowupFlag(FileTransfer self)
{
    return self->followupFlag;
}

char* FileTransfer_getFileData(FileTransfer self)
{
    return (char*)self->fileData;
}

int
FileTransfer_getFileCheckSum(FileTransfer self)
{
    return self->fileCheckSum;
}

void FileTransfer_destroy(FileTransfer self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}


/*************************************************
 * FileTransferAffirm : InformationObject 读文件数据传输确认
 *************************************************/
static bool
FileTransferAffirm_encode(FileTransferAffirm self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

//    uint8_t operateType;           //1字节:操作标识  6：读文件数据响应
//    uint32_t fileID;               //4字节：文件ID
//    uint32_t segmentnumber;        //4字节：数据段号,可以使用文件内容的偏移指针值
//    uint8_t followupFlag;          //1字节:后续标志,0：无后续,1：有后续

    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, (uint8_t)(self->fileID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, (uint8_t)(self->segmentnumber % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x1000000) % 0x100));

    Frame_setNextByte (frame, self->followupFlag);

    return true;
}

struct sInformationObjectVFT FileTransferAffirmVFT = {
        (EncodeFunction) FileTransferAffirm_encode,
        (DestroyFunction) FileTransferAffirm_destroy
};

static void
FileTransferAffirm_initialize(FileTransferAffirm self)
{
    self->virtualFunctionTable = &(FileTransferAffirmVFT);
    self->type = F_FR_NA_1;
}

FileTransferAffirm FileTransferAffirm_create(FileTransferAffirm self, int ioa, uint8_t operateType, uint32_t fileID, uint32_t segmentnumber, uint8_t followupFlag)
{
    if (self == NULL)
        self = (FileTransferAffirm) GLOBAL_MALLOC(sizeof(struct sFileTransferAffirm));

    if (self != NULL) {
        FileTransferAffirm_initialize(self);

        self->objectAddress = ioa;
        self->operateType = operateType;

        self->fileID = fileID;
        self->segmentnumber = segmentnumber;
        self->followupFlag = followupFlag;
    }

    return self;
}

void FileTransferAffirm_destroy(FileTransferAffirm self)
{
    GLOBAL_FREEMEM(self);
}


/*************************************************
 * FileActivateWrite : InformationObject 写文件激活
 *************************************************/
static bool
FileActivateWrite_encode(FileActivateWrite self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

//    uint8_t operateType;           //1字节:操作标识  3：读文件激活
//    uint8_t fileNamelength;        //1字节：文件长度
//    char* fileName;             //x字节：文件名
//    uint32_t fileID;               //4字节：文件ID
//    uint32_t fileSize;             //4字节：文件大小

    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, self->fileNamelength);

    for(int i=0;i<self->fileNamelength;i++)
    {
       Frame_setNextByte (frame, (uint8_t)(self->fileName[i]));
    }

    Frame_setNextByte (frame, (uint8_t)(self->fileID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, (uint8_t)(self->fileSize % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x1000000) % 0x100));

    return true;
}

struct sInformationObjectVFT FileActivateWriteVFT = {
        (EncodeFunction) FileActivateWrite_encode,
        (DestroyFunction) FileActivateWrite_destroy
};

static void
FileActivateWrite_initialize(FileActivateWrite self)
{
    self->virtualFunctionTable = &(FileActivateWriteVFT);
    self->type = F_FR_NA_1;
}

//FileActivateWrite FileActivateWrite_create(FileActivateWrite self, int ioa, uint8_t operateType, uint8_t fileNamelength, char* fileName,uint32_t fileID, uint32_t fileSize);
FileActivateWrite FileActivateWrite_create(FileActivateWrite self, int ioa, uint8_t operateType, uint8_t fileNamelength, char* fileName,uint32_t fileID, uint32_t fileSize)
{
    if (self == NULL)
        self = (FileActivateWrite) GLOBAL_MALLOC(sizeof(struct sFileActivateWrite));

    if (self != NULL) {
        FileActivateWrite_initialize(self);

        self->objectAddress = ioa;
        self->operateType = operateType;

        self->fileNamelength = fileNamelength;
        self->fileName = fileName;
        self->fileID = fileID;
        self->fileSize = fileSize;
    }

    return self;
}

void FileActivateWrite_destroy(FileActivateWrite self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}


/*************************************************
 * FileActivateAffirmWrite : InformationObject 写文件激活确认
 *************************************************/
//FileActivateAffirmWrite
static bool
FileActivateAffirmWrite_encode(FileActivateAffirmWrite self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

//    uint8_t operateType;           //1字节:操作标识  4：读文件激活确认
//    uint8_t resultDescribe;        //1字节:结果描述字 0-成功 1-失败
//    uint8_t fileNamelength;        //1字节：文件长度
//    char* fileName;             //x字节：文件名
//    uint32_t fileID;               //4字节：文件ID
//    uint32_t fileSize;             //4字节：文件大小
    Frame_setNextByte (frame, self->operateType);//4：读文件激活确认

    Frame_setNextByte (frame, self->resultDescribe);

    Frame_setNextByte (frame, self->fileNamelength);
    Frame_appendBytes(frame, (uint8_t*)(&(self->fileName)), self->fileNamelength);

    Frame_setNextByte (frame, (uint8_t)(self->fileID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, (uint8_t)(self->fileSize % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileSize / 0x1000000) % 0x100));



    return true;
}

struct sInformationObjectVFT FileActivateAffirmWriteVFT = {
        (EncodeFunction) FileActivateAffirmWrite_encode,
        (DestroyFunction) FileActivateAffirmWrite_destroy
};

static void
FileActivateAffirmWrite_initialize(FileActivateAffirmWrite self)
{
    self->virtualFunctionTable = &(FileActivateAffirmWriteVFT);
    self->type = F_FR_NA_1;
}


FileActivateAffirmWrite
FileActivateAffirmWrite_getFromBuffer(FileActivateAffirmWrite self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if (self == NULL)
       self = (FileActivateAffirmWrite) GLOBAL_MALLOC(sizeof(struct sFileActivateAffirmWrite));

    if (self != NULL) {

        FileActivateAffirmWrite_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }
        startIndex++;//附加数据包类型 2：文件传输；
        //    uint8_t operateType;           //1字节:操作标识  8：写文件激活确认
        //    uint8_t resultDescribe;        //1字节:结果描述字 0：成功 1：未知错误 2. 文件名不支持 3：长度超范围
        //    uint8_t fileNamelength;        //1字节：文件长度
        //    char* fileName;                //x字节：文件名
        //    uint32_t fileID;               //4字节：文件ID
        //    uint32_t fileSize;             //4字节：文件大小
        //操作标识   8：写文件激活确认
        self->operateType = msg[startIndex++];
        //结果描述字
        self->resultDescribe = msg[startIndex++];
        //文件名长度
        self->fileNamelength = msg [startIndex++];
        //文件名称
        char fileName_temp[256];
        for(int len=0;len<256;len++)
        {
            if(len<self->fileNamelength)
            {
                //self->fileName[len]=msg [startIndex++];
                fileName_temp[len]=msg [startIndex++];
            }
            else
            {
                //self->fileName[len]='\0';
                fileName_temp[len]='\0';
            }
        }
        self->fileName = (char*)fileName_temp;
        //文件ID
        self->fileID = msg [startIndex++];
        self->fileID += (msg [startIndex++] * 0x100);
        self->fileID += (msg [startIndex++] * 0x10000);
        self->fileID += (msg [startIndex++] * 0x1000000);
        //文件大小
        self->fileSize = msg [startIndex++];
        self->fileSize += (msg [startIndex++] * 0x100);
        self->fileSize += (msg [startIndex++] * 0x10000);
        self->fileSize += (msg [startIndex++] * 0x1000000);

    }



    return self;
}

int
FileActivateAffirmWrite_getOperateType(FileActivateAffirmWrite self)
{
    return self->operateType;
}

int
FileActivateAffirmWrite_getResultDescribe(FileActivateAffirmWrite self)
{
    return self->resultDescribe;
}

int
FileActivateAffirmWrite_getFilenamelen(FileActivateAffirmWrite self)
{
    return self->fileNamelength;
}

char* FileActivateAffirmWrite_getFilename(FileActivateAffirmWrite self)
{
    return (char*)(self->fileName);
}

unsigned int
FileActivateAffirmWrite_getFileID(FileActivateAffirmWrite self)
{
    return self->fileID;
}

unsigned int
FileActivateAffirmWrite_getFilesize(FileActivateAffirmWrite self)
{
    return self->fileSize;
}

void FileActivateAffirmWrite_destroy(FileActivateAffirmWrite self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}


/*************************************************
 * FileTransferWrite : InformationObject 写文件数据传输
 *************************************************/
//FileTransfer
static bool
FileTransferWrite_encode(FileTransferWrite self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, (uint8_t)(self->fileID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, (uint8_t)(self->segmentnumber % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x1000000) % 0x100));

    Frame_setNextByte (frame, self->followupFlag);

    //fileData
//    for(int len=0;len<256;len++)
//    {
//        if(self->fileData[len]!='\0')//&&Frame_getMsgSize(frame)<255
//        {
//            Frame_setNextByte (frame, self->fileData[len]);
//        }
//        else
//            break;

//    }
    for(int i=0;i<self->fileDatasize;i++)
    {
        Frame_setNextByte (frame, self->fileData[i]);
    }

    Frame_setNextByte (frame, self->fileCheckSum);

    //================for debug
    int sum = 0;
    for(int i=0;i<self->fileDatasize;i++)//fileData.size()
    {
        sum += self->fileData[i];
    }
    uint8_t checkSum = sum&0xFF;

    if(self->fileCheckSum != checkSum)
    {
        DEBUG_PRINT("DEBUG_LIB60870:[FileTransferWrite_encode]checksum error");
    }

    return true;
}

struct sInformationObjectVFT FileTransferWriteVFT = {
        (EncodeFunction) FileTransferWrite_encode,
        (DestroyFunction) FileTransferWrite_destroy
};

static void
FileTransferWrite_initialize(FileTransferWrite self)
{
    self->virtualFunctionTable = &(FileTransferWriteVFT);
    self->type = F_FR_NA_1;
}

FileTransferWrite FileTransferWrite_create(FileTransferWrite self, int ioa, uint8_t operateType, uint32_t fileID, uint32_t segmentnumber, uint8_t followupFlag,char* fileData,int fileDatasize,uint8_t fileCheckSum)
{
    if (self == NULL)
        self = (FileTransferWrite) GLOBAL_MALLOC(sizeof(struct sFileTransferWrite));

    if (self != NULL) {
        FileTransferWrite_initialize(self);

        self->objectAddress = ioa;
        self->operateType = operateType;

        self->fileID = fileID;
        self->segmentnumber = segmentnumber;
        self->followupFlag = followupFlag;
        self->fileData = fileData;
        self->fileDatasize = fileDatasize;
//        for(int i=0;i<self->fileDatasize;i++)
//        {
//           self->fileData[i] =  fileData[i];
//        }

        self->fileCheckSum = fileCheckSum;

        //================for debug
        int sum = 0;
        for(int i=0;i<fileDatasize;i++)//fileData.size()
        {
            sum += fileData[i];
        }
        uint8_t checkSum = sum&0xFF;

        if(fileCheckSum != checkSum)
        {
            DEBUG_PRINT("DEBUG_LIB60870:[FileTransferWrite_create]checksum error");
        }

    }

    return self;
}


void FileTransferWrite_destroy(FileTransferWrite self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}


/*************************************************
 * FileTransferAffirm : InformationObject 写文件数据传输确认
 *************************************************/
static bool
FileTransferAffirmWrite_encode(FileTransferAffirmWrite self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence)
{
    InformationObject_encodeBase((InformationObject) self, frame, parameters, isSequence);

    Frame_setNextByte (frame, 2);//附加数据包类型 1-备用  2-文件传输  3-备用  4-备用

//    uint8_t operateType;           //1字节:操作标识  6：读文件数据响应
//    uint32_t fileID;               //4字节：文件ID
//    uint32_t segmentnumber;        //4字节：数据段号,可以使用文件内容的偏移指针值
//    uint8_t followupFlag;          //1字节:后续标志,0：无后续,1：有后续

    Frame_setNextByte (frame, self->operateType);

    Frame_setNextByte (frame, (uint8_t)(self->fileID % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->fileID / 0x1000000) % 0x100));

    Frame_setNextByte (frame, (uint8_t)(self->segmentnumber % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x100) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x10000) % 0x100));
    Frame_setNextByte (frame, (uint8_t)((self->segmentnumber / 0x1000000) % 0x100));

    Frame_setNextByte (frame, self->resultDescribe);

    return true;
}

struct sInformationObjectVFT FileTransferAffirmWriteVFT = {
        (EncodeFunction) FileTransferAffirmWrite_encode,
        (DestroyFunction) FileTransferAffirmWrite_destroy
};

static void
FileTransferAffirmWrite_initialize(FileTransferAffirmWrite self)
{
    self->virtualFunctionTable = &(FileTransferAffirmWriteVFT);
    self->type = F_FR_NA_1;
}

FileTransferAffirmWrite
FileTransferAffirmWrite_getFromBuffer(FileTransferAffirmWrite self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence)
{
    if (self == NULL)
       self = (FileTransferAffirmWrite) GLOBAL_MALLOC(sizeof(struct sFileTransferAffirmWrite));

    if (self != NULL) {

        FileTransferAffirmWrite_initialize(self);

        if (!isSequence) {
            InformationObject_getFromBuffer((InformationObject) self, parameters, msg, startIndex);

            startIndex += parameters->sizeOfIOA; /* skip IOA */
        }
        startIndex++;//附加数据包类型 2：文件传输；
        //    uint8_t operateType;           //1字节:操作标识  4：读文件激活确认
        //    uint32_t fileID;               //4字节：文件ID
        //    uint32_t segmentnumber;        //4字节：数据段号
        //    uint8_t resultDescribe;        //1字节:结果描述字 0-成功 1-失败


        //操作标识   2：读目录确认
        self->operateType = msg[startIndex++];
        //文件ID
        self->fileID = msg [startIndex++];
        self->fileID += (msg [startIndex++] * 0x100);
        self->fileID += (msg [startIndex++] * 0x10000);
        self->fileID += (msg [startIndex++] * 0x1000000);
        //数据段号
        self->segmentnumber = msg [startIndex++];
        self->segmentnumber += (msg [startIndex++] * 0x100);
        self->segmentnumber += (msg [startIndex++] * 0x10000);
        self->segmentnumber += (msg [startIndex++] * 0x1000000);
        //结果描述字
        self->resultDescribe = msg[startIndex++];

    }



    return self;
}

int FileTransferAffirmWrite_getOperateType(FileTransferAffirmWrite self)
{
    return self->operateType;
}

unsigned char FileTransferAffirmWrite_getResultDescribe(FileTransferAffirmWrite self)
{
    return self->resultDescribe;
}



unsigned int FileTransferAffirmWrite_getFileID(FileTransferAffirmWrite self)
{
    return self->fileID;
}

unsigned int FileTransferAffirmWrite_getFilesegmentnumber(FileTransferAffirmWrite self)
{
    return self->segmentnumber;
}


void FileTransferAffirmWrite_destroy(FileTransferAffirmWrite self)
{
    GLOBAL_FREEMEM(self);
}
















//========== <210>	：文件服务 ==========//
