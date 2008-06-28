/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
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

unsigned isp1362_read_hc_reg16(unsigned reg);
unsigned isp1362_read_hc_reg32(unsigned reg);
void isp1362_write_hc_reg16(unsigned reg, unsigned data);
void isp1362_write_hc_reg32(unsigned reg, unsigned data);

#define ISP1362_OTG_CONTROL             0x62
#define ISP1362_OTG_STATUS              0x67 /* read only */
#define ISP1362_OTG_INTERRUPT           0x68
#define ISP1362_OTG_INT_ENABLE          0x69
#define ISP1362_OTG_TIMER               0x6a
#define ISP1362_OTG_ALT_TIMER           0x6c

#define ISP1362_HC_REVISION             0x00 /* read only */
#define ISP1362_HC_CONTROL              0x01
#define ISP1362_HC_COMMAND_STATUS       0x02
#define ISP1362_HC_INT_STATUS           0x03
#define ISP1362_HC_INT_ENABLE           0x04
#define ISP1362_HC_INT_DISABLE          0x05
#define ISP1362_HC_FM_INTERVAL          0x0d
#define ISP1362_HC_FM_REMAINING         0x0e
#define ISP1362_HC_FM_NUMBER            0x0f
#define ISP1362_HC_LS_THRESHOLD         0x11
#define ISP1362_HC_RH_DESCRIPTOR_A      0x12
#define ISP1362_HC_RH_DESCRIPTOR_B      0x13
#define ISP1362_HC_RH_STATUS            0x14
#define ISP1362_HC_RH_PORT_STATUS1      0x15
#define ISP1362_HC_RH_PORT_STATUS2      0x16
#define ISP1362_HC_HARDWARE_CONFIG      0x20
#define ISP1362_HC_DMA_CONFIG           0x21
#define ISP1362_HC_TRANSFER_COUNTER     0x22
#define ISP1362_HC_UP_INTERRUPT         0x24
#define ISP1362_HC_UP_INT_ENABLE        0x25
#define ISP1362_HC_CHIP_ID              0x27 /* read only */
#define ISP1362_HC_SCRATCH              0x28
#define ISP1362_HC_SOFTWARE_RESET       0x29 /* write only */
#define ISP1362_HC_BUFFER_STATUS        0x2c
#define ISP1362_HC_DIRECT_ADDR_LEN      0x32
#define ISP1362_HC_DIRECT_ADDR_DATA     0x45
#define ISP1362_HC_ISTL_BUF_SIZE        0x30
#define ISP1362_HC_ISTL0_BUF_PORT       0x40
#define ISP1362_HC_ISTL1_BUF_PORT       0x42
#define ISP1362_HC_ISTL_TOGGLE_RATE     0x47
#define ISP1362_HC_INTL_BUF_SIZE        0x33
#define ISP1362_HC_INTL_BUF_PORT        0x43
#define ISP1362_HC_INTL_BLK_SIZE        0x53
#define ISP1362_HC_INTL_PRD_DONE_MAP    0x17 /* read only */
#define ISP1362_HC_INTL_PTD_SKIP_MAP    0x18
#define ISP1362_HC_INTL_LAST_PTD        0x19
#define ISP1362_HC_INTL_CUR_ACT_PTD     0x1a /* read only */
#define ISP1362_HC_ATL_BUF_SIZE         0x34
#define ISP1362_HC_ATL_BUF_PORT         0x44
#define ISP1362_HC_ATL_BLK_SIZE         0x54
#define ISP1362_HC_ATL_PTD_DONE_MAP     0x1b /* read only */
#define ISP1362_HC_ATL_PTD_SKIP_MAP     0x1c
#define ISP1362_HC_ATL_LAST_PTD         0x1d
#define ISP1362_HC_ATL_CUR_ACT_PTD      0x1e /* read only */
#define ISP1362_HC_ATL_PTD_DONE_THR_CNT 0x51
#define ISP1362_HC_ATL_PTD_DONE_THR_TMO 0x52

unsigned isp1362_read_dc_reg16(unsigned reg);
unsigned isp1362_read_dc_reg32(unsigned reg);
void isp1362_write_dc_reg16(unsigned reg, unsigned data);
void isp1362_write_dc_reg32(unsigned reg, unsigned data);

#define ISP1362_DC_CTRL_OUT_CFG_W           0x20
#define ISP1362_DC_CTRL_IN_CFG_W            0x21
#define ISP1362_DC_ENDPOINT_CFG_BASE_W      0x22
#define ISP1362_DC_CTRL_OUT_CFG_R           0x30
#define ISP1362_DC_CTRL_IN_CFG_R            0x31
#define ISP1362_DC_ENDPOINT_CFG_BASE_R      0x32
#define ISP1362_DC_ADDRESS_W                0xb6
#define ISP1362_DC_ADDRESS_R                0xb7
#define ISP1362_DC_MODE_W                   0xb8
#define ISP1362_DC_MODE_R                   0xb9
#define ISP1362_DC_HARDWARE_CONFIG_W        0xba
#define ISP1362_DC_HARDWARE_CONFIG_R        0xbb
#define ISP1362_DC_INT_ENABLE_W             0xc2
#define ISP1362_DC_INT_ENABLE_R             0xc3
#define ISP1362_DC_DMA_CONFIG_W             0xf0
#define ISP1362_DC_DMA_CONFIG_R             0xf1
#define ISP1362_DC_DMA_COUNTER_W            0xf2
#define ISP1362_DC_DMA_COUNTER_R            0xf3
#define ISP1362_DC_RESET                    0xf6
#define ISP1362_DC_CTRL_IN_BUF_W            0x01
#define ISP1362_DC_ENDPOINT_BUF_BASE_W      0x02
#define ISP1362_DC_CTRL_OUT_BUF_R           0x10
#define ISP1362_DC_ENDPOINT_BUF_BASE_R      0x12
#define ISP1362_DC_CTRL_OUT_STALL           0x40
#define ISP1362_DC_CTRL_IN_STALL            0x41
#define ISP1362_DC_ENDPOINT_STALL_BASE      0x42
#define ISP1362_DC_CTRL_OUT_STATUS_R        0x50
#define ISP1362_DC_CTRL_IN_STATUS_R         0x51
#define ISP1362_DC_ENDPOINT_STATUS_BASE_R   0x52
#define ISP1362_DC_CTRL_IN_VALIDATE         0x61
#define ISP1362_DC_ENDPOINT_VALIDATE_BASE   0x62
#define ISP1362_DC_CTRL_OUT_CLEAR           0x70
#define ISP1362_DC_ENDPOINT_CLEAR_BASE      0x72
#define ISP1362_DC_CTRL_OUT_UNSTALL         0x80
#define ISP1362_DC_CTRL_IN_UNSTALL          0x81
#define ISP1362_DC_ENDPOINT_UNSTALL_BASE    0x82
#define ISP1362_DC_CTRL_OUT_STAT_IMG_R      0xd0
#define ISP1362_DC_CTRL_IN_STAT_IMG_R       0xd1
#define ISP1362_DC_ENDPOINT_STAT_IMG_BASE_R 0xd2
#define ISP1362_DC_SETUP_ACK                0xf4
#define ISP1362_DC_CTRL_OUT_ERROR_R         0xa0
#define ISP1362_DC_CTRL_IN_ERROR_R          0xa1
#define ISP1362_DC_ENDPOINT_ERROR_BASE_R    0xa2
#define ISP1362_DC_UNLOCK_DEVICE            0xb0
#define ISP1362_DC_SCRATCH_W                0xb2
#define ISP1362_DC_SCRATCH_R                0xb3
#define ISP1362_DC_FRAME_NUMBER_R           0xb4
#define ISP1362_DC_CHIP_ID_R                0xb5
#define ISP1362_DC_INTERRUPT_R              0xc0

void isp1362_init(void);
