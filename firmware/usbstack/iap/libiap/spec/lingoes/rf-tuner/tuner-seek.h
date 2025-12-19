#pragma once
#include <stdint.h>

/* [1] P.302 4.7.23 Command 0x12: TunerSeekStart */

enum IAPTunerSeekOperationID {
    IAPTunerSeekOperationID_NoSeek                                          = 0x00,
    IAPTunerSeekOperationID_SeekUpFromBeginning                             = 0x01,
    IAPTunerSeekOperationID_SeekDownFromEnd                                 = 0x02,
    IAPTunerSeekOperationID_SeekUpFromCurrent                               = 0x03,
    IAPTunerSeekOperationID_SeekDownFromCurrent                             = 0x04,
    IAPTunerSeekOperationID_SeekUpFromBeginningWithRssiThreshold            = 0x05,
    IAPTunerSeekOperationID_SeekDownFromEndWithRssiThreshold                = 0x06,
    IAPTunerSeekOperationID_SeekUpFromCurrentWithRssiThreshold              = 0x07,
    IAPTunerSeekOperationID_SeekDownFromCurrentWithRssiThreshold            = 0x08,
    IAPTunerSeekOperationID_SeekUpFromBeginningForHDSignal                  = 0x09,
    IAPTunerSeekOperationID_SeekDownFromEndForHDSignal                      = 0x0A,
    IAPTunerSeekOperationID_SeekUpFromCurrentForHDSignal                    = 0x0B,
    IAPTunerSeekOperationID_SeekDownFromCurrentForHDSignal                  = 0x0C,
    IAPTunerSeekOperationID_SeekUpFromBeginningForHDSignalWithRssiThreshold = 0x0D,
    IAPTunerSeekOperationID_SeekDownFromEndForHDSignalWithRssiThreshold     = 0x0E,
    IAPTunerSeekOperationID_SeekUpFromCurrentForHDSignalWithRssiThreshold   = 0x0F,
    IAPTunerSeekOperationID_SeekDownFromCurrentForHDSignalWithRssiThreshold = 0x10,
};

struct IAPTunerSeekStartPayload {
    uint8_t operation_id; /* IAPTunerSeekOperationID */
} __attribute__((packed));

struct IAPTunerSeekDonePayload {
    uint32_t freq_khz;
    uint8_t  signal_strength;
} __attribute__((packed));
