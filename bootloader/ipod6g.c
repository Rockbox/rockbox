/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"

#include "inttypes.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "../kernel-internal.h"
#include "file_internal.h"
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "version.h"

#include "s5l8702.h"
#include "pmu-target.h"
#include "i2c-s5l8702.h"
#include "clocking-s5l8702.h"
#include "crypto-s5l8702.h"
#include "cwheel-s5l8702.h"
#include "piezo.h"


/*
 * Preliminary HW initialization
 */

/* -- clocks and power -- */
void syscon_preinit(void)
{
    /* at this moment CG16_SYS is using PLL0 @108 MHz
       CClk = 108 MHz, HClk = 54 MHz, PClk = 27 MHz */

    CLKCON0 &= ~CLKCON0_SDR_DISABLE_BIT;

    PLLMODE &= ~PLLMODE_OSCSEL_BIT; /* CG16_SEL_OSC = OSC0 */
    cg16_config(&CG16_SYS, true, CG16_SEL_OSC, 1, 1);
    soc_set_system_divs(1, 1, 1);

    /* stop all PLLs */
    for (int pll = 0; pll < 3; pll++)
        pll_onoff(pll, false);

    pll_config(2, PLLOP_DM, 1, 36, 1, 32400);
    pll_onoff(2, true);
    soc_set_system_divs(1, 2, 2 /*hprat*/);
    cg16_config(&CG16_SYS,   true,  CG16_SEL_PLL2, 1, 1);
    cg16_config(&CG16_2L,    false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_SVID,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_AUD0,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_AUD1,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_AUD2,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_RTIME, true,  CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_5L,    false, CG16_SEL_OSC,  1, 1);

    soc_set_hsdiv(1);

    PWRCON_AHB = ~((1 << CLOCKGATE_SMx) |
                   (1 << CLOCKGATE_SM1));
    PWRCON_APB = ~((1 << (CLOCKGATE_TIMER - 32)) |
                   (1 << (CLOCKGATE_GPIO - 32)));
}

/* -- GPIO -- */
static uint32_t gpio_data[16] =
{
    0x5322222F, 0xEEEEEE00, 0x2332EEEE, 0x3333E222,
    0x33333333, 0x33333333, 0x3F000E33, 0xEEEEEEEE,
    0xEEEEEEEE, 0xEEEEEEEE, 0xE0EEEEEE, 0xEE00EE0E,
    0xEEEE0EEE, 0xEEEEEEEE, 0xEE2222EE, 0xEEEE0EEE
};

void gpio_preinit(void)
{
    for (int i = 0; i < 16; i++) {
        PCON(i) = gpio_data[i];
        PUNB(i) = 0;
        PUNC(i) = 0;
    }
}

/* -- i2c -- */
void i2c_preinit(int bus)
{
    #define wait_rdy(bus)   {while(IICUNK10(bus));}

    clockgate_enable(CLOCKGATE_I2C0, true);
    wait_rdy(bus);
    IICADD(bus) = 0x40;   /* own slave address */
    wait_rdy(bus);
    IICUNK14(bus) = 0;
    wait_rdy(bus);
    IICUNK18(bus) = 0;
    wait_rdy(bus);
    IICSTAT(bus) = 0x80;  /* master Rx mode, Tx/Rx off */
    wait_rdy(bus);
    IICCON(bus) = 0;
    wait_rdy(bus);
    IICSTAT(bus) = 0;     /* slave Rx mode, Tx/Rx off */
    wait_rdy(bus);
    IICDS(bus) = 0x40;
    wait_rdy(bus);
    IICCON(bus) = (1 << 8) |  /* unknown */
                  (1 << 7) |  /* ACK_GEN */
                  (0 << 6) |  /* CLKSEL = PCLK/16 */
                  (0 << 5) |  /* INT_EN = disabled */
                  (4 << 0);   /* CK_REG */
    wait_rdy(bus);
    IICSTAT(bus) = 0x10;  /* slave Rx mode, Tx/Rx on */
    wait_rdy(bus);
    clockgate_enable(CLOCKGATE_I2C0, false);
}

/* -- PMU -- */
#include "pcf5063x.h"
void pmu_preinit(void)
{
    unsigned char rd_buf[8];

    /* reset OOC shutdown register */
    pmu_wr(PCF5063X_REG_OOCSHDWN, 0);

    /* configure LDOs (mV = 900+100*val) */
    pmu_wr_multiple(PCF5063X_REG_LDO1OUT, 14,
            "\x15\x01"    /* LDO_UNK1:   3000 mV, enabled */
            "\x15\x01"    /* LDO_UNK2:   3000 mV, enabled */
            "\x15\x01"    /* LDO_LCD:    3000 mV, enabled */
            "\x09\x01"    /* LDO_CODEC:  1800 mV, enabled */
            "\x15\x01"    /* LDO_UNK5:   3000 mV, disabled */
            "\x15\x04"    /* LDO_CWHEEL: 3000 mV, ON when GPIO2==1 */
            "\x18\x00");  /* LDO_ACCY:   3300 mV, disabled */

    /* LDO_CWHEEL is ON in standby state,
       LDO_CWHEEL and MEMLDO are ON in UNKNOWN state (TBC) */
    pmu_wr_multiple(PCF5063X_REG_STBYCTL1, 2, "\x00\x8C");

    /* GPIO1,2 = input, GPIO3 = output */
    pmu_wr_multiple(PCF5063X_REG_GPIOCTL, 3, "\x03\x00\x00");

    /* GPO selection = fixed 0 */
    pmu_wr(PCF5063X_REG_GPOCFG, 0);

    /* DOWN2 converter (SDRAM): 1800 mV (625+25*val), enabled,
       startup current limit = 15mA*0x10 (TBC) */
    pmu_wr_multiple(PCF5063X_REG_DOWN2OUT, 4, "\x2F\x01\x00\x10");
    /* MEMLDO: 1800 mV (900+100*val), enabled */
    pmu_wr_multiple(PCF5063X_REG_MEMLDOOUT, 2, "\x09\x01");

    /* AUTOLDO (HDD): 3400 mV (625+25*val), disabled,
       current limit = 40mA*0x19 = 1000 mA, limit always active */
    pmu_wr_multiple(PCF5063X_REG_AUTOOUT, 4, "\x6F\x00\x00\x59");

    /* Vbatok = 3.2V, Vsysok = 2.9V, 62ms debounce filter enabled */
    pmu_wr_multiple(PCF5063X_REG_BVMCTL, 2, "\x0A\x04");

    pmu_wr(0x58, 0); /* reserved */

    /* configure PMU interrupt masks */
    pmu_wr_multiple(PCF5063X_REG_INT1M, 5, "\xB0\x03\xFE\xFC\xFF");
    pmu_wr(IPOD6G_PCF50635_REG_INT6M, 0xFD);

    /* OCC: all enabled (!ONKEY, EXTON1..3, RTC, USB, ADAPTER),
       config debounce time, activation phases and internal CLK32K,
       wakeup on rising edge for EXTON1 and EXTON2,
       wakeup on falling edge for EXTON3 and !ONKEY */
    pmu_wr_multiple(PCF5063X_REG_OOCWAKE, 5, "\xDF\xAA\x4A\x05\x27");

    /* clear PMU interrupts */
    pmu_rd_multiple(PCF5063X_REG_INT1, 5, rd_buf);
    pmu_rd(IPOD6G_PCF50635_REG_INT6);

    /* GPO selection = LED external NFET drive signal */
    pmu_wr(PCF5063X_REG_GPOCFG, 1);
    /* LED converter OFF, overvoltage protection enabled,
       OCP limit is 500 mA, led_dimstep = 16 * 0x06 / 32768 */
    pmu_wr_multiple(PCF5063X_REG_LEDENA, 3, "\x00\x05\x06");
}

/* -- MIU -- */
void miu_preinit(bool selfrefreshing)
{
    if (selfrefreshing)
        MIUCON = 0x11;      /* TBC: self-refresh -> IDLE */

    MIUCON = 0x80D;         /* remap = 1 (IRAM mapped to 0x0),
                               TBC: SDRAM bank and column configuration */
    MIU_REG(0xF0) = 0x0;

    MIUAREF = 0x6105D;      /* Auto-Refresh enabled,
                               Row refresh interval = 0x5d/12MHz = 7.75 uS */
    MIUSDPARA = 0x1FB621;

    MIU_REG(0x200) = 0x1845;
    MIU_REG(0x204) = 0x1845;
    MIU_REG(0x210) = 0x1800;
    MIU_REG(0x214) = 0x1800;
    MIU_REG(0x220) = 0x1845;
    MIU_REG(0x224) = 0x1845;
    MIU_REG(0x230) = 0x1885;
    MIU_REG(0x234) = 0x1885;
    MIU_REG(0x14) = 0x19;       /* 2^19 = 0x2000000 = SDRAMSIZE (32Mb) */
    MIU_REG(0x18) = 0x19;       /* 2^19 = 0x2000000 = SDRAMSIZE (32Mb) */
    MIU_REG(0x1C) = 0x790682B;
    MIU_REG(0x314) &= ~0x10;

    for (int i = 0; i < 0x24; i++)
        MIU_REG(0x2C + i*4) &= ~(1 << 24);

    MIU_REG(0x1CC) = 0x540;
    MIU_REG(0x1D4) |= 0x80;

    MIUCOM = 0x33;     /* No action CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x233;    /* Precharge all banks CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x333;    /* Auto-refresh CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x333;    /* Auto-refresh CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x33;

    if (!selfrefreshing)
    {
        MIUMRS = 0x33;    /* MRS: Bust Length = 8, CAS = 3 */
        MIUCOM = 0x133;   /* Mode Register Set CMD */
        MIUCOM = 0x33;
        MIUCOM = 0x33;
        MIUCOM = 0x33;
        MIUMRS = 0x8040;  /* EMRS: Strength = 1/4, Self refresh area = Full */
        MIUCOM = 0x133;   /* Mode Register Set CMD */
        MIUCOM = 0x33;
        MIUCOM = 0x33;
        MIUCOM = 0x33;
    }

    MIUAREF |= 0x61000;   /* Auto-refresh enabled */
}

static void hw_preinit(void)
{
    bool gpio3out, coldboot;

    syscon_preinit();
    gpio_preinit();
    i2c_preinit(0);

    /* get (previously) configured output selection for GPIO3 */
    gpio3out = (pmu_rd(PCF5063X_REG_GPIO3CFG) & 7);
    /* coldboot: when set, device has been in NoPower state */
    coldboot = (pmu_rd(PCF5063X_REG_OOCSHDWN) >> 3) & 1;

    pmu_preinit();
    miu_preinit(!coldboot && !gpio3out);
}


/* Safety measure - maximum allowed firmware image size.
   The largest known current (October 2009) firmware is about 6.2MB so
   we set this to 8MB.
*/
#define MAX_LOADSIZE (8*1024*1024)

#define EMCORE_BIN "/.apps/emcore.bin"

void bss_init(void);
extern uint32_t _movestart;
extern uint32_t start_loc;

/* The buffer to load the firmware into - use an uncached alias of 0x8000000 */
unsigned char *loadbuffer = (unsigned char *)S5L8702_UNCACHED_ADDR(0x8000000);

/* tone sequences: period (uS), duration (ms), silence (ms) */
static uint16_t alive[] = { 500,100,0, 0 };

extern int line;

void fatal_error(void)
{
    extern int line;
    bool holdstatus=false;

    /* System font is 6 pixels wide */
    printf("Hold MENU+SELECT to");
    printf("reboot then SELECT+PLAY");
    printf("for disk mode");
    lcd_update();

    while (1) {
        if (button_hold() != holdstatus) {
            if (button_hold()) {
                holdstatus=true;
                lcd_puts(0, line, "Hold switch on!");
            } else {
                holdstatus=false;
                lcd_puts(0, line, "               ");
            }
            lcd_update();
        }
    }
}

static int load_bin(unsigned char* buf, const char* filename, int buffer_size)
{
    int fd;
    int rc;
    int len;
    int ret = 0;

    /* full path passed */
    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return EFILE_NOT_FOUND;

    len = filesize(fd) - 8;

    if (len > buffer_size)
    {
        ret = EFILE_TOO_BIG;
        goto end;
    }

    rc = read(fd, buf, len);
    if(rc < len)
    {
        ret = EREAD_IMAGE_FAILED;
        goto end;
    }

    ret = len;

end:
    close(fd);
    return ret;
}

/* IM3 size aligned to NOR sector size */
#define GET_NOR_SZ(hinfo) \
    (ALIGN_UP(IM3HDR_SZ + get_uint32le((hinfo)->data_sz), 0x1000))

static uint32_t get_uint32le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static int load_of(void)
{
    struct Im3Info *hinfo;
    void *fw_addr;
    int nor_offset;

    /* ROMBOOT loads IM3 header at IRAM0_ORIG and data at IRAM1_ORIG,
     * actually IRAM1_ORIG contains current RB loader IM3 header, it
     * will be replaced by OF IM3 header.
     */
    hinfo = (struct Im3Info*)IRAM1_ORIG; /* actual RB loader IM3 hdr */
    fw_addr = (void*)IRAM0_ORIG;
    nor_offset = NORBOOT_OFF + GET_NOR_SZ(hinfo); /* OF NOR offset */

    if (read_im3(nor_offset, hinfo, fw_addr))
        return 0;

    return -1;
}

/* How it works:
 *
 *  1) S5L8702 ROMBOOT loads and executes an IM3 image located at NOR
 *     offset=32Kb:
 *     - IM3 body (RB bootloader) is loaded at IRAM0_ORIG (0x22000000)
 *     - IM3 header (0x800 bytes) is loaded at IRAM1_ORIG (0x22010000)
 *  2) once executed, RB booloader moves itself from IRAM0_ORIG to
 *     IRAM1_ORIG+0x800, preserving current IM3 header that contains
 *     the NOR offset where the ONB (original NORBOOT, see dualboot.c)
 *     is located.
 *  3) ONB is loaded from NOR to IRAM0_ORIG (body) and IRAM1_ORIG
 *     (header), ONB will be needed if the user chooses to use the OF.
 *  4) wait ~1 second from the start of execution for the user button
 *     selection. SDRAM cannot be modified until we know the user is
 *     not going to launch OF because sometimes the OF switch to
 *     'hibernated' state, leaving runtime data in SDRAM autorefresh
 *     mode. So we should wait for the user selection with no kernel,
 *     no LCD, no HDD...
 *  5) if the user chooses to run OF (including diagmode and diskmode),
 *     then it is lauched executing the ONB.
 *  6) if user chooses to run RB then the kernel is launched and
 *     rockbox.ipod loaded and executed as usual.
 */
void main(void)
{
    int btn = BUTTON_NONE;
    int rc = 0;
    bool button_was_held;
    char *filename, *bootname;

    usec_timer_init();
    piezo_seq(alive);

    int32_t stop_time = USEC_TIMER + 1000000;

    /* Check the button hold status as soon as possible - to
       give the user maximum chance to turn it on in order to
       reset the settings in rockbox. */
    cwheel_set_gpio(CWMODE_MAN | CWFLAG_HSPOLL);
    button_was_held = button_hold();

    /* loads OF in IRAM0, interrupt vectors are destroyed */
    rc = load_of();

    while ((int32_t)USEC_TIMER - stop_time < 0);

    if (!button_was_held)
        btn = read_all_buttons(5);

    if (button_was_held ||
            ((btn != BUTTON_NONE) && (btn != BUTTON_RIGHT))) {
        if (rc >= 0)
            /* Branch to start of IRAM */
            asm volatile("mov pc, %0"::"r"(IRAM0_ORIG));
    }
    /* clever decision! restore interrupt vectors */
    memcpy((void*)IRAM0_ORIG, &_movestart, 4*(&start_loc-&_movestart));

    hw_preinit();

    memory_init();

    /*
     * XXX: BSS is initialized now, do not use .bss before this line
     */
    bss_init();

    system_init();
    kernel_init();
    i2c_init();

    enable_irq();

    lcd_init();

    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();
    lcd_update();
    sleep(HZ/30);

    font_init();
    lcd_setfont(FONT_SYSFIXED);

    button_init();

    /* TODO: battery trap */
    /*powermgmt_init();*/

    backlight_init(); /* Turns on the backlight */

    verbose = true;

    printf("Rockbox boot loader");
    printf("Version: %s", rbversion);

    if (rc<0) {
        printf("load OF error: %d",rc);
        fatal_error();
    }

    rc = storage_init();
    if (rc != 0) {
        printf("ATA error: %d", rc);
        fatal_error();
    }

    filesystem_init();

    rc = disk_mount_all();
    if (rc<=0) {
        printf("No partition found");
        fatal_error();
    }

    if (btn == BUTTON_RIGHT) {
        bootname = "emCORE";
        filename = EMCORE_BIN;
        printf("Loading %s...", bootname);
        rc = load_bin(loadbuffer, filename, MAX_LOADSIZE);
    }
    else {
        bootname = "Rockbox";
        filename = BOOTFILE;
        printf("Loading %s...", bootname);
        rc = load_firmware(loadbuffer, filename, MAX_LOADSIZE);
    }

    if (rc <= EFILE_EMPTY) {
        printf("Error!");
        printf("Can't load %s: ", filename);
        printf(loader_strerror(rc));
        fatal_error();
    }

    printf("%s loaded.", bootname);

    /* If we get here, we have a new firmware image at 0x08000000, run it */
    printf("Executing...");

    disable_irq();

    /* Disable caches and protection unit */
    asm volatile(
        "mrc 15, 0, r0, c1, c0, 0 \n"
        "bic r0, r0, #0x1000      \n"
        "bic r0, r0, #0x5         \n"
        "mcr 15, 0, r0, c1, c0, 0 \n"
    );

    /* Branch to start of DRAM */
    asm volatile("mov pc, %0"::"r"(DRAM_ORIG));
}
