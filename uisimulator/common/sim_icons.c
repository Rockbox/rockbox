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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifdef HAVE_LCD_CHARCELLS

#include "sim_icons.h"

#include <lcd.h>
#include <kernel.h>
#include <sprintf.h>
#include <string.h>
#include <debug.h>

#define XPOS_volume 54
#define XPOS_volume1 XPOS_volume + 15
#define XPOS_volume2 XPOS_volume1 + 2
#define XPOS_volume3 XPOS_volume2 + 2
#define XPOS_volume4 XPOS_volume3 + 2
#define XPOS_volume5 XPOS_volume4 + 2

#define BMPHEIGHT_battery 7
#define BMPWIDTH_battery 14
const unsigned char battery[] = {
0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f, 0x14, 0x1c, 

};
#define BMPHEIGHT_battery_bit 3
#define BMPWIDTH_battery_bit 2
const unsigned char battery_bit[] = {
0x07, 0x07, 

};
#define BMPHEIGHT_pause 7
#define BMPWIDTH_pause 7
const unsigned char pause[] = {
0x7f, 0x7f, 0x7f, 0x00, 0x7f, 0x7f, 0x7f, 

};
#define BMPHEIGHT_play 7
#define BMPWIDTH_play 8
const unsigned char play[] = {
0x7f, 0x7f, 0x3e, 0x3e, 0x1c, 0x1c, 0x08, 0x08, 

};
#define BMPHEIGHT_record 8
#define BMPWIDTH_record 4
const unsigned char record[] = {
0x08, 0x1c, 0x1c, 0x08, 

};
#define BMPHEIGHT_volume 7
#define BMPWIDTH_volume 13
const unsigned char volume[] = {
0x0e, 0x30, 0x60, 0x30, 0x0e, 0x30, 0x48, 0x48, 0x48, 0x30, 0x00, 0x7e, 0x00, 

};
#define BMPHEIGHT_volume1 7
#define BMPWIDTH_volume1 2
const unsigned char volume1[] = {
0x00, 0x70, 

};
#define BMPHEIGHT_volume2 7
#define BMPWIDTH_volume2 2
const unsigned char volume2[] = {
0x00, 0x78, 

};
#define BMPHEIGHT_volume3 7
#define BMPWIDTH_volume3 2
const unsigned char volume3[] = {
0x00, 0x7c, 

};
#define BMPHEIGHT_volume4 7
#define BMPWIDTH_volume4 2
const unsigned char volume4[] = {
0x00, 0x7e, 

};
#define BMPHEIGHT_volume5 7
#define BMPWIDTH_volume5 2
const unsigned char volume5[] = {
0x00, 0x7f, 

};
#define BMPHEIGHT_usb 7
#define BMPWIDTH_usb 22
const unsigned char usb[] = {
0x08, 0x1c, 0x1c, 0x08, 0x0c, 0x0e, 0x1b, 0x39, 0x69, 0x49, 0x49, 0x49, 0x4b, 0x4b, 0x48, 0x48, 0x68, 0x68, 0x08, 0x1c, 0x1c, 0x08, 

};
#define BMPHEIGHT_audio 7
#define BMPWIDTH_audio 27
const unsigned char audio[] = {
0x1c, 0x22, 0x41, 0x79, 0x55, 0x55, 0x79, 0x41, 0x5d, 0x61, 0x61, 0x5d, 0x41, 0x7d, 0x65, 0x65, 0x59, 0x41, 0x7d, 0x41, 0x59, 0x65, 0x65, 0x59, 0x41, 0x22, 0x1c, 

};
#define BMPHEIGHT_param 7
#define BMPWIDTH_param 31
const unsigned char param[] = {
0x1c, 0x22, 0x41, 0x7d, 0x55, 0x55, 0x49, 0x41, 0x79, 0x55, 0x55, 0x79, 0x41, 0x7d, 0x55, 0x75, 0x69, 0x41, 0x79, 0x55, 0x55, 0x79, 0x41, 0x7d, 0x49, 0x51, 0x49, 0x7d, 0x41, 0x22, 0x1c, 

};

#define BMPHEIGHT_repeat 7
#define BMPWIDTH_repeat 12
const unsigned char repeat[] = {
0x1c, 0x22, 0x41, 0x41, 0x41, 0x41, 0x71, 0x71, 0x61, 0x61, 0x41, 0x40, 

};

#define BMPHEIGHT_repeat1 7
#define BMPWIDTH_repeat1 3
const unsigned char repeat1[] = {
0x42, 0x7f, 0x40, 

};

struct icon_info
{
    const unsigned char* bitmap;
    int xpos;
    int ypos;
    int width;
    int height;
};

static struct icon_info icons [] = 
{
    { battery, 0, 0, BMPWIDTH_battery, BMPHEIGHT_battery },                /* ICON_BATTERY */
    { battery_bit, 2, 2, BMPWIDTH_battery_bit, BMPHEIGHT_battery_bit },    /* ICON_BATTERY_1 */
    { battery_bit, 5, 2, BMPWIDTH_battery_bit, BMPHEIGHT_battery_bit },    /* ICON_BATTERY_2 */
    { battery_bit, 8, 2, BMPWIDTH_battery_bit, BMPHEIGHT_battery_bit },    /* ICON_BATTERY_3 */
    { usb, 0, 40, BMPWIDTH_usb, BMPHEIGHT_usb },                           /* ICON_USB */
    { play, 20, 0, BMPWIDTH_play, BMPHEIGHT_play },                        /* ICON_PLAY */
    { record, 35, 0, BMPWIDTH_record, BMPHEIGHT_record },                  /* ICON_RECORD */
    { pause, 50, 0, BMPWIDTH_pause, BMPHEIGHT_pause },                     /* ICON_PAUSE */
    { audio, 40, 40, BMPWIDTH_audio, BMPHEIGHT_audio },                    /* ICON_AUDIO */
    { repeat, XPOS_volume-4-BMPWIDTH_repeat,
      0, BMPWIDTH_repeat, BMPHEIGHT_repeat },                              /* ICON_REPEAT */
    { repeat1, XPOS_volume-10, 0, BMPWIDTH_repeat1, BMPHEIGHT_repeat1 },   /* ICON_1 */
    { volume, XPOS_volume, 0, BMPWIDTH_volume, BMPHEIGHT_volume },         /* ICON_VOLUME */
    { volume1, XPOS_volume1, 0, BMPWIDTH_volume1, BMPHEIGHT_volume1 },     /* ICON_VOLUME1 */
    { volume2, XPOS_volume2, 0, BMPWIDTH_volume2, BMPHEIGHT_volume2 },     /* ICON_VOLUME2 */
    { volume3, XPOS_volume3, 0, BMPWIDTH_volume3, BMPHEIGHT_volume3 },     /* ICON_VOLUME3 */
    { volume4, XPOS_volume4, 0, BMPWIDTH_volume4, BMPHEIGHT_volume4 },     /* ICON_VOLUME4 */
    { volume5, XPOS_volume5, 0, BMPWIDTH_volume5, BMPHEIGHT_volume5 },     /* ICON_VOLUME5 */
    { param, 90, 40, BMPWIDTH_param, BMPHEIGHT_param },                    /* ICON_PARAM */
};

void display_icon(int icon, bool enable)
{
    if (enable)
        lcd_bitmap((unsigned char*)icons[icon].bitmap, icons[icon].xpos, icons[icon].ypos, icons[icon].width, icons[icon].height, true);
    else
        lcd_clearrect(icons[icon].xpos, icons[icon].ypos, icons[icon].width, icons[icon].height);
}

void sim_battery_icon(int icon, bool enable)
{
    static bool battery_icons[4] = { true, true, true, true };
    int i;
    
    battery_icons[icon] = enable;
    
    for (i = 0; i <= ICON_BATTERY_3; ++i)
    {
        display_icon(icon, battery_icons[i]);
    }
};

void
lcd_icon(int icon, bool enable)
{
    switch (icon)
    {
    case ICON_BATTERY:
    case ICON_BATTERY_1:
    case ICON_BATTERY_2:
    case ICON_BATTERY_3:
        sim_battery_icon(icon, enable);
        break;
    default:
        display_icon(icon, enable);
        break;
    }
    lcd_update();
}

#endif /* HAVE_LCD_CHARCELLS */
