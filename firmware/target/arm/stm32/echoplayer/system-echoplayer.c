/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "system.h"
#include "button.h"
#include "gpio-stm32h7.h"
#include "clock-echoplayer.h"
#include "system-echoplayer.h"
#include "regs/stm32h743/fmc.h"
#include "regs/stm32h743/pwr.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/rtc.h"
#include "regs/cortex-m/cm_scb.h"

#ifdef BOOTLOADER
# define BOOTLOADER_INIT 1
#else
# define BOOTLOADER_INIT 0
#endif

#define F_INPUT      GPIOF_INPUT(GPIO_PULL_DISABLED)
#define F_INPUT_PU   GPIOF_INPUT(GPIO_PULL_UP)
#define F_INPUT_PD   GPIOF_INPUT(GPIO_PULL_DOWN)
#define F_OUT_LS(x)  GPIOF_OUTPUT(x, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_DISABLED)
#define F_SDRAM      GPIOF_FUNCTION(12, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_VERYHIGH, GPIO_PULL_DISABLED)
#define F_I2C1       GPIOF_FUNCTION(4, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_DISABLED)
#define F_SDMMC1     GPIOF_FUNCTION(12, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_MEDIUM, GPIO_PULL_UP)
#define F_SDMMC1CK   GPIOF_FUNCTION(12, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_MEDIUM, GPIO_PULL_DISABLED)
#define F_SAI1       GPIOF_FUNCTION(6, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_VERYHIGH, GPIO_PULL_DISABLED)
#define F_SPI5       GPIOF_FUNCTION(5, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_DISABLED)
#define F_LCD_AF14   GPIOF_FUNCTION(14, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_DISABLED)
#define F_LCD_AF9    GPIOF_FUNCTION(9, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_DISABLED)
#define F_LPTIM4_OUT GPIOF_FUNCTION(3, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_LOW, GPIO_PULL_DISABLED)
#define F_LPTIM1_OUT GPIOF_FUNCTION(1, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_VERYHIGH, GPIO_PULL_DISABLED)

#if STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB1
# define F_OTG_FS   GPIOF_ANALOG()
# define F_ULPI     GPIOF_FUNCTION(10, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_VERYHIGH, GPIO_PULL_DISABLED)
# define F_MCO1     GPIOF_FUNCTION(0, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_VERYHIGH, GPIO_PULL_DISABLED)
#else
# define F_OTG_FS   GPIOF_FUNCTION(10, GPIO_TYPE_PUSH_PULL, GPIO_SPEED_VERYHIGH, GPIO_PULL_DISABLED)
# define F_ULPI     GPIOF_ANALOG()
# define F_MCO1     GPIOF_ANALOG()
#endif

enum echoplayer_boot_reason echoplayer_boot_reason = ECHOPLAYER_BOOT_REASON_NORMAL;

static const struct gpio_setting gpios[] = {
    STM_DEFGPIO(GPIO_BUTTON_A,          F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_B,          F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_X,          F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_Y,          F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_START,      F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_SELECT,     F_INPUT),    /* external pulldown */
    STM_DEFGPIO(GPIO_BUTTON_UP,         F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_DOWN,       F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_LEFT,       F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_RIGHT,      F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_VOL_UP,     F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_VOL_DOWN,   F_INPUT_PU),
    STM_DEFGPIO(GPIO_BUTTON_POWER,      F_INPUT_PD),
    STM_DEFGPIO(GPIO_BUTTON_HOLD,       F_INPUT_PU),
    STM_DEFGPIO(GPIO_POWER_1V8,         F_OUT_LS(0)), /* active high */
    STM_DEFGPIO(GPIO_CODEC_AVDD_EN,     F_OUT_LS(1)), /* active low */
    STM_DEFGPIO(GPIO_CODEC_DVDD_EN,     F_OUT_LS(1)), /* active low */
    STM_DEFGPIO(GPIO_CODEC_RESET,       F_OUT_LS(0)), /* active low */
    STM_DEFGPIO(GPIO_USBPHY_POWER_EN,   F_OUT_LS(1)), /* active low */
    STM_DEFGPIO(GPIO_USBPHY_RESET,      F_OUT_LS(0)), /* active low */
    STM_DEFGPIO(GPIO_CHARGER_CHARGING,  F_INPUT_PU),  /* active low */
    STM_DEFGPIO(GPIO_CHARGER_ENABLE,    F_INPUT),     /* hi-z: off, gnd: on */
    STM_DEFGPIO(GPIO_SDMMC_DETECT,      F_INPUT_PU),  /* active low */
    STM_DEFGPIO(GPIO_LINEOUT_DETECT,    F_INPUT),     /* external pullup */
    STM_DEFGPIO(GPIO_LCD_RESET,         F_OUT_LS(0)), /* active low */
    STM_DEFGPIO(GPIO_BACKLIGHT,         F_LPTIM1_OUT),
    STM_DEFGPIO(GPIO_USB_VBUS,          F_INPUT),     /* active high */
};

/* TODO - replace hex constants - there are probably mistakes here */

static const struct pingroup_setting pingroups[] = {
    /* FMC - SDRAM1 */
    STM_DEFPINS(GPIO_A, 0x0080, F_SDRAM),
    STM_DEFPINS(GPIO_C, 0x000c, F_SDRAM),
    STM_DEFPINS(GPIO_D, 0xc703, F_SDRAM),
    STM_DEFPINS(GPIO_E, 0xff83, F_SDRAM),
    STM_DEFPINS(GPIO_F, 0xf83f, F_SDRAM),
    STM_DEFPINS(GPIO_G, 0x8137, F_SDRAM),
    /* OTG_FS */
    STM_DEFPINS(GPIO_A, 0x1800, F_OTG_FS),
    /* OTG_HS - ULPI */
    STM_DEFPINS(GPIO_A, 0x0028, F_ULPI),
    STM_DEFPINS(GPIO_B, 0x3c23, F_ULPI),
    STM_DEFPINS(GPIO_C, 0x0001, F_ULPI),
    STM_DEFPINS(GPIO_H, 0x0010, F_ULPI),
    STM_DEFPINS(GPIO_I, 0x0800, F_ULPI),
    /* MCO1 for USB PHY */
    STM_DEFPINS(GPIO_A, 0x0100, F_MCO1),
    /* I2C1 */
    STM_DEFPINS(GPIO_B, 0x00c0, F_I2C1),
    /* SDMMC1 */
    STM_DEFPINS(GPIO_C, 0x1000, F_SDMMC1CK),
    STM_DEFPINS(GPIO_C, 0x0f00, F_SDMMC1),
    STM_DEFPINS(GPIO_D, 0x0004, F_SDMMC1),
    /* SAI1 */
    STM_DEFPINS(GPIO_E, 0x007c, F_SAI1),
    /* SPI5 */
    STM_DEFPINS(GPIO_F, 0x02c0, F_SPI5),
    /* LCD */
    STM_DEFPINS(GPIO_F, 0x0400, F_LCD_AF14),
    STM_DEFPINS(GPIO_G, 0x0cc0, F_LCD_AF14),
    STM_DEFPINS(GPIO_G, 0x1000, F_LCD_AF9),
    STM_DEFPINS(GPIO_H, 0xff00, F_LCD_AF14),
    STM_DEFPINS(GPIO_I, 0x06e7, F_LCD_AF14),
};

INIT_ATTR static void fmc_init(void)
{
    /* configure clock */
    reg_writef(RCC_D1CCIPR, FMCSEL_V(AHB));

    /* ungate FMC peripheral */
    reg_writef(RCC_AHB3ENR, FMCEN(1));

    /* configure FMC */
    reg_writef(FMC_SDCR(0),
               RPIPE(0),    /* Not clear why this would be useful so leave at 0 */
               RBURST(1),   /* Enable burst read optimization */
               SDCLK(2),    /* 2x fmc_ker_ck per sdclk (120 MHz, tCK = 8.33ns) */
               WP(0),       /* Write protect off */
               CAS(2),      /* 2 cycle CAS latency */
               NB(1),       /* 4 internal banks */
               MWID(1),     /* 16-bit data bus width */
               NR(2),       /* 13 row address bits */
               NC(1));      /* 9 column address bits */
    reg_writef(FMC_SDTR(0),
               TRCD(2),     /* 15-20 ns <= 3 tCK */
               TRP(2),      /* 15-20 ns <= 3 tCK */
               TWR(2),      /* WR delay >= 2 tCK, but must be >= RAS - RCD (3 tCK) */
               TRC(7),      /* 55-65 ns <= 7-8 tCK */
               TRAS(5),     /* 40-45 ns <= 5-6 tCK */
               TXSR(8),     /* 70-75 ns <= 9 tCK */
               TMRD(1));    /* MR delay == 2 tCK */

    reg_writef(FMC_BCR(0), FMCEN(1));

    /* send SDRAM initialization commands */
    reg_writef(FMC_SDCMR, CTB1(1), CTB2(0), MODE(1), NRFS(1));
    udelay(100);

    reg_writef(FMC_SDCMR, CTB1(1), CTB2(0), MODE(2), NRFS(1));
    reg_writef(FMC_SDCMR, CTB1(1), CTB2(0), MODE(3), NRFS(8));
    reg_writef(FMC_SDCMR, CTB1(1), CTB2(0), MODE(4), NRFS(1), MRD(0x220));
    reg_writef(FMC_SDCMR, CTB1(1), CTB2(0), MODE(3), NRFS(8));

    /*
     * Refresh time calculation:
     * -> 64ms/8192 rows = 7.81 us/row
     * -> 937 tCK per row, minus 20 tCK margin from datasheet
     */
    reg_writef(FMC_SDRTR, REIE(0), COUNT(917), CRE(0));
}

void system_init(void)
{
    if (BOOTLOADER_INIT)
    {
        /* Enable clocks for all used GPIO banks */
        reg_writef(RCC_AHB4ENR,
                   GPIOAEN(1), GPIOBEN(1), GPIOCEN(1), GPIODEN(1),
                   GPIOEEN(1), GPIOFEN(1), GPIOGEN(1), GPIOHEN(1), GPIOIEN(1));

        /*
         * NOTE: I think it's possible to disable clocks for the banks which
         * we don't need to access at runtime because these are only clocking
         * register access. Probably a micro-optimization but it supposedly
         * does save a few uA/MHz.
         */
    }

    /*
     * Set cpu_power_on high as early as possible to
     * ensure we won't brown out if the power button
     * isn't pressed.
     */
    gpio_configure_single(GPIO_CPU_POWER_ON, F_OUT_LS(1));

    /* Set vector table address */
    extern char __vectors_arm[];
    reg_var(CM_SCB_VTOR) = (uint32_t)__vectors_arm;

#if defined(DEBUG)
    system_debug_enable(true);
#endif

    if (BOOTLOADER_INIT)
    {
        /* Enable CPU cache */
        stm32_enable_caches();

        /* Initialize system clocks */
        echoplayer_clock_init();
    }

    /* Enable systick early due to udelay() needed for FMC init */
    stm32_systick_enable();

    if (BOOTLOADER_INIT)
    {
        /* Configure GPIOs and start FMC */
        gpio_configure_all(gpios, ARRAYLEN(gpios),
                           pingroups, ARRAYLEN(pingroups));
        fmc_init();

        /* Read & clear reset source */
        uint32_t rsr = reg_var(RCC_RSR);
        reg_assignf(RCC_RSR, RMVF(1));

        /*
         * Determine boot reason -- SFTRST means a software reset
         * occurred, which may be a reboot or a power off
         */
        if (reg_vreadf(rsr, RCC_RSR, SFTRSTF))
        {
            reg_writef(RCC_APB4ENR, RTCAPBEN(1));

            if (reg_readf(RTC_CR, WUTE))
                echoplayer_boot_reason = ECHOPLAYER_BOOT_REASON_SW_REBOOT;
            else
                echoplayer_boot_reason = ECHOPLAYER_BOOT_REASON_SW_POWEROFF;
        }
    }

    /* Disable RTC_OUT pin */
    echoplayer_set_rtcout_mode(ECHOPLAYER_RTCOUT_DISABLED);
}

void system_exception_wait(void)
{
    while (button_read_device() != (BUTTON_POWER | BUTTON_START));
}

void echoplayer_set_rtcout_mode(enum echoplayer_rtcout_mode mode)
{
    reg_writef(RCC_APB4ENR, RTCAPBEN(1));
    reg_writef(PWR_CR1, DBP(1));

    reg_writef(RTC_WPR, KEY_V(KEY1));
    reg_writef(RTC_WPR, KEY_V(KEY2));

    reg_writef(RTC_OR, OUT_RMP(0), ALARM_TYPE_V(PUSH_PULL));

    switch (mode)
    {
    case ECHOPLAYER_RTCOUT_REBOOT:
        /*
         * Use the inverted wakeup timer output to keep power
         * enabled during reset. If, somehow, the system does
         * not reset properly then the wakeup timer will drive
         * the RTC_OUT pin low after 1 second and cut power.
         */
        while (!reg_readf(RTC_ISR, WUTWF));
        reg_writef(RTC_ISR, WUTF(0));
        reg_writef(RTC_WUTR, VALUE(STM32_LSE_FREQ / 8));
        reg_writef(RTC_CR, OSEL_V(WAKEUP), POL(1), WUCKSEL(0), WUTE(1));
        break;

    case ECHOPLAYER_RTCOUT_DISABLED:
    default:
        reg_writef(RTC_CR, OSEL_V(DISABLED), POL(0), WUTE(0));
        break;
    }

    reg_writef(RTC_WPR, KEY(0));
    reg_writef(PWR_CR1, DBP(0));
}
