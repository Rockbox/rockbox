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
#include "status.h"


#define BUFFER_SIZE 1024

#define OUTSIDE_BUFFER -10
#define OUTSIDE_FILE   -11

static int fd;
static int file_size;

static char buffer[BUFFER_SIZE+1];
static int buffer_pos; /* Position of the buffer in the file */

static char display_lines; /* number of lines on the display */
static char display_columns; /* number of columns on the display */
static int begin_line; /* Index of the first line displayed on the lcd */
static int end_line;   /* Index of the last line displayed on the lcd */
static int begin_line_pos; /* Position of the first_line in the bufffer */
static int end_line_pos; /* Position of the last_line in the buffer */

/*
 * Known issue: The caching algorithm will fail (display incoherent data) if
 * the total space of the lines that are displayed on the screen exceeds the
 * buffer size (only happens with very long lines).
 */

static void display_line_count(void)
{
#ifdef HAVE_LCD_BITMAP
    int w,h;
    lcd_getstringsize("M", &w, &h);
    display_lines = LCD_HEIGHT / h;
    display_columns = LCD_WIDTH / w;
#else
    display_lines = 2;
    display_columns = 11;
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

    for (i=0; i <= end_line - begin_line; i++) {
        if (line_pos == OUTSIDE_BUFFER ||
            line_pos == OUTSIDE_FILE)
            break;
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
    display_line_count();
    for (i=0; i<display_lines; ++i) {
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
    switch ( ret ) {
        case OUTSIDE_BUFFER:
            begin_line_pos = find_next_line(begin_line_pos);
            fill_buffer(begin_line_pos+buffer_pos);
            end_line_pos = find_next_line(end_line_pos);
            break;

        case OUTSIDE_FILE:
            return;

        default:
            begin_line_pos = find_next_line(begin_line_pos);
            end_line_pos = ret;
            break;
    }
    begin_line++;
    end_line++;
}

static void viewer_scroll_up(void)
{
    int ret;

    ret = find_prev_line(begin_line_pos);
    switch ( ret ) {
        case OUTSIDE_BUFFER:
            end_line_pos = find_prev_line(end_line_pos);
            fill_buffer(buffer_pos+end_line_pos-BUFFER_SIZE);
            begin_line_pos = find_prev_line(begin_line_pos);
            break;

        case OUTSIDE_FILE:
            return;

        default:
            end_line_pos = find_prev_line(end_line_pos);
            begin_line_pos = ret;
            break;
    }
    begin_line--;
    end_line--;
}

static int pagescroll(int col)
{
    bool exit = false;
    int i;

    while (!exit) {
        switch (button_get(true)) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_ON | BUTTON_UP:
            case BUTTON_ON | BUTTON_UP | BUTTON_REPEAT:
#else
            case BUTTON_ON | BUTTON_LEFT:
            case BUTTON_ON | BUTTON_LEFT | BUTTON_REPEAT:
#endif
                for (i=0; i<display_lines; i++)
                    viewer_scroll_up();
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_ON | BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REPEAT:
#else
            case BUTTON_ON | BUTTON_RIGHT:
            case BUTTON_ON | BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                for (i=0; i<display_lines; i++)
                    viewer_scroll_down();
                break;
                            
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_ON | BUTTON_LEFT:
            case BUTTON_ON | BUTTON_LEFT | BUTTON_REPEAT:
#else
            case BUTTON_ON | BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_ON | BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
#endif
                col -= display_columns;
                if (col < 0)
                    col = 0;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_ON | BUTTON_RIGHT:
            case BUTTON_ON | BUTTON_RIGHT | BUTTON_REPEAT:
#else
            case BUTTON_ON | BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_ON | BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                col += display_columns;
                break;

            case BUTTON_ON | BUTTON_REL: 
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REL:
            case BUTTON_ON | BUTTON_UP | BUTTON_REL:
#else
            case BUTTON_ON | BUTTON_RIGHT | BUTTON_REL:
            case BUTTON_ON | BUTTON_LEFT | BUTTON_REL:
            case BUTTON_ON | BUTTON_MENU | BUTTON_RIGHT | BUTTON_REL:
            case BUTTON_ON | BUTTON_MENU | BUTTON_LEFT | BUTTON_REL:
#endif
                exit = true;
                break;
        }
        if ( !exit )
            viewer_draw(col);
    }

    return col;
}
bool viewer_run(char* file)
{
    bool exit=false;
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
    while (!exit) {
        button = button_get(true);

        switch ( button ) {

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1:
            case BUTTON_OFF:
#else
            case BUTTON_STOP:
#endif
                viewer_exit();
                exit = true;
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
                if (col < 0)
                    col = 0;
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

            case BUTTON_ON:
#ifdef HAVE_PLAYER_KEYPAD
            case BUTTON_ON | BUTTON_MENU:
#endif
                col = pagescroll(col);
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
#ifdef HAVE_LCD_CHARCELLS
                status_set_param(false);
#endif
                viewer_exit();
                return true;
        }
    }
    return false;
}
