/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Albert Veli
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Improvised creds goes to:
 *
 * - Anders Clausen for ingeniously inventing the name Invadrox.
 * - Linus Nielsen-Feltzing for patiently answering n00b questions.
 */

#include "plugin.h"
#include "highscore.h"
#include "helper.h"

PLUGIN_HEADER

/* Original graphics is only 1bpp so it should be portable
 * to most targets. But for now, only support the simple ones.
 */
#ifndef HAVE_LCD_BITMAP
    #error INVADROX: Unsupported LCD
#endif

#if (LCD_DEPTH < 2)
    #error INVADROX: Unsupported LCD
#endif

/* #define DEBUG */
#ifdef DEBUG
#include <stdio.h>
#define DBG(format, arg...) { printf("%s: " format, __FUNCTION__, ## arg); }
#else
#define DBG(format, arg...) {}
#endif

#if CONFIG_KEYPAD == IRIVER_H100_PAD

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_ON

#elif CONFIG_KEYPAD == IRIVER_H300_PAD

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_SELECT

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_PLAY

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#define QUIT BUTTON_MENU
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_SELECT

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_SELECT

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_SELECT

#elif CONFIG_KEYPAD == SANSA_E200_PAD

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_SELECT

#elif CONFIG_KEYPAD == ELIO_TPJ1022_PAD

/* TODO: Figure out which buttons to use for Tatung Elio TPJ-1022 */
#define QUIT BUTTON_AB
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_MENU

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD

#define QUIT BUTTON_BACK
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define FIRE BUTTON_SELECT

#else
    #error INVADROX: Unsupported keypad
#endif


#ifndef UNUSED
#define UNUSED __attribute__ ((unused))
#endif

#ifndef ABS
#define ABS(x) (((x) < 0) ? (-(x)) : (x))
#endif


/* Defines common to all models */
#define UFO_Y (SCORENUM_Y + FONT_HEIGHT + ALIEN_HEIGHT)
#define PLAYFIELD_Y (LCD_HEIGHT - SHIP_HEIGHT - 2)
#define PLAYFIELD_WIDTH (LCD_WIDTH - 2 * PLAYFIELD_X)
#define LEVEL_X (LCD_WIDTH - PLAYFIELD_X - LIVES_X - LEVEL_WIDTH - 2 * NUMBERS_WIDTH - 3 * NUM_SPACING)
#define SHIP_MIN_X (PLAYFIELD_X + PLAYFIELD_WIDTH / 5 - SHIELD_WIDTH / 2 - SHIP_WIDTH)
#define SHIP_MAX_X (PLAYFIELD_X + 4 * PLAYFIELD_WIDTH / 5 + SHIELD_WIDTH / 2)
/* SCORE_Y = 0 for most targets. Gigabeat redefines it later. */
#define SCORE_Y 0
#define MAX_LIVES 8


/* iPod Video defines */
#if (LCD_WIDTH == 320) && (LCD_HEIGHT == 240)

/* Original arcade game size 224x240, 1bpp with
 * red overlay at top and green overlay at bottom.
 *
 * iPod Video: 320x240x16
 * ======================
 * X: 48p padding at left/right gives 224p playfield in middle.
 *    10p "border" gives 204p actual playfield. UFO use full 224p.
 * Y: Use full 240p.
 *
 * MAX_X = (204 - 12) / 2 - 1 = 95
 *
 * Y: Score text 7       0
 *    Space      10      7
 *    Score      7       17
 *    Space      8       24
 * 3  Ufo        7       32
 * 2  Space      Aliens start at 32 + 3 * 8 = 56
 * 0  aliens     9*8     56  -
 *    space     ~7*8     128  | 18.75 aliens space between
 *    shield     2*8     182  | first alien and ship.
 *    space      8       198  | MAX_Y = 18
 *    ship       8       206 -
 *    space      2*8     214
 *    hline      1       230 - PLAYFIELD_Y
 *    bottom border 10   240
 *    Lives and Level goes inside bottom border
 */

#define ARCADISH_GRAPHICS
#define PLAYFIELD_X 48
#define SHIP_Y (PLAYFIELD_Y - 3 * SHIP_HEIGHT)
#define ALIEN_START_Y (UFO_Y + 3 * ALIEN_HEIGHT)
#define SCORENUM_X (PLAYFIELD_X + NUMBERS_WIDTH)
#define SCORENUM_Y SCORE_Y + (2 * (FONT_HEIGHT + 1) + 1)
#define HISCORE_X (LCD_WIDTH - PLAYFIELD_X - HISCORE_WIDTH)
#define HISCORENUM_X (LCD_WIDTH - PLAYFIELD_X - 1 - 6 * NUMBERS_WIDTH - 5 * NUM_SPACING)
#define SHIELD_Y (PLAYFIELD_Y - 6 * SHIP_HEIGHT)
#define LIVES_X 10
#define MAX_X 95
#define MAX_Y 18


#elif (LCD_WIDTH == 176) && (LCD_HEIGHT == 220)

/* Sandisk Sansa e200: 176x220x16
 * ==============================
 * X: No padding. 8p border -> 160p playfield.
 *
 * 8p Aliens with 3p spacing -> 88 + 30 = 118p aliens block.
 * (160 - 118) / 2 = 21 rounds for whole block (more than original)
 * MAX_X = (160 - 8) / 2 - 1 = 75 rounds for single alien (less than original)
 *
 *    LOGO       70       0
 *    Score text 5        70
 *    Space      5        75
 * Y  Score      5        80
 *    Space      10       85
 * 2  Ufo        5        95
 * 2  Space      10      100
 * 0  aliens     9*5     110 -
 *    space     ~7*5     155  | 18.6 aliens space between
 *    shield     2*5     188  | first alien and ship.
 *    space      5       198  | MAX_Y = 18
 *    ship       5       203 -
 *    space      5       208
 *    hline      1       213 PLAYFIELD_Y
 *    bottom border 6
 *    LCD_HEIGHT         220
 *    Lives and Level goes inside bottom border
 */

#define TINY_GRAPHICS
#define PLAYFIELD_X 0
#define SHIP_Y (PLAYFIELD_Y - 2 * SHIP_HEIGHT)
#define SHIELD_Y (SHIP_Y - SHIP_HEIGHT - SHIELD_HEIGHT)
#define ALIEN_START_Y (UFO_Y + 3 * SHIP_HEIGHT)
/* Redefine SCORE_Y */
#undef SCORE_Y
#define SCORE_Y 70
#define SCORENUM_X (PLAYFIELD_X + NUMBERS_WIDTH)
#define SCORENUM_Y (SCORE_Y + 2 * FONT_HEIGHT)
#define HISCORE_X (LCD_WIDTH - PLAYFIELD_X - HISCORE_WIDTH)
#define HISCORENUM_X (LCD_WIDTH - PLAYFIELD_X - 1 - 6 * NUMBERS_WIDTH - 5 * NUM_SPACING)
#define LIVES_X 8
#define MAX_X 75
#define MAX_Y 18


#elif (LCD_WIDTH == 176) && (LCD_HEIGHT == 132)

/* iPod Nano: 176x132x16
 * ======================
 * X: No padding. 8p border -> 160p playfield.
 *
 *    LIVES_X 8
 *    ALIEN_WIDTH 8
 *    ALIEN_HEIGHT 5
 *    ALIEN_SPACING 3
 *    SHIP_WIDTH 10
 *    SHIP_HEIGHT 5
 *    FONT_HEIGHT 5
 *    UFO_WIDTH 10
 *    UFO_HEIGHT 5
 *    SHIELD_WIDTH 15
 *    SHIELD_HEIGHT 10
 *    MAX_X 75
 *    MAX_Y = 18
 *    ALIEN_START_Y (UFO_Y + 12)
 *
 * 8p Aliens with 3p spacing -> 88 + 30 = 118p aliens block.
 * (160 - 118) / 2 = 21 rounds for whole block (more than original)
 * MAX_X = (160 - 8) / 2 - 1 = 75 rounds for single alien (less than original)
 *
 * Y: Scoreline  5         0 (combine scoretext and numbers on same line)
 *    Space      5         5
 * 1  Ufo        5        10
 * 3  Space      7        15
 * 2  aliens     9*5      22 -
 *    space     ~7*5      67  | Just above 18 aliens space between
 *    shield     2*5     100  | first alien and ship.
 *    space      5       110  | MAX_Y = 18
 *    ship       5       115 -
 *    space      5       120
 *    hline      1       125 PLAYFIELD_Y
 *    bottom border 6    126
 *    LCD_HEIGHT         131
 *    Lives and Level goes inside bottom border
 */

#define TINY_GRAPHICS
#define PLAYFIELD_X 0
#define SHIP_Y (PLAYFIELD_Y - 2 * SHIP_HEIGHT)
#define ALIEN_START_Y (UFO_Y + 12)
#define SCORENUM_X (PLAYFIELD_X + SCORE_WIDTH + NUMBERS_WIDTH + NUM_SPACING)
#define SCORENUM_Y SCORE_Y
#define HISCORENUM_X (LCD_WIDTH - PLAYFIELD_X - 4 * NUMBERS_WIDTH - 3 * NUM_SPACING)
#define HISCORE_X (HISCORENUM_X - NUMBERS_WIDTH - NUM_SPACING - HISCORE_WIDTH)
#define SHIELD_Y (SHIP_Y - SHIP_HEIGHT - SHIELD_HEIGHT)
#define LIVES_X 8
#define MAX_X 75
#define MAX_Y 18

#elif (LCD_WIDTH == 160) && (LCD_HEIGHT == 128)

/* iAudio X5, iRiver H10 20Gb, iPod 3g/4g: 160x128x16
 * ======================================
 * X: No padding. No border -> 160p playfield.
 *
 *    LIVES_X 0
 *    ALIEN_WIDTH 8
 *    ALIEN_HEIGHT 5
 *    ALIEN_SPACING 3
 *    SHIP_WIDTH 10
 *    SHIP_HEIGHT 5
 *    FONT_HEIGHT 5
 *    UFO_WIDTH 10
 *    UFO_HEIGHT 5
 *    SHIELD_WIDTH 15
 *    SHIELD_HEIGHT 10
 *    MAX_X 75
 *    MAX_Y = 18
 *    ALIEN_START_Y (UFO_Y + 10)
 *
 * 8p Aliens with 3p spacing -> 88 + 30 = 118p aliens block.
 * (160 - 118) / 2 = 21 rounds for whole block (more than original)
 * MAX_X = (160 - 8) / 2 - 1 = 75 rounds for single alien (less than original)
 *
 * Y: Scoreline  5         0 (combine scoretext and numbers on same line)
 *    Space      5         5
 * 1  Ufo        5        10
 * 2  Space      5        15
 * 8  aliens     9*5      20 -
 *    space     ~6*5      65  | Just above 18 aliens space between
 *    shield     2*5      96  | first alien and ship.
 *    space      5       106  | MAX_Y = 18
 *    ship       5       111 -
 *    space      5       116
 *    hline      1       121 PLAYFIELD_Y
 *    bottom border 6    122
 *    LCD_HEIGHT         128
 *    Lives and Level goes inside bottom border
 */

#define TINY_GRAPHICS
#define PLAYFIELD_X 0
#define SHIP_Y (PLAYFIELD_Y - 2 * SHIP_HEIGHT)
#define ALIEN_START_Y (UFO_Y + 10)
#define SCORENUM_X (PLAYFIELD_X + SCORE_WIDTH + NUMBERS_WIDTH + NUM_SPACING)
#define SCORENUM_Y SCORE_Y
#define HISCORENUM_X (LCD_WIDTH - PLAYFIELD_X - 4 * NUMBERS_WIDTH - 3 * NUM_SPACING)
#define HISCORE_X (HISCORENUM_X - NUMBERS_WIDTH - NUM_SPACING - HISCORE_WIDTH)
#define SHIELD_Y (SHIP_Y - SHIP_HEIGHT - SHIELD_HEIGHT)
#define LIVES_X 0
#define MAX_X 75
#define MAX_Y 18


#elif (LCD_WIDTH == 240) && (LCD_HEIGHT == 320)

/* Gigabeat: 240x320x16
 * ======================
 * X: 8p padding at left/right gives 224p playfield in middle.
 *    10p "border" gives 204p actual playfield. UFO use full 224p.
 * Y: Use bottom 240p for playfield and top 80 pixels for logo.
 *
 * MAX_X = (204 - 12) / 2 - 1 = 95
 *
 * Y: Score text 7       0   + 80
 *    Space      10      7   + 80
 *    Score      7       17  + 80
 *    Space      8       24  + 80
 * 3  Ufo        7       32  + 80
 * 2  Space      Aliens start at 32 + 3 * 8 = 56
 * 0  aliens     9*8     56  -
 *    space     ~7*8     128  | 18.75 aliens space between
 *    shield     2*8     182  | first alien and ship.
 *    space      8       198  | MAX_Y = 18
 *    ship       8       206 -
 *    space      2*8     214
 *    hline      1       230 310  - PLAYFIELD_Y
 *    bottom border 10   240 320
 *    Lives and Level goes inside bottom border
 */

#define ARCADISH_GRAPHICS
#define PLAYFIELD_X 8
#define SHIP_Y (PLAYFIELD_Y - 3 * SHIP_HEIGHT)
#define ALIEN_START_Y (UFO_Y + 3 * ALIEN_HEIGHT)
/* Redefine SCORE_Y */
#undef SCORE_Y
#define SCORE_Y 80
#define SCORENUM_X (PLAYFIELD_X + NUMBERS_WIDTH)
#define SCORENUM_Y SCORE_Y + (2 * (FONT_HEIGHT + 1) + 1)
#define HISCORE_X (LCD_WIDTH - PLAYFIELD_X - HISCORE_WIDTH)
#define HISCORENUM_X (LCD_WIDTH - PLAYFIELD_X - 1 - 6 * NUMBERS_WIDTH - 5 * NUM_SPACING)
#define SHIELD_Y (PLAYFIELD_Y - 6 * SHIP_HEIGHT)
#define LIVES_X 10
#define MAX_X 95
#define MAX_Y 18

#elif (LCD_WIDTH == 220) && (LCD_HEIGHT == 176)

/* TPJ1022, H300, iPod Color: 220x176x16
 * ============================
 * X: 0p padding at left/right gives 220p playfield in middle.
 *    8p "border" gives 204p actual playfield. UFO use full 220p.
 * Y: Use full 176p for playfield.
 *
 * MAX_X = (204 - 12) / 2 - 1 = 95
 *
 * Y: Score text 7       0
 *    Space      8       7
 * 1  Ufo        7       15
 * 7  Space      Aliens start at 15 + 3 * 8 = 56
 * 6  aliens     9*8     25  -
 *    space     ~7*8     103  | 15.6 aliens space between
 *    shield     2*8     126  | first alien and ship.
 *    space      8       142  | MAX_Y = 15
 *    ship       8       150 -
 *    space      8       158
 *    hline      1       166 - PLAYFIELD_Y
 *    bottom border 10   176
 *    Lives and Level goes inside bottom border
 */

#define ARCADISH_GRAPHICS
#define PLAYFIELD_X 0
#define SHIP_Y (PLAYFIELD_Y - 2 * SHIP_HEIGHT)
#define ALIEN_START_Y (UFO_Y + 10)
#define SCORENUM_Y SCORE_Y
#define SCORENUM_X (PLAYFIELD_X + SCORE_WIDTH + NUMBERS_WIDTH + NUM_SPACING)
#define HISCORENUM_X (LCD_WIDTH - PLAYFIELD_X - 4 * NUMBERS_WIDTH - 3 * NUM_SPACING)
#define HISCORE_X (HISCORENUM_X - NUMBERS_WIDTH - NUM_SPACING - HISCORE_WIDTH)
#define SHIELD_Y (PLAYFIELD_Y - 5 * SHIP_HEIGHT)
#define LIVES_X 8
#define MAX_X 95
#define MAX_Y 15

#else
    #error INVADROX: Unsupported LCD type
#endif


/* Defines common to each "graphic type" */
#ifdef ARCADISH_GRAPHICS

#define STRIDE 71
#define SHIP_SRC_X 24
#define SHIP_WIDTH 16
#define SHIP_HEIGHT 8
#define SHOT_HEIGHT 5
#define ALIEN_WIDTH 12
#define ALIEN_EXPLODE_SRC_X 52
#define ALIEN_EXPLODE_SRC_Y 39
#define ALIEN_EXPLODE_WIDTH 13
#define ALIEN_EXPLODE_HEIGHT 7
#define ALIEN_HEIGHT 8
#define ALIEN_SPACING 4
#define ALIEN_SPEED 2
#define UFO_SRC_X 40
#define UFO_WIDTH 16
#define UFO_HEIGHT 7
#define UFO_EXPLODE_WIDTH 21
#define UFO_EXPLODE_HEIGHT 8
#define UFO_SPEED 1
#define FONT_HEIGHT 7
#define LEVEL_SRC_Y 24
#define LEVEL_WIDTH 37
#define SCORE_SRC_X 24
#define SCORE_SRC_Y 31
#define SCORE_WIDTH 37
#define HISCORE_WIDTH 61
#define NUM_SPACING 3
#define NUMBERS_SRC_Y 38
#define NUMBERS_WIDTH 5
#define SHIELD_SRC_X 40
#define SHIELD_SRC_Y 15
#define SHIELD_WIDTH 22
#define SHIELD_HEIGHT 16
#define FIRE_WIDTH 8
#define FIRE_HEIGHT 8
#define FIRE_SPEED 8
#define BOMB_SRC_X 62
#define BOMB_WIDTH 3
#define BOMB_HEIGHT 7
#define BOMB_SPEED 3
#define ALIENS 11
unsigned char fire_sprite[FIRE_HEIGHT] = {
    (1 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (0 << 1) | 1,
    (0 << 7) | (0 << 6) | (1 << 5) | (0 << 4) | (0 << 3) | (0 << 2) | (1 << 1) | 0,
    (0 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 0,
    (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1,
    (0 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1,
    (0 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 0,
    (0 << 7) | (0 << 6) | (1 << 5) | (0 << 4) | (0 << 3) | (1 << 2) | (0 << 1) | 0,
    (1 << 7) | (0 << 6) | (0 << 5) | (1 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | 1
};

#elif defined TINY_GRAPHICS

#define STRIDE 53
#define SHIP_SRC_X 16
#define SHIP_WIDTH 10
#define SHIP_HEIGHT 5
#define SHOT_HEIGHT 4
#define ALIEN_WIDTH 8
#define ALIEN_HEIGHT 5
#define ALIEN_EXPLODE_SRC_X 40
#define ALIEN_EXPLODE_SRC_Y 26
#define ALIEN_EXPLODE_WIDTH 10
#define ALIEN_EXPLODE_HEIGHT 5
#define ALIEN_SPACING 3
#define ALIEN_SPEED 2
#define UFO_SRC_X 26
#define UFO_WIDTH 11
#define UFO_HEIGHT 5
#define UFO_EXPLODE_WIDTH 14
#define UFO_EXPLODE_HEIGHT 5
#define UFO_SPEED 1
#define FONT_HEIGHT 5
#define LEVEL_SRC_Y 15
#define LEVEL_WIDTH 29
#define NUMBERS_WIDTH 4
#define NUM_SPACING 2
#define SCORE_SRC_X 17
#define SCORE_SRC_Y 20
#define SCORE_WIDTH 28
#define HISCORE_WIDTH 45
#define NUMBERS_SRC_Y 25
#define SHIELD_SRC_X 29
#define SHIELD_SRC_Y 10
#define SHIELD_WIDTH 15
#define SHIELD_HEIGHT 10
#define FIRE_WIDTH 6
#define FIRE_HEIGHT 6
#define FIRE_SPEED 6
#define BOMB_SRC_X 44
#define BOMB_WIDTH 3
#define BOMB_HEIGHT 5
#define BOMB_SPEED 2
#define ALIENS 11
unsigned char fire_sprite[FIRE_HEIGHT] = {
    (1 << 5) | (0 << 4) | (0 << 3) | (1 << 2) | (0 << 1) | 1,
    (0 << 5) | (1 << 4) | (0 << 3) | (0 << 2) | (0 << 1) | 0,
    (0 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 0,
    (0 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1,
    (0 << 5) | (1 << 4) | (0 << 3) | (0 << 2) | (1 << 1) | 0,
    (1 << 5) | (0 << 4) | (1 << 3) | (0 << 2) | (0 << 1) | 1
};

#else
    #error Graphic type not defined
#endif


/* Colors */
#if (LCD_DEPTH >= 8)
#define SLIME_GREEN LCD_RGBPACK(31, 254, 31)
#define UFO_RED LCD_RGBPACK(254, 31, 31)
#elif (LCD_DEPTH == 2)
#define SLIME_GREEN LCD_LIGHTGRAY
#define UFO_RED LCD_LIGHTGRAY
#else
#error LCD type not implemented yet
#endif

/* Alien states */
#define DEAD 0
#define ALIVE 1
#define BOMBER 2

/* Fire/bomb/ufo states */
#define S_IDLE 0
#define S_ACTIVE 1
#define S_SHOWSCORE 2
#define S_EXPLODE -9

/* Fire/bomb targets */
#define TARGET_TOP 0
#define TARGET_SHIELD 1
#define TARGET_SHIP 2
#define TARGET_BOTTOM 3
#define TARGET_UFO 4

#define HISCOREFILE PLUGIN_GAMES_DIR "/invadrox.high"


/* The time (in ms) for one iteration through the game loop - decrease this
 * to speed up the game - note that current_tick is (currently) only accurate
 * to 10ms.
 */
#define CYCLETIME 40


static struct plugin_api* rb;

/* Physical x is at PLAYFIELD_X + LIVES_X + x * ALIEN_SPEED
 * Physical y is at y * ALIEN_HEIGHT
 */
struct alien {
    unsigned char x;     /* x-coordinate (0 - 95) */
    unsigned char y;     /* y-coordinate (0 - 18) */
    unsigned char type;  /* 0 (Kang), 1 (Kodos), 2 (Serak) */
    unsigned char state; /* Dead, alive or bomber */
};

/* Aliens box 5 rows * ALIENS aliens in each row */
struct alien aliens[5 * ALIENS];

#define MAX_BOMBS 4
struct bomb {
    int x, y;
    unsigned char type;
    unsigned char frame; /* Current animation frame */
    unsigned char frames; /* Number of frames in animation */
    unsigned char target; /* Remember target during explosion frames */
    int state; /* 0 (IDLE) = inactive, 1 (FIRE) or negative, exploding */
};
struct bomb bombs[MAX_BOMBS];
/* Increase max_bombs at higher levels */
int max_bombs;

/* Raw framebuffer value of shield/ship green color */
fb_data screen_green, screen_white;

/* For optimization, precalculate startoffset of each scanline */
unsigned int ytab[LCD_HEIGHT];

/* external bitmaps */
extern const fb_data invadrox[];
#if (LCD_WIDTH == 320) && (LCD_HEIGHT == 240)
/* iPod Video only */
extern const fb_data invadrox_left[];
extern const fb_data invadrox_right[];
#endif
#if ((LCD_WIDTH == 240) && (LCD_HEIGHT == 320)) || ((LCD_WIDTH == 176) && (LCD_HEIGHT == 220))
/* Gigabeat F, Sansa e200 */
extern const fb_data invadrox_logo[];
#endif


int lives = 2;
int score = 0;
int scores[3] = { 30, 20, 10 };
int level = 0;
struct highscore hiscore;
bool game_over = false;
int ship_x, old_ship_x, ship_dir, ship_acc, max_ship_speed;
int ship_frame, ship_frame_counter;
bool ship_hit;
int fire, fire_target, fire_x, fire_y;
int curr_alien, aliens_paralyzed, gamespeed;
int ufo_state, ufo_x;
bool level_finished;
bool aliens_down, aliens_right, hit_left_border, hit_right_border;


/* No standard get_pixel function yet, use this hack instead */
#if (LCD_DEPTH >= 8)

inline fb_data get_pixel(int x, int y)
{
    return rb->lcd_framebuffer[ytab[y] + x];
}

#elif (LCD_DEPTH == 2)

#if (LCD_PIXELFORMAT == HORIZONTAL_PACKING)
static const unsigned char shifts[4] = {
    6, 4, 2, 0
};
/* Horizontal packing */
inline fb_data get_pixel(int x, int y)
{
    return (rb->lcd_framebuffer[ytab[y] + (x >> 2)] >> shifts[x & 3]) & 3;
}
#else
/* Vertical packing */
static const unsigned char shifts[4] = {
    0, 2, 4, 6
};
inline fb_data get_pixel(int x, int y)
{
    return (rb->lcd_framebuffer[ytab[y] + x] >> shifts[y & 3]) & 3;
}
#endif /* Horizontal/Vertical packing */

#else
    #error get_pixel: pixelformat not implemented yet
#endif


/* Draw "digits" least significant digits of num at (x,y) */
void draw_number(int x, int y, int num, int digits)
{
    int i;
    int d;

    for (i = digits - 1; i >= 0; i--) {
        d = num % 10;
        num = num / 10;
        rb->lcd_bitmap_part(invadrox, d * NUMBERS_WIDTH, NUMBERS_SRC_Y,
                            STRIDE, x + i * (NUMBERS_WIDTH + NUM_SPACING), y,
                            NUMBERS_WIDTH, FONT_HEIGHT);
    }
    /* Update lcd */
    rb->lcd_update_rect(x, y, 4 * NUMBERS_WIDTH + 3 * NUM_SPACING, FONT_HEIGHT);
}


inline void draw_score(void)
{
    draw_number(SCORENUM_X, SCORENUM_Y, score, 4);
    if (score > hiscore.score) {
        /* Draw new hiscore (same as score) */
        draw_number(HISCORENUM_X, SCORENUM_Y, score, 4);
    }
}


void draw_level(void)
{
    rb->lcd_bitmap_part(invadrox, 0, LEVEL_SRC_Y,
                        STRIDE, LEVEL_X, PLAYFIELD_Y + 2,
                        LEVEL_WIDTH, FONT_HEIGHT);
    draw_number(LEVEL_X + LEVEL_WIDTH + 2 * NUM_SPACING, PLAYFIELD_Y + 2, level, 2);
}


void draw_lives(void)
{
    int i;
    /* Lives num */
    rb->lcd_bitmap_part(invadrox, lives * NUMBERS_WIDTH, NUMBERS_SRC_Y,
                        STRIDE, PLAYFIELD_X + LIVES_X, PLAYFIELD_Y + 2,
                        NUMBERS_WIDTH, FONT_HEIGHT);

    /* Ships */
    for (i = 0; i < (lives - 1); i++) {
        rb->lcd_bitmap_part(invadrox, SHIP_SRC_X, 0, STRIDE,
                            PLAYFIELD_X + LIVES_X + SHIP_WIDTH + i * (SHIP_WIDTH + NUM_SPACING),
                            PLAYFIELD_Y + 1, SHIP_WIDTH, SHIP_HEIGHT);
    }

    /* Erase ship to the righ (if less than MAX_LIVES) */
    if (lives < MAX_LIVES) {
        rb->lcd_fillrect(PLAYFIELD_X + LIVES_X + SHIP_WIDTH + i * (SHIP_WIDTH + NUM_SPACING),
                         PLAYFIELD_Y + 1, SHIP_WIDTH, SHIP_HEIGHT);
    }
    /* Update lives (and level) part of screen */
    rb->lcd_update_rect(PLAYFIELD_X + LIVES_X, PLAYFIELD_Y + 1,
                        PLAYFIELD_WIDTH - 2 * LIVES_X, MAX(FONT_HEIGHT + 1, SHIP_HEIGHT + 1));
}


inline void draw_aliens(void)
{
    int i;

    for (i = 0; i < 5 * ALIENS; i++) {
        rb->lcd_bitmap_part(invadrox, aliens[i].x & 1 ? ALIEN_WIDTH : 0, aliens[i].type * ALIEN_HEIGHT,
                            STRIDE, PLAYFIELD_X + LIVES_X + aliens[i].x * ALIEN_SPEED,
                            ALIEN_START_Y + aliens[i].y * ALIEN_HEIGHT,
                            ALIEN_WIDTH, ALIEN_HEIGHT);
    }
}


/* Return false if there is no next alive alien (round is over) */
inline bool next_alien(void)
{
    bool ret = true;

    do {
        curr_alien++;
        if (curr_alien % ALIENS == 0) {
            /* End of this row. Move up one row. */
            curr_alien -= 2 * ALIENS;
            if (curr_alien < 0) {
                /* No more aliens in this round. */
                curr_alien = 4 * ALIENS;
                ret = false;
            }
        }
    } while (aliens[curr_alien].state == DEAD && ret);

    if (!ret) {
        /* No more alive aliens. Round finished. */
        if (hit_right_border) {
            if (hit_left_border) {
                DBG("ERROR: both left and right borders are set (%d)\n", curr_alien);
            }
            /* Move down-left next round */
            aliens_right = false;
            aliens_down = true;
            hit_right_border = false;
        } else if (hit_left_border) {
            /* Move down-right next round */
            aliens_right = true;
            aliens_down = true;
            hit_left_border = false;
        } else {
            /* Not left nor right. Set down to false. */
            aliens_down = false;
        }
    }

    return ret;
}


/* All aliens have been moved.
 * Set curr_alien to first alive.
 * Return false if no-one is left alive.
 */
bool first_alien(void)
{
    int i, y;

    for (y = 4; y >= 0; y--) {
        for (i = y * ALIENS; i < (y + 1) * ALIENS; i++) {
            if (aliens[i].state != DEAD) {
                curr_alien = i;
                return true;
            }
        }
    }

    /* All aliens dead. */
    level_finished = true;

    return false;
}


bool move_aliens(void)
{
    int x, y, old_x, old_y;

    /* Move current alien (curr_alien is pointing to a living alien) */

    old_x = aliens[curr_alien].x;
    old_y = aliens[curr_alien].y;

    if (aliens_down) {
        aliens[curr_alien].y++;
        if (aliens[curr_alien].y == MAX_Y) {
            /* Alien is at bottom. Game Over. */
            DBG("Alien %d is at bottom. Game Over.\n", curr_alien);
            game_over = true;
            return false;
        }
    }

    if (aliens_right) {
        /* Moving right */
        if (aliens[curr_alien].x < MAX_X) {
            aliens[curr_alien].x++;
        }

        /* Now, after move, check if we hit the right border. */
        if (aliens[curr_alien].x == MAX_X) {
            hit_right_border = true;
        }

    } else {
        /* Moving left */
        if (aliens[curr_alien].x > 0) {
            aliens[curr_alien].x--;
        }

        /* Now, after move, check if we hit the left border. */
        if (aliens[curr_alien].x == 0) {
            hit_left_border = true;
        }
    }

    /* Erase old position */
    x = PLAYFIELD_X + LIVES_X + old_x * ALIEN_SPEED;
    y = ALIEN_START_Y + old_y * ALIEN_HEIGHT;
    if (aliens[curr_alien].y != old_y) {
        /* Moved in y-dir. Erase whole alien. */
        rb->lcd_fillrect(x, y, ALIEN_WIDTH, ALIEN_HEIGHT);
    } else {
        if (aliens_right) {
            /* Erase left edge */
            rb->lcd_fillrect(x, y, ALIEN_SPEED, ALIEN_HEIGHT);
        } else {
            /* Erase right edge */
            x += ALIEN_WIDTH - ALIEN_SPEED;
            rb->lcd_fillrect(x, y, ALIEN_SPEED, ALIEN_HEIGHT);
        }
    }

    /* Draw alien at new pos */
    x = PLAYFIELD_X + LIVES_X + aliens[curr_alien].x * ALIEN_SPEED;
    y = ALIEN_START_Y + aliens[curr_alien].y * ALIEN_HEIGHT;
    rb->lcd_bitmap_part(invadrox,
                        aliens[curr_alien].x & 1 ? ALIEN_WIDTH : 0, aliens[curr_alien].type * ALIEN_HEIGHT,
                        STRIDE, x, y, ALIEN_WIDTH, ALIEN_HEIGHT);

    if (!next_alien()) {
        /* Round finished. Set curr_alien to first alive from bottom. */
        if (!first_alien()) {
            /* Should never happen. Taken care of in move_fire(). */
            return false;
        }
        /* TODO: Play next background sound */
    }

    return true;
}


inline void draw_ship(void)
{
    /* Erase old ship */
    if (old_ship_x < ship_x) {
        /* Move right. Erase leftmost part of ship. */
        rb->lcd_fillrect(old_ship_x, SHIP_Y, ship_x - old_ship_x, SHIP_HEIGHT);
    } else if (old_ship_x > ship_x) {
        /* Move left. Erase rightmost part of ship. */
        rb->lcd_fillrect(ship_x + SHIP_WIDTH, SHIP_Y, old_ship_x - ship_x, SHIP_HEIGHT);
    }

    /* Draw ship */
    rb->lcd_bitmap_part(invadrox, SHIP_SRC_X, ship_frame * SHIP_HEIGHT,
                        STRIDE, ship_x, SHIP_Y, SHIP_WIDTH, SHIP_HEIGHT);
    if (ship_hit) {
        /* Alternate between frame 1 and 2 during hit */
        ship_frame_counter++;
        if (ship_frame_counter > 2) {
            ship_frame_counter = 0;
            ship_frame++;
            if (ship_frame > 2) {
                ship_frame = 1;
            }
        }
    }

    /* Save ship_x for next time */
    old_ship_x = ship_x;
}


inline void fire_alpha(int xc, int yc, fb_data color)
{
    int x, y;
    unsigned char mask;

    rb->lcd_set_foreground(color);

    for (y = 0; y < FIRE_HEIGHT; y++) {
        mask = 1 << (FIRE_WIDTH - 1);
        for (x = -(FIRE_WIDTH / 2); x < (FIRE_WIDTH / 2); x++) {
            if (fire_sprite[y] & mask) {
                rb->lcd_drawpixel(xc + x, yc + y);
            }
            mask >>= 1;
        }
    }

    rb->lcd_set_foreground(LCD_BLACK);
}


void move_fire(void)
{
    bool hit_green = false;
    bool hit_white = false;
    int i, j;
    static int exploding_alien = -1;
    fb_data pix;

    if (fire == S_IDLE) {
        return;
    }

    /* Alien hit. Wait until explosion is finished. */
    if (aliens_paralyzed < 0) {
        aliens_paralyzed++;
        if (aliens_paralyzed == 0) {
            /* Erase exploding_alien */
            rb->lcd_fillrect(PLAYFIELD_X + LIVES_X + aliens[exploding_alien].x * ALIEN_SPEED,
                             ALIEN_START_Y + aliens[exploding_alien].y * ALIEN_HEIGHT,
                             ALIEN_EXPLODE_WIDTH, ALIEN_HEIGHT);
            fire = S_IDLE;
            /* Special case. We killed curr_alien. */
            if (exploding_alien == curr_alien) {
                if (!next_alien()) {
                    /* Round finished. Set curr_alien to first alive from bottom. */
                    first_alien();
                }
            }
        }
        return;
    }

    if (fire == S_ACTIVE) {

        /* Erase */
        rb->lcd_vline(fire_x, fire_y, fire_y + SHOT_HEIGHT);

        /* Check top */
        if (fire_y <= SCORENUM_Y + FONT_HEIGHT + 4) {

            /* TODO: Play explode sound */

            fire = S_EXPLODE;
            fire_target = TARGET_TOP;
            fire_alpha(fire_x, fire_y, UFO_RED);
            return;
        }

        /* Move */
        fire_y -= FIRE_SPEED;

        /* Hit UFO? */
        if (ufo_state == S_ACTIVE) {
            if ((ABS(ufo_x + UFO_WIDTH / 2 - fire_x) <= UFO_WIDTH / 2) &&
                (fire_y <= UFO_Y + UFO_HEIGHT)) {
                ufo_state = S_EXPLODE;
                fire = S_EXPLODE;
                fire_target = TARGET_UFO;
                /* Center explosion */
                ufo_x -= (UFO_EXPLODE_WIDTH - UFO_WIDTH) / 2;
                rb->lcd_bitmap_part(invadrox, UFO_SRC_X, UFO_HEIGHT,
                                    STRIDE, ufo_x, UFO_Y - 1, UFO_EXPLODE_WIDTH, UFO_EXPLODE_HEIGHT);
                return;
            }
        }

        /* Hit bomb? (check position, not pixel value) */
        for (i = 0; i < max_bombs; i++) {
            if (bombs[i].state == S_ACTIVE) {
                /* Count as hit if within BOMB_WIDTH pixels */
                if ((ABS(bombs[i].x - fire_x) < BOMB_WIDTH) &&
                    (fire_y - bombs[i].y < BOMB_HEIGHT)) {
                    /* Erase bomb */
                    rb->lcd_fillrect(bombs[i].x, bombs[i].y, BOMB_WIDTH, BOMB_HEIGHT);
                    bombs[i].state = S_IDLE;
                    /* Explode ship fire */
                    fire = S_EXPLODE;
                    fire_target = TARGET_SHIELD;
                    fire_alpha(fire_x, fire_y, LCD_WHITE);
                    return;
                }
            }
        }

        /* Check for hit*/
        for (i = FIRE_SPEED; i >= 0; i--) {
            pix = get_pixel(fire_x, fire_y + i);
            if(pix == screen_white) {
                hit_white = true;
                fire_y += i;
                break;
            }
            if(pix == screen_green) {
                hit_green = true;
                fire_y += i;
                break;
            }
        }

        if (hit_green) {
            /* Hit shield */

            /* TODO: Play explode sound */

            fire = S_EXPLODE;
            fire_target = TARGET_SHIELD;
            /* Center explosion around hit pixel */
            fire_y -= FIRE_HEIGHT / 2;
            fire_alpha(fire_x, fire_y, SLIME_GREEN);
            return;
        }

        if (hit_white) {

            /* Hit alien? */
            for (i = 0; i < 5 * ALIENS; i++) {
                if (aliens[i].state != DEAD &&
                    (ABS(fire_x - (PLAYFIELD_X + LIVES_X + aliens[i].x * ALIEN_SPEED +
                                   ALIEN_WIDTH / 2)) <= ALIEN_WIDTH / 2) &&
                    (ABS(fire_y - (ALIEN_START_Y + aliens[i].y * ALIEN_HEIGHT +
                                   ALIEN_HEIGHT / 2)) <= ALIEN_HEIGHT / 2)) {

                    /* TODO: play alien hit sound */

                    if (aliens[i].state == BOMBER) {
                        /* Set (possible) alien above to bomber */
                        for (j = i - ALIENS; j >= 0; j -= ALIENS) {
                            if (aliens[j].state != DEAD) {
                                /* printf("New bomber (%d, %d)\n", j % ALIENS, j / ALIENS); */
                                aliens[j].state = BOMBER;
                                break;
                            }
                        }
                    }
                    aliens[i].state = DEAD;
                    exploding_alien = i;
                    score += scores[aliens[i].type];
                    draw_score();
                    /* Update score part of screen */
                    rb->lcd_update_rect(SCORENUM_X, SCORENUM_Y,
                                        PLAYFIELD_WIDTH - 2 * NUMBERS_WIDTH, FONT_HEIGHT);

                    /* Paralyze aliens S_EXPLODE frames */
                    aliens_paralyzed = S_EXPLODE;
                    rb->lcd_bitmap_part(invadrox, ALIEN_EXPLODE_SRC_X, ALIEN_EXPLODE_SRC_Y,
                                        STRIDE, PLAYFIELD_X + LIVES_X + aliens[i].x * ALIEN_SPEED,
                                        ALIEN_START_Y + aliens[i].y * ALIEN_HEIGHT,
                                        ALIEN_EXPLODE_WIDTH, ALIEN_EXPLODE_HEIGHT);
                    /* Since alien is 1 pixel taller than explosion sprite, erase bottom line */
                    rb->lcd_hline(PLAYFIELD_X + LIVES_X + aliens[i].x * ALIEN_SPEED,
                                  PLAYFIELD_X + LIVES_X + aliens[i].x * ALIEN_SPEED + ALIEN_WIDTH,
                                  ALIEN_START_Y + (aliens[i].y + 1) * ALIEN_HEIGHT - 1);
                    return;
                }
            }
        }

        /* Draw shot */
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_vline(fire_x, fire_y, fire_y + SHOT_HEIGHT);
        rb->lcd_set_foreground(LCD_BLACK);
    } else if (fire < S_IDLE) {
        /* Count up towards S_IDLE, then erase explosion */
        fire++;
        if (fire == S_IDLE) {
            /* Erase explosion */
            if (fire_target == TARGET_TOP) {
                rb->lcd_fillrect(fire_x - (FIRE_WIDTH / 2), fire_y, FIRE_WIDTH, FIRE_HEIGHT);
            } else if (fire_target == TARGET_SHIELD) {
                /* Draw explosion with black pixels */
                fire_alpha(fire_x, fire_y, LCD_BLACK);
            }
        }
    }
}


/* Return a BOMBER alien */
inline int random_bomber(void)
{
    int i, col;

    /* TODO: Weigh higher probability near ship */
    col = rb->rand() % ALIENS;
    for (i = col + 4 * ALIENS; i >= 0; i -= ALIENS) {
        if (aliens[i].state == BOMBER) {
            return i;
        }
    }

    /* No BOMBER found in this col */

    for (i = 0; i < 5 * ALIENS; i++) {
        if (aliens[i].state == BOMBER) {
            return i;
        }
    }

    /* No BOMBER found at all (error?) */

    return -1;
}


inline void draw_bomb(int i)
{
    rb->lcd_bitmap_part(invadrox, BOMB_SRC_X + bombs[i].type * BOMB_WIDTH,
                        bombs[i].frame * (BOMB_HEIGHT + 1),
                        STRIDE, bombs[i].x, bombs[i].y,
                        BOMB_WIDTH, BOMB_HEIGHT);
    /* Advance frame */
    bombs[i].frame++;
    if (bombs[i].frame == bombs[i].frames) {
        bombs[i].frame = 0;
    }
}


void move_bombs(void)
{
    int i, j, bomber;
    bool abort;

    for (i = 0; i < max_bombs; i++) {

        switch (bombs[i].state) {

        case S_IDLE:
            if (ship_hit) {
                continue;
            }
            bomber = random_bomber();
            if (bomber < 0) {
                DBG("ERROR: No bomber available\n");
                continue;
            }
            /* x, y */
            bombs[i].x = PLAYFIELD_X + LIVES_X + aliens[bomber].x * ALIEN_SPEED + ALIEN_WIDTH / 2;
            bombs[i].y = ALIEN_START_Y + (aliens[bomber].y + 1) * ALIEN_HEIGHT;

            /* Check for duplets in x and y direction */
            abort = false;
            for (j = i - 1; j >= 0; j--) {
                if ((bombs[j].state == S_ACTIVE) &&
                    ((bombs[i].x == bombs[j].x) || (bombs[i].y == bombs[j].y))) {
                    abort = true;
                    break;
                }
            }
            if (abort) {
                /* Skip this one, continue with next bomb */
                /* printf("Bomb %d duplet of %d\n", i, j); */
                continue;
            }

            /* Passed, set type */
            bombs[i].type = rb->rand() % 3;
            bombs[i].frame = 0;
            if (bombs[i].type == 0) {
                bombs[i].frames = 3;
            } else if (bombs[i].type == 1) {
                bombs[i].frames = 4;
            } else {
                bombs[i].frames = 6;
            }

            /* Bombs away */
            bombs[i].state = S_ACTIVE;
            draw_bomb(i);
            continue;

            break;

        case S_ACTIVE:
            /* Erase old position */
            rb->lcd_fillrect(bombs[i].x, bombs[i].y, BOMB_WIDTH, BOMB_HEIGHT);

            /* Move */
            bombs[i].y += BOMB_SPEED;

            /* Check if bottom hit */
            if (bombs[i].y + BOMB_HEIGHT >= PLAYFIELD_Y) {
                bombs[i].y = PLAYFIELD_Y - FIRE_HEIGHT + 1;
                fire_alpha(bombs[i].x, bombs[i].y, LCD_WHITE);
                bombs[i].state = S_EXPLODE;
                bombs[i].target = TARGET_BOTTOM;
                break;
            }

            /* Check for green (ship or shield) */
            for (j = BOMB_HEIGHT; j >= BOMB_HEIGHT - BOMB_SPEED; j--) {
                bombs[i].target = 0;
                if(get_pixel(bombs[i].x + BOMB_WIDTH / 2, bombs[i].y + j) == screen_green) {
                    /* Move to hit pixel */
                    bombs[i].x += BOMB_WIDTH / 2;
                    bombs[i].y += j;

                    /* Check if ship is hit */
                    if (bombs[i].y > SHIELD_Y + SHIELD_HEIGHT && bombs[i].y < PLAYFIELD_Y) {

                        /* TODO: play ship hit sound */

                        ship_hit = true;
                        ship_frame = 1;
                        ship_frame_counter = 0;
                        bombs[i].state = S_EXPLODE * 4;
                        bombs[i].target = TARGET_SHIP;
                        rb->lcd_bitmap_part(invadrox, SHIP_SRC_X, 1 * SHIP_HEIGHT, STRIDE,
                                            ship_x, SHIP_Y, SHIP_WIDTH, SHIP_HEIGHT);
                        break;
                    }
                    /* Shield hit */
                    bombs[i].state = S_EXPLODE;
                    bombs[i].target = TARGET_SHIELD;
                    /* Center explosion around hit pixel in shield */
                    bombs[i].y -= FIRE_HEIGHT / 2;
                    fire_alpha(bombs[i].x, bombs[i].y, SLIME_GREEN);
                    break;
                }
            }

            if (bombs[i].target != 0) {
                /* Hit ship or shield, continue */
                continue;
            }

            draw_bomb(i);
            break;

        default:
            /* If we get here state should be < 0, exploding */
            bombs[i].state++;
            if (bombs[i].state == S_IDLE) {
                if (ship_hit) {
                    /* Erase explosion */
                    rb->lcd_fillrect(ship_x, SHIP_Y, SHIP_WIDTH, SHIP_HEIGHT);
                    rb->lcd_update_rect(ship_x, SHIP_Y, SHIP_WIDTH, SHIP_HEIGHT);
                    ship_hit = false;
                    ship_frame = 0;
                    ship_x = PLAYFIELD_X + 2 * LIVES_X;
                    lives--;
                    if (lives == 0) {
                        game_over = true;
                        return;
                    }
                    draw_lives();
                    /* Sleep 1s to give player time to examine lives left */
                    rb->sleep(HZ);
                }
                /* Erase explosion (even if ship hit, might be another bomb) */
                fire_alpha(bombs[i].x, bombs[i].y, LCD_BLACK);
            }
            break;
        }
    }
}


inline void move_ship(void)
{
    ship_dir += ship_acc;
    if (ship_dir > max_ship_speed) {
        ship_dir = max_ship_speed;
    }
    if (ship_dir < -max_ship_speed) {
        ship_dir = -max_ship_speed;
    }
    ship_x += ship_dir;
    if (ship_x < SHIP_MIN_X) {
        ship_x = SHIP_MIN_X;
    }
    if (ship_x > SHIP_MAX_X) {
        ship_x = SHIP_MAX_X;
    }

    draw_ship();
}


/* Unidentified Flying Object */
void move_ufo(void)
{
    static int ufo_speed;
    static int counter;
    int mystery_score;

    switch (ufo_state) {

    case S_IDLE:

        if (rb->rand() % 500 == 0) {
            /* Uh-oh, it's time to launch a mystery UFO */

            /* TODO: Play UFO sound */

            if (rb->rand() % 2) {
                ufo_speed = UFO_SPEED;
                ufo_x = PLAYFIELD_X;
            } else {
                ufo_speed = -UFO_SPEED;
                ufo_x = LCD_WIDTH - PLAYFIELD_X - UFO_WIDTH;
            }
            ufo_state = S_ACTIVE;
            /* UFO will be drawn next frame */
        }
        break;

    case S_ACTIVE:
        /* Erase old pos */
        rb->lcd_fillrect(ufo_x, UFO_Y, UFO_WIDTH, UFO_HEIGHT);
        /* Move */
        ufo_x += ufo_speed;
        /* Check bounds */
        if (ufo_x < PLAYFIELD_X || ufo_x > LCD_WIDTH - PLAYFIELD_X - UFO_WIDTH) {
            ufo_state = S_IDLE;
            break;
        }
        /* Draw new pos */
        rb->lcd_bitmap_part(invadrox, UFO_SRC_X, 0,
                            STRIDE, ufo_x, UFO_Y, UFO_WIDTH, UFO_HEIGHT);
        break;

    case S_SHOWSCORE:
        counter++;
        if (counter == S_IDLE) {
            /* Erase mystery number */
            rb->lcd_fillrect(ufo_x, UFO_Y, 3 * NUMBERS_WIDTH + 2 * NUM_SPACING, FONT_HEIGHT);
            ufo_state = S_IDLE;
        }
        break;

    default:
        /* Exploding */
        ufo_state++;
        if (ufo_state == S_IDLE) {
            /* Erase explosion */
            rb->lcd_fillrect(ufo_x, UFO_Y - 1, UFO_EXPLODE_WIDTH, UFO_EXPLODE_HEIGHT);
            ufo_state = S_SHOWSCORE;
            counter = S_EXPLODE * 4;
            /* Draw mystery_score, sleep, increase score and continue */
            mystery_score = 50 + (rb->rand() % 6) * 50;
            if (mystery_score < 100) {
                draw_number(ufo_x, UFO_Y, mystery_score, 2);
            } else {
                draw_number(ufo_x, UFO_Y, mystery_score, 3);
            }
            score += mystery_score;
            draw_score();
        }
        break;
    }
}


void draw_background(void)
{

#if (LCD_WIDTH == 320) && (LCD_HEIGHT == 240)
    /* Erase background to black */
    rb->lcd_fillrect(PLAYFIELD_X, 0, PLAYFIELD_WIDTH, LCD_HEIGHT);
    /* Left and right bitmaps */
    rb->lcd_bitmap(invadrox_left, 0, 0, PLAYFIELD_X, LCD_HEIGHT);
    rb->lcd_bitmap(invadrox_right, LCD_WIDTH - PLAYFIELD_X, 0, PLAYFIELD_X, LCD_HEIGHT);
#else
    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif

#if ((LCD_WIDTH == 240) && (LCD_HEIGHT == 320)) || ((LCD_WIDTH == 176) && (LCD_HEIGHT == 220))
    rb->lcd_bitmap(invadrox_logo, 0, 0, LCD_WIDTH, SCORE_Y);
#endif

    rb->lcd_update();
}


void new_level(void)
{
    int i;

    draw_background();
    /* Give an extra life for each new level */
    if (lives < MAX_LIVES) {
        lives++;
    }
    draw_lives();

    /* Score */
    rb->lcd_bitmap_part(invadrox, SCORE_SRC_X, SCORE_SRC_Y,
                        STRIDE, PLAYFIELD_X, SCORE_Y, SCORE_WIDTH, FONT_HEIGHT);
    /* Hi-score */
    rb->lcd_bitmap_part(invadrox, 0, SCORE_SRC_Y,
                        STRIDE, HISCORE_X, SCORE_Y,
                        HISCORE_WIDTH, FONT_HEIGHT);
    draw_score();
    draw_number(HISCORENUM_X, SCORENUM_Y, hiscore.score, 4);

    level++;
    draw_level();
    level_finished = false;

    ufo_state = S_IDLE;

    /* Init alien positions and states */
    for (i = 0; i < 4 * ALIENS; i++) {
        aliens[i].x = 0 + (i % ALIENS) * ((ALIEN_WIDTH + ALIEN_SPACING) / ALIEN_SPEED);
        aliens[i].y = 2 * (i / ALIENS);
        aliens[i].state = ALIVE;
    }
    /* Last row, bombers */
    for (i = 4 * ALIENS; i < 5 * ALIENS; i++) {
        aliens[i].x = 0 + (i % ALIENS) * ((ALIEN_WIDTH + ALIEN_SPACING) / ALIEN_SPEED);
        aliens[i].y = 2 * (i / ALIENS);
        aliens[i].state = BOMBER;
    }

    /* Init bombs to inactive (S_IDLE) */
    for (i = 0; i < MAX_BOMBS; i++) {
        bombs[i].state = S_IDLE;
    }

    /* Start aliens closer to earth from level 2 */
    for (i = 0; i < 5 * ALIENS; i++) {
        if (level < 6) {
            aliens[i].y += level - 1;
        } else {
            aliens[i].y += 5;
        }
    }

    /* Max concurrent bombs */
    max_bombs = 1;

    gamespeed = 2;

    if (level > 1) {
        max_bombs++;
    }

    /* Increase speed */
    if (level > 2) {
        gamespeed++;
    }

    if (level > 3) {
        max_bombs++;
    }

    /* Increase speed more */
    if (level > 4) {
        gamespeed++;
    }

    if (level > 5) {
        max_bombs++;
    }

    /* 4 shields */
    for (i = 1; i <= 4; i++) {
        rb->lcd_bitmap_part(invadrox, SHIELD_SRC_X, SHIELD_SRC_Y, STRIDE,
                            PLAYFIELD_X + i * PLAYFIELD_WIDTH / 5 - SHIELD_WIDTH / 2,
                            SHIELD_Y, SHIELD_WIDTH, SHIELD_HEIGHT);
    }

    /* Bottom line */
    rb->lcd_set_foreground(SLIME_GREEN);
    rb->lcd_hline(PLAYFIELD_X, LCD_WIDTH - PLAYFIELD_X, PLAYFIELD_Y);
    /* Restore foreground to black (for fast erase later). */
    rb->lcd_set_foreground(LCD_BLACK);

    ship_x = PLAYFIELD_X + 2 * LIVES_X;
    if (level == 1) {
        old_ship_x = ship_x;
    }
    ship_dir = 0;
    ship_acc = 0;
    ship_frame = 0;
    ship_hit = false;
    fire = S_IDLE;
    /* Start moving the bottom row left to right */
    curr_alien = 4 * ALIENS;
    aliens_paralyzed = 0;
    aliens_right = true;
    aliens_down = false;
    hit_left_border = false;
    hit_right_border = false;
    /* TODO: Change max_ship_speed to 3 at higher levels */
    max_ship_speed = 2;

    draw_aliens();

    rb->lcd_update();
}


void init_invadrox(void)
{
    int i;

    /* Seed random number generator with a "random" number */
    rb->srand(rb->get_time()->tm_sec + rb->get_time()->tm_min * 60);

    /* Precalculate start of each scanline */
    for (i = 0; i < LCD_HEIGHT; i++) {
#if (LCD_DEPTH >= 8)
        ytab[i] = i * LCD_WIDTH;
#elif (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == HORIZONTAL_PACKING)
        ytab[i] = i * (LCD_WIDTH / 4);
#elif (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_PACKING)
        ytab[i] = (i / 4) * LCD_WIDTH;
#else
        #error pixelformat not implemented yet
#endif
    }

    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_BLACK);

    highscore_init(rb);
    if (highscore_load(HISCOREFILE, &hiscore, 1) < 0) {
        /* Init hiscore to 0 */
        rb->strncpy(hiscore.name, "Invader", sizeof(hiscore.name));
        hiscore.score = 0;
        hiscore.level = 1;
    }

    /* Init alien types in aliens array */
    for (i = 0; i < 1 * ALIENS; i++) {
        aliens[i].type = 0; /* Kang */
    }
    for (; i < 3 * ALIENS; i++) {
        aliens[i].type = 1; /* Kodos */
    }
    for (; i < 5 * ALIENS; i++) {
        aliens[i].type = 2; /* Serak */
    }


    /* Save screen white color */
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_drawpixel(0, 0);
    rb->lcd_update_rect(0, 0, 1, 1);
    screen_white = get_pixel(0, 0);

    /* Save screen green color */
    rb->lcd_set_foreground(SLIME_GREEN);
    rb->lcd_drawpixel(0, 0);
    rb->lcd_update_rect(0, 0, 1, 1);
    screen_green = get_pixel(0, 0);

    /* Restore black foreground */
    rb->lcd_set_foreground(LCD_BLACK);

    new_level();

    /* Flash score at start */
    for (i = 0; i < 5; i++) {
        rb->lcd_fillrect(SCORENUM_X, SCORENUM_Y,
                         4 * NUMBERS_WIDTH + 3 * NUM_SPACING,
                         FONT_HEIGHT);
        rb->lcd_update_rect(SCORENUM_X, SCORENUM_Y,
                            4 * NUMBERS_WIDTH + 3 * NUM_SPACING,
                            FONT_HEIGHT);
        rb->sleep(HZ / 10);
        draw_number(SCORENUM_X, SCORENUM_Y, score, 4);
        rb->sleep(HZ / 10);
    }
}


inline bool handle_buttons(void)
{
    static unsigned int oldbuttonstate = 0;

    unsigned int released, pressed, newbuttonstate;

    if (ship_hit) {
        /* Don't allow ship movement during explosion */
        newbuttonstate = 0;
    } else {
        newbuttonstate = rb->button_status();
    }
    if(newbuttonstate == oldbuttonstate) {
        if (newbuttonstate == 0) {
            /* No button pressed. Stop ship. */
            ship_acc = 0;
            if (ship_dir > 0) {
                ship_dir--;
            }
            if (ship_dir < 0) {
                ship_dir++;
            }
        }
        /* return false; */
        goto check_usb;
    }
    released = ~newbuttonstate & oldbuttonstate;
    pressed = newbuttonstate & ~oldbuttonstate;
    oldbuttonstate = newbuttonstate;
    if (pressed) {
        if (pressed & LEFT) {
            if (ship_acc > -1) {
                ship_acc--;
            }
        }
        if (pressed & RIGHT) {
            if (ship_acc < 1) {
                ship_acc++;
            }
        }
        if (pressed & FIRE) {
            if (fire == S_IDLE) {
                /* Fire shot */
                fire_x = ship_x + SHIP_WIDTH / 2;
                fire_y = SHIP_Y - SHOT_HEIGHT;
                fire = S_ACTIVE;
                /* TODO: play fire sound */
            }
        }
#ifdef RC_QUIT
        if (pressed & RC_QUIT) {
            rb->splash(HZ * 1, "Quit");
            return true;
        }
#endif
        if (pressed & QUIT) {
            rb->splash(HZ * 1, "Quit");
            return true;
        }
    }
    if (released) {
        if ((released & LEFT)) {
            if (ship_acc < 1) {
                ship_acc++;
            }
        }
        if ((released & RIGHT)) {
            if (ship_acc > -1) {
                ship_acc--;
            }
        }
    }

check_usb:

    /* Quit if USB is connected */
    if (rb->button_get(false) == SYS_USB_CONNECTED) {
        return true;
    }

    return false;
}


void game_loop(void)
{
    int i, end;

    /* Print dimensions (just for debugging) */
    DBG("%03dx%03dx%02d\n", LCD_WIDTH, LCD_HEIGHT, LCD_DEPTH);

    /* Init */
    init_invadrox();

    while (1) {
        /* Convert CYCLETIME (in ms) to HZ */
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;

        if (handle_buttons()) {
            return;
        }

        /* Animate */
        move_ship();
        move_fire();

        /* Check if level is finished (marked by move_fire) */
        if (level_finished) {
            /* TODO: Play level finished sound */
            new_level();
        }

        move_ufo();

        /* Move aliens */
        if (!aliens_paralyzed && !ship_hit) {
            for (i = 0; i < gamespeed; i++) {
                if (!move_aliens()) {
                    if (game_over) {
                        return;
                    }
                }
            }
        }

        /* Move alien bombs */
        move_bombs();
        if (game_over) {
            return;
        }

        /* Update "playfield" rect */
        rb->lcd_update_rect(PLAYFIELD_X, SCORENUM_Y + FONT_HEIGHT,
                            PLAYFIELD_WIDTH,
                            PLAYFIELD_Y + 1 - SCORENUM_Y - FONT_HEIGHT);

        /* Wait until next frame */
        DBG("%d (%d)\n", end - *rb->current_tick, (CYCLETIME * HZ) / 1000);
        if (end > *rb->current_tick) {
            rb->sleep(end - *rb->current_tick);
        } else {
            rb->yield();
        }

    } /* end while */
}


/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, UNUSED void* parameter)
{
    rb = api;

    rb->lcd_setfont(FONT_SYSFIXED);
    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

    /* now go ahead and have fun! */
    game_loop();

    /* Game Over. */
    /* TODO: Play game over sound */
    rb->splash(HZ * 2, "Game Over");
    if (score > hiscore.score) {
        /* Save new hiscore */
        hiscore.score = score;
        hiscore.level = level;
        highscore_save(HISCOREFILE, &hiscore, 1);
    }

    /* Restore user's original backlight setting */
    rb->lcd_setfont(FONT_UI);
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

    return PLUGIN_OK;
}



/**
 * GNU Emacs settings: Kernighan & Richie coding style with
 *                     4 spaces indent and no tabs.
 * Local Variables:
 *  c-file-style: "k&r"
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 * End:
 */
