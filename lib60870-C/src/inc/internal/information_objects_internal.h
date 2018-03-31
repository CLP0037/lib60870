/*
 * information_objects_internal.h
 *
 *  Created on: Aug 5, 2016
 *      Author: mzillgit
 */

#ifndef SRC_INC_INFORMATION_OBJECTS_INTERNAL_H_
#define SRC_INC_INFORMATION_OBJECTS_INTERNAL_H_

#include "../api/cs101_information_objects.h"
#include "frame.h"

#define MAX_MENUNUM 64
#define MAX_FILENUM 64

typedef struct sInformationObjectVFT* InformationObjectVFT;

bool
InformationObject_encode(InformationObject self, Frame frame, CS101_AppLayerParameters parameters, bool isSequence);

void
InformationObject_setObjectAddress(InformationObject self, int ioa);

TypeID
InformationObject_getType(InformationObject self);

int
InformationObject_ParseObjectAddress(CS101_AppLayerParameters parameters, uint8_t* msg, int startIndex);

SinglePointInformation
SinglePointInformation_getFromBuffer(SinglePointInformation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueScaledWithCP56Time2a
MeasuredValueScaledWithCP56Time2a_getFromBuffer(MeasuredValueScaledWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

StepPositionInformation
StepPositionInformation_getFromBuffer(StepPositionInformation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

StepPositionWithCP56Time2a
StepPositionWithCP56Time2a_getFromBuffer(StepPositionWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

StepPositionWithCP24Time2a
StepPositionWithCP24Time2a_getFromBuffer(StepPositionWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

DoublePointInformation
DoublePointInformation_getFromBuffer(DoublePointInformation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

DoublePointWithCP24Time2a
DoublePointWithCP24Time2a_getFromBuffer(DoublePointWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

DoublePointWithCP56Time2a
DoublePointWithCP56Time2a_getFromBuffer(DoublePointWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

SinglePointWithCP24Time2a
SinglePointWithCP24Time2a_getFromBuffer(SinglePointWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

SinglePointWithCP56Time2a
SinglePointWithCP56Time2a_getFromBuffer(SinglePointWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

//42故障事件
FaultEventWithCP56Time2a
FaultEventWithCP56Time2a_getFromBuffer(FaultEventWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

BitString32
BitString32_getFromBuffer(BitString32 self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

Bitstring32WithCP24Time2a
Bitstring32WithCP24Time2a_getFromBuffer(Bitstring32WithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

Bitstring32WithCP56Time2a
Bitstring32WithCP56Time2a_getFromBuffer(Bitstring32WithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueNormalized
MeasuredValueNormalized_getFromBuffer(MeasuredValueNormalized self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueNormalizedWithCP24Time2a
MeasuredValueNormalizedWithCP24Time2a_getFromBuffer(MeasuredValueNormalizedWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueNormalizedWithCP56Time2a
MeasuredValueNormalizedWithCP56Time2a_getFromBuffer(MeasuredValueNormalizedWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueScaled
MeasuredValueScaled_getFromBuffer(MeasuredValueScaled self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueScaledWithCP24Time2a
MeasuredValueScaledWithCP24Time2a_getFromBuffer(MeasuredValueScaledWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueShort
MeasuredValueShort_getFromBuffer(MeasuredValueShort self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueShortWithCP24Time2a
MeasuredValueShortWithCP24Time2a_getFromBuffer(MeasuredValueShortWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueShortWithCP56Time2a
MeasuredValueShortWithCP56Time2a_getFromBuffer(MeasuredValueShortWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

IntegratedTotals
IntegratedTotals_getFromBuffer(IntegratedTotals self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

IntegratedTotalsWithCP24Time2a
IntegratedTotalsWithCP24Time2a_getFromBuffer(IntegratedTotalsWithCP24Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

IntegratedTotalsWithCP56Time2a
IntegratedTotalsWithCP56Time2a_getFromBuffer(IntegratedTotalsWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

EventOfProtectionEquipment
EventOfProtectionEquipment_getFromBuffer(EventOfProtectionEquipment self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

PackedStartEventsOfProtectionEquipment
PackedStartEventsOfProtectionEquipment_getFromBuffer(PackedStartEventsOfProtectionEquipment self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

PackedOutputCircuitInfo
PackedOutputCircuitInfo_getFromBuffer(PackedOutputCircuitInfo self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

PackedSinglePointWithSCD
PackedSinglePointWithSCD_getFromBuffer(PackedSinglePointWithSCD self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

MeasuredValueNormalizedWithoutQuality
MeasuredValueNormalizedWithoutQuality_getFromBuffer(MeasuredValueNormalizedWithoutQuality self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

EventOfProtectionEquipmentWithCP56Time2a
EventOfProtectionEquipmentWithCP56Time2a_getFromBuffer(EventOfProtectionEquipmentWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

PackedStartEventsOfProtectionEquipmentWithCP56Time2a
PackedStartEventsOfProtectionEquipmentWithCP56Time2a_getFromBuffer(PackedStartEventsOfProtectionEquipmentWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

PackedOutputCircuitInfoWithCP56Time2a
PackedOutputCircuitInfoWithCP56Time2a_getFromBuffer(PackedOutputCircuitInfoWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

SingleCommand
SingleCommand_getFromBuffer(SingleCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SingleCommandWithCP56Time2a
SingleCommandWithCP56Time2a_getFromBuffer(SingleCommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

DoubleCommand
DoubleCommand_getFromBuffer(DoubleCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

StepCommand
StepCommand_getFromBuffer(StepCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SetpointCommandNormalized
SetpointCommandNormalized_getFromBuffer(SetpointCommandNormalized self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SetpointCommandScaled
SetpointCommandScaled_getFromBuffer(SetpointCommandScaled self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SetpointCommandShort
SetpointCommandShort_getFromBuffer(SetpointCommandShort self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

Bitstring32Command
Bitstring32Command_getFromBuffer(Bitstring32Command self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

ReadCommand
ReadCommand_getFromBuffer(ReadCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

ClockSynchronizationCommand
ClockSynchronizationCommand_getFromBuffer(ClockSynchronizationCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

InterrogationCommand
InterrogationCommand_getFromBuffer(InterrogationCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

ParameterNormalizedValue
ParameterNormalizedValue_getFromBuffer(ParameterNormalizedValue self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

ParameterScaledValue
ParameterScaledValue_getFromBuffer(ParameterScaledValue self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

ParameterFloatValue
ParameterFloatValue_getFromBuffer(ParameterFloatValue self, CS101_AppLayerParameters parameters,
        uint8_t* msqg, int msgSize, int startIndex);

ParameterActivation
ParameterActivation_getFromBuffer(ParameterActivation self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

EndOfInitialization
EndOfInitialization_getFromBuffer(EndOfInitialization self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

DoubleCommandWithCP56Time2a
DoubleCommandWithCP56Time2a_getFromBuffer(DoubleCommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

StepCommandWithCP56Time2a
StepCommandWithCP56Time2a_getFromBuffer(StepCommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SetpointCommandNormalizedWithCP56Time2a
SetpointCommandNormalizedWithCP56Time2a_getFromBuffer(SetpointCommandNormalizedWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SetpointCommandScaledWithCP56Time2a
SetpointCommandScaledWithCP56Time2a_getFromBuffer(SetpointCommandScaledWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SetpointCommandShortWithCP56Time2a
SetpointCommandShortWithCP56Time2a_getFromBuffer(SetpointCommandShortWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

Bitstring32CommandWithCP56Time2a
Bitstring32CommandWithCP56Time2a_getFromBuffer(Bitstring32CommandWithCP56Time2a self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

CounterInterrogationCommand
CounterInterrogationCommand_getFromBuffer(CounterInterrogationCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

TestCommand
TestCommand_getFromBuffer(TestCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

ResetProcessCommand
ResetProcessCommand_getFromBuffer(ResetProcessCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

DelayAcquisitionCommand
DelayAcquisitionCommand_getFromBuffer(DelayAcquisitionCommand self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

FileReady
FileReady_getFromBuffer(FileReady self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

SectionReady
SectionReady_getFromBuffer(SectionReady self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

FileCallOrSelect
FileCallOrSelect_getFromBuffer(FileCallOrSelect self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

FileLastSegmentOrSection
FileLastSegmentOrSection_getFromBuffer(FileLastSegmentOrSection self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

FileACK
FileACK_getFromBuffer(FileACK self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

FileSegment
FileSegment_getFromBuffer(FileSegment self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex);

FileDirectory
FileDirectory_getFromBuffer(FileDirectory self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

//远程参数读写===新增
RemoteReposition
RemoteReposition_getFromBuffer(RemoteReposition self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

RemoteReadSN
RemoteReadSN_getFromBuffer(RemoteReadSN self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);//读当前定值区号

ParamValue_Read
ParamValue_Read_getFromBuffer(ParamValue_Read self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence,int nums);

ParamValue_Write
ParamValue_Write_getFromBuffer(ParamValue_Write self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence,int nums);

//文件服务===新增
FileCallMenu
FileCallMenu_getFromBuffer(FileCallMenu self, CS101_AppLayerParameters parameters,
        uint8_t* msg, int msgSize, int startIndex, bool isSequence);

//FileCallMenuAffirm
FileCallMenuAffirm
FileCallMenuAffirm_getFromBuffer(FileCallMenuAffirm self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence);

//FileActivateAffirm
FileActivateAffirm
FileActivateAffirm_getFromBuffer(FileActivateAffirm self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence);

//FileTransfer
FileTransfer
FileTransfer_getFromBuffer(FileTransfer self, CS101_AppLayerParameters parameters,
                                 uint8_t* msg, int msgSize, int startIndex, bool isSequence);

/********************************************
 * static InformationObject type definitions
 ********************************************/

struct sSinglePointInformation {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    bool value;
    QualityDescriptor quality;
};

struct sStepPositionInformation {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t vti;
    QualityDescriptor quality;
};

struct sStepPositionWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t vti;
    QualityDescriptor quality;

    struct sCP56Time2a timestamp;
};

struct sStepPositionWithCP24Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t vti;
    QualityDescriptor quality;

    struct sCP24Time2a timestamp;
};

struct sDoublePointInformation {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    DoublePointValue value;
    QualityDescriptor quality;
};

struct sDoublePointWithCP24Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    DoublePointValue value;
    QualityDescriptor quality;

    struct sCP24Time2a timestamp;
};

struct sDoublePointWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    DoublePointValue value;
    QualityDescriptor quality;

    struct sCP56Time2a timestamp;
};

struct sSinglePointWithCP24Time2a {
    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    bool value;
    QualityDescriptor quality;

    struct sCP24Time2a timestamp;
};

struct sSinglePointWithCP56Time2a {
    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    bool value;
    QualityDescriptor quality;

    struct sCP56Time2a timestamp;
};

//42==故障事件信息 FaultEventWithCP56Time2a
struct sFaultEven_YX
{
    unsigned int        addr;
    unsigned char 		State;
    struct sCP56Time2a timestamp;
};

struct sFaultEven_YC
{
    unsigned int addr;
    float Yc_Value;
};

struct sFaultEventWithCP56Time2a {
    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    bool isEncodeYXelseYC;
    bool isEncodefirstframe;

    int type_YX;  //遥信类型
    int num_YX;   //带时标遥信个数
    //bool value_YX;
    struct sFaultEven_YX structFaultEven_YX[1024];

    int type_YC;  //遥测类型
    int num_YC;   //遥测个数
    //float value_YC;
    struct sFaultEven_YC structFaultEven_YC[1024];

};

struct sBitString32 {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint32_t value;
    QualityDescriptor quality;
};

struct sBitstring32WithCP24Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint32_t value;
    QualityDescriptor quality;

    struct sCP24Time2a timestamp;
};

struct sBitstring32WithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint32_t value;
    QualityDescriptor quality;

    struct sCP56Time2a timestamp;
};

struct sMeasuredValueNormalized {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    QualityDescriptor quality;
};

struct sMeasuredValueNormalizedWithoutQuality {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];
};

struct sMeasuredValueNormalizedWithCP24Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    QualityDescriptor quality;

    struct sCP24Time2a timestamp;
};

struct sMeasuredValueNormalizedWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    QualityDescriptor quality;

    struct sCP56Time2a timestamp;
};

struct sMeasuredValueScaled {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    QualityDescriptor quality;
};

struct sMeasuredValueScaledWithCP24Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    QualityDescriptor quality;

    struct sCP24Time2a timestamp;
};

struct sMeasuredValueScaledWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    QualityDescriptor quality;

    struct sCP56Time2a timestamp;
};

struct sMeasuredValueShort {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    float value;

    QualityDescriptor quality;
};

struct sMeasuredValueShortWithCP24Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    float value;

    QualityDescriptor quality;

    struct sCP24Time2a timestamp;
};

struct sMeasuredValueShortWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    float value;

    QualityDescriptor quality;

    struct sCP56Time2a timestamp;
};

struct sIntegratedTotals {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    struct sBinaryCounterReading totals;
};

struct sIntegratedTotalsWithCP24Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    struct sBinaryCounterReading totals;

    struct sCP24Time2a timestamp;
};

struct sIntegratedTotalsWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    struct sBinaryCounterReading totals;

    struct sCP56Time2a timestamp;
};

struct sEventOfProtectionEquipment {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    tSingleEvent event;

    struct sCP16Time2a elapsedTime;

    struct sCP24Time2a timestamp;
};

struct sEventOfProtectionEquipmentWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    tSingleEvent event;

    struct sCP16Time2a elapsedTime;

    struct sCP56Time2a timestamp;
};

struct sPackedStartEventsOfProtectionEquipment {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    StartEvent event;

    QualityDescriptorP qdp;

    struct sCP16Time2a elapsedTime;

    struct sCP24Time2a timestamp;
};

struct sPackedStartEventsOfProtectionEquipmentWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    StartEvent event;

    QualityDescriptorP qdp;

    struct sCP16Time2a elapsedTime;

    struct sCP56Time2a timestamp;
};

struct sPackedOutputCircuitInfo {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    OutputCircuitInfo oci;

    QualityDescriptorP qdp;

    struct sCP16Time2a operatingTime;

    struct sCP24Time2a timestamp;
};

struct sPackedOutputCircuitInfoWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    OutputCircuitInfo oci;

    QualityDescriptorP qdp;

    struct sCP16Time2a operatingTime;

    struct sCP56Time2a timestamp;
};

struct sPackedSinglePointWithSCD {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    tStatusAndStatusChangeDetection scd;

    QualityDescriptor qds;
};

struct sSingleCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t sco;
};

struct sSingleCommandWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t sco;

    struct sCP56Time2a timestamp;
};

struct sDoubleCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t dcq;
};

struct sDoubleCommandWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t dcq;

    struct sCP56Time2a timestamp;
};

struct sStepCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t dcq;
};

struct sStepCommandWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t dcq;

    struct sCP56Time2a timestamp;
};

struct sSetpointCommandNormalized {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    uint8_t qos; /* Qualifier of setpoint command */
};

struct sSetpointCommandNormalizedWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    uint8_t qos; /* Qualifier of setpoint command */

    struct sCP56Time2a timestamp;
};

struct sSetpointCommandScaled {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    uint8_t qos; /* Qualifier of setpoint command */
};

struct sSetpointCommandScaledWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t encodedValue[2];

    uint8_t qos; /* Qualifier of setpoint command */

    struct sCP56Time2a timestamp;
};

struct sSetpointCommandShort {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    float value;

    uint8_t qos; /* Qualifier of setpoint command */
};

struct sSetpointCommandShortWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    float value;

    uint8_t qos; /* Qualifier of setpoint command */

    struct sCP56Time2a timestamp;
};

struct sBitstring32Command {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint32_t value;
};

struct sBitstring32CommandWithCP56Time2a {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint32_t value;

    struct sCP56Time2a timestamp;
};

struct sReadCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;
};

struct sClockSynchronizationCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    struct sCP56Time2a timestamp;
};

struct sInterrogationCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t qoi;
};

struct sCounterInterrogationCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t qcc;
};

struct sTestCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t byte1;
    uint8_t byte2;
};

struct sResetProcessCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    QualifierOfRPC qrp;
};

struct sDelayAcquisitionCommand {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    struct sCP16Time2a delay;
};

struct sParameterActivation {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    QualifierOfParameterActivation qpa;
};

struct sEndOfInitialization {

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t coi;
};

struct sFileReady {//<120>	：=文件准备就绪

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint16_t nof; /* name of file */

    uint32_t lengthOfFile;

    uint8_t frq; /* file ready qualifier */
};

struct sSectionReady {//<121>	：=节准备就绪

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint16_t nof; /* name of file */

    uint8_t nameOfSection;

    uint32_t lengthOfSection;

    uint8_t srq; /* section ready qualifier */
};

struct sFileCallOrSelect {//<122>	：=召唤目录（可选），选择文件（可选），召唤文件，召唤节

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint16_t nof; /* name of file */

    uint8_t nameOfSection;

    uint8_t scq; /* select and call qualifier */
};

struct sFileLastSegmentOrSection {//<123>	：=最后的节，最后的段

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint16_t nof; /* name of file */

    uint8_t nameOfSection;

    uint8_t lsq; /* last section or segment qualifier */

    uint8_t chs; /* checksum of section or segment */
};

struct sFileACK {//<124>	：=认可文件，认可节

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint16_t nof; /* name of file */

    uint8_t nameOfSection;

    uint8_t afq; /* AFQ (acknowledge file or section qualifier) */
};

struct sFileSegment {//<125>	：=段

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint16_t nof; /* name of file */

    uint8_t nameOfSection;

    uint8_t los; /* length of segment */

    uint8_t* data; /* user data buffer - file payload */
};

struct sFileDirectory {//<126>	：=目录

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint16_t nof; /* name of file */

    int lengthOfFile; /* LOF */

    uint8_t sof; /* state of file */

    struct sCP56Time2a creationTime;
};

//远程参数读写
//<200>：= 切换定值区 C_SR_NA_1
//<201>：= 读定值区号 C_RR_NA_1
//<202>：= 读参数和定值 C_RS_NA_1
//<203>：= 写参数和定值 C_WS_NA_1
struct sRemoteReposition{//200==切换定制区
    int objectAddress;
    TypeID type;
    InformationObjectVFT virtualFunctionTable;

    uint16_t SN;//定值区号 SN  2 字节
};

struct sRemoteReadSN{//201==读定值区号
    int objectAddress;
    TypeID type;
    InformationObjectVFT virtualFunctionTable;

    uint16_t SN1;//当前定值区区号 SN1
    uint16_t SN2;//终端支持的最小定值区号 SN2
    int64_t SN3;//终端支持的最大定值区号 SN3
};

struct _sParamRead{
    int informationAddr;//信息体地址  3 字节
    uint8_t tagType;//Tag 类型  1 字节
    uint8_t datalen;//数据长度  1 字节
    char datavalue[128];//数据值  由数据长度决定

};

struct sParamRead{
    int paramNums;//参数个数
    struct _sParamRead structParamRead[1024];
};

struct sParamValue_Read{//202==读参数和定值
    int objectAddress;
    TypeID type;
    InformationObjectVFT virtualFunctionTable;

    int infoaddr[1024];
    //int paramNums;//参数个数

    uint8_t paramTI;//参数特征标识
    uint16_t SN;//定值区号 SN
    sParamRead __sParamRead;//sParamRead *ParamRead;
};

struct sParamWrite{
    int informationAddr;//信息体地址  3 字节
    uint8_t tagType;//Tag 类型  1 字节
    uint8_t datalen;//数据长度  1 字节
    char datavalue[128];//数据值  由数据长度决定

};
struct sParamValue_Write{//203==写参数和定值
    int objectAddress;
    TypeID type;
    InformationObjectVFT virtualFunctionTable;

    uint8_t paramTI;//参数特征标识
    uint16_t SN;//定值区号 SN
    int paramNums;//参数个数
    struct sParamWrite structParamWrite[1024];
};

//========== <210>	：文件服务 ==========//
struct sFileCallMenu{//文件目录召唤

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t operateType;           //1字节:操作标识  1：读目录
    uint32_t catalogueID;          //4字节：目录ID
    uint8_t catalogueNamelength;   //1字节：目录名长度
    char* catalogueName;           //x字节：目录名
    uint8_t callflag;              //1字节：召唤标志(0：目录下所有文件；1：目录下满足搜索时间段的文件)
    struct sCP56Time2a startTime;  //7字节：查询起始时间
    struct sCP56Time2a endTime;    //7字节：查询终止时间
};

struct sFileActivate{//读文件激活

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t operateType;           //1字节:操作标识  3：读文件激活
    uint8_t fileNamelength;        //1字节：文件长度
    char* fileName;             //x字节：文件名
};

struct sFileTransferAffirm{//读文件传输确认

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t operateType;           //1字节:操作标识  6：读文件数据响应
    uint32_t fileID;               //4字节：文件ID
    uint32_t segmentnumber;        //4字节：数据段号,可以使用文件内容的偏移指针值
    uint8_t followupFlag;          //1字节:后续标志,0：无后续,1：有后续

};

struct sMenu{//目录
    uint8_t fileNamelength;        //1字节：文件长度
    //uint8_t* fileName;//char* fileName;                //x字节：文件名
    char fileName[256];
    uint8_t fileProperty;          //1字节：文件属性
    uint32_t fileSize;             //4字节：文件大小
    struct sCP56Time2a fileTime;   //7字节：文件时间

};


//affirm ==== 确认回复
struct sFileCallMenuAffirm{//目录召唤确认

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t operateType;           //1字节:操作标识  2：读目录(目录召唤确认)
    uint8_t resultDescribe;        //1字节:结果描述字 0-成功 1-失败
    uint32_t catalogueID;          //4字节：目录ID
    uint8_t followupFlag;          //1字节:后续标志,0：无后续,1：有后续
    uint8_t file_num;              //1字节：本帧文件数量(通常1帧数据5个目录名称)

    struct sMenu structMenu[MAX_MENUNUM];//文件1 ... 文件n[MAX_MENUNUM]
};

struct sFileActivateAffirm{//读文件激活确认

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t operateType;           //1字节:操作标识  4：读文件激活确认
    uint8_t resultDescribe;        //1字节:结果描述字 0-成功 1-失败
    uint8_t fileNamelength;        //1字节：文件长度
    //char* fileName;             //x字节：文件名
    char fileName[256];
    uint32_t fileID;               //4字节：文件ID
    uint32_t fileSize;             //4字节：文件大小

};

struct sFile{//文件

};

struct sFileTransfer{//读文件传输

    int objectAddress;

    TypeID type;

    InformationObjectVFT virtualFunctionTable;

    uint8_t operateType;           //1字节:操作标识  5：读文件数据
    uint32_t fileID;               //4字节：文件ID
    uint32_t segmentnumber;        //4字节：数据段号,可以使用文件内容的偏移指针值
    uint8_t followupFlag;          //1字节:后续标志,0：无后续,1：有后续
    //char* fileData;                //x字节：文件数据
    char fileData[256];
    uint8_t file_currentlen; //当前段长度
    uint8_t fileCheckSum;          //1字节:校验码(校验范围：文件数据 校验算法：单字节模和运算)
};


//========== <210>	：文件服务 ==========//


union uInformationObject {
    struct sSinglePointInformation m1;
    struct sStepPositionInformation m2;
    struct sStepPositionWithCP24Time2a m3;
    struct sStepPositionWithCP56Time2a m4;
    struct sDoublePointInformation m5;
    struct sDoublePointWithCP24Time2a m6;
    struct sDoublePointWithCP56Time2a m7;
    struct sSinglePointWithCP24Time2a m8;
    struct sSinglePointWithCP56Time2a m9;
    struct sBitString32 m10;
    struct sBitstring32WithCP24Time2a m11;
    struct sBitstring32WithCP56Time2a m12;
    struct sMeasuredValueNormalized m13;
    struct sMeasuredValueNormalizedWithCP24Time2a m14;
    struct sMeasuredValueNormalizedWithCP56Time2a m15;
    struct sMeasuredValueScaled m16;
    struct sMeasuredValueScaledWithCP24Time2a m17;
    struct sMeasuredValueScaledWithCP56Time2a m18;
    struct sMeasuredValueShort m19;
    struct sMeasuredValueShortWithCP24Time2a m20;
    struct sMeasuredValueShortWithCP56Time2a m21;
    struct sIntegratedTotals m22;
    struct sIntegratedTotalsWithCP24Time2a m23;
    struct sIntegratedTotalsWithCP56Time2a m24;
    struct sSingleCommand m25;
    struct sSingleCommandWithCP56Time2a m26;
    struct sDoubleCommand m27;
    struct sStepCommand m28;
    struct sSetpointCommandNormalized m29;
    struct sSetpointCommandScaled m30;
    struct sSetpointCommandShort m31;
    struct sBitstring32Command m32;
    struct sReadCommand m33;
    struct sClockSynchronizationCommand m34;
    struct sInterrogationCommand m35;
    struct sParameterActivation m36;
    struct sEventOfProtectionEquipmentWithCP56Time2a m37;
    struct sStepCommandWithCP56Time2a m38;
};

#endif /* SRC_INC_INFORMATION_OBJECTS_INTERNAL_H_ */
