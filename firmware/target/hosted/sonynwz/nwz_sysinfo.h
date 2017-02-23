/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Amaury Pouly
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
#ifndef __NWZ_SYSINFO_H__
#define __NWZ_SYSINFO_H__

#define NWZ_SYSINFO_DEV     "/dev/icx_sysinfo"
#define NWZ_SYSINFO_TYPE    's'

#define NWZ_SYSINFO_GET_SYS_INFO _IOR(NWZ_SYSINFO_TYPE, 0, unsigned int *)
#define NWZ_SYSINFO_PUT_SYS_INFO _IOW(NWZ_SYSINFO_TYPE, 1, unsigned int *)
#define NWZ_SYSINFO_GET_BRD_REVS _IOR(NWZ_SYSINFO_TYPE, 2, unsigned int *)
#define NWZ_SYSINFO_PUT_BRD_REVS _IOW(NWZ_SYSINFO_TYPE, 3, unsigned int *)
#define NWZ_SYSINFO_GET_MEM_SIZE _IOR(NWZ_SYSINFO_TYPE, 4, unsigned int *)
#define NWZ_SYSINFO_PUT_MEM_SIZE _IOW(NWZ_SYSINFO_TYPE, 5, unsigned int *)
#define NWZ_SYSINFO_GET_BTT_TYPE _IOR(NWZ_SYSINFO_TYPE, 6, unsigned int *)
#define NWZ_SYSINFO_PUT_BTT_TYPE _IOW(NWZ_SYSINFO_TYPE, 7, unsigned int *)
#define NWZ_SYSINFO_GET_LCD_TYPE _IOR(NWZ_SYSINFO_TYPE, 8, unsigned int *)
#define NWZ_SYSINFO_PUT_LCD_TYPE _IOW(NWZ_SYSINFO_TYPE, 9, unsigned int *)
#define NWZ_SYSINFO_GET_NPE_TYPE _IOR(NWZ_SYSINFO_TYPE, 10, unsigned int *)
#define NWZ_SYSINFO_PUT_NPE_TYPE _IOW(NWZ_SYSINFO_TYPE, 11, unsigned int *)
#define NWZ_SYSINFO_GET_DAC_TYPE _IOR(NWZ_SYSINFO_TYPE, 12, unsigned int *)
#define NWZ_SYSINFO_PUT_DAC_TYPE _IOW(NWZ_SYSINFO_TYPE, 13, unsigned int *)
#define NWZ_SYSINFO_GET_NCR_TYPE _IOR(NWZ_SYSINFO_TYPE, 14, unsigned int *)
#define NWZ_SYSINFO_PUT_NCR_TYPE _IOW(NWZ_SYSINFO_TYPE, 15, unsigned int *)
#define NWZ_SYSINFO_GET_SPK_TYPE _IOR(NWZ_SYSINFO_TYPE, 16, unsigned int *)
#define NWZ_SYSINFO_PUT_SPK_TYPE _IOW(NWZ_SYSINFO_TYPE, 17, unsigned int *)
#define NWZ_SYSINFO_GET_FMT_TYPE _IOR(NWZ_SYSINFO_TYPE, 18, unsigned int *)
#define NWZ_SYSINFO_PUT_FMT_TYPE _IOW(NWZ_SYSINFO_TYPE, 19, unsigned int *)
#define NWZ_SYSINFO_GET_OSG_TYPE _IOR(NWZ_SYSINFO_TYPE, 20, unsigned int *)
#define NWZ_SYSINFO_PUT_OSG_TYPE _IOW(NWZ_SYSINFO_TYPE, 21, unsigned int *)
#define NWZ_SYSINFO_GET_WAB_TYPE _IOR(NWZ_SYSINFO_TYPE, 22, unsigned int *)
#define NWZ_SYSINFO_PUT_WAB_TYPE _IOW(NWZ_SYSINFO_TYPE, 23, unsigned int *)
#define NWZ_SYSINFO_GET_TSP_TYPE _IOR(NWZ_SYSINFO_TYPE, 24, unsigned int *)
#define NWZ_SYSINFO_PUT_TSP_TYPE _IOW(NWZ_SYSINFO_TYPE, 25, unsigned int *)
#define NWZ_SYSINFO_GET_GSR_TYPE _IOR(NWZ_SYSINFO_TYPE, 26, unsigned int *)
#define NWZ_SYSINFO_PUT_GSR_TYPE _IOW(NWZ_SYSINFO_TYPE, 27, unsigned int *)
#define NWZ_SYSINFO_GET_MIC_TYPE _IOR(NWZ_SYSINFO_TYPE, 28, unsigned int *)
#define NWZ_SYSINFO_PUT_MIC_TYPE _IOW(NWZ_SYSINFO_TYPE, 29, unsigned int *)
#define NWZ_SYSINFO_GET_WMP_TYPE _IOR(NWZ_SYSINFO_TYPE, 30, unsigned int *)
#define NWZ_SYSINFO_PUT_WMP_TYPE _IOW(NWZ_SYSINFO_TYPE, 31, unsigned int *)
#define NWZ_SYSINFO_GET_SMS_TYPE _IOR(NWZ_SYSINFO_TYPE, 32, unsigned int *)
#define NWZ_SYSINFO_PUT_SMS_TYPE _IOW(NWZ_SYSINFO_TYPE, 33, unsigned int *)
#define NWZ_SYSINFO_GET_HCG_TYPE _IOR(NWZ_SYSINFO_TYPE, 34, unsigned int *)
#define NWZ_SYSINFO_PUT_HCG_TYPE _IOW(NWZ_SYSINFO_TYPE, 35, unsigned int *)
#define NWZ_SYSINFO_GET_RTC_TYPE _IOR(NWZ_SYSINFO_TYPE, 36, unsigned int *)
#define NWZ_SYSINFO_PUT_RTC_TYPE _IOW(NWZ_SYSINFO_TYPE, 37, unsigned int *)
#define NWZ_SYSINFO_GET_SDC_TYPE _IOR(NWZ_SYSINFO_TYPE, 38, unsigned int *)
#define NWZ_SYSINFO_PUT_SDC_TYPE _IOW(NWZ_SYSINFO_TYPE, 39, unsigned int *)
#define NWZ_SYSINFO_GET_SCG_TYPE _IOR(NWZ_SYSINFO_TYPE, 40, unsigned int *)
#define NWZ_SYSINFO_PUT_SCG_TYPE _IOW(NWZ_SYSINFO_TYPE, 41, unsigned int *)
#define NWZ_SYSINFO_GET_NFC_TYPE _IOR(NWZ_SYSINFO_TYPE, 42, unsigned int *)
#define NWZ_SYSINFO_PUT_NFC_TYPE _IOW(NWZ_SYSINFO_TYPE, 43, unsigned int *)

#endif /* __NWZ_SYSINFO_H__ */
