 /*
  * Unfortunately I couldn't find any datasheet for this touch controller nor
  * any other information. I tried to send Melfas an email but their servers
  * seem to be full in this period. The best thing I could find is a Linux
  * driver written by Samsung.
  * In the opensource package for YP-R1 there are also some more information
  * in the file r1TouchMelfasReg.h, which at the moment are not used (I2C stuff
  * and error codes)
  * 
  * The rest, function definitions, are written by me (Lorenzo Miori)
  * 
  * mcs5000_ts.c - Touchscreen driver for MELFAS MCS-5000 controller
  *
  * Copyright (C) 2009 Samsung Electronics Co.Ltd
  * Author: Joonyoung Shim <jy0922.shim@samsung.com>
  *
  * Based on wm97xx-core.c
  *
  *  This program is free software; you can redistribute  it and/or modify it
  *  under  the terms of  the GNU General  Public License as published by the
  *  Free Software Foundation;  either version 2 of the  License, or (at your
  *  option) any later version.
  *
  */

#include "r1TouchIoctl.h"
 
/* TODO place this in general rockbox includes, may be useful in the future */

/* Registers */
#define MCS5000_TS_STATUS               0x00
#define STATUS_OFFSET                   0
#define STATUS_NO                       (0 << STATUS_OFFSET)
#define STATUS_INIT                     (1 << STATUS_OFFSET)
#define STATUS_SENSING                  (2 << STATUS_OFFSET)
#define STATUS_COORD                    (3 << STATUS_OFFSET)
#define STATUS_GESTURE                  (4 << STATUS_OFFSET)
#define ERROR_OFFSET                    4
#define ERROR_NO                        (0 << ERROR_OFFSET)
#define ERROR_POWER_ON_RESET            (1 << ERROR_OFFSET)
#define ERROR_INT_RESET                 (2 << ERROR_OFFSET)
#define ERROR_EXT_RESET                 (3 << ERROR_OFFSET)
#define ERROR_INVALID_REG_ADDRESS       (8 << ERROR_OFFSET)
#define ERROR_INVALID_REG_VALUE         (9 << ERROR_OFFSET)

#define MCS5000_TS_OP_MODE              0x01
#define RESET_OFFSET                    0
#define RESET_NO                        (0 << RESET_OFFSET)
#define RESET_EXT_SOFT                  (1 << RESET_OFFSET)
#define OP_MODE_OFFSET                  1
#define OP_MODE_SLEEP                   (0 << OP_MODE_OFFSET)
#define OP_MODE_ACTIVE                  (1 << OP_MODE_OFFSET)
#define GESTURE_OFFSET                  4
#define GESTURE_DISABLE                 (0 << GESTURE_OFFSET)
#define GESTURE_ENABLE                  (1 << GESTURE_OFFSET)
#define PROXIMITY_OFFSET                5
#define PROXIMITY_DISABLE               (0 << PROXIMITY_OFFSET)
#define PROXIMITY_ENABLE                (1 << PROXIMITY_OFFSET)
#define SCAN_MODE_OFFSET                6
#define SCAN_MODE_INTERRUPT             (0 << SCAN_MODE_OFFSET)
#define SCAN_MODE_POLLING               (1 << SCAN_MODE_OFFSET)
#define REPORT_RATE_OFFSET              7
#define REPORT_RATE_40                  (0 << REPORT_RATE_OFFSET)
#define REPORT_RATE_80                  (1 << REPORT_RATE_OFFSET)

#define MCS5000_TS_SENS_CTL             0x02
#define MCS5000_TS_FILTER_CTL           0x03
#define PRI_FILTER_OFFSET               0
#define SEC_FILTER_OFFSET               4

#define MCS5000_TS_X_SIZE_UPPER         0x08
#define MCS5000_TS_X_SIZE_LOWER         0x09
#define MCS5000_TS_Y_SIZE_UPPER         0x0A
#define MCS5000_TS_Y_SIZE_LOWER         0x0B

#define MCS5000_TS_INPUT_INFO           0x10
#define INPUT_TYPE_OFFSET               0
#define INPUT_TYPE_NONTOUCH             (0 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_SINGLE               (1 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_DUAL                 (2 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_PALM                 (3 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_PROXIMITY            (7 << INPUT_TYPE_OFFSET)
#define GESTURE_CODE_OFFSET             3
#define GESTURE_CODE_NO                 (0 << GESTURE_CODE_OFFSET)

#define MCS5000_TS_X_POS_UPPER          0x11
#define MCS5000_TS_X_POS_LOWER          0x12
#define MCS5000_TS_Y_POS_UPPER          0x13
#define MCS5000_TS_Y_POS_LOWER          0x14
#define MCS5000_TS_Z_POS                0x15
#define MCS5000_TS_WIDTH                0x16
#define MCS5000_TS_GESTURE_VAL          0x17
#define MCS5000_TS_MODULE_REV           0x20
#define MCS5000_TS_FIRMWARE_VER         0x21

/* Touchscreen absolute values */
#define MCS5000_MAX_XC                  0x3ff
#define MCS5000_MAX_YC                  0x3ff

/**
 * Two possibilities for hand usage
 */
typedef enum HandSetting {
    RIGHT_HAND,
    LEFT_HAND,
} HandSetting;

/* Open device */
void mcs5000_init(void);
/* Close device */
void mcs5000_close(void);
/* Power up the chip (voltages) */
void mcs5000_power(void);
/* Shutdown the chip (voltages) */
void mcs5000_shutdown(void);
/* Set user hand usage */
void mcs5000_set_hand(HandSetting handSetting);
/* Set touchscreen sensitivity. Valid values are 1,2,4,8 */
void mcs5000_set_sensitivity(int level);
/* Read controller's data */
int mcs5000_read(TsRawData *touchData);
