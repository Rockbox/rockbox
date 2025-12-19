#pragma once

#define HID_BUFFER_SIZE  1024
#define SEND_BUFFER_SIZE 767 /* 0x2ff - 1: input_report_size_table_hs[0x0C].size - sizeof(report_id) */

enum IAPPhase {
    IAPPhase_Connected = 0, /* initial state, waiting for StartIDPS */
    IAPPhase_IDPS,          /* idps started */
    IAPPhase_Auth,          /* idps completed, authenticating accessory */
    IAPPhase_Authed,        /* authentication completed, processing requests */
};
