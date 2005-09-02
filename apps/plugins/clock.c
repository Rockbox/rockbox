/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: clock.c,v 2.60 2003/12/8
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

#if defined(HAVE_LCD_BITMAP) && defined(HAVE_RTC)

#define CLOCK_VERSION "2.60"

#define MODE_ANALOG 1
#define MODE_DIGITAL 2
#define MODE_LCD 3
#define MODE_BINARY 4
#define MODE_FULLSCREEN 5

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

/* This bool is used for most of the while loops */
bool done = false;

static struct plugin_api* rb;

/***********************************************************
 * Used for hands to define lengths at a given time - ANALOG
 **********************************************************/
static const unsigned char xminute[] = {
56,59,61,64,67,70,72,75,77,79,80,82,83,84,84,84,84,84,83,82,80,79,77,75,72,70,
67,64,61,59,56,53,51,48,45,42,40,37,35,33,32,30,29,28,28,28,28,28,29,30,32,33,
35,37,40,42,45,48,51,53 };
static const unsigned char yminute[] = {
55,54,54,53,53,51,50,49,47,45,43,41,39,36,34,32,30,28,25,23,21,19,17,15,14,13,
11,11,10,10, 9,10,10,11,11,13,14,15,17,19,21,23,25,28,30,32,34,36,39,41,43,45,
47,49,50,51,53,53,54,54 };
static const unsigned char yhour[] = {
47,47,46,46,46,45,44,43,42,41,39,38,36,35,33,32,31,29,28,26,25,23,22,21,20,19,
18,18,18,17,17,17,18,18,18,19,20,21,22,23,25,26,28,29,31,32,33,35,36,38,39,41,
42,43,44,45,46,46,46,47 };
static const unsigned char xhour[] = {
56,58,59,61,63,65,67,68,70,71,72,73,74,74,75,75,75,74,74,73,72,71,70,68,67,65,
63,61,59,58,56,54,53,51,49,47,45,44,42,41,40,39,38,38,37,37,37,38,38,39,40,41,
42,44,45,47,49,51,53,54 };

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

/************************************
 * "Clockbox" clock logo - by Adam S.
 ***********************************/
const unsigned char clocklogo[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xe0, 0xf0, 0xf0,
0x78, 0x78, 0x3c, 0x3c, 0xfc, 0xfc, 0xfc, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
0x1e, 0x3c, 0x3c, 0x3c, 0x7c, 0x7c, 0xf8, 0xf8, 0xf0, 0x30, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xe0, 0xf8, 0xfc, 0xfe, 0xff, 0x3f, 0x3f, 0x7f, 0x73, 0xf1, 0xe0,
0xe0, 0xc0, 0xc0, 0x80, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0xe0,
0xfe, 0xff, 0xff, 0x7f, 0x03, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
0x80, 0xf0, 0xf8, 0xfc, 0xfe, 0x3e, 0x0e, 0x0f, 0x07, 0x07, 0x0f, 0x1f, 0xfe,
0xfe, 0xfc, 0xf8, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xf0, 0xf8, 0xfc, 0x7e, 0x1e,
0x0f, 0x0f, 0x07, 0x0f, 0x0f, 0x0f, 0x1e, 0x1e, 0x00, 0x00, 0x00, 0x00, 0xf0,
0xff, 0xff, 0xff, 0xff, 0xe1, 0xf0, 0xf8, 0x7c, 0x3e, 0x1f, 0x0f, 0x07, 0x03,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x01, 0x0f, 0x1f, 0x3f, 0x3f, 0x3f, 0x3f, 0x0e, 0x00, 0x40, 0x7c, 0x7f,
0x7f, 0x7f, 0x77, 0x70, 0x70, 0x70, 0x70, 0x70, 0x30, 0x00, 0x00, 0x00, 0x00,
0x0f, 0x3f, 0x7f, 0x7f, 0xfd, 0xf0, 0xe0, 0xe0, 0xf0, 0xf0, 0xf8, 0xff, 0xff,
0xdf, 0xcf, 0xc3, 0xc0, 0xc0, 0x80, 0x80, 0x1f, 0x3f, 0x7f, 0x7f, 0xf8, 0xf0,
0xf0, 0xf0, 0xf0, 0xf0, 0xf8, 0xf8, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xfe, 0xff,
0x7f, 0x3f, 0x07, 0x03, 0x0f, 0x3f, 0x7f, 0xfe, 0xfc, 0xf0, 0xc0, 0x00, 0x00,
0x00, 0x00, 0x80, 0xc0, 0xc0, 0xc0, 0xc0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0x1f, 0x3f, 0x7f, 0xff, 0xff, 0xfc, 0xf0, 0xe0, 0xc0, 0x80, 0x80, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0xc0, 0xc0,
0xc0, 0xe0, 0xf0, 0xf0, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xfe, 0xff, 0xff, 0x7f, 0x73,
0x71, 0x71, 0xf9, 0xff, 0xff, 0xff, 0x8f, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xfc,
0xfe, 0xff, 0xff, 0x07, 0x03, 0x01, 0x01, 0x01, 0x01, 0xc3, 0xff, 0xff, 0xff,
0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0xc7, 0xff, 0xff, 0xff, 0xfc,
0xfe, 0xdf, 0x0f, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x07, 0x07, 0x07, 0x07,
0x07, 0x07, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03,
0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x1f, 0x1f, 0x1f, 0x1f, 0x1c, 0x1c,
0x1c, 0x1e, 0x1f, 0x0f, 0x0f, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07,
0x0f, 0x1f, 0x1f, 0x1e, 0x1c, 0x1c, 0x1c, 0x1e, 0x1f, 0x0f, 0x07, 0x07, 0x01,
0x00, 0x00, 0x00, 0x10, 0x1c, 0x1e, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x01, 0x07,
0x1f, 0x1f, 0x1f, 0x1c, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

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

/* settings saved to this location */
static const char default_filename[] = "/.rockbox/rocks/.clock_settings";

/* names of contributors */
const char* credits[] = {
"Zakk Roberts",
"Linus N. Feltzing",
"BlueChip",
"T.P. Diffenbach",
"David McIntyre",
"Justin D. Young",
"Lee Pilgrim",
"top_bloke",
"Adam S.",
"Scott Myran",
"Tony Kirk",
"Jason Tye"
};

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
"Logo",
"Design",
"Design",
"Design"
};

/***********************************
 * This is saved to default_filename
 **********************************/
struct saved_settings
{
    /* general */
    int clock; /* 1: analog, 2: digital led, 3: digital lcd, 4: full, 5: binary */
    bool backlight_on;
    int save_mode; /* 1: on exit, 2: automatically, 3: manually */
    bool display_counter;

    /* analog */
    bool analog_digits;
    int analog_date; /* 0: off, 1: american, 2: european */
    bool analog_secondhand;
    int analog_time; /* 0: off, 1: 24h, 2: 12h */

    /* digital */
    int digital_seconds; /* 0: off, 1: digital, 2: bar, 3: fullscreen */
    int digital_date; /* 0: off, 1: american, 2: european */
    bool digital_blinkcolon;
    bool digital_12h;

    /* LCD */
    int lcd_seconds; /* 0: off, 1: lcd, 2: bar, 3: fullscreen */
    int lcd_date; /* 0: off, 1: american, 2: european */
    bool lcd_blinkcolon;
    bool lcd_12h;

    /* fullscreen */
    bool fullscreen_border;
    bool fullscreen_secondhand;
    bool fullscreen_invertseconds;
} settings;

/************************
 * Setting default values
 ***********************/
void reset_settings(void)
{
    /* general */
    settings.clock = 1; /* 1: analog, 2: digital led, 3: digital lcd, 4: full, 5: binary */
    settings.backlight_on = true;
    settings.save_mode = 1; /* 1: on exit, 2: automatically, 3: manually */
    settings.display_counter = true;

    /* analog */
    settings.analog_digits = false;
    settings.analog_date = 0; /* 0: off, 1: american, 2: european */
    settings.analog_secondhand = true;
    settings.analog_time = false; /* 0: off, 1: 24h, 2: 12h */

    /* digital */
    settings.digital_seconds = 1; /* 0: off, 1: digital, 2: bar, 3: fullscreen */
    settings.digital_date = 1; /* 0: off, 1: american, 2: european */
    settings.digital_blinkcolon = false;
    settings.digital_12h = true;

    /* LCD */
    settings.lcd_seconds = 1; /* 0: off, 1: lcd, 2: bar, 3: fullscreen */
    settings.lcd_date = 1; /* 0: off, 1: american, 2: european */
    settings.lcd_blinkcolon = false;
    settings.lcd_12h = true;

    /* fullscreen */
    settings.fullscreen_border = true;
    settings.fullscreen_secondhand = true;
    settings.fullscreen_invertseconds = false;
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
        rb->snprintf(buf, sizeof(buf), "Saving Settings");
        rb->lcd_getstringsize(buf, &buf_w, &buf_h);
        rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
        show_clock_logo(true, true);

        rb->lcd_update();
    }

    /* create the settings file and write the settings to it */
    fd = rb->creat(default_filename, O_WRONLY);

    if(fd >= 0)
    {
        rb->write (fd, &settings, sizeof(struct saved_settings));
        rb->close(fd);

        if(interface)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->snprintf(buf, sizeof(buf), "Saved Settings");
            rb->lcd_getstringsize(buf, &buf_w, &buf_h);
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
        }
    }
    else
    {
        if(interface)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->snprintf(buf, sizeof(buf), "Save Failed");
            rb->lcd_getstringsize(buf, &buf_w, &buf_h);
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
        }
    }

    if(interface)
    {
        rb->lcd_update();

        rb->sleep(HZ);

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

    rb->snprintf(buf, sizeof(buf), "Clock %s", CLOCK_VERSION);
    rb->lcd_getstringsize(buf, &buf_w, &buf_h);
    rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 48, buf);

    rb->snprintf(buf, sizeof(buf), "Loading Settings");
    rb->lcd_getstringsize(buf, &buf_w, &buf_h);
    rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
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
            rb->snprintf(buf, sizeof(buf), "Loaded Settings");
            rb->lcd_getstringsize(buf, &buf_w, &buf_h);
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
        }
        else /* bail out */
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->snprintf(buf, sizeof(buf), "Old Settings File");
            rb->lcd_getstringsize(buf, &buf_w, &buf_h);
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
            reset_settings();
        }
    }
    else /* bail out */
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 56, 112, 8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->snprintf(buf, sizeof(buf), "No Settings File");
        rb->lcd_getstringsize(buf, &buf_w, &buf_h);
        rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);

        /* use the default in this case */
        reset_settings();
    }

    rb->lcd_update();

#ifndef SIMULATOR
    rb->ata_sleep();
#endif

    rb->sleep(HZ);

    /* make the logo fly out */
    exit_logo();
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
    if(settings.clock == 2)
    {
        if(settings.digital_12h)
        {
            if(hour > 12)
                rb->lcd_mono_bitmap(pm, 97, 55, 15, 8);
            else
                rb->lcd_mono_bitmap(am, 1, 55, 15, 8);
        }
    }
    else
    {
        if(settings.lcd_12h)
        {
            if(hour > 12)
                rb->lcd_mono_bitmap(pm, 97, 55, 15, 8);
            else
                rb->lcd_mono_bitmap(am, 1, 55, 15, 8);
        }
    }

    /* Now change to 12H mode if requested */
    if(settings.clock == 2)
    {
        if(settings.digital_12h)
        {
            if(hour > 12)
                hour -= 12;
            if(hour == 0)
                hour = 12;
        }
    }
    else
    {
        if(settings.lcd_12h)
        {
            if(hour > 12)
                hour -= 12;
            if(hour == 0)
                hour = 12;
        }
    }

    draw_7seg_digit(hour / 10, xpos, y, width, height, lcd);
    xpos += width + 6;
    draw_7seg_digit(hour % 10, xpos, y, width, height, lcd);
    xpos += width + 6;

    if(colon)
    {
        rb->lcd_drawline(xpos, y + height/3 + 2,
                         xpos, y + height/3 + 2);
        rb->lcd_drawline(xpos+1, y + height/3 + 1,
                         xpos+1, y + height/3 + 3);
        rb->lcd_drawline(xpos+2, y + height/3,
                         xpos+2, y + height/3 + 4);
        rb->lcd_drawline(xpos+3, y + height/3 + 1,
                         xpos+3, y + height/3 + 3);
        rb->lcd_drawline(xpos+4, y + height/3 + 2,
                         xpos+4, y + height/3 + 2);

        rb->lcd_drawline(xpos, y + height-height/3 + 2,
                         xpos, y + height-height/3 + 2);
        rb->lcd_drawline(xpos+1, y + height-height/3 + 1,
                         xpos+1, y + height-height/3 + 3);
        rb->lcd_drawline(xpos+2, y + height-height/3,
                         xpos+2, y + height-height/3 + 4);
        rb->lcd_drawline(xpos+3, y + height-height/3 + 1,
                         xpos+3, y + height-height/3 + 3);
        rb->lcd_drawline(xpos+4, y + height-height/3 + 2,
                         xpos+4, y + height-height/3 + 2);
    }

    xpos += 12;

    draw_7seg_digit(minute / 10, xpos, y, width, height, lcd);
    xpos += width + 6;
    draw_7seg_digit(minute % 10, xpos, y, width, height, lcd);
    xpos += width + 6;
}

/*************
 * Binary mode
 ************/
void binary(int hour, int minute, int second)
{
    /* temporary modifiable versions of ints */
    int temphour = hour;
    int tempmin = minute;
    int tempsec = second;

    rb->lcd_clear_display();

    /******
     * HOUR
     *****/
    if(temphour >= 32)
    {
        rb->lcd_mono_bitmap(bitmap_1, 0, 1, 15, 20);
        temphour -= 32;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 0, 1, 15, 20);
    if(temphour >= 16)
    {
        rb->lcd_mono_bitmap(bitmap_1, 19, 1, 15, 20);
        temphour -= 16;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 19, 1, 15, 20);
    if(temphour >= 8)
    {
        rb->lcd_mono_bitmap(bitmap_1, 38, 1, 15, 20);
        temphour -= 8;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 38, 1, 15, 20);
    if(temphour >= 4)
    {
        rb->lcd_mono_bitmap(bitmap_1, 57, 1, 15, 20);
        temphour -= 4;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 57, 1, 15, 20);
    if(temphour >= 2)
    {
        rb->lcd_mono_bitmap(bitmap_1, 76, 1, 15, 20);
        temphour -= 2;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 76, 1, 15, 20);
    if(temphour >= 1)
    {
        rb->lcd_mono_bitmap(bitmap_1, 95, 1, 15, 20);
        temphour -= 1;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 95, 1, 15, 20);

    /*********
     * MINUTES
     ********/
    if(tempmin >= 32)
    {
        rb->lcd_mono_bitmap(bitmap_1, 0, 21, 15, 20);
        tempmin -= 32;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 0, 21, 15, 20);
    if(tempmin >= 16)
    {
        rb->lcd_mono_bitmap(bitmap_1, 19, 21, 15, 20);
        tempmin -= 16;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 19, 21, 15, 20);
    if(tempmin >= 8)
    {
        rb->lcd_mono_bitmap(bitmap_1, 38, 21, 15, 20);
        tempmin -= 8;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 38, 21, 15, 20);
    if(tempmin >= 4)
    {
        rb->lcd_mono_bitmap(bitmap_1, 57, 21, 15, 20);
        tempmin -= 4;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 57, 21, 15, 20);
    if(tempmin >= 2)
    {
        rb->lcd_mono_bitmap(bitmap_1, 76, 21, 15, 20);
        tempmin -= 2;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 76, 21, 15, 20);
    if(tempmin >= 1)
    {
        rb->lcd_mono_bitmap(bitmap_1, 95, 21, 15, 20);
        tempmin -= 1;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 95, 21, 15, 20);

    /*********
     * SECONDS
     ********/
    if(tempsec >= 32)
    {
        rb->lcd_mono_bitmap(bitmap_1, 0, 42, 15, 20);
        tempsec -= 32;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 0, 42, 15, 20);
    if(tempsec >= 16)
    {
        rb->lcd_mono_bitmap(bitmap_1, 19, 42, 15, 20);
        tempsec -= 16;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 19, 42, 15, 20);
    if(tempsec >= 8)
    {
        rb->lcd_mono_bitmap(bitmap_1, 38, 42, 15, 20);
        tempsec -= 8;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 38, 42, 15, 20);
    if(tempsec >= 4)
    {
        rb->lcd_mono_bitmap(bitmap_1, 57, 42, 15, 20);
        tempsec -= 4;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 57, 42, 15, 20);
    if(tempsec >= 2)
    {
        rb->lcd_mono_bitmap(bitmap_1, 76, 42, 15, 20);
        tempsec -= 2;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 76, 42, 15, 20);
    if(tempsec >= 1)
    {
        rb->lcd_mono_bitmap(bitmap_1, 95, 42, 15, 20);
        tempsec -= 1;
    }
    else
        rb->lcd_mono_bitmap(bitmap_0, 95, 42, 15, 20);

    rb->lcd_update();
}

/****************
 * Shows the logo
 ***************/
void show_clock_logo(bool animate, bool show_clock_text)
{
    int y_position;

    unsigned char *clogo = (unsigned char *)clocklogo;

    rb->snprintf(buf, sizeof(buf), "Clock %s", CLOCK_VERSION);
    rb->lcd_getstringsize(buf, &buf_w, &buf_h);

    /* animate logo */
    if(animate)
    {
        /* move down the screen */
        for(y_position = 0; y_position <= 26; y_position++)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_drawline(0, y_position/2-1, 111, y_position/2-1);
            rb->lcd_drawline(0, y_position/2+38, 111, y_position/2+38);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_mono_bitmap(clogo, 0, y_position/2, 112, 37);
            if(show_clock_text)
                rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 48, buf);
            rb->lcd_update();
        }
        /* bounce back up a little */
        for(y_position = 26; y_position >= 16; y_position--)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_drawline(0, y_position/2-1, 111, y_position/2-1);
            rb->lcd_drawline(0, y_position/2+38, 111, y_position/2+38);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_mono_bitmap(clogo, 0, y_position/2, 112, 37);
            if(show_clock_text)
                rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 48, buf);
            rb->lcd_update();
        }
        /* and go back down again */
        for(y_position = 16; y_position <= 20; y_position++)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_drawline(0, y_position/2-1, 111, y_position/2-1);
            rb->lcd_drawline(0, y_position/2+38, 111, y_position/2+38);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_mono_bitmap(clogo, 0, y_position/2, 112, 37);
            if(show_clock_text)
                rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 48, buf);
            rb->lcd_update();
        }
    }
    else /* don't animate, just show */
    {
        rb->lcd_mono_bitmap(clogo, 0, 10, 112, 37);
        if(show_clock_text)
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 48, buf);
        rb->lcd_update();
    }
}

/********************
 * Logo flies off lcd
 *******************/
void exit_logo(void)
{
    int y_position;

    unsigned char *clogo = 0;
    clogo = (unsigned char *)clocklogo;

    /* fly downwards */
    for(y_position = 20; y_position <= 128; y_position++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_drawline(0, y_position/2-1, 111, y_position/2-1);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_mono_bitmap(clogo, 0, y_position/2, 112, 37);
        rb->lcd_update();
    }
}

/*******************
 * Rolls the credits
 ******************/
/* The following function is pretty confusing, so
 * it's extra well commented. */
bool roll_credits(void)
{
    int j=0, namepos, jobpos; /* namepos/jobpos are x coords for strings of text */
    int btn;
    int numnames = 12; /* amount of people in the credits array */
    int pause;

    /* used to center the text */
    char name[20];
    char job[15];
    int name_w, name_h;
    int job_w, job_h;
    int credits_w, credits_h, credits_pos;
    int progress_pos, progress_percent=0;

    /* shows "[Credits] XX/XX" */
    char elapsednames[16];

    /* put text into variable, and save the width and height of the text */
    rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %02d/%02d", j+1, numnames);
    rb->lcd_getstringsize(elapsednames, &credits_w, &credits_h);

    /* fly in text from the left */
    for(credits_pos = 0 - credits_w; credits_pos <= (LCD_WIDTH/2)-(credits_w/2); credits_pos++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_drawline(credits_pos-1, 0, credits_pos-1, 8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update(); /* update the whole lcd to slow down the loop */
    }

    /* unfold progressbar from the right */
    for(progress_pos = LCD_WIDTH; progress_pos >= 40; progress_pos--)
    {
        rb->scrollbar(progress_pos, 9, LCD_WIDTH-progress_pos, 7, numnames*4, 0, progress_percent, HORIZONTAL);
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_drawline(0, 0, 0, 30);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_update(); /* update the whole lcd to slow down the loop */
    }

    /* now roll the credits */
    for(j=0; j < numnames; j++)
    {
        rb->lcd_clear_display();

        show_clock_logo(false, false);

        rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %02d/%02d", j+1, numnames);
        rb->lcd_putsxy(credits_pos-1, 0, elapsednames);

        /* used to center the text */
        rb->snprintf(name, sizeof(name), "%s", credits[j]);
        rb->snprintf(job, sizeof(job), "%s", jobs[j]);
        rb->lcd_getstringsize(name, &name_w, &name_h);
        rb->lcd_getstringsize(job, &job_w, &job_h);

        rb->scrollbar(progress_pos, 9, LCD_WIDTH-progress_pos, 7, numnames*4, 0, progress_percent, HORIZONTAL);

        /* line 1 flies in */
        for (namepos=0-name_w; namepos < (LCD_WIDTH/2)-(name_w/2)-2; namepos++)
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

        progress_percent++;
        rb->scrollbar(progress_pos, 9, LCD_WIDTH-progress_pos, 7, numnames*4, 0, progress_percent, HORIZONTAL);
        rb->lcd_update_rect(progress_pos, 8, 112-progress_pos, 8);

        /* now line 2 flies in */
        for(jobpos=LCD_WIDTH; jobpos > (LCD_WIDTH/2)-(job_w+2)/2; jobpos--) /* we use (job_w+2) to ensure it fits on the LCD */
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 56, 112, 8); /* clear trails */
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(jobpos, 56, job);
            rb->lcd_update();

            /* exit on keypress */
            btn = rb->button_get(false);
            if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                return false;
        }

        progress_percent++;
        rb->scrollbar(progress_pos, 9, LCD_WIDTH-progress_pos, 7, numnames*4, 0, progress_percent, HORIZONTAL);
        rb->lcd_update_rect(progress_pos, 8, 112-progress_pos, 8);

        /* pause (2s) and scan for button presses */
        for (pause = 0; pause < 10; pause++)
        {
            rb->sleep((HZ*2)/10); /* wait a moment */

            namepos++;
            jobpos--;

            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, 48, 112, 16);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, 48, name);
            rb->lcd_putsxy(jobpos, 56, job);
            rb->lcd_update();

            btn = rb->button_get(false);
            if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                return false;
        }

        progress_percent++;
        rb->scrollbar(progress_pos, 9, LCD_WIDTH-progress_pos, 7, numnames*4, 0, progress_percent, HORIZONTAL);
        rb->lcd_update_rect(progress_pos, 8, 112-progress_pos, 8);

        /* fly out both lines at same time */
        namepos=((LCD_WIDTH/2)-(name_w/2))+8;
        jobpos=((LCD_WIDTH/2)-(job_w+2)/2)-8;
        while(namepos<LCD_WIDTH || jobpos > 0-job_w)
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

            namepos++;
            jobpos--;
        }

        progress_percent++;
        rb->scrollbar(progress_pos, 9, LCD_WIDTH-progress_pos, 7, numnames*4, 0, progress_percent, HORIZONTAL);

        /* pause (.2s) */
        rb->sleep(HZ/2);

        /* and scan for button presses */
        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
             return false;
    }

    /* now make the text exit to the right */
    for(credits_pos = (LCD_WIDTH/2)-(credits_w/2); credits_pos <= 112; credits_pos++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_drawline(credits_pos-1, 0, credits_pos-1, 8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update();
    }

    /* fold progressbar in to the right */
    for(progress_pos = 42; progress_pos < 112; progress_pos++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_drawline(progress_pos-1, 8, progress_pos-1, 16);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->scrollbar(progress_pos, 9, LCD_WIDTH-progress_pos, 7, numnames*4, 0, progress_percent, HORIZONTAL);
        rb->lcd_update(); /* update the whole lcd to slow down the loop */
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

    rb->snprintf(buf, sizeof(buf), "Credits");
    rb->lcd_getstringsize(buf, &buf_w, &buf_h);
    rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);

    /* show the logo with an animation and the clock version text */
    show_clock_logo(true, true);

    rb->lcd_update();

    /* pause while button scanning */
    for (j = 0; j < 10; j++)
    {
        rb->sleep((HZ*2)/10);

        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
            return false;
    }

    /* then roll the credits */
    roll_credits();

    return false;
}

/**********************************************************************
 * Cleanup on plugin return
 **********************************************************************/

void cleanup(void *parameter)
{
    (void)parameter;

    if(settings.save_mode == 1)
        save_settings(true);

    /* restore set backlight timeout */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
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

/******************
 * F1 Screen - HELP
 *****************/
bool f1_screen(void)
{
    int button;
    int screen = 1;
    done = false;

    while (!done)
    {
        rb->lcd_clear_display();

        if(screen == 1)
        {
            rb->snprintf(buf, sizeof(buf), "<<---- 1/9 NEXT>>");
            rb->lcd_getstringsize(buf, &buf_w, &buf_h);
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
        }
        else if(screen == 9)
        {
            rb->snprintf(buf, sizeof(buf), "<<BACK 9/9 ---->>");
            rb->lcd_getstringsize(buf, &buf_w, &buf_h);
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
        }
        else
        {
            rb->snprintf(buf, sizeof(buf), "<<BACK %d/9 NEXT>>", screen);
            rb->lcd_getstringsize(buf, &buf_w, &buf_h);
            rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);
        }

        /* page one */
        if(screen == 1)
        {
            rb->lcd_puts(0, 0, "Using Clock " CLOCK_VERSION ":");
            rb->lcd_puts(0, 1, "To navigate this");
            rb->lcd_puts(0, 2, "help, use LEFT and");
            rb->lcd_puts(0, 3, "RIGHT. F1 returns");
            rb->lcd_puts(0, 4, "you to the clock.");
            rb->lcd_puts(0, 5, "At any mode, [F1]");
            rb->lcd_puts(0, 6, "will show you this");
        }
        else if(screen == 2)
        {
            rb->lcd_puts(0, 0, "help screen. [F2]");
            rb->lcd_puts(0, 1, "will start/stop");
            rb->lcd_puts(0, 2, "the Counter. Hold");
            rb->lcd_puts(0, 3, "F2 to reset the");
            rb->lcd_puts(0, 4, "Counter. [ON+F2]");
            rb->lcd_puts(0, 5, "gives you counting");
            rb->lcd_puts(0, 6, "options.");
        }
        else if(screen == 3)
        {
            rb->lcd_puts(0, 0, "At any mode, [F3]");
            rb->lcd_puts(0, 1, "opens the Options");
            rb->lcd_puts(0, 2, "screen. In Options");
            rb->lcd_puts(0, 3, "use UP/DN to move");
            rb->lcd_puts(0, 4, "and PLAY to toggle.");
            rb->lcd_puts(0, 5, "[ON+F3] shows you");
            rb->lcd_puts(0, 6, "General Settings.");
        }
        else if(screen == 4)
        {
            rb->lcd_puts(0, 0, "[ON] at any mode");
            rb->lcd_puts(0, 1, "will present you");
            rb->lcd_puts(0, 2, "with a credits roll");
            rb->lcd_puts(0, 3, "[PLAY] from any");
            rb->lcd_puts(0, 4, "mode will show the");
            rb->lcd_puts(0, 5, "MODE SELECTOR. Use");
            rb->lcd_puts(0, 6, "UP/DOWN to select");
        }
        else if(screen == 5)
        {
            rb->lcd_puts(0, 0, "a mode and PLAY to");
            rb->lcd_puts(0, 1, "go to it.");
            rb->lcd_puts(0, 2, "_-=CLOCK MODES=-_");
            rb->lcd_puts(0, 3, "*ANALOG: Shows a");
            rb->lcd_puts(0, 4, "small round clock");
            rb->lcd_puts(0, 5, "in the center of");
            rb->lcd_puts(0, 6, "LCD. Options appear");
        }
        else if(screen == 6)
        {
            rb->lcd_puts(0, 0, "around it.");
            rb->lcd_puts(0, 1, "*DIGITAL: Shows an");
            rb->lcd_puts(0, 2, "LCD imitation with");
            rb->lcd_puts(0, 3, "virtual 'segments'.");
            rb->lcd_puts(0, 4, "*LCD: Shows another");
            rb->lcd_puts(0, 5, "imitation of an");
            rb->lcd_puts(0, 6, "LCD display.");
        }
        else if(screen == 7)
        {
            rb->lcd_puts(0, 0, "*FULLSCREEN: Like");
            rb->lcd_puts(0, 1, "Analog mode, but");
            rb->lcd_puts(0, 2, "uses the whole LCD.");
            rb->lcd_puts(0, 3, "Less options are");
            rb->lcd_puts(0, 4, "available in this");
            rb->lcd_puts(0, 5, "mode.");
            rb->lcd_puts(0, 6, "*BINARY: Shows a");
        }
        else if(screen == 8)
        {
            rb->lcd_puts(0, 0, "binary clock. For");
            rb->lcd_puts(0, 1, "help on reading");
            rb->lcd_puts(0, 2, "binary, see:");
            rb->lcd_puts_scroll(0, 3, "http://en.wikipedia.org/wiki/Binary_numeral_system");
            rb->lcd_puts(0, 4, "_-=OTHER KEYS=-_");
            rb->lcd_puts(0, 5, "[DWN] will disable");
            rb->lcd_puts(0, 6, "Rockbox's idle");
        }
        else if(screen == 9)
        {
            rb->lcd_puts(0, 0, "poweroff feature.");
            rb->lcd_puts(0, 1, "[UP] will reenable");
            rb->lcd_puts(0, 2, "it. [LEFT] will");
            rb->lcd_puts(0, 3, "turn off the back-");
            rb->lcd_puts(0, 4, " light, [RIGHT]");
            rb->lcd_puts(0, 5, "will turn it on.");
            rb->lcd_puts(0, 6, "[OFF] exits plugin.");
        }

        rb->lcd_update();

        button = rb->button_get_w_tmo(HZ/4);
        switch(button)
        {
            case BUTTON_F1: /* exit */
            case BUTTON_OFF:
                done = true;
                break;

            case BUTTON_LEFT:
                if(screen > 1)
                    screen --;
                break;

            case BUTTON_RIGHT:
                if(screen < 9)
                    screen++;
                break;

            default:
                if(rb->default_event_handler_ex(button, cleanup, NULL)
                   == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
    return true;
}

/*************************
 * Draws a checkbox bitmap
 ************************/
void draw_checkbox(bool setting, int x, int y)
{
    if(setting) /* checkbox is on */
        rb->lcd_mono_bitmap(checkbox_full, x, y, 8, 6);
    else /* checkbox is off */
        rb->lcd_mono_bitmap(checkbox_empty, x, y, 8, 6);
}

void draw_settings(void)
{
    if(settings.clock == 1)
    {
        rb->lcd_puts(0, 0, "OPTIONS (Analog)");

        rb->lcd_puts(2, 4, "Digits");

        if(settings.analog_date == 0)
            rb->lcd_puts(2, 5, "Date: Off");
        else if(settings.analog_date == 1)
            rb->lcd_puts(2, 5, "Date: American");
        else
            rb->lcd_puts(2, 5, "Date: European");

        if(settings.analog_time == 0)
            rb->lcd_puts(2, 6, "Time Readout:Off");
        else if(settings.analog_time == 1)
            rb->lcd_puts(2, 6, "Time Readout:24h");
        else
            rb->lcd_puts(2, 6, "Time Readout:12h");

        rb->lcd_puts(2, 7, "Second Hand");

        /* Draw checkboxes */
        draw_checkbox(settings.analog_digits, 1, 33);

        if(settings.analog_date == 0)
            rb->lcd_mono_bitmap(checkbox_empty, 1, 41, 8, 6);
        else if(settings.analog_date == 1)
            rb->lcd_mono_bitmap(checkbox_half, 1, 41, 8, 6);
        else
            rb->lcd_mono_bitmap(checkbox_full, 1, 41, 8, 6);

        if(settings.analog_time == 0)
            rb->lcd_mono_bitmap(checkbox_empty, 1, 49, 8, 6);
        else if(settings.analog_time == 1)
            rb->lcd_mono_bitmap(checkbox_half, 1, 49, 8, 6);
        else
            rb->lcd_mono_bitmap(checkbox_full, 1, 49, 8, 6);

        draw_checkbox(settings.analog_secondhand, 1, 57);
    }
    else if(settings.clock == 2)
    {
        rb->lcd_puts(0, 0, "OPTIONS (Digital)");

        if(settings.digital_date == 0)
            rb->lcd_puts(2, 4, "Date: Off");
        else if(settings.digital_date == 1)
            rb->lcd_puts(2, 4, "Date: American");
        else
            rb->lcd_puts(2, 4, "Date: European");

        if(settings.digital_seconds == 0)
            rb->lcd_puts(2, 5, "Seconds: Off");
        else if(settings.digital_seconds == 1)
            rb->lcd_puts(2, 5, "Seconds: Text");
        else if(settings.digital_seconds == 2)
            rb->lcd_puts(2, 5, "Seconds: Bar");
        else
            rb->lcd_puts(2, 5, "Seconds: Inverse");

        rb->lcd_puts(2, 6, "Blinking Colon");
        rb->lcd_puts(2, 7, "12-Hour Format");

        /* Draw checkboxes */
        if(settings.digital_date == 0)
            rb->lcd_mono_bitmap(checkbox_empty, 1, 33, 8, 6);
        else if(settings.digital_date == 1)
            rb->lcd_mono_bitmap(checkbox_half, 1, 33, 8, 6);
        else
            rb->lcd_mono_bitmap(checkbox_full, 1, 33, 8, 6);

        if(settings.digital_seconds == 0)
            rb->lcd_mono_bitmap(checkbox_empty, 1, 41, 8, 6);
        else if(settings.digital_seconds == 1)
            rb->lcd_mono_bitmap(checkbox_onethird, 1, 41, 8, 6);
        else if(settings.digital_seconds == 2)
            rb->lcd_mono_bitmap(checkbox_twothird, 1, 41, 8, 6);
        else
            rb->lcd_mono_bitmap(checkbox_full, 1, 41, 8, 6);

        draw_checkbox(settings.digital_blinkcolon, 1, 49);
        draw_checkbox(settings.digital_12h, 1, 57);
    }
    else if(settings.clock == 3)
    {
        rb->lcd_puts(0, 0, "OPTIONS (LCD)");

        if(settings.lcd_date == 0)
            rb->lcd_puts(2, 4, "Date: Off");
        else if(settings.lcd_date == 1)
            rb->lcd_puts(2, 4, "Date: American");
        else
            rb->lcd_puts(2, 4, "Date: European");

        if(settings.lcd_seconds == 0)
            rb->lcd_puts(2, 5, "Seconds: Off");
        else if(settings.lcd_seconds == 1)
            rb->lcd_puts(2, 5, "Seconds: Text");
        else if(settings.lcd_seconds == 2)
            rb->lcd_puts(2, 5, "Seconds: Bar");
        else
            rb->lcd_puts(2, 5, "Seconds: Inverse");

        rb->lcd_puts(2, 6, "Blinking Colon");
        rb->lcd_puts(2, 7, "12-Hour Format");

        /* Draw checkboxes */
        if(settings.lcd_date == 0)
            rb->lcd_mono_bitmap(checkbox_empty, 1, 33, 8, 6);
        else if(settings.lcd_date == 1)
            rb->lcd_mono_bitmap(checkbox_half, 1, 33, 8, 6);
        else
            rb->lcd_mono_bitmap(checkbox_full, 1, 33, 8, 6);

        if(settings.lcd_seconds == 0)
            rb->lcd_mono_bitmap(checkbox_empty, 1, 41, 8, 6);
        else if(settings.lcd_seconds == 1)
            rb->lcd_mono_bitmap(checkbox_onethird, 1, 41, 8, 6);
        else if(settings.lcd_seconds == 2)
            rb->lcd_mono_bitmap(checkbox_twothird, 1, 41, 8, 6);
        else
            rb->lcd_mono_bitmap(checkbox_full, 1, 41, 8, 6);

        draw_checkbox(settings.lcd_blinkcolon, 1, 49);
        draw_checkbox(settings.lcd_12h, 1, 57);
    }
    else if(settings.clock == 4)
    {
        rb->lcd_puts(0, 0, "OPTIONS (Full)");

        rb->lcd_puts(2, 4, "Border");
        rb->lcd_puts(2, 5, "Second Hand");
        rb->lcd_puts(2, 6, "Invert Seconds");

        draw_checkbox(settings.fullscreen_border, 1, 33);
        draw_checkbox(settings.fullscreen_secondhand, 1, 41);
        draw_checkbox(settings.fullscreen_invertseconds, 1, 49);
    }
    else if(settings.clock == 5)
    {
        rb->lcd_puts(0, 0, "OPTIONS (Binary)");
        rb->lcd_puts(2, 4, "-- NO OPTIONS --");
    }
}

/*********************
 * F3 Screen - OPTIONS
 ********************/
bool f3_screen(void)
{
    /* cursor positions */
    int invert_analog=1,analog_y,analog_dummy;
    int invert_digital=1,digital_y,digital_dummy;
    int invert_lcd=1,lcd_y,lcd_dummy;
    int invert_full=1,full_y,full_dummy;

    done = false;

    while (!done)
    {
        rb->lcd_clear_display();

        rb->lcd_puts(0, 1, "UP/DN: move, L/R:");
        rb->lcd_puts(0, 2, "change, OFF: done");

        draw_settings();

        if(settings.clock == 1)
        {
            /* Draw line selector */
            switch(invert_analog)
            {
                case 1: cursor(0, 32, 112, 8); break;
                case 2: cursor(0, 40, 112, 8); break;
                case 3: cursor(0, 48, 112, 8); break;
                case 4: cursor(0, 56, 112, 8); break;
            }
            rb->lcd_update();

            switch(rb->button_get_w_tmo(HZ/4))
            {
                case BUTTON_UP:
                    if(invert_analog > 1)
                    {
                        analog_y = 32+(8*(invert_analog-1));
                        analog_dummy = analog_y;
                        for(; analog_y>analog_dummy-8; analog_y--)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, analog_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_analog--;
                    }
                    break;

                case BUTTON_DOWN:
                    if(invert_analog < 4)
                    {
                        analog_y = 32+(8*(invert_analog-1));
                        analog_dummy = analog_y;
                        for(; analog_y<analog_dummy+8; analog_y++)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, analog_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_analog++;
                    }
                    break;

                case BUTTON_LEFT:
                    if(invert_analog == 1)
                        settings.analog_digits = false;
                    else if(invert_analog == 2)
                    {
                        if(settings.analog_date > 0)
                            settings.analog_date--;
                    }
                    else if(invert_analog == 3)
                    {
                        if(settings.analog_time > 0)
                            settings.analog_time--;
                    }
                    else
                        settings.analog_secondhand = false;
                    break;

                case BUTTON_RIGHT:
                    if(invert_analog == 1)
                        settings.analog_digits = true;
                    else if(invert_analog == 2)
                    {
                        if(settings.analog_date < 2)
                            settings.analog_date++;
                    }
                    else if(invert_analog == 3)
                    {
                        if(settings.analog_time < 2)
                            settings.analog_time++;
                    }
                    else
                        settings.analog_secondhand = true;
                    break;

                case BUTTON_F3:
                case BUTTON_OFF:
                    if(settings.save_mode == 2)
                        save_settings(false);
                    done = true;
                    break;
            }
        }
        else if(settings.clock == 2)
        {
            /* Draw a line selector */
            switch(invert_digital)
            {
                case 1: cursor(0, 32, 112, 8); break;
                case 2: cursor(0, 40, 112, 8); break;
                case 3: cursor(0, 48, 112, 8); break;
                case 4: cursor(0, 56, 112, 8); break;
            }
            rb->lcd_update();

            switch(rb->button_get_w_tmo(HZ/4))
            {
                case BUTTON_UP:
                    if(invert_digital > 1)
                    {
                        digital_y = 32+(8*(invert_digital-1));
                        digital_dummy = digital_y;
                        for(; digital_y>digital_dummy-8; digital_y--)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, digital_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_digital--;
                    }
                    break;

                case BUTTON_DOWN:
                    if(invert_digital < 4)
                    {
                        digital_y = 32+(8*(invert_digital-1));
                        digital_dummy = digital_y;
                        for(; digital_y<digital_dummy+8; digital_y++)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, digital_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_digital++;
                    }
                    break;

                case BUTTON_LEFT:
                    if(invert_digital == 1)
                    {
                        if(settings.digital_date > 0)
                            settings.digital_date--;
                    }
                    else if(invert_digital == 2)
                    {
                        if(settings.digital_seconds > 0)
                            settings.digital_seconds--;
                    }
                    else if(invert_digital == 3)
                        settings.digital_blinkcolon = false;
                    else
                        settings.digital_12h = false;
                    break;

                case BUTTON_RIGHT:
                    if(invert_digital == 1)
                    {
                        if(settings.digital_date < 2)
                            settings.digital_date++;
                    }
                    else if(invert_digital == 2)
                    {
                        if(settings.digital_seconds < 3)
                            settings.digital_seconds++;
                    }
                    else if(invert_digital == 3)
                        settings.digital_blinkcolon = true;
                    else
                        settings.digital_12h = true;
                    break;

                case BUTTON_F3:
                case BUTTON_OFF:
                    if(settings.save_mode == 2)
                        save_settings(false);
                    done = true;
                    break;
            }
        }
        else if(settings.clock == 3)
        {
            /* Draw a line selector */
            switch(invert_lcd)
            {
                case 1: cursor(0, 32, 112, 8); break;
                case 2: cursor(0, 40, 112, 8); break;
                case 3: cursor(0, 48, 112, 8); break;
                case 4: cursor(0, 56, 112, 8); break;
            }
            rb->lcd_update();

            switch(rb->button_get_w_tmo(HZ/4))
            {
                case BUTTON_UP:
                    if(invert_lcd > 1)
                    {
                        lcd_y = 32+(8*(invert_lcd-1));
                        lcd_dummy = lcd_y;
                        for(; lcd_y>lcd_dummy-8; lcd_y--)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, lcd_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_lcd--;
                    }
                    break;

                case BUTTON_DOWN:
                    if(invert_lcd < 4)
                    {
                        lcd_y = 32+(8*(invert_lcd-1));
                        lcd_dummy = lcd_y;
                        for(; lcd_y<lcd_dummy+8; lcd_y++)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, lcd_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_lcd++;
                    }
                    break;

                case BUTTON_LEFT:
                    if(invert_lcd == 1)
                    {
                        if(settings.lcd_date > 0)
                            settings.lcd_date--;
                    }
                    else if(invert_lcd == 2)
                    {
                        if(settings.lcd_seconds > 0)
                            settings.lcd_seconds--;
                    }
                    else if(invert_lcd == 3)
                        settings.lcd_blinkcolon = false;
                    else
                        settings.lcd_12h = false;
                    break;

                case BUTTON_RIGHT:
                    if(invert_lcd == 1)
                    {
                        if(settings.lcd_date < 2)
                            settings.lcd_date++;
                    }
                    else if(invert_lcd == 2)
                    {
                        if(settings.lcd_seconds < 3)
                            settings.lcd_seconds++;
                    }
                    else if(invert_lcd == 3)
                        settings.lcd_blinkcolon = true;
                    else
                        settings.lcd_12h = true;
                    break;

                case BUTTON_F3:
                case BUTTON_OFF:
                    if(settings.save_mode == 2)
                        save_settings(false);
                    done = true;
                    break;
            }
        }
        else if(settings.clock == 4)
        {
            /* Draw a line selector */
            switch(invert_full)
            {
                case 1: cursor(0, 32, 112, 8); break;
                case 2: cursor(0, 40, 112, 8); break;
                case 3: cursor(0, 48, 112, 8); break;
                case 4: cursor(0, 56, 112, 8); break;
            }

            rb->lcd_update();

            switch(rb->button_get_w_tmo(HZ/4))
            {
                case BUTTON_UP:
                    if(invert_full > 1)
                    {
                        full_y = 32+(8*(invert_full-1));
                        full_dummy = full_y;
                        for(; full_y>full_dummy-8; full_y--)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, full_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_full--;
                    }
                    break;

                case BUTTON_DOWN:
                    if(invert_full < 3)
                    {
                        full_y = 32+(8*(invert_full-1));
                        full_dummy = full_y;
                        for(; full_y<full_dummy+8; full_y++)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 32, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);
                            draw_settings();
                            cursor(0, full_y, 112, 8);
                            rb->lcd_update();
                        }
                        invert_full++;
                    }
                    break;

                case BUTTON_LEFT:
                    if(invert_full == 1)
                        settings.fullscreen_border = false;
                    else if(invert_full == 2)
                        settings.fullscreen_secondhand = false;
                    else if(invert_full ==3)
                        settings.fullscreen_invertseconds = false;
                    break;

                case BUTTON_RIGHT:
                    if(invert_full == 1)
                        settings.fullscreen_border = true;
                    else if(invert_full == 2)
                        settings.fullscreen_secondhand = true;
                    else if(invert_full ==3)
                        settings.fullscreen_invertseconds = true;
                    break;

                case BUTTON_F3:
                case BUTTON_OFF:
                    if(settings.save_mode == 2)
                        save_settings(false);
                    done = true;
                    break;
            }
        }
        else
        {
            rb->lcd_update();

            switch(rb->button_get_w_tmo(HZ/4))
            {
                case BUTTON_F3:
                case BUTTON_OFF:
                    done = true;
                    break;
            }
        }
    }
    return true;
}

/**********************************
 * Confirm resetting of settings,
 * used in general_settings()    */
void confirm_reset(void)
{
    bool ask_reset_done = false;

    while(!ask_reset_done)
    {
        rb->lcd_clear_display();

        rb->snprintf(buf, sizeof(buf), "Reset Settings?");
        rb->lcd_getstringsize(buf, &buf_w, &buf_h);
        rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 0, buf);

        rb->lcd_puts(0, 2, "PLAY = Yes");
        rb->lcd_puts(0, 3, "Any Other = No");

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
            case BUTTON_PLAY:
                reset_settings();
                rb->splash(HZ*2, true, "Settings Reset");
                ask_reset_done = true;
                break;

            case BUTTON_F1:
            case BUTTON_F2:
            case BUTTON_F3:
            case BUTTON_DOWN:
            case BUTTON_UP:
            case BUTTON_LEFT:
            case BUTTON_RIGHT:
            case BUTTON_ON:
            case BUTTON_OFF:
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
    int cursorpos=1, cursor_y, cursor_dummy;
    done = false;

    while(!done)
    {
        rb->lcd_clear_display();

        rb->snprintf(buf, sizeof(buf), "General Settings");
        rb->lcd_getstringsize(buf, &buf_w, &buf_h);
        rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 0, buf);

        rb->lcd_puts(2, 2, "Reset Settings");
        rb->lcd_puts(2, 3, "Save Settings");
        rb->lcd_puts(2, 4, "Show Counter");
        if(settings.save_mode == 1) /* save on exit */
            rb->lcd_puts(2, 5, "Save: on Exit");
        else if(settings.save_mode == 2)
            rb->lcd_puts(2, 5, "Save: Automatic");
        else
            rb->lcd_puts(2, 5, "Save: Manually");

        rb->snprintf(buf, sizeof(buf), "UP/DOWN to move");
        rb->lcd_getstringsize(buf, &buf_w, &buf_h);
        rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 48, buf);
        rb->snprintf(buf, sizeof(buf), "L/R to change");
        rb->lcd_getstringsize(buf, &buf_w, &buf_h);
        rb->lcd_putsxy(LCD_WIDTH/2-buf_w/2, 56, buf);

        rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);
        rb->lcd_mono_bitmap(arrow, 1, 25, 8, 6);
        draw_checkbox(settings.display_counter, 1, 33);

        if(settings.save_mode == 1)
            rb->lcd_mono_bitmap(checkbox_onethird, 1, 41, 8, 6);
        else if(settings.save_mode == 2)
            rb->lcd_mono_bitmap(checkbox_twothird, 1, 41, 8, 6);
        else
            rb->lcd_mono_bitmap(checkbox_full, 1, 41, 8, 6);

        switch(cursorpos)
        {
            case 1: cursor(0, 16, 112, 8); break;
            case 2: cursor(0, 24, 112, 8); break;
            case 3: cursor(0, 32, 112, 8); break;
            case 4: cursor(0, 40, 112, 8); break;
        }

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
            case BUTTON_OFF:
            case BUTTON_F3:
                if(settings.save_mode == 2)
                    save_settings(false);
                done = true;
                break;

                case BUTTON_UP:
                    if(cursorpos > 1)
                    {
                        cursor_y = 16+(8*(cursorpos-1));
                        cursor_dummy = cursor_y;
                        for(; cursor_y>cursor_dummy-8; cursor_y--)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 16, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            rb->lcd_puts(2, 2, "Reset Settings");
                            rb->lcd_puts(2, 3, "Save Settings");
                            rb->lcd_puts(2, 4, "Show Counter");
                            if(settings.save_mode == 1) /* save on exit */
                                rb->lcd_puts(2, 5, "Save: on Exit");
                            else if(settings.save_mode == 2)
                                rb->lcd_puts(2, 5, "Save: Automatic");
                            else
                                rb->lcd_puts(2, 5, "Save: Manually");
                            rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 25, 8, 6);
                            draw_checkbox(settings.display_counter, 1, 33);
                            if(settings.save_mode == 1)
                                rb->lcd_mono_bitmap(checkbox_onethird, 1, 41, 8, 6);
                            else if(settings.save_mode == 2)
                                rb->lcd_mono_bitmap(checkbox_twothird, 1, 41, 8, 6);
                            else
                                rb->lcd_mono_bitmap(checkbox_full, 1, 41, 8, 6);

                            cursor(0, cursor_y, 112, 8);
                            rb->lcd_update();
                        }
                        cursorpos--;
                    }
                    break;

                case BUTTON_DOWN:
                    if(cursorpos < 4)
                    {
                        cursor_y = 16+(8*(cursorpos-1));
                        cursor_dummy = cursor_y;
                        for(; cursor_y<cursor_dummy+8; cursor_y++)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 16, 112, 32);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            rb->lcd_puts(2, 2, "Reset Settings");
                            rb->lcd_puts(2, 3, "Save Settings");
                            rb->lcd_puts(2, 4, "Show Counter");
                            if(settings.save_mode == 1) /* save on exit */
                                rb->lcd_puts(2, 5, "Save: on Exit");
                            else if(settings.save_mode == 2)
                                rb->lcd_puts(2, 5, "Save: Automatic");
                            else
                                rb->lcd_puts(2, 5, "Save: Manually");
                            rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 25, 8, 6);
                            draw_checkbox(settings.display_counter, 1, 33);
                            if(settings.save_mode == 1)
                                rb->lcd_mono_bitmap(checkbox_onethird, 1, 41, 8, 6);
                            else if(settings.save_mode == 2)
                                rb->lcd_mono_bitmap(checkbox_twothird, 1, 41, 8, 6);
                            else
                                rb->lcd_mono_bitmap(checkbox_full, 1, 41, 8, 6);

                            cursor(0, cursor_y, 112, 8);
                            rb->lcd_update();
                        }
                        cursorpos++;
                    }
                    break;

            case BUTTON_LEFT:
                if(cursorpos == 3)
                    settings.display_counter = false;
                else
                {
                    if(settings.save_mode > 1)
                    {
                        settings.save_mode--;
                        save_settings(false);
                    }
                }
                break;

            case BUTTON_RIGHT:
                if(cursorpos == 1)
                    confirm_reset();
                else if(cursorpos == 2)
                    save_settings(false);
                else if(cursorpos == 3)
                    settings.display_counter = true;
                else
                {
                    if(settings.save_mode < 3)
                    {
                        settings.save_mode++;
                        save_settings(false);
                    }
                }
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
    int tempyear = 0;

    struct tm* current_time = rb->get_time();

    int fill = LCD_WIDTH * second / 60;

    char moday[8];
    char dateyr[6];
    char tmhrmin[7];
    char tmsec[3];

    if(year == 2004)
        tempyear = 04;

    /* american date readout */
    if(settings.analog_date == 1)
        rb->snprintf(moday, sizeof(moday), "%02d/%02d", month, day);
    else
        rb->snprintf(moday, sizeof(moday), "%02d.%02d", day, month);
    rb->snprintf(dateyr, sizeof(dateyr), "%d", year);
    rb->snprintf(tmhrmin, sizeof(tmhrmin), "%02d:%02d", hour, minute);
    rb->snprintf(tmsec, sizeof(tmsec), "%02d", second);

    /* Analog Extras */
    if(settings.clock == 1)
    {
        /* DIGITS around the face */
        if (settings.analog_digits)
        {
            rb->lcd_putsxy((LCD_WIDTH/2)-6, 0, "12");
            rb->lcd_putsxy(20, (LCD_HEIGHT/2)-4, "9");
            rb->lcd_putsxy((LCD_WIDTH/2)-4, 56, "6");
            rb->lcd_putsxy(86, (LCD_HEIGHT/2)-4, "3");
        }

        /* Digital readout */
        if (settings.analog_time != 0)
        {
            /* HH:MM */
            rb->lcd_putsxy(1, 4, tmhrmin);
            /* SS */
            rb->lcd_putsxy(10, 12, tmsec);

            /* AM/PM indicator */
            if(settings.analog_time == 2)
            {
                if(current_time->tm_hour > 12) /* PM */
                    rb->lcd_mono_bitmap(pm, 96, 1, 15, 8);
                else /* AM */
                    rb->lcd_mono_bitmap(am, 96, 1, 15, 8);
            }
        }

        /* Date readout */
        if(settings.analog_date != 0)
        {
            /* MM-DD (or DD.MM) */
            rb->lcd_putsxy(1, 48, moday);
            rb->lcd_putsxy(3, 56, dateyr);
        }
    }
    else if(settings.clock == 2)
    {
        /* Date readout */
        if(settings.digital_date == 1) /* american mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d/%d/%d", month, day, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH/2)-(w/2), 56, buf);
        }
        else if(settings.digital_date == 2) /* european mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d.%d.%d", day, month, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH/2)-(w/2), 56, buf);
        }

        /* Second readout */
        if(settings.digital_seconds == 1)
        {
            rb->snprintf(buf, sizeof(buf), "%d", second);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH/2)-(w/2), 5, buf);
        }

        /* Second progressbar */
        if(settings.digital_seconds == 2)
        {
            rb->scrollbar(0, 0, 112, 4, 60, 0, second, HORIZONTAL);
        }

        /* Invert the whole LCD as the seconds go */
        if(settings.digital_seconds == 3)
        {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(0, 0, fill, 64);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
    }
    else if(settings.clock == 3) /* LCD mode */
    {
        /* Date readout */
        if(settings.lcd_date == 1) /* american mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d/%d/%d", month, day, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH/2)-(w/2), 56, buf);
        }
        else if(settings.lcd_date == 2) /* european mode */
        {
            rb->snprintf(buf, sizeof(buf), "%d.%d.%d", day, month, year);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH/2)-(w/2), 56, buf);
        }

        /* Second readout */
        if(settings.lcd_seconds == 1)
        {
            rb->snprintf(buf, sizeof(buf), "%d", second);
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH/2)-(w/2), 5, buf);
        }

        /* Second progressbar */
        if(settings.lcd_seconds == 2)
        {
            rb->scrollbar(0, 0, 112, 4, 60, 0, second, HORIZONTAL);
        }

        /* Invert the whole LCD as the seconds go */
        if(settings.lcd_seconds == 3)
        {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(0, 0, fill, 64);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
    }
    else if(settings.clock == 4) /* Fullscreen mode */
    {
        if(settings.fullscreen_border)
        {
            /* Draw the circle */
            for(i=0; i < 60; i+=5)
            {
                rb->lcd_drawpixel(xminute_full[i],
                                  yminute_full[i]);
                rb->lcd_drawrect(xminute_full[i]-1,
                                 yminute_full[i]-1,
                                 3, 3);
            }
        }
        if(settings.fullscreen_invertseconds)
        {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(0, 0, fill, 64);
            rb->lcd_set_drawmode(DRMODE_SOLID);
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

    done = false;

    while(!done)
    {
        rb->lcd_clear_display();

        rb->lcd_puts(0, 0, "MODE SELECTOR");
        rb->lcd_puts(2, 1, "Analog");
        rb->lcd_puts(2, 2, "Digital");
        rb->lcd_puts(2, 3, "LCD");
        rb->lcd_puts(2, 4, "Fullscreen");
        rb->lcd_puts(2, 5, "Binary");
        rb->lcd_puts(0, 6, "UP/DOWN: Choose");
        rb->lcd_puts(0, 7, "PLAY:Go|OFF:Cancel");

        /* draw an arrow next to all of them */
        rb->lcd_mono_bitmap(arrow, 1, 9, 8, 6);
        rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);
        rb->lcd_mono_bitmap(arrow, 1, 25, 8, 6);
        rb->lcd_mono_bitmap(arrow, 1, 33, 8, 6);
        rb->lcd_mono_bitmap(arrow, 1, 41, 8, 6);

        /* draw line selector */
        switch(cursorpos)
        {
            case 1: cursor(0, 8, 112, 8);  break;
            case 2: cursor(0, 16, 112, 8); break;
            case 3: cursor(0, 24, 112, 8); break;
            case 4: cursor(0, 32, 112, 8); break;
            case 5: cursor(0, 40, 112, 8); break;
        }

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
                case BUTTON_UP:
                    if(cursorpos > 1)
                    {
                        cursor_y = 8+(8*(cursorpos-1));
                        cursor_dummy = cursor_y;
                        for(; cursor_y>cursor_dummy-8; cursor_y--)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 8, 112, 40);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            rb->lcd_puts(0, 0, "MODE SELECTOR");
                            rb->lcd_puts(2, 1, "Analog");
                            rb->lcd_puts(2, 2, "Digital");
                            rb->lcd_puts(2, 3, "LCD");
                            rb->lcd_puts(2, 4, "Fullscreen");
                            rb->lcd_puts(2, 5, "Binary");
                            rb->lcd_puts(0, 6, "UP/DOWN: Choose");
                            rb->lcd_puts(0, 7, "PLAY:Go|OFF:Cancel");

                            /* draw an arrow next to all of them */
                            rb->lcd_mono_bitmap(arrow, 1, 9, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 25, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 33, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 41, 8, 6);

                            cursor(0, cursor_y, 112, 8);
                            rb->lcd_update();
                        }
                        cursorpos--;
                    }
                    break;

                case BUTTON_DOWN:
                    if(cursorpos < 5)
                    {
                        cursor_y = 8+(8*(cursorpos-1));
                        cursor_dummy = cursor_y;
                        for(; cursor_y<cursor_dummy+8; cursor_y++)
                        {
                            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                            rb->lcd_fillrect(0, 8, 112, 40);
                            rb->lcd_set_drawmode(DRMODE_SOLID);

                            rb->lcd_puts(0, 0, "MODE SELECTOR");
                            rb->lcd_puts(2, 1, "Analog");
                            rb->lcd_puts(2, 2, "Digital");
                            rb->lcd_puts(2, 3, "LCD");
                            rb->lcd_puts(2, 4, "Fullscreen");
                            rb->lcd_puts(2, 5, "Binary");
                            rb->lcd_puts(0, 6, "UP/DOWN: Choose");
                            rb->lcd_puts(0, 7, "PLAY:Go|OFF:Cancel");

                            /* draw an arrow next to all of them */
                            rb->lcd_mono_bitmap(arrow, 1, 9, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 17, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 25, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 33, 8, 6);
                            rb->lcd_mono_bitmap(arrow, 1, 41, 8, 6);

                            cursor(0, cursor_y, 112, 8);
                            rb->lcd_update();
                        }
                        cursorpos++;
                    }
                    break;

            case BUTTON_PLAY:
            case BUTTON_RIGHT:
                settings.clock = cursorpos;
                done = true;
                break;

            case BUTTON_OFF:
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
        if(led_on)
            or_b(0x40, &PBDRL);
        else
            and_b(~0x40, &PBDRL);
#endif

        /* exit on keypress */
        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
        {
#ifndef SIMULATOR
            and_b(~0x40, &PBDRL); /* shut off the red led */
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
    if(settings.display_counter)
    {
        if(settings.clock == 1)
            rb->lcd_puts(11, 7, count_text);
        else if(settings.clock == 2)
            rb->lcd_putsxy(1, 5, count_text);
        else if(settings.clock == 3)
            rb->lcd_putsxy(1, 5, count_text);
        else if(settings.clock == 4)
            rb->lcd_puts(6, 6, count_text);
    }
}

/******************
 * Counter settings
 *****************/
void counter_options(void)
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
        rb->lcd_puts(0, 0, "Counter Options");
        rb->lcd_puts(2, 2, "Count UP");
        rb->lcd_puts(2, 3, "Count DOWN...");
        rb->lcd_puts(4, 4, "Target Time:");
        rb->lcd_puts(4, 5, target_time);
        rb->lcd_puts(0, 7, "OFF: Return");

        /* tell user what mode is selected */
        rb->checkbox(1, 17, 8, 6, counting_up);
        rb->checkbox(1, 25, 8, 6, !counting_up);

        /* draw a cursor */
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        switch(cursorpos)
        {
            case 1: rb->lcd_fillrect(0, 16, 112, 8); break;
            case 2: rb->lcd_fillrect(0, 24, 112, 8); break;
            case 3: rb->lcd_fillrect(24, 40, 06, 8); break;
            case 4: rb->lcd_fillrect(36, 40, 12, 8); break;
            case 5: rb->lcd_fillrect(54, 40, 12, 8); break;
        }
        rb->lcd_set_drawmode(DRMODE_SOLID);

        if(cursorpos > 2)
            editing_target = true;
        else
            editing_target = false;

        rb->lcd_update();

        /* button scan */
        switch(rb->button_get_w_tmo(HZ/4))
        {
            case BUTTON_OFF: /* exit screen */
                current = counting_up;
                if(current != original)
                    counter = 0;
                done = true;
                break;

            case BUTTON_UP: /* increase / move cursor */
            case BUTTON_UP | BUTTON_REPEAT:
                if(!editing_target)
                {
                    if(cursorpos > 1)
                        cursorpos--;
                }
                else
                {
                    if(cursorpos == 3)
                        if(target_hour < 9)
                            target_hour++;
                        else
                            target_hour = 0;
                    else if(cursorpos == 4)
                        if(target_minute < 59)
                            target_minute++;
                        else
                            target_minute = 0;
                    else
                        if(target_second < 59)
                            target_second++;
                        else
                            target_second = 0;
                }
                break;

            case BUTTON_DOWN: /* decrease / move cursor */
            case BUTTON_DOWN | BUTTON_REPEAT:
                if(!editing_target)
                {
                    if(cursorpos < 3)
                        cursorpos++;
                }
                else
                {
                    if(cursorpos == 3)
                        if(target_hour > 0)
                            target_hour--;
                        else
                            target_hour = 9;
                    else if(cursorpos == 4)
                        if(target_minute > 0)
                            target_minute--;
                        else
                            target_minute = 59;
                    else
                        if(target_second > 0)
                            target_second--;
                        else
                            target_second = 59;
                }
                break;

            case BUTTON_LEFT: /* move cursor */
                if(cursorpos > 3)
                    cursorpos--;
                else
                    cursorpos = 5;
                break;

            case BUTTON_RIGHT: /* move cursor */
                if(cursorpos < 5)
                    cursorpos++;
                else
                    cursorpos = 3;
                break;

            case BUTTON_PLAY: /* toggle */
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
        }
    }
}

/**********************************************************************
 * Plugin starts here
 **********************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;

    /* time ints */
    int i;
    int hour;
    int minute;
    int second;
    int temphour;
    int last_second = -1;
    int pos = 0;

    /* date ints */
    int year;
    int day;
    int month;

    /* poweroff activated or not? */
    bool reset_timer = false;

    bool f2_held = false;

    struct tm* current_time;

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;
    
    /* universal font */
    rb->lcd_setfont(FONT_SYSFIXED);

    load_settings();

    /* set backlight timeout */
    rb->backlight_set_timeout(settings.backlight_on);

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

            /* flash counter if needed
            if(!counting && displayed_value != 0)
                display_counter = !display_counter;
            else
                display_counter = true; */

            /* and then print it */
            show_counter();

            /*************
             * Analog Mode
             ************/
            if(settings.clock == 1)
            {
                /* Second hand */
                if(settings.analog_secondhand)
                {
                    pos = 90-second;
                    if(pos >= 60)
                        pos -= 60;

                    rb->lcd_drawline((LCD_WIDTH/2), (LCD_HEIGHT/2),
                                     xminute[pos], yminute[pos]);
                }

                pos = 90-minute;
                if(pos >= 60)
                    pos -= 60;

                /* Minute hand, thicker than the second hand */
                rb->lcd_drawline(LCD_WIDTH/2, LCD_HEIGHT/2,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2-1,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2+1,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2+1,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2-1,
                                 xminute[pos], yminute[pos]);

                if(hour > 12)
                    hour -= 12;

                hour = hour*5 + minute/12;;
                pos = 90-hour;
                if(pos >= 60)
                    pos -= 60;

                /* Hour hand, thick as the minute hand but shorter */
                rb->lcd_drawline(LCD_WIDTH/2, LCD_HEIGHT/2, xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2-1,
                                 xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2+1,
                                 xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2+1,
                                 xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2-1,
                                 xhour[pos], yhour[pos]);

                /* Draw the circle */
                for(i=0; i < 60; i+=5)
                {
                    rb->lcd_drawpixel(xminute[i],
                                      yminute[i]);
                    rb->lcd_drawrect(xminute[i]-1,
                                     yminute[i]-1,
                                     3, 3);
                }

                /* Draw the cover over the center */
                rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)+3,
                                 (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)+3);
                rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)+2,
                                 (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)+2);
                rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)+1,
                                 (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)+1);
                rb->lcd_drawline((LCD_WIDTH/2)-4, LCD_HEIGHT/2,
                                 (LCD_WIDTH/2)+4, LCD_HEIGHT/2);
                rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)-1,
                                 (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)-1);
                rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)-2,
                                 (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)-2);
                rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)-3,
                                 (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)-3);
                rb->lcd_drawpixel(LCD_WIDTH/2, LCD_HEIGHT/2);
            }
            /**************
             * Digital mode
             *************/
            else if(settings.clock == 2)
            {
                if(settings.digital_blinkcolon)
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, second & 1, false);
                else
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, true, false);
            }
            /**********
             * LCD mode
             *********/
            else if(settings.clock == 3)
            {
                if(settings.lcd_blinkcolon)
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, second & 1, true);
                else
                    draw_7seg_time(hour, minute, 8, 16, 16, 32, true, true);
            }
            /*****************
             * Fullscreen mode
             ****************/
            else if(settings.clock == 4)
            {
                /* Second hand */
                if(settings.fullscreen_secondhand)
                {
                    pos = 90-second;
                    if(pos >= 60)
                        pos -= 60;

                    rb->lcd_drawline((LCD_WIDTH/2), (LCD_HEIGHT/2),
                                     xminute_full[pos], yminute_full[pos]);
                }

                pos = 90-minute;
                if(pos >= 60)
                    pos -= 60;

                /* Minute hand, thicker than the second hand */
                rb->lcd_drawline(LCD_WIDTH/2, LCD_HEIGHT/2,
                                 xminute_full[pos], yminute_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2-1,
                                 xminute_full[pos], yminute_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2+1,
                                 xminute_full[pos], yminute_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2+1,
                                 xminute_full[pos], yminute_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2-1,
                                 xminute_full[pos], yminute_full[pos]);

                if(hour > 12)
                    hour -= 12;

                hour = hour*5 + minute/12;
                pos = 90-hour;
                if(pos >= 60)
                    pos -= 60;

                /* Hour hand, thick as the minute hand but shorter */
                rb->lcd_drawline(LCD_WIDTH/2, LCD_HEIGHT/2, xhour_full[pos], yhour_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2-1,
                                 xhour_full[pos], yhour_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2+1,
                                 xhour_full[pos], yhour_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2+1,
                                 xhour_full[pos], yhour_full[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2-1,
                                 xhour_full[pos], yhour_full[pos]);

                /* Draw the cover over the center */
                rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)+3,
                                 (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)+3);
                rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)+2,
                                 (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)+2);
                rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)+1,
                                 (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)+1);
                rb->lcd_drawline((LCD_WIDTH/2)-4, LCD_HEIGHT/2,
                                 (LCD_WIDTH/2)+4, LCD_HEIGHT/2);
                rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)-1,
                                 (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)-1);
                rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)-2,
                                 (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)-2);
                rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)-3,
                                 (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)-3);
                rb->lcd_drawpixel(LCD_WIDTH/2, LCD_HEIGHT/2);
            }
            /*************
             * Binary mode
             ************/
            else
            {
                binary(hour, minute, second);
            }
        }

        if(settings.analog_time == 2 && temphour == 0)
            temphour = 12;
        if(settings.analog_time == 2 && temphour > 12)
            temphour -= 12;

        draw_extras(year, day, month, temphour, minute, second);

        if(reset_timer)
            rb->reset_poweroff_timer();

        rb->lcd_update();

        /*************************
         * Scan for button presses
         ************************/
        button = rb->button_get_w_tmo(HZ/10);
        switch (button)
        {
            case BUTTON_OFF: /* save and exit */
                cleanup(NULL);
                return PLUGIN_OK;

            case BUTTON_ON | BUTTON_REL: /* credit roll */
                show_credits();
                break;

            case BUTTON_ON | BUTTON_F2: /* counter options */
                counter_options();
                break;

            case BUTTON_ON | BUTTON_F3: /* general settings */
                general_settings();
                break;

            case BUTTON_F1: /* help */
                f1_screen();
                break;

            case BUTTON_F2 | BUTTON_REL: /* start/stop counter */
                if(settings.display_counter)
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

            case BUTTON_F2 | BUTTON_REPEAT: /* reset counter */
                if(settings.display_counter)
                {
                    f2_held = true;  /* Ignore the release event */
                    counter = 0;
                    start_tick = *rb->current_tick;
                }
                break;

            case BUTTON_F3: /* options */
                f3_screen();
                break;

            case BUTTON_UP: /* enable idle poweroff */
                reset_timer = false;
                rb->splash(HZ*2, true, "Idle Poweroff ENABLED");
                break;

            case BUTTON_DOWN: /* disable idle poweroff */
                reset_timer = true;
                rb->splash(HZ*2, true, "Idle Poweroff DISABLED");
                break;

            case BUTTON_LEFT: /* backlight off */
                settings.backlight_on = false;
                rb->splash(HZ, true, "Backlight OFF");
                rb->backlight_set_timeout(settings.backlight_on);
                break;

            case BUTTON_RIGHT: /* backlight on */
                settings.backlight_on = true;
                rb->splash(HZ, true, "Backlight ON");
                rb->backlight_set_timeout(settings.backlight_on);
                break;

            case BUTTON_PLAY: /* select a mode */
                select_mode();
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
