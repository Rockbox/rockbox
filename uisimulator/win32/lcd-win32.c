/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <windows.h>
#include <process.h>
#include "uisw32.h"
#include "lcd.h"

//
// simulator specific code
//

// varaibles
unsigned char       display[LCD_WIDTH][LCD_HEIGHT/8]; // the display
char                bitmap[LCD_HEIGHT][LCD_WIDTH]; // the ui display

BITMAPINFO2 bmi =
{
	sizeof (BITMAPINFOHEADER),
	LCD_WIDTH, -LCD_HEIGHT, 1, 8,
	BI_RGB, 0, 0, 0, 2, 2,
	UI_LCD_BGCOLOR, 0, // green background color
	UI_LCD_BLACK, 0 // black color
}; // bitmap information


// lcd_update
// update lcd
void lcd_update()
{
	int x, y;
    if (hGUIWnd == NULL)
        _endthread ();

	for (x = 0; x < LCD_WIDTH; x++)
		for (y = 0; y < LCD_HEIGHT; y++)
            bitmap[y][x] = ((display[x][y/8] >> (y & 7)) & 1);

	InvalidateRect (hGUIWnd, NULL, FALSE);

    // natural sleep :)
    Sleep (50);
}

// lcd_backlight
// set backlight state of lcd
void lcd_backlight (
                    bool on // switch backlight on or off?
                    )
{
    if (on)
    {
        RGBQUAD blon = {UI_LCD_BGCOLORLIGHT, 0};
        bmi.bmiColors[0] = blon;
    }
    else
    {
        RGBQUAD blon = {UI_LCD_BGCOLOR, 0};
        bmi.bmiColors[0] = blon;
    }

	InvalidateRect (hGUIWnd, NULL, FALSE);
}