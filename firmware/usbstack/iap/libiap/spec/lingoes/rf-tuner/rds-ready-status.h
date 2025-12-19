#pragma once
#include <stdint.h>

/* [1] P.310 4.7.32 Command 0x1B: RetRdsReadyStatus */

enum IAPRdsDataParsedType {
    IAPRdsDataParsedType_RadioText          = 0x04,
    IAPRdsDataParsedType_ProgramServiceName = 0x1E,
};

enum IAPRdsDataRawType {
    IAPRdsDataRawType_RdsGroupData = 0x05,
};

struct IAPRetRdsReadyStatusPayload {
    uint32_t status; /* 1 << IAPRdsData{Parsed,Raw}Type | ... */
} __attribute__((packed));

