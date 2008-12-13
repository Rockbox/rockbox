/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 1999 Chris Allegretta
 * Copyright (C) 2005 Alastair S - ported to podzilla
 * Copyright (C) 2005 Jonas Häggqvist - ported to rockbox
 *   
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

/*
 * TODO: 
 *  - The font is a bit large, create smaller one
 *  - For colour/greyscale displays, the font from the xscreensaver xmatrix
 *    should be converted and used
 * BUGS:
 *  - The animation "dies" after a few seconds, not sure why. Works in sim.
 *  Symtom Fixed Oct 2007 GRaTT (Gerritt Gonzales)
 */


#include "plugin.h"
PLUGIN_HEADER

/* Images */
#define MAXCHARS 27 - 1
extern const fb_data matrix_bold[];
extern const fb_data matrix_normal[];
#define COL_W 14
#define COL_H 15

#define COLS LCD_WIDTH/COL_W
#define ROWS LCD_HEIGHT/COL_H

#define LEFTMARGIN (LCD_WIDTH-(COLS*COL_W))/2
#define TOPMARGIN  (LCD_HEIGHT-(ROWS*COL_H))/2

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MATRIX_EXIT BUTTON_MENU
#define MATRIX_SLEEP_MORE BUTTON_SCROLL_BACK|BUTTON_REPEAT
#define MATRIX_SLEEP_LESS BUTTON_SCROLL_FWD|BUTTON_REPEAT
#define MATRIX_PAUSE BUTTON_PLAY
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MATRIX_EXIT BUTTON_OFF
#define MATRIX_SLEEP_MORE BUTTON_UP
#define MATRIX_SLEEP_LESS BUTTON_DOWN
#define MATRIX_PAUSE BUTTON_SELECT
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define MATRIX_EXIT BUTTON_POWER
#define MATRIX_SLEEP_MORE BUTTON_SCROLL_UP|BUTTON_REPEAT
#define MATRIX_SLEEP_LESS BUTTON_SCROLL_DOWN|BUTTON_REPEAT
#define MATRIX_PAUSE BUTTON_PLAY
#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define MATRIX_EXIT BUTTON_POWER
#define MATRIX_SLEEP_MORE BUTTON_UP
#define MATRIX_SLEEP_LESS BUTTON_DOWN
#define MATRIX_PAUSE BUTTON_PLAY
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define MATRIX_EXIT BUTTON_POWER
#define MATRIX_SLEEP_MORE BUTTON_UP
#define MATRIX_SLEEP_LESS BUTTON_DOWN
#define MATRIX_PAUSE BUTTON_SELECT
#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define MATRIX_EXIT BUTTON_POWER
#define MATRIX_SLEEP_MORE BUTTON_SCROLL_BACK|BUTTON_REPEAT
#define MATRIX_SLEEP_LESS BUTTON_SCROLL_FWD|BUTTON_REPEAT
#define MATRIX_PAUSE BUTTON_SELECT
#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define MATRIX_EXIT BUTTON_POWER
#define MATRIX_SLEEP_MORE BUTTON_UP
#define MATRIX_SLEEP_LESS BUTTON_DOWN
#define MATRIX_PAUSE BUTTON_SELECT
#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define MATRIX_EXIT BUTTON_BACK
#define MATRIX_SLEEP_MORE BUTTON_UP
#define MATRIX_SLEEP_LESS BUTTON_DOWN
#define MATRIX_PAUSE BUTTON_SELECT
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define MATRIX_EXIT BUTTON_RC_REC
#define MATRIX_SLEEP_MORE BUTTON_RC_VOL_UP
#define MATRIX_SLEEP_LESS BUTTON_RC_VOL_DOWN
#define MATRIX_PAUSE BUTTON_RC_PLAY
#elif (CONFIG_KEYPAD == COWOND2_PAD)
#define MATRIX_EXIT BUTTON_POWER
#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define MATRIX_EXIT BUTTON_POWER
#define MATRIX_SLEEP_MORE BUTTON_VOLUP
#define MATRIX_SLEEP_LESS BUTTON_VOLDOWN
#define MATRIX_PAUSE BUTTON_PLAY
#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define MATRIX_EXIT BUTTON_BACK
#define MATRIX_SLEEP_MORE BUTTON_UP
#define MATRIX_SLEEP_LESS BUTTON_DOWN
#define MATRIX_PAUSE BUTTON_PLAY
#else
#error Unsupported keypad
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef MATRIX_EXIT
#define MATRIX_EXIT       BUTTON_TOPLEFT
#endif
#ifndef MATRIX_SLEEP_MORE
#define MATRIX_SLEEP_MORE BUTTON_MIDRIGHT
#endif
#ifndef MATRIX_SLEEP_LESS
#define MATRIX_SLEEP_LESS BUTTON_MIDLEFT
#endif
#ifndef MATRIX_PAUSE
#define MATRIX_PAUSE      BUTTON_CENTER
#endif
#endif

#define SLEEP HZ/50

/* Codec api pointer */
static const struct plugin_api* rb;

/* Each position is of this type */
typedef struct cmatrix {
    int val;
    int bold;
} cmatrix;

/* The matrix - who'd have guessed it was just a few hundred bytes? */
static cmatrix matrix[ROWS][COLS];
static int length[COLS];
static int spaces[COLS];
static int updates[COLS];

static void matrix_init(void) {
    int i,j;

    /* Seed rand */
    rb->srand(*rb->current_tick);

    /* Make the matrix */
    for (i = 0; i <= ROWS; i++) {
        for (j = 0; j <= COLS - 1; j++ ) {
            matrix[i][j].val = -1;
            matrix[i][j].bold = 0;
        }
    }

    for (j = 0; j <= COLS - 1; j++) {
        /* Set up spaces[] array of how many spaces to skip */
        spaces[j] = rb->rand() % ROWS + 1;

        /* And length of the stream */
        length[j] = rb->rand() % (ROWS - 3) + 3;

        /* Sentinel value for creation of new objects */
        matrix[1][j].val = 129;

        /* And set updates[] array for update speed. */
        updates[j] = rb->rand() % 3 + 1;
    }
}

static void matrix_blit_char(const int row, const int col, int cha)
{
    if (cha == 129 || cha == 2 || cha > MAXCHARS)
        cha = 0;

    if (matrix[row][col].bold == 1) {
        rb->lcd_bitmap_part(matrix_bold, cha*COL_W, 0, 392,
                col*COL_W + LEFTMARGIN, row*COL_H + TOPMARGIN, COL_W, COL_H);
    }
    else {
        rb->lcd_bitmap_part(matrix_normal, cha*COL_W, 0, 392,
                col*COL_W + LEFTMARGIN, row*COL_H + TOPMARGIN, COL_W, COL_H);
    }
}

static void matrix_loop(void)
{
    int i, j = 0, y, z, firstcoldone = 0;
    static int count = 0;
    int randomness = 6;

    count++;
    if (count > 4)
        count = 1;

    for (j = 0; j <= COLS - 1; j++) {
        if (count > updates[j]) {
            /* New style scrolling */
            if (matrix[0][j].val == -1 && matrix[1][j].val == 129
                    && spaces[j] > 0) {
                matrix[0][j].val = -1;
                spaces[j]--;
            } else if (matrix[0][j].val == -1 && matrix[1][j].val == 129){
                length[j] = rb->rand() % (ROWS - 3) + 3;
                matrix[0][j].val = rb->rand() % (MAXCHARS-1) + 1;
                if (rb->rand() % 2 == 1)
                    matrix[0][j].bold = 2;
                spaces[j] = rb->rand() % ROWS + 1;
            }
            i = 0;
            y = 0;
            firstcoldone = 0;
            while (i <= ROWS) {
	        /* Skip over spaces */
		/* this is whear the characters were disappearing */
/*	       
                while (i <= ROWS && (matrix[i][j].val == 129 ||
                            matrix[i][j].val == -1))
                    i++;
*/
	/* A little more random now for spaces */
               if (rb->rand() % randomness == 1){
	           while (i <= ROWS && (matrix[i][j].val == 129 ||
                            matrix[i][j].val == -1)){
                    i++;
		    randomness--;
	            if(randomness <=1)
	                randomness = 6;}
                }else{
		    randomness++;
	            if(randomness >6)
	            randomness = 6;
		}


                if (i > ROWS)
                    break;

                /* Go to the head of this collumn */
                z = i;
                y = 0;
                while (i <= ROWS && (matrix[i][j].val != 129 &&
                            matrix[i][j].val != -1)) {
                    i++;
                    y++;
                }

                if (i > ROWS) {
                    matrix[z][j].val = 129;
                    matrix[ROWS][j].bold = 1;
                    matrix_blit_char(z - 1, j, matrix[z][j].val);
                    continue;
                }

                matrix[i][j].val = rb->rand() % (MAXCHARS-1) + 1;

                if (matrix[i - 1][j].bold == 2) {
                    matrix[i - 1][j].bold = 1;
                    matrix[i][j].bold = 2;
                }

                /* If we're at the top of the collumn and it's reached its
                 * full length (about to start moving down), we do this
                 * to get it moving. This is also how we keep segments
                 * not already growing from growing accidentally =>
                 */
                if (y > length[j] || firstcoldone) {
                    matrix[z][j].val = 129;
                    matrix[0][j].val = -1;
                }
                firstcoldone = 1;
                i++;
            }
            for (i = 1; i <= ROWS; i++) {
                if (matrix[i][j].val == 0 || matrix[i][j].bold == 2) {
                    if (matrix[i][j].val == 0)
                        matrix_blit_char(i - 1, j, 20);
                    else
                        matrix_blit_char(i - 1, j, matrix[i][j].val);
                } else {
                    if (matrix[i][j].val == 1)
                        matrix_blit_char(i - 1, j, 2);
                    else if (matrix[i][j].val == -1)
                        matrix_blit_char(i - 1, j, 129);
                    else
                        matrix_blit_char(i - 1, j, matrix[i][j].val);
                }
            }
        }
    }
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter) {
    int button;
    int sleep = SLEEP;
    bool frozen = false;

    (void)parameter;
    rb = api;

    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_clear_display();
    matrix_init();

    while (1) {
        if (!frozen) {
            matrix_loop();
            rb->lcd_update();
            rb->sleep(sleep);
        }
        button = rb->button_get(frozen);
        switch(button) {
            case MATRIX_PAUSE:
                frozen = !frozen;
                break;
            case MATRIX_EXIT:
                return PLUGIN_OK;
                break;
            case MATRIX_SLEEP_MORE:
                /* Sleep longer */
                sleep += SLEEP;
                break;
            case MATRIX_SLEEP_LESS:
                /* Sleep less */
                sleep -= SLEEP;
                if (sleep < 0) sleep = 0;
                break;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                    return PLUGIN_USB_CONNECTED;
                }
                break;
        }
    }
    return PLUGIN_OK;
}
