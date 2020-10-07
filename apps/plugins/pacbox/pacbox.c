/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Copyright (c) 1997-2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
 * AI code (c) 2017 Moshe Piekarski
 *
 * ToDo convert all score to pinky location
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

#include "plugin.h"
#include "arcade.h"
#include "pacbox.h"
#include "pacbox_lcd.h"
#include "wsg3.h"
#include "lib/configfile.h"
#include "lib/playback_control.h"
#include "lib/helper.h"
static fb_data *lcd_fb;

/*Allows split screen jump and makes pacman invincible if you start at 18 credits (for testing purposes)*/
//#define CHEATS 1

/* Enable AI on all targets */
#define AI 1

struct pacman_settings {
    int difficulty;
    int numlives;
    int bonus;
    int ghostnames;
    int showfps;
    int sound;
    int ai;
};

static struct pacman_settings settings;
static struct pacman_settings old_settings;
static bool sound_playing = false;

#define SETTINGS_VERSION 2
#define SETTINGS_MIN_VERSION 2
#define SETTINGS_FILENAME "pacbox.cfg"

static char* difficulty_options[] = { "Normal", "Hard" };
static char* numlives_options[] = { "1", "2", "3", "5" };
static char* bonus_options[] = {"10000", "15000", "20000", "No Bonus"};
static char* ghostnames_options[] = {"Normal", "Alternate"};
static char* yesno_options[] = {"No", "Yes"};

#ifdef AI
static unsigned char ai_direction[15][205] =              /* level turn directions */
{
    {2,1,3,1,2,0,3,0,3,1,2,1,3,1,3,0,3,0,2,0,3,0,2,1,3,1,2,0,2,1,3,1,3,0,3,1,3,1,2,0,3,0,3,0,3,0,2,0,3,1,2,0,2,0,3,1,2,1,2,1,2,0,2,0,2,0,3,1,3,1,2,1,2,1,3,0,2,0,2,0,2,1,2,0,3,0,2,1,3,0,3,2,1,3,0,3,1,2,1,2,0,2,3,1,3,0,3,0,2,4}, /* first level */
    {2,0,3,1,3,1,2,0,2,0,2,0,3,1,3,1,2,1,2,0,3,0,3,0,3,1,3,1,2,1,2,0,2,0,3,0,2,0,3,1,2,0,3,1,2,0,2,1,2,1,2,0,2,0,3,1,0,3,0,3,2,0,2,0,2,0,3,1,0,3,2,0,2,1,2,1,3,0,3,1,3,1,3,0,3,0,1,2,1,3,1,3,1,2},/* second level*/
    {2,1,3,1,2,0,2,0,3,0,2,1,3,1,2,1,2,0,3,0,3,0,3,1,3,1,2,1,2,0,2,0,2,3,0,3,0,3,0,2,0,3,1,2,0,2,0,3,1,2,1,2,1,3,1,3,1,2,0,2,1,3,1,3,0,3,0,2,0,3,1,2,1,2,0,3,1,3,1,2,1,2,0,2,0,2,0,3,1,2,0,2,1,3,0,1,2,1,2,0},/*third level*/
    {2,1,3,1,2,0,2,0,3,0,2,1,3,1,2,1,2,0,3,0,3,0,3,1,3,1,2,1,2,0,2,0,2,3,0,3,0,3,0,2,0,3,1,2,0,2,0,3,1,2,1,2,1,3,1,3,1,2,0,2,0,3,1,3,1,2,1,2,0,2,0,3,0,2,0,3,0,2,1,2,1,3,0,3,0,2,1,0,3,1,3,1,2,1,2,1,3,1,3,0,3,0,2,0,2,1,2,1,3,1,3,0,3,1,3,2,0,2,0,2,0,2},/*level four*/
    {2,2,1,2,1,3,0,3,0,2,0,2,0,3,0,2,0,2,0,3,1,2,1,3,1,2,1,3,1,2,1,2,1,3,1,3,0,2,0,3,1,2,1,2,0,2,0,3,0,2,0,3,1,3,0,2,1,2,1,2,0,2,0,2,0,3,0,3,1,3,1,2,1,3,1,3,0,3,0,2,0,2,1,3,1,3,0,2,1,2,0,3,1,3,1,3,1,2,1,2,0,2,1,2,0,2,0,3,1,3,0,2,0,2},/*levels 5,7,8,11*/
    {2,2,1,2,1,3,0,3,0,2,0,2,0,3,0,2,0,2,0,3,1,2,1,3,1,2,1,3,1,2,1,2,1,3,1,3,0,2,0,3,1,2,1,2,0,2,0,3,0,2,0,3,1,3,0,2,1,2,1,2,0,3,0,2,0,3,1,3,0,2,1,2,1,2,0,2,0,3,0,2,1,2,0,3,0,2,1,3,0,2,1,3,1,3,0,3,1,2,1,2,1,2,0,2,0,3,1,3,0,1,0,3,1},/*level six*/
    {2,1,3,1,2,0,2,0,3,0,2,0,3,0,3,1,2,0,2,0,2,1,3,0,3,1,3,0,2,1,2,0,3,1,3,1,2,1,3,1,3,0,2,0,3,1,2,1,3,1,2,0,3,0,2,0,3,1,2,0,3,1,2,1,2,1,2,0,2,0,2,0,3,0,3,1,3,1,3,0,3,1,2,1,3,1,3,1,2,0,3,0,2,1,3,1,2,0,3,0,2,1,3,1,3,2,1,3,0,2,0,2,0,2,1,3,1,2,1,3,1,2,0,3,0,2,0,3,1,3,1,2,0,2,0,3,1,2,0,3,0,2,1,3,0,3,1,2,1,3,1,2,1,2,1,2,0,2,1,3,0,2,1,2,1,2,0,2,1,2,0,2,1,2,0,2,0,3,0,2,0,3,1,2,0,2,1,3,1,2,1,3,1,3,0,2},/*level nine*/
    {2,2,1,2,1,3,0,3,0,2,0,2,0,3,0,2,0,2,0,3,1,2,1,3,1,2,1,3,1,2,1,2,1,3,1,3,0,2,0,3,1,2,1,2,0,2,0,3,0,2,0,3,1,3,0,2,1,2,1,2,0,3,0,2,0,3,1,2,0,2,1,2,1,2,1,2,0,2,0,3,0,2,1,3,0,3,0,1,2,1,3,1,2,1,0,3,0,2,0,2,1,2,0,2,0,3,1,3,0,2,1,2,1,2,0,3,0,2,0,3,1,2,0,2,1,2,1,2,1,2,0,2,0,3,0,2,1,3,0,3,0,1,2,1,3,1,2,1,0,3,0,2,0,2,1,2,0,2,0,3,1,3,0,2,1,2,1,2,},/*level ten*/
    {2,1,3,1,2,0,2,0,3,0,2,0,3,0,3,1,2,0,2,0,2,1,3,0,3,1,3,0,2,1,2,0,3,1,3,1,2,1,3,1,3,0,2,0,3,1,2,1,3,1,2,0,3,0,2,0,3,1,2,0,3,1,2,1,2,1,2,0,2,0,2,0,3,0,3,1,3,1,3,0,3,1,2,1,3,1,3,1,2,0,3,0,2,1,3,1,2,0,3,0,2,1,3,1,3,2,1,3,0,2,0,2,0,2,1,3,1,2,1,3,1,2,0,3,0,2,0,3,1,3,1,2,0,2,0,3,1,2,0,3,0,2,1,3,0,1,3,0,3,1,3,0,2,1,3,1,3,1,2,1,2,1,3,0,2,1,2,1,2,0,2,0,2,0,3,1,2,1,2,1,2,0,2,0,2,0,3,1,3,1,2,0,2},/*level twelve*/
    {2,2,1,2,1,3,0,3,0,2,0,2,0,3,0,2,0,2,0,3,1,2,1,3,1,2,1,3,1,2,1,2,1,3,1,3,0,2,0,3,1,2,1,2,0,2,0,3,0,2,0,3,1,2,0,3,1,2,0,2,3,1,2,1,2,1,3,1,2,0,2,0,2,0,3,1,2,1,2,1,2,0,2,1,2,1,3,1,3,0,3,0,3,0,3,0,2,1,3,1,3,1,3,0,3,0,2,0,3,0,2,1,3,0,3,1,3,2,1},/*level fourteen*/
    {2,1,3,1,2,0,2,0,3,0,2,0,3,0,3,1,2,0,2,0,2,1,3,0,3,1,3,0,2,1,2,0,3,1,3,1,2,1,3,1,3,0,2,0,3,1,2,1,3,1,2,0,3,0,2,0,3,1,2,0,3,1,2,1,2,1,2,0,2,0,2,0,3,0,3,1,3,1,3,0,3,1,2,1,3,1,3,1,2,0,3,0,2,1,3,1,2,0,3,0,2,1,3,1,3,2,1,3,0,2,0,2,0,2,1,3,1,2,1,3,1,2,0,3,0,2,0,3,1,3,1,2,0,3,0,3,1,2,1,3,1,3,0,2,0,3,0,2,0,3,0,2,1,3,1,3,0,3,0,3,0,2,1,3,1,2,1,3,0,3,1,2,0,3,0,2,0,2,0,3,1,0,1,2,1,3,0,2,0,2,1,2,0,2,1,3,2},
    {2,2,1,2,1,3,0,3,0,2,0,3,0,2,0,2,0,3,0,2,1,3,1,2,1,3,1,3,1,3,1,2,0,2,0,2,0,3,1,2,0,2,0,2,0,3,0,3,1,3,0,3,1,2,1,2,0,2,0,3,0,2,1,3,1,2,0,2,0,2,1,3,1,2,1,3,1,2,1,3,0,2,0,3,0,2},
    {2,2,1,2,1,3,0,3,0,2,0,3,0,2,0,2,0,3,0,2,1,3,1,2,1,3,1,3,1,3,1,2,0,2,0,2,0,3,1,2,0,2,0,2,0,3,0,3,1,3,1,2,1,3,1,2,1,2,1,3,1,3,0,2,0,3,1,3,0,2,1,3,1,2,0,2,1,3,1,2,1,3,1,3,1,3,1,2,1},
    {2,1,3,1,2,0,3,0,3,1,2,1,3,1,3,0,3,0,3,0,2,1,3,1,3,0,3,0,2,0,2,1,2,1,3,1,2,1,2,0,2,0,3,0,2,0,3,0,3,1,2,0,2,1,3,1,3,1,3,0,3,0,2,1,3,1,2,1,2,0,2,0,3,0,3,1,2,1,3,0},
    {2,1,3,1,2,0,3,0,2,0,3,1,2,0,3,1,2,1,2,0,2,0,2,0,3,0,3,0,2,1,3,0,2,1,3,1,2,1,2,1,2,0,2,1,2,1,2,1,3,0,1,3,0,1,2,1,2,1,2,0,3,1,2}
};
static unsigned char ai_location[15][205] =               /* level turn locations */
{
    {0,52,58,57,61,34,60,37,54,43,55,42,58,43,61,46,60,49,57,48,54,49,51,42,52,43,55,39,54,34,55,34,58,37,26,52,58,57,61,48,60,49,57,52,42,57,39,48,35,57,40,54,39,48,35,52,37,51,40,48,43,45,42,42,39,39,35,43,37,49,40,48,43,42,49,49,45,45,42,42,39,39,40,34,39,43,35,34,40,37,39,3,39,55,52,54,57,55,57,58,54,57,3,52,58,55,57,57,54,53},   /* first level */
    {0,47,54,52,58,57,61,45,60,42,57,39,54,43,55,49,58,48,61,34,60,37,54,40,51,49,52,57,55,57,58,54,54,51,48,52,39,48,35,57,40,54,39,57,40,54,35,48,37,40,56,37,57,33,54,37,2,54,40,51,4,42,48,39,39,34,35,37,1,39,4,45,35,39,37,34,40,37,39,40,40,43,43,46,42,49,16,40,48,42,48,45,52,55}, /*second level */
    {0,52,58,57,61,45,60,42,57,43,54,39,55,49,58,48,61,34,60,37,54,40,51,49,52,57,55,57,58,54,54,51,51,4,49,48,52,42,57,39,48,35,57,40,54,39,48,35,52,37,51,40,48,43,49,46,52,55,39,54,34,55,34,58,37,57,43,54,42,51,49,52,48,55,39,35,43,37,49,40,48,43,45,42,42,39,39,35,43,37,39,35,34,37,43,3,37,39,40,34}, /*third level */
    {0,52,58,57,61,45,60,42,57,43,54,39,55,49,58,48,61,34,60,37,54,40,51,49,52,57,55,57,58,54,54,51,51,4,49,48,52,42,57,39,48,35,57,40,54,39,48,35,52,37,51,40,48,43,49,46,52,55,45,54,39,48,40,49,49,52,48,55,45,54,42,45,43,42,42,39,43,35,39,37,34,40,37,39,43,35,34,1,35,37,37,49,40,48,43,42,52,43,55,46,54,49,51,42,48,39,52,34,55,34,58,37,54,43,55,4,45,54,42,48,39,1}, /*fourth level */
    {0,14,39,58,34,61,46,60,49,57,45,54,42,45,43,42,42,39,39,35,43,37,42,40,43,43,42,49,49,52,48,55,42,58,43,61,57,60,54,54,57,55,57,58,54,57,48,54,52,39,48,35,52,37,57,35,54,46,51,49,42,48,39,42,34,39,37,35,43,37,49,40,48,43,49,46,52,42,57,39,39,35,34,37,40,3,43,35,39,40,34,39,40,40,43,43,49,52,48,55,45,54,39,58,36,57,34,54,37,55,43,54,39,51}, /*fifth level */
    {0,14,39,58,34,61,46,60,49,57,45,54,42,45,43,42,42,39,39,35,43,37,42,40,43,43,42,49,49,52,48,55,42,58,43,61,57,60,54,54,57,55,57,58,54,57,48,54,52,39,48,35,52,37,57,35,54,46,51,49,42,45,43,42,42,39,52,40,57,39,51,40,48,43,45,42,42,39,43,35,39,40,34,39,37,35,34,37,37,35,34,3,37,46,40,45,49,49,42,52,39,58,36,57,34,54,37,55,52,4,54,48,37,},/*sixth level*/
    {0,52,58,57,61,45,60,42,57,43,54,42,51,49,48,52,55,45,54,42,48,39,52,40,45,49,52,52,48,51,49,42,45,49,52,52,55,42,58,43,61,57,60,54,54,57,55,57,58,57,61,48,60,49,57,48,54,52,55,48,54,52,55,51,58,48,61,45,60,42,57,39,48,40,45,49,46,40,49,49,48,52,52,48,55,52,58,57,61,48,60,49,57,42,58,43,61,34,60,37,54,34,55,34,58,1,34,61,43,60,42,57,39,48,54,52,57,55,57,58,57,61,48,60,49,57,39,48,40,52,43,55,39,42,34,39,37,40,34,39,37,35,34,37,37,35,43,37,42,40,43,43,42,46,51,49,42,48,39,55,52,48,51,49,42,52,39,48,54,52,51,51,42,52,39,48,54,42,57,39,54,35,57,37,4,35,48,37,49,40,48,43,49,46,52,39},/*ninth level*/
    {0,14,39,58,34,61,46,60,49,57,45,54,42,45,43,42,42,39,39,35,43,37,42,40,43,43,42,49,49,52,48,55,42,58,43,61,57,60,54,54,57,55,57,58,54,57,48,54,52,39,48,35,52,37,57,35,54,46,51,49,42,45,43,42,42,39,57,40,54,39,51,40,48,43,42,46,39,42,34,39,37,35,34,40,37,39,43,3,37,42,40,43,43,42,4,45,43,42,42,39,39,58,36,57,34,54,37,55,52,48,51,49,42,52,45,60,},/*tenth level*/
    {0,52,58,57,61,45,60,42,57,43,54,42,51,49,48,52,55,45,54,42,48,39,52,40,45,49,52,52,48,51,49,42,45,49,52,52,55,42,58,43,61,57,60,54,54,57,55,57,58,57,61,48,60,49,57,48,54,52,55,48,54,52,55,51,58,48,61,45,60,42,57,39,48,40,45,49,46,40,49,49,48,52,52,48,55,52,58,57,61,48,60,49,57,42,58,43,61,34,60,37,54,34,55,34,58,1,34,61,43,60,42,57,39,48,54,52,57,55,57,58,57,61,48,60,49,57,39,48,40,52,43,55,39,42,34,39,37,40,34,39,37,35,34,37,37,4,46,40,45,49,46,52,39,42,40,43,43,49,49,42,52,39,55,52,48,51,49,42,52,39,48,54,39,48,35,57,37,51,40,48,43,45,42,42,39,39,35,43,37,57,40,54,35},/*twelfth level*/
    {0,14,39,58,34,61,46,60,49,57,45,54,42,45,43,42,42,39,39,35,43,37,42,40,43,43,42,49,49,52,48,55,42,58,43,61,57,60,54,54,57,55,57,58,54,57,48,54,52,39,48,35,52,37,48,35,57,37,54,35,4,52,37,51,40,48,43,49,49,42,48,39,42,34,39,57,40,54,46,51,49,42,48,39,52,34,55,34,58,37,54,40,51,49,48,52,39,42,40,43,43,49,52,52,48,37,42,34,39,37,35,34,37,37,35,43,37,4,39},/*fourteenth level*/
    {0,52,58,57,61,45,60,42,57,43,54,42,51,49,48,52,55,45,54,42,48,39,52,40,45,45,52,52,48,51,49,42,45,49,52,52,55,42,58,43,61,57,60,54,54,57,55,57,58,57,61,48,60,49,57,48,54,52,55,48,54,52,55,51,58,48,61,45,60,42,57,39,48,40,45,49,46,40,49,49,48,52,52,48,55,52,58,57,61,48,60,49,57,42,58,43,61,34,60,37,54,34,55,34,58,1,34,61,43,60,42,57,39,48,54,52,57,55,57,58,57,61,48,60,49,57,39,48,40,52,43,55,39,54,40,45,49,49,42,52,43,55,52,54,51,48,37,41,34,39,37,35,34,37,37,46,40,45,46,42,49,39,42,40,43,43,42,49,49,48,52,55,39,54,40,48,54,39,48,35,57,2,3,37,54,40,57,39,54,35,48,37,45,35,39,37,40},
    {0,14,39,58,34,61,46,60,49,57,48,54,52,48,51,45,48,42,49,39,42,40,43,43,42,49,49,52,52,58,57,61,45,60,42,57,39,39,52,55,45,54,39,42,34,39,37,35,43,37,46,35,52,46,51,49,42,48,54,42,57,35,54,37,57,40,54,39,45,35,34,40,37,52,34,55,34,58,34,61,57,60,57,57,57,53},
    {0,14,39,58,34,61,46,60,49,57,48,54,52,48,51,45,48,42,49,39,42,40,43,43,42,49,49,52,52,58,57,61,45,60,42,57,39,39,52,55,45,54,39,42,34,39,37,35,43,37,49,40,48,43,49,49,42,52,34,55,34,58,37,42,34,35,43,37,57,35,54,37,57,40,54,35,48,37,49,40,48,43,49,46,52,52,57,55,57},
    {0,52,58,57,61,34,60,37,54,43,55,42,58,43,61,46,60,49,57,52,54,48,55,52,58,55,57,57,54,54,48,51,49,42,52,43,55,39,58,36,57,34,54,37,42,34,39,37,35,43,37,39,35,34,37,37,46,40,49,49,48,52,35,48,37,49,40,48,43,45,42,42,39,52,35,57,37,54,40,57},
    {0,52,58,57,61,48,60,49,57,48,54,52,55,48,54,57,55,57,58,54,48,51,45,48,42,49,39,52,35,48,37,52,34,48,37,52,46,51,49,45,37,39,33,38,35,35,49,34,60,37,53,60,42,55,60,38,35,58,40,54,35,57,37}
};
#endif

static struct configdata config[] =
{
   {TYPE_ENUM, 0, 2, { .int_p = &settings.difficulty }, "Difficulty",
    difficulty_options},
   {TYPE_ENUM, 0, 4, { .int_p = &settings.numlives }, "Pacmen Per Game",
    numlives_options},
   {TYPE_ENUM, 0, 4, { .int_p = &settings.bonus }, "Bonus", bonus_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.ghostnames }, "Ghost Names",
    ghostnames_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.showfps }, "Show FPS",
    yesno_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.sound }, "Sound",
    yesno_options},
#ifdef AI
   {TYPE_ENUM, 0, 2, { .int_p = &settings.ai }, "AI",
    yesno_options},
#endif
};

static bool loadFile( const char * name, unsigned char * buf, int len )
{
    char filename[MAX_PATH];

    rb->snprintf(filename,sizeof(filename), ROCKBOX_DIR "/pacman/%s",name);

    int fd = rb->open( filename, O_RDONLY);

    if( fd < 0 ) {
        return false;
    }

    int n = rb->read( fd, buf, len);

    rb->close( fd );

    if( n != len ) {
        return false;
    }

    return true;
}

static bool loadROMS( void )
{
    bool romsLoaded = false;

    romsLoaded = loadFile( "pacman.6e", ram_,           0x1000) &&
                 loadFile( "pacman.6f", ram_+0x1000,    0x1000) &&
                 loadFile( "pacman.6h", ram_+0x2000,    0x1000) &&
                 loadFile( "pacman.6j", ram_+0x3000,    0x1000) &&
                 loadFile( "pacman.5e", charset_rom_,   0x1000) &&
                 loadFile( "pacman.5f", spriteset_rom_, 0x1000);

    if( romsLoaded ) {
        decodeROMs();
        reset_PacmanMachine();
    }

    return romsLoaded;
}

/* A buffer to render Pacman's 244x288 screen into */
static unsigned char video_buffer[ScreenWidth*ScreenHeight] __attribute__ ((aligned (16)));

static long start_time;
static long video_frames = 0;

static int dipDifficulty[] = { DipDifficulty_Normal, DipDifficulty_Hard };
static int dipLives[] = { DipLives_1, DipLives_2, DipLives_3, DipLives_5 };
static int dipBonus[] = { DipBonus_10000, DipBonus_15000, DipBonus_20000, 
                          DipBonus_None };
static int dipGhostNames[] = { DipGhostNames_Normal, DipGhostNames_Alternate };

static int settings_to_dip(struct pacman_settings settings)
{
    return ( DipPlay_OneCoinOneGame | 
             DipCabinet_Upright | 
             DipMode_Play |
             DipRackAdvance_Off |

             dipDifficulty[settings.difficulty] |
             dipLives[settings.numlives] |
             dipBonus[settings.bonus] |
             dipGhostNames[settings.ghostnames]
           );
}

static bool pacbox_menu(void)
{
    int selected=0;
    int result;
    int menu_quit=0;
    int new_setting;
    bool need_restart = false;

    static const struct opt_items noyes[2] = {
        { "No", -1 },
        { "Yes", -1 },
    };

    static const struct opt_items difficulty_options[2] = {
        { "Normal", -1 },
        { "Harder", -1 },
    };

    static const struct opt_items numlives_options[4] = {
        { "1", -1 },
        { "2", -1 },
        { "3", -1 },
        { "5", -1 },
    };

    static const struct opt_items bonus_options[4] = {
        { "10000 points", -1 },
        { "15000 points", -1 },
        { "20000 points", -1 },
        { "No bonus", -1 },
    };

    static const struct opt_items ghostname_options[2] = {
        { "Normal", -1 },
        { "Alternate", -1 },
    };

    enum
    {
        PBMI_DIFFICULTY = 0,
        PBMI_PACMEN_PER_GAME,
        PBMI_BONUS_LIFE,
        PBMI_GHOST_NAMES,
        PBMI_DISPLAY_FPS,
        PBMI_SOUND,
#ifdef AI
        PBMI_AI,
#endif
        PBMI_RESTART,
        PBMI_QUIT,
    };

    MENUITEM_STRINGLIST(menu, "Pacbox Menu", NULL,
                        "Difficulty", "Pacmen Per Game", "Bonus Life",
                        "Ghost Names", "Display FPS", "Sound",
#ifdef AI
                        "AI",
#endif
                        "Restart", "Quit");

    rb->button_clear_queue();
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

    while (!menu_quit) {
        result=rb->do_menu(&menu, &selected, NULL, false);

        switch(result)
        {
            case PBMI_DIFFICULTY:
                new_setting=settings.difficulty;
                rb->set_option("Difficulty", &new_setting, INT,
                               difficulty_options , 2, NULL);
                if (new_setting != settings.difficulty) {
                    settings.difficulty=new_setting;
                    need_restart=true;
                }
                break;
            case PBMI_PACMEN_PER_GAME:
                new_setting=settings.numlives;
                rb->set_option("Pacmen Per Game", &new_setting, INT,
                               numlives_options , 4, NULL);
                if (new_setting != settings.numlives) {
                    settings.numlives=new_setting;
                    need_restart=true;
                }
                break;
            case PBMI_BONUS_LIFE:
                new_setting=settings.bonus;
                rb->set_option("Bonus Life", &new_setting, INT,
                               bonus_options , 4, NULL);
                if (new_setting != settings.bonus) {
                    settings.bonus=new_setting;
                    need_restart=true;
                }
                break;
            case PBMI_GHOST_NAMES:
                new_setting=settings.ghostnames;
                rb->set_option("Ghost Names", &new_setting, INT,
                               ghostname_options , 2, NULL);
                if (new_setting != settings.ghostnames) {
                    settings.ghostnames=new_setting;
                    need_restart=true;
                }
                break;
            case PBMI_DISPLAY_FPS:
                rb->set_option("Display FPS",&settings.showfps,INT,
                               noyes, 2, NULL);
                break;
            case PBMI_SOUND:
                rb->set_option("Sound",&settings.sound, INT,
                               noyes, 2, NULL);
                break;
#ifdef AI
            case PBMI_AI:
                rb->set_option("AI",&settings.ai, INT,
                               noyes, 2, NULL);
                break;
#endif
            case PBMI_RESTART:
                need_restart=true;
                menu_quit=1;
                break;
            default:
                menu_quit=1;
                break;
        }
    }
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_PAL256);
#endif

    if (need_restart) {
        init_PacmanMachine(settings_to_dip(settings));
    }

    /* Possible results: 
         exit game
         restart game
         usb connected
    */
    return (result==PBMI_QUIT);
}

/* Sound is emulated in ISR context, so not much is done per sound frame */
#define NBSAMPLES    128
static uint32_t sound_buf[NBSAMPLES];
#if CONFIG_CPU == MCF5249
/* Not enough to put this in IRAM */
static int16_t raw_buf[NBSAMPLES];
#else
static int16_t raw_buf[NBSAMPLES] IBSS_ATTR;
#endif

/*
    Audio callback
 */
static void get_more(const void **start, size_t *size)
{
    int32_t *out, *outend;
    int16_t *raw;

    /* Emulate the audio for the current register settings */
    playSound(raw_buf, NBSAMPLES);

    out = sound_buf;
    outend = out + NBSAMPLES;
    raw = raw_buf;

    /* Convert to stereo */
    do
    {
        uint32_t sample = (uint16_t)*raw++;
        *out++ = sample | (sample << 16);
    }
    while (out < outend);

    *start = sound_buf;
    *size = NBSAMPLES*sizeof(sound_buf[0]); 
}

/*
    Start the sound emulation
*/
static void start_sound(void)
{
    int sr_index;

    if (sound_playing)
        return;

#ifndef PLUGIN_USE_IRAM    
    /* Ensure control of PCM - stopping music itn't obligatory */
    rb->plugin_get_audio_buffer(NULL);
#endif

    /* Get the closest rate >= to what is preferred */
    sr_index = rb->round_value_to_list32(PREFERRED_SAMPLING_RATE,
                        rb->hw_freq_sampr, HW_NUM_FREQ, false);

    if (rb->hw_freq_sampr[sr_index] < PREFERRED_SAMPLING_RATE
        && sr_index > 0)
    {
        /* Round up */
        sr_index--;
    }

    wsg3_set_sampling_rate(rb->hw_freq_sampr[sr_index]);

    rb->pcm_set_frequency(rb->hw_freq_sampr[sr_index]);
    rb->pcm_play_data(get_more, NULL, NULL, 0);

    sound_playing = true;
}

/*
    Stop the sound emulation
*/
static void stop_sound(void)
{
    if (!sound_playing)
        return;

    rb->pcm_play_stop();
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);

    sound_playing = false;
}

/* use buttons for joystick */
void joystick(void)
{
    int status;
    /* Check the button status */
    status = rb->button_status();
    rb->button_clear_queue();
    /*handle buttons if AI is off */
#ifdef PACMAN_HAS_REMOTE
    setDeviceMode( Joy1_Left, (status & PACMAN_LEFT || status == PACMAN_RC_LEFT) ? DeviceOn : DeviceOff);
    setDeviceMode( Joy1_Right, (status & PACMAN_RIGHT || status == PACMAN_RC_RIGHT) ? DeviceOn : DeviceOff);
    setDeviceMode( Joy1_Up, (status & PACMAN_UP || status == PACMAN_RC_UP) ? DeviceOn : DeviceOff);
    setDeviceMode( Joy1_Down, (status & PACMAN_DOWN || status == PACMAN_RC_DOWN) ? DeviceOn : DeviceOff);
    setDeviceMode( CoinSlot_1, (status & PACMAN_COIN || status == PACMAN_RC_COIN) ? DeviceOn : DeviceOff);
    setDeviceMode( Key_OnePlayer, (status & PACMAN_1UP || status == PACMAN_RC_1UP) ? DeviceOn : DeviceOff);
    setDeviceMode( Key_TwoPlayers, (status & PACMAN_2UP || status == PACMAN_RC_2UP) ? DeviceOn : DeviceOff);
#else
    setDeviceMode( Joy1_Left, (status & PACMAN_LEFT) ? DeviceOn : DeviceOff);
    setDeviceMode( Joy1_Right, (status & PACMAN_RIGHT) ? DeviceOn : DeviceOff);
    setDeviceMode( Joy1_Up, (status & PACMAN_UP) ? DeviceOn : DeviceOff);
    setDeviceMode( Joy1_Down, (status & PACMAN_DOWN) ? DeviceOn : DeviceOff);
    setDeviceMode( CoinSlot_1, (status & PACMAN_COIN) ? DeviceOn : DeviceOff);
    setDeviceMode( Key_OnePlayer, (status & PACMAN_1UP) ? DeviceOn : DeviceOff);
#ifdef PACMAN_2UP
    setDeviceMode( Key_TwoPlayers, (status & PACMAN_2UP) ? DeviceOn : DeviceOff);
#endif
#endif    
#ifdef CHEATS
// skip level for testing purposes
    if(status == SKIP_LEVEL)
    {
//dots
        ram_[0x4E0E] = 242;
//level
        ram_[0x4E13] = 254;
    }
#endif
}
#ifdef AI
/* blank controls */
void clear_joystick(void)
{
    setDeviceMode( Joy1_Left, DeviceOff);
    setDeviceMode( Joy1_Right, DeviceOff);
    setDeviceMode( Joy1_Up, DeviceOff);
    setDeviceMode( Joy1_Down, DeviceOff);

}

/* Make turns */
void ai_turn( unsigned char level, unsigned char turn)
{
     switch(ai_direction[level][turn])
            {
                case 0:
                    clear_joystick();
                    setDeviceMode( Joy1_Up, DeviceOn);
                    break;
                case 1:
                    clear_joystick();
                    setDeviceMode( Joy1_Down, DeviceOn);
                    break;
                case 2:
                    clear_joystick();
                    setDeviceMode( Joy1_Right, DeviceOn);
                    break;
                case 3:
                    clear_joystick();
                    setDeviceMode( Joy1_Left, DeviceOn);
                    break;
                case 4:
                    clear_joystick();
                    break;
            }
}
/*
    Decide turns automatically
*/
unsigned char ai( unsigned char turn )
{
    unsigned char position;                       /* pac-mans current position */
    unsigned char level;   /* current game level */
    unsigned char map[20] = {0,1,2,3,4,5,4,4,6,7,4,8,8,9,10,10,11,10,12,12};

    /*Select level map*/
    if(ram_[0x4E13] < 20)
        level=map[ram_[0x4E13]];
    else if(ram_[0x4E13] != 255)
        level=13;
    else
        level=14;

    /* AI can't start in middle of a level */
    if( turn > 210)
    {
        rb->splash(HZ/2, "AI will engage at next level start");
        return 0;
    }
    
    /* reset joystick direction on level start */
    if(!(ram_[0x4E0E] || turn))
    {
        /*levels that start facing right */
        if((level != 4) && (level != 11) && (level != 7) && (level != 5) && (level != 9) && (level !=12))
            clear_joystick();
        else
        {
            clear_joystick();
            setDeviceMode( Joy1_Right, DeviceOn);
        }
        return 1;
    }
    if( turn == 0)
    {
        if( (ram_[0x4E0E] == 1) && (ram_[0x4D3A] == 47))
        {
            turn=1;
        }
        joystick();
        return turn;
    }

    if((turn != 0) && (turn !=209))
    {
        /* set which axis to look for pac-man along */
        position = ram_[0x4D3A];
        if( ai_direction[level][turn-1] < 2)
        {
            position = ram_[0x4D39];
        }


        /*move joystick if necessary */
            if(ai_location[level][turn] < 30)
            {
                if((ai_location[level][turn] < 10) && (ai_location[level][turn] > 0)) /* handle turns using ghosts eaten as basis for turn timing */
                {
                    if( ram_[0x4DD0] == ai_location[level][turn])
                    {
                        ai_turn(level,turn);
                        turn++;
                    }
                }

                if( ram_[0x4D31] == (ai_location[level][turn] + 30)) /* handle turns using pinky's location as basis for turn timing */
                {
                    ai_turn(level,turn);
                    turn++;
                }  
            }else if( position == ai_location[level][turn] ) /* handle turns using pacman's location as basis for turn timing */
            {
                ai_turn(level,turn);
                turn++;
            } 
    }

    /* reset turn counter and joystick direction on level start */
    if(ram_[0x4E0E] == 0 )
    {
        /*levels that start facing right */
        if((level != 4) && (level != 11) && (level != 7) && (level != 5) && (level != 9) && (level !=12))
            clear_joystick();
        else
        {
            clear_joystick();
            setDeviceMode( Joy1_Right, DeviceOn);
        }
        return 1;
    }
    return turn;
}
#endif

/*
    Runs the game engine for one frame.
*/
static int gameProc( void )
{
    int fps;
    int status;
    long end_time;
    int frame_counter = 0;
    int yield_counter = 0;
#ifdef AI
    unsigned char turn = 250;
#endif

    if (settings.sound)
        start_sound();

    while (1)
    {
        /* Run the machine for one frame (1/60th second) */
        run();

/*Make Pac-man invincible*/
#ifdef CHEATS
        if(ram_[0x4E6E]== 23)
            ram_[0x4DA5]=00;
#endif



        frame_counter++;

        /* Check the button status */
        status = rb->button_status();
        rb->button_clear_queue();

#ifdef HAS_BUTTON_HOLD
        if (rb->button_hold())
        status = PACMAN_MENU;
#endif

        if ((status & PACMAN_MENU) == PACMAN_MENU
#ifdef PACMAN_RC_MENU
            || status == PACMAN_RC_MENU
#endif
        ) {
            bool menu_res;

            end_time = *rb->current_tick;

            stop_sound();

            menu_res = pacbox_menu();

            rb->lcd_clear_display();
#ifdef HAVE_REMOTE_LCD
            rb->lcd_remote_clear_display();
            rb->lcd_remote_update();
#endif
            if (menu_res)
                return 1;

            if (settings.sound)
                start_sound();

            start_time += *rb->current_tick-end_time;
        }
#ifdef AI
        if(!settings.ai)
        {
            joystick();
            turn = 250;
        }
        /* run ai */
        if (settings.ai && !ram_[0x4E02])
            turn = ai(turn);
        else
            joystick();
#else
        joystick();
#endif
        /* We only update the screen every third frame - Pacman's native 
           framerate is 60fps, so we are attempting to display 20fps */
        if (frame_counter == 60 / FPS) {

            frame_counter = 0;
            video_frames++;

            yield_counter ++;

            if (yield_counter == FPS) {
                yield_counter = 0;
                rb->yield ();
            }

            /* The following functions render the Pacman screen from the 
               contents of the video and color ram.  We first update the 
               background, and then draw the Sprites on top. 
            */

            renderBackground( video_buffer );
            renderSprites( video_buffer );

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
            rb->lcd_blit_pal256(    video_buffer, 0, 0, XOFS, YOFS, 
                                    ScreenWidth, ScreenHeight);
#else
            blit_display(lcd_fb ,video_buffer);
#endif

            if (settings.showfps) {
                fps = (video_frames*HZ*100) / (*rb->current_tick-start_time);
                rb->lcd_putsxyf(0,0,"%d.%02d / %d fps  ",fps/100,fps%100,FPS);
            }

#if !defined(HAVE_LCD_MODES) || \
    defined(HAVE_LCD_MODES) && !(HAVE_LCD_MODES & LCD_MODE_PAL256)
            rb->lcd_update();
#endif

            /* Keep the framerate at Pacman's 60fps */
            end_time = start_time + (video_frames*HZ)/FPS;
            while (TIME_BEFORE(*rb->current_tick,end_time)) {
                rb->sleep(1);
            }
        }
    }

    stop_sound();

    return 0;
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_update();

    struct viewport *vp_main = rb->lcd_set_viewport(NULL);
    lcd_fb = vp_main->buffer->fb_ptr;

    /* Set the default settings */
    settings.difficulty = 0; /* Normal */
    settings.numlives = 2;   /* 3 lives */
    settings.bonus = 0;      /* 10000 points */
    settings.ghostnames = 0; /* Normal names */
    settings.showfps = 0;    /* Do not show FPS */
    settings.sound = 0;      /* Sound off by default */
    settings.ai = 0;         /* AI off by default */

    if (configfile_load(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_MIN_VERSION
                       ) < 0)
    {
        /* If the loading failed, save a new config file (as the disk is
           already spinning) */
        configfile_save(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_VERSION);
    }

    /* Keep a copy of the saved version of the settings - so we can check if 
       the settings have changed when we quit */
    old_settings = settings;

#ifdef HAVE_BACKLIGHT
    /*Turn off backlight for ai*/
    if(settings.ai)
        backlight_ignore_timeout();
#endif

    /* Initialise the hardware */
    init_PacmanMachine(settings_to_dip(settings));
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_PAL256);
#endif

    /* Load the romset */
    if (loadROMS()) {
        start_time = *rb->current_tick-1;

        gameProc();

        /* Save the user settings if they have changed */
        if (rb->memcmp(&settings,&old_settings,sizeof(settings))!=0) {
            rb->splash(0, "Saving settings...");
            configfile_save(SETTINGS_FILENAME, config,
                            sizeof(config)/sizeof(*config),
                            SETTINGS_VERSION);
        }
    } else {
        rb->splashf(HZ*2, "No ROMs in %s/pacman/", ROCKBOX_DIR);
    }
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
#ifdef HAVE_BACKLIGHT
    backlight_use_settings();
#endif
    return PLUGIN_OK;
}
