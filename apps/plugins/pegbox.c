/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Tom Ross
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

#if (LCD_WIDTH >= 138) && (LCD_HEIGHT >= 110)
#include "pegbox_menu_top.h"
#include "pegbox_menu_items.h"
#include "pegbox_header.h"
#endif
#include "pegbox_pieces.h"

static struct plugin_api* rb;

PLUGIN_HEADER

/* final game return status */
#define PB_END  3
#define PB_USB  2
#define PB_QUIT 1

#define DATA_FILE   PLUGIN_DIR "/games/pegbox.data"
#define SAVE_FILE   PLUGIN_DIR "/games/pegbox.save"

#define ROWS       8   /* Number of rows on each board */
#define COLS       12  /* Number of columns on each board */
#define NUM_LEVELS 15  /* Number of levels */

#define SPACE     0
#define WALL      1
#define TRIANGLE  2
#define CROSS     3
#define SQUARE    4
#define CIRCLE    5
#define HOLE      6
#define PLAYER    7

#if CONFIG_KEYPAD == RECORDER_PAD
#define PEGBOX_SELECT   BUTTON_ON
#define PEGBOX_QUIT     BUTTON_OFF
#define PEGBOX_SAVE     BUTTON_F2
#define PEGBOX_RESTART  BUTTON_ON
#define PEGBOX_LVL_UP   BUTTON_F1
#define PEGBOX_LVL_DOWN BUTTON_F3
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "ON"
#define LVL_UP_TEXT "F1"
#define LVL_DOWN_TEXT "F3"
#define SAVE_TEXT "F2"
#define QUIT_TEXT "OFF"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define PEGBOX_SELECT   BUTTON_MENU
#define PEGBOX_QUIT     BUTTON_OFF
#define PEGBOX_SAVE     (BUTTON_MENU | BUTTON_UP)
#define PEGBOX_RESTART  (BUTTON_MENU | BUTTON_DOWN)
#define PEGBOX_LVL_UP   (BUTTON_MENU | BUTTON_LEFT)
#define PEGBOX_LVL_DOWN (BUTTON_MENU | BUTTON_RIGHT)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "M+DOWN"
#define LVL_UP_TEXT "M+RIGHT"
#define LVL_DOWN_TEXT "M+LEFT"
#define SAVE_TEXT "M+UP"
#define QUIT_TEXT "OFF"

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_OFF
#define PEGBOX_SAVE     BUTTON_MODE
#define PEGBOX_RESTART  BUTTON_SELECT
#define PEGBOX_LVL_UP   BUTTON_ON
#define PEGBOX_LVL_DOWN BUTTON_REC
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "NAVI"
#define LVL_UP_TEXT "PLAY"
#define LVL_DOWN_TEXT "REC"
#define SAVE_TEXT "AB"
#define QUIT_TEXT "OFF"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define PEGBOX_SELECT   (BUTTON_SELECT|BUTTON_REL)
#define PEGBOX_QUIT     (BUTTON_SELECT|BUTTON_MENU)
#define PEGBOX_SAVE     (BUTTON_SELECT|BUTTON_RIGHT)
#define PEGBOX_RESTART  (BUTTON_SELECT|BUTTON_LEFT)
#define PEGBOX_LVL_UP   BUTTON_SCROLL_BACK
#define PEGBOX_LVL_DOWN BUTTON_SCROLL_FWD
#define PEGBOX_UP       BUTTON_MENU
#define PEGBOX_DOWN     BUTTON_PLAY
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "SELECT+LEFT"
#define LVL_UP_TEXT "SCROLL BACK"
#define LVL_DOWN_TEXT "SCROLL FWD"
#define SAVE_TEXT "SELECT+RIGHT"
#define QUIT_TEXT "SELECT+MENU"

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_SAVE     BUTTON_PLAY
#define PEGBOX_RESTART  BUTTON_REC
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "-"
#define LVL_DOWN_TEXT "-"
#define SAVE_TEXT "PLAY"
#define QUIT_TEXT "OFF"

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define PEGBOX_SELECT   BUTTON_MODE
#define PEGBOX_QUIT     BUTTON_PLAY
#define PEGBOX_SAVE     (BUTTON_EQ|BUTTON_MODE)
#define PEGBOX_RESTART  BUTTON_MODE
#define PEGBOX_LVL_UP   (BUTTON_EQ|BUTTON_UP)
#define PEGBOX_LVL_DOWN (BUTTON_EQ|BUTTON_DOWN)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "MODE"
#define LVL_UP_TEXT "EQ+UP"
#define LVL_DOWN_TEXT "EQ+DOWN"
#define SAVE_TEXT "EQ+MODE"
#define QUIT_TEXT "PLAY"

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define PEGBOX_SELECT   BUTTON_PLAY
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_SAVE     BUTTON_FF
#define PEGBOX_RESTART  (BUTTON_FF|BUTTON_REPEAT)
#define PEGBOX_LVL_UP   (BUTTON_FF|BUTTON_SCROLL_UP)
#define PEGBOX_LVL_DOWN (BUTTON_FF|BUTTON_SCROLL_DOWN)
#define PEGBOX_UP       BUTTON_SCROLL_UP
#define PEGBOX_DOWN     BUTTON_SCROLL_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "LONG FF"
#define LVL_UP_TEXT "FF+SCROLL_UP"
#define LVL_DOWN_TEXT "FF+SCROLL_DOWN"
#define SAVE_TEXT "FF"
#define QUIT_TEXT "OFF"

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_SAVE     BUTTON_REC
#define PEGBOX_RESTART  BUTTON_SELECT
#define PEGBOX_LVL_UP   BUTTON_SCROLL_BACK
#define PEGBOX_LVL_DOWN BUTTON_SCROLL_FWD
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "SELECT"
#define LVL_UP_TEXT "SCROLL_UP"
#define LVL_DOWN_TEXT "SCROLL_DOWN"
#define SAVE_TEXT "REC"
#define QUIT_TEXT "POWER"

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_SAVE     BUTTON_MENU
#define PEGBOX_RESTART  BUTTON_A
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "POWER"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"
#define SAVE_TEXT "MENU"
#define QUIT_TEXT "A"

#elif CONFIG_KEYPAD == MROBE100_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_SAVE     BUTTON_MENU
#define PEGBOX_RESTART  BUTTON_PLAY
#define PEGBOX_LVL_UP   (BUTTON_DISPLAY | BUTTON_REL)
#define PEGBOX_LVL_DOWN (BUTTON_DISPLAY | BUTTON_REPEAT)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "PLAY"
#define LVL_UP_TEXT "DISPLAY+UP"
#define LVL_DOWN_TEXT "DISPLAY+DOWN"
#define SAVE_TEXT "MENU"
#define QUIT_TEXT "POWER"

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_SAVE     (BUTTON_REC | BUTTON_SELECT)
#define PEGBOX_RESTART  (BUTTON_REC | BUTTON_REL)
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"
#define SAVE_TEXT "REC+SELECT"
#define QUIT_TEXT "POWER"

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define PEGBOX_SELECT   BUTTON_RC_PLAY
#define PEGBOX_QUIT     BUTTON_RC_REC
#define PEGBOX_SAVE     BUTTON_RC_MENU
#define PEGBOX_RESTART  BUTTON_RC_MODE
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_RC_VOL_UP
#define PEGBOX_DOWN     BUTTON_RC_VOL_DOWN
#define PEGBOX_RIGHT    BUTTON_RC_FF
#define PEGBOX_LEFT     BUTTON_RC_REW

#define RESTART_TEXT "REM. MODE"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"
#define SAVE_TEXT "REM. PLAY"
#define QUIT_TEXT "REM. REC"

#elif CONFIG_KEYPAD == COWOND2_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_SAVE     BUTTON_MENU
#define PEGBOX_RESTART  BUTTON_MINUS
#define PEGBOX_LVL_UP   BUTTON_PLUS
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define RESTART_TEXT "MINUS"
#define LVL_UP_TEXT "PLUS"
#define LVL_DOWN_TEXT "-"
#define SAVE_TEXT "MENU"
#define QUIT_TEXT "POWER"
#endif

#if (LCD_WIDTH >= 320) && (LCD_HEIGHT >= 240)
#define LEVEL_TEXT_X   59
#define PEGS_TEXT_X    276
#define TEXT_Y         28
#elif (LCD_WIDTH >= 240) && (LCD_HEIGHT >= 192)
#define LEVEL_TEXT_X   59
#define PEGS_TEXT_X    196
#define TEXT_Y         28
#elif (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
#define LEVEL_TEXT_X   49
#define PEGS_TEXT_X    186
#define TEXT_Y         28
#elif (LCD_WIDTH >= 176) && (LCD_HEIGHT >= 132)
#define LEVEL_TEXT_X   38
#define PEGS_TEXT_X    155
#define TEXT_Y         17
#elif (LCD_WIDTH >= 160) && (LCD_HEIGHT >= 128)
#define LEVEL_TEXT_X   37
#define PEGS_TEXT_X    140
#define TEXT_Y         13
#elif (LCD_WIDTH >= 138) && (LCD_HEIGHT >= 110)
#define LEVEL_TEXT_X   28
#define PEGS_TEXT_X    119
#define TEXT_Y         15
#elif (LCD_WIDTH >= 128) && (LCD_HEIGHT >= 128)
#define LEVEL_TEXT_X   28
#define PEGS_TEXT_X    119
#define TEXT_Y         15
#endif

#ifdef HAVE_LCD_COLOR
#define BG_COLOR           LCD_BLACK
#define TEXT_BG            LCD_RGBPACK(189,189,189)
#endif

#define BOARD_X   (LCD_WIDTH-12*BMPWIDTH_pegbox_pieces)/2

#if (LCD_WIDTH >= 138) && (LCD_HEIGHT >= 110)
#define BOARD_Y   BMPHEIGHT_pegbox_header+2
#else
#define BOARD_Y   0
#endif

struct game_context {
    unsigned int level;
    unsigned int highlevel;
    signed int player_row;
    signed int player_col;
    unsigned int num_left;
    bool save_exist;
    unsigned int playboard[ROWS][COLS];
};

char levels[NUM_LEVELS][ROWS][COLS] = {
    /* Level 1 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 0, 0, 0, 7, 0, 0, 5, 0, 0, 0, 1,},
     {1, 0, 0, 0, 0, 3, 3, 2, 0, 0, 0, 1,},
     {1, 0, 0, 0, 4, 6, 0, 5, 0, 0, 0, 1,},
     {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 2 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 0, 0, 0, 2, 2, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1,},
     {7, 0, 0, 0, 2, 2, 5, 5, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 2, 2, 0, 0, 0, 1,}},

    /* Level 3 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 0, 0, 0, 0, 0, 2, 0, 7, 0, 0, 0,},
     {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 2, 1,},
     {1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 4 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {6, 0, 4, 0, 2, 0, 2, 0, 0, 0, 0, 1,},
     {6, 0, 3, 0, 5, 0, 2, 0, 0, 0, 0, 1,},
     {6, 0, 5, 0, 4, 7, 2, 0, 0, 0, 0, 1,},
     {6, 0, 2, 0, 4, 0, 2, 0, 3, 0, 0, 1,},
     {6, 0, 4, 0, 5, 0, 2, 0, 0, 0, 0, 1,},
     {6, 0, 5, 0, 4, 0, 2, 0, 0, 0, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 5 */
    {{0, 6, 6, 0, 4, 6, 0, 0, 6, 0, 0, 0,},
     {0, 6, 6, 0, 4, 4, 0, 0, 6, 0, 0, 2,},
     {2, 6, 6, 0, 6, 6, 6, 0, 1, 2, 2, 2,},
     {0, 6, 6, 0, 6, 4, 6, 0, 1, 2, 0, 2,},
     {0, 6, 6, 0, 6, 7, 6, 5, 6, 0, 0, 0,},
     {2, 6, 6, 0, 6, 6, 6, 0, 6, 0, 0, 0,},
     {0, 6, 6, 0, 4, 0, 0, 0, 6, 0, 0, 0,},
     {0, 6, 6, 0, 0, 5, 0, 0, 6, 5, 5, 0,}},

    /* Level 6 */
    {{7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
     {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,},
     {2, 0, 0, 2, 0, 0, 0, 2, 2, 0, 0, 0,},
     {0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0,},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
     {0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1,},
     {0, 3, 0, 0, 0, 0, 0, 0, 5, 4, 6, 0,},
     {0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1,}},

    /* Level 7 */
    {{1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 6, 0, 0, 4, 6, 0, 1, 1, 1,},
     {1, 1, 1, 1, 0, 1, 5, 1, 0, 1, 1, 1,},
     {1, 1, 1, 2, 3, 3, 7, 4, 2, 6, 1, 1,},
     {1, 1, 1, 1, 0, 1, 2, 1, 0, 0, 0, 1,},
     {1, 1, 1, 1, 0, 0, 5, 0, 0, 1, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,}},

    /* Level 8 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {0, 0, 3, 3, 3, 3, 3, 4, 3, 3, 0, 0,},
     {0, 0, 3, 3, 3, 2, 3, 3, 5, 3, 0, 0,},
     {7, 0, 3, 3, 3, 2, 3, 3, 4, 3, 0, 0,},
     {0, 0, 3, 3, 4, 5, 3, 3, 3, 3, 0, 0,},
     {0, 0, 3, 3, 5, 2, 3, 3, 3, 3, 0, 0,},
     {0, 0, 3, 3, 2, 4, 3, 3, 3, 3, 0, 0,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 9 */
    {{0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,},
     {0, 3, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,},
     {0, 0, 0, 0, 2, 4, 4, 3, 0, 1, 1, 0,},
     {0, 1, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1,},
     {0, 0, 0, 2, 2, 7, 1, 0, 0, 0, 0, 2,},
     {0, 0, 0, 0, 2, 1, 0, 0, 1, 1, 1, 1,},
     {0, 3, 1, 0, 2, 5, 5, 0, 0, 0, 3, 0,},
     {0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0,}},

    /* Level 10 */
    {{1, 1, 1, 1, 2, 1, 1, 1, 0, 0, 0, 0,},
     {1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 7,},
     {0, 0, 4, 0, 6, 6, 6, 0, 0, 0, 3, 0,},
     {0, 3, 3, 0, 6, 6, 6, 0, 4, 3, 4, 0,},
     {0, 3, 3, 0, 6, 6, 6, 0, 4, 3, 4, 0,},
     {0, 0, 0, 0, 6, 6, 6, 0, 3, 0, 0, 0,},
     {1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0,},
     {1, 1, 1, 1, 1, 2, 1, 1, 0, 0, 0, 0,}},

    /* Level 11 */
    {{1, 7, 1, 0, 1, 1, 6, 0, 0, 1, 1, 0,},
     {1, 0, 0, 0, 5, 4, 6, 6, 0, 2, 2, 0,},
     {1, 2, 1, 2, 0, 1, 6, 0, 0, 2, 2, 0,},
     {1, 0, 0, 2, 0, 1, 0, 0, 0, 3, 3, 0,},
     {1, 2, 1, 0, 0, 1, 0, 1, 0, 3, 3, 0,},
     {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0,},
     {0, 3, 4, 3, 0, 1, 0, 1, 0, 0, 0, 0,},
     {0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 1, 1,}},

    /* Level 12 */
    {{7, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,},
     {1, 2, 1, 2, 1, 2, 1, 1, 0, 0, 0, 1,},
     {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1,},
     {1, 2, 1, 2, 1, 2, 1, 0, 0, 0, 0, 1,},
     {0, 0, 0, 0, 0, 0, 1, 1, 5, 5, 6, 1,},
     {1, 2, 1, 2, 1, 2, 1, 1, 0, 2, 2, 1,},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1,},
     {1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1,}},

    /* Level 13 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {0, 0, 4, 0, 2, 0, 5, 0, 4, 0, 0, 6,},
     {0, 0, 2, 0, 5, 0, 2, 0, 4, 0, 0, 6,},
     {7, 0, 3, 0, 4, 0, 5, 0, 4, 0, 0, 6,},
     {0, 0, 5, 0, 4, 0, 2, 0, 4, 0, 0, 6,},
     {0, 0, 4, 0, 2, 0, 5, 0, 4, 0, 0, 6,},
     {0, 0, 3, 0, 3, 0, 2, 0, 4, 0, 0, 6,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 14 */
    {{1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,},
     {1, 1, 0, 2, 0, 0, 4, 0, 0, 2, 0, 1,},
     {1, 6, 0, 0, 5, 1, 0, 1, 1, 0, 0, 1,},
     {1, 1, 1, 0, 0, 3, 5, 3, 0, 0, 1, 1,},
     {1, 1, 0, 0, 1, 1, 0, 1, 5, 0, 0, 6,},
     {1, 1, 0, 2, 0, 0, 4, 0, 0, 0, 7, 1,},
     {1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 15 */
    {{0, 0, 0, 6, 0, 6, 0, 6, 0, 6, 0, 1,},
     {0, 0, 0, 6, 0, 6, 0, 6, 0, 6, 0, 1,},
     {0, 3, 4, 6, 0, 6, 0, 6, 0, 6, 0, 2,},
     {0, 4, 0, 6, 0, 6, 4, 6, 0, 6, 0, 1,},
     {0, 3, 0, 6, 0, 6, 0, 6, 0, 6, 0, 1,},
     {7, 0, 0, 6, 4, 6, 0, 6, 0, 6, 0, 1,},
     {0, 0, 4, 6, 0, 6, 0, 6, 4, 6, 0, 1,},
     {0, 0, 4, 6, 0, 6, 0, 6, 0, 6, 0, 1,}}
};


/*****************************************************************************
* display_text() formats and outputs text.
******************************************************************************/
static void display_text(char *str, bool waitkey)
{
    int chars_by_line;
    int lines_by_screen;
    int chars_for_line;
    int current_line = 0;
    int char_width, char_height;
    int first_char_index = 0;
    char *ptr_char;
    char *ptr_line;
    int i;
    char line[255];
    int key;
    bool go_on;

    rb->lcd_clear_display();
    
    rb->lcd_getstringsize("a", &char_width, &char_height);

    chars_by_line = LCD_WIDTH / char_width;
    lines_by_screen = LCD_HEIGHT / char_height;

    do
    {
        ptr_char = str + first_char_index;
        chars_for_line = 0;
        i = 0;
        ptr_line = line;
        while (i < chars_by_line)
        {
            switch (*ptr_char)
            {
                case '\t':
                case ' ':
                    *(ptr_line++) = ' ';
                case '\n':
                case '\0':
                    chars_for_line = i;
                    break;

                default:
                    *(ptr_line++) = *ptr_char;
            }
            if (*ptr_char == '\n' || *ptr_char == '\0')
                break;
            ptr_char++;
            i++;
        }

        if (chars_for_line == 0)
            chars_for_line = i;

        line[chars_for_line] = '\0';

        /* test if we have cut a word. If it is the case we don't have to */
        /* skip the space */
        if (i == chars_by_line && chars_for_line == chars_by_line)
            first_char_index += chars_for_line;
        else
            first_char_index += chars_for_line + 1;

        /* print the line on the screen */
        rb->lcd_putsxy(0, current_line * char_height, line);

        /* if the number of line showed on the screen is equals to the */
        /* maximum number of line we can show, we wait for a key pressed to */
        /* clear and show the remaining text. */
        current_line++;
        if (current_line == lines_by_screen || *ptr_char == '\0')
        {
            current_line = 0;
            rb->lcd_update();
            go_on = false;
            while (waitkey && !go_on)
            {
                key = rb->button_get(true);
                switch (key)
                {
                    case PEGBOX_QUIT:
                    case PEGBOX_LEFT:
                    case PEGBOX_DOWN:
                        go_on = true;
                        break;

                    default:
                       /*if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                        {
                            usb_detected = true;
                            go_on = true;
                            break;
                        }*/
                        break;
                }
            }
            rb->lcd_clear_display();
        }
    } while (*ptr_char != '\0');
}

/*****************************************************************************
* draw_board() draws the game's current level.
******************************************************************************/
static void draw_board(struct game_context* pb) {
    unsigned int r, c, type;
    pb->num_left = 0;
#if (LCD_WIDTH >= 138) && (LCD_HEIGHT >= 110)
    char str[5];

    rb->lcd_clear_display();
    rb->lcd_bitmap(pegbox_header,0,0,LCD_WIDTH, BMPHEIGHT_pegbox_header);
    rb->lcd_drawrect((LCD_WIDTH-12*BMPWIDTH_pegbox_pieces)/2-2,
                     BMPHEIGHT_pegbox_header,
                     12*BMPWIDTH_pegbox_pieces+4,8*BMPWIDTH_pegbox_pieces+4);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_fillrect((LCD_WIDTH-12*BMPWIDTH_pegbox_pieces)/2-1,BMPHEIGHT_pegbox_header+1,
                     12*BMPWIDTH_pegbox_pieces+2,8*BMPWIDTH_pegbox_pieces+2);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(TEXT_BG);
#endif

#else
    rb->lcd_clear_display();
#endif

    for (r=0 ; r < ROWS ; r++) {
        for (c = 0 ; c < COLS ; c++) {

            type = pb->playboard[r][c];

            switch(type) {
                case SPACE:
                    break;

                default:
                    rb->lcd_bitmap_part(pegbox_pieces, 0, 
                                        (type-1)*BMPWIDTH_pegbox_pieces, 
                                        BMPWIDTH_pegbox_pieces, 
                                        c * BMPWIDTH_pegbox_pieces + BOARD_X, 
                                        r * BMPWIDTH_pegbox_pieces + BOARD_Y, 
                                        BMPWIDTH_pegbox_pieces, 
                                        BMPWIDTH_pegbox_pieces);
                    break;
            }

            if(pb->playboard[r][c] == PLAYER) {
                pb->player_row=r;
                pb->player_col=c;
            }
            else if (type != WALL && type != SPACE && type != HOLE)
                pb->num_left++;
        }
    }
#if (LCD_WIDTH >= 138) && (LCD_HEIGHT >= 110)
    rb->snprintf(str, 3, "%d", pb->level);
    rb->lcd_putsxy(LEVEL_TEXT_X, TEXT_Y, str);
    rb->snprintf(str, 3, "%d", pb->num_left);
    rb->lcd_putsxy(PEGS_TEXT_X, TEXT_Y, str);
#endif

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(BG_COLOR);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    /* print out the screen */
    rb->lcd_update();
}

/*****************************************************************************
* load_level() loads the player's current level from the array and sets the
* player's position.
******************************************************************************/
static void load_level(struct game_context* pb) {
    int r, c;

    for(r = 0; r < ROWS; r++)
        for(c = 0; c < COLS; c++)
            pb->playboard[r][c] = levels[pb->level-1][r][c];
}

/*****************************************************************************
* new_piece() creates a new piece at a specified location. The player
* navigates through the pieces and selects one.
******************************************************************************/
static void new_piece(struct game_context* pb, unsigned int x_loc, 
                          unsigned int y_loc) {
    int button;
    bool exit = false;

    pb->playboard[x_loc][y_loc] = TRIANGLE;

    while (!exit) {
        draw_board(pb);
        button = rb->button_get(true);
        switch(button){
            case PEGBOX_LEFT:
            case (PEGBOX_LEFT|BUTTON_REPEAT):
            case PEGBOX_DOWN:
            case (PEGBOX_DOWN|BUTTON_REPEAT):
                if (pb->playboard[x_loc][y_loc] < 5)
                    pb->playboard[x_loc][y_loc]++;
                else
                    pb->playboard[x_loc][y_loc] = TRIANGLE;
                break;
            case PEGBOX_RIGHT:
            case (PEGBOX_RIGHT|BUTTON_REPEAT):
            case PEGBOX_UP:
            case (PEGBOX_UP|BUTTON_REPEAT):
                if (pb->playboard[x_loc][y_loc] > 2)
                    pb->playboard[x_loc][y_loc]--;
                else
                    pb->playboard[x_loc][y_loc] = CIRCLE;
                break;
            case PEGBOX_SELECT:
                exit = true;
                break;
        }
    }
}

/*****************************************************************************
* move_player() moves the player and pieces and updates the board accordingly.
******************************************************************************/
static void move_player(struct game_context* pb, signed int x_dir, 
                            signed int y_dir) {
    unsigned int type1, type2;
    signed int r,c;
    
    r = pb->player_row+y_dir;
    c = pb->player_col+x_dir;
    
    type1 = pb->playboard[r][c];
    type2 = pb->playboard[r+y_dir][c+x_dir];

    if (r == ROWS || c == COLS || r < 0 || c < 0 || type1 == WALL)
        return;
    else if(type1 != SPACE) {
        if (type2 == WALL || r+y_dir == ROWS || c+x_dir == COLS || 
            r+y_dir < 0 || c+x_dir < 0)
            return;
    }


    pb->playboard[pb->player_row][pb->player_col] = SPACE;
    pb->player_row += y_dir;
    pb->player_col += x_dir;

    if (type1 == HOLE) {
        draw_board(pb);
        rb->splash(HZ*2, "You fell down a hole!");
        load_level(pb);
    }
    else if (type1 == SPACE)
        pb->playboard[r][c] = PLAYER;
    else {
        pb->playboard[r][c] = PLAYER;
        if(type1 == type2) {
            if (type1 == TRIANGLE)
                pb->playboard[r+y_dir][c+x_dir] = WALL;
            else if (type1 == CROSS) {
                pb->playboard[r][c] = SPACE;
                new_piece(pb, r+y_dir, c+x_dir);
                pb->playboard[r][c] = PLAYER;
            }
            else
                pb->playboard[r+y_dir][c+x_dir] = SPACE;
        }
        else if (type2 == SPACE)
            pb->playboard[r+y_dir][c+x_dir] = type1;
        else if (type2 == HOLE) {
            if (type1 == SQUARE)
                pb->playboard[r+y_dir][c+x_dir] = SPACE;
        }
        else {
            rb->splash(HZ*2, "Illegal Move!");
            load_level(pb);
        }
    }

    draw_board(pb);
}

/*****************************************************************************
* pegbox_loadgame() loads the saved game and returns load success.
******************************************************************************/
static bool pegbox_loadgame(struct game_context* pb) {
    signed int fd;
    bool loaded = false;

    /* open game file */
    fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0) return loaded;

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &pb->level, sizeof(pb->level)) <= 0) break;
        if(rb->read(fd, &pb->playboard, sizeof(pb->playboard)) <= 0)
        {
            loaded = true;
            break;
        }
        break;
    }

    rb->close(fd);
    return loaded;
}

/*****************************************************************************
* pegbox_savegame() saves the current game state.
******************************************************************************/
static void pegbox_savegame(struct game_context* pb) {
    unsigned int fd;

    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &pb->level, sizeof(pb->level));
    rb->write(fd, &pb->playboard, sizeof(pb->playboard));
    rb->close(fd);
}

/*****************************************************************************
* pegbox_loaddata() loads the level and highlevel and returns load success.
******************************************************************************/
static void pegbox_loaddata(struct game_context* pb) {
    signed int fd;

    /* open game file */
    fd = rb->open(DATA_FILE, O_RDONLY);
    if(fd < 0) {
        pb->level = 1;
        pb->highlevel = 1;
        return;
    }

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &pb->level, sizeof(pb->level)) <= 0) break;
        if(rb->read(fd, &pb->highlevel, sizeof(pb->highlevel)) <= 0) break;
        break;
    }

    rb->close(fd);
    return;
}

/*****************************************************************************
* pegbox_savedata() saves the level and highlevel.
******************************************************************************/
static void pegbox_savedata(struct game_context* pb) {
    unsigned int fd;

    /* write out the game state to the save file */
    fd = rb->open(DATA_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &pb->level, sizeof(pb->level));
    rb->write(fd, &pb->highlevel, sizeof(pb->highlevel));
    rb->close(fd);
}

/*****************************************************************************
* pegbox_callback() is the default event handler callback which is called
*     on usb connect and shutdown.
******************************************************************************/
static void pegbox_callback(void* param) {
    struct game_context* pb = (struct game_context*) param;
    rb->splash(HZ, "Saving data...");
    pegbox_savedata(pb);
}

/*****************************************************************************
* pegbox_menu() is the initial menu at the start of the game.
******************************************************************************/
static unsigned int pegbox_menu(struct game_context* pb) {
    int button;
    char str[30];
    unsigned int startlevel = 1, loc = 0;
    bool breakout = false, can_resume = false;
    
    if (pb->num_left > 0 || pb->save_exist)
        can_resume = true;
        
    while(!breakout){
#if (LCD_WIDTH >= 138) && (LCD_HEIGHT >= 110)
        rb->lcd_clear_display();
        rb->lcd_bitmap(pegbox_menu_top,0,0,LCD_WIDTH, BMPHEIGHT_pegbox_menu_top);

         /* menu bitmaps */
        if (loc == 0) {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                (BMPHEIGHT_pegbox_menu_items/9), 
                                BMPWIDTH_pegbox_menu_items, 
                                (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                BMPHEIGHT_pegbox_menu_top, 
                                BMPWIDTH_pegbox_menu_items, 
                                (BMPHEIGHT_pegbox_menu_items/9));
        }
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 0, 
                                BMPWIDTH_pegbox_menu_items, 
                                (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                BMPHEIGHT_pegbox_menu_top, 
                                BMPWIDTH_pegbox_menu_items, 
                                (BMPHEIGHT_pegbox_menu_items/9));
        }
        if (can_resume) {
            if (loc == 1) {
                rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                    (BMPHEIGHT_pegbox_menu_items/9)*3, 
                                    BMPWIDTH_pegbox_menu_items, 
                                    (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                    BMPHEIGHT_pegbox_menu_top+
                                    (BMPHEIGHT_pegbox_menu_items/9), 
                                    BMPWIDTH_pegbox_menu_items, 
                                    (BMPHEIGHT_pegbox_menu_items/9));
            }
            else {
                rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                    (BMPHEIGHT_pegbox_menu_items/9)*2, 
                                    BMPWIDTH_pegbox_menu_items, 
                                    (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                    BMPHEIGHT_pegbox_menu_top+
                                    (BMPHEIGHT_pegbox_menu_items/9), 
                                    BMPWIDTH_pegbox_menu_items, 
                                    (BMPHEIGHT_pegbox_menu_items/9));
            }
        } 
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                (BMPHEIGHT_pegbox_menu_items/9)*4, 
                                BMPWIDTH_pegbox_menu_items, 
                                (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                BMPHEIGHT_pegbox_menu_top+
                                (BMPHEIGHT_pegbox_menu_items/9), 
                                BMPWIDTH_pegbox_menu_items, 
                                (BMPHEIGHT_pegbox_menu_items/9));
        }

        if (loc==2) {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                (BMPHEIGHT_pegbox_menu_items/9)*6, 
                                BMPWIDTH_pegbox_menu_items, 
                                (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                BMPHEIGHT_pegbox_menu_top+
                                (BMPHEIGHT_pegbox_menu_items/9)*2, 
                                BMPWIDTH_pegbox_menu_items, 
                                (BMPHEIGHT_pegbox_menu_items/9));
        }
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                (BMPHEIGHT_pegbox_menu_items/9)*5, 
                                BMPWIDTH_pegbox_menu_items, 
                                (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                BMPHEIGHT_pegbox_menu_top+
                                (BMPHEIGHT_pegbox_menu_items/9)*2, 
                                BMPWIDTH_pegbox_menu_items, 
                                (BMPHEIGHT_pegbox_menu_items/9));
        }

        if (loc==3) {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                (BMPHEIGHT_pegbox_menu_items/9)*8, 
                                BMPWIDTH_pegbox_menu_items, 
                                (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                BMPHEIGHT_pegbox_menu_top+
                                (BMPHEIGHT_pegbox_menu_items/9)*3, 
                                BMPWIDTH_pegbox_menu_items, 
                                (BMPHEIGHT_pegbox_menu_items/9));
        }
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 
                                (BMPHEIGHT_pegbox_menu_items/9)*7, 
                                BMPWIDTH_pegbox_menu_items, 
                                (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2, 
                                BMPHEIGHT_pegbox_menu_top+
                                (BMPHEIGHT_pegbox_menu_items/9)*3, 
                                BMPWIDTH_pegbox_menu_items, 
                                (BMPHEIGHT_pegbox_menu_items/9));
        }
#else
        unsigned int w,h;
        rb->lcd_clear_display();
        rb->lcd_getstringsize("PegBox", &w, &h);
        rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, "PegBox");
        rb->lcd_putsxy((LCD_WIDTH)/4, 16, "New Game");
        rb->lcd_putsxy((LCD_WIDTH)/4, 24, "Resume");
        rb->lcd_putsxy((LCD_WIDTH)/4, 32, "Help");
        rb->lcd_putsxy((LCD_WIDTH)/4, 40, "Quit");
        
        if(!can_resume)
            rb->lcd_drawline((LCD_WIDTH)/4, 28, (LCD_WIDTH)/4+30, 28);
        
        rb->lcd_putsxy((LCD_WIDTH)/4-8, loc*8+16, "*");
        
        
#endif
        rb->snprintf(str, 28, "Start on level %d of %d", startlevel, 
                     pb->highlevel);
#if (LCD_WIDTH >= 138) && (LCD_HEIGHT >= 110)
        rb->lcd_putsxy(0, BMPHEIGHT_pegbox_menu_top+4*
                         (BMPHEIGHT_pegbox_menu_items/9)+8, str);
#elif LCD_WIDTH > 112
        rb->lcd_putsxy(0, LCD_HEIGHT - 8, str);
#else
        rb->lcd_puts_scroll(0, 7, str);
#endif
        rb->lcd_update();
        
        /* handle menu button presses */
        button = rb->button_get(true);

        switch(button) {
            case PEGBOX_SELECT: /* start playing */
                if (loc == 0) {
                    breakout = true;
                    pb->level = startlevel;
                    load_level(pb);
                }
                else if (loc == 1 && can_resume) {
                    if(pb->save_exist)
                    {
                        rb->remove(SAVE_FILE);
                        pb->save_exist = false;
                    }
                    breakout = true;
                } 
                else if (loc == 2)
                    display_text("How to Play\nTo beat each level, you must "
                                 "destroy all of the pegs. If two like pegs are "
                                 "pushed into each other they disappear except "
                                 "for triangles which form a solid block and "
                                 "crosses which allow you to choose a "
                                 "replacement block.\n\n"
                                 "Controls\n"
#if LCD_HEIGHT > 64
                                 RESTART_TEXT " to restart level\n"
                                 LVL_UP_TEXT " to go up a level\n"
                                 LVL_DOWN_TEXT " to go down a level\n"
                                 SAVE_TEXT " to save\n"
                                 QUIT_TEXT " to quit\n",true);
#else
                                 RESTART_TEXT ": restart\n"
                                 LVL_UP_TEXT ": level up\n"
                                 LVL_DOWN_TEXT " level down\n"
                                 SAVE_TEXT " save game\n"
                                 QUIT_TEXT " quit\n",true);
#endif
                else if (loc == 3)
                    return PB_QUIT;
                break;

            case PEGBOX_QUIT:  /* quit program */
                return PB_QUIT;
                
            case (PEGBOX_UP|BUTTON_REPEAT):
            case PEGBOX_UP:
                if (loc <= 0)
                    loc = 3;
                else
                    loc--;
                if (!can_resume && loc == 1) {
                    loc = 0;
                }
                break;


            case (PEGBOX_DOWN|BUTTON_REPEAT):
            case PEGBOX_DOWN:
                if (loc >= 3)
                    loc = 0;
                else
                    loc++;
                if (!can_resume && loc == 1) {
                    loc = 2;
                }
                break;

            case (PEGBOX_RIGHT|BUTTON_REPEAT):
            case PEGBOX_RIGHT:     /* increase starting level */
                if(startlevel >= pb->highlevel) {
                    startlevel = 1;
                } else {
                    startlevel++;
                }
                break;

            case (PEGBOX_LEFT|BUTTON_REPEAT):
            case PEGBOX_LEFT:   /* decrease starting level */
                if(startlevel <= 1) {
                    startlevel = pb->highlevel;
                } else {
                    startlevel--;
                }
                break;

            default:
                if(rb->default_event_handler_ex(button, pegbox_callback,
                   (void*) pb) == SYS_USB_CONNECTED)
                    return PB_USB;
                break;
        }

    }
    draw_board(pb);

    return 0;
}

/*****************************************************************************
* pegbox() is the main game subroutine, it returns the final game status.
******************************************************************************/
static int pegbox(struct game_context* pb) {
    int temp_var;
  
    /********************
    *       menu        *
    ********************/
    temp_var = pegbox_menu(pb);
    if (temp_var == PB_QUIT || temp_var == PB_USB)
        return temp_var;

    while (true) {
        temp_var = rb->button_get(true);
        switch(temp_var){
            case PEGBOX_LEFT:             /* move cursor left */
            case (PEGBOX_LEFT|BUTTON_REPEAT):
                move_player(pb, -1, 0);
                break;

            case PEGBOX_RIGHT:            /* move cursor right */
            case (PEGBOX_RIGHT|BUTTON_REPEAT):
                move_player(pb, 1, 0);
                break;

            case PEGBOX_DOWN:             /* move cursor down */
            case (PEGBOX_DOWN|BUTTON_REPEAT):
                move_player(pb, 0, 1);
                break;

            case PEGBOX_UP:               /* move cursor up */
            case (PEGBOX_UP|BUTTON_REPEAT):
                move_player(pb, 0, -1);
                break;

            case PEGBOX_SAVE:           /* save and end game */
                rb->splash(HZ, "Saving game...");
                pegbox_savegame(pb);
                /* fall through to PEGBOX_QUIT */

            case PEGBOX_QUIT:
                return PB_END;

            case PEGBOX_RESTART:
                load_level(pb);
                draw_board(pb);
                break;

#ifdef PEGBOX_LVL_UP
            case PEGBOX_LVL_UP:
            case (PEGBOX_LVL_UP|BUTTON_REPEAT):
                if(pb->level < pb->highlevel) {
                    pb->level++;
                    load_level(pb);
                    draw_board(pb);
                }
                break;
#endif

#ifdef PEGBOX_LVL_DOWN
            case PEGBOX_LVL_DOWN:
            case (PEGBOX_LVL_DOWN|BUTTON_REPEAT):
                if(pb->level > 1) {
                    pb->level--;
                    load_level(pb);
                    draw_board(pb);
                }
                break;
#endif

        }

        if(pb->num_left == 0) {
            rb->splash(HZ*2, "Nice Pegging!");
            if(pb->level == NUM_LEVELS) {
                draw_board(pb);
                rb->splash(HZ*2, "You Won!");
                break;
            }
            else {
                pb->level++;
                load_level(pb);
                draw_board(pb);
            }

            if(pb->level > pb->highlevel)
                pb->highlevel = pb->level;

        }
    }

    return PLUGIN_OK;
}


/*****************************************************************************
* plugin entry point.
******************************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    bool exit = false;
    struct game_context pb;

    (void)parameter;
    rb = api;

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif 
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(BG_COLOR);
#endif

    rb->splash(0, "Loading...");
    pegbox_loaddata(&pb);
    pb.save_exist = pegbox_loadgame(&pb);
    pb.num_left = 0;
   
    rb->lcd_clear_display();


    while(!exit) {
        switch(pegbox(&pb)){
            case PB_END:
                break;

            case PB_USB:
                rb->lcd_setfont(FONT_UI);
                return PLUGIN_USB_CONNECTED;

            case PB_QUIT:
                rb->splash(HZ, "Saving data...");
                pegbox_savedata(&pb);
                exit = true;
                break;

            default:
                break;
        }
    }

    rb->lcd_setfont(FONT_UI);
    return PLUGIN_OK;
}
