/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Mats Lidell <matsl@contactor.se>
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
#include "config.h"
#ifdef HAVE_LCD_CHARCELLS

#include <lcd.h>
#include <kernel.h>
#include <sprintf.h>
#include <string.h>
#include <debug.h>

extern void lcd_print_icon(int x, int icon_line, bool enable, char **icon);

static char* icon_battery_bit[]=
{
    "-----",
    "     ",
    "*****",
    "*****",
    "*****",
    "*****",
    "*****",
    "*****",
    "     ",
    "-----",
    NULL
};

static char* icon_battery[]=
{
    "*********************  ",
    "*                   *  ",
    "* ----- ----- ----- *  ",
    "* ----- ----- ----- ***",
    "* ----- ----- ----- * *",
    "* ----- ----- ----- * *",
    "* ----- ----- ----- ***",
    "* ----- ----- ----- *  ",
    "*                   *  ",
    "*********************  ",
    NULL
};

static char* icon_volume[]=
{
    "             ",
    "             ",
    "             ",
    "             ",
    "*   *      * ",
    "*   *      * ",
    " * *  ***  * ",
    " * * *   * * ",
    "  *  *   * * ",
    "  *   ***  * ",
    NULL
};

static char* icon_volume_1[]=
{
    "  ",
    "  ",
    "  ",
    "  ",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    NULL
};

static char* icon_volume_2[]=
{
    "  ",
    "  ",
    "  ",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    NULL
};

static char* icon_volume_3[]=
{
    "  ",
    "  ",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    NULL
};

static char* icon_volume_4[]=
{
    "  ",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    NULL
};

static char* icon_volume_5[]=
{
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    "**",
    NULL
};

static char* icon_pause[]=
{
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    " ****  **** ",
    NULL
};

static char* icon_play[]=
{
    "**          ",
    "*****       ",
    "*******     ",
    "*********   ",
    "*********** ",
    "*********   ",
    "*******     ",
    "*****       ",
    "**          ",
    "            ",
    NULL
};

static char* icon_record[]=
{
    "    ***     ",
    "   *****    ",
    "  *******   ",
    " *********  ",
    " *********  ",
    " *********  ",
    "  *******   ",
    "   *****    ",
    "    ***     ",
    "            ",
    NULL
};

static char* icon_usb[]=
{
    "        *********      ",
    "      **      **       ",
    "     *                 ",
    " ** *              **  ",
    "***********************",
    " **   *            **  ",
    "       *               ",
    "        **      **     ",
    "          ********     ",
    "                **     ",
    NULL
};

static char* icon_audio[]=
{
    "   ***************************   ",
    " **                           ** ",
    "*    **   *   * ****   *  ***   *",
    "*   *  *  *   * *   *  * *   *  *",
    "*  *    * *   * *    * * *   *  *",
    "*  ****** *   * *    * * *   *  *",
    "*  *    * *   * *   *  * *   *  *",
    "*  *    *  ***  ****   *  ***   *",
    " **                           ** ",
    "   ***************************   ",
    NULL
};

static char* icon_param[]=
{
    "   *********************************   ",
    " **                                 ** ",
    "*  ****    **   ****    **   **   **  *",
    "*  *   *  *  *  *   *  *  *  **   **  *",
    "*  *   * *    * *   * *    * * * * *  *",
    "*  ****  ****** ****  ****** * * * *  *",
    "*  *     *    * *   * *    * *  *  *  *",
    "*  *     *    * *   * *    * *  *  *  *",
    " **                                 ** ",
    "   *********************************   ",
    NULL
};

static char* icon_repeat[]=
{
    "                ",
    "   *************",
    "  *             ",
    " *              ",
    "*               ",
    "*               ",
    "*       **      ",
    " *      ****    ",
    "  *     ******  ",
    "   *************",
   NULL
};

static char* icon_repeat2[]=
{
    "   ",
    "  *",
    " **",
    "***",
    "  *",
    "  *",
    "  *",
    "  *",
    "  *",
    "  *",
    NULL
};


struct icon_info
{
    char** bitmap;
    int xpos;
    int row;
};

#define ICON_VOLUME_POS 102
#define ICON_VOLUME_SIZE 14
#define ICON_VOLUME_X_SIZE 2

static struct icon_info icons [] =
{
    
  {icon_battery, 0, 0},
  {icon_battery_bit, 2, 0},
  {icon_battery_bit, 8, 0},
  {icon_battery_bit, 14, 0},
  {icon_usb, 0, 1},
  {icon_play, 36, 0},
  {icon_record, 48, 0},
  {icon_pause, 60, 0},
  {icon_audio, 37, 1},
  {icon_repeat, 74, 0},
  {icon_repeat2, 94, 0},
  {icon_volume, ICON_VOLUME_POS, 0},
  {icon_volume_1, ICON_VOLUME_POS+ICON_VOLUME_SIZE, 0},
  {icon_volume_2, ICON_VOLUME_POS+ICON_VOLUME_SIZE+(1*ICON_VOLUME_X_SIZE)+1, 0},
  {icon_volume_3, ICON_VOLUME_POS+ICON_VOLUME_SIZE+(2*ICON_VOLUME_X_SIZE)+2, 0},
  {icon_volume_4, ICON_VOLUME_POS+ICON_VOLUME_SIZE+(3*ICON_VOLUME_X_SIZE)+3, 0},
  {icon_volume_5, ICON_VOLUME_POS+ICON_VOLUME_SIZE+(4*ICON_VOLUME_X_SIZE)+4, 0},
  {icon_param, 90, 1}
};

void
lcd_icon(int icon, bool enable)
{
  lcd_print_icon(icons[icon].xpos, icons[icon].row, enable,
		 icons[icon].bitmap);
}

#endif /* HAVE_LCD_CHARCELLS */


