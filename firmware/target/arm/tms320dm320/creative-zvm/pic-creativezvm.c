/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Maurus Cuelenaere
*
* Creative Zen Vision:M interrupt based PIC driver
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "button-target.h"
#include "i2c-dm320.h"


#include "lcd-target.h"
#include "lcd.h"
#include "sprintf.h"
#include "font.h"

#ifndef ZEN_VISION
/* Creative Zen Vision:M */
#define BTN_LEFT                        0x5F00
#define BTN_RIGHT                       0x4F00
#define BTN_BACK                        0xBF00
#define BTN_CUSTOM                      0x8F00
#define BTN_PLAY                        0x2F00
#define BTN_POWER                       0x0F00
#define BTN_MENU                        0x9F00
#define BTN_HOLD                        0x9F06
#define BTN_UNHOLD                      0xAF06

#define BTN_REL                         1

#define BTN_TOUCHPAD_PRESS              0x1F00
#define BTN_TOUCHPAD_CORNER_DOWN        0xAF00
#define BTN_TOUCHPAD_CORNER_UP          0x3F00

#define HEADPHONE_PLUGIN_A              0x5707
#define HEADPHONE_PLUGIN_B              0x5F07
#define HEADPHONE_UNPLUG_A              0x3707
#define HEADPHONE_UNPLUG_B              0x3F07

#define DOCK_INSERT                     0x6707
#define DOCK_UNPLUG                     0xDF06
#define DOCK_USB_INSERT                 0x2F06
#define DOCK_USB_UNPLUG                 0x3F06
#define DOCK_POWER_INSERT               0x2707
#define DOCK_POWER_UNPLUG               0x2F07

#else
/* Creative Zen Vision */
#define BTN_LEFT                        0xCF00
#define BTN_RIGHT                       0xEF00
#define BTN_BACK                        0xBF00
#define BTN_CUSTOM                      0x0
#define BTN_PLAY                        0x2F00
#define BTN_POWER                       0x0F00
#define BTN_MENU                        0x9F00
#define BTN_HOLD                        0x9F06
#define BTN_UNHOLD                      0xAF06
/* TODO: other values
First number is just pressing it, second is when you release it or keep it pressed a bit longer
On/Off    = 0F00 && 0F01
Hold      = 9F06 && AF06
Volume Up = 6F00 && 6F01
Vol Down  = 7F00 && 7F01
Up        = DF00 && DF01
Right     = EF00 && EF01
Down      = FF00 && FF01
Left      = CF00 && CF01
Back      = BF00 && BF01
Menu      = 9F00 && Etcetera
Ok        = 1F00
Play      = 2F00
Next      = 4F00
Prev      = 5F00

USB       = 2F06
USB ouot  = 3F06
Headphones= AF06
Hdphns out= BF06
Charger   = 4F06 -> 9F05
Chgrout   = 5F06 -> 8F05
AV in     = 8F06
AV out    = 9F06 */

#define BTN_REL                         1

#define BTN_TOUCHPAD_PRESS              0x8F00
#define BTN_TOUCHPAD_LONG_PRESS         0x0F00
#define BTN_TOUCHPAD_CORNER_DOWN        0xD700
#define BTN_TOUCHPAD_CORNER_LONG_DOWN   0x5700
#define BTN_TOUCHPAD_CORNER_UP          0x9F00
#define BTN_TOUCHPAD_CORNER_LONG_UP     0x1F00

#define HEADPHONE_PLUGIN_A              0xAF06
#define HEADPHONE_PLUGIN_B              0xAF06
#define HEADPHONE_UNPLUG_A              0xBF06
#define HEADPHONE_UNPLUG_B              0xBF06

#define DOCK_INSERT                     0x0
#define DOCK_UNPLUG                     0x0
#define DOCK_USB_INSERT                 0x2F06
#define DOCK_USB_UNPLUG_A               0x3F06
#define DOCK_USB_UNPLUG_B               0x3F06
#define DOCK_POWER_INSERT               0x4F06
#define DOCK_POWER_UNPLUG               0x5F06
#define DOCK_AV_INSERT                  0x8F06
#define DOCK_AV_UNPLUG                  0x9F06
#endif

#define PIC_ADR                         0x07

#define MASK_TV_OUT(x)                  ((x >> 14) & 1)
#define MASK_xx1(x)                     ((x >> 9) & 3)
#define MASK_xx2(x)                     ((x >> 4) & 1)
#define MASK_xx3(x)                     ((x >> 5) & 1)
#define MASK_xx4(x)                     ((x >> 6) & 1)
#define MASK_xx5(x)                     ((x >> 13) & 1)
#define MASK_xx6(x)                     ((x >> 12) & 1)
#define MASK_xx7(x)                     ((x >> 11) & 1)

#define NONBUTTON_HEADPHONE             (1 << 0)
#define NONBUTTON_DOCK                  (1 << 1)
#define NONBUTTON_USB                   (1 << 2)
#define NONBUTTON_POWER                 (1 << 3)
#define NONBUTTON_VIDEOUT               (1 << 4)

static unsigned int btn;
static bool hold_switch;
static unsigned char nonbtn;
static unsigned int pic_init_value;
static unsigned int pic_init2_value;
static unsigned int last_btn;
static long last_tick;
static int tick_diff;

#define TICK_MIN 0x33
#define TICK_MAX 0x34

/* Taken from scramble.c and modified */
static inline unsigned short le2short(unsigned char* buf)
{
   return (unsigned short)((buf[1] << 8) | buf[0]);
}

#define map_button(BtN,BtN2) case BtN: \
                                 btn ^= BtN2; \
                                 btn &= BtN2; \
                                 break; \
                             case BtN ^ BTN_REL: \
                                 btn ^= BtN2; \
                                 btn &= BtN2; \
                                 break;

#ifdef BUTTON_DEBUG
static bool sw = false;
#endif

void GIO0(void)
{
    unsigned char msg[4];
    i2c_read(PIC_ADR, msg, sizeof(msg));
    tick_diff = current_tick - last_tick;
    last_tick = current_tick;
    unsigned short btn_press = le2short(msg);
    if(tick_diff >= TICK_MIN && tick_diff <= TICK_MAX)
    {
        /* Ignore this, as it is a hold event */
        IO_INTC_IRQ1 = INTR_IRQ1_EXT0;
        return;
    }
    last_btn = btn_press;
    switch(btn_press)
    {
        map_button(BTN_LEFT,                 BUTTON_LEFT);
        map_button(BTN_RIGHT,                BUTTON_RIGHT);
        map_button(BTN_BACK,                 BUTTON_BACK);
        map_button(BTN_CUSTOM,               BUTTON_CUSTOM);
        map_button(BTN_MENU,                 BUTTON_MENU);
        map_button(BTN_PLAY,                 BUTTON_PLAY);
        map_button(BTN_POWER,                BUTTON_POWER);
        map_button(BTN_TOUCHPAD_PRESS,       BUTTON_SELECT);
        map_button(BTN_TOUCHPAD_CORNER_DOWN, BUTTON_DOWN);
        map_button(BTN_TOUCHPAD_CORNER_UP,   BUTTON_UP);
    case BTN_HOLD:
        hold_switch = true;
        break;
    case BTN_UNHOLD:
        hold_switch = false;
        break;
    case HEADPHONE_PLUGIN_A:
    case HEADPHONE_PLUGIN_B:
        nonbtn |= NONBUTTON_HEADPHONE;
        break;
    case HEADPHONE_UNPLUG_A:
    case HEADPHONE_UNPLUG_B:
        nonbtn &= ~NONBUTTON_HEADPHONE;
        break;
    case DOCK_INSERT:
        nonbtn |= NONBUTTON_DOCK;
        break;
    case DOCK_UNPLUG:
        nonbtn &= ~(NONBUTTON_DOCK | NONBUTTON_USB | NONBUTTON_POWER);
        break;
    case DOCK_USB_INSERT:
        nonbtn |= NONBUTTON_USB;
        break;
    case DOCK_USB_UNPLUG:
        nonbtn &= ~NONBUTTON_USB;
        break;
    case DOCK_POWER_INSERT:
        nonbtn |= NONBUTTON_POWER;
        break;
    case DOCK_POWER_UNPLUG:
        nonbtn &= ~NONBUTTON_POWER;
        break;
    }
#ifdef BUTTON_DEBUG
    unsigned char weergvn[10];
#ifdef BOOTLOADER
    lcd_set_foreground((sw ? LCD_RGBPACK(255,0,0) : LCD_RGBPACK(0,255,0) ));
#endif
    snprintf(weergvn, sizeof(char)*10, "%x", (unsigned int)((msg[3] << 24) | (msg[2] << 16) | (msg[1] << 8) | msg[0]));
    lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*10, LCD_HEIGHT-SYSFONT_HEIGHT*10, weergvn);
    snprintf(weergvn, sizeof(char)*10, "%x", btn);
    lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*10, LCD_HEIGHT-SYSFONT_HEIGHT*7, weergvn);
#ifdef BOOTLOADER
    lcd_set_foreground(LCD_BLACK);
#endif
    lcd_update();
    sw = !sw;
#endif
    /* Mask GIO0 interrupt */
    IO_INTC_IRQ1 = INTR_IRQ1_EXT0;
}

void send_command_to_pic(unsigned char in, unsigned char* out, unsigned int length)
{
    /* Disable GIO0 interrupt */
    IO_INTC_EINT1 &= ~INTR_EINT1_EXT0;
    /* Clear EXT0 interrupt */
    IO_INTC_IRQ1 = INTR_IRQ1_EXT0;
    /* Write command to I²C */
    restart:
    i2c_write(PIC_ADR, &in, 1);
    /* Wait for PIC */
    int i = 0;
    while(!(IO_INTC_IRQ1 & INTR_IRQ1_EXT0))
    {
        sleep(0);
        i++;
        if(i > 50)
            goto restart;
    }
    /* Read return from I²C */
    i2c_read(PIC_ADR, out, length);
    /* Re-enable GIO0 interrupt */
    IO_INTC_EINT1 |= INTR_EINT1_EXT0;
}

bool headphones_inserted(void)
{
    return (bool)(nonbtn & NONBUTTON_HEADPHONE);
}

void button_init_device(void)
{
    /* TODO: I suppose GIO0 has to be set to input and enable interrupts on it? */
    /* Enable GIO0 interrupt */
    IO_INTC_EINT1 |= INTR_EINT1_EXT0;
    btn = nonbtn = pic_init_value = pic_init2_value = last_btn = hold_switch = 0;
    /* Initialize PIC */
    send_command_to_pic(1, &pic_init_value, sizeof(pic_init_value));
    send_command_to_pic(2, &pic_init2_value, sizeof(pic_init2_value));
}

int get_debug_info(int choice)
{
    switch(choice)
    {
        case 1:
            return pic_init_value;
        case 2:
            return pic_init2_value;
        case 3:
            return last_btn;
        case 4:
            return nonbtn;
        case 5:
            return tick_diff;
    }
    return -1;
}

int button_read_device(void)
{
    return btn;
}

bool button_hold(void)
{
    return hold_switch;
}

bool button_usb_connected(void)
{
    return (bool)(nonbtn & NONBUTTON_USB);
}
