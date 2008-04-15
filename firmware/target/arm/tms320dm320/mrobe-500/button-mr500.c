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

static enum touchpad_mode current_mode = TOUCHPAD_POINT;
static int touchpad_buttons[3][3] = {
    {BUTTON_TOPLEFT, BUTTON_TOPMIDDLE, BUTTON_TOPRIGHT},
    {BUTTON_MIDLEFT, BUTTON_CENTER, BUTTON_MIDRIGHT},
    {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT},
};

void touchpad_set_mode(enum touchpad_mode mode)
{
    current_mode = mode;
}
enum touchpad_mode touchpad_get_mode(void)
{
    return current_mode;
}

static struct touch_calibration_point topleft, bottomright;

/* Jd's tests.. These will hopefully work for everyone so we dont have to
 *  create a calibration screen.
 *  Portait:
 *      (0,0) = 200, 3900
 *      (480,640) = 3880, 270
 *  Landscape:
 *      (0,0) = 200, 270
 *      (640,480) = 3880, 3900
*/

static int touch_to_pixels(short val_x, short val_y)
{
    short x,y;

#if CONFIG_ORIENTATION == SCREEN_PORTAIT
    x=val_x;
    y=val_y;
#else
    x=val_y;
    y=val_x;
#endif

    x = (x-topleft.val_x)*(bottomright.px_x - topleft.px_x) / (bottomright.val_x - topleft.val_x) + topleft.px_x;
    y = (y-topleft.val_y)*(bottomright.px_y - topleft.px_y) / (bottomright.val_y - topleft.val_y) + topleft.px_y;

    if (x < 0)
        x = 0;
    else if (x>=LCD_WIDTH)
        x=LCD_WIDTH-1;
        
    if (y < 0)
        y = 0;
    else if (y>=LCD_HEIGHT)
        y=LCD_HEIGHT-1;


    return (x<<16)|y;
}

void button_init_device(void)
{
    touch_available = false;
    /* GIO is the power button, set as input */
    IO_GIO_DIR0 |= 0x01;

#if CONFIG_ORIENTATION == SCREEN_PORTAIT
    topleft.val_x = 200;        
    topleft.val_y = 3900;
    
    bottomright.val_x = 3880;
    bottomright.val_y = 270;
#else
    topleft.val_x = 270;
    topleft.val_y = 200;
    
    bottomright.val_x = 3900;
    bottomright.val_y = 3880;
#endif

    topleft.px_x = 0;
    topleft.px_y = 0;

    bottomright.px_x = LCD_WIDTH;
    bottomright.px_y = LCD_HEIGHT;

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
    uart1_puts(data, 5);
}

#define TOUCH_MARGIN 8
char r_buffer[5];
int r_button = BUTTON_NONE;
int button_read_device(int *data)
{
    int retval, calbuf;
    static int oldbutton = BUTTON_NONE;
    
    r_button=BUTTON_NONE;
    *data = 0;

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
            switch (current_mode)
            {
                case TOUCHPAD_POINT:
                    r_button |= BUTTON_TOUCHPAD;
                    break;
                case TOUCHPAD_BUTTON:
                {
                    int px_x = ((*data&0xffff0000)>>16), px_y = ((*data&0x0000ffff));
                    r_button |= touchpad_buttons[px_y/(LCD_HEIGHT/3)][px_x/(LCD_WIDTH/3)];
                    oldbutton = r_button;
                    break;
                }
            }
        }
        last_touch = current_tick;
        touch_available = false;
    }
    remote_heartbeat();

    if ((IO_GIO_BITSET0&0x01) == 0)
    {
        r_button |= BUTTON_POWER;
        oldbutton=r_button;
    }

    retval=uart1_gets_queue(r_buffer, 5);
    do
    {
        for(calbuf=0;calbuf<4;calbuf++)
        {
            if((r_buffer[calbuf]&0xF0)==0xF0 && (r_buffer[calbuf+1]&0xF0)!=0xF0)
                break;
        }
        calbuf++;
        if(calbuf==5)
            calbuf=0;
        if(retval>=0)
        {
            r_button |= r_buffer[calbuf];
            oldbutton=r_button;
        }
        else
        {
            r_button=oldbutton;
        }
    } while((retval=uart1_gets_queue(r_buffer, 5))>=5);

    return r_button;
}

/* Touchpad data available interupt */
void read_battery_inputs(void);
void GIO14(void)
{
    short tsadc = tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS);
    short adscm = (tsadc&TSADC_ADSCM_MASK)>>TSADC_ADSCM_SHIFT;
    switch (adscm)
    {
        case 1:
        case 2:
            touch_available = true;
            break;
        case 0xb:
            read_battery_inputs();
            break;
    }
    //touch_available = true;
    IO_INTC_IRQ2 = (1<<3); /* IRQ_GIO14 == 35 */
}
