/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Rob Purchase
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
#include "pcf50635.h"
#include "i2c.h"
#include "system.h"

#define PCF50635_ADDR 0xe6

int pcf50635_write(int address, unsigned char val)
{
    unsigned char data[] = { address, val };
    return i2c_write(PCF50635_ADDR, data, 2);
}

int pcf50635_write_multiple(int address, const unsigned char* buf, int count)
{
    int i;

    for (i = 0; i < count; i++)
        pcf50635_write(address + i, buf[i]);

    return 0;
}

int pcf50635_read(int address)
{
    unsigned char val = -1;
    i2c_readmem(PCF50635_ADDR, address, &val, 1);
    return val;
}

int pcf50635_read_multiple(int address, unsigned char* buf, int count)
{
    return i2c_readmem(PCF50635_ADDR, address, buf, count);
}

void pcf50635_init(void)
{
#ifdef COWON_D2
    /* Configure outputs as per OF */
    pcf50635_write(PCF5063X_REG_DOWN1OUT, 0x13);  /* DOWN1 = 1.2V */
    pcf50635_write(PCF5063X_REG_DOWN1CTL, 0x1e);  /* DOWN1 DVM step = max */
    pcf50635_write(PCF5063X_REG_DOWN1ENA, 0x1);   /* DOWN1 enable */
    pcf50635_write(PCF5063X_REG_DOWN2OUT, 0x2f);  /* DOWN2 = 1.8V */
    pcf50635_write(PCF5063X_REG_DOWN2CTL, 0x1e);  /* DOWN2 DVM step = max */
    pcf50635_write(PCF5063X_REG_DOWN2ENA, 0x1);   /* DOWN2 enable */
    pcf50635_write(PCF5063X_REG_AUTOOUT,  0x5f);  /* AUTO = 3.0V */
    pcf50635_write(PCF5063X_REG_AUTOENA,  0x1);   /* AUTO enable */
    pcf50635_write(PCF5063X_REG_LDO1OUT,  0x18);  /* LDO1 = 3.3V */
    pcf50635_write(PCF5063X_REG_LDO1ENA,  0x1);   /* LDO1 enable */
    pcf50635_write(PCF5063X_REG_LDO2OUT,  0x15);  /* LDO2 = 3.0V */
    pcf50635_write(PCF5063X_REG_LDO2ENA,  0x1);   /* LDO2 enable */
    pcf50635_write(PCF5063X_REG_LDO3ENA,  0x0);   /* LDO3 disable */
    pcf50635_write(PCF5063X_REG_LDO4OUT,  0x15);  /* LDO4 = 3.0V */
    pcf50635_write(PCF5063X_REG_LDO4ENA,  0x1);   /* LDO4 enable */
    pcf50635_write(PCF5063X_REG_LDO5OUT,  0x9);   /* LDO5 = 1.8V */
    pcf50635_write(PCF5063X_REG_LDO5ENA,  0x1);   /* LDO4 enable */
    pcf50635_write(PCF5063X_REG_LDO6OUT,  0xc);   /* LDO6 = 2.1V */
    pcf50635_write(PCF5063X_REG_LDO6ENA,  0x1);   /* LDO4 enable */
    pcf50635_write(PCF5063X_REG_HCLDOENA, 0x0);   /* HCLDO disable */

    /* Configure automatic battery charging as per OF */
    pcf50635_write(PCF5063X_REG_MBCC1,
        pcf50635_read(PCF5063X_REG_MBCC1) | 7);   /* auto charge termination & resume */
    pcf50635_write(PCF5063X_REG_MBCC2,    0xa8);  /* Vmax = 4.2V, Vbatcond = 2.7V, long debounce */
    pcf50635_write(PCF5063X_REG_MBCC3,    0x2a);  /* precharge level = 16% */
    pcf50635_write(PCF5063X_REG_MBCC4,    0x94);  /* fastcharge level = 58% */
    pcf50635_write(PCF5063X_REG_MBCC5,    0xff);  /* fastcharge level (usb) = 100%  */
    pcf50635_write(PCF5063X_REG_MBCC6,    0x4);   /* cutoff level = 12.5% */
    pcf50635_write(PCF5063X_REG_MBCC7,    0xc1);  /* bat-sysimax = 2.2A, USB = 500mA */
    pcf50635_write(PCF5063X_REG_BVMCTL,   0xe);   /* batok level = 3.4V */
    
    /* IRQ masks */
    pcf50635_write(PCF5063X_REG_INT1M,    0x8a);  /* enable alarm, usbins, adpins */
    pcf50635_write(PCF5063X_REG_INT2M,    0xff);  /* mask all */
    pcf50635_write(PCF5063X_REG_INT3M,    0x7f);  /* enable onkey1s */
    pcf50635_write(PCF5063X_REG_INT4M,    0xfd);  /* enable lowbat */
    pcf50635_write(PCF5063X_REG_INT5M,    0xff);  /* mask all */

    pcf50635_write(PCF5063X_REG_OOCMODE,  0x0);
    pcf50635_write(PCF5063X_REG_OOCCTL,   0x2);   /* actphrst = phase 3 */
    pcf50635_write(PCF5063X_REG_OOCWAKE,          /* adapter, usb, (rtc) wake */
        (pcf50635_read(PCF5063X_REG_OOCWAKE) & 0x10) | 0xc1);

    /* We don't care about the GPIOs, disable them */
    pcf50635_write(PCF5063X_REG_GPIOCTL,  0x0);
    pcf50635_write(PCF5063X_REG_GPIO1CFG, 0x0);
    pcf50635_write(PCF5063X_REG_GPIO2CFG, 0x0);
    pcf50635_write(PCF5063X_REG_GPIO3CFG, 0x0);
#endif
}

void pcf50635_read_adc(int adc, short* res1, short* res2)
{
    int adcs1 = 0, adcs2 = 0, adcs3 = 0;

    int level = disable_irq_save();

    pcf50635_write(PCF5063X_REG_ADCC1, PCF5063X_ADCC1_ADCSTART | adc);

    do {
        adcs3 = pcf50635_read(PCF5063X_REG_ADCS3);
    } while (!(adcs3 & PCF5063X_ADCS3_ADCRDY));

    if (res1 != NULL) adcs1 = pcf50635_read(PCF5063X_REG_ADCS1);
    if (res2 != NULL) adcs2 = pcf50635_read(PCF5063X_REG_ADCS2);

    pcf50635_write(PCF5063X_REG_ADCC1, 0);

    restore_interrupt(level);

    if (res1 != NULL) *res1 = (adcs1 << 2) | (adcs3 & 3);
    if (res2 != NULL) *res2 = (adcs2 << 2) | ((adcs3 & 0xC) >> 2);
}
