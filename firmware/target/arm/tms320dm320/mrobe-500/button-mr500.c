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
static bool touch_available = false;

static struct touch_calibration_point topleft, bottomright;
static bool using_calibration = false;
void use_calibration(bool enable)
{
    using_calibration = enable;
}
/* Jd's tests.. These will hopefully work for everyone so we dont have to
    create a calibration screen. and 
(0,0) = 0x00c0, 0xf40
(480,320) = 0x0f19, 0x00fc
*/
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
    touch_available = false;
    /* GIO is the power button, set as input */
    IO_GIO_DIR0 |= 0x01;
    topleft.px_x = 0;       topleft.px_y = 0;
    topleft.val_x = 0x00c0; topleft.val_y = 0xf40;
    
    bottomright.px_x = LCD_WIDTH;   bottomright.px_y = LCD_HEIGHT;
    bottomright.val_x = 0x0f19;     bottomright.val_y = 0x00fc;
    using_calibration = true;
    
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

static void remote_heartbeat(void)
{
    char data[5] = {0x11, 0x30, 0x11^0x30, 0x11+0x30, '\0'};
    uart1_puts(data);
}

#define TOUCH_MARGIN 8
int button_read_device(int *data)
{
    char c;
    int i = 0;
    int btn = BUTTON_NONE;
    *data = 0;
    
    if ((IO_GIO_BITSET0&0x01) == 0)
        btn |= BUTTON_POWER;
    if (touch_available)
    {
        short x,y;
        static long last_touch = 0;
        bool send_touch = false;
        tsc2100_read_values(&x,  &y, &last_z1, &last_z2);
        if (TIME_BEFORE(last_touch + HZ/5, current_tick))
        {
            if ((x > last_x + TOUCH_MARGIN) ||
                (x < last_x - TOUCH_MARGIN) ||
                (y > last_y + TOUCH_MARGIN) ||
                (y < last_y - TOUCH_MARGIN))
            {
                send_touch = true;
            }
        }
        else
            send_touch = true;
        if (send_touch)
        {
            last_x = x;
            last_y = y;
            *data = touch_to_pixels(x, y);
            btn |= BUTTON_TOUCHPAD;
        }
        last_touch = current_tick;
        touch_available = false;
    }
    remote_heartbeat();
    while (uart1_getch(&c))
    {
        if (i==0 && (c == BUTTON_START_BYTE || c == BUTTON_START_BYTE2) )
        {
            i++;
        }
        else if (i)
        {
            i++;
            if(i==2)
            {
                if (c& (1<<7))
                    btn |= BUTTON_RC_HEART;
                if (c& (1<<6))
                    btn |= BUTTON_RC_MODE;
                if (c& (1<<5))
                    btn |= BUTTON_RC_VOL_DOWN;
                if (c& (1<<4))
                    btn |= BUTTON_RC_VOL_UP;
                if (c& (1<<3))
                    btn |= BUTTON_RC_REW;
                if (c& (1<<2))
                    btn |= BUTTON_RC_FF;
                if (c& (1<<1))
                    btn |= BUTTON_RC_DOWN;
                if (c& (1<<0))
                    btn |= BUTTON_RC_PLAY;
            }
            else if(i==5)
            {
                i=0;
            }
        }
    }
    return btn;
}

/* Touchpad data available interupt */
void GIO14(void)
{
    touch_available = true;
    IO_INTC_IRQ2 = (1<<3); /* IRQ_GIO14 == 35 */
}
