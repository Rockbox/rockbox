#pragma once
#include <stdint.h>

enum IAPRegisterDescriptorCountryCodes {
    IAPRegisterDescriptorCountryCodes_Undefined         = 0x00,
    IAPRegisterDescriptorCountryCodes_Arabic            = 0x01,
    IAPRegisterDescriptorCountryCodes_Belgian           = 0x02,
    IAPRegisterDescriptorCountryCodes_CanadianBilingual = 0x03,
    IAPRegisterDescriptorCountryCodes_CanadianFrench    = 0x04,
    IAPRegisterDescriptorCountryCodes_CzechRepublic     = 0x05,
    IAPRegisterDescriptorCountryCodes_Danish            = 0x06,
    IAPRegisterDescriptorCountryCodes_Finnish           = 0x07,
    IAPRegisterDescriptorCountryCodes_French            = 0x08,
    IAPRegisterDescriptorCountryCodes_German            = 0x09,
    IAPRegisterDescriptorCountryCodes_Greek             = 0x0A,
    IAPRegisterDescriptorCountryCodes_Hebrew            = 0x0B,
    IAPRegisterDescriptorCountryCodes_Hungarian         = 0x0C,
    IAPRegisterDescriptorCountryCodes_International     = 0x0D,
    IAPRegisterDescriptorCountryCodes_Italian           = 0x0E,
    IAPRegisterDescriptorCountryCodes_Japan             = 0x0F,
    IAPRegisterDescriptorCountryCodes_Korean            = 0x10,
    IAPRegisterDescriptorCountryCodes_LatinAmerican     = 0x11,
    IAPRegisterDescriptorCountryCodes_Netherlands       = 0x12,
    IAPRegisterDescriptorCountryCodes_Norwegian         = 0x13,
    IAPRegisterDescriptorCountryCodes_Persian           = 0x14,
    IAPRegisterDescriptorCountryCodes_Poland            = 0x15,
    IAPRegisterDescriptorCountryCodes_Portuguese        = 0x16,
    IAPRegisterDescriptorCountryCodes_Russian           = 0x17,
    IAPRegisterDescriptorCountryCodes_Slovakia          = 0x18,
    IAPRegisterDescriptorCountryCodes_Spanish           = 0x19,
    IAPRegisterDescriptorCountryCodes_Swedish           = 0x1A,
    IAPRegisterDescriptorCountryCodes_SwissFrench       = 0x1B,
    IAPRegisterDescriptorCountryCodes_SwissGerman       = 0x1C,
    IAPRegisterDescriptorCountryCodes_Switzerland       = 0x1D,
    IAPRegisterDescriptorCountryCodes_Taiwan            = 0x1E,
    IAPRegisterDescriptorCountryCodes_TurkishQ          = 0x1F,
    IAPRegisterDescriptorCountryCodes_UK                = 0x20,
    IAPRegisterDescriptorCountryCodes_US                = 0x21,
    IAPRegisterDescriptorCountryCodes_Croatian          = 0x22,
    IAPRegisterDescriptorCountryCodes_TurkishF          = 0x23,
    IAPRegisterDescriptorCountryCodes_Thai              = 0xFA,
    IAPRegisterDescriptorCountryCodes_Flemish           = 0xFB,
    IAPRegisterDescriptorCountryCodes_Romanian          = 0xFC,
    IAPRegisterDescriptorCountryCodes_Bulgarian         = 0xFD,
    IAPRegisterDescriptorCountryCodes_Chinese           = 0xFE,
};

struct IAPRegisterDescriptorPayload {
    uint8_t  index;
    uint16_t vid;
    uint16_t pid;
    uint8_t  country_code; /* IAPRegisterDescriptorCountryCodes */
    uint8_t  descriptor[];
} __attribute__((packed));

enum IAPHIDReportReportType {
    IAPHIDReportReportType_Input  = 0x00,
    IAPHIDReportReportType_Output = 0x01,
};

struct IAPIPodHIDReportPayload {
    uint8_t index;
    uint8_t report_type; /* = IAPHIDReportReportType_Input */
    uint8_t report[];
} __attribute__((packed));

struct IAPAccessoryHIDReportPayload {
    uint8_t index;
    uint8_t report_type; /* = IAPHIDReportReportType_Output */
    uint8_t report[];
} __attribute__((packed));

struct IAPUnregisterDescriptorPayload {
    uint8_t index;
} __attribute__((packed));
