/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Philipp Pertermann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied. 
 *
 ****************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#ifndef SIMULATOR /* don't want this code in the simulator */
#if CONFIG_CODEC != SWCODEC /* only for MAS-targets */

/* The different drawing modes */
#define DRAW_MODE_FILLED  0
#define DRAW_MODE_OUTLINE 1
#define DRAW_MODE_PIXEL   2
#define DRAW_MODE_COUNT   3

#define MAX_PEAK 0x8000

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define OSCILLOGRAPH_QUIT BUTTON_OFF
#define OSCILLOGRAPH_SPEED_UP BUTTON_UP
#define OSCILLOGRAPH_SPEED_DOWN BUTTON_DOWN
#define OSCILLOGRAPH_ROLL BUTTON_F1
#define OSCILLOGRAPH_MODE BUTTON_F2
#define OSCILLOGRAPH_SPEED_RESET BUTTON_F3
#define OSCILLOGRAPH_PAUSE BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define OSCILLOGRAPH_QUIT BUTTON_OFF
#define OSCILLOGRAPH_SPEED_UP BUTTON_UP
#define OSCILLOGRAPH_SPEED_DOWN BUTTON_DOWN
#define OSCILLOGRAPH_ROLL BUTTON_RIGHT
#define OSCILLOGRAPH_MODE BUTTON_MENU
#define OSCILLOGRAPH_SPEED_RESET BUTTON_LEFT

#endif

/* global api struct pointer */
static struct plugin_api* rb;
/* number of ticks between two volume samples */
static int  speed = 1;
/* roll == true -> lcd rolls */
static bool roll = true;
/* see DRAW_MODE_XXX constants for valid values */
static int  drawMode = DRAW_MODE_FILLED;

/**
 * cleanup on return / usb
 */
void cleanup(void *parameter)
{
    (void)parameter;
    /* restore to default roll position.
       Looks funny if you forget to do this... */
    rb->lcd_roll(0);
    rb->lcd_update();
}

/**
 * Displays a vertically scrolling oscillosgraph using
 * hardware scrolling of the display. The user can change
 * speed
 */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;
    /* stores current volume value left */
    int  left;
    /* stores current volume value right */
    int  right;
    /* specifies the current position on the lcd */
    int  y = LCD_WIDTH - 1;

    /* only needed when drawing lines */
    int  lastLeft = 0;
    int  lastRight = 0;
    int  lasty = 0;
    
    bool exit = false;

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;
    
    /* the main loop */
    while (!exit) {

        /* read the volume info from MAS */
        left = rb->mas_codec_readreg(0xC) / (MAX_PEAK / (LCD_WIDTH / 2 - 2));
        right = rb->mas_codec_readreg(0xD) / (MAX_PEAK / (LCD_WIDTH / 2 - 2));

        /* delete current line */
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_drawline(0, y, LCD_WIDTH-1, y);

        rb->lcd_set_drawmode(DRMODE_SOLID);
        switch (drawMode) {
            case DRAW_MODE_FILLED:
                rb->lcd_drawline(LCD_WIDTH / 2 + 1        , y, 
                                 LCD_WIDTH / 2 + 1 + right, y);
                rb->lcd_drawline(LCD_WIDTH / 2 - 1        , y,
                                 LCD_WIDTH / 2 - 1 -left  , y);
                break;

            case DRAW_MODE_OUTLINE:
                /* last position needed for lines */
                lasty = MAX(y-1, 0);

                /* Here real lines were neccessary because 
                   anything else was ugly. */
                rb->lcd_drawline(LCD_WIDTH / 2 + right     , y, 
                                 LCD_WIDTH / 2 + lastRight , lasty);
                rb->lcd_drawline(LCD_WIDTH / 2 - left    , y,
                                 LCD_WIDTH / 2 - lastLeft, lasty);

                /* have to store the old values for drawing lines
                   the next time */
                lastRight = right;
                lastLeft = left;
                break;

            case DRAW_MODE_PIXEL:
                /* straight and simple */
                rb->lcd_drawpixel(LCD_WIDTH / 2 + right, y);
                rb->lcd_drawpixel(LCD_WIDTH / 2 - left, y);
                break;
        }


        /* increment and adjust the drawing position */
        y++;
        if (y >=  LCD_HEIGHT)
            y = 0;

        /* I roll before update because otherwise the new
           line would appear at the wrong end of the display */
        if (roll)
            rb->lcd_roll(y);

        /* now finally make the new sample visible */
        rb->lcd_update_rect(0, MAX(y-1, 0), LCD_WIDTH, 2);

        /* There are two mechanisms to alter speed:
           1.) slowing down is achieved by increasing
           the time waiting for user input. This
           mechanism uses positive values.
           2.) speeding up is achieved by leaving out
           the user input check for (-speed) volume
           samples. For this mechanism negative values
           are used.
        */
               
        if (speed >= 0 || ((speed < 0) && (y % (-speed) == 0))) {
            bool draw = false;

            /* speed values > 0 slow the oszi down. By user input
               speed might become < 1. If a value < 1 was
               passed user input would be disabled. Thus
               it must be ensured that at least 1 is passed. */

            /* react to user input */
            button = rb->button_get_w_tmo(MAX(speed, 1));
            switch (button) {
                case OSCILLOGRAPH_SPEED_UP:
                    speed++;
                    draw = true;
                    break;

                case OSCILLOGRAPH_SPEED_DOWN:
                    speed--;
                    draw = true;
                    break;

#ifdef OSCILLOGRAPH_PAUSE
                case OSCILLOGRAPH_PAUSE:
                    /* pause the demo */
                    rb->button_get(true);
                    break;
#endif

                case OSCILLOGRAPH_ROLL:
                    /* toggle rolling */
                    roll = !roll;
                    break;

                case OSCILLOGRAPH_MODE:
                    /* step through the display modes */
                    drawMode ++;
                    drawMode = drawMode % DRAW_MODE_COUNT;

                    /* lcd buffer might be rolled so that
                       the transition from LCD_HEIGHT to 0 
                       takes place in the middle of the screen.
                       That produces ugly results in DRAW_MODE_OUTLINE
                       mode. If rolling is enabled this change will
                       be reverted before the next update anyway.*/
                    rb->lcd_roll(0);
                    break;

                case OSCILLOGRAPH_SPEED_RESET:
                    speed = 1;
                    draw = true;
                    break;

                case OSCILLOGRAPH_QUIT:
                    exit = true;
                    break;

                default:
                    if (rb->default_event_handler_ex(button, cleanup, NULL)
                        == SYS_USB_CONNECTED)
                        return PLUGIN_USB_CONNECTED;
                    break;
            }

            if (draw) {
                char buf[16];
                rb->snprintf(buf, sizeof buf, "Speed: %d", -speed);
                rb->lcd_putsxy(0, (y + LCD_HEIGHT - 8) % LCD_HEIGHT, buf);
                rb->lcd_update_rect(0, (y + LCD_HEIGHT - 8) % LCD_HEIGHT, 
                                    LCD_WIDTH, 8);
            }
        }
    }

    cleanup(NULL);
    /* standard return */
    return PLUGIN_OK;
}

#endif /* if using MAS */
#endif /* #ifndef SIMULATOR */
#endif
