/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: clock.c,v 3.00 2003/12/8
 *
 * Copyright (C) 2003 Zakk Roberts
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*****************************
 * RELEASE NOTES

***** VERSION 3.00 **
New, simpler UI - every screen can be accessed from the new Main Menu.
Huge code cleanup - many major functions rewritten. Functions optimized,
targetting scalability. Number of variables reduced majorly. Faster, simpler.
New clock mode: plain (simple, large text). ON now controls counter
(press toggle/hold reset). Fancier credits roll. New logo. iRiver and iPod ports
are working but not yet scaled to fit their LCDs.

***** VERSION 2.60 **
Fixed general settings typo, split up settings function, added cursor animations,
and updated cursor look (rounded edges).

***** VERSION 2.51 **
-"Show Counter" option is now saved to disk

***** VERSION 2.50 **
-New general settings mode added, -reworked options screen,
-cleaned up a few things and removed redundant code, -faster
load_settings(), fixed a help-screen bug (thanks to zeekoe)

***** VERSION 2.40 **
-Cleaned and optimized code, -removed unused code and bitmaps,
-Progressbar and more animations at credits screen, -centered
text all over, -general settings added at ON+F3, -new arrow bitmap
for general settings and mode selector, -bugfix: 12:00AM is no longer
00:00AM

***** VERSION 2.31 **
Fixed credits roll - now displays all names. Features
improved animations. Also revised release notes.

***** VERSION 2.30 **
Tab indentation removed, and -Counter screen added
at ON+F2, with countdown options

***** VERSION 2.22 **
Fixed two bugs:
Digital settings are now independent of LCD settings
12/24h "Analog" settings are now displayed correctly.

***** VERSION 2.21 **
-Changed the behaviour of F2

***** VERSION 2.20 **
Few small bugs taken care of. New features: -New binary mode,
-new mode selector, -new feature, "counter", and -redesigned help screen.

***** VERSION 2.10 **
New bug fixes, and some new features: -an LCD imitation mode, and
-American and European date modes are an option.

***** VERSION 2.00 [BETA] **
Major update, lots of bugfixes and new features.
New Features: -Fullscreen mode introduced, -modes have independent
settings, -credit roll added, -options screen reworked, -logo selector,
and -much- cleaner code. Analog changes include: -removed border option,
and -added both 12/24h time readouts. Digital changes include: -centered
second and date readouts and also -introduced two new additional ways
of graphically conveying second progress: a bar, and a LCD-invert mode.

***** VERSION 1.0 **
Original release, featuring analog / digital modes and a few options.
*****************************/
#include "plugin.h"
#include "time.h"

#if !(CONFIG_KEYPAD == IPOD_3G_PAD) /* let's not compile for 3g for now */

PLUGIN_HEADER

#define CLOCK_VERSION "v3.0"

#define ANALOG     1
#define DIGITAL    2
#define LCD        3
#define FULLSCREEN 4
#define BINARY     5
#define PLAIN      6

#define OFFSET 1

#define UP    1
#define DOWN -1

/* we need to "fake" the LCD width/height, because this plugin isn't
 * yet adapted to other screen sizes */
#define LCDWIDTH  112
#define LCDHEIGHT 64

#if (CONFIG_KEYPAD == RECORDER_PAD)

#define COUNTER_TOGGLE_BUTTON (BUTTON_ON|BUTTON_REL)
#define COUNTER_RESET_BUTTON (BUTTON_ON|BUTTON_REPEAT)
#define MENU_BUTTON BUTTON_PLAY
#define ALT_MENU_BUTTON BUTTON_F1
#define EXIT_BUTTON BUTTON_OFF
#define MOVE_UP_BUTTON BUTTON_UP
#define MOVE_DOWN_BUTTON BUTTON_DOWN
#define CHANGE_UP_BUTTON BUTTON_RIGHT
#define CHANGE_DOWN_BUTTON BUTTON_LEFT

#define YESTEXT "Play"
#define NAVI_BUTTON_TEXT_LEFT "LEFT"
#define NAVI_BUTTON_TEXT_RIGHT "RIGHT"
#define EXIT_BUTTON_TEXT "OFF"
#define MENU_BUTTON_TEXT "PLAY"
#define COUNTER_BUTTON_TEXT "ON"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)

#define COUNTER_TOGGLE_BUTTON (BUTTON_PLAY|BUTTON_REL)
#define COUNTER_RESET_BUTTON (BUTTON_PLAY|BUTTON_REPEAT)
#define MENU_BUTTON BUTTON_SELECT
#define EXIT_BUTTON BUTTON_MENU
#define MOVE_UP_BUTTON BUTTON_SCROLL_BACK
#define MOVE_DOWN_BUTTON BUTTON_SCROLL_FWD
#define CHANGE_UP_BUTTON BUTTON_RIGHT
#define CHANGE_DOWN_BUTTON BUTTON_LEFT

#define YESTEXT "Select"
#define NAVI_BUTTON_TEXT_LEFT "LEFT"
#define NAVI_BUTTON_TEXT_RIGHT "RIGHT"
#define EXIT_BUTTON_TEXT "MENU"
#define MENU_BUTTON_TEXT "SELECT"
#define COUNTER_BUTTON_TEXT "PLAY"

#elif (CONFIG_KEYPAD == IRIVER_H300_PAD)

#define COUNTER_TOGGLE_BUTTON (BUTTON_ON|BUTTON_REL)
#define COUNTER_RESET_BUTTON (BUTTON_ON|BUTTON_REPEAT)
#define MENU_BUTTON BUTTON_SELECT
#define EXIT_BUTTON BUTTON_OFF
#define MOVE_UP_BUTTON BUTTON_UP
#define MOVE_DOWN_BUTTON BUTTON_DOWN
#define CHANGE_UP_BUTTON BUTTON_RIGHT
#define CHANGE_DOWN_BUTTON BUTTON_LEFT

#define YESTEXT "Select/Navi"
#define NAVI_BUTTON_TEXT_LEFT "LEFT"
#define NAVI_BUTTON_TEXT_RIGHT "RIGHT"
#define EXIT_BUTTON_TEXT "STOP"
#define MENU_BUTTON_TEXT "NAVI"
#define COUNTER_BUTTON_TEXT "PLAY"

#endif

/************
 * Prototypes
 ***********/
void show_clock_logo(bool animate, bool show_clock_text);
void exit_logo(void);
void save_settings(bool interface);

/********************
 * Misc counter stuff
 *******************/
int start_tick = 0;
int passed_time = 0;
int counter = 0;
int displayed_value = 0;
int count_h, count_m, count_s;
char count_text[8];
bool counting = false;
bool counting_up = true;
int target_hour=0, target_minute=0, target_second=0;
int remaining_h=0, remaining_m=0, remaining_s=0;
bool editing_target = false;

/*********************
 * Used to center text
 ********************/
char buf[20];
int buf_w, buf_h;

/********************
 * Everything else...
 *******************/
int menupos = 1;
bool idle_poweroff = true; /* poweroff activated or not? */

/* This bool is used for most of the while loops */
bool done = false;

static struct plugin_api* rb;

/***********************************************************
 * Used for hands to define lengths at a given time - ANALOG
 **********************************************************/
unsigned char xminute[61];
static const unsigned char yminute[] = {
55,54,54,53,53,51,50,49,47,45,43,41,39,36,34,32,30,28,25,23,21,19,17,15,14,13,
11,11,10,10, 9,10,10,11,11,13,14,15,17,19,21,23,25,28,30,32,34,36,39,41,43,45,
47,49,50,51,53,53,54,54 };
static const unsigned char yhour[] = {
47,47,46,46,46,45,44,43,42,41,39,38,36,35,33,32,31,29,28,26,25,23,22,21,20,19,
18,18,18,17,17,17,18,18,18,19,20,21,22,23,25,26,28,29,31,32,33,35,36,38,39,41,
42,43,44,45,46,46,46,47 };
unsigned char xhour[61];

/**************************************************************
 * Used for hands to define lengths at a give time - FULLSCREEN
 *************************************************************/
static const unsigned char xminute_full[] = {
56,58,61,65,69,74,79,84,91,100,110,110,110,110,110,110,110,110,110,110,110,100,
91,84,79,74,69,65,61,58,56,54,51,47,43,38,33,28,21,12,1,1,1,1,1,1,1,1,1,1,1,12,
21,28,33,38,43,47,51,54 };
static const unsigned char yminute_full[] = {
62,62,62,62,62,62,62,62,62,62,62,53,45,40,36,32,28,24,19,11,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,11,19,24,28,32,36,40,45,53,62,62,62,62,62,62,62,62,62,62 };
static const unsigned char xhour_full[] = {
56,58,60,63,66,69,73,78,84,91,100,100,100,100,100,100,100,100,100,100,100,91,84,
78,73,69,66,63,60,58,56,54,52,49,46,43,39,34,28,21,12,12,12,12,12,12,12,12,12,
12,12,21,28,34,39,43,46,49,52,54 };
static const unsigned char yhour_full[] = {
52,52,52,52,52,52,52,52,52,52,52,46,41,37,34,32,30,27,23,18,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,18,23,27,30,32,34,37,41,46,52,52,
52,52,52,52,52,52,52,52 };

/****************************
 * BITMAPS
 ****************************/
/*************************
 * "0" bitmap - for binary
 ************************/
static const unsigned char bitmap_0[] = {
0xc0, 0xf0, 0x3c, 0x0e, 0x06, 0x03, 0x03, 0x03, 0x03, 0x06, 0x0e, 0x3c, 0xf0,
0xc0, 0x00, 0x1f, 0x7f, 0xe0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
0xe0, 0x7f, 0x1f, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06,
0x03, 0x03, 0x01, 0x00, 0x00, 0x00 };
/*************************
 * "1" bitmap - for binary
 ************************/
static const unsigned char bitmap_1[] = {
0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x06,
0x06, 0x06, 0x06, 0x06, 0x06, 0x00 };
/**********************************
 * Empty circle bitmap - for binary
 *********************************/
const unsigned char circle_empty[] = {
0xf0, 0x0c, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x0c,
0xf0, 0x03, 0x0c, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x10,
0x0c, 0x03 };
/*********************************
 * Full circle bitmap - for binary
 ********************************/
const unsigned char circle_full[] = {
0xf0, 0xfc, 0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc,
0xf0, 0x03, 0x0f, 0x1f, 0x1f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x1f, 0x1f,
0x0f, 0x03 };

/*******************************
 * Colon bitmap - for plain mode
 ******************************/
static const unsigned char plain_colon[] = {
0x00, 0x00, 0x00, 0x00, 0x00,
0x1e, 0x3f, 0x3f, 0x3f, 0x1e,
0x80, 0xc0, 0xc0, 0xc0, 0x80,
0x07, 0x0f, 0x0f, 0x0f, 0x07 };
/*****************************
 * "0" bitmap - for plain mode
 ****************************/
const unsigned char plain_0[] = {
0x00, 0xe0, 0xf8, 0xfc, 0xfe, 0x1e, 0x0f, 0x07, 0x07, 0x07, 0x0f, 0x1f, 0x7e,
0xfc, 0xfc, 0xf0, 0x80,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0xff, 0xff, 0xff,
0x0f, 0xff, 0xff, 0xff, 0xf0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xf0,
0xff, 0xff, 0x7f, 0x0f,
0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0f, 0x07, 0x07,
0x03, 0x01, 0x00, 0x00 };
/*****************************
 * "1" bitmap - for plain mode
 ****************************/
const unsigned char plain_1[] = {
0x00, 0x00, 0xc0, 0xe0, 0xe0, 0xf0, 0x78, 0xf8, 0xfc, 0xfe, 0xff, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00 };
/*****************************
 * "2" bitmap - for plain mode
 ****************************/
const unsigned char plain_2[] = {
0x18, 0x3c, 0x1e, 0x0e, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x1e, 0xfe, 0xfc,
0xf8, 0xf0, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xf0, 0xfc, 0xff, 0x3f,
0x0f, 0x03, 0x00, 0x00,
0x00, 0x00, 0x80, 0xc0, 0xf0, 0xf8, 0xfe, 0x7f, 0x1f, 0x07, 0x03, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x0c, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e,
0x0e, 0x0e, 0x0e, 0x0e };
/*****************************
 * "3" bitmap - for plain mode
 ****************************/
const unsigned char plain_3[] = {
0x00, 0x04, 0x0e, 0x0e, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x1e, 0xfe,
0xfc, 0xf8, 0xf0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x70, 0x70, 0xf8, 0xdc, 0xdf,
0x8f, 0x87, 0x01, 0x00,
0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83,
0xff, 0xff, 0xff, 0x7c,
0x02, 0x07, 0x07, 0x0f, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x07, 0x07,
0x07, 0x03, 0x01, 0x00 };
/*****************************
 * "4" bitmap - for plain mode
 ****************************/
const unsigned char plain_4[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xf0, 0xf8, 0xfc, 0xfe,
0xff, 0x00, 0x00, 0x00,
0x00, 0x80, 0xe0, 0xf0, 0x78, 0x3c, 0x1f, 0x07, 0x03, 0x01, 0xff, 0xff, 0xff,
0xff, 0x00, 0x00, 0x00,
0x0f, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0xff, 0xff, 0xff,
0xff, 0x0e, 0x0e, 0x0e,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f,
0x0f, 0x00, 0x00, 0x00 };
/*****************************
 * "5" bitmap - for plain mode
 ****************************/
const unsigned char plain_5[] = {
0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
0x07, 0x07, 0x07, 0x00,
0x00, 0x1f, 0x3f, 0x1f, 0x1f, 0x0c, 0x0e, 0x0e, 0x0e, 0x0e, 0x1e, 0x1e, 0x7c,
0xfc, 0xf8, 0xf0, 0xc0,
0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0,
0xff, 0xff, 0xff, 0x3f,
0x02, 0x07, 0x07, 0x0f, 0x0f, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x07, 0x07,
0x03, 0x01, 0x00, 0x00 };
/*****************************
 * "6" bitmap - for plain mode
 ****************************/
const unsigned char plain_6[] = {
0x00, 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0x7c, 0x3e, 0x1e, 0x0f, 0x07, 0x02,
0x00, 0x00, 0x00, 0x00,
0xf0, 0xfc, 0xff, 0xff, 0xff, 0x73, 0x39, 0x38, 0x38, 0x38, 0x38, 0x78, 0xf0,
0xf0, 0xe0, 0xc0, 0x00,
0x1f, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc1,
0xff, 0xff, 0xff, 0x7f,
0x00, 0x00, 0x01, 0x03, 0x07, 0x07, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x07,
0x07, 0x03, 0x01, 0x00 };
/*****************************
 * "7" bitmap - for plain mode
 ****************************/
const unsigned char plain_7[] = {
0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xc7, 0xf7, 0xff,
0xff, 0x7f, 0x1f, 0x07,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xf8, 0xff, 0xff, 0x1f, 0x07,
0x01, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x80, 0xe0, 0xfc, 0xff, 0x7f, 0x1f, 0x03, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x0c, 0x0f, 0x0f, 0x0f, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00 };
/*****************************
 * "8" bitmap - for plain mode
 ****************************/
const unsigned char plain_8[] = {
0x00, 0xf0, 0xfc, 0xfe, 0xfe, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x0f, 0xfe,
0xfe, 0xfc, 0xf0, 0x00,
0x00, 0x81, 0xc3, 0xef, 0xef, 0xff, 0x7e, 0x3c, 0x38, 0x78, 0xfc, 0xfe, 0xff,
0xcf, 0x87, 0x01, 0x00,
0x7e, 0xff, 0xff, 0xff, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x83,
0xff, 0xff, 0xff, 0x7e,
0x00, 0x01, 0x03, 0x07, 0x07, 0x0f, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x07,
0x07, 0x03, 0x01, 0x00 };
/*****************************
 * "9" bitmap - for plain mode
 ****************************/
const unsigned char plain_9[] = {
0xe0, 0xf8, 0xfc, 0xfe, 0x3e, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x0e, 0x3e,
0xfc, 0xf8, 0xf0, 0x80,
0x0f, 0x3f, 0x7f, 0xff, 0xf8, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xf0,
0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x81, 0xc1, 0xe1, 0xf9, 0xfc, 0x7f,
0x3f, 0x0f, 0x03, 0x00,
0x00, 0x00, 0x00, 0x00, 0x04, 0x0e, 0x0f, 0x07, 0x07, 0x03, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00 };

/**********************
 * Digital colon bitmap
 *********************/
const unsigned char digital_colon[] = {
0x04, 0x0e, 0x1f, 0x0e, 0x04 };

/********************************************
 * Used to define current bitmap - PLAIN MODE
 *******************************************/
const char *plain_bitmaps[] = {
plain_0, plain_1, plain_2, plain_3, plain_4, plain_5, plain_6,
plain_7, plain_8, plain_9 };

/**************
 * PM indicator
 *************/
static const unsigned char pm[] = {
0xFF,0xFF,0x33,0x33,0x33,0x1E,0x0C,0x00,0xFF,0xFF,0x06,0x0C,0x06,0xFF,0xFF };
/**************
 * AM Indicator
 *************/
static const unsigned char am[] = {
0xFE,0xFF,0x1B,0x1B,0xFF,0xFE,0x00,0x00,0xFF,0xFF,0x06,0x0C,0x06,0xFF,0xFF };

/**************
 * Arrow bitmap
 *************/
static const unsigned char arrow[] = {
0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x1E, 0x0C };

/***************************
 * Unchecked checkbox bitmap
 **************************/
const unsigned char checkbox_empty[] = {
0x3F, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x3F };
/*****************************
 * 1/3 checked checkbox bitmap
 ****************************/
const unsigned char checkbox_onethird[] = {
0x3F, 0x2B, 0x35, 0x21, 0x21, 0x21, 0x21, 0x3F };
/*****************************
 * 1/2 checked checkbox bitmap
 ****************************/
const unsigned char checkbox_half[] = {
0x3F, 0x2B, 0x35, 0x2B, 0x21, 0x21, 0x21, 0x3F };
/*****************************
 * 2/3 checked checkbox bitmap
 ****************************/
const unsigned char checkbox_twothird[] = {
0x3F, 0x2B, 0x35, 0x2B, 0x35, 0x21, 0x21, 0x3F };
/*************************
 * Checked checkbox bitmap
 ************************/
const unsigned char checkbox_full[] = {
0x3F, 0x2B, 0x35, 0x2B, 0x35, 0x2B, 0x35, 0x3F };

/*********************
 * Clock logo (112x37)
 ********************/
const unsigned char clocklogo[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf0, 0xf8, 0xfc, 0x7c,
0x3c, 0x3e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x3e, 0x3e, 0x1c, 0x08, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xfc, 0xfe, 0xff, 0xff, 0xff,
0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xfe, 0xff, 0xff, 0xff, 0x0e, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xc0, 0xf0, 0xfc, 0xff, 0xff, 0x7f, 0x1f, 0x07, 0x03, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xe0, 0xf0, 0xf0,
0xf0, 0x78, 0x78, 0x78, 0x78, 0x78, 0xf8, 0xf8, 0xf0, 0xf0, 0xe0, 0xe0, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xe0, 0xf0,
0xf0, 0xf8, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x30, 0x00, 0x00,
0x00, 0x00, 0x00, 0x80, 0xfe, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00,
0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf0, 0x60, 0x00,
0xfc, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xc0, 0xfe, 0xff, 0xff, 0xff, 0x7f, 0x03, 0x00, 0x00,
0x00, 0x00, 0x00, 0x80, 0xf0, 0xfc, 0xff, 0xff, 0x7f, 0x0f, 0x03, 0x01, 0x80,
0xc0, 0xf0, 0x9c, 0x07, 0x01, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff,
0xfe, 0x00, 0x00, 0x00, 0x80, 0xf0, 0xfc, 0xff, 0xff, 0x7f, 0x0f, 0x03, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xf0, 0xf8, 0xfc, 0x3e,
0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00, 0x00,
0x0f, 0x7f, 0xff, 0xff, 0xff, 0xf8, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01,
0x03, 0x03, 0x07, 0x0c, 0x18, 0x80, 0xc0, 0xf0, 0xfe, 0xff, 0xff, 0x3f, 0x0f,
0x01, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x80, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x03, 0x03, 0x07, 0x1f, 0x3f, 0xff,
0xfc, 0xf8, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x0f, 0x0f, 0x1f, 0x1f, 0x1e, 0x1e, 0x1e,
0x1e, 0x1e, 0x1e, 0x0f, 0x0f, 0x0f, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x03, 0x07, 0x0f, 0x0f, 0x1f, 0x1f, 0x1e, 0x0e, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x0f, 0x1f, 0x1f, 0x1e, 0x1e,
0x1e, 0x1e, 0x1e, 0x0f, 0x0f, 0x0f, 0x07, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x0f, 0x1f, 0x1f, 0x1e,
0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
0x0c, 0x1f, 0x1f, 0x1f, 0x0f, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x07, 0x0f, 0x1f, 0x1f, 0x0e, 0x00, 0x00 };

/******************
 * Time's Up bitmap
 *****************/
const unsigned char timesup[] = {
0x78, 0x78, 0x78, 0x38, 0x08, 0x08, 0xf8, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x04,
0x04, 0x04, 0x0c, 0x3c, 0x3c, 0x3c, 0x04, 0x04, 0x04, 0xfc, 0xfc, 0xfc, 0xfc,
0xfe, 0xfe, 0x06, 0x03, 0x03, 0x05, 0x05, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff,
0xfc, 0xfc, 0xf8, 0xf0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xf0, 0xfc, 0xfc,
0xfc, 0xfc, 0xfc, 0x20, 0x22, 0x22, 0x22, 0x22, 0x02, 0x02, 0x02, 0x02, 0x0e,
0xfe, 0xfe, 0xfe, 0xfe, 0x06, 0x06, 0x06, 0x06, 0x06, 0x0e, 0x1c, 0x3c, 0x3c,
0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xfc, 0xfc, 0xfc, 0xfe, 0x00,
0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0xfe, 0xfe, 0xfe, 0xfe,
0x86, 0x0e, 0x3e, 0xfe, 0xfe, 0xfe, 0xfe, 0x1e,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
0x03, 0x0f, 0x3f, 0xff, 0xff, 0xfc, 0xf0, 0xfc, 0xff, 0x7f, 0x1f, 0x03, 0xff,
0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0xff, 0xff, 0xff, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0e, 0x0e, 0x0e, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0xc0, 0xe0, 0xe0, 0xe3, 0xc7, 0x8f, 0x0f,
0x1f, 0x1f, 0x3e, 0xfe, 0xfc, 0xf8, 0xf8, 0xf0,
0x08, 0x08, 0x08, 0x08, 0x18, 0x18, 0x1f, 0x1f, 0x1f, 0x1f, 0x0f, 0x0f, 0x0c,
0x08, 0x08, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0f, 0x0f, 0x0f, 0x0f,
0x0f, 0x0f, 0x0c, 0x08, 0x08, 0x88, 0x80, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x03, 0x0f, 0x0f, 0x07, 0x01, 0x10, 0x18, 0x1c, 0x0f,
0x0f, 0x0f, 0x0f, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c,
0x0f, 0x0f, 0x0f, 0x0f, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1c, 0x1c,
0x1c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x0f, 0x0f, 0x0f, 0x0f,
0x0e, 0x0c, 0x0c, 0x0f, 0x0f, 0x0f, 0x0f, 0x03,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0xc8, 0xf8, 0xf8, 0xf8, 0x18, 0x08,
0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xe0, 0xe0, 0x60, 0x60, 0xe0, 0xe0,
0xe0, 0xe0, 0xa0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xc0, 0xc0, 0xe0, 0xf1,
0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3f, 0x7f, 0xff, 0xff, 0xf0,
0xe0, 0xc0, 0xc0, 0xc0, 0xe0, 0xf0, 0xf0, 0x7c, 0x7f, 0x3f, 0x07, 0x04, 0x04,
0x04, 0x04, 0x04, 0x04, 0x04, 0xe4, 0xff, 0xff, 0xff, 0xc0, 0x80, 0x80, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3,
0xe1, 0xe7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x02, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* settings are saved to this location */
static const char default_filename[] = "/.rockbox/rocks/.clock_settings";

/* names of contributors */
const char* credits[] = {
"Zakk Roberts",
"Linus Feltzing",
"BlueChip",
"T.P. Diffenbach",
"David McIntyre",
"Justin Young",
"Lee Pilgrim",
"top_bloke",
"Adam Spirer",
"Scott Myran",
"Tony Kirk",
"Jason Tye" };

/* ...and how they helped */
const char* jobs[] = {
"Code",
"Code",
"Code",
"Code",
"Code",
"Code",
"Code",
"Code",
"Pre-3.0 Logo",
"Design",
"Design",
"Design" };

/*********************************************************
 * Some arrays/definitions for drawing settings/menu text.
 * Modes are abbreviated to one letter i.e. "analog" = "a"
 ********************************************************/
#define analog_digits_text "Digits"
const char* analog_date_text[] = {
"Date: Off",
"Date: American",
"Date: European" };
#define analog_secondhand_text "Second Hand"
const char* analog_time_text[] = {
"Show Time: Off",
"Show Time: 24hr",
"Show Time: 12hr", };
const char* digital_seconds_text[] = {
"Seconds: Off",
"Seconds: Digital",
"Seconds: Bar",
"Seconds: Inverse" };
const char* digital_date_text[] = {
"Date: Off",
"Date: American",
"Date: European" };
#define digital_blinkcolon_text "Blinking Colon"
#define digital_12h_text "12-Hour Format"
const char* lcd_seconds_text[] = {
"Seconds: Off",
"Seconds: Digital",
"Seconds: Bar",
"Seconds: Inverse" };
const char* lcd_date_text[] = {
"Date: Off",
"Date: American",
"Date: European" };
#define lcd_blinkcolon_text "Blinking Colon"
#define lcd_12h_text "12-Hour Format"
#define fullscreen_border_text "Show Border"
#define fullscreen_secondhand_text "Second Hand"
#define fullscreen_invertseconds_text "Invert Seconds"
#define binary_dots_text "Dot Mode"
#define plain_12h_text "12-Hour Format"
const char* plain_date_text[] = {
"Date: Off",
"Date: American",
"Date: European" };
#define plain_blinkcolon_text "Blinking Colon"
const char* menu_entries[] = {
"View Clock",
"Mode Selector",
"Counter Settings",
"Mode Settings",
"General Settings",
"Help",
"Credits" };
const char* mode_selector_entries[] = {
"Analog",
"Digital",
"LCD",
"Fullscreen",
"Binary",
"Plain" };
#define general_reset_text "Reset Settings"
#define general_save_text "Save Settings"
#define general_counter_text "Show Counter"
const char* general_savesetting_text[] = {
"Save: Manually",
"Save: on Exit",
"Save: Automatic" };
#define general_idle_text "Idle Poweroff"
const char* general_backlight_text[] = {
"Backlight: Off",
"Backlight: RB",
"Backlight: On" };

#define ANALOG_SETTINGS     4
#define DIGITAL_SETTINGS    4
#define LCD_SETTINGS        4
#define FULLSCREEN_SETTINGS 3
#define BINARY_SETTINGS     1
#define PLAIN_SETTINGS      3
#define GENERAL_SETTINGS    4

#define analog_digits 0
#define analog_date 1
#define analog_secondhand 2
#define analog_time 3
#define digital_seconds 0
#define digital_date 1
#define digital_blinkcolon 2
#define digital_12h 3
#define lcd_seconds 0
#define lcd_date 1
#define lcd_blinkcolon 2
#define lcd_12h 3
#define fullscreen_border 0
#define fullscreen_secondhand 1
#define fullscreen_invertseconds 2
#define binary_dots 0
#define plain_12h 0
#define plain_date 1
#define plain_blinkcolon 2
#define general_counter 0
#define general_savesetting 1
#define general_backlight 2

/***********************************
 * This is saved to default_filename
 **********************************/
struct saved_settings
{
    /* general */
    int clock; /* 1: analog, 2: digital, 3: lcd, 4: full, 5: binary, 6: plain */
    int general[GENERAL_SETTINGS];
    int analog[ANALOG_SETTINGS];
    int digital[DIGITAL_SETTINGS];
    int lcd[LCD_SETTINGS];
    int fullscreen[FULLSCREEN_SETTINGS];
    int binary[BINARY_SETTINGS];
    int plain[PLAIN_SETTINGS];
} settings;

int analog_max[ANALOG_SETTINGS] = {1, 2, 1, 2};
int digital_max[DIGITAL_SETTINGS] = {3, 2, 1, 1};
#define fullscreen_max 1
#define binary_max 1
int plain_max[PLAIN_SETTINGS] = {1, 2, 1};
int general_max[GENERAL_SETTINGS] = {1, 2, 2, 1};

/************************
 * Setting default values
 ***********************/
void reset_settings(void)
{
    settings.clock = 1;
    settings.general[general_counter] = 1;
    settings.general[general_savesetting] = 1;
    settings.general[general_backlight] = 2;
    settings.analog[analog_digits] = false;
    settings.analog[analog_date] = 0;
    settings.analog[analog_secondhand] = true;
    settings.analog[analog_time] = false;
    settings.digital[digital_seconds] = 1;
    settings.digital[digital_date] = 1;
    settings.digital[digital_blinkcolon] = false;
    settings.digital[digital_12h] = true;
    settings.lcd[lcd_seconds] = 1;
    settings.lcd[lcd_date] = 1;
    settings.lcd[lcd_blinkcolon] = false;
    settings.lcd[lcd_12h] = true;
    settings.fullscreen[fullscreen_border] = true;
    settings.fullscreen[fullscreen_secondhand] = true;
    settings.fullscreen[fullscreen_invertseconds] = false;
    settings.plain[plain_12h] = true;
    settings.plain[plain_date] = 1;
    settings.plain[plain_blinkcolon] = false;
}

/************************************************
 * Precalculated sine * 16384 (fixed point 18.14)
 ***********************************************/
static const short sin_table[91] =
{
    0,     285,   571,   857,   1142,  1427,  1712,  1996,  2280,  2563,
    2845,  3126,  3406,  3685,  3963,  4240,  4516,  4790,  5062,  5334,
    5603,  5871,  6137,  6401,  6663,  6924,  7182,  7438,  7691,  7943,
    8191,  8438,  8682,  8923,  9161,  9397,  9630,  9860,  10086, 10310,
    10531, 10748, 10963, 11173, 11381, 11585, 11785, 11982, 12175, 12365,
    12550, 12732, 12910, 13084, 13254, 13420, 13582, 13740, 13894, 14043,
    14188, 14329, 14466, 14598, 14725, 14848, 14967, 15081, 15190, 15295,
    15395, 15491, 15582, 15668, 15749, 15825, 15897, 15964, 16025, 16082,
    16135, 16182, 16224, 16261, 16294, 16321, 16344, 16361, 16374, 16381,
    16384 };

/******************************
 * Sine function (from plasma.c
 *****************************/
static short sin(int val)
{
    /* value should be between 0 and 360 degree for correct lookup*/
    val%=360;
    if(val<0)
        val+=360;

    /* Speed improvement through successive lookup */
    if (val < 181)
    {
        if (val < 91)
            return (short)sin_table[val]; /* phase 0-90 degree */
        else
            return (short)sin_table[180-val]; /* phase 91-180 degree */
    }
    else
    {
        if (val < 271)
            return -(short)sin_table[val-180]; /* phase 181-270 degree */
        else
            return -(short)sin_table[360-val]; /* phase 270-359 degree */
    }
    return 0;
}

/********************************
 * Simple function to center text
 *******************************/
void center_text(int y, char* text)
{
    rb->snprintf(buf, sizeof(buf), "%s", text);
    rb->lcd_getstringsize(buf, &buf_w, &buf_h);
    rb->lcd_putsxy(LCDWIDTH/2 - buf_w/2, y, text);
}

/**************************
 * Cleanup on plugin return
 *************************/
void cleanup(void *parameter)
{
    (void)parameter;

    if(settings.general[general_savesetting] == 1)
        save_settings(true);

    /* restore set backlight timeout */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/********************************
 * Saves "saved_settings" to disk
 *******************************/
void save_settings(bool interface)
{
    int fd;

    if(interface)
    {
        rb->lcd_clear_display();

        /* display information */
        center_text(56, "Saving Settings");
        show_clock_logo(true, true);

        rb->lcd_update();
    }

    fd = rb->creat(default_filename, O_WRONLY); /* create the settings file */

    if(fd >= 0) /* file exists, save successful */
    {
        rb->write (fd, &settings, sizeof(struct saved_settings));
        rb->close(fd);

        if(interface)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            center_text(56, "Saved Settings");
        }
    }
    else /* couldn't save for some reason */
    {
        if(interface)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            center_text(56, "Save Failed!");
        }
    }

    if(interface)
    {
        rb->lcd_update();

        rb->sleep(HZ); /* pause a second */

        exit_logo();
    }
}

/**********************************
 * Loads "saved_settings" from disk
 *********************************/
void load_settings(void)
{
    /* open the settings file */
    int fd;
    fd = rb->open(default_filename, O_RDONLY);

    center_text(48, "Clock " CLOCK_VERSION);
    center_text(56, "Loading Settings");

    show_clock_logo(true, true);
    rb->lcd_update();

    if(fd >= 0) /* does file exist? */
    {
        if(rb->filesize(fd) == sizeof(struct saved_settings)) /* if so, is it the right size? */
        {
            rb->read(fd, &settings, sizeof(struct saved_settings));
            rb->close(fd);

            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            center_text(56, "Loaded Settings");
        }
        else /* must be invalid, bail out */
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            center_text(56, "Old Settings File");
            reset_settings();
        }
    }
    else /* must be missing, bail out */
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 56, 112, 8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        center_text(56, "No Settings File");

        /* use the default in this case */
        reset_settings();
    }

    rb->lcd_update();

#ifndef SIMULATOR
    rb->ata_sleep();
#endif

    rb->sleep(HZ);

    exit_logo();
}

/*******************************
 * Init clock, set up x/y tables
 ******************************/
void init_clock(void)
{
    #define ANALOG_VALUES 60
    #define ANALOG_MIN_RADIUS 28
    #define ANALOG_HR_RADIUS 20
    #define ANALOG_CENTER 56
    #define PI 3.141592
    int i;

    rb->lcd_setfont(FONT_SYSFIXED); /* universal font */

    load_settings();

    /* set backlight timeout */
    if(settings.general[general_backlight] == 0)
        rb->backlight_set_timeout(0);
    else if(settings.general[general_backlight] == 1)
        rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
    else if(settings.general[general_backlight] == 2)
        rb->backlight_set_timeout(1);

    for(i=0; i<ANALOG_VALUES; i++)
    {
        xminute[i] = (sin(360 * i / ANALOG_VALUES) * ANALOG_MIN_RADIUS / 16384) +
                      ANALOG_CENTER;
        xhour[i] = (sin(360 * i / ANALOG_VALUES) * ANALOG_HR_RADIUS / 16384) +
                      ANALOG_CENTER;
    }
}

/*******************
 * Analog clock mode
 ******************/
void analog_clock(int hour, int minute, int second)
{
    int pos, i;

    /* Second hand */
    if(settings.analog[analog_secondhand])
    {
        pos = 90-second;
        if(pos >= 60)
            pos -= 60;

        rb->lcd_drawline((LCDWIDTH/2), (LCDHEIGHT/2),
                         xminute[pos], yminute[pos]);
    }

    pos = 90-minute;
    if(pos >= 60)
        pos -= 60;

    /* Minute hand, thicker than the second hand */
    rb->lcd_drawline(LCDWIDTH/2, LCDHEIGHT/2,
                     xminute[pos], yminute[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2-1,
                     xminute[pos], yminute[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2+1,
                     xminute[pos], yminute[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2+1,
                     xminute[pos], yminute[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2-1,
                     xminute[pos], yminute[pos]);

    if(hour > 12)
        hour -= 12;

    hour = (hour*5) + (minute/12);
    pos = 90-hour;
    if(pos >= 60)
        pos -= 60;

    /* Hour hand, thick as the minute hand but shorter */
    rb->lcd_drawline(LCDWIDTH/2, LCDHEIGHT/2,
                     xhour[pos], yhour[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2-1,
                     xhour[pos], yhour[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2+1,
                     xhour[pos], yhour[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2+1,
                     xhour[pos], yhour[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2-1,
                     xhour[pos], yhour[pos]);

    /* Draw the circle */
    for(i=0; i < 60; i+=5)
        rb->lcd_fillrect(xminute[i]-1, yminute[i]-1, 3, 3);

    /* Draw the cover over the center */
    rb->lcd_drawline((LCDWIDTH/2)-1, (LCDHEIGHT/2)+3,
                     (LCDWIDTH/2)+1, (LCDHEIGHT/2)+3);
    rb->lcd_drawline((LCDWIDTH/2)-3, (LCDHEIGHT/2)+2,
                     (LCDWIDTH/2)+3, (LCDHEIGHT/2)+2);
    rb->lcd_drawline((LCDWIDTH/2)-4, (LCDHEIGHT/2)+1,
                     (LCDWIDTH/2)+4, (LCDHEIGHT/2)+1);
    rb->lcd_drawline((LCDWIDTH/2)-4, LCDHEIGHT/2,
                     (LCDWIDTH/2)+4, LCDHEIGHT/2);
    rb->lcd_drawline((LCDWIDTH/2)-4, (LCDHEIGHT/2)-1,
                     (LCDWIDTH/2)+4, (LCDHEIGHT/2)-1);
    rb->lcd_drawline((LCDWIDTH/2)-3, (LCDHEIGHT/2)-2,
                     (LCDWIDTH/2)+3, (LCDHEIGHT/2)-2);
    rb->lcd_drawline((LCDWIDTH/2)-1, (LCDHEIGHT/2)-3,
                     (LCDWIDTH/2)+1, (LCDHEIGHT/2)-3);
}

/*************************************************************
 * 7-Segment LED/LCD imitation code, by Linus Nielsen Feltzing
 ************************************************************/
/*
       a     0    b
        #########c
       #         #`
       #         #
      1#         #2
       #         #
       #    3    #
      c ######### d
       #         #
       #         #
      4#         #5
       #         #
       #    6    #
      e ######### f
*/
static unsigned int point_coords[6][2] =
{
    {0, 0}, /* a */
    {1, 0}, /* b */
    {0, 1}, /* c */
    {1, 1}, /* d */
    {0, 2}, /* e */
    {1, 2}  /* f */
};

/********************************************
 * The end points (a-f) for each segment line
 *******************************************/
static unsigned int seg_points[7][2] =
{
    {0,1}, /* a to b */
    {0,2}, /* a to c */
    {1,3}, /* b to d */
    {2,3}, /* c to d */
    {2,4}, /* c to e */
    {3,5}, /* d to f */
    {4,5}  /* e to f */
};

/**********************************************************************
 * Lists that tell which segments (0-6) to enable for each digit (0-9),
 * the list is terminated with -1
 *********************************************************************/
static int digit_segs[10][8] =
{
    {0,1,2,4,5,6, -1},   /* 0 */
    {2,5, -1},           /* 1 */
    {0,2,3,4,6, -1},     /* 2 */
    {0,2,3,5,6, -1},     /* 3 */
    {1,2,3,5, -1},       /* 4 */
    {0,1,3,5,6, -1},     /* 5 */
    {0,1,3,4,5,6, -1},   /* 6 */
    {0,2,5, -1},         /* 7 */
    {0,1,2,3,4,5,6, -1}, /* 8 */
    {0,1,2,3,5,6, -1}    /* 9 */
};

/***********************************
 * Draws one segment - LED imitation
 **********************************/
void draw_seg_led(int seg, int x, int y, int width, int height)
{
    int p1 = seg_points[seg][0];
    int p2 = seg_points[seg][1];
    int x1 = point_coords[p1][0];
    int y1 = point_coords[p1][1];
    int x2 = point_coords[p2][0];
    int y2 = point_coords[p2][1];

    /* It draws parallel lines of different lengths for thicker segments */
    if(seg == 0 || seg == 3 || seg == 6)
    {
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2,
                         x + x2 * width - 1 , y + y2 * height / 2);

        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 - 1,
                         x + x2 * width - 2, y + y2 * height / 2 - 1);
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 + 1,
                         x + x2 * width - 2, y + y2 * height / 2 + 1);

        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 - 2,
                         x + x2 * width - 3, y + y2 * height / 2 - 2);
        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 + 2,
                         x + x2 * width - 3, y + y2 * height / 2 + 2);
    }
    else
    {
        rb->lcd_drawline(x + x1 * width, y + y1 * height / 2 + 1,
                         x + x2 * width , y + y2 * height / 2 - 1);

        rb->lcd_drawline(x + x1 * width - 1, y + y1 * height / 2 + 2,
                         x + x2 * width - 1, y + y2 * height / 2 - 2);
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2 + 2,
                         x + x2 * width + 1, y + y2 * height / 2 - 2);

        rb->lcd_drawline(x + x1 * width - 2, y + y1 * height / 2 + 3,
                         x + x2 * width - 2, y + y2 * height / 2 - 3);

        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 + 3,
                         x + x2 * width + 2, y + y2 * height / 2 - 3);
    }
}

/***********************************
 * Draws one segment - LCD imitation
 **********************************/
void draw_seg_lcd(int seg, int x, int y, int width, int height)
{
    int p1 = seg_points[seg][0];
    int p2 = seg_points[seg][1];
    int x1 = point_coords[p1][0];
    int y1 = point_coords[p1][1];
    int x2 = point_coords[p2][0];
    int y2 = point_coords[p2][1];

    if(seg == 0)
    {
        rb->lcd_drawline(x + x1 * width,     y + y1 * height / 2 - 1,
                         x + x2 * width,     y + y2 * height / 2 - 1);
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2,
                         x + x2 * width - 1, y + y2 * height / 2);
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 + 1,
                         x + x2 * width - 2, y + y2 * height / 2 + 1);
        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 + 2,
                         x + x2 * width - 3, y + y2 * height / 2 + 2);
    }
    else if(seg == 3)
    {
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2,
                         x + x2 * width - 1, y + y2 * height / 2);
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 - 1,
                         x + x2 * width - 2, y + y2 * height / 2 - 1);
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 + 1,
                         x + x2 * width - 2, y + y2 * height / 2 + 1);
        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 - 2,
                         x + x2 * width - 3, y + y2 * height / 2 - 2);
        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 + 2,
                         x + x2 * width - 3, y + y2 * height / 2 + 2);
    }
    else if(seg == 6)
    {
        rb->lcd_drawline(x + x1 * width,     y + y1 * height / 2 + 1,
                         x + x2 * width,     y + y2 * height / 2 + 1);
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2,
                         x + x2 * width - 1, y + y2 * height / 2);
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 - 1,
                         x + x2 * width - 2, y + y2 * height / 2 - 1);
        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 - 2,
                         x + x2 * width - 3, y + y2 * height / 2 - 2);

    }
    else if(seg == 1 || seg == 4)
    {
        rb->lcd_drawline(x + x1 * width - 1, y + y1 * height / 2,
                         x + x2 * width - 1, y + y2 * height / 2);
        rb->lcd_drawline(x + x1 * width,     y + y1 * height / 2 + 1,
                         x + x2 * width,     y + y2 * height / 2 - 1);
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2 + 2,
                         x + x2 * width + 1, y + y2 * height / 2 - 2);
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 + 3,
                         x + x2 * width + 2, y + y2 * height / 2 - 3);
    }
    else if(seg == 2 || seg == 5)
    {
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2,
                         x + x2 * width + 1, y + y2 * height / 2);
        rb->lcd_drawline(x + x1 * width,     y + y1 * height / 2 + 1,
                         x + x2 * width,     y + y2 * height / 2 - 1);
        rb->lcd_drawline(x + x1 * width - 1, y + y1 * height / 2 + 2,
                         x + x2 * width - 1, y + y2 * height / 2 - 2);
        rb->lcd_drawline(x + x1 * width - 2, y + y1 * height / 2 + 3,
                         x + x2 * width - 2, y + y2 * height / 2 - 3);
    }
}

/*****************
 * Draws one digit
 ****************/
void draw_7seg_digit(int digit, int x, int y, int width, int height, bool lcd_display)
{
    int i;
    int c;

    for(i = 0;digit_segs[digit][i] >= 0;i++)
    {
        c = digit_segs[digit][i];

        if(!lcd_display)
            draw_seg_led(c, x, y, width, height);
        else
            draw_seg_lcd(c, x, y, width, height);
    }
}

/*****************************************************
 * Draws the entire 7-segment hour-minute time display
 ****************************************************/
void draw_7seg_time(int hour, int minute, int x, int y, int width, int height,
bool colon, bool lcd)
{
    int xpos = x;

    /* Draw AM/PM indicator */
    if(settings.clock == DIGITAL)
    {
        if(settings.digital[digital_12h])
        {
            if(hour > 12)
                rb->lcd_mono_bitmap(pm, 97, 55, 15, 8);
            else
                rb->lcd_mono_bitmap(am, 1, 55, 15, 8);
        }
    }
    else
    {
        if(settings.lcd[lcd_12h])
        {
            if(hour > 12)
                rb->lcd_mono_bitmap(pm, 97, 55, 15, 8);
            else
                rb->lcd_mono_bitmap(am, 1, 55, 15, 8);
        }
    }

    /* Now change to 12H mode if requested */
    if(settings.clock == DIGITAL)
    {
        if(settings.digital[digital_12h])
        {
            if(hour >= 12)
                hour -= 12;
        }
    }
    else
    {
        if(settings.lcd[lcd_12h])
        {
            if(hour >= 12)
                hour -= 12;
        }
    }

    draw_7seg_digit(hour / 10, xpos, y, width, height, lcd);
    xpos += width + 6;
    draw_7seg_digit(hour % 10, xpos, y, width, height, lcd);
    xpos += width + 6;

    if(colon)
    {
        rb->lcd_mono_bitmap(digital_colon, xpos, y + height-height/3, 5, 5);
        rb->lcd_mono_bitmap(digital_colon, xpos, y + height/3, 5, 5);
    }

    xpos += 12;

    draw_7seg_digit(minute / 10, xpos, y, width, height, lcd);
    xpos += width + 6;
    draw_7seg_digit(minute % 10, xpos, y, width, height, lcd);
    xpos += width + 6;
}

/***********************
 * Fullscreen clock mode
 **********************/
void fullscreen_clock(int hour, int minute, int second)
{
    int pos;

    /* Second hand */
    if(settings.fullscreen[fullscreen_secondhand])
    {
        pos = 90-second;
        if(pos >= 60)
            pos -= 60;

        rb->lcd_drawline((LCDWIDTH/2), (LCDHEIGHT/2),
                         xminute_full[pos], yminute_full[pos]);
    }

    pos = 90-minute;
    if(pos >= 60)
        pos -= 60;

    /* Minute hand, thicker than the second hand */
    rb->lcd_drawline(LCDWIDTH/2, LCDHEIGHT/2,
                     xminute_full[pos], yminute_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2-1,
                     xminute_full[pos], yminute_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2+1,
                     xminute_full[pos], yminute_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2+1,
                     xminute_full[pos], yminute_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2-1,
                     xminute_full[pos], yminute_full[pos]);

    if(hour > 12)
        hour -= 12;

    hour = hour*5 + minute/12;
    pos = 90-hour;
    if(pos >= 60)
        pos -= 60;

    /* Hour hand, thick as the minute hand but shorter */
    rb->lcd_drawline(LCDWIDTH/2, LCDHEIGHT/2, xhour_full[pos], yhour_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2-1,
                     xhour_full[pos], yhour_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2+1,
                     xhour_full[pos], yhour_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2-1, LCDHEIGHT/2+1,
                     xhour_full[pos], yhour_full[pos]);
    rb->lcd_drawline(LCDWIDTH/2+1, LCDHEIGHT/2-1,
                     xhour_full[pos], yhour_full[pos]);

    /* Draw the cover over the center */
    rb->lcd_drawline((LCDWIDTH/2)-1, (LCDHEIGHT/2)+3,
                     (LCDWIDTH/2)+1, (LCDHEIGHT/2)+3);
    rb->lcd_drawline((LCDWIDTH/2)-3, (LCDHEIGHT/2)+2,
                     (LCDWIDTH/2)+3, (LCDHEIGHT/2)+2);
    rb->lcd_drawline((LCDWIDTH/2)-4, (LCDHEIGHT/2)+1,
                     (LCDWIDTH/2)+4, (LCDHEIGHT/2)+1);
    rb->lcd_drawline((LCDWIDTH/2)-4, LCDHEIGHT/2,
                     (LCDWIDTH/2)+4, LCDHEIGHT/2);
    rb->lcd_drawline((LCDWIDTH/2)-4, (LCDHEIGHT/2)-1,
                     (LCDWIDTH/2)+4, (LCDHEIGHT/2)-1);
    rb->lcd_drawline((LCDWIDTH/2)-3, (LCDHEIGHT/2)-2,
                     (LCDWIDTH/2)+3, (LCDHEIGHT/2)-2);
    rb->lcd_drawline((LCDWIDTH/2)-1, (LCDHEIGHT/2)-3,
                     (LCDWIDTH/2)+1, (LCDHEIGHT/2)-3);
}

/*******************
 * Binary clock mode
 ******************/
void binary_clock(int hour, int minute, int second)
{
    int i, xpos=0;
    int mode_var[3]; /* pointers to h, m, s arguments */
    int mode; /* 0 = hour, 1 = minute, 2 = second */

    mode_var[0] = hour;
    mode_var[1] = minute;
    mode_var[2] = second;

    for(mode = 0; mode < 3; mode++)
    {
        for(i = 32; i > 0; i /= 2)
        {
            if(mode_var[mode] >= i)
            {
                if(settings.binary[binary_dots])
                    rb->lcd_mono_bitmap(circle_full, xpos*19, (20*mode)+1, 14, 14);
                else
                    rb->lcd_mono_bitmap(bitmap_1, xpos*19, (20*mode)+1, 15, 20);
                mode_var[mode] -= i;
            }
            else
            {
                if(settings.binary[binary_dots])
                    rb->lcd_mono_bitmap(circle_empty, xpos*19, (20*mode)+1, 14, 14);
                else
                    rb->lcd_mono_bitmap(bitmap_0, xpos*19, (20*mode)+1, 15, 20);
            }

            xpos++;
        }

        xpos=0; /* reset the x-pos for next mode */
    }
}

/******************
 * Plain clock mode
 *****************/
void plain_clock(int hour, int minute, int second, bool colon)
{
    int x_offset=0;

    if(settings.plain[plain_12h])
    {
        if(hour > 12)
            rb->lcd_mono_bitmap(pm, 97, 10, 15, 8);
        else
            rb->lcd_mono_bitmap(am, 97, 10, 15, 8);

        if(hour > 12)
            hour -= 12;
        if(hour == 0)
            hour = 12;
    }

    if(settings.plain[plain_12h]) /* scoot the display over for the am/pm bitmap */
        x_offset = -10;

    rb->lcd_mono_bitmap(plain_bitmaps[hour/10], 10+x_offset, 0, 17, 28);
    rb->lcd_mono_bitmap(plain_bitmaps[hour%10], 30+x_offset, 0, 17, 28);

    if(colon)
        rb->lcd_mono_bitmap(plain_colon, 50+x_offset, 0, 5, 28);

    rb->lcd_mono_bitmap(plain_bitmaps[minute/10], 60+x_offset, 0, 17, 28);
    rb->lcd_mono_bitmap(plain_bitmaps[minute%10], 80+x_offset, 0, 17, 28);

    rb->lcd_mono_bitmap(plain_bitmaps[second/10], 70, 32, 17, 28);
    rb->lcd_mono_bitmap(plain_bitmaps[second%10], 90, 32, 17, 28);
}

/****************
 * Shows the logo
 ***************/
void show_clock_logo(bool animate, bool show_clock_text)
{
    int y_position;

    if(animate) /* animate logo */
    {
        /* move down the screen */
        for(y_position = -74; y_position <= 20; y_position+=(40-y_position)/20)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 0, 112, 48);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_mono_bitmap(clocklogo, 0, y_position/2, 112, 37);
            if(show_clock_text)
                center_text(48, "Clock " CLOCK_VERSION);
            rb->lcd_update();
        }
    }
    else /* don't animate, just show */
    {
        rb->lcd_mono_bitmap(clocklogo, 0, 10, 112, 37);
        if(show_clock_text)
            center_text(48, "Clock " CLOCK_VERSION);
        rb->lcd_update();
    }
}

/********************
 * Logo flies off lcd
 *******************/
void exit_logo()
{
    int y_position;

    for(y_position = 20; y_position <= 128; y_position+=y_position/20)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 10, 112, (y_position/2));
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_mono_bitmap(clocklogo, 0, y_position/2, 112, 37);
        rb->lcd_update();
    }
}

/*******************
 * Rolls the credits
 ******************/
/* The following function is pretty confusing, so it's extra well commented. */
bool roll_credits(void)
{
    int j=0, namepos, jobpos; /* namepos/jobpos are x coords for strings of text */
    int offset_dummy;
    int btn, pause;
    int numnames = 12; /* amount of people in the credits array */

    /* used to center the text */
    char name[20], job[15];
    int name_w, name_h, job_w, job_h;
    int credits_w, credits_h, credits_pos;
    int name_targetpos, job_targetpos, credits_targetpos;

    /* shows "[Credits] XX/XX" */
    char elapsednames[16];

    /* put text into variable, and save the width and height of the text */
    rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %02d/%02d",
                 j+1, numnames);
    rb->lcd_getstringsize(elapsednames, &credits_w, &credits_h);
    credits_targetpos = (LCDWIDTH/2)-(credits_w/2);

    /* fly in text from the left */
    for(credits_pos = 0 - credits_w; credits_pos <= credits_targetpos;
        credits_pos += (credits_targetpos-credits_pos + 14) / 7)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_drawline(credits_pos-1, 0, credits_pos-1, 8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update(); /* update the whole lcd to slow down the loop */
    }

    /* now roll the credits */
    for(j=0; j < numnames; j++)
    {
        rb->lcd_clear_display();

        show_clock_logo(false, false);

        rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %02d/%02d",
                     j+1, numnames);
        rb->lcd_putsxy(credits_pos-1, 0, elapsednames);

        /* used to center the text */
        rb->snprintf(name, sizeof(name), "%s", credits[j]);
        rb->snprintf(job, sizeof(job), "%s", jobs[j]);
        rb->lcd_getstringsize(name, &name_w, &name_h);
        rb->lcd_getstringsize(job, &job_w, &job_h);

        name_targetpos = -10;
        job_targetpos = (LCDWIDTH/2)-(job_w/2)+10;

        /* line 1 flies in */
        for(namepos = 0-name_w; namepos <= name_targetpos;
            namepos += (name_targetpos - namepos + 14) / 7)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 48, 112, 8); /* clear any trails left behind */
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, 48, name);
            rb->lcd_update();

            /* exit on keypress */
            btn = rb->button_get(false);
            if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                return false;
        }

        /* line 2 flies in - we use (job_w+2) to ensure it fits on the LCD */
        for(jobpos = LCDWIDTH; jobpos >= job_targetpos;
            jobpos -= (jobpos - job_targetpos + 14) / 7, namepos++)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 48, 112+job_w, 16); /* clear trails */
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, 48, name);
            rb->lcd_putsxy(jobpos, 56, job);
            rb->lcd_update();

            /* exit on keypress */
            btn = rb->button_get(false);
            if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                return false;
        }

        /* pause and scan for button presses */
        for(pause = 0; pause < 30; pause++)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 48, 112, 16);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, 48, name);
            rb->lcd_putsxy(jobpos, 56, job);
            rb->lcd_update();

            btn = rb->button_get(false);
            if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                return false;

            namepos++;
            jobpos--;

            rb->sleep(HZ/20); /* slight pause */
        }

        offset_dummy = 1;

        /* fly out both lines at same time */
        while(namepos<LCDWIDTH+10 || jobpos > 0-job_w)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 48, 112, 16); /* clear trails */
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, 48, name);
            rb->lcd_putsxy(jobpos, 56, job);
            rb->lcd_update();

            /* exit on keypress */
            btn = rb->button_get(false);
            if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                return false;

            namepos += offset_dummy;
            jobpos -= offset_dummy;

            offset_dummy++;
        }

        /* pause (.5s) */
        rb->sleep(HZ/2);

        /* and scan for button presses */
        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
             return false;
    }

    offset_dummy = 1;

    /* now make the text exit to the right */
    for(credits_pos = (LCDWIDTH/2)-(credits_w/2); credits_pos <= 122;
        credits_pos += offset_dummy, offset_dummy++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, 112, 8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update();
    }

    exit_logo();

    return true;
}

/****************************************
 * Shows the logo, then rolls the credits
 ***************************************/
bool show_credits(void)
{
    int j = 0;
    int btn;

    rb->lcd_clear_display();

    center_text(56, "Credits");

    /* show the logo with an animation and the clock version text */
    show_clock_logo(true, true);

    rb->lcd_update();

    /* pause while button scanning */
    for (j = 0; j < 5; j++)
    {
        rb->sleep(HZ/5);

        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
            return false;
    }

    roll_credits(); /* then roll the credits */

    return false;
}

/**************
 * Draws cursor
 *************/
void cursor(int x, int y, int w, int h)
{
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_fillrect(x, y, w, h);

    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_drawpixel(x, y);
    rb->lcd_drawpixel(x+w-1, y);
    rb->lcd_drawpixel(x, y+h-1);
    rb->lcd_drawpixel(x+w-1, y+h-1);

    rb->lcd_set_drawmode(DRMODE_SOLID);
}

/*************
 * Help screen
 ************/
bool help_screen(void)
{
    int screen = 1;
    done = false;

    while (!done)
    {
        rb->lcd_clear_display();

        if(screen == 1)
            center_text(56, "------ 1/2 NEXT>>");
        else if(screen == 2)
            center_text(56, "<<BACK 2/2 ------");

        if(screen == 1) /* page one */
        {
            rb->lcd_puts(0, 0, "Help - Clock " CLOCK_VERSION ":");
            rb->lcd_puts(0, 2, "To navigate this");
            rb->lcd_puts(0, 3, "help, use " NAVI_BUTTON_TEXT_LEFT " and");
            rb->lcd_puts(0, 4, NAVI_BUTTON_TEXT_RIGHT ". "
                               EXIT_BUTTON_TEXT " returns");
            rb->lcd_puts(0, 5, "you to the clock.");
            rb->lcd_puts(0, 6, "In any mode, " MENU_BUTTON_TEXT);
        }
        else if(screen == 2) /* page two */
        {
            rb->lcd_puts(0, 0, "will show you the");
            rb->lcd_puts(0, 1, "main menu. " COUNTER_BUTTON_TEXT " will");
            rb->lcd_puts(0, 2, "start/stop counter.");
            rb->lcd_puts(0, 3, "Hold " COUNTER_BUTTON_TEXT " to reset");
            rb->lcd_puts(0, 4, "counter. " EXIT_BUTTON_TEXT " exits");
            rb->lcd_puts(0, 5, "any screen or the");
            rb->lcd_puts(0, 6, "clock itself.");
        }

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
            case EXIT_BUTTON:
                done = true;
                break;

            case CHANGE_DOWN_BUTTON:
                if(screen > 1)
                    screen --;
                break;

            case CHANGE_UP_BUTTON:
                if(screen < 2)
                    screen++;
                break;
        }
    }
    return true;
}

/*************************
 * Draws a checkbox bitmap
 ************************/
void draw_checkbox(int setting, int startnum, int numsettings, int x, int y)
{
    if(setting) /* checkbox is on */
        rb->lcd_mono_bitmap(checkbox_full, x, y, 8, 6);
    else /* checkbox is off */
        rb->lcd_mono_bitmap(checkbox_empty, x, y, 8, 6);

    if(numsettings-startnum == 2)
    {
        if(setting == 0+startnum)
            rb->lcd_mono_bitmap(checkbox_empty, x, y, 8, 6);
        else if(setting == 1+startnum)
            rb->lcd_mono_bitmap(checkbox_full, x, y, 8, 6);
    }
    else if(numsettings-startnum == 3)
    {
        if(setting == 0+startnum)
            rb->lcd_mono_bitmap(checkbox_empty, x, y, 8, 6);
        else if(setting == 1+startnum)
            rb->lcd_mono_bitmap(checkbox_half, x, y, 8, 6);
        else if(setting == 2+startnum)
            rb->lcd_mono_bitmap(checkbox_full, x, y, 8, 6);
    }
    else if(numsettings-startnum == 4)
    {
        if(setting == 0+startnum)
            rb->lcd_mono_bitmap(checkbox_empty, x, y, 8, 6);
        else if(setting == 1+startnum)
            rb->lcd_mono_bitmap(checkbox_onethird, x, y, 8, 6);
        else if(setting == 2+startnum)
            rb->lcd_mono_bitmap(checkbox_twothird, x, y, 8, 6);
        else if(setting == 3+startnum)
            rb->lcd_mono_bitmap(checkbox_full, x, y, 8, 6);
    }
}

/**************************************
 * Settings screen for the current mode
 *************************************/
void draw_settings(void)
{
    if(settings.clock == ANALOG)
    {
        rb->lcd_puts(2, 0, analog_digits_text);
        rb->lcd_puts(2, 1, analog_date_text[settings.analog[analog_date]]);
        rb->lcd_puts(2, 2, analog_secondhand_text);
        rb->lcd_puts(2, 3, analog_time_text[settings.analog[analog_time]]);

        /* Draw checkboxes */
        draw_checkbox(settings.analog[analog_digits], 0, 1, 1, 1);
        draw_checkbox(settings.analog[analog_date], 0, 3, 1, 9);
        draw_checkbox(settings.analog[analog_secondhand], 0, 1, 1, 17);
        draw_checkbox(settings.analog[analog_time], 0, 3, 1, 25);
    }
    else if(settings.clock == DIGITAL)
    {
        rb->lcd_puts(2, 0, digital_seconds_text[settings.digital[digital_seconds]]);
        rb->lcd_puts(2, 1, digital_date_text[settings.digital[digital_date]]);
        rb->lcd_puts(2, 2, digital_blinkcolon_text);
        rb->lcd_puts(2, 3, digital_12h_text);

        draw_checkbox(settings.digital[digital_seconds], 0, 4, 1, 1);
        draw_checkbox(settings.digital[digital_date], 0, 3, 1, 9);
        draw_checkbox(settings.digital[digital_blinkcolon], 0, 1, 1, 17);
        draw_checkbox(settings.digital[digital_12h], 0, 1, 1, 25);
    }
    else if(settings.clock == LCD)
    {
        rb->lcd_puts(2, 0, lcd_seconds_text[settings.lcd[lcd_seconds]]);
        rb->lcd_puts(2, 1, lcd_date_text[settings.lcd[lcd_date]]);
        rb->lcd_puts(2, 2, lcd_blinkcolon_text);
        rb->lcd_puts(2, 3, lcd_12h_text);

        draw_checkbox(settings.lcd[lcd_seconds], 0, 4, 1, 1);
        draw_checkbox(settings.lcd[lcd_date], 0, 3, 1, 9);
        draw_checkbox(settings.lcd[lcd_blinkcolon], 0, 1, 1, 17);
        draw_checkbox(settings.lcd[lcd_12h], 0, 1, 1, 25);
    }
    else if(settings.clock == FULLSCREEN)
    {
        rb->lcd_puts(2, 0, fullscreen_border_text);
        rb->lcd_puts(2, 1, fullscreen_secondhand_text);
        rb->lcd_puts(2, 2, fullscreen_invertseconds_text);

        draw_checkbox(settings.fullscreen[fullscreen_border], 0, 1, 1, 1);
        draw_checkbox(settings.fullscreen[fullscreen_secondhand], 0, 1, 1, 9);
        draw_checkbox(settings.fullscreen[fullscreen_invertseconds], 0, 1, 1, 17);
    }
    else if(settings.clock == BINARY)
    {
        rb->lcd_puts(2, 0, binary_dots_text);

        draw_checkbox(settings.binary[binary_dots], 0, 2, 1, 1);
    }
    else if(settings.clock == PLAIN)
    {
        rb->lcd_puts(2, 0, plain_12h_text);
        rb->lcd_puts(2, 1, plain_date_text[settings.plain[plain_date]]);
        rb->lcd_puts(2, 2, plain_blinkcolon_text);

        draw_checkbox(settings.plain[plain_12h], 0, 1, 1, 1);
        draw_checkbox(settings.plain[plain_date], 0, 3, 1, 9);
        draw_checkbox(settings.plain[plain_blinkcolon], 0, 1, 1, 17);
    }
}

/***********************************
 * Change a given setting up or down
 **********************************/
void change_setting(int setting, int ofs, bool general_settings)
{
    if(ofs == 1)
    {
        if(general_settings)
        {
            if(settings.general[setting-3] < general_max[setting-3])
                settings.general[setting-3]++;
        }
        else
        {
            if(settings.clock == ANALOG)
            {
                if(settings.analog[setting] < analog_max[setting])
                    settings.analog[setting]++;
            }
            else if(settings.clock == DIGITAL)
            {
                if(settings.digital[setting] < digital_max[setting])
                    settings.digital[setting]++;
            }
            else if(settings.clock == LCD)
            {
                if(settings.lcd[setting] < digital_max[setting])
                    settings.lcd[setting]++;
            }
            else if(settings.clock == FULLSCREEN)
            {
                if(settings.fullscreen[setting] < fullscreen_max)
                    settings.fullscreen[setting]++;
            }
            else if(settings.clock == BINARY)
            {
                if(settings.binary[setting] < binary_max)
                    settings.binary[setting]++;
            }
            else if(settings.clock == PLAIN)
            {
                if(settings.plain[setting] < plain_max[setting])
                    settings.plain[setting]++;
            }
        }
    }
    else if(ofs == -1)
    {
        if(general_settings)
        {
            if(settings.general[setting-3] > 0)
                settings.general[setting-3]--;
        }
        else
        {
            if(settings.clock == ANALOG)
            {
                if(settings.analog[setting] > 0)
                    settings.analog[setting]--;
            }
            else if(settings.clock == DIGITAL)
            {
                if(settings.digital[setting] > 0)
                    settings.digital[setting]--;
            }
            else if(settings.clock == LCD)
            {
                if(settings.lcd[setting] > 0)
                    settings.lcd[setting]--;
            }
            else if(settings.clock == FULLSCREEN)
            {
                if(settings.fullscreen[setting] > 0)
                    settings.fullscreen[setting]--;
            }
            else if(settings.clock == BINARY)
            {
                if(settings.binary[setting] > 0)
                    settings.binary[setting]--;
            }
            else if(settings.clock == PLAIN)
            {
                if(settings.plain[setting] > 0)
                    settings.plain[setting]--;
            }
        }
    }
}

/**************************************
 * Settings screen for the current mode
 *************************************/
void settings_screen(void)
{
    /* cursor positions */
    int cursorpos=1,cursor_y,cursor_dummy;

    int mode_numsettings[6] = {ANALOG_SETTINGS, DIGITAL_SETTINGS, LCD_SETTINGS,
                               FULLSCREEN_SETTINGS, BINARY_SETTINGS, PLAIN_SETTINGS};

    done = false;

    while (!done)
    {
        rb->lcd_clear_display();

        draw_settings();

        cursor(0, 8*(cursorpos-1), 112, 8);

        switch(rb->button_get_w_tmo(HZ/8))
        {
            case MOVE_UP_BUTTON:
                if(cursorpos > 1)
                {
                    cursor_y = (8*(cursorpos-1));
                    cursor_dummy = cursor_y;
                    for(; cursor_y>=cursor_dummy-8; cursor_y-=2)
                    {
                        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        rb->lcd_fillrect(0, 8, 112, 56);
                        rb->lcd_set_drawmode(DRMODE_SOLID);
                        draw_settings();
                        cursor(0, cursor_y, 112, 8);
                        rb->lcd_update();
                    }
                    cursorpos--;
                }
                break;

            case MOVE_DOWN_BUTTON:
                if(cursorpos < mode_numsettings[settings.clock-1])
                {
                    cursor_y = (8*(cursorpos-1));
                    cursor_dummy = cursor_y;
                    for(; cursor_y<=cursor_dummy+8; cursor_y+=2)
                    {
                        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        rb->lcd_fillrect(0, 8, 112, 56);
                        rb->lcd_set_drawmode(DRMODE_SOLID);
                        draw_settings();
                        cursor(0, cursor_y, 112, 8);
                        rb->lcd_update();
                    }
                    cursorpos++;
                }
                break;

            case CHANGE_DOWN_BUTTON:
                change_setting(cursorpos-1, -1, false);
                break;


            case CHANGE_UP_BUTTON:
                change_setting(cursorpos-1, 1, false);
                break;

            case EXIT_BUTTON:
            case MENU_BUTTON:
                done = true;
                break;
        }

        rb->lcd_update();
    }
}

/***********************************************************
 * Confirm resetting of settings, used in general_settings()
 **********************************************************/
void confirm_reset(void)
{
    bool ask_reset_done = false;
    char play_text[20];

    rb->snprintf(play_text, sizeof(play_text), "%s = Yes", YESTEXT);

    while(!ask_reset_done)
    {
        rb->lcd_clear_display();

        center_text(0, "Reset Settings?");
        rb->lcd_puts(0, 2, play_text);
        rb->lcd_puts(0, 3, "Any Other = No");

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
            case MENU_BUTTON:
                reset_settings();
                rb->splash(HZ*2, true, "Settings Reset!");
                ask_reset_done = true;
                break;

            case COUNTER_TOGGLE_BUTTON:
#if CONFIG_KEYPAD == RECORDER_PAD /* dupes or plain annoying on the ipod touchpad */
            case MOVE_DOWN_BUTTON:
            case MOVE_UP_BUTTON:
#endif
            case CHANGE_DOWN_BUTTON:
            case CHANGE_UP_BUTTON:
            case EXIT_BUTTON:
                ask_reset_done = true;
                break;
        }
    }
}

/************************************
 * General settings. Reset, save, etc
 ***********************************/
void general_settings(void)
{
    int cursorpos=1,cursor_y,cursor_dummy;
    done = false;

    while(!done)
    {
        rb->lcd_clear_display();

        center_text(0, "General Settings");
        rb->lcd_puts(2, 1, general_reset_text);
        rb->lcd_puts(2, 2, general_save_text);
        rb->lcd_puts(2, 3, general_counter_text);
        rb->lcd_puts(2, 4, general_savesetting_text[settings.general[general_savesetting]]);
        rb->lcd_puts(2, 5, general_backlight_text[settings.general[general_backlight]]);
        rb->lcd_puts(2, 6, general_idle_text);

        rb->lcd_mono_bitmap(arrow, 1, 9, 8, 6);
        rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);

        draw_checkbox(settings.general[general_counter], 0, 1, 1, 25);
        draw_checkbox(settings.general[general_savesetting], 0, 3, 1, 33);
        draw_checkbox(settings.general[general_backlight], 0, 3, 1, 41);
        draw_checkbox(idle_poweroff, 0, 1, 1, 49);

        cursor(0, cursorpos*8, 112, 8);

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
            case EXIT_BUTTON:
            case MENU_BUTTON:
                if(settings.general[general_savesetting] == 2)
                    save_settings(false);

                /* set backlight timeout */
                if(settings.general[general_backlight] == 0)
                    rb->backlight_set_timeout(-1);
                else if(settings.general[general_backlight] == 1)
                    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
                else if(settings.general[general_backlight] == 2)
                    rb->backlight_set_timeout(1);

                done = true;
                break;

            case MOVE_UP_BUTTON:
                if(cursorpos > 1)
                {
                    cursor_y = 8+(8*(cursorpos-1));
                    cursor_dummy = cursor_y;
                    for(; cursor_y>cursor_dummy-8; cursor_y-=2)
                    {
                        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        rb->lcd_fillrect(0, 8, 112, 56);
                        rb->lcd_set_drawmode(DRMODE_SOLID);

                        rb->lcd_puts(2, 1, general_reset_text);
                        rb->lcd_puts(2, 2, general_save_text);
                        rb->lcd_puts(2, 3, general_counter_text);
                        rb->lcd_puts(2, 4, general_savesetting_text[settings.general[general_savesetting]]);
                        rb->lcd_puts(2, 5, general_backlight_text[settings.general[general_backlight]]);
                        rb->lcd_puts(2, 6, general_idle_text);

                        rb->lcd_mono_bitmap(arrow, 1, 9, 8, 6);
                        rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);

                        draw_checkbox(settings.general[general_counter], 0, 1, 1, 25);
                        draw_checkbox(settings.general[general_savesetting], 0, 3, 1, 33);
                        draw_checkbox(settings.general[general_backlight], 0, 3, 1, 41);
                        draw_checkbox(idle_poweroff, 0, 1, 1, 49);

                        cursor(0, cursor_y, 112, 8);
                        rb->lcd_update();
                    }
                    cursorpos--;
                }
                break;

            case MOVE_DOWN_BUTTON:
                if(cursorpos < 6)
                {
                    cursor_y = 8+(8*(cursorpos-1));
                    cursor_dummy = cursor_y;
                    for(; cursor_y<cursor_dummy+8; cursor_y+=2)
                    {
                        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        rb->lcd_fillrect(0, 8, 112, 56);
                        rb->lcd_set_drawmode(DRMODE_SOLID);

                        rb->lcd_puts(2, 1, general_reset_text);
                        rb->lcd_puts(2, 2, general_save_text);
                        rb->lcd_puts(2, 3, general_counter_text);
                        rb->lcd_puts(2, 4, general_savesetting_text[settings.general[general_savesetting]]);
                        rb->lcd_puts(2, 5, general_backlight_text[settings.general[general_backlight]]);
                        rb->lcd_puts(2, 6, general_idle_text);

                        rb->lcd_mono_bitmap(arrow, 1, 9, 8, 6);
                        rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);

                        draw_checkbox(settings.general[general_counter], 0, 1, 1, 25);
                        draw_checkbox(settings.general[general_savesetting], 0, 3, 1, 33);
                        draw_checkbox(settings.general[general_backlight], 0, 3, 1, 41);
                        draw_checkbox(idle_poweroff, 0, 1, 1, 49);

                        cursor(0, cursor_y, 112, 8);
                        rb->lcd_update();
                    }
                    cursorpos++;
                }
                break;

            case CHANGE_DOWN_BUTTON:
                if(cursorpos == 1 || cursorpos == 2)
                    done = true;
                if(cursorpos >= 3 && cursorpos <= 5)
                {
                    change_setting(cursorpos, -1, true);
                    if(cursorpos == 4)
                        save_settings(false);
                }
                else if(cursorpos == 6)
                    idle_poweroff = false;
                break;

            case CHANGE_UP_BUTTON:
                if(cursorpos == 1)
                    confirm_reset();
                else if(cursorpos == 2)
                    save_settings(false);
                else if(cursorpos >= 3 && cursorpos <= 5)
                {
                    change_setting(cursorpos, 1, true);
                    if(cursorpos == 4)
                        save_settings(false);
                }
                else if(cursorpos == 6)
                    idle_poweroff = true;
                break;
        }
    }
}

/****************************************
 * Draws the extras, IE border, digits...
 ***************************************/
void draw_extras(int year, int day, int month, int hour, int minute, int second)
{
    char buf[11];
    int w, h;
    int i;

    struct tm* current_time = rb->get_time();

    int fill = LCDWIDTH * second / 60;

    char moday[8];
    char dateyr[6];
    char tmhrmin[7];
    char tmsec[3];

    /* american date readout */
    if(settings.analog[analog_date] == 1)
        rb->snprintf(moday, sizeof(moday), "%02d/%02d", month, day);
    else
        rb->snprintf(moday, sizeof(moday), "%02d.%02d", day, month);
    rb->snprintf(dateyr, sizeof(dateyr), "%d", year);
    rb->snprintf(tmhrmin, sizeof(tmhrmin), "%02d:%02d", hour, minute);
    rb->snprintf(tmsec, sizeof(tmsec), "%02d", second);

    /* Analog Extras */
    if(settings.clock == ANALOG)
    {
        if(settings.analog[analog_digits]) /* Digits around the face */
        {
            rb->lcd_putsxy((LCDWIDTH/2)-6, 0, "12");
            rb->lcd_putsxy(20, (LCDHEIGHT/2)-4, "9");
            rb->lcd_putsxy((LCDWIDTH/2)-4, 56, "6");
            rb->lcd_putsxy(86, (LCDHEIGHT/2)-4, "3");
        }
        if(settings.analog[analog_time] != 0) /* Digital readout */
        {
            /* HH:MM */
            rb->lcd_putsxy(1, 4, tmhrmin);
            /* SS */
            rb->lcd_putsxy(10, 12, tmsec);

            /* AM/PM indicator */
            if(settings.analog[analog_time] == 2)
            {
                if(current_time->tm_hour > 12) /* PM */
                    rb->lcd_mono_bitmap(pm, 96, 1, 15, 8);
                else /* AM */
                    rb->lcd_mono_bitmap(am, 96, 1, 15, 8);
            }
        }
        if(settings.analog[analog_date] != 0) /* Date readout */
        {
            /* MM-DD (or DD.MM) */
            rb->lcd_putsxy(1, 48, moday);
            rb->lcd_putsxy(3, 56, dateyr);
        }
    }
    else if(settings.clock == DIGITAL)
    {
        /* Date readout */
        if(settings.digital[digital_date] == 1) /* American mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d/%d/%d", month, day, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCDWIDTH/2)-(w/2), 56, buf);
        }
        else if(settings.digital[digital_date] == 2) /* European mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d.%d.%d", day, month, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCDWIDTH/2)-(w/2), 56, buf);
        }
        if(settings.digital[digital_seconds] == 1) /* Second readout */
        {
            rb->snprintf(buf, sizeof(buf), "%d", second);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCDWIDTH/2)-(w/2), 5, buf);
        }
        else if(settings.digital[digital_seconds] == 2) /* Second progressbar */
            rb->scrollbar(0, 0, 112, 4, 60, 0, second, HORIZONTAL);
        else if(settings.digital[digital_seconds] == 3) /* Invert the LCD as seconds pass */
        {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(0, 0, fill, 64);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
    }
    else if(settings.clock == LCD) /* LCD mode */
    {
        /* Date readout */
        if(settings.lcd[lcd_date] == 1) /* american mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d/%d/%d", month, day, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCDWIDTH/2)-(w/2), 56, buf);
        }
        else if(settings.lcd[lcd_date] == 2) /* european mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d.%d.%d", day, month, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCDWIDTH/2)-(w/2), 56, buf);
        }
        if(settings.lcd[lcd_seconds] == 1) /* Second readout */
        {
            rb->snprintf(buf, sizeof(buf), "%d", second);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCDWIDTH/2)-(w/2), 5, buf);
        }
        else if(settings.lcd[lcd_seconds] == 2) /* Second progressbar */
        {
            rb->scrollbar(0, 0, 112, 4, 60, 0, second, HORIZONTAL);
        }
        else if(settings.lcd[lcd_seconds] == 3) /* Invert the LCD as seconds pass */
        {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(0, 0, fill, 64);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
    }
    else if(settings.clock == FULLSCREEN) /* Fullscreen mode */
    {
        if(settings.fullscreen[fullscreen_border])
        {
            for(i=0; i < 60; i+=5) /* Draw the circle */
                rb->lcd_fillrect(xminute_full[i]-1, yminute_full[i]-1, 3, 3);
        }
        if(settings.fullscreen[fullscreen_invertseconds]) /* Invert the LCD as seconds pass */
        {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(0, 0, fill, 64);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
    }
    else if(settings.clock == PLAIN) /* Plain mode */
    {
        /* Date readout */
        if(settings.plain[plain_date] == 1) /* american mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d/%d/%d", month, day, year);
            rb->lcd_putsxy(0, 38, buf);
        }
        else if(settings.plain[plain_date] == 2) /* european mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d.%d.%d", day, month, year);
            rb->lcd_putsxy(0, 38, buf);
        }
    }
}

/***************
 * Select a mode
 **************/
void select_mode(void)
{
    int cursorpos = settings.clock;
    int cursor_dummy, cursor_y;
    int i;

    done = false;

    while(!done)
    {
        rb->lcd_clear_display();

        center_text(0, "Mode Selector");
        for(i=0; i<6; i++)
        {
            rb->lcd_puts(2, i+1, mode_selector_entries[i]);
            rb->lcd_mono_bitmap(arrow, 1, 8*(i+1)+1, 8, 6);
        }

        cursor(0, 8*cursorpos, 112, 8); /* draw cursor */

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
                case MOVE_UP_BUTTON:
                    if(cursorpos > 1)
                    {
                        cursor_y = 8+(8*(cursorpos-1));
                        cursor_dummy = cursor_y;
                        for(; cursor_y>cursor_dummy-8; cursor_y-=2)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 8, 112, 56);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            for(i=0; i<6; i++)
                            {
                                rb->lcd_puts(2, i+1, mode_selector_entries[i]);
                                rb->lcd_mono_bitmap(arrow, 1, 8*(i+1)+1, 8, 6);
                            }

                            cursor(0, cursor_y, 112, 8);
                            rb->lcd_update();
                        }
                        cursorpos--;
                    }
                    break;

                case MOVE_DOWN_BUTTON:
                    if(cursorpos < 6)
                    {
                        cursor_y = 8+(8*(cursorpos-1));
                        cursor_dummy = cursor_y;
                        for(; cursor_y<cursor_dummy+8; cursor_y+=2)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 8, 112, 56);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            for(i=0; i<6; i++)
                            {
                                rb->lcd_puts(2, i+1, mode_selector_entries[i]);
                                rb->lcd_mono_bitmap(arrow, 1, 8*(i+1)+1, 8, 6);
                            }

                            cursor(0, cursor_y, 112, 8);
                            rb->lcd_update();
                        }
                        cursorpos++;
                    }
                    break;

            case MENU_BUTTON:
            case CHANGE_UP_BUTTON:
                settings.clock = cursorpos;
                done = true;
                break;

            case EXIT_BUTTON:
            case CHANGE_DOWN_BUTTON:
                done = true;
                break;
        }
    }
}

/********************
 * Counter's all done
 *******************/
void counter_finished(void)
{
    int btn;
    int xpos = 0;
    bool bouncing_up = false;
    bool led_on = true;
    unsigned char *times_up = 0;
    times_up = (unsigned char *)timesup;

    done = false;

    while(!done)
    {
        rb->lcd_clear_display();

        /* draw "TIME'S UP" text */
        rb->lcd_mono_bitmap(times_up, 0, xpos, 112, 50);

        /* invert lcd */
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_fillrect(0, 0, 112, 64);
        rb->lcd_set_drawmode(DRMODE_SOLID);

        rb->lcd_update();

        /* pause */
        rb->sleep(HZ/25);

        /* move bitmap up/down 1px */
        if(bouncing_up)
        {
            if(xpos > 0)
                xpos--;
            else
                bouncing_up = false;

            led_on = true;
        }
        else
        {
            if(xpos < 14)
                xpos++;
            else
               bouncing_up = true;

            led_on = false;
        }

        /* turn red led on and off */
#ifndef SIMULATOR
#if (CONFIG_KEYPAD == RECORDER_PAD) /* only for recorders */
        if(led_on)
            or_b(0x40, &PBDRL);
        else
            and_b(~0x40, &PBDRL);
#endif
#endif

        /* exit on keypress */
        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
        {
#ifndef SIMULATOR
#if (CONFIG_KEYPAD == RECORDER_PAD) /* only for recorders */
            and_b(~0x40, &PBDRL); /* shut off the red led */
#endif
#endif
            done = true;
        }
    }
}

/*********************
 * Display the counter
 ********************/
void show_counter(void)
{
    /* increment counter */
    if(counting)
    {
        passed_time = *rb->current_tick - start_tick;
    }
    else
        passed_time = 0;

    displayed_value = counter + passed_time;
    displayed_value = displayed_value / HZ;

    /* these are the REAL displayed values */
    count_s = displayed_value % 60;
    count_m = displayed_value % 3600 / 60;
    count_h = displayed_value / 3600;

    /* compute "counting down" displayed value */
    if(!counting_up)
    {
        remaining_s = target_second - count_s;
        remaining_m = target_minute - count_m;
        remaining_h = target_hour - count_h;
    }

    if(remaining_s < 0)
    {
        remaining_s += 60;
        remaining_m--;
    }
    if(remaining_m < 0)
    {
        remaining_m += 60;
        remaining_h--;
    }
    if(remaining_h < 0)
    {
        /* reset modes */
        counting = false;
        counting_up = true;

        /* all done! */
        counter_finished();

        /* reset all counter values */
        remaining_h = target_hour = 0;
        remaining_m = target_minute = 0;
        remaining_s = target_second = 0;
    }

    if(counting_up)
        rb->snprintf(count_text, sizeof(count_text), "%d:%02d:%02d", count_h, count_m, count_s);
    else
        rb->snprintf(count_text, sizeof(count_text), "%d:%02d:%02d", remaining_h, remaining_m, remaining_s);

    /* allows us to flash the counter if it's paused */
    if(settings.general[general_counter])
    {
        if(settings.clock == ANALOG)
            rb->lcd_putsxy(69, 56, count_text);
        else if(settings.clock == DIGITAL)
            rb->lcd_putsxy(1, 5, count_text);
        else if(settings.clock == LCD)
            rb->lcd_putsxy(1, 5, count_text);
        else if(settings.clock == FULLSCREEN)
            rb->lcd_puts(6, 6, count_text);
        else if(settings.clock == PLAIN)
            rb->lcd_putsxy(0, 50, count_text);
    }
}

/******************
 * Counter settings
 *****************/
void counter_settings(void)
{
    int cursorpos = 1;
    char target_time[15];
    bool original = counting_up;
    bool current = counting_up;

    done = false;

    while(!done)
    {
        /* print text to string */
        rb->snprintf(target_time, sizeof(target_time), "%d:%02d:%02d", target_hour, target_minute, target_second);

        /* draw text on lcd */
        rb->lcd_clear_display();

        center_text(0, "Counter Settings");
        rb->lcd_puts(2, 2, "Count UP");
        rb->lcd_puts(2, 3, "Count DOWN...");
        rb->lcd_puts(4, 4, "Target Time:");
        rb->lcd_puts(4, 5, target_time);
        rb->lcd_puts(0, 7, "OFF: Return");

        /* tell user what mode is selected */
        rb->checkbox(1, 17, 8, 6, counting_up);
        rb->checkbox(1, 25, 8, 6, !counting_up);

        switch(cursorpos)
        {
            case 1: case 2: cursor(0, (8*cursorpos)+8, 112, 8); break;
            case 3: cursor(24, 40, 06, 8); break;
            case 4: cursor(36, 40, 12, 8); break;
            case 5: cursor(54, 40, 12, 8); break;
        }

        if(cursorpos > 2)
            editing_target = true;
        else
            editing_target = false;

        rb->lcd_update();

        /* button scan */
        switch(rb->button_get_w_tmo(HZ/4))
        {
            case MOVE_UP_BUTTON: /* increase / move cursor */
            case MOVE_UP_BUTTON | BUTTON_REPEAT:
                if(!editing_target)
                {
                    if(cursorpos > 1)
                        cursorpos--;
                }
                else
                {
                    if(cursorpos == 3)
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        if(target_hour < 9)
                            target_hour++;
                        else
                            target_hour = 0;
#elif CONFIG_KEYPAD == IPOD_4G_PAD
                        if(target_hour > 0)
                            target_hour--;
                        else
                          target_hour = 9;
#endif
                    else if(cursorpos == 4)
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        if(target_minute < 59)
                            target_minute++;
                        else
                          target_minute = 0;
#elif CONFIG_KEYPAD == IPOD_4G_PAD
                        if(target_minute > 0)
                            target_minute--;
                        else
                          target_minute = 59;
#endif
                    else
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        if(target_second < 59)
                            target_second++;
                        else
                          target_second = 0;
#elif CONFIG_KEYPAD == IPOD_4G_PAD
                        if(target_second > 0)
                            target_second--;
                        else
                          target_second = 59;
#endif
                }
                break;

            case MOVE_DOWN_BUTTON: /* decrease / move cursor */
            case MOVE_DOWN_BUTTON | BUTTON_REPEAT:
                if(!editing_target)
                {
                    if(cursorpos < 3)
                        cursorpos++;
                }
                else
                {
                    if(cursorpos == 3)
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        if(target_hour > 0)
                            target_hour--;
                        else
                            target_hour = 9;
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        if(target_hour < 9)
                            target_hour++;
                        else
                          target_hour = 0;
#endif
                    else if(cursorpos == 4)
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        if(target_minute > 0)
                            target_minute--;
                        else
                          target_minute = 59;
#elif CONFIG_KEYPAD == IPOD_4G_PAD
                        if(target_minute < 59)
                            target_minute++;
                        else
                          target_minute = 0;
#endif
                    else
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        if(target_second > 0)
                            target_second--;
                        else
                          target_second = 59;
#elif CONFIG_KEYPAD == IPOD_4G_PAD
                        if(target_second < 59)
                            target_second++;
                        else
                          target_second = 0;
#endif
                }
                break;

            case CHANGE_DOWN_BUTTON: /* move cursor */
                if(cursorpos > 3)
                    cursorpos--;
                else
                    cursorpos = 5;
                break;

            case CHANGE_UP_BUTTON: /* move cursor */
                if(cursorpos < 5)
                    cursorpos++;
                else
                    cursorpos = 3;
                break;

            case MENU_BUTTON: /* toggle */
                if(cursorpos == 1)
                    counting_up = true;
                if(cursorpos == 2)
                    counting_up = false;
                if(cursorpos >= 3 && cursorpos <= 5)
                {
                    cursorpos = 2;
                    counting_up = false;
                }
                break;

                case EXIT_BUTTON:
                current = counting_up;
                if(current != original)
                    counter = 0;
                done = true;
                break;
        }
    }
}

/***********
 * Main menu
 **********/
void main_menu(void)
{
    int cursor_dummy, cursor_y;
    int i;

    done = false;

    while(!done)
    {
        rb->lcd_clear_display();

        center_text(0, "Main Menu");
        for(i=0; i<7; i++) /* draw menu items and icons */
        {
            rb->lcd_puts(2, i+1, menu_entries[i]);
            rb->lcd_mono_bitmap(arrow, 1, 8*(i+1)+1, 8, 6);
        }

        cursor(0, 8*menupos, 112, 8); /* draw cursor */

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
                case MOVE_UP_BUTTON:
                    if(menupos > 1)
                    {
                        cursor_y = 8+(8*(menupos-1));
                        for(cursor_dummy = cursor_y; cursor_y>cursor_dummy-8; cursor_y-=2)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 8, 112, 56);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            for(i=0; i<7; i++) /* draw menu items and icons */
                            {
                                rb->lcd_puts(2, i+1, menu_entries[i]);
                                rb->lcd_mono_bitmap(arrow, 1, 8*(i+1)+1, 8, 6);
                            }

                            cursor(0, cursor_y, 112, 8); /* draw cursor */
                            rb->lcd_update();
                        }
                        menupos--;
                    }
                    else
                        menupos = 7;
                    break;

                case MOVE_DOWN_BUTTON:
                    if(menupos < 7)
                    {
                        cursor_y = 8+(8*(menupos-1));
                        for(cursor_dummy = cursor_y; cursor_y<cursor_dummy+8; cursor_y+=2)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 8, 112, 56);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            for(i=0; i<7; i++) /* draw menu items and icons */
                            {
                                rb->lcd_puts(2, i+1, menu_entries[i]);
                                rb->lcd_mono_bitmap(arrow, 1, 8*(i+1)+1, 8, 6);
                            }

                            cursor(0, cursor_y, 112, 8); /* draw cursor */
                            rb->lcd_update();
                        }
                        menupos++;
                    }
                    else
                        menupos=1;
                    break;

            case MENU_BUTTON:
            case CHANGE_UP_BUTTON:
                switch(menupos)
                {
                    case 1:
                        done = true;
                        break;

                    case 2:
                        select_mode();
                        break;

                    case 3:
                        counter_settings();
                        break;

                    case 4:
                        settings_screen();
                        break;

                    case 5:
                        general_settings();
                        break;

                    case 6:
                        help_screen();
                        break;

                    case 7:
                        show_credits();
                        done = true;
                        break;
                }
                break;

            case EXIT_BUTTON:
            case CHANGE_DOWN_BUTTON:
#ifdef ALT_MENU_BUTTON
            case ALT_MENU_BUTTON:
#endif
                done = true;
                break;
        }
    }
}

/**********************************************************************
 * Plugin starts here
 **********************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;

    /* time/date ints */
    int hour, minute, second;
    int temphour;
    int last_second = -1;
    int year, day, month;

    bool f2_held = false;

    struct tm* current_time;

    (void)parameter;
    rb = api;

    init_clock();

    while (1)
    {
        /*********************
         * Time info
         *********************/
        current_time = rb->get_time();
        hour = current_time->tm_hour;
        minute = current_time->tm_min;
        second = current_time->tm_sec;
        temphour = current_time->tm_hour;

        /*********************
         * Date info
         *********************/
        year = current_time->tm_year + 1900;
        day = current_time->tm_mday;
        month = current_time->tm_mon + 1;

        done = false;

        if(second != last_second)
        {
            rb->lcd_clear_display();

            /* show counter */
            show_counter();

            /* Analog mode */
            if(settings.clock == ANALOG)
                analog_clock(hour, minute, second);
            /* Digital mode */
            else if(settings.clock == DIGITAL)
            {
                if(settings.digital[digital_blinkcolon])
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, second & 1, false);
                else
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, true, false);
            }
            /* LCD mode */
            else if(settings.clock == LCD)
            {
                if(settings.lcd[lcd_blinkcolon])
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, second & 1, true);
                else
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, true, true);
            }
            /* Fullscreen mode */
            else if(settings.clock == FULLSCREEN)
                fullscreen_clock(hour, minute, second);
            /* Binary mode */
            else if(settings.clock == BINARY)
                binary_clock(hour, minute, second);
            /* Plain mode */
            else if(settings.clock == PLAIN)
            {
                if(settings.plain[plain_blinkcolon])
                    plain_clock(hour, minute, second, second & 1);
                else
                    plain_clock(hour, minute, second, true);
            }
        }

        if(settings.analog[analog_time] == 2 && temphour == 0)
            temphour = 12;
        if(settings.analog[analog_time] == 2 && temphour > 12)
            temphour -= 12;

        draw_extras(year, day, month, temphour, minute, second);

#if (CONFIG_KEYPAD == IPOD_4G_PAD)
        rb->lcd_drawline (113, 0, 113, 65);
        rb->lcd_drawline (0, 65, 113, 65);
#endif

        if(!idle_poweroff)
            rb->reset_poweroff_timer();

        rb->lcd_update();

        /*************************
         * Scan for button presses
         ************************/
        button = rb->button_get_w_tmo(HZ/10);
        switch (button)
        {
            case EXIT_BUTTON: /* save and exit */
                cleanup(NULL);
                return PLUGIN_OK;

            case COUNTER_TOGGLE_BUTTON: /* start/stop counter */
                if(settings.general[general_counter])
                {
                    if(!f2_held) /* Ignore if the counter was reset */
                    {
                        if(counting)
                        {
                            counting = false;
                            counter += passed_time;
                        }
                        else
                        {
                            counting = true;
                            start_tick = *rb->current_tick;
                        }
                    }
                    f2_held = false;
                }
                break;

            case COUNTER_RESET_BUTTON: /* reset counter */
                if(settings.general[general_counter])
                {
                    f2_held = true;  /* Ignore the release event */
                    counter = 0;
                    start_tick = *rb->current_tick;
                }
                break;

            case MENU_BUTTON: /* main menu */
#ifdef ALT_MENU_BUTTON
            case ALT_MENU_BUTTON:
#endif
                main_menu();
                break;

            default:
                if(rb->default_event_handler_ex(button, cleanup, NULL)
                   == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

#endif
