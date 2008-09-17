/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Frank Gevaerts
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
#ifndef _S6D0154_H_
#define _S6D0154_H_

#define S6D0154_REG_VERSION 0x00
#define S6D0154_REG_DRIVER_OUTPUT_CONTROL 0x01
#define S6D0154_REG_LCD_DRIVING_WAVEFORM_CONTROL 0x02
#define S6D0154_REG_ENTRY_MODE 0x03
#define S6D0154_REG_DISPLAY_CONTROL 0x07
#define S6D0154_REG_BLANK_PERIOD_CONTROL 0x08
#define S6D0154_REG_FRAME_CYCLE_CONTROL 0x0B
#define S6D0154_REG_EXTERNAL_INTERFACE_CONTROL 0x0C
#define S6D0154_REG_START_OSCILLATION 0x0F
#define S6D0154_REG_POWER_CONTROL_1 0x10
#define S6D0154_REG_POWER_CONTROL_2 0x11
#define S6D0154_REG_POWER_CONTROL_3 0x12
#define S6D0154_REG_POWER_CONTROL_4 0x13
#define S6D0154_REG_POWER_CONTROL_5 0x14
#define S6D0154_REG_VCI_RECYCLING 0x15
#define S6D0154_REG_RAM_ADDRESS_REGISTER_1 0x20
#define S6D0154_REG_RAM_ADDRESS_REGISTER_2 0x21
#define S6D0154_REG_GRAM_READ_WRITE 0x22
#define S6D0154_REG_RESET 0x28
#define S6D0154_REG_FLM_FUNCTION 0x29
#define S6D0154_REG_GATE_SCAN_POSITION 0x30
#define S6D0154_REG_VERTICAL_SCROLL_CONTROL_1A 0x31
#define S6D0154_REG_VERTICAL_SCROLL_CONTROL_1B 0x32
#define S6D0154_REG_VERTICAL_SCROLL_CONTROL_2 0x33
#define S6D0154_REG_PARTIAL_SCREEN_DRIVING_POSITION_1 0x34
#define S6D0154_REG_PARTIAL_SCREEN_DRIVING_POSITION_2 0x35
#define S6D0154_REG_HORIZONTAL_WINDOW_ADDRESS_1 0x36
#define S6D0154_REG_HORIZONTAL_WINDOW_ADDRESS_2 0x37
#define S6D0154_REG_VERTICAL_WINDOW_ADDRESS_1 0x38
#define S6D0154_REG_VERTICAL_WINDOW_ADDRESS_2 0x39
#define S6D0154_REG_SUB_PANEL_CONTROL 0x40
#define S6D0154_REG_MDDI_LINK_WAKEUP_START_POSITION 0x41
#define S6D0154_REG_SUB_PANEL_SELECTION_INDEX 0x42
#define S6D0154_REG_SUB_PANEL_DATA_WRITE_INDEX 0x43
#define S6D0154_REG_GPIO_VALUE 0x44
#define S6D0154_REG_GPIO_IO_CONTROL 0x45
#define S6D0154_REG_GPIO_CLEAR 0x46
#define S6D0154_REG_GPIO_INTERRUPT_ENABLE 0x47
#define S6D0154_REG_GPIO_POLARITY_SELECTION 0x48

#define S6D0154_REG_GAMMA_CONTROL_1 0x50
#define S6D0154_REG_GAMMA_CONTROL_2 0x51
#define S6D0154_REG_GAMMA_CONTROL_3 0x52
#define S6D0154_REG_GAMMA_CONTROL_4 0x53
#define S6D0154_REG_GAMMA_CONTROL_5 0x54
#define S6D0154_REG_GAMMA_CONTROL_6 0x55
#define S6D0154_REG_GAMMA_CONTROL_7 0x56
#define S6D0154_REG_GAMMA_CONTROL_8 0x57
#define S6D0154_REG_GAMMA_CONTROL_9 0x58
#define S6D0154_REG_GAMMA_CONTROL_10 0x59

#define S6D0154_REG_MTP_TEST_KEY 0x80
#define S6D0154_REG_MTP_CONTROL_REGISTERS 0x81
#define S6D0154_REG_MTP_DATA_READ_WRITE 0x82
#define S6D0154_REG_PRODUCT_NAME_VERSION_WRITE 0x83

#endif
