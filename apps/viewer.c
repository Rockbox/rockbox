/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "file.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "font.h"
#include "settings.h"
#include "icons.h"
#include "screens.h"

#define BUFFER_SIZE 1024

#define OUTSIDE_BUFFER -10
#define OUTSIDE_FILE   -11

static int fd;
static int file_size;

static char buffer[BUFFER_SIZE+1];
static int buffer_pos; /* Position of the buffer in the file */

static int begin_line; /* Index of the first line displayed on the lcd */
static int end_line;   /* Index of the last line displayed on the lcd */
static int begin_line_pos; /* Position of the first_line in the bufffer */
static int end_line_pos; /* Position of the last_line in the buffer */

/*
 * Known issue: The caching algorithm will fail (display incoherent data) if
 * the total space of the lines that are displayed on the screen exceeds the
 * buffer size (only happens with very long lines).
 */

static int display_line_count(void)
{
#ifdef HAVE_LCD_BITMAP
    int fh;
    fh = font_get(FONT_UI)->height;
    if (global_settings.statusbar)
        return (LCD_HEIGHT - STATUSBAR_HEIGHT) / fh;
    else
        return LCD_HEIGHT/fh;
#else
    return 2;
#endif
}

static int find_next_line(int pos)
{
    int i;

    if (pos==OUTSIDE_BUFFER || pos==OUTSIDE_FILE)
        return pos;

    i = pos;
    if (buffer_pos+i>=file_size) {
        return OUTSIDE_FILE;
    }
    while (1) {
        i++;
        if (buffer_pos+i==file_size) {
            return i;
        }
        if (i>=BUFFER_SIZE) {
            return OUTSIDE_BUFFER;
        }
        if (buffer[i]==0) {
            return i;
        }
    }
}

static int find_prev_line(int pos)
{
    int i;

    if (pos==OUTSIDE_BUFFER || pos==OUTSIDE_FILE)
        return pos;

    i = pos;
    if (buffer_pos+i<0) {
        return OUTSIDE_FILE;
    }
    while (1) {
        i--;
        if (buffer_pos+i<0) {
            return i;
        }
        if (i<0) {
            return OUTSIDE_BUFFER;
        }
        if (buffer[i]==0) {
            return i;
        }
    }
}

static void viewer_draw(int col)
{
    int i, j;
    char* str;
    int line_pos;

    lcd_clear_display();

    line_pos = begin_line_pos;

    for (i=0;
         i<=end_line-begin_line &&
         line_pos!=OUTSIDE_BUFFER &&
         line_pos!=OUTSIDE_FILE;
         i++) {
        str = buffer + line_pos + 1;
        for (j=0; j<col && *str!=0; ++j)
            str++;
        lcd_puts(0, i, str);
        line_pos = find_next_line(line_pos);
    }

    lcd_update();
}

static void fill_buffer(int pos)
{
    int i;
    int numread;

    if (pos>=file_size-BUFFER_SIZE)
        pos = file_size-BUFFER_SIZE;
    if (pos<0)
        pos = 0;

    lseek(fd, pos, SEEK_SET);
    numread = read(fd, buffer, BUFFER_SIZE);

    begin_line_pos -= pos - buffer_pos;
    end_line_pos -= pos - buffer_pos;
    buffer_pos = pos;

    buffer[numread] = 0;
    for(i=0;i<numread;i++) {
        switch(buffer[i]) {
            case '\r':
                buffer[i] = ' ';
                break;
            case '\n':
                buffer[i] = 0;
                break;
            default:
                break;
        }
    }
}

static bool viewer_init(char* file)
{
    int i;
    int ret;
    int disp_lines;

    fd = open(file, O_RDONLY);
    if (fd==-1)
        return false;

    file_size = lseek(fd, 0, SEEK_END);

    buffer_pos = 0;
    begin_line = 0;
    begin_line_pos = -1;
    end_line = -1;
    end_line_pos = -1;
    fill_buffer(0);
    disp_lines = display_line_count();
    for (i=0; i<disp_lines; ++i) {
        ret = find_next_line(end_line_pos);
        if (ret!=OUTSIDE_FILE && ret!=OUTSIDE_BUFFER) {
            end_line_pos = ret;
            end_line++;
        }
    }

    return true;
}

static void viewer_exit(void)
{
    close(fd);
}

static void viewer_scroll_down(void)
{
    int ret;

    ret = find_next_line(end_line_pos);
    if (ret==OUTSIDE_BUFFER) {
        begin_line_pos = find_next_line(begin_line_pos);
        fill_buffer(begin_line_pos+buffer_pos);
        end_line_pos = find_next_line(end_line_pos);
    } else if (ret==OUTSIDE_FILE) {
        return;
    } else {
        begin_line_pos = find_next_line(begin_line_pos);
        end_line_pos = ret;
    }
    begin_line++;
    end_line++;
}

static void viewer_scroll_up(void)
{
    int ret;

    ret = find_prev_line(begin_line_pos);
    if (ret==OUTSIDE_BUFFER) {
        end_line_pos = find_prev_line(end_line_pos);
        fill_buffer(buffer_pos+end_line_pos-BUFFER_SIZE);
        begin_line_pos = find_prev_line(begin_line_pos);
    } else if (ret==OUTSIDE_FILE) {
        return;
    } else {
        end_line_pos = find_prev_line(end_line_pos);
        begin_line_pos = ret;
    }
    begin_line--;
    end_line--;
}

bool viewer_run(char* file)
{
    int button;
    int col = 0;
    int ok;

#ifdef HAVE_LCD_BITMAP
    /* no margins */
    lcd_setmargins(0, 0);
#endif

    ok = viewer_init(file);
    if (!ok) {
        lcd_clear_display();
        lcd_puts(0, 0, "Error");
        lcd_update();
        sleep(HZ);
        viewer_exit();
        return false;
    }

    viewer_draw(col);
    while(1) {
        button = button_get(true);

        switch ( button ) {

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1:
            case BUTTON_ON:
#else
            case BUTTON_STOP:
            case BUTTON_ON:
#endif
                viewer_exit();
                return true;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#else
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
#endif
                viewer_scroll_up();
                viewer_draw(col);
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#else
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                viewer_scroll_down();
                viewer_draw(col);
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
#else
            case BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
#endif
                col--;
                viewer_draw(col);
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
#else
            case BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                col++;
                viewer_draw(col);
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_PARAM, false);
#endif
                viewer_exit();
                return true;
        }
    }
}

