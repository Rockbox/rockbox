/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* this file also handles the touch screen driver interface */

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"
#include "backlight-target.h"
#include "uart-target.h"
#include "tsc2100.h"
#include "string.h"

#define BUTTON_TIMEOUT 50

#define BUTTON_START_BYTE 0xF0
#define BUTTON_START_BYTE2 0xF4 /* not sure why, but sometimes you get F0 or F4, */
                                /* but always the same one for the session? */
static short last_x, last_y, last_z1, last_z2; /* for the touch screen */
static int last_touch;

static struct touch_calibration_point topleft, bottomright;
static bool using_calibration = false;
void use_calibration(bool enable)
{
    using_calibration = enable;
}

void set_calibration_points(struct touch_calibration_point *tl,
                            struct touch_calibration_point *br)
{
    memcpy(&topleft, tl, sizeof(struct touch_calibration_point));
    memcpy(&bottomright, br, sizeof(struct touch_calibration_point));
}
static int touch_to_pixels(short val_x, short val_y)
{
    short x,y;
    int x1,x2;
    if (!using_calibration)
        return (val_x<<16)|val_y;
    x1 = topleft.val_x; x2 = bottomright.val_x;
    x = (val_x-x1)*(bottomright.px_x - topleft.px_x) / (x2 - x1) + topleft.px_x;
    x1 = topleft.val_y; x2 = bottomright.val_y;
    y = (val_y-x1)*(bottomright.px_y - topleft.px_y) / (x2 - x1) + topleft.px_y;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    return (x<<16)|y;
}
void button_init_device(void)
{
    last_touch = 0;
    /* GIO is the power button, set as input */
    IO_GIO_DIR0 |= 0x01;
    
    
    /* Enable the touchscreen interrupt */
    IO_INTC_EINT2 |= (1<<3); /* IRQ_GIO14 */
#if 0
    tsc2100_writereg(TSADC_PAGE, TSADC_ADDRESS, 
                     TSADC_PSTCM|
                     (0x2<<TSADC_ADSCM_SHIFT)| /* scan x,y,z1,z2 */
                     (0x1<<TSADC_RESOL_SHIFT) /* 8 bit resolution */
                     );
    /* doesnt work for some reason... 
        setting to 8bit would probably be better than the 12bit currently */
#endif
}

inline bool button_hold(void)
{
    return false;
}
#ifdef BOOTLOADER
int button_get_last_touch(void)
{
    int ret_val = last_touch;
    last_touch = 0;
    return ret_val;
}
#endif

int button_read_device(void)
{
    char data[5], c;
    int i = 0;
    int btn = BUTTON_NONE;
    
    if (last_touch)
        btn |= BUTTON_TOUCHPAD;
    
    if ((IO_GIO_BITSET0&0x01) == 0)
        btn |= BUTTON_POWER;
        
    uart1_heartbeat();
    while (uart1_available())
    {
        if (uart1_getch(&c))
        {
            if (i && (data[0] == BUTTON_START_BYTE || data[0] == BUTTON_START_BYTE2))
            {
                data[i++] = c;
            }
            else if (c == BUTTON_START_BYTE ||
                     c == BUTTON_START_BYTE2)
            {
                data[0] = c;
                i = 1;
            }
            
            if (i == 5)
            {
                if (data[1]& (1<<7))
                    btn |= BUTTON_RC_HEART;
                if (data[1]& (1<<6))
                    btn |= BUTTON_RC_MODE;
                if (data[1]& (1<<5))
                    btn |= BUTTON_RC_VOL_DOWN;
                if (data[1]& (1<<4))
                    btn |= BUTTON_RC_VOL_UP;
                if (data[1]& (1<<3))
                    btn |= BUTTON_RC_REW;
                if (data[1]& (1<<2))
                    btn |= BUTTON_RC_FF;
                if (data[1]& (1<<1))
                    btn |= BUTTON_RC_DOWN;
                if (data[1]& (1<<0))
                    btn |= BUTTON_RC_PLAY;
                break;
            }
        }
    }
    return btn;
}

void GIO14(void)
{
    tsc2100_read_values(&last_x,  &last_y,
                        &last_z1, &last_z2);
    last_touch = touch_to_pixels(last_x, last_y);
    IO_INTC_IRQ2 = (1<<3);
}
