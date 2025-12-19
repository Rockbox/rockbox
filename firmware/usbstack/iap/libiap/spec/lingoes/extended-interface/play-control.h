#pragma once
#include <stdint.h>

/* [1] P.428 5.1.37 Command 0x0029: PlayControl */

enum IAPPlayControlCode {
    IAPPlayControlCode_TogglePlayPause = 0x01, /* 1.00 */
    IAPPlayControlCode_Stop            = 0x02, /* 1.00 */
    IAPPlayControlCode_NextTrack       = 0x03, /* 1.00 */
    IAPPlayControlCode_PrevTrack       = 0x04, /* 1.00 */
    IAPPlayControlCode_StartFF         = 0x05, /* 1.00 */
    IAPPlayControlCode_StartRew        = 0x06, /* 1.00 */
    IAPPlayControlCode_EndFFRew        = 0x07, /* 1.00 */
    IAPPlayControlCode_Next            = 0x08, /* 1.06 */
    IAPPlayControlCode_Prev            = 0x09, /* 1.06 */
    IAPPlayControlCode_Play            = 0x0A, /* 1.13 */
    IAPPlayControlCode_Pause           = 0x0B, /* 1.13 */
    IAPPlayControlCode_NextChapter     = 0x0C, /* 1.14 */
    IAPPlayControlCode_PrevChapter     = 0x0D, /* 1.14 */
    IAPPlayControlCode_ResumeIPod      = 0x0E, /* 1.14 */
};

struct IAPPlayControlPayload {
    uint8_t code; /* IAPPlayControlCode */
};
