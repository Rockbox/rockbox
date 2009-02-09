/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#include "jz4740.h"

/*
 * Clock Generation Module
 */
#define TO_MHZ(x) ((x)/1000000), ((x)%1000000)/10000
#define TO_KHZ(x) ((x)/1000), ((x)%1000)/10

static void display_clocks(void)
{
	unsigned int cppcr = REG_CPM_CPPCR;  /* PLL Control Register */
	unsigned int cpccr = REG_CPM_CPCCR;  /* Clock Control Register */
	unsigned int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	unsigned int od[4] = {1, 2, 2, 4};

	printf("CPPCR   :    0x%08x", cppcr);
	printf("CPCCR   :    0x%08x", cpccr);
	printf("PLL     :    %s", (cppcr & CPM_CPPCR_PLLEN) ? "ON" : "OFF");
	printf("m:n:o   :    %d:%d:%d",
			__cpm_get_pllm() + 2,
			__cpm_get_plln() + 2,
			od[__cpm_get_pllod()]
		);
	printf("C:H:M:P :   %d:%d:%d:%d", 
			div[__cpm_get_cdiv()],
			div[__cpm_get_hdiv()],
			div[__cpm_get_mdiv()],
			div[__cpm_get_pdiv()]
		);
    printf("U:L:I:P:M : %d:%d:%d:%d:%d",
			__cpm_get_udiv() + 1,
			__cpm_get_ldiv() + 1,
			__cpm_get_i2sdiv()+1,
			__cpm_get_pixdiv()+1,
			__cpm_get_mscdiv()+1
        );
	printf("PLL Freq:    %3d.%02d MHz", TO_MHZ(__cpm_get_pllout()));
	printf("CCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_cclk()));
	printf("HCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_hclk()));
	printf("MCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_mclk()));
	printf("PCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_pclk()));
	printf("LCDCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_lcdclk()));
	printf("PIXCLK  : %6d.%02d KHz",    TO_KHZ(__cpm_get_pixclk()));
	printf("I2SCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_i2sclk()));
	printf("USBCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_usbclk()));
	printf("MSCCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_mscclk()));
	printf("EXTALCLK:    %3d.%02d MHz", TO_MHZ(__cpm_get_extalclk()));
	printf("RTCCLK  :    %3d.%02d KHz", TO_KHZ(__cpm_get_rtcclk()));
}

static void display_enabled_clocks(void)
{
	unsigned long lcr = REG_CPM_LCR;
	unsigned long clkgr = REG_CPM_CLKGR;

	printf("Low Power Mode : %s", 
			((lcr & CPM_LCR_LPM_MASK) == CPM_LCR_LPM_IDLE) ?
			"IDLE" : (((lcr & CPM_LCR_LPM_MASK) == CPM_LCR_LPM_SLEEP) ? "SLEEP" : "HIBERNATE")
          );
    
	printf("Doze Mode      : %s", 
			(lcr & CPM_LCR_DOZE_ON) ? "ON" : "OFF");
	if (lcr & CPM_LCR_DOZE_ON)
		printf("     duty      : %d", (int)((lcr & CPM_LCR_DOZE_DUTY_MASK) >> CPM_LCR_DOZE_DUTY_BIT));
    
	printf("IPU            : %s",
			(clkgr & CPM_CLKGR_IPU) ? "stopped" : "running");
	printf("DMAC           : %s",
			(clkgr & CPM_CLKGR_DMAC) ? "stopped" : "running");
	printf("UHC            : %s",
			(clkgr & CPM_CLKGR_UHC) ? "stopped" : "running");
	printf("UDC            : %s",
			(clkgr & CPM_CLKGR_UDC) ? "stopped" : "running");
	printf("LCD            : %s",
			(clkgr & CPM_CLKGR_LCD) ? "stopped" : "running");
	printf("CIM            : %s",
			(clkgr & CPM_CLKGR_CIM) ? "stopped" : "running");
	printf("SADC           : %s",
			(clkgr & CPM_CLKGR_SADC) ? "stopped" : "running");
	printf("MSC            : %s",
			(clkgr & CPM_CLKGR_MSC) ? "stopped" : "running");
	printf("AIC1           : %s",
			(clkgr & CPM_CLKGR_AIC1) ? "stopped" : "running");
	printf("AIC2           : %s",
			(clkgr & CPM_CLKGR_AIC2) ? "stopped" : "running");
	printf("SSI            : %s",
			(clkgr & CPM_CLKGR_SSI) ? "stopped" : "running");
	printf("I2C            : %s",
			(clkgr & CPM_CLKGR_I2C) ? "stopped" : "running");
	printf("RTC            : %s",
			(clkgr & CPM_CLKGR_RTC) ? "stopped" : "running");
	printf("TCU            : %s",
			(clkgr & CPM_CLKGR_TCU) ? "stopped" : "running");
	printf("UART1          : %s",
			(clkgr & CPM_CLKGR_UART1) ? "stopped" : "running");
	printf("UART0          : %s",
			(clkgr & CPM_CLKGR_UART0) ? "stopped" : "running");
}

bool __dbg_ports(void)
{
    return false;
}

bool __dbg_hw_info(void)
{
    return false;
}

