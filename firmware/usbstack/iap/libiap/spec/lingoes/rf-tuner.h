#pragma once
#include <stdint.h>

/* [1] P.288 Table 4-111 RF Tuner lingo command summary */
enum IAPRFTunerCommandID {
    IAPRFTunerCommandID_AccessoryAck             = 0x00, /* 1.00, from acc, acc-ack.h */
    IAPRFTunerCommandID_GetTunerCaps             = 0x01, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetTunerCaps             = 0x02, /* 1.00, from acc, tuner-caps.h */
    IAPRFTunerCommandID_GetTunerCtrl             = 0x03, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetTunerCtrl             = 0x04, /* 1.00, from acc, tuner-ctrl.h */
    IAPRFTunerCommandID_SetTunerCtrl             = 0x05, /* 1.00, from dev, tuner-ctlr.h */
    IAPRFTunerCommandID_GetTunerBand             = 0x06, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetTunerBand             = 0x07, /* 1.00, from acc, tuner-band.h */
    IAPRFTunerCommandID_SetTunerBand             = 0x08, /* 1.00, from dev, tuner-band.h*/
    IAPRFTunerCommandID_GetTunerFreq             = 0x09, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetTunerFreq             = 0x0A, /* 1.00, from acc, tuner-freq.h */
    IAPRFTunerCommandID_SetTunerFreq             = 0x0B, /* 1.00, from dev, tuner-freq.h */
    IAPRFTunerCommandID_GetTunerMode             = 0x0C, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetTunerMode             = 0x0D, /* 1.00, from acc, tuner-mode.h */
    IAPRFTunerCommandID_SetTunerMode             = 0x0E, /* 1.00, from dev, tuner-mode.h */
    IAPRFTunerCommandID_GetTunerSeekRssi         = 0x0F, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetTunerSeekRssi         = 0x10, /* 1.00, from acc, tuner-seek-rssi.h */
    IAPRFTunerCommandID_SetTunerSeekRssi         = 0x11, /* 1.00, from dev, tuner-seek-rssi.h */
    IAPRFTunerCommandID_TunerSeekStart           = 0x12, /* 1.00, from dev, tuner-seek.h */
    IAPRFTunerCommandID_TunerSeekDone            = 0x13, /* 1.00, from acc, tuner-seek.h */
    IAPRFTunerCommandID_GetTunerStatus           = 0x14, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetTunerStatus           = 0x15, /* 1.00, from acc, tuner-status.h */
    IAPRFTunerCommandID_GetStatusNotifyMask      = 0x16, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetStatusNotifyMask      = 0x17, /* 1.00, from acc, status-notification.h */
    IAPRFTunerCommandID_SetStatusNotifyMask      = 0x18, /* 1.00, from dev, status-notification.h */
    IAPRFTunerCommandID_StatusChangeNotify       = 0x19, /* 1.00, from acc, status-notification.h */
    IAPRFTunerCommandID_GetRdsReadyStatus        = 0x1A, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetRdsReadyStatus        = 0x1B, /* 1.00, from acc, rds-ready-status.h */
    IAPRFTunerCommandID_GetRdsData               = 0x1C, /* 1.00, from dev, rds-data.h */
    IAPRFTunerCommandID_RetRdsData               = 0x1D, /* 1.00, from acc, rds-data.h */
    IAPRFTunerCommandID_GetRdsNotifyMask         = 0x1E, /* 1.00, from dev, no payload */
    IAPRFTunerCommandID_RetRdsNotifyMask         = 0x1F, /* 1.00, from acc, rds-notify-mask.h */
    IAPRFTunerCommandID_SetRdsNotifyMask         = 0x20, /* 1.00, from dev, rds-notify-mask.h */
    IAPRFTunerCommandID_RdsReadyNotify           = 0x21, /* 1.00, from acc, rds-data.h */
    IAPRFTunerCommandID_GetHDProgramServiceCount = 0x25, /* 1.01, from dev, no paylaod */
    IAPRFTunerCommandID_RetHDProgramServiceCount = 0x26, /* 1.01, from acc, hd-program.h */
    IAPRFTunerCommandID_GetHDProgramService      = 0x27, /* 1.01, from dev, no payload */
    IAPRFTunerCommandID_RetHDProgramService      = 0x28, /* 1.01, from acc, hd-program.h */
    IAPRFTunerCommandID_SetHDProgramService      = 0x29, /* 1.01, from dev, hd-program.h */
    IAPRFTunerCommandID_GetHDDataReadyStatus     = 0x2A, /* 1.01, from dev, no payload */
    IAPRFTunerCommandID_RetHDDataReadyStatus     = 0x2B, /* 1.01, from acc, hd-data.h */
    IAPRFTunerCommandID_GetHDData                = 0x2C, /* 1.01, from dev, hd-data.h */
    IAPRFTunerCommandID_RetHDData                = 0x2D, /* 1.01, from acc, hd-data.h */
    IAPRFTunerCommandID_GetHDDataNotifyMask      = 0x2E, /* 1.01, from dev, no payload */
    IAPRFTunerCommandID_RetHDDataNotifyMask      = 0x2F, /* 1.01, from acc, hd-data.h */
    IAPRFTunerCommandID_SetHDDataNotifyMask      = 0x30, /* 1.01, from dev, hd-data.h */
    IAPRFTunerCommandID_HDDataReadyNotify        = 0x31, /* 1.01, from acc, hd-data.h */
};

#include "rf-tuner/acc-ack.h"
#include "rf-tuner/hd-data.h"
#include "rf-tuner/hd-program.h"
#include "rf-tuner/rds-data.h"
#include "rf-tuner/rds-notify-mask.h"
#include "rf-tuner/rds-ready-status.h"
#include "rf-tuner/status-notification.h"
#include "rf-tuner/tuner-band.h"
#include "rf-tuner/tuner-caps.h"
#include "rf-tuner/tuner-ctrl.h"
#include "rf-tuner/tuner-freq.h"
#include "rf-tuner/tuner-mode.h"
#include "rf-tuner/tuner-seek-rssi.h"
#include "rf-tuner/tuner-seek.h"
#include "rf-tuner/tuner-status.h"
