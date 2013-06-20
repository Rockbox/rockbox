/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Lorenzo Miori
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
#include "config.h"
#include "system.h"
#include "gpmi-imx233.h"
#include "pinctrl-imx233.h"
#include "button-target.h"
#include "fat.h"
#include "disk.h"
#include "usb.h"
#include "debug.h"
#include "nand.h"
#include "storage.h"
#include "nand-ypz5.h"
#include "nand-target.h"
#include "nand_id.h"
#include "pinctrl-imx233.h"
#include "regs/regs-gpmi.h"
#include "regs/regs-clkctrl.h"

#include "button.h"
#include "font.h"
#include "action.h"

/* NOTE: this code is a very-wip one!! Don't care about it*/

// here begins the generic code to imx233-like platform

/* This stuff will belong to a specific file! */

void imx233_gpmi_wait_busy(void)
{
    while (BF_RD(GPMI_CTRL0, RUN));
}

void imx233_gpmi_wait_completition(void)
{
    while(BF_RD(GPMI_STAT, FIFO_EMPTY));
}

void imx233_gpmi_device_reset(bool enable)
{
    if (enable) BF_WR_V(GPMI_CTRL1, DEV_RESET, ENABLED);
    else BF_WR_V(GPMI_CTRL1, DEV_RESET, DISABLED);
}

/**
 * Setup NAND timings as well as other options
 */
void imx233_gpmi_timing_config(uint16_t busy_timeout,
                             uint8_t address_setup,
                             uint8_t data_hold,
                             uint8_t data_setup)
{
    BF_WR(GPMI_TIMING1, DEVICE_BUSY_TIMEOUT, busy_timeout);
    BF_WR(GPMI_TIMING0, ADDRESS_SETUP, address_setup);
    BF_WR(GPMI_TIMING0, DATA_HOLD, data_hold);
    BF_WR(GPMI_TIMING0, DATA_SETUP, data_setup);
}

// here begins the NAND code

void nand_setup_init(void)
{
    /* ... this too ... */
    *(volatile uint32_t*)0x80040000 = (1 << 16);
    udelay(10000);
    BF_WR(CLKCTRL_GPMI, DIV, 30);
    BF_WR(CLKCTRL_GPMI, CLKGATE, 0);
    while (BF_RD(CLKCTRL_GPMI, BUSY));
    udelay(10000);
    
    /* Let's begin setting up the necessary pins, but first
     make sure the NAND reset pin is low to avoid inadvertent
     operations during powerup and setup
     */
    imx233_gpmi_device_reset(true);

    /* I/O 8-bit width pins */
    //imx233_pinctrl_acquire_pin_mask(0, 0xff, "NAND data");
    imx233_pinctrl_set_function(0, 0, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 1, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 2, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 3, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 4, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 5, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 6, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 7, PINCTRL_FUNCTION_MAIN);

    /* Chip select pins [CE3-CE4 in order] */
    imx233_pinctrl_set_function(0, 14, PINCTRL_FUNCTION_ALT2);
    imx233_pinctrl_set_function(0, 15, PINCTRL_FUNCTION_ALT2);
    imx233_pinctrl_set_function(3, 0, PINCTRL_FUNCTION_ALT1);
    imx233_pinctrl_set_function(3, 1, PINCTRL_FUNCTION_ALT1);    

    /* GPMI A0/A1 - NAND CLE/ALE */
    imx233_pinctrl_set_function(0, 22, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_drive(0, 22, PINCTRL_DRIVE_8mA);
    imx233_pinctrl_set_function(0, 23, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_drive(0, 23, PINCTRL_DRIVE_8mA);

    /* NAND Status and interrupt */
    imx233_pinctrl_set_function(0, 16, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 17, PINCTRL_FUNCTION_MAIN); /* GPMI_RDn */
    imx233_pinctrl_set_drive(0, 17, PINCTRL_DRIVE_8mA);
    imx233_pinctrl_set_function(0, 18, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 19, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 20, PINCTRL_FUNCTION_MAIN);
    imx233_pinctrl_set_function(0, 21, PINCTRL_FUNCTION_MAIN); /* GPMI_WRn */
    imx233_pinctrl_set_drive(0, 21, PINCTRL_DRIVE_8mA);

    /* GPMI Reset Pin */
    imx233_pinctrl_set_function(1, 20, PINCTRL_FUNCTION_MAIN);

    /* Setup GPMI interface */

    imx233_reset_block(&HW_GPMI_CTRL0);
    BF_SET(GPMI_CTRL0, SFTRST);
    BF_CLR(GPMI_CTRL0, CLKGATE);
    BF_CLR(GPMI_CTRL0, SFTRST);
/*
tR = 25
tPROG = 200
tCBSY = 1
tBERS = 1500
 */
    /* Set NAND mode for GPMI interface */
    BF_WR_V(GPMI_CTRL1, GPMI_MODE, NAND);
    BF_WR(GPMI_CTRL1, CAMERA_MODE, 0);
    /* Interrupt polarity */
    BF_WR_V(GPMI_CTRL1, ATA_IRQRDY_POLARITY, ACTIVEHIGH);

    /* Setup NAND timings and other parameters */
    BF_WR(GPMI_CTRL1, DSAMPLE_TIME, 0); // check
    imx233_gpmi_timing_config(50000, 1, 2, 4);
    
    /* Finally disable reset pin */
    imx233_gpmi_device_reset(false);
}

static uint32_t nand_send_cmd_addr(uint32_t bank, uint8_t cmd, uint8_t addr)
{
    imx233_gpmi_wait_busy();
    /* CLE high -> write command, 0x90
     * Then for auto-increment it goes to
     * ALE high -> write address 0x00
     */
    HW_GPMI_CTRL0 = BF_OR8(GPMI_CTRL0, RUN(1), COMMAND_MODE(0), ADDRESS(1), CS(bank), LOCK_CS(0), WORD_LENGTH(1), ADDRESS_INCREMENT(1), XFER_COUNT(2));
    HW_GPMI_DATA = (addr << 8) | cmd;
    imx233_gpmi_wait_busy();
    return 0;
}

static uint32_t nand_send_cmd(uint32_t bank, uint8_t cmd)
{
    imx233_gpmi_wait_busy();
    /* CLE high -> write command, 0x90
     * Then for auto-increment it goes to
     * ALE high -> write address 0x00
     */
    HW_GPMI_CTRL0 = BF_OR8(GPMI_CTRL0, RUN(1), COMMAND_MODE(0), ADDRESS(1), CS(bank), LOCK_CS(0), WORD_LENGTH(1), ADDRESS_INCREMENT(0), XFER_COUNT(1));
    HW_GPMI_DATA = cmd;
    imx233_gpmi_wait_busy();
    return 0;
}

static uint32_t nand_send_address(uint32_t bank, uint8_t addr)
{
    imx233_gpmi_wait_busy();
    HW_GPMI_CTRL0 = BF_OR8(GPMI_CTRL0,
                           RUN(1),
                           COMMAND_MODE(0),
                           ADDRESS(2),
                           CS(bank),
                           LOCK_CS(0),
                           WORD_LENGTH(1),
                           ADDRESS_INCREMENT(0),
                           XFER_COUNT(1));
    HW_GPMI_DATA = addr;
    imx233_gpmi_wait_busy();
    return 0;
}

static uint32_t nand_transfer_data(uint32_t bank, bool write,
                                     void* buffer, uint16_t size)
{
    int left = size;
    int i = 0;
    HW_GPMI_CTRL0 = BF_OR8(GPMI_CTRL0,
                           RUN(1),
                           COMMAND_MODE(1),
                           ADDRESS(0),
                           CS(bank),
                           LOCK_CS(0),
                           WORD_LENGTH(1),
                           ADDRESS_INCREMENT(0),
                           XFER_COUNT(size));
    imx233_gpmi_wait_completition();
    
    while (left > 0)
    {
        if (write)
        {
            HW_GPMI_DATA = *(char*)buffer;
            left -= 1;
            buffer += 1;
        }
        else
        {
            uint32_t temp = HW_GPMI_DATA;
            //uint32_t byte;
            memcpy(buffer, &temp, MIN(left, 4));
//             for (byte = 0; (byte < 4) && (left < size); byte++)
//             {
//                 *((char*)buffer++) = (uint8_t) (0x000000FF & (temp >> (byte * 8)));
//                 left--;
//             }
            left -= MIN(left, 4);
            buffer += MIN(left, 4);
        }
        i++;
    }
    
    return 0;
}

/**
 * Retrieve NAND ID
 * bank: chip selection
 * returns ID
 */
uint32_t nand_get_chip_type(int bank)
{
    uint32_t ret;
    //nand_send_cmd_addr(bank, NAND_READ_ID, 0);
    nand_send_cmd(bank, NAND_READ_ID);
    nand_send_address(bank, 0);
    nand_transfer_data(bank, false, &ret, 4);
    return ret;
}

static const struct nand_device_info_type nand_deviceinfotable[] =
{
    /**
     * Let's strip last byte, doesn't really help
     *  0xECD3552558
        0xECD3142564
    */
    // sizes and parameters are still dummy ;)
    // 2,45% is the percentage of spare blocks -> MaxNumBadBlocks = 160 for these nands
    //{0xECD3519558, 1024, 968, 0x40, 6, 2, 1, 2, 1},
    // tPROG = 0.95 - 2 ms; tDBSY = 0.5 - 1 us; Nop = 1; tBERS = 1.5 - 10 ms
    // id, total blocks, pages per block, 
    {0x2555D3EC, 4096, 128, 128, 6, 2, 1, 2, 1}, // K9HAG08U1M (4GB model, 4 chips) //58h
    
    {0x2514D3EC, 1024, 968, 0x40, 6, 2, 1, 2, 1}, // K9LAG08U0M (1GB model, 1 chip)
    //{0xECD5552568, 1024, 968, 0x40, 6, 2, 1, 2, 1},
};

void nand_power_down(void)
{
    /* TODO */
}

void nand_set_active(void)
{
    /* TODO */
}

long nand_last_activity(void)
{
    return 0;
}

int nand_spinup_time(void)
{
    return 0;
}

static uint8_t nand_tunk1[4];
static uint8_t nand_twp[4];
static uint8_t nand_tunk2[4];
static uint8_t nand_tunk3[4];
static int nand_type[4];
static int nand_interleaved = 0;
static int nand_cached = 0;

static uint8_t nand_data[0x800] STORAGE_ALIGN_ATTR;
static uint8_t nand_ctrl[0x200] STORAGE_ALIGN_ATTR;
static uint8_t nand_spare[0x40] STORAGE_ALIGN_ATTR;
static uint8_t nand_ecc[0x30] STORAGE_ALIGN_ATTR;

uint32_t nand_device_init(void)
{
    uint32_t type;
    uint32_t i, j;

    /* Assume there are 0 banks initially */
    for (i = 0; i < 4; i++) nand_type[i] = -1;
    nand_setup_init();

    /* Now that the flash is powered on, detect how
       many banks we really have and initialize them. */
    for (i = 0; i < 4; i++)
    {
        nand_tunk1[i] = 7;
        nand_twp[i] = 7;
        nand_tunk2[i] = 7;
        nand_tunk3[i] = 7;
        type = nand_get_chip_type(i);
        if (type >= 0xFFFFFFF0)
        {
            nand_type[i] = (int)type;
            continue;
        }
        for (j = 0; ; j++)
        {
            if (j == ARRAYLEN(nand_deviceinfotable)) break;
            else if (nand_deviceinfotable[j].id == type)
            {
                nand_type[i] = j;
                break;
            }
        }
        nand_tunk1[i] = nand_deviceinfotable[nand_type[i]].tunk1;
        nand_twp[i] = nand_deviceinfotable[nand_type[i]].twp;
        nand_tunk2[i] = nand_deviceinfotable[nand_type[i]].tunk2;
        nand_tunk3[i] = nand_deviceinfotable[nand_type[i]].tunk3;
    }
    if (nand_type[0] < 0) return nand_type[0];
    nand_interleaved = ((nand_deviceinfotable[nand_type[0]].id >> 22) & 1);
    nand_cached = ((nand_deviceinfotable[nand_type[0]].id >> 23) & 1);

//     nand_last_activity_value = current_tick;
//     create_thread(nand_thread, nand_stack,
//                   sizeof(nand_stack), 0, "nand"
//                   IF_PRIO(, PRIORITY_USER_INTERFACE)
//                   IF_COP(, CPU));

    return 0;
}

uint32_t nand_read_page(uint32_t bank, uint32_t page, void* databuffer,
                        void* sparebuffer, uint32_t doecc,
                        uint32_t checkempty)
{
    uint8_t* data = databuffer;
    uint8_t* spare = nand_spare;
//     if (databuffer && !((uint32_t)databuffer & 0xf))
//         data = (uint8_t*)databuffer;
//     if (sparebuffer && !((uint32_t)sparebuffer & 0xf))
//         spare = (uint8_t*)sparebuffer;
//     mutex_lock(&nand_mtx);
//     nand_last_activity_value = current_tick;
//     led(true);
//     if (!nand_powered) nand_power_up();
    uint32_t rc, eccresult;
//     switch (f->DataSize) {
//         case 1024: ROW_SHIFT = 11; BLK_SHIFT = (11 + 7); break;
//         case 2048: ROW_SHIFT = 12; BLK_SHIFT = (12 + 7); break;
//         case 4096: ROW_SHIFT = 13; BLK_SHIFT = (13 + 7); break;
//         case 8192: ROW_SHIFT = 14; BLK_SHIFT = (14 + 7); break;
//     row->byte1 = ((reg8_t) ((offset >> (ROW_SHIFT - 4)) & 0xFF));
//     row->byte2 = ((reg8_t) ((offset >> (ROW_SHIFT + 4)) & 0xFF));
//     row->byte3 = ((reg8_t) ((offset >> (ROW_SHIFT + 12)) & 0x0F));
//     
//     col->byte1 = ((reg8_t) ((offset <<  4) & 0xFF));
//     col->byte2 = ((reg8_t) ((offset >>  4) & NAND_PAGE_MASK));
//     
#define ROW_SHIFT   13
    nand_send_cmd(bank, NAND_READ);
    /* Column address 1 */
    nand_send_address(bank, 0);
    /* Column address 2 */
    nand_send_address(bank, 0);
    /* Row address 1 */
    nand_send_address(bank, (uint8_t) ((page >> (ROW_SHIFT - 4)) & 0xFF));
    /* Row address 2 */
    nand_send_address(bank, (uint8_t) ((page >> (ROW_SHIFT + 4)) & 0xFF));
    /* Row address 3 */
    nand_send_address(bank, (uint8_t) ((page >> (ROW_SHIFT + 12)) & 0x0F));
        //return nand_unlock(1);
//    if (nand_send_address(page, databuffer ? 0 : 0x800))
//        return nand_unlock(1);
    nand_send_cmd(bank, NAND_READ_ADDRESS);
//    if (nand_wait_status_ready(bank))
//        return nand_unlock(1);
//    if (databuffer)
    /* tR ~ 50 us */
    udelay(75);
    /* Read <size of the page> data */
    nand_transfer_data(bank, 0, nand_data, 0x800);
    rc = 0;
//     if (!doecc)
//     {
//         if (databuffer && data != databuffer) memcpy(databuffer, data, 0x800);
//         if (sparebuffer)
//         {
//             if (nand_transfer_data(bank, 0, spare, 0x40))
//                 return nand_unlock(1);
//             if (sparebuffer && spare != sparebuffer) 
//                 memcpy(sparebuffer, spare, 0x800);
//             if (checkempty)
//                 rc = nand_check_empty((uint8_t*)sparebuffer) << 1;
//         }
//         return nand_unlock(rc);
//     }
    /* Follows <spare area size> data read */
    nand_transfer_data(bank, false, nand_spare, 0x40);
//     if (databuffer)
//     {
//         memcpy(nand_ecc, &spare[0xC], 0x28);
//         rc |= (ecc_decode(3, data, nand_ecc) & 0xF) << 4;
//         if (data != databuffer) memcpy(databuffer, data, 0x800);
//     }
//     memset(nand_ctrl, 0xFF, 0x200);
//     memcpy(nand_ctrl, spare, 0xC);
//     memcpy(nand_ecc, &spare[0x34], 0xC);
//     eccresult = ecc_decode(0, nand_ctrl, nand_ecc);
//     rc |= (eccresult & 0xF) << 8;
//     if (sparebuffer)
//     {
//         if (spare != sparebuffer) memcpy(sparebuffer, spare, 0x40);
//         if (eccresult & 1) memset(sparebuffer, 0xFF, 0xC);
//         else memcpy(sparebuffer, nand_ctrl, 0xC);
//     }
//     if (checkempty) rc |= nand_check_empty(spare) << 1;
// 
//     return nand_unlock(rc);
        return 0;
}

bool dbg_hw_info_nand(void)
{
    
    nand_device_init();
    
    lcd_setfont(FONT_SYSFIXED);

    unsigned current_page = 0;
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
                current_page++;
                break;
            case ACTION_STD_PREV:
                if(current_page > 0)
                    current_page--;
                break;
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        for(int i = 0; i < 4; i++)
            lcd_putsf(0, i, "BANK%d = 0x%08x", i, nand_deviceinfotable[nand_type[i]].id);
        memset(&nand_spare, 0, 0x40);
        memset(&nand_data, 0, 0x800);
        nand_read_page(0, current_page, nand_data, nand_spare, 0, 0);
//        nand_get_chip_type(0);
//        for (int x = 0; x < 0x800; x++)
//        int di=0;
//            lcd_putsf(0, 6, "D: %c %c %c %c %c", nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++]);
//            lcd_putsf(0, 7, "D: %c %c %c %c %c", nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++]);
//            lcd_putsf(0, 8, "D: %c %c %c %c %c", nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++]);
//            lcd_putsf(0, 9, "D: %c %c %c %c %c", nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++], nand_data[di++]);
//          unsigned cur_line = 6;
//          unsigned last_line = lcd_getheight() / font_get(lcd_getfont())->height;
//          unsigned cur_idx = 0;
//          if(cur_idx++ >= top_user && cur_line < last_line)
//                  lcd_putsf(0, cur_line++, "DATA: %c %d", (char)(nand_data[top_user]), (nand_data[top_user]));
//          if(cur_idx < top_user)
//              top_user = cur_idx - 1;
        lcd_update();
        yield();
    }
}

#if 0
Samsung K9F1G08U0A,1,5,ECF1801540,0,0,MP000
Samsung K9F1G08U0M,1,5,ECF1801540,0,0,MP000
Samsung K9F2G08U0A,1,5,ECDA109544,0,0,MP000
Samsung K9F2G08U0M,1,5,ECDA801550,0,0,MP000
Samsung K9F4G08U0A,1,5,ECDC109554,0,0,MP000
Samsung K9F4G08U0M,1,5,ECDC109554,0,0,MP000
Samsung K9F8G08U0M,1,5,ECD310A664,0,0,MP000
Samsung K9G2G08U0M,1,5,ECDA142544,0,0,MP000
Samsung K9G4G08U0A,1,5,ECDC142554,0,0,MP000
Samsung K9G4G08UOM,1,5,ECDC142554,0,0,MP000
Samsung K9G8G08U0A,1,5,ECD314A564,0,0,MP000
Samsung K9G8G08U0M,1,5,ECD3142564,0,0,MP000
Samsung K9GAG08U0M,1,5,ECD514B674,0,0,MP000
Samsung K9HBG08U1A,2,5,ECD555A568,0,0,MP000
Samsung K9HBG08U1M,2,5,ECD5552568,0,0,MP000
Samsung K9HCG08U1M,2,5,ECD755B678,0,0,MP000
Samsung K9HCG08U5M,4,5,ECD514B674,0,0,MP000
Samsung K9K4G08U0M,1,5,ECDCC11554,0,0,MP000
Samsung K9K8G08U0A,1,5,ECD3519558,0,0,MP000
Samsung K9K8G08U0M,1,5,ECD3519558,0,0,MP000
Samsung K9KAG08U0M,1,5,ECD551A668,0,0,MP000
Samsung K9L8G08U0A,1,5,ECD3552558,0,0,MP000
Samsung K9L8G08UOM,1,5,ECD3552558,0,0,MP000
Samsung K9LAG08U0A,1,5,ECD555A568,0,0,MP000
Samsung K9LAG08U0M,1,5,ECD5552568,0,0,MP000
Samsung K9LBG08U0M,1,5,ECD755B678,0,0,MP000
Samsung K9MBG08U5M,4,5,ECD3552558,0,0,MP000
Samsung K9MDG08U5M,4,5,ECD755B678,0,0,MP000
Samsung K9MDG08U5M,4,5,ECD755B678,0,0,MP000
Samsung K9NBG08U5A,4,5,ECD3519558,0,0,MP000
Samsung K9NBG08U5M,4,5,ECD3519558,0,0,MP000
Samsung K9W8G08U1M,2,5,ECDCC11554,0,0,MP000
Samsung K9WAG08U1A,2,5,ECD3519558,0,0,MP000
Samsung K9WAG08U1M,2,5,ECD3519558,0,0,MP000
Samsung K9WBG08U1M,2,5,ECD551A668,0,0,MP000
#endif