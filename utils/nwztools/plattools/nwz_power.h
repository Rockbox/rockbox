/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __NWZ_POWER_H__
#define __NWZ_POWER_H__

/** power */

#define NWZ_POWER_DEV   "/dev/icx_power"

#define NWZ_POWER_TYPE  'P'

/* ioctl request */
#define NWZ_POWER_GET_STATUS                _IOR(NWZ_POWER_TYPE, 0, int *)
#define NWZ_POWER_SET_VBUS_LIMIT            _IOW(NWZ_POWER_TYPE, 1, int *)
#define NWZ_POWER_GET_BAT_ADVAL             _IOR(NWZ_POWER_TYPE, 2, int *)
#define NWZ_POWER_GET_BAT_GAUGE             _IOR(NWZ_POWER_TYPE, 3, int *)
#define NWZ_POWER_ENABLE_CHARGER            _IOW(NWZ_POWER_TYPE, 4, int *)
#define NWZ_POWER_ENABLE_DEBUG_PORT         _IO (NWZ_POWER_TYPE, 5)
#define NWZ_POWER_IS_FULLY_CHARGED          _IOR(NWZ_POWER_TYPE, 6, int *)
#define NWZ_POWER_AVG_VALUE                 _IO (NWZ_POWER_TYPE, 7)
#define NWZ_POWER_CONSIDERATION_CHARGE      _IOW(NWZ_POWER_TYPE, 8, int)
#define NWZ_POWER_GET_VBUS_LIMIT            _IOR(NWZ_POWER_TYPE, 9, int *)
#define NWZ_POWER_GET_CHARGE_SWITCH         _IOR(NWZ_POWER_TYPE,10, int *)
#define NWZ_POWER_GET_SAMPLE_COUNT          _IOR(NWZ_POWER_TYPE,11, int *)
#define NWZ_POWER_SET_CHARGE_CURRENT        _IOW(NWZ_POWER_TYPE,12, int *)
#define NWZ_POWER_GET_CHARGE_CURRENT        _IOR(NWZ_POWER_TYPE,13, int *)
#define NWZ_POWER_GET_VBUS_ADVAL            _IOR(NWZ_POWER_TYPE,14, int *)
#define NWZ_POWER_GET_VSYS_ADVAL            _IOR(NWZ_POWER_TYPE,15, int *)
#define NWZ_POWER_GET_VBAT_ADVAL            _IOR(NWZ_POWER_TYPE,16, int *)
#define NWZ_POWER_CHG_VBUS_LIMIT            _IOW(NWZ_POWER_TYPE,17, int *)
#define NWZ_POWER_SET_ACCESSARY_CHARGE_MODE _IOW(NWZ_POWER_TYPE,18, int *)
#define NWZ_POWER_GET_ACCESSARY_CHARGE_MODE _IOR(NWZ_POWER_TYPE,19, int *)

/* NWZ_POWER_GET_STATUS bitmap */
#define NWZ_POWER_STATUS_CHARGE_INIT    0x100 /* initializing charging */
#define NWZ_POWER_STATUS_CHARGE_STATUS  0x0c0 /* charging status mask */
#define NWZ_POWER_STATUS_VBUS_DET       0x020 /* vbus detected */
#define NWZ_POWER_STATUS_AC_DET         0x010 /* ac adapter detected */
#define NWZ_POWER_STATUS_CHARGE_LOW     0x001 /* full voltage of 4.1 instead of 4.2 */
/* NWZ_POWER_STATUS_CHARGING_MASK value */
#define NWZ_POWER_STATUS_CHARGE_STATUS_CHARGING 0xc0
#define NWZ_POWER_STATUS_CHARGE_STATUS_SUSPEND  0x80
#define NWZ_POWER_STATUS_CHARGE_STATUS_TIMEOUT  0x40
#define NWZ_POWER_STATUS_CHARGE_STATUS_NORMAL   0x00

/* base values to convert from adval to mV (represents mV for adval of 255) */
#define NWZ_POWER_AD_BASE_VBAT          4664
#define NWZ_POWER_AD_BASE_VBUS          7254
#define NWZ_POWER_AD_BASE_VSYS          5700

/* battery gauge */
#define NWZ_POWER_BAT_NOBAT      -100 /* no battery */
#define NWZ_POWER_BAT_VERYLOW   -99 /* very low */
#define NWZ_POWER_BAT_LOW       0 /* low */
#define NWZ_POWER_BAT_GAUGE0    1 /* ____ */
#define NWZ_POWER_BAT_GAUGE1    2 /* O___ */
#define NWZ_POWER_BAT_GAUGE2    3 /* OO__ */
#define NWZ_POWER_BAT_GAUGE3    4 /* OOO_ */
#define NWZ_POWER_BAT_GAUGE4    5 /* OOOO */

/* NWZ_POWER_GET_ACCESSARY_CHARGE_MODE */
#define NWZ_POWER_ACC_CHARGE_NONE   0
#define NWZ_POWER_ACC_CHARGE_VBAT   1
#define NWZ_POWER_ACC_CHARGE_VSYS   2

/** pminfo
 *
 * This driver seems to collect the state of the device on boot. Thus one
 * can know if a key was pressed when booting for example.
 */

#define NWZ_PMINFO_DEV  "/dev/icx_pminfo"

#define NWZ_PMINFO_TYPE 'b'

/* ioctl request */

#define NWZ_PMINFO_GET_FACTOR       _IOR(NWZ_PMINFO_TYPE, 0, unsigned int *)
#define NWZ_PMINFO_CLR_DETRSTFLG    _IO(NWZ_PMINFO_TYPE, 1)

/* NWZ_PMINFO_GET_FACTOR bitmap
 * WARNING this information may not apply to all players and some bits do not
 * exists on some players */
#define ICX_PMINFO_FACTOR_RTC_WAL  0x20000000 /* RTC Weekly Alarm */
#define ICX_PMINFO_FACTOR_RTC_DAL  0x10000000 /* RTC Daily Alarm */
#define ICX_PMINFO_FACTOR_VBUS     0x08000000 /* VBUS in/out */
#define ICX_PMINFO_FACTOR_DC_POWER 0x04000000 /* AC Adaptor in/out */
#define ICX_PMINFO_FACTOR_USB_WAKE 0x01000000 /* USB Wake */
#define ICX_PMINFO_FACTOR_CHARGE   0x00400000 /* Charge */
#define ICX_PMINFO_FACTOR_CRADLE   0x00080000 /* Cradle in/out */
#define ICX_PMINFO_FACTOR_AB_EV    0x00008000 /* ab event */
#define ICX_PMINFO_FACTOR_NC_SW    0x00004000 /* nc switch */
#define ICX_PMINFO_FACTOR_HOLD_SW  0x00002000 /* hold switch */
#define ICX_PMINFO_FACTOR_KEY_PAD  0x00001000 /* keypad */
#define ICX_PMINFO_FACTOR_KEY_CODE 0x00000FFF /* keycode */

#endif /* __NWZ_POWER_H__ */
