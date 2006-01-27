/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Ben Basha (Paprica)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "configfile.h" /* Part of libplugin */

PLUGIN_HEADER

/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 30

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

/* H100 and H300 don't have scroll events */
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)

#define QUIT BUTTON_MENU
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_SCROLL_BACK
#define DOWN BUTTON_SCROLL_FWD

#define SCROLL_FWD(x) ((x) & BUTTON_SCROLL_FWD)
#define SCROLL_BACK(x) ((x) & BUTTON_SCROLL_BACK)

#else
#error Unsupported keypad
#endif

static struct plugin_api* rb;

/* External bitmaps */
extern const fb_data brickmania_ball[];
extern const fb_data brickmania_gameover[];
extern const fb_data brickmania_help[];
extern const fb_data brickmania_menu_bg[];
extern const fb_data brickmania_no_resume[];
extern const fb_data brickmania_quit[];
extern const fb_data brickmania_resume[];
extern const fb_data brickmania_sel_help[];
extern const fb_data brickmania_sel_quit[];
extern const fb_data brickmania_sel_resume[];
extern const fb_data brickmania_sel_start[];
extern const fb_data brickmania_start[];

/* normal, glue, fire */
extern const fb_data brickmania_pads[];

/* +life, -life, glue, fire, normal */
extern const fb_data brickmania_powerups[];

/* purple, red, blue, pink, green, yellow orange */
extern const fb_data brickmania_bricks[];

/* TO DO: This needs adjusting correctly for larger than 220x176 LCDS */
#if (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)

/* Offsets for LCDS > 220x176 */
#define XOFS ((LCD_WIDTH-220)/2)
#define YOFS ((LCD_HEIGHT-176)/2)

#define PAD_WIDTH 40
#define PAD_HEIGHT 5
#define PAD_POS_Y LCD_HEIGHT - 7
#define BRICK_HEIGHT 8
#define BRICK_WIDTH 20
#define BALL 5

#define BMPHEIGHT_help 19
#define BMPWIDTH_help 37

#define BMPHEIGHT_sel_help 19
#define BMPWIDTH_sel_help 37

#define BMPHEIGHT_resume 17
#define BMPWIDTH_resume 96

#define BMPHEIGHT_no_resume 17
#define BMPWIDTH_no_resume 96

#define BMPHEIGHT_quit 19
#define BMPWIDTH_quit 33

#define BMPHEIGHT_sel_quit 19
#define BMPWIDTH_sel_quit 33

#define BMPHEIGHT_sel_resume 17
#define BMPWIDTH_sel_resume 96

#define BMPHEIGHT_sel_start 20
#define BMPWIDTH_sel_start 112

#define BMPHEIGHT_start 20
#define BMPWIDTH_start 112

#define BMPHEIGHT_powerup 6
#define BMPWIDTH_powerup 10

#define BMPXOFS_resume (62+XOFS)
#define BMPYOFS_resume (100+YOFS)
#define BMPXOFS_quit (93+XOFS)
#define BMPYOFS_quit (138+YOFS)
#define BMPXOFS_start (55+XOFS)
#define BMPYOFS_start (78+YOFS)
#define BMPXOFS_help (92+XOFS)
#define BMPYOFS_help (118+YOFS)
#define HIGHSCORE_XPOS (7+XOFS)
#define HIGHSCORE_YPOS (56+YOFS)


#else
#error Unsupported LCD Size
#endif

int levels_num = 29;
    
static unsigned char levels[29][8][10] = {
    { /* level1 */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x2,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x2},
        {0x0,0x2,0x1,0x0,0x0,0x0,0x0,0x1,0x2,0x0},
        {0x0,0x0,0x2,0x1,0x0,0x0,0x1,0x2,0x0,0x0},
        {0x0,0x0,0x0,0x2,0x1,0x1,0x2,0x0,0x0,0x0},
        {0x7,0x0,0x0,0x7,0x2,0x2,0x7,0x0,0x0,0x7},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level2 */
        {0x0,0x0,0x7,0x7,0x1,0x1,0x7,0x7,0x0,0x0},
        {0x0,0x1,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x2,0x1,0x0,0x0,0x1,0x2,0x1,0x1},
        {0x1,0x2,0x0,0x2,0x2,0x2,0x2,0x0,0x2,0x1},
        {0x0,0x1,0x2,0x0,0x0,0x0,0x0,0x2,0x1,0x0},
        {0x0,0x0,0x1,0x2,0x2,0x2,0x2,0x1,0x0,0x0}
    },
    { /* level3 */
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x3,0x23,0x23,0x3,0x0,0x0,0x2,0x22,0x22,0x2},
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6},
        {0x5,0x25,0x25,0x5,0x0,0x0,0x6,0x26,0x26,0x6},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6}
    },
    { /* level4 */
        {0x0,0x0,0x0,0x27,0x27,0x27,0x27,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x27,0x7,0x7,0x27,0x0,0x0,0x0},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x26,0x6,0x0,0x2,0x2,0x2,0x2,0x0,0x6,0x26},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1}
    },
    { /* level5 */
        {0x1,0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4},
        {0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0},
        {0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5},
        {0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5,0x5},
        {0x0,0x33,0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0},
        {0x3,0x33,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x36},
        {0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x6,0x36},
        {0x0,0x24,0x24,0x0,0x25,0x25,0x0,0x26,0x26,0x0}
    },
    { /* level6 */
        {0x0,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x0},
        {0x3,0x1,0x3,0x7,0x0,0x0,0x7,0x3,0x1,0x3},
        {0x3,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x3},
        {0x0,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5},
        {0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x5},
        {0x0,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level7 */
        {0x0,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x6},
        {0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x0,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level8 */
        {0x0,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x0},
        {0x0,0x0,0x0,0x4,0x0,0x0,0x4,0x0,0x0,0x0},
        {0x6,0x6,0x0,0x2,0x32,0x32,0x2,0x0,0x6,0x6},
        {0x0,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x0},
        {0x0,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x0},
        {0x0,0x0,0x0,0x5,0x25,0x25,0x5,0x0,0x0,0x0},
        {0x0,0x5,0x5,0x25,0x5,0x5,0x25,0x5,0x5,0x0},
        {0x5,0x5,0x25,0x5,0x5,0x5,0x5,0x25,0x5,0x5}
    },
    { /* level9 */
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x3,0x2},
        {0x2,0x0,0x3,0x0,0x1,0x1,0x0,0x3,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x0,0x1,0x0,0x3,0x3,0x0,0x1,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x2,0x0,0x0,0x1,0x1,0x0,0x0,0x2,0x2},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    { /* level10 */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x4,0x1,0x1,0x4,0x0,0x0,0x0},
        {0x0,0x0,0x3,0x4,0x1,0x1,0x4,0x3,0x0,0x0},
        {0x0,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x0},
        {0x1,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x1}
    },
    { /* level11 */
        {0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x0,0x0,0x7,0x7,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x0,0x7,0x7,0x7,0x7,0x0,0x0,0x2},
        {0x0,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x0},
        {0x0,0x2,0x0,0x1,0x0,0x0,0x1,0x0,0x2,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5}
    },
    { /* level 12 */
        {0x2,0x1,0x3,0x1,0x0,0x0,0x1,0x3,0x1,0x2},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x1,0x0,0x1,0x1,0x0,0x1,0x1,0x1},
        {0x0,0x1,0x0,0x1,0x6,0x6,0x1,0x0,0x1,0x0},
        {0x0,0x0,0x1,0x1,0x6,0x6,0x1,0x1,0x0,0x0},
        {0x1,0x1,0x1,0x7,0x0,0x0,0x7,0x1,0x1,0x1},
        {0x1,0x1,0x7,0x1,0x0,0x0,0x1,0x7,0x1,0x1},
        {0x2,0x2,0x0,0x2,0x2,0x2,0x2,0x0,0x2,0x2}
    },
    {/* levell13 */
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x2},
        {0x2,0x0,0x2,0x3,0x3,0x3,0x3,0x3,0x0,0x2},
        {0x2,0x0,0x2,0x4,0x4,0x4,0x4,0x4,0x0,0x2},
        {0x2,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    {/* level14 */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x4,0x4,0x4,0x4,0x2,0x2,0x4,0x4,0x4,0x4},
        {0x4,0x0,0x0,0x0,0x2,0x2,0x0,0x0,0x0,0x4},
        {0x4,0x0,0x0,0x2,0x3,0x3,0x2,0x0,0x0,0x4},
        {0x4,0x0,0x2,0x23,0x3,0x3,0x23,0x2,0x0,0x4},
        {0x4,0x0,0x2,0x22,0x2,0x2,0x22,0x2,0x0,0x4},
        {0x4,0x0,0x6,0x21,0x5,0x5,0x21,0x6,0x0,0x4},
        {0x4,0x6,0x1,0x1,0x5,0x5,0x1,0x1,0x6,0x4}
    },
    {/* level 15 */
        {0x4,0x4,0x4,0x4,0x4,0x3,0x3,0x3,0x3,0x3},
        {0x2,0x2,0x1,0x1,0x1,0x1,0x1,0x5,0x0,0x0},
        {0x2,0x2,0x1,0x1,0x1,0x0,0x1,0x6,0x0,0x0},
        {0x2,0x1,0x1,0x2,0x1,0x1,0x1,0x5,0x0,0x0},
        {0x2,0x1,0x2,0x2,0x2,0x1,0x1,0x6,0x0,0x0},
        {0x2,0x1,0x2,0x2,0x2,0x1,0x3,0x5,0x3,0x0},
        {0x2,0x1,0x1,0x2,0x1,0x1,0x1,0x6,0x0,0x0},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    {/* level 16 (Rockbox) by ts-x */
        {0x2,0x2,0x3,0x3,0x3,0x4,0x4,0x5,0x0,0x5},
        {0x2,0x0,0x3,0x0,0x3,0x4,0x0,0x5,0x5,0x0},
        {0x2,0x0,0x3,0x3,0x3,0x4,0x4,0x5,0x0,0x5},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x6,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x7,0x7,0x7,0x1,0x1,0x1,0x2,0x0,0x2,0x0},
        {0x7,0x0,0x7,0x1,0x0,0x1,0x0,0x2,0x0,0x0},
        {0x7,0x7,0x7,0x1,0x1,0x1,0x2,0x0,0x2,0x0}
    },
    {/* level 17 (Alien) by ts-x */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x1,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x1},
        {0x2,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x2},
        {0x1,0x0,0x1,0x2,0x2,0x2,0x2,0x1,0x0,0x1},
        {0x2,0x0,0x0,0x1,0x2,0x2,0x1,0x0,0x0,0x2},
        {0x2,0x1,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x2},
        {0x2,0x2,0x1,0x0,0x1,0x1,0x0,0x1,0x2,0x2}
    },
    {/* level 18 (Tetris) by ts-x */
        {0x0,0x2,0x0,0x0,0x3,0x4,0x0,0x2,0x2,0x0},
        {0x0,0x2,0x7,0x0,0x3,0x4,0x0,0x2,0x2,0x0},
        {0x2,0x2,0x7,0x0,0x3,0x4,0x0,0x6,0x2,0x2},
        {0x2,0x2,0x7,0x7,0x3,0x4,0x0,0x6,0x2,0x2},
        {0x2,0x1,0x7,0x7,0x3,0x4,0x4,0x6,0x5,0x5},
        {0x2,0x1,0x0,0x7,0x3,0x4,0x4,0x6,0x5,0x5},
        {0x1,0x1,0x1,0x7,0x3,0x0,0x6,0x6,0x5,0x5},
        {0x1,0x1,0x1,0x0,0x3,0x0,0x6,0x6,0x5,0x5}
    },
    { /* level 19 (Stalactites) by ts-x */
        {0x5,0x2,0x6,0x3,0x4,0x7,0x5,0x3,0x1,0x2},
        {0x5,0x2,0x6,0x3,0x4,0x7,0x5,0x3,0x1,0x2},
        {0x5,0x0,0x6,0x3,0x4,0x7,0x5,0x0,0x1,0x2},
        {0x5,0x2,0x6,0x3,0x4,0x0,0x5,0x3,0x1,0x2},
        {0x5,0x0,0x6,0x0,0x4,0x7,0x5,0x0,0x1,0x0},
        {0x5,0x0,0x0,0x3,0x4,0x0,0x0,0x0,0x1,0x2},
        {0x0,0x0,0x6,0x0,0x0,0x0,0x5,0x0,0x0,0x0},
        {0x5,0x0,0x0,0x0,0x4,0x0,0x0,0x0,0x1,0x0}
    },
    { /* level 20 (Maze) by ts-x */
        {0x1,0x1,0x21,0x1,0x1,0x1,0x1,0x1,0x1,0x21},
        {0x1,0x0,0x0,0x3,0x0,0x0,0x3,0x1,0x31,0x1},
        {0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x31,0x0,0x1},
        {0x21,0x0,0x21,0x3,0x0,0x3,0x0,0x3,0x0,0x2},
        {0x1,0x0,0x1,0x21,0x0,0x12,0x0,0x0,0x0,0x0},
        {0x31,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x3,0x0},
        {0x1,0x0,0x1,0x0,0x1,0x1,0x31,0x1,0x1,0x2},
        {0x22,0x0,0x2,0x1,0x1,0x1,0x1,0x1,0x1,0x21}
    },
    { /* level 21 (Dentist) by ts-x */
        {0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0},
        {0x2,0x2,0x0,0x6,0x0,0x6,0x0,0x6,0x2,0x2},
        {0x2,0x6,0x0,0x6,0x0,0x6,0x0,0x6,0x0,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x0,0x2},
        {0x2,0x0,0x6,0x0,0x6,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x6,0x0,0x6,0x0,0x6,0x0,0x6,0x2},
        {0x2,0x2,0x6,0x0,0x6,0x0,0x6,0x0,0x2,0x2},
        {0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0}
    },
    { /* level 22 (Spider) by ts-x */
        {0x31,0x3,0x1,0x1,0x0,0x0,0x1,0x1,0x3,0x31},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x33,0x1,0x1,0x36,0x1,0x1,0x36,0x1,0x1,0x33},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x1,0x0,0x0},
        {0x21,0x3,0x1,0x21,0x2,0x2,0x21,0x1,0x3,0x21},
        {0x0,0x0,0x0,0x1,0x21,0x1,0x1,0x0,0x0,0x0},
        {0x3,0x1,0x3,0x1,0x0,0x0,0x1,0x3,0x1,0x3}
    },
    { /* level 23 (Pool) by ts-x */
        {0x0,0x7,0x7,0x7,0x0,0x7,0x7,0x7,0x7,0x0},
        {0x0,0x0,0x5,0x0,0x2,0x0,0x0,0x0,0x2,0x0},
        {0x7,0x3,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x7},
        {0x7,0x0,0x0,0x0,0x0,0x0,0x0,0x5,0x0,0x7},
        {0x7,0x0,0x4,0x0,0x0,0x3,0x0,0x0,0x0,0x7},
        {0x7,0x0,0x0,0x6,0x0,0x0,0x0,0x0,0x4,0x7},
        {0x0,0x0,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x7,0x7,0x7,0x7,0x0,0x7,0x7,0x7,0x0}
    },
    { /* level 24 (Vorbis Fish) by ts-x */
        {0x0,0x0,0x4,0x4,0x5,0x5,0x5,0x0,0x0,0x5},
        {0x0,0x4,0x6,0x4,0x4,0x5,0x5,0x5,0x0,0x5},
        {0x5,0x6,0x0,0x6,0x4,0x4,0x4,0x5,0x5,0x5},
        {0x5,0x6,0x0,0x6,0x4,0x4,0x4,0x4,0x5,0x5},
        {0x0,0x5,0x6,0x4,0x4,0x5,0x5,0x4,0x5,0x0},
        {0x5,0x5,0x4,0x4,0x5,0x5,0x5,0x4,0x5,0x5},
        {0x5,0x4,0x4,0x4,0x5,0x5,0x4,0x4,0x5,0x5},
        {0x0,0x0,0x4,0x4,0x4,0x4,0x4,0x5,0x0,0x5}
    },
    {/* level 25 (Rainbow) by ts-x */
        {0x0,0x4,0x1,0x0,0x0,0x0,0x0,0x1,0x4,0x0},
        {0x24,0x1,0x3,0x1,0x0,0x0,0x21,0x3,0x1,0x24},
        {0x1,0x23,0x5,0x3,0x1,0x21,0x3,0x5,0x3,0x21},
        {0x3,0x5,0x6,0x5,0x3,0x3,0x5,0x6,0x5,0x3},
        {0x5,0x6,0x7,0x6,0x5,0x5,0x6,0x7,0x6,0x5},
        {0x6,0x7,0x2,0x27,0x6,0x6,0x27,0x2,0x7,0x6},
        {0x7,0x2,0x0,0x2,0x27,0x27,0x2,0x0,0x2,0x7},
        {0x32,0x0,0x0,0x0,0x2,0x2,0x0,0x0,0x0,0x32}
    },
    { /* level 26 (Bowtie) by ts-x */
        {0x5,0x1,0x5,0x1,0x0,0x0,0x1,0x5,0x1,0x5},
        {0x1,0x0,0x0,0x1,0x5,0x5,0x1,0x0,0x0,0x1},
        {0x5,0x0,0x6,0x0,0x1,0x1,0x0,0x6,0x0,0x5},
        {0x1,0x0,0x6,0x6,0x0,0x0,0x6,0x6,0x0,0x1},
        {0x1,0x0,0x6,0x6,0x0,0x0,0x6,0x6,0x0,0x1},
        {0x5,0x0,0x6,0x0,0x1,0x1,0x0,0x6,0x0,0x5},
        {0x1,0x0,0x0,0x1,0x5,0x5,0x1,0x0,0x0,0x1},
        {0x5,0x1,0x5,0x1,0x0,0x0,0x1,0x5,0x1,0x5}
    },
    { /* level 27 (Frog) by ts-x */
        {0x0,0x5,0x25,0x0,0x0,0x0,0x0,0x25,0x5,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5},
        {0x25,0x0,0x0,0x5,0x6,0x6,0x5,0x0,0x0,0x25},
        {0x5,0x0,0x3,0x0,0x6,0x6,0x0,0x3,0x0,0x5},
        {0x5,0x0,0x31,0x0,0x6,0x6,0x0,0x31,0x0,0x5},
        {0x5,0x0,0x0,0x5,0x6,0x6,0x5,0x0,0x0,0x5},
        {0x5,0x5,0x5,0x35,0x0,0x0,0x35,0x5,0x5,0x5},
        {0x0,0x25,0x5,0x0,0x4,0x4,0x0,0x5,0x25,0x0}
    },
    { /* level 28 (DigDug) by ts-x */
        {0x35,0x5,0x5,0x25,0x0,0x25,0x25,0x5,0x5,0x35},
        {0x6,0x0,0x0,0x6,0x0,0x6,0x6,0x0,0x0,0x6},
        {0x7,0x0,0x37,0x37,0x0,0x37,0x37,0x7,0x0,0x7},
        {0x7,0x0,0x7,0x0,0x0,0x0,0x7,0x7,0x7,0x7},
        {0x4,0x4,0x4,0x24,0x0,0x24,0x4,0x0,0x0,0x4},
        {0x4,0x4,0x0,0x0,0x0,0x4,0x4,0x0,0x4,0x4},
        {0x24,0x24,0x4,0x4,0x4,0x4,0x0,0x0,0x24,0x4},
        {0x1,0x1,0x1,0x1,0x1,0x1,0x21,0x21,0x1,0x1}
    },
    { /* TheEnd */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x22,0x22,0x26,0x0,0x0,0x26,0x24,0x24,0x0,0x0},
        {0x22,0x0,0x26,0x26,0x0,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x26,0x0,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x0,0x26,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x0,0x26,0x0,0x26,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x0,0x0,0x26,0x24,0x24,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    }
};

int pad_pos_x,x,y;
int ball_pos_x;
int ball_pos_y;
int bally,balltempy;
int ballx,balltempx;
int life;
int start_game,con_game;
int pad_type;
int on_the_pad=0; /* for glue pad */
int score=0,vscore=0;
bool flip_sides=false;
int cur_level=0;
int brick_on_board=0;

typedef struct cube {
    int powertop;
    int power;
    char poweruse;
    char used;
    int color;
    int hits;
} cube;
cube brick[80];



typedef struct sfire {
    int top;
    int left;
} sfire;
sfire fire[30];


int highscore;
#define MAX_POINTS 200000 /* i dont think it needs to be more */
static struct configdata config[] =
{
   {TYPE_INT, 0, MAX_POINTS, &highscore, "highscore", NULL, NULL}
};

void int_game(int new_game)
{
    int i,j;
    ballx=0;
    bally=0;
    pad_pos_x=LCD_WIDTH/2-PAD_WIDTH/2;
    ball_pos_y=PAD_POS_Y-BALL;
    ball_pos_x=pad_pos_x+(PAD_WIDTH/2)-2;
    start_game =1;
    con_game =0;
    pad_type=0;
    on_the_pad=0;
    balltempy=0;
    balltempx=0;
    flip_sides=false;
    
    if (new_game==1)
     brick_on_board=0;

     for(i=0;i<=7;i++){
       for(j=0;j<=9;j++){
            brick[i*10+j].poweruse=(levels[cur_level][i][j]==0?0:1);
            if (i*10+j<=30) fire[i*10+j].top=-8;
                if (new_game==1) {

                brick[i*10+j].power=rb->rand()%25; /* +8 make the game with less powerups */
                brick[i*10+j].hits=levels[cur_level][i][j]>=10?
                                     levels[cur_level][i][j]/16-1:0;
                brick[i*10+j].powertop=30+i*10+BRICK_HEIGHT;
                brick[i*10+j].used=(levels[cur_level][i][j]==0?0:1);
                brick[i*10+j].color=(levels[cur_level][i][j]>=10?
                                    levels[cur_level][i][j]%16:levels[cur_level][i][j])-1;
                if (levels[cur_level][i][j]!=0) brick_on_board++;
            }
       }
     } 


}


#define HIGH_SCORE "highscore.cfg"

#define MENU_LENGTH 4
int sw,i,w;

int game_menu(int when)
{
    int button,cur=0;
    char str[10];

    rb->lcd_bitmap(brickmania_menu_bg,0,0,LCD_WIDTH,LCD_HEIGHT);
    while (true) {
        for(i=0;i<MENU_LENGTH;i++) {
            if (cur==0)
                rb->lcd_bitmap(brickmania_sel_start,
                               BMPXOFS_start,BMPYOFS_start,
                               BMPWIDTH_sel_start,BMPHEIGHT_sel_start);
            else
                rb->lcd_bitmap(brickmania_start,BMPXOFS_start,BMPYOFS_start,
                               BMPWIDTH_start,BMPHEIGHT_start);

            if (when==1) {
                if (cur==1)
                    rb->lcd_bitmap(brickmania_sel_resume,
                                   BMPXOFS_resume,BMPYOFS_resume,
                                   BMPWIDTH_sel_resume,BMPHEIGHT_sel_resume);
                else
                    rb->lcd_bitmap(brickmania_resume,
                                   BMPXOFS_resume,BMPYOFS_resume,
                                   BMPWIDTH_resume,BMPHEIGHT_resume);

            } else {
                rb->lcd_bitmap(brickmania_no_resume,
                               BMPXOFS_resume,BMPYOFS_resume,
                               BMPWIDTH_no_resume,BMPHEIGHT_no_resume);
            }


            if (cur==2)
                rb->lcd_bitmap(brickmania_sel_help,BMPXOFS_help,BMPYOFS_help,
                               BMPWIDTH_sel_help,BMPHEIGHT_sel_help);
            else
                rb->lcd_bitmap(brickmania_help,BMPXOFS_help,BMPYOFS_help,
                               BMPWIDTH_help,BMPHEIGHT_help);

            if (cur==3)
                rb->lcd_bitmap(brickmania_sel_quit,BMPXOFS_quit,BMPYOFS_quit,
                               BMPWIDTH_sel_quit,BMPHEIGHT_sel_quit);
            else
                rb->lcd_bitmap(brickmania_quit,BMPXOFS_quit,BMPYOFS_quit,
                               BMPWIDTH_quit,BMPHEIGHT_quit);
        }

        /* high score */
        rb->lcd_setfont(FONT_SYSFIXED);
        rb->lcd_set_background(LCD_RGBPACK(0,0,140));
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_putsxy(HIGHSCORE_XPOS, HIGHSCORE_YPOS, "High Score");
        rb->snprintf(str, sizeof(str), "%d", highscore);
        rb->lcd_getstringsize("High Score", &sw, NULL);
        rb->lcd_getstringsize(str, &w, NULL);
        rb->lcd_putsxy(HIGHSCORE_XPOS+sw/2-w/2, HIGHSCORE_YPOS+9, str);
        rb->lcd_setfont(FONT_UI);

        rb->lcd_update();

        button = rb->button_get(true);
        switch(button){
            case UP:
                if (cur==0)
                    cur = MENU_LENGTH-1;
                else
                    cur--;
                if (when==0 && cur==1) {
                    cur = 0;
                };
                break;

            case DOWN:
                if (cur==MENU_LENGTH-1)
                    cur = 0;
                else
                    cur++;
                if (when==0 && cur==1) {
                    cur=2;
                };
                break;

            case SELECT:
                if (cur==0) {
                    score=0;
                    vscore=0;
                    return 0;
                } else if (cur==1 && when==1) {
                    return 1;
                } else if (cur==2) {
                     return 2;
                } else if (cur==3) {
                     return 3;
                }
                break;
            case QUIT:
                return 3;
                break;
        }
    }
}

int help(int when)
{
    int w,h;
    int button;

    while(true){
        rb->lcd_setfont(FONT_SYSFIXED);
        rb->lcd_set_background(LCD_BLACK);
        rb->lcd_clear_display();
        rb->lcd_set_background(LCD_BLACK);
        rb->lcd_set_foreground(LCD_WHITE);

        rb->lcd_getstringsize("BrickMania", &w, &h);
        rb->lcd_putsxy(LCD_WIDTH/2-w/2, 1, "BrickMania");

        rb->lcd_set_foreground(LCD_RGBPACK(245,0,0));
        rb->lcd_putsxy(1, 1*(h+2),"Aim");
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_putsxy(1, 2*(h+2),"destroy all the bricks by bouncing");
        rb->lcd_putsxy(1, 3*(h+2),"the ball of them using the paddle.");

        rb->lcd_set_foreground(LCD_RGBPACK(245,0,0));
        rb->lcd_putsxy(1, 5*(h+2),"Controls");
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_putsxy(1, 6*(h+2),"< & > Move the paddle");
        rb->lcd_putsxy(1, 7*(h+2),"NAVI  Releases the ball/Fire!");
        rb->lcd_putsxy(1, 8*(h+2),"STOP  Opens menu/Quit");

        rb->lcd_set_foreground(LCD_RGBPACK(245,0,0));
        rb->lcd_putsxy(1, 10*(h+2),"Specials");
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_putsxy(1, 11*(h+2),"N Normal:returns paddle to normal");
        rb->lcd_putsxy(1, 12*(h+2),"D DIE!:loses a life");
        rb->lcd_putsxy(1, 13*(h+2),"L Life:gains a life/power up");
        rb->lcd_putsxy(1, 14*(h+2),"F Fire:allows you to shoot bricks");
        rb->lcd_putsxy(1, 15*(h+2),"G Glue:ball sticks to paddle");
        rb->lcd_update();

        button=rb->button_get(true);
        switch (button) {
            case QUIT:
                rb->lcd_setfont(FONT_UI);
                switch (game_menu(when)) {
                    case 0:
                        cur_level=0;
                        life=2;
                        int_game(1);
                        break;
                    case 1:
                        con_game=1;
                        break;
                    case 2:
                        if (help(when)==1) return 1;
                        break;
                    case 3:
                        return 1;
                        break;
                }
                return NULL;
                break;
        }
    }
    return NULL;
}

int pad_check(int ballxc,int mode,int pon) 
{
    /* pon: positive(1) or negative(0) */

    if (mode==0) {
        if (pon == 0)
            return -ballxc;
        else
            return ballxc;
    } else {
        if (ballx > 0)
            return ballxc;
        else
           return ballxc*-1;
    }
}

int fire_space(void)
{
    int t;
    for(t=0;t<=30;t++)
        if (fire[t].top+7 < 0)
            return t;

    return NULL;
}

int game_loop(void){
    int j,i,k,bricky,brickx;
    char s[20];
    int sec_count=0,num_count=10;
    int end;
    
    rb->srand( *rb->current_tick );

    configfile_init(rb);
    configfile_load(HIGH_SCORE,config,1,0);

    switch(game_menu(0)){
        case 0:
            cur_level = 0;
            life = 2;
            int_game(1);
            break;
        case 1:
            con_game = 1;
            break;
        case 2:
            if (help(0) == 1) return 1;
            break;
        case 3:
            return 1;
            break;
    }

    while(true) {
        /* Convert CYCLETIME (in ms) to HZ */
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;

        if (life >= 0) {
            rb->lcd_set_background(LCD_BLACK);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_clear_display();
            rb->lcd_set_background(LCD_BLACK);
            rb->lcd_set_foreground(LCD_WHITE);
            
            if (flip_sides) {
                  if (*rb->current_tick>=sec_count){
                      sec_count=*rb->current_tick+HZ;
                      if (num_count!=0)
                           num_count--;
                      else
                           flip_sides=false;
                  }
                  rb->snprintf(s, sizeof(s), "%d", num_count);
                  rb->lcd_getstringsize(s, &sw, NULL);                  
                  rb->lcd_putsxy(LCD_WIDTH/2-2, 150, s);
            }     

            /* write life num */
            rb->snprintf(s, sizeof(s), "Life: %d", life);
            rb->lcd_putsxy(2, 2, s);

            rb->snprintf(s, sizeof(s), "Level %d", cur_level+1);
            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH-sw-2, 2, s);

            if (vscore<score) vscore++;
            rb->snprintf(s, sizeof(s), "%d", vscore);
            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 2, s);

            /* continue game */
            if (con_game== 1 && start_game!=1) {
                rb->lcd_getstringsize("Press NAVI To Continue", &sw, NULL);
                rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 150, "Press NAVI to continue");
                sec_count=*rb->current_tick+HZ;
            }

            /* draw the ball */
            rb->lcd_bitmap(brickmania_ball,ball_pos_x, ball_pos_y, BALL, BALL);

            if (brick_on_board==0) brick_on_board--;

            /* if the pad is fire */
            for(i=0;i<=30;i++) {
                if (fire[i].top+7>0) {
                    if (con_game!=1)
                        fire[i].top-=4;
                    rb->lcd_vline(fire[i].left, fire[i].top, fire[i].top+7);
                }
            }

            /* the bricks */
            for (i=0;i<=7;i++) {
                for (j=0;j<=9;j++) {
                    if (brick[i*10+j].power<6) {
                        if (brick[i*10+j].poweruse==2) {
                            if (con_game!=1)
                                brick[i*10+j].powertop+=2;
                                rb->lcd_bitmap_part(brickmania_powerups,0,
                                        BMPHEIGHT_powerup*brick[i*10+j].power,
                                        BMPWIDTH_powerup,10+j*BRICK_WIDTH+5, 
                                        brick[i*10+j].powertop, 10, 6);
                            }
                        }

                        if ((pad_pos_x<10+j*BRICK_WIDTH+5 && pad_pos_x+PAD_WIDTH>10+j*BRICK_WIDTH+5) && brick[i*10+j].powertop+6>=PAD_POS_Y && brick[i*10+j].poweruse==2) {
                        switch(brick[i*10+j].power) {
                            case 0:
                                life++;
                                score+=50;
                                break;
                            case 1:
                                life--;
                                if (life>=0) {
                                    int_game(0);
                                    rb->sleep(HZ*2);
                                }
                                break;
                            case 2:
                                score+=34;
                                pad_type=1;
                                break;
                            case 3:
                                score+=47;
                                pad_type=2;
                                on_the_pad=0;
                                break;
                            case 4:
                                score+=23;
                                pad_type=0;
                                on_the_pad=0;
                                flip_sides=false;
                                break;
                            case 5:
                                score+=23; 
                                sec_count=*rb->current_tick+HZ;
                                num_count=10;
                                flip_sides=!flip_sides;
                                break;   
                        }
                    brick[i*10+j].poweruse=1;
                }

                if (brick[i*10+j].powertop>PAD_POS_Y)
                    brick[i*10+j].poweruse=1;

                brickx=10+j*BRICK_WIDTH;
                bricky=30+i*8;
                if (pad_type==2) {
                    for (k=0;k<=30;k++) {
                        if (fire[k].top+7>0) {
                            if (brick[i*10+j].used==1 && (fire[k].left+1 >= brickx && fire[k].left+1 <= brickx+BRICK_WIDTH) && (bricky+BRICK_HEIGHT>fire[k].top)){
                                score+=13;
                                fire[k].top=-16;
                                if (brick[i*10+j].hits > 0){
                                    brick[i*10+j].hits--;
                                    score+=3;    
                                }                     
                                else {
                                    brick[i*10+j].used=0;
                                    if (brick[i*10+j].power!=10) brick[i*10+j].poweruse=2;
                                        brick_on_board--;
                                }
                            }
                        }
                    }
                }

                if (brick[i*10+j].used==1)
                    rb->lcd_bitmap_part(brickmania_bricks,0,BRICK_HEIGHT*brick[i*10+j].color,BRICK_WIDTH,10+j*BRICK_WIDTH, 30+i*8, BRICK_WIDTH, BRICK_HEIGHT);
                    if (ball_pos_y <100) {
                        if (brick[i*10+j].used==1) {
                            if ((ball_pos_x+ballx+3 >= brickx && ball_pos_x+ballx+3 <= brickx+BRICK_WIDTH) && ((bricky-4<ball_pos_y+BALL && bricky>ball_pos_y+BALL) || (bricky+4>ball_pos_y+BALL+BALL && bricky<ball_pos_y+BALL+BALL)) && (bally >0)){
                                balltempy=bricky-ball_pos_y-BALL;
                            } else if ((ball_pos_x+ballx+3 >= brickx && ball_pos_x+ballx+3 <= brickx+BRICK_WIDTH) && ((bricky+BRICK_HEIGHT+4>ball_pos_y && bricky+BRICK_HEIGHT<ball_pos_y) || (bricky+BRICK_HEIGHT-4<ball_pos_y-BALL && bricky+BRICK_HEIGHT>ball_pos_y-BALL)) && (bally <0)){
                                balltempy=-(ball_pos_y-(bricky+BRICK_HEIGHT));
                            }

                            if ((ball_pos_y+3 >= bricky && ball_pos_y+3 <= bricky+BRICK_HEIGHT) && ((brickx-4<ball_pos_x+BALL && brickx>ball_pos_x+BALL) || (brickx+4>ball_pos_x+BALL+BALL && brickx<ball_pos_x+BALL+BALL)) && (ballx >0)) {
                                balltempx=brickx-ball_pos_x-BALL;
                            } else if ((ball_pos_y+bally+3 >= bricky && ball_pos_y+bally+3 <= bricky+BRICK_HEIGHT) && ((brickx+BRICK_WIDTH+4>ball_pos_x && brickx+BRICK_WIDTH<ball_pos_x) || (brickx+BRICK_WIDTH-4<ball_pos_x-BALL && brickx+BRICK_WIDTH>ball_pos_x-BALL)) && (ballx <0)) {
                                balltempx=-(ball_pos_x-(brickx+BRICK_WIDTH));
                            }

                            if ((ball_pos_x+3 >= brickx && ball_pos_x+3 <= brickx+BRICK_WIDTH) && ((bricky+BRICK_HEIGHT==ball_pos_y) || (bricky+BRICK_HEIGHT-6<=ball_pos_y && bricky+BRICK_HEIGHT>ball_pos_y)) && (bally <0)) { /* bottom line */
                                if (brick[i*10+j].hits > 0){
                                    brick[i*10+j].hits--;
                                    score+=2;    
                                }                   
                                else {
                                    brick[i*10+j].used=0;
                                    if (brick[i*10+j].power!=10) 
                                        brick[i*10+j].poweruse=2;     
                                }
                                
                                bally = bally*-1;
                            } else if ((ball_pos_x+3 >= brickx && ball_pos_x+3 <= brickx+BRICK_WIDTH) && ((bricky==ball_pos_y+BALL) || (bricky+6>=ball_pos_y+BALL && bricky<ball_pos_y+BALL)) && (bally >0)) { /* top line */
                                if (brick[i*10+j].hits > 0){
                                    brick[i*10+j].hits--;
                                    score+=2;    
                                }                    
                                else {
                                    brick[i*10+j].used=0;
                                    if (brick[i*10+j].power!=10) 
                                        brick[i*10+j].poweruse=2;     
                                }
                                    
                                bally = bally*-1;
                            }

                            if ((ball_pos_y+3 >= bricky && ball_pos_y+3 <= bricky+BRICK_HEIGHT) && ((brickx==ball_pos_x+BALL) || (brickx+6>=ball_pos_x+BALL && brickx<ball_pos_x+BALL)) && (ballx > 0)) { /* left line */
                                if (brick[i*10+j].hits > 0){
                                    brick[i*10+j].hits--;
                                    score+=2;    
                                }                    
                                else {
                                    brick[i*10+j].used=0;
                                    if (brick[i*10+j].power!=10) 
                                        brick[i*10+j].poweruse=2;     
                                }                                   
                                ballx = ballx*-1;

                            } else if ((ball_pos_y+3 >= bricky && ball_pos_y+3 <= bricky+BRICK_HEIGHT) && ((brickx+BRICK_WIDTH==ball_pos_x) || (brickx+BRICK_WIDTH-6<=ball_pos_x && brickx+BRICK_WIDTH>ball_pos_x)) && (ballx < 0)) { /* Right line */
                                if (brick[i*10+j].hits > 0){
                                    brick[i*10+j].hits--;
                                    score+=2;    
                                }                     
                                else {
                                    brick[i*10+j].used=0;
                                    if (brick[i*10+j].power!=10) 
                                        brick[i*10+j].poweruse=2;     
                                }
                                    
                                ballx = ballx*-1;
                            }

                            if (brick[i*10+j].used==0){
                                brick_on_board--;
                                score+=8;
                            }
                        }
                    }
                }
            }

            /* draw the pad */
            rb->lcd_bitmap_part(brickmania_pads,0,pad_type*PAD_HEIGHT,PAD_WIDTH,pad_pos_x, PAD_POS_Y, PAD_WIDTH, PAD_HEIGHT);

            if ((ball_pos_x >= pad_pos_x && ball_pos_x <= pad_pos_x+PAD_WIDTH) && (PAD_POS_Y-4<ball_pos_y+BALL && PAD_POS_Y>ball_pos_y+BALL) && (bally >0))
                balltempy=PAD_POS_Y-ball_pos_y-BALL;
            else if ((4>ball_pos_y && 0<ball_pos_y) && (bally <0))
                balltempy=-ball_pos_y;
            if ((LCD_WIDTH-4<ball_pos_x+BALL && LCD_WIDTH>ball_pos_x+BALL) && (ballx >0))
                balltempx=LCD_WIDTH-ball_pos_x-BALL;
            else if ((4>ball_pos_x && 0<ball_pos_x) && (ballx <0))
                balltempx=-ball_pos_x;

            /* top line */
            if (ball_pos_y<= 0)
                bally = bally*-1;
            /* bottom line */
            else if (ball_pos_y+BALL >= LCD_HEIGHT) {
                life--;
                if (life>=0){
                    int_game(0);
                    rb->sleep(HZ*2);
                }
            }

            /* left line ,right line */
            if ((ball_pos_x <= 0) || (ball_pos_x+BALL >= LCD_WIDTH))
                ballx = ballx*-1;

            if ((ball_pos_y+5 >= PAD_POS_Y && (ball_pos_x >= pad_pos_x && ball_pos_x <= pad_pos_x+PAD_WIDTH)) &&
                        start_game != 1 && on_the_pad==0) {
                if ((ball_pos_x+3 >= pad_pos_x && ball_pos_x+3 <= pad_pos_x+5) || (ball_pos_x +2>= pad_pos_x+35 && ball_pos_x+2 <= pad_pos_x+40)) {
                    bally = 2*-1;
                    if (ball_pos_x != 0 && ball_pos_x+BALL!=LCD_WIDTH)
                        ballx = pad_check(6,0,ball_pos_x+2<=pad_pos_x+(PAD_WIDTH/2)?0:1);
                    } else if ((ball_pos_x+3 >= pad_pos_x+5 && ball_pos_x+3 <= pad_pos_x+10) || (ball_pos_x+2 >= pad_pos_x+30 && ball_pos_x+2 <= pad_pos_x+35)) {
                        bally = 3*-1;

                        if (ball_pos_x != 0 && ball_pos_x+BALL!=LCD_WIDTH) 
                            ballx = pad_check(4,0,ball_pos_x+2<=pad_pos_x+(PAD_WIDTH/2)?0:1);
                    } else if ((ball_pos_x+3 >= pad_pos_x+10 && ball_pos_x+3 <= pad_pos_x+15) || (ball_pos_x+2 >= pad_pos_x+25 && ball_pos_x+2 <= pad_pos_x+30)) {
                        bally = 4*-1;

                        if (ball_pos_x != 0 && ball_pos_x+BALL!=LCD_WIDTH)
                            ballx = pad_check(3,0,ball_pos_x+2<=pad_pos_x+(PAD_WIDTH/2)?0:1);
                    } else if ((ball_pos_x+3 >= pad_pos_x+13 && ball_pos_x+3 <= pad_pos_x+18) || (ball_pos_x+2 >= pad_pos_x+22 && ball_pos_x+2 <= pad_pos_x+25)) {
                        bally = 4*-1;
                        if (ball_pos_x != 0 && ball_pos_x+BALL!=LCD_WIDTH)
                            ballx = pad_check(2,1,NULL);
                    } else {
                        bally = 4*-1;
                    }
                }

                if (on_the_pad!=1) {
                    ball_pos_x+=balltempx!=0?balltempx:ballx;
                    ball_pos_y+=balltempy!=0?balltempy:bally;

                    balltempy=0;
                    balltempx=0;
                }

                if (ball_pos_y+5 >= PAD_POS_Y && (pad_type==1 && on_the_pad==0) &&
                        (ball_pos_x >= pad_pos_x && ball_pos_x <= pad_pos_x+PAD_WIDTH)){
                    bally=0;
                    on_the_pad=1;
                }

                rb->lcd_update();

                if (brick_on_board < 0) {
                    if (cur_level+1<levels_num) {
                        rb->sleep(HZ * 2);
                        cur_level++;
                        score+=100;
                        int_game(1);
                    } else {
                        rb->lcd_getstringsize("Congratulations!", &sw, NULL);
                        rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 140, "Congratulations!");
                        rb->lcd_getstringsize("You have finished the game!", &sw, NULL);
                        rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 157, "You have finished the game!");
                        vscore=score;
                        rb->lcd_update();
                        if (score>highscore) {
                            rb->sleep(HZ*2);
                            highscore=score;
                            rb->splash(HZ*2,true,"New High Score");
                        } else {
                            rb->sleep(HZ * 4);
                        }

                        switch(game_menu(0)){
                            case 0:
                                life=2;
                                cur_level=0;
                                int_game(1);
                                break;
                            case 1:
                                con_game=1;
                                break;
                            case 2:
                                if (help(0)==1) return 1;
                                break;
                            case 3:
                                return 1;
                                break;
                            };
                       }
                  }

                  int move_button,button;
                  int button_right,button_left;
                  button=rb->button_get(false);
                  move_button=rb->button_status();

                  button_right=((move_button & RIGHT) || (SCROLL_FWD(button)));
                  button_left=((move_button & LEFT) || (SCROLL_BACK(button)));

                  if ((button_right && flip_sides==false) || (button_left && flip_sides==true)) {
                      if (pad_pos_x+8+PAD_WIDTH > LCD_WIDTH) {
                          if (start_game==1 || on_the_pad==1) ball_pos_x+=LCD_WIDTH-pad_pos_x-PAD_WIDTH;
                              pad_pos_x+=LCD_WIDTH-pad_pos_x-PAD_WIDTH;
                      } else {
                          if ((start_game==1 || on_the_pad==1))
                              ball_pos_x+=8;
                          pad_pos_x+=8;
                      }
                  } else if ((button_left && flip_sides==false) || (button_right && flip_sides==true)) {
                      if (pad_pos_x-8 < 0) {
                          if (start_game==1 || on_the_pad==1) ball_pos_x-=pad_pos_x;
                              pad_pos_x-=pad_pos_x;
                      } else {
                          if (start_game==1 || on_the_pad==1) ball_pos_x-=8;
                              pad_pos_x-=8;
                      }
                  }


                  switch(button) {
                      case SELECT:
                          if (start_game==1 && con_game!=1 && pad_type!=1) {
                              bally=-4;
                              ballx=pad_pos_x+(PAD_WIDTH/2)-2>=LCD_WIDTH/2?2:-2;
                              start_game =0;
                          } else if (pad_type==1 && on_the_pad==1) {
                              on_the_pad=0;
                              if (start_game!=1 && con_game==1) {
                                  start_game =0;
                                  con_game=0;
                              }
                          } else if (pad_type==2 && con_game!=1) {
                              int tfire;
                              tfire=fire_space();
                              fire[tfire].top=PAD_POS_Y-7;
                              fire[tfire].left=pad_pos_x+1;
                              tfire=fire_space();
                              fire[tfire].top=PAD_POS_Y-7;
                              fire[tfire].left=pad_pos_x+PAD_WIDTH-1;
                          } else if (con_game==1 && start_game!=1) {
                              ballx=x;
                              bally=y;
                              con_game=0;
                          }
                          break;
                     case QUIT:
                         switch(game_menu(1)){
                             case 0:
                                 life=2;
                                 cur_level=0;
                                 int_game(1);
                                 break;
                             case 1:
                                 if (ballx!=0 && bally !=0)
                                     con_game=1;
                                 break;
                             case 2:
                                 if (help(1)==1) return 1;
                                 break;
                             case 3:
                                 return 1;
                                 break;
                         };
                         if (ballx!=0) x=ballx;
                         ballx=0;
                         if (bally!=0) y=bally;
                         bally=0;
                         break;
                     }
             } else {
                 rb->lcd_bitmap(brickmania_gameover,LCD_WIDTH/2-55,LCD_HEIGHT-87,110,52);
                 rb->lcd_update();
                 if (score>highscore) {
                      rb->sleep(HZ*2);
                      highscore=score;
                      rb->splash(HZ*2,true,"New High Score");
                 } else {
                   rb->sleep(HZ * 3);
                 }

                 ballx=0;
                 bally=0;

                 switch(game_menu(0)){
                     case 0:
                         cur_level=0;
                         life=2;
                         int_game(1);
                         break;
                     case 1:
                         con_game=1;
                         break;
                     case 2:
                         if (help(0)==1) return 1;
                         break;
                     case 3:
                         return 1;
                         break;
                 }
            }
        if (end > *rb->current_tick) 
            rb->sleep(end-*rb->current_tick);
    }
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;
    rb = api;
#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

    bally=0;
    ballx=0;

    /* Permanently enable the backlight (unless the user has turned it off) */
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1);

    /* now go ahead and have fun! */
    while (game_loop()!=1);

    configfile_save(HIGH_SCORE,config,1,0);

    /* Restore user's original backlight setting */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);

#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif

    return PLUGIN_OK;
}
