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

#ifndef SIMULATOR /* don't want this code in the simulator */

#include <stdlib.h>
#include <sprintf.h>
#include "menu.h"
#include "lcd.h"
#include "button.h"
#include "mas.h"
#include "system.h"

/* The different drawing modes */
#define DRAW_MODE_FILLED  0
#define DRAW_MODE_OUTLINE 1
#define DRAW_MODE_PIXEL   2
#define DRAW_MODE_COUNT   3

#define MAX_PEAK 0x8000

/* number of ticks between two volume samples */
static int  speed = 1;
/* roll == true -> lcd rolls */
static bool roll = true;
/* see DRAW_MODE_XXX constants for valid values */
static int  drawMode = DRAW_MODE_FILLED;

/**
 * Displays a vertically scrolling oscillosgraph using
 * hardware scrolling of the display. The user can change
 * speed
 */
bool oscillograph(void)
{
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

    /* the main loop */
    while (!exit) {

        /* read the volume info from MAS */
        left = mas_codec_readreg(0xC) / (MAX_PEAK / (LCD_WIDTH / 2 - 2));
        right = mas_codec_readreg(0xD) / (MAX_PEAK / (LCD_WIDTH / 2 - 2));

        /* delete current line */
        lcd_clearline(0, y, LCD_WIDTH-1, y);

        switch (drawMode) {
            case DRAW_MODE_FILLED:
                lcd_drawline(LCD_WIDTH / 2 + 1        , y, 
                             LCD_WIDTH / 2 + 1 + right, y);
                lcd_drawline(LCD_WIDTH / 2 - 1        , y,
                             LCD_WIDTH / 2 - 1 -left  , y);
                break;

            case DRAW_MODE_OUTLINE:
                /* last position needed for lines */
                lasty = MAX(y-1, 0);

                /* Here real lines were neccessary because 
                   anything else was ugly. */
                lcd_drawline(LCD_WIDTH / 2 + right     , y, 
                             LCD_WIDTH / 2 + lastRight , lasty);
                lcd_drawline(LCD_WIDTH / 2 - left    , y,
                             LCD_WIDTH / 2 - lastLeft, lasty);

                /* have to store the old values for drawing lines
                   the next time */
                lastRight = right;
                lastLeft = left;
                break;

            case DRAW_MODE_PIXEL:
                /* straight and simple */
                lcd_drawpixel(LCD_WIDTH / 2 + right, y);
                lcd_drawpixel(LCD_WIDTH / 2 - left, y);
                break;
        }


        /* increment and adjust the drawing position */
        y++;
        if (y >=  LCD_HEIGHT)
            y = 0;

        /* I roll before update because otherwise the new
           line would appear at the wrong end of the display */
        if (roll)
            lcd_roll(y);

        /* now finally make the new sample visible */
        lcd_update_rect(0, MAX(y-1, 0), LCD_WIDTH, 2);

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
            switch (button_get_w_tmo(MAX(speed, 1))) {
                case BUTTON_UP:
                    speed++;
                    draw = true;
                    break;

                case BUTTON_DOWN:
                    speed--;
                    draw = true;
                    break;

                case BUTTON_PLAY:
                    /* pause the demo */
                    button_get(true);
                    break;

                case BUTTON_F1:
                    /* toggle rolling */
                    roll = !roll;
                    break;

                case BUTTON_F2:
                    /* step through the display modes */
                    drawMode ++;
                    drawMode = drawMode % DRAW_MODE_COUNT;

                    /* lcd buffer might be rolled so that
                       the transition from LCD_HEIGHT to 0 
                       takes place in the middle of the screen.
                       That produces ugly results in DRAW_MODE_OUTLINE
                       mode. If rolling is enabled this change will
                       be reverted before the next update anyway.*/
                    lcd_roll(0);
                    break;

                case BUTTON_F3:
                    speed = 1;
                    draw = true;
                    break;

                case BUTTON_OFF:
                    exit = true;
                    break;
            }

            if (draw) {
                char buf[16];
                snprintf(buf, sizeof buf, "Speed: %d", -speed);
                lcd_putsxy(0, (y + LCD_HEIGHT - 8) % LCD_HEIGHT, buf);
                lcd_update_rect(0, (y + LCD_HEIGHT - 8) % LCD_HEIGHT, 
                                LCD_WIDTH, 8);
            }
        }    
    }

    /* restore to default roll position. 
       Looks funny if you forget to do this... */
    lcd_roll(0);
    lcd_update();

    /* standard return */
    return false;
}

#endif /* #ifndef SIMULATOR */