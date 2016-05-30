/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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

/** Driver for the Freescale MPR121 Capacitive Proximity Sensor */
#include "system.h"
#include "kernel.h"
#include "usb.h"
#include "mpr121.h"
#include "mpr121-zenxfi3.h"
#include "i2c-imx233.h"
#include "pinctrl-imx233.h"

#define MPR121_I2C_ADDR     0xb4

/* NOTE on the architecture of the driver
 *
 * All non-time-critical operations (setup, gpio/pwm changes) are done with
 * blocking i2c transfers to make the code simpler. Since reading the touch
 * status is time critical, it is done asynchronously: when the IRQ pin is
 * asserted, it will disable IRQ pin sensing and trigger an asynchronous i2c
 * transfer to read touch status. When the transfer finishes, the driver will
 * renable IRQ pin sensing. */

static unsigned touch_status = 0; /* touch bitmask as reported by mpr121 */
static struct imx233_i2c_xfer_t read_status_xfer; /* async transfer to read touch status */
static uint8_t read_status_sel_reg; /* buffer for async transfer operation */
static uint8_t read_status_buf[2]; /* buffer for async transfer operation */

static void mpr121_irq_cb(int bank, int pin, intptr_t user);

static void touch_status_i2c_cb(struct imx233_i2c_xfer_t *xfer, enum imx233_i2c_error_t status)
{
    (void) xfer;
    (void) status;
    /* put status in the global variable */
    touch_status = read_status_buf[0] | read_status_buf[1] << 8;
    /* start sensing IRQ pin again */
    imx233_pinctrl_setup_irq(0, 18, true, true, false, &mpr121_irq_cb, 0);
}

void mpr121_irq_cb(int bank, int pin, intptr_t user)
{
    (void) bank;
    (void) pin;
    (void) user;
    /* NOTE the callback will not be fired until interrupt is enabled back.
     *
     * now setup an asynchronous i2c transfer to read touch status register,
     * this is a readmem operation with a first stage to select register
     * and a second stage to read status (2 bytes) */
    read_status_sel_reg = REG_TOUCH_STATUS;

    read_status_xfer.next = NULL;
    read_status_xfer.fast_mode = true;
    read_status_xfer.dev_addr = MPR121_I2C_ADDR;
    read_status_xfer.mode = I2C_READ;
    read_status_xfer.count[0] = 1; /* set touch status register address */
    read_status_xfer.data[0] = &read_status_sel_reg;
    read_status_xfer.count[1] = 2;
    read_status_xfer.data[1] = &read_status_buf;
    read_status_xfer.tmo_ms = 1000;
    read_status_xfer.callback = &touch_status_i2c_cb;

    imx233_i2c_transfer(&read_status_xfer);
}

static inline int mpr121_write_reg(uint8_t reg, uint8_t data)
{
    return i2c_writemem(MPR121_I2C_ADDR, reg, &data, 1);
}

static inline int mpr121_read_reg(uint8_t reg, uint8_t *data)
{
    return i2c_readmem(MPR121_I2C_ADDR, reg, data, 1);
}

void mpr121_init(void)
{
    /* soft reset */
    mpr121_write_reg(REG_SOFTRESET, REG_SOFTRESET__MAGIC);
    /* enable interrupt */
    imx233_pinctrl_acquire(0, 18, "mpr121_int");
    imx233_pinctrl_set_function(0, 18, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 18, false);
    imx233_pinctrl_setup_irq(0, 18, true, true, false, &mpr121_irq_cb, 0);
}

void mpr121_set_config(struct mpr121_config_t *conf)
{
    /* stop mode */
    mpr121_write_reg(REG_ELECTRODE, 0);
    /* write baseline values */
    for(int i = 0; i < ELECTRODE_COUNT; i++)
        mpr121_write_reg(REG_ExBV(i), conf->ele[i].bv);
    /* write eleprox bv */
    mpr121_write_reg(REG_EPROXBV, conf->eleprox.bv);
    /* write global fields */
    mpr121_write_reg(REG_MHDR, conf->filters.ele.rising.mhd);
    mpr121_write_reg(REG_NHDR, conf->filters.ele.rising.nhd);
    mpr121_write_reg(REG_NCLR, conf->filters.ele.rising.ncl);
    mpr121_write_reg(REG_FDLR, conf->filters.ele.rising.fdl);
    mpr121_write_reg(REG_MHDF, conf->filters.ele.falling.mhd);
    mpr121_write_reg(REG_NHDF, conf->filters.ele.falling.nhd);
    mpr121_write_reg(REG_NCLF, conf->filters.ele.falling.ncl);
    mpr121_write_reg(REG_FDLF, conf->filters.ele.falling.fdl);
    mpr121_write_reg(REG_NHDT, conf->filters.ele.touched.nhd);
    mpr121_write_reg(REG_NCLT, conf->filters.ele.touched.ncl);
    mpr121_write_reg(REG_FDLT, conf->filters.ele.touched.fdl);
    mpr121_write_reg(REG_MHDPROXR, conf->filters.eleprox.rising.mhd);
    mpr121_write_reg(REG_NHDPROXR, conf->filters.eleprox.rising.nhd);
    mpr121_write_reg(REG_NCLPROXR, conf->filters.eleprox.rising.ncl);
    mpr121_write_reg(REG_FDLPROXR, conf->filters.eleprox.rising.fdl);
    mpr121_write_reg(REG_MHDPROXF, conf->filters.eleprox.falling.mhd);
    mpr121_write_reg(REG_NHDPROXF, conf->filters.eleprox.falling.nhd);
    mpr121_write_reg(REG_NCLPROXF, conf->filters.eleprox.falling.ncl);
    mpr121_write_reg(REG_FDLPROXF, conf->filters.eleprox.falling.fdl);
    mpr121_write_reg(REG_NHDPROXT, conf->filters.eleprox.touched.nhd);
    mpr121_write_reg(REG_NCLPROXT, conf->filters.eleprox.touched.ncl);
    mpr121_write_reg(REG_FDLPROXT, conf->filters.eleprox.touched.fdl);
    /* touch & release thresholds */
    for(int i = 0; i < ELECTRODE_COUNT; i++)
    {
        mpr121_write_reg(REG_ExTTH(i), conf->ele[i].tth);
        mpr121_write_reg(REG_ExRTH(i), conf->ele[i].rth);
    }
    mpr121_write_reg(REG_EPROXTTH, conf->eleprox.tth);
    mpr121_write_reg(REG_EPROXRTH, conf->eleprox.rth);
    /* debounce */
    mpr121_write_reg(REG_DEBOUNCE, REG_DEBOUNCE__DR(conf->debounce.dr) |
        REG_DEBOUNCE__DT(conf->debounce.dt));
    /* analog-front end and filters */
    mpr121_write_reg(REG_AFE, REG_AFE__CDC(conf->global.cdc) |
        REG_AFE__FFI(conf->global.ffi));
    mpr121_write_reg(REG_FILTER, REG_FILTER__CDT(conf->global.cdt) |
        REG_FILTER__ESI(conf->global.esi) | REG_FILTER__SFI(conf->global.sfi));
    /* electrode charge */
    for(int i = 0; i < ELECTRODE_COUNT; i++)
        mpr121_write_reg(REG_CDCx(i), conf->ele[i].cdc);
    mpr121_write_reg(REG_CDCPROX, conf->eleprox.cdc);
    for(int i = 0; i < ELECTRODE_COUNT; i += 2)
    {
        mpr121_write_reg(REG_CDTx(i), REG_CDTx__CDT0(conf->ele[i].cdt) |
            REG_CDTx__CDT1(conf->ele[i+1].cdt));
    }
    mpr121_write_reg(REG_CDTPROX, conf->eleprox.cdt);
    /* Auto-Configuration */
    mpr121_write_reg(REG_AUTO_CONF, REG_AUTO_CONF__ACE(conf->autoconf.en) |
        REG_AUTO_CONF__ARE(conf->autoconf.ren) |
        REG_AUTO_CONF__BVA(conf->cal_lock) |
        REG_AUTO_CONF__RETRY(conf->autoconf.retry) |
        REG_AUTO_CONF__FFI(conf->global.ffi));
    mpr121_write_reg(REG_AUTO_CONF2, REG_AUTO_CONF2__ACFIE(conf->autoconf.acfie) |
        REG_AUTO_CONF2__ARFIE(conf->autoconf.arfie) |
        REG_AUTO_CONF2__OORIE(conf->autoconf.oorie) |
        REG_AUTO_CONF2__SCTS(conf->autoconf.scts));
    mpr121_write_reg(REG_USL, conf->autoconf.usl);
    mpr121_write_reg(REG_LSL, conf->autoconf.lsl);
    mpr121_write_reg(REG_TL, conf->autoconf.tl);
    /* electrode configuration */
    mpr121_write_reg(REG_ELECTRODE, REG_ELECTRODE__ELE_EN(conf->ele_en) |
        REG_ELECTRODE__ELEPROX_EN(conf->eleprox_en) |
        REG_ELECTRODE__CL(conf->cal_lock));
    /* gpio config */
    uint8_t ctl = 0;
    for(int i = ELE_GPIO_FIRST; i <= ELE_GPIO_LAST; i++)
        if(ELE_GPIO_CTL0(conf->ele[i].gpio))
            ctl |= REG_GPIO_CTL0__CTL0x(i);
    mpr121_write_reg(REG_GPIO_CTL0, ctl);
    ctl = 0;
    for(int i = ELE_GPIO_FIRST; i <= ELE_GPIO_LAST; i++)
        if(ELE_GPIO_CTL1(conf->ele[i].gpio))
            ctl |= REG_GPIO_CTL1__CTL1x(i);
    mpr121_write_reg(REG_GPIO_CTL1, ctl);
    ctl = 0;
    for(int i = ELE_GPIO_FIRST; i <= ELE_GPIO_LAST; i++)
        if(ELE_GPIO_DIR(conf->ele[i].gpio))
            ctl |= REG_GPIO_DIR__DIRx(i);
    mpr121_write_reg(REG_GPIO_DIR, ctl);
    ctl = 0;
    for(int i = ELE_GPIO_FIRST; i <= ELE_GPIO_LAST; i++)
        if(ELE_GPIO_EN(conf->ele[i].gpio))
            ctl |= REG_GPIO_EN__ENx(i);
    mpr121_write_reg(REG_GPIO_EN, ctl);
}

void mpr121_set_gpio_output(int ele, int gpio_val)
{
    switch(gpio_val)
    {
        case ELE_GPIO_SET:
            mpr121_write_reg(REG_GPIO_SET, REG_GPIO_SET__SETx(ele));
            break;
        case ELE_GPIO_CLR:
            mpr121_write_reg(REG_GPIO_CLR, REG_GPIO_CLR__CLRx(ele));
            break;
        case ELE_GPIO_TOG:
            mpr121_write_reg(REG_GPIO_TOG, REG_GPIO_TOG__TOGx(ele));
            break;
        default:
            break;
    }
}

void mpr121_set_gpio_pwm(int ele, int pwm)
{
    uint8_t reg_val;
    mpr121_read_reg(REG_PWMx(ele), &reg_val);
    if(REG_PWMx_IS_PWM0(ele))
        reg_val = (reg_val & ~REG_PWMx__PWM0_BM) | REG_PWMx__PWM0(pwm);
    else
        reg_val = (reg_val & ~REG_PWMx__PWM1_BM) | REG_PWMx__PWM1(pwm);
    mpr121_write_reg(REG_PWMx(ele), reg_val);
}

unsigned mpr121_get_touch_status(void)
{
    return touch_status;
}
