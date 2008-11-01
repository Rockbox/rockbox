/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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

/* Driver for the ARM PL180 SD/MMC controller inside AS3525 SoC */

#include "config.h" /* for HAVE_MULTIVOLUME */

#include "as3525.h"
#include "pl180.h"
#include "panic.h"
#include "stdbool.h"
#include "ata.h"

#define     NAND_AS3525 0
#define     SD_AS3525   1
static const int pl180_base[2] = { NAND_FLASH_BASE, SD_MCI_BASE };

/* ARM PL180 registers */
#define MMC_POWER(i)       (*(volatile unsigned long *) (pl180_base[i]+0x00))
#define MMC_CLOCK(i)       (*(volatile unsigned long *) (pl180_base[i]+0x04))
#define MMC_ARGUMENT(i)    (*(volatile unsigned long *) (pl180_base[i]+0x08))
#define MMC_COMMAND(i)     (*(volatile unsigned long *) (pl180_base[i]+0x0C))
#define MMC_RESPCMD(i)     (*(volatile unsigned long *) (pl180_base[i]+0x10))
#define MMC_RESP0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x14))
#define MMC_RESP1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x18))
#define MMC_RESP2(i)       (*(volatile unsigned long *) (pl180_base[i]+0x1C))
#define MMC_RESP3(i)       (*(volatile unsigned long *) (pl180_base[i]+0x20))
#define MMC_DATACTRL(i)    (*(volatile unsigned long *) (pl180_base[i]+0x2C))
#define MMC_STATUS(i)      (*(volatile unsigned long *) (pl180_base[i]+0x34))
#define MMC_CLEAR(i)       (*(volatile unsigned long *) (pl180_base[i]+0x38))
#define MMC_MASK0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x3C))
#define MMC_MASK1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x40))
#define MMC_SELECT(i)      (*(volatile unsigned long *) (pl180_base[i]+0x44))


/* SD commands */
#define GO_IDLE_STATE       0
#define MMC_CMD_READ_CID    2
#define SEND_IF_COND        8
#define SEND_OP_COND        41
#define APP_CMD             55

/* command flags */
#define MMC_NO_FLAGS    (0<<0)
#define MMC_RESP        (1<<0)
#define MMC_LONG_RESP   (1<<1)
#define MMC_ARG         (1<<2)

#ifdef BOOTLOADER
#define DEBUG
void reset_screen(void);
void printf(const char *format, ...);
#endif

struct mmc_command
{
    int cmd;
    int arg;
    int resp[4];
    int flags;
};

static inline void mci_delay(void) { int i = 0xffff; while(i--) ; }

static void mci_set_clock_divider(const int drive, int divider)
{
    int clock = MMC_CLOCK(drive);

    if(divider > 1)
    {
        /* use divide logic */
        clock &= ~MCI_CLOCK_BYPASS;

        /* convert divider to MMC_CLOCK logic */
        divider = (divider/2) - 1;
        if(divider >= 256)
            divider = 255;
    }
    else
    {
        /* bypass dividing logic */
        clock |= MCI_CLOCK_BYPASS;
        divider = 0;
    }

    MMC_CLOCK(drive) = clock | divider;

    mci_delay();
}

static int send_cmd(const int drive, struct mmc_command *cmd)
{
    int val, status;

    while(MMC_STATUS(drive) & MCI_CMD_ACTIVE); /* useless */

    if(MMC_COMMAND(drive) & MCI_COMMAND_ENABLE) /* clears existing command */
    {
        MMC_COMMAND(drive) = 0;
        mci_delay();
    }

    val = cmd->cmd | MCI_COMMAND_ENABLE;
    if(cmd->flags & MMC_RESP)
    {
        val |= MCI_COMMAND_RESPONSE;
        if(cmd->flags & MMC_LONG_RESP)
            val |= MCI_COMMAND_LONG_RESPONSE;
    }

    MMC_CLEAR(drive) = 0x7ff;

    MMC_ARGUMENT(drive) = (cmd->flags & MMC_ARG) ? cmd->arg : 0;
    MMC_COMMAND(drive) = val;

    while(MMC_STATUS(drive) & MCI_CMD_ACTIVE);

    MMC_COMMAND(drive) = 0;
    MMC_ARGUMENT(drive) = ~0;

    do
    {
        status = MMC_STATUS(drive);
        if(cmd->flags & MMC_RESP)
        {
            if(status & MCI_CMD_TIMEOUT)
            {
                if(cmd->cmd == SEND_IF_COND)
                    break; /* SDHC test can fail */
                panicf("Response timeout");
            }
            else if(status & (MCI_CMD_CRC_FAIL|MCI_CMD_RESP_END))
            {   /* resp received */
                cmd->resp[0] = MMC_RESP0(drive);
                if(cmd->flags & MMC_LONG_RESP)
                {
                    cmd->resp[1] = MMC_RESP1(drive);
                    cmd->resp[2] = MMC_RESP2(drive);
                    cmd->resp[3] = MMC_RESP3(drive);
                }
                break;
            }
        }
        else
            if(status & MCI_CMD_SENT)
                break;

    } while(1);

    MMC_CLEAR(drive) = 0x7ff;
    return status;
}

static void sd_init_card(const int drive)
{
    struct mmc_command cmd_app, cmd_op_cond, cmd_idle, cmd_if_cond;
    int status;
    bool sdhc;

#ifdef DEBUG
    reset_screen();
    printf("now - powered up");
#endif

    cmd_idle.cmd = GO_IDLE_STATE;
    cmd_idle.arg = 0;
    cmd_idle.flags = MMC_NO_FLAGS;
    if(send_cmd(drive, &cmd_idle) != MCI_CMD_SENT)
        panicf("goto idle failed!");
#ifdef DEBUG
    else
        printf("now - idle");
#endif

    mci_delay();

    cmd_if_cond.cmd = SEND_IF_COND;
    cmd_if_cond.arg = (1 /* 2.7-3.6V */ << 8) | 0xAA /* check pattern */;
    cmd_if_cond.flags = MMC_RESP | MMC_ARG;

    cmd_app.cmd = APP_CMD;
    cmd_app.flags = MMC_RESP | MMC_ARG;
    cmd_app.arg = 0; /* 31:16 RCA (0) , 15:0 stuff bits */

    cmd_op_cond.cmd = SEND_OP_COND;
    cmd_op_cond.flags = MMC_RESP | MMC_ARG;

#ifdef DEBUG
    printf("now - card powering up");
#endif

    sdhc = false;
    status = send_cmd(drive, &cmd_if_cond);
    if(status & (MCI_CMD_CRC_FAIL|MCI_CMD_RESP_END))
    {
        if((cmd_if_cond.resp[0] & 0xFFF) == cmd_if_cond.arg)
            sdhc = true;
#ifdef DEBUG
        else
            printf("Bad resp: %x",cmd_if_cond.arg);
#endif
    }
#ifdef DEBUG
    else
        printf("cmd_if_cond stat: 0x%x",status);

    printf("%s Capacity",sdhc?"High":"Normal");
    mci_delay();
    mci_delay();
    mci_delay();
#endif

#ifdef DEBUG
    int loop = 0;
#endif
    do {
        mci_delay();
        mci_delay();
#ifdef DEBUG
        reset_screen();
        printf("Loop number #%d", ++loop);
#endif
        /* app_cmd */
        status = send_cmd(drive, &cmd_app);
        if( !(status & (MCI_CMD_CRC_FAIL|MCI_CMD_RESP_END)) ||
            !(cmd_app.resp[0] & (1<<5)) )
        {
            panicf("app_cmd failed");
        }

        cmd_op_cond.arg = sdhc ? 0x40FF8000 : (8<<0x14); /* ocr */
        status = send_cmd(drive, &cmd_op_cond);
        if(!(status & (MCI_CMD_CRC_FAIL|MCI_CMD_RESP_END)))
            panicf("cmd_op_cond failed");

#ifdef DEBUG
        printf("OP COND: 0x%.8x", cmd_op_cond.resp[0]);
#endif
    } while(!(cmd_op_cond.resp[0] & (1<<31))); /* until card is powered up */

#ifdef DEBUG
    printf("now - card ready !");
#endif
}

static void init_pl180_controller(const int drive)
{
    MMC_COMMAND(drive) = MMC_DATACTRL(drive) = 0;
    MMC_CLEAR(drive) = 0x7ff;

    MMC_MASK0(drive) = MMC_MASK1(drive) = 0;  /* disable all interrupts */

    MMC_POWER(drive) = MCI_POWER_UP|(10 /*voltage*/ << 2); /* use OF voltage */
    mci_delay();

    MMC_POWER(drive) |= MCI_POWER_ON;
    mci_delay();

    MMC_SELECT(drive) = 0;

    MMC_CLOCK(drive) = MCI_CLOCK_ENABLE;
    MMC_CLOCK(drive) &= ~MCI_CLOCK_POWERSAVE;

    /* set MCLK divider */
    mci_set_clock_divider(drive, 200);
}

int ata_init(void)
{
    /* reset peripherals */

    CCU_SRC =
#ifdef HAVE_MULTIVOLUME
        CCU_SRC_SDMCI_EN |
#endif
        CCU_SRC_NAF_EN | CCU_SRC_IDE_EN | CCU_SRC_IDE_AHB_EN | CCU_SRC_MST_EN;

    CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    CCU_SRL = 0;

    GPIOC_DIR &= ~(1<<1);
    if(GPIOC_PIN(1))
        CCU_SPARE1 |= 4;    /* sets bit 2 of undocumented register */
    else
        CCU_SPARE1 &= ~4;   /* or clear it */

    CGU_IDE = (1<<7)|(1<<6);  /* enable, 24MHz clock */
    CGU_MEMSTICK = (1<<8);    /* enable, 24MHz clock */

    CGU_PERI |= CGU_NAF_CLOCK_ENABLE;
#ifdef HAVE_MULTIVOLUME
    CGU_PERI |= CGU_MCI_CLOCK_ENABLE;
#endif

    CCU_IO &= ~8;           /* bits 3:2 = 01, xpd is SD interface */
    CCU_IO |= 4;

    init_pl180_controller(NAND_AS3525);
    sd_init_card(NAND_AS3525);

#ifdef HAVE_MULTIVOLUME
    init_pl180_controller(SD_AS3525);
    sd_init_card(SD_AS3525);
#endif

    return 0;
}

int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return 0; /* TODO */
}

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return 0; /* TODO */
}
