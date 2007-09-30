/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _USBSTACK_H_
#define _USBSTACK_H_

#include <errno.h>

#define USB_STACK_MAX_SETTINGS_NAME     32*10       /* should be enough for > 10 driver names */

/* usb stack configuration */
#ifndef USBSTACK_CAPS
#define USBSTACK_CAPS 0     /* default: use no controller */
#endif

#define CONTROLLER_DEVICE    (1 << 0)
#define CONTROLLER_HOST      (1 << 1)

/*
 * error codes
 */
#define ENOFREESLOT             1
#define EWRONGCONTROLLERTYPE    2
#define ENODRIVERFOUND          3
#define EHWCRITICAL             4

enum usb_controller_type {
    DEVICE = 0,
    HOST,
};

/*
 * stack routines
 */
void usb_stack_init(void);
void usb_stack_start(void);
void usb_stack_stop(void);

void usb_controller_select(int type);
int usb_stack_get_mode(void);
int usb_device_driver_bind(const char* name);
void ubs_device_driver_unbind(void);

/* used by apps settings code */
unsigned char device_driver_names[USB_STACK_MAX_SETTINGS_NAME];

#endif /*_USBSTACK_H_*/
