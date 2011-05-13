/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYNAPTICS_RMI_H
#define SYNAPTICS_RMI_H

#define RMI_PAGE_SELECT             0xff /* $xxff */
#define RMI_MAKE_REG(fn, reg)       (((fn) << 8) | (reg))
#define RMI_DEVICE_CONTROL          0x0 /* $0000 */
#define RMI_INTERRUPT_ENABLE        0x1 /* $0001 */
#define RMI_INTERRUPT_REQUEST       0x3 /* $0003 */
#define RMI_PRODUCT_ID              0x210 /* $0210 */
#define RMI_PRODUCT_ID_LEN          16
#define RMI_2D_TOUCHPAD_FUNCTION       0x10
#define RMI_FUNCTION_PRESENCE(fn)   RMI_MAKE_REG(0x03, fn) /* $03xx */
#define RMI_FUNCTION_VERSION(fn)    RMI_MAKE_REG(fn, 0) /* $xx00 */
#define RMI_DATA_REGISTER(data_src) RMI_MAKE_REG(0x4, data_src) /* $04xx */
/* 2D TouchPad/ClearPad */
#define RMI_2D_REG(reg)             RMI_MAKE_REG(RMI_2D_TOUCHPAD_FUNCTION, reg)
#define RMI_2D_SENSOR_PROP1(sens)   RMI_2D_REG(8 * (sens) + 0x2)
#define RMI_2D_SENSOR_PROP2(sens)   RMI_2D_REG(8 * (sens) + 0x3)
#define RMI_2D_SENSOR_XMAX_MSB(s)   RMI_2D_REG(8 * (s) + 0x4)
#define RMI_2D_SENSOR_XMAX_LSB(s)   RMI_2D_REG(8 * (s) + 0x5)
#define RMI_2D_SENSOR_YMAX_MSB(s)   RMI_2D_REG(8 * (s) + 0x6)
#define RMI_2D_SENSOR_YMAX_LSB(s)   RMI_2D_REG(8 * (s) + 0x7)
#define RMI_2D_SENSOR_RESOLUTION(s) RMI_2D_REG(8 * (s) + 0x8)
#define RMI_2D_GLOBAL_SETTINGS      RMI_2D_REG(0x41)
#define RMI_2D_GESTURE_SETTINGS     RMI_2D_REG(0x42)
#define RMI_2D_GESTURE_PRESS_TIME_BM    0x7
#define RMI_2D_GESTURE_PRESS_TIME_300MS 0
#define RMI_2D_GESTURE_PRESS_TIME_400MS 1
#define RMI_2D_GESTURE_PRESS_TIME_500MS 2
#define RMI_2D_GESTURE_PRESS_TIME_600MS 3
#define RMI_2D_GESTURE_PRESS_TIME_700MS 4
#define RMI_2D_GESTURE_PRESS_TIME_800MS 5
#define RMI_2D_GESTURE_PRESS_TIME_900MS 6
#define RMI_2D_GESTURE_PRESS_TIME_1S    7
#define RMI_2D_GESTURE_FLICK_DIST_BM    (0x7 << 3)
#define RMI_2D_GESTURE_FLICK_DIST_BP    3
#define RMI_2D_GESTURE_FLICK_DIST_4MM   0
#define RMI_2D_GESTURE_FLICK_DIST_5MM   1
#define RMI_2D_GESTURE_FLICK_DIST_6MM   2
#define RMI_2D_GESTURE_FLICK_DIST_7MM   3
#define RMI_2D_GESTURE_FLICK_DIST_8MM   4
#define RMI_2D_GESTURE_FLICK_DIST_9MM   5
#define RMI_2D_GESTURE_FLICK_DIST_10MM  6
#define RMI_2D_GESTURE_FLICK_DIST_11MM  7
#define RMI_2D_GESTURE_FLICK_TIME_BM    (2 << 6)
#define RMI_2D_GESTURE_FLICK_TIME_BP    6
#define RMI_2D_GESTURE_FLICK_TIME_250MS 0
#define RMI_2D_GESTURE_FLICK_TIME_400MS 1
#define RMI_2D_GESTURE_FLICK_TIME_550MS 2
#define RMI_2D_GESTURE_FLICK_TIME_700MS 3
#define RMI_2D_SENSITIVITY_ADJ      RMI_2D_REG(0x44)
#define RMI_2D_MIN_DIST             RMI_2D_REG(0x45)
/* 2D TouchPad/ClearPad data registers */
struct rmi_2d_absolute_data_t
{
    unsigned char misc;
    unsigned char z;
    unsigned char x_msb;
    unsigned char x_lsb;
    unsigned char y_msb;
    unsigned char y_lsb;
} __attribute__((packed));

#define RMI_2D_ABS_MISC_NR_FINGERS_BM   7
#define RMI_2D_ABS_MISC_GESTURE         (1 << 3)
#define RMI_2D_ABS_MISC_WIDTH_BM        0xf0
#define RMI_2D_ABS_MISC_WIDTH_BP        4
#define RMI_2D_GEST_MISC_TAP_CODE_BM    7
#define RMI_2D_GEST_MISC_NO_TAP         0
#define RMI_2D_GEST_MISC_SINGLE_TAP     1
#define RMI_2D_GEST_MISC_TAP_AND_HOLD   2
#define RMI_2D_GEST_MISC_DOUBLE_TAP     3
#define RMI_2D_GEST_MISC_PRESS          (1 << 3)
#define RMI_2D_GEST_MISC_FLICK          (1 << 4)
#define RMI_2D_GEST_MISC_PINCH          (1 << 5)
#define RMI_2D_GEST_MISC_CONFIRMED      (1 << 7)
#define RMI_2D_GEST_FLICK_X_BM          0x0f
#define RMI_2D_GEST_FLICK_Y_BM          0xf0
#define RMI_2D_GEST_FLICK_Y_BP          4

struct rmi_2d_relative_data_t
{
    signed char x;  /* signed */
    signed char y; /* signed */
} __attribute__((packed));

struct rmi_2d_gesture_data_t
{
    unsigned char misc;
    unsigned char flick;
} __attribute__((packed));

/* Initialize the RMI driver, the i2c_bus_index is the bus index returned by
 * the generic_i2c driver; the i2c_dev_addr is the i2c address of the device.
 * NOTE: the driver automatically handles the page select mechanism used for
 *       RMI over i2c and assumes a standard page select register at 0xff. */
int rmi_init(int i2c_bus_index, int i2c_dev_addr);
/* Read one or more registers.
 * WARNING: don't cross a page boundary ! */
int rmi_read(int address, int byte_count, unsigned char *buffer);
/* Read a single register (return -1 on error)
 * WARNING: beware of register consistency (N x read 1 byte != reads N bytes) */
int rmi_read_single(int address); /* return byte value or <0 in case of error */
/* Write one of more register
 * WARNING: don't cross a page boundary ! */
int rmi_write(int address, int byte_count, const unsigned char *buffer);
/* Write one register
 * WARNING: don't cross a page boundary ! */
int rmi_write_single(int address, unsigned char byte);

#endif
