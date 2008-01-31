/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Jonas H�gqvist
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/
#include "plugin.h"
#include "playergfx.h"

PLUGIN_HEADER

#ifdef HAVE_LCD_BITMAP
#define DISPLAY_WIDTH LCD_WIDTH
#define DISPLAY_HEIGHT LCD_HEIGHT
#define RAND_SCALE 5

#ifdef HAVE_REMOTE_LCD
#define REMOTE_WIDTH LCD_REMOTE_WIDTH
#define REMOTE_HEIGHT LCD_REMOTE_HEIGHT
#include "remote_rockboxlogo.h"
#define REMOTE_LOGO_WIDTH BMPWIDTH_remote_rockboxlogo
#define REMOTE_LOGO_HEIGHT BMPHEIGHT_remote_rockboxlogo
#define REMOTE_LOGO remote_rockboxlogo
extern const fb_remote_data remote_rockboxlogo[];
#endif /* HAVE_REMOTE_LCD */

#define LOGO rockboxlogo
#include "rockboxlogo.h"
#define LOGO_WIDTH BMPWIDTH_rockboxlogo
#define LOGO_HEIGHT BMPHEIGHT_rockboxlogo
extern const fb_data rockboxlogo[];

#else /* !LCD_BITMAP */
#define DISPLAY_WIDTH 55
#define DISPLAY_HEIGHT 14
#define RAND_SCALE 2
#define LOGO_WIDTH 16
#define LOGO_HEIGHT 7
#define LOGO rockbox16x7
const unsigned char rockbox16x7[] = {
    0x47, 0x18, 0xa6, 0xd8, 0x66, 0xde, 0xb7, 0x9b,
    0x76, 0xdb, 0x26, 0xdb, 0x66, 0xde,
};
#endif /* !LCD_BITMAP */

/* variable button definitions */
#if CONFIG_KEYPAD == PLAYER_PAD
#define LP_QUIT BUTTON_STOP
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y (BUTTON_ON | BUTTON_LEFT)
#define LP_INC_Y (BUTTON_ON | BUTTON_RIGHT)
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define LP_QUIT BUTTON_MENU
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_SCROLL_BACK
#define LP_INC_Y BUTTON_SCROLL_FWD
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define LP_QUIT BUTTON_PLAY
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_DOWN
#define LP_INC_Y BUTTON_UP
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define LP_QUIT BUTTON_POWER
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_DOWN
#define LP_INC_Y BUTTON_UP
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define LP_QUIT BUTTON_POWER
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_DOWN
#define LP_INC_Y BUTTON_UP

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define LP_QUIT BUTTON_POWER
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_DOWN
#define LP_INC_Y BUTTON_UP

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define LP_QUIT BUTTON_POWER
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_SCROLL_DOWN
#define LP_INC_Y BUTTON_SCROLL_UP

#elif CONFIG_KEYPAD == MROBE500_PAD
#define LP_QUIT BUTTON_POWER
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_RC_DOWN
#define LP_INC_Y BUTTON_RC_PLAY

#else
#define LP_QUIT BUTTON_OFF
#define LP_DEC_X BUTTON_LEFT
#define LP_INC_X BUTTON_RIGHT
#define LP_DEC_Y BUTTON_DOWN
#define LP_INC_Y BUTTON_UP
#endif

#ifdef CONFIG_REMOTE_KEYPAD
#if (CONFIG_REMOTE_KEYPAD == H100_REMOTE) || \
    (CONFIG_REMOTE_KEYPAD == H300_REMOTE)
#define LP_R_QUIT BUTTON_RC_STOP
#define LP_R_DEC_X BUTTON_RC_REW
#define LP_R_INC_X BUTTON_RC_FF
#define LP_R_DEC_Y BUTTON_RC_SOURCE
#define LP_R_INC_Y BUTTON_RC_BITRATE
#endif
#endif /* CONFIG_REMOTE_KEYPAD */


enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    int button;
    int timer = 10;
    int x = (DISPLAY_WIDTH / 2) - (LOGO_WIDTH / 2);
    int y = (DISPLAY_HEIGHT / 2) - (LOGO_HEIGHT / 2);
    struct plugin_api* rb = api;
    int dx;
    int dy;
#ifdef HAVE_LCD_CHARCELLS
    int cpos = -1;
    int old_cpos = -1;
#endif

    (void)parameter;

#ifdef HAVE_LCD_CHARCELLS
    if (!pgfx_init(rb, 4, 2)) {
        rb->splash(HZ*2, "Old LCD :(");
        return PLUGIN_OK;
    }
#endif
    rb->srand(*rb->current_tick);
    dx = rb->rand()%(2*RAND_SCALE+1) - RAND_SCALE;
    dy = rb->rand()%(2*RAND_SCALE+1) - RAND_SCALE;

    while (1) {
#ifdef HAVE_LCD_BITMAP
        rb->lcd_clear_display();
        rb->lcd_bitmap(LOGO, x, y, LOGO_WIDTH, LOGO_HEIGHT);
#ifdef REMOTE_LOGO
        rb->lcd_remote_clear_display();
        rb->lcd_remote_bitmap(REMOTE_LOGO,
                (x * (REMOTE_WIDTH - REMOTE_LOGO_WIDTH)) / (DISPLAY_WIDTH - LOGO_WIDTH),
                (y * (REMOTE_HEIGHT - REMOTE_LOGO_HEIGHT)) / (DISPLAY_HEIGHT - LOGO_HEIGHT),
                REMOTE_LOGO_WIDTH, REMOTE_LOGO_HEIGHT);
#endif
#else
        pgfx_clear_display();
        pgfx_mono_bitmap(LOGO, x % 5, y, LOGO_WIDTH, LOGO_HEIGHT);
        cpos = x / 5;
#endif

        x += dx;
        if (x < 0) {
            dx = -dx;
            x = 0;
        }
        if (x > DISPLAY_WIDTH - LOGO_WIDTH) {
            dx = -dx;
            x = DISPLAY_WIDTH - LOGO_WIDTH;
        }

        y += dy;
        if (y < 0) {
            dy = -dy;
            y = 0;
        }
        if (y > DISPLAY_HEIGHT - LOGO_HEIGHT) {
            dy = -dy;
            y = DISPLAY_HEIGHT - LOGO_HEIGHT;
        }

#ifdef HAVE_LCD_BITMAP
        rb->lcd_update();
#ifdef REMOTE_LOGO
        rb->lcd_remote_update();
#endif
#else
        if (cpos != old_cpos) {
            rb->lcd_clear_display();
            pgfx_display(cpos, 0);
            old_cpos = cpos;
        }
        pgfx_update();
#endif
        rb->sleep(HZ/timer);
        
        button = rb->button_get(false);
        switch (button) {
            case LP_QUIT:
#ifdef CONFIG_REMOTE_KEYPAD
            case LP_R_QUIT:
#endif
#ifdef HAVE_LCD_CHARCELLS
                pgfx_release();
#endif
                return PLUGIN_OK;
            case LP_DEC_X:
#ifdef CONFIG_REMOTE_KEYPAD
            case LP_R_DEC_X:
#endif
                if (dx)
                    dx += (dx < 0) ? 1 : -1;
                break;
            case LP_INC_X:
#ifdef CONFIG_REMOTE_KEYPAD
            case LP_R_INC_X:
#endif
                dx += (dx < 0) ? -1 : 1;
                break;
            case LP_DEC_Y:
#ifdef CONFIG_REMOTE_KEYPAD
            case LP_R_DEC_Y:
#endif
                if (dy)
                    dy += (dy < 0) ? 1 : -1;
                break;
            case LP_INC_Y:
#ifdef CONFIG_REMOTE_KEYPAD
            case LP_R_INC_Y:
#endif
                dy += (dy < 0) ? -1 : 1;
                break;
                
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
#ifdef HAVE_LCD_CHARCELLS
                    pgfx_release();
#endif
                    return PLUGIN_USB_CONNECTED;
                }
                break;
        }
    }
}

