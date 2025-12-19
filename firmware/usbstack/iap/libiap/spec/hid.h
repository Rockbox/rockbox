#pragma once
#include <stdint.h>

/* [2] P.56 Table 3-2 Link control byte usage */
enum IAPHIDReportLinkControlBits {
    IAPHIDReportLinkControlBits_Continue     = 1 << 0,
    IAPHIDReportLinkControlBits_MoreToFollow = 1 << 1,
};

struct IAPHIDReport {
    uint8_t report_id;
    uint8_t link_control; /* IAPHIDReportLinkControlBits */
    uint8_t data[];
} __attribute__((packed));
