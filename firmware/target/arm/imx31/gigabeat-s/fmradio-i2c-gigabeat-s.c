/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Physical interface of the SI4700 in the Gigabeat S
 *
 * Copyright (C) 2008 by Nils Wallménius
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
#include "mc13783.h"
#include "i2c-imx31.h"
#include "fmradio_i2c.h"

static struct i2c_node si4700_i2c_node =
{
    .num  = I2C2_NUM,
    .ifdr = I2C_IFDR_DIV192, /* 66MHz/.4MHz = 165, closest = 192 = 343750Hz */
                             /* Just hard-code for now - scaling may require 
                              * updating */
    .addr = (0x20),
};

void fmradio_i2c_init(void)
{
    /* RST: LOW */
    imx31_regclr32(&GPIO1_DR, (1 << 26));
    /* RST: OUT */
    imx31_regset32(&GPIO1_GDIR, (1 << 26));

    /* I2C2 SCL: IN, I2C2: SDA IN */
    imx31_regclr32(&GPIO2_GDIR, (3 << 14));
    /* I2C2 SCL LO, I2C2 SDA LO */
    imx31_regclr32(&GPIO2_DR, (3 << 14));

    /* open-drain pins - external pullups on PCB. Pullup default but
     * disabled */
    imx31_regmod32(&SW_PAD_CTL_DSR_DTE1_RI_DTE1_DCD_DTE1,
        /* RI_DTE1 (I2C2_SCLK) */
        SW_PAD_CTL_IO2w(SW_PAD_CTL_PUE_PKE_DISABLE |
                        SW_PAD_CTL_PUS_UP_100K |
                        SW_PAD_CTL_HYS |
                        SW_PAD_CTL_ODE) |
        /* DCD_DTE1 (I2C2_SDA) */
        SW_PAD_CTL_IO1w(SW_PAD_CTL_PUE_PKE_DISABLE |
                        SW_PAD_CTL_PUS_UP_100K |
                        SW_PAD_CTL_HYS |
                        SW_PAD_CTL_ODE),
        SW_PAD_CTL_IO2 | SW_PAD_CTL_IO1);
    /* set outputs to I2C2 */
    imx31_regmod32(&SW_MUX_CTL_RI_DTE1_DCD_DTE1_DTR_DCE2_RXD2,
       /* RI_DTE1 => I2C2_SCLK */
       SW_MUX_CTL_SIG4w(SW_MUX_OUT_ALT2 | SW_MUX_IN_ALT2) |
       /* DCD_DTE1 => I2C2_SDA */
       SW_MUX_CTL_SIG3w(SW_MUX_OUT_ALT2 | SW_MUX_IN_ALT2),
       SW_MUX_CTL_SIG4 | SW_MUX_CTL_SIG3);
}

void fmradio_i2c_enable(bool enable)
{
    if (enable)
    {
        uint32_t io_pin_mux = SW_MUX_CTL_RI_DTE1_DCD_DTE1_DTR_DCE2_RXD2;
        /* place in GPIO mode to hold SDIO low during RESET release,
         * SEN1 should be high already (pullup) and GPIO3 left alone */
        imx31_regset32(&GPIO2_GDIR, (1 << 15)); /* SDIO OUT */
        /* I2C2_SDA => MCU2_15 */ 
        imx31_regmod32(&SW_MUX_CTL_RI_DTE1_DCD_DTE1_DTR_DCE2_RXD2,
            SW_MUX_CTL_SIG3w(SW_MUX_OUT_GPIO_DR | SW_MUX_IN_GPIO_PSR_ISR),
            SW_MUX_CTL_SIG3);
        /* enable CLK32KMCU clock */
        mc13783_set(MC13783_POWER_CONTROL0, MC13783_CLK32KMCUEN);
        /* enable the fm chip (release RESET) */
        imx31_regset32(&GPIO1_DR, (1 << 26));
        sleep(HZ/100);
        /* busmode should be selected - OK to release SDIO */
        imx31_regclr32(&GPIO2_GDIR, (1 << 15)); /* SDIO IN */
        /* restore pin mux (DCD_DTE1 => I2C2_SDA) */
        imx31_regmod32(&SW_MUX_CTL_RI_DTE1_DCD_DTE1_DTR_DCE2_RXD2,
                       io_pin_mux, SW_MUX_CTL_SIG3);
        /* the si4700 is the only thing connected to i2c2 so
           we can diable the i2c module when not in use */
        i2c_enable_node(&si4700_i2c_node, true);
    }
    else
    {
        /* the si4700 is the only thing connected to i2c2 so
           we can diable the i2c module when not in use */
        i2c_enable_node(&si4700_i2c_node, false);
        /* disable the fm chip */
        imx31_regclr32(&GPIO1_DR, (1 << 26));
        /* disable CLK32KMCU clock */
        mc13783_clear(MC13783_POWER_CONTROL0, MC13783_CLK32KMCUEN);
    }
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    (void)address;
    i2c_write(&si4700_i2c_node, buf, count);
    return 0;
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    i2c_read(&si4700_i2c_node, -1, buf, count);
    return 0;
}

int si4700_st(void)
{
    return (GPIO1_DR & (1 << 28)) >> 28;
}
