/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Philipp Pertermann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

#ifndef SIMULATOR
#ifdef HAVE_LCD_BITMAP

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define SPLITEDIT_QUIT BUTTON_OFF
#define SPLITEDIT_PLAY BUTTON_PLAY
#define SPLITEDIT_SAVE BUTTON_F1
#define SPLITEDIT_LOOP_MODE BUTTON_F2
#define SPLITEDIT_SCALE BUTTON_F3
#define SPLITEDIT_SPEED50 (BUTTON_ON | BUTTON_LEFT)
#define SPLITEDIT_SPEED100 (BUTTON_ON | BUTTON_PLAY)
#define SPLITEDIT_SPEED150 (BUTTON_ON | BUTTON_RIGHT)
#define SPLITEDIT_MENU_RUN BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SPLITEDIT_QUIT BUTTON_OFF
#define SPLITEDIT_PLAY_PRE BUTTON_MENU
#define SPLITEDIT_PLAY (BUTTON_MENU | BUTTON_REL)
#define SPLITEDIT_SAVE (BUTTON_MENU | BUTTON_LEFT)
#define SPLITEDIT_LOOP_MODE (BUTTON_MENU | BUTTON_UP)
#define SPLITEDIT_SCALE (BUTTON_MENU | BUTTON_RIGHT)
#define SPLITEDIT_MENU_RUN BUTTON_RIGHT

#endif

#define BMPHEIGHT 7
#define BMPWIDTH 13
unsigned char LOOP_BMP[][13] =
{
  {0xfc,0x00,0x10,0x11,0x93,0x7f,0x13,0x11,0x7c,0x38,0x10,0x00,0x7c}, /*ALL */
  {0x81,0x03,0x7f,0x03,0x91,0x10,0x10,0x10,0x7c,0x38,0x10,0x00,0x7c}, /*FROM*/
  {0xfc,0x00,0x10,0x10,0x90,0x10,0x7c,0x38,0x11,0x03,0x7f,0x03,0x01}, /*TO  */
  {0x80,0x10,0x10,0x11,0x93,0x7f,0x13,0x11,0x10,0x7c,0x38,0x10,0x00}, /*FREE*/
};

unsigned char CUT_BMP[] =
{
  0xc1,0x63,0x63,0x36,0xb6,0x1c,0x1c,0x36,0x77,0x55,0x55,0x55,0x32,
};

unsigned char SCALE_BMP[][13] =
{
  {0x80,0x06,0x49,0x66,0xb0,0x18,0x0c,0x06,0x33,0x49,0x30,0x00,0x00}, /*lin*/
  {0x80,0x30,0x78,0x48,0xff,0x7f,0x00,0x7f,0x7f,0x48,0x78,0x30,0x00}, /*db*/
};

#define TIMEBAR_Y 9
#define TIMEBAR_HEIGHT 4

#define OSCI_X 0
#define OSCI_Y (TIMEBAR_Y + TIMEBAR_HEIGHT + 1)
#define OSCI_WIDTH LCD_WIDTH
#define OSCI_HEIGHT (LCD_HEIGHT - BMPHEIGHT - OSCI_Y - 1)

/* Indices of the menu items in the save editor, see save_editor */
#define SE_PART1_SAVE 0
#define SE_PART1_NAME 1
#define SE_PART2_SAVE 2
#define SE_PART2_NAME 3
#define SE_SAVE 4
#define SE_COUNT 5

/* the global api pointer */
static struct plugin_api* rb;

/* contains the file name of the song that is to be split */
static char path_mp3[MAX_PATH];

/* Exit code of this plugin */
static enum plugin_status splitedit_exit_code = PLUGIN_OK;

/* The range in time that the displayed aerea comprises */
static unsigned int range_start = 0;
static unsigned int range_end = 0;

/* The range in time that is being looped */
static unsigned int play_start = 0;
static unsigned int play_end = 0;

/* Point in time (pixel) at which the split mark is set */
static int split_x = OSCI_X + (OSCI_WIDTH / 2);

/* Contains the peak values */
static unsigned char osci_buffer[OSCI_WIDTH];

/* if true peak values from a previous loop are only overwritten
   if the new value is greater than the old value */
static bool osci_valid = false;

/**
 * point in time from which on the osci_buffer is invalid
 * if set to ~(unsigned int)0 the entire osci_buffer is invalid
 */
static unsigned int validation_start = ~(unsigned int)0;

/* all the visible aerea is looped */
#define LOOP_MODE_ALL  0

/* loop starts at split point, ends at right visible border */
#define LOOP_MODE_FROM 1

/* loop start at left visible border, ends at split point */
#define LOOP_MODE_TO   2

/* let the song play without looping */
#define LOOP_MODE_FREE 3

/* see LOOP_MODE_XXX constants vor valid values */
static int loop_mode = LOOP_MODE_FREE;

/* minimal allowed timespan (ms) of the visible (and looped) aerea*/
#define MIN_RANGE_SIZE 1000

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * time     - time to format, in milliseconds.
 */
static void format_time_ms(char* buf, int buf_size, int time)
{
    rb->snprintf(buf, buf_size, "%d:%02d:%03d", time / 60000,
        time % 60000 / 1000, (time % 60000) % 1000);
}

/**
 * converts screen coordinate (pixel) to time (ms)
 */
static int xpos_to_time(int xpos)
{
    int retval = 0;
    int range = range_end - range_start;
    retval = range_start + (((xpos - OSCI_X) * range) / OSCI_WIDTH);
    return retval;
}

/**
 * Converts time (ms) to screen coordinates (pixel).
 */
static int time_to_xpos(unsigned int time)
{
    int retval = OSCI_X;

    /* clip the range */
    if (time < range_start)
    {
        retval = OSCI_X;
    }
    else
    if (time >= range_end)
    {
        retval = OSCI_X + OSCI_WIDTH;
    }

    /* do the calculation */
    else
    {
        int range = range_end - range_start;
        retval = OSCI_X + ((time - range_start) * OSCI_WIDTH) / range ;
    }
    return retval;
}

/**
 * Updates the display of the textual data only.
 */
static void update_data(void)
{
    char buf[20];
    char timebuf[10];
    int w, h;

    /* split point */
    format_time_ms(timebuf, sizeof buf, xpos_to_time(split_x));
    rb->snprintf(buf, sizeof buf, "Split at: %s", timebuf);

    rb->lcd_getstringsize(buf, &w, &h);

    rb->lcd_clearrect(0, 0, LCD_WIDTH, h);
    rb->lcd_puts(0, 0, buf);
    rb->lcd_update_rect(0, 0, LCD_WIDTH, h);
}

/**
 * Displays which part of the song is visible
 * in the osci.
 */
static void update_timebar(struct mp3entry *mp3)
{
    rb->scrollbar
    (
        0, TIMEBAR_Y, LCD_WIDTH, TIMEBAR_HEIGHT,
        mp3->length, range_start, range_end,
        HORIZONTAL
    );
    rb->lcd_update_rect(0, TIMEBAR_Y, LCD_WIDTH, TIMEBAR_HEIGHT);
}

/**
 * Marks the entire area of the osci buffer invalid.
 * It will be drawn with new values in the next loop.
 */
void splitedit_invalidate_osci(void)
{
    osci_valid = false;
    validation_start = ~(unsigned int)0;
}

/**
 * Returns the loop mode. See the LOOP_MODE_XXX constants above.
 */
int splitedit_get_loop_mode(void)
{
    return loop_mode;
}

/**
 * Updates the icons that display the Fn key hints.
 */
static void update_icons(void)
{
    rb->lcd_clearrect(0, LCD_HEIGHT - BMPHEIGHT, LCD_WIDTH, BMPHEIGHT);

    /* The CUT icon */
    rb->lcd_bitmap(CUT_BMP,
        LCD_WIDTH / 3 / 2 - BMPWIDTH / 2, LCD_HEIGHT - BMPHEIGHT,
        BMPWIDTH, BMPHEIGHT, true);

    /* The loop mode icon */
    rb->lcd_bitmap(LOOP_BMP[splitedit_get_loop_mode()],
        LCD_WIDTH/3 + LCD_WIDTH/3 / 2 - BMPWIDTH/2, LCD_HEIGHT - BMPHEIGHT,
        BMPWIDTH, BMPHEIGHT, true);

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
    /* The scale icon */
    rb->lcd_bitmap(SCALE_BMP[rb->peak_meter_get_use_dbfs()],
        2 *LCD_WIDTH/3 + LCD_WIDTH/3 / 2 - BMPWIDTH/2, LCD_HEIGHT - BMPHEIGHT,
        BMPWIDTH, BMPHEIGHT, true);
#else
    {
        static int idx;
        if (idx < 0 || idx > 1) idx = 0;
        idx = 1 - idx;
        rb->lcd_bitmap(SCALE_BMP[idx],
            2 *LCD_WIDTH/3 + LCD_WIDTH/3 / 2 - BMPWIDTH/2, LCD_HEIGHT - BMPHEIGHT,
            BMPWIDTH, BMPHEIGHT, true);
    }
#endif

    rb->lcd_update_rect(0, LCD_HEIGHT - BMPHEIGHT, LCD_WIDTH, BMPHEIGHT);
}

/**
 * Sets the loop mode. See the LOOP_MODE_XXX constants above.
 */
void splitedit_set_loop_mode(int mode)
{
    int old_loop_mode = loop_mode;
    /* range restriction */
    loop_mode = mode % (LOOP_MODE_FREE + 1);
    switch (loop_mode)
    {
        case LOOP_MODE_ALL:
            play_start = range_start;
            play_end = range_end;
            break;

        case LOOP_MODE_FROM:
            play_start = xpos_to_time(split_x);
            play_end = range_end;
            break;

        case LOOP_MODE_TO:
            play_start = range_start;
            play_end = xpos_to_time(split_x);
            break;

        case LOOP_MODE_FREE:
            /* play_start is used when the song plays beyond its end */
            play_start = range_start;
            play_end = range_end;
            break;
    }

    if (loop_mode != old_loop_mode)
    {
        update_icons();
    }
}

/**
 * Readraws the osci without clear.
 */
static void redraw_osci(void)
{
    int x;
    for (x = 0; x < OSCI_WIDTH; x++)
    {
        if (osci_buffer[x] > 0)
        {
            rb->lcd_drawline
            (
                OSCI_X + x, OSCI_Y + OSCI_HEIGHT - 1,
                OSCI_X + x, OSCI_Y + OSCI_HEIGHT - osci_buffer[x] - 1
            );
        }
    }
}

/**
 * Sets the range of time in which the user can finetune the split
 * point. The split point is the center of the time range.
 */
static void set_range_by_time(
    struct mp3entry *mp3,
    unsigned int split_time,
    unsigned int range)
{
    if (mp3 != NULL)
    {
        if (range < MIN_RANGE_SIZE)
        {
            range = MIN_RANGE_SIZE;
        }
        range_start = (split_time > range / 2) ? (split_time - range / 2) : 0;
        range_end = MIN(range_start + range, mp3->length);
        split_x = time_to_xpos(split_time);

        splitedit_invalidate_osci();

        /* this sets the play_start / play_end */
        splitedit_set_loop_mode(splitedit_get_loop_mode());

        update_data();
        update_timebar(mp3);
    }
}

/**
 * Set the split point in screen coordinates
 */
void splitedit_set_split_x(int newx)
{
    int minx = split_x - 2 > 0 ? split_x - 2: 0;

    /* remove old split point from screen, only if moved */
    if (split_x != newx)
    {
        rb->lcd_invertrect (minx, OSCI_Y, 5, 1);
        rb->lcd_invertrect (split_x-1 > 0 ? split_x - 1: 0, OSCI_Y + 1, 3, 1);
        rb->lcd_invertrect (split_x, OSCI_Y + 2, 1, OSCI_HEIGHT - 2);
        rb->lcd_update_rect(minx, OSCI_Y, 5, OSCI_HEIGHT);
    }

    if (newx >= OSCI_X && newx < OSCI_X + OSCI_WIDTH)
    {
        split_x = newx;
        /* in LOOP_FROM / LOOP_TO modes play_start /play_end must be updated */
        splitedit_set_loop_mode(splitedit_get_loop_mode());

        /* display new split time  */
        update_data();
    }

    /* display new split point */
    minx = split_x - 2 > 0 ? split_x - 2: 0;
    rb->lcd_invertrect (minx, OSCI_Y, 5, 1);
    rb->lcd_invertrect (split_x - 1 > 0 ? split_x - 1: 0, OSCI_Y + 1, 3, 1);
    rb->lcd_invertrect (split_x, OSCI_Y + 2, 1, OSCI_HEIGHT - 2);
    rb->lcd_update_rect(minx, OSCI_Y, 5, OSCI_HEIGHT);
}

/**
 * returns the split point in screen coordinates
 */
int splitedit_get_split_x(void)
{
    return split_x;
}

/**
 * Clears the osci area and redraws it
 */
static void update_osci(void)
{
    rb->lcd_clearrect(OSCI_X, OSCI_Y, OSCI_WIDTH, OSCI_HEIGHT);
    redraw_osci();
    splitedit_set_split_x(splitedit_get_split_x());
    rb->lcd_update_rect(OSCI_X, OSCI_Y, OSCI_WIDTH, OSCI_HEIGHT);
}

/**
 * Zooms the visable and loopable range by the factor
 * (counter / denominator). The split point is used as
 * center point of the new selected range.
 */
static void zoom(struct mp3entry *mp3, int counter, int denominator)
{
    unsigned char oldbuf[OSCI_WIDTH];
    int oldrange = range_end - range_start;
    int range = oldrange * counter / denominator;
    int i;
    int oldindex;
    int oldsplitx;
    int splitx;
    int split;

    /* for stretching / shrinking a second buffer is needed */
    rb->memcpy(&oldbuf, &osci_buffer, sizeof osci_buffer);

    /* recalculate the new range and split point */
    oldsplitx = split_x;
    split = xpos_to_time(split_x);

    set_range_by_time(mp3, split, range);
    range = range_end - range_start;

    splitx = time_to_xpos(split);

    /* strech / shrink the existing osci buffer */
    for (i = 0; i < OSCI_WIDTH; i++)
    {
        /* oldindex = (i + OSCI_X - splitx) * range / oldrange + oldsplitx ;*/
        oldindex = (i*range / oldrange) + oldsplitx - (splitx*range /oldrange);
        if (oldindex >= 0 && oldindex < OSCI_WIDTH)
        {
            osci_buffer[i] = oldbuf[oldindex];
        }
        else
        {
            osci_buffer[i] = 0;
        }
    }

    splitx = time_to_xpos(split);
    splitedit_set_split_x(splitx);
    splitedit_invalidate_osci();

}

static void scroll(struct mp3entry *mp3)
{
    zoom(mp3, 1, 1);
    rb->lcd_update_rect(OSCI_X, OSCI_Y, LCD_WIDTH, OSCI_HEIGHT);
    update_osci();
    update_data();
}

/**
 * Zooms in by 3/4
 */
void splitedit_zoom_in(struct mp3entry *mp3)
{
    rb->lcd_clearrect(OSCI_X, OSCI_Y, OSCI_WIDTH, OSCI_HEIGHT);
    zoom(mp3, 3, 4);
    rb->lcd_update_rect(OSCI_X, OSCI_Y, LCD_WIDTH, OSCI_HEIGHT);
    update_osci();
    update_data();
}

/**
 * Zooms out by 4/3
 */
void splitedit_zoom_out(struct mp3entry *mp3)
{
    rb->lcd_clearrect(OSCI_X, OSCI_Y, LCD_WIDTH, OSCI_HEIGHT);
    zoom(mp3, 4, 3);
    rb->lcd_update_rect(OSCI_X, OSCI_Y, LCD_WIDTH, OSCI_HEIGHT);
    update_osci();
    update_data();
}

/**
 * Append part_no to the file name.
 */
static void generateFileName(char* file_name, int part_no)
{
    if (rb->strlen(file_name) <MAX_PATH)
    {
        int len = rb->strlen(file_name);
        int ext_len = rb->strlen(".mp3");
        if (rb->strcasecmp(
            &file_name[len - ext_len],
            ".mp3") == 0)
        {
            int i = 0;
            /* shift the extension one position to the right*/
            for (i = len; i > len - ext_len; i--)
            {
                file_name[i] = file_name[i - 1];
            }
            file_name[len - ext_len] = '0' + part_no;
        }
        else
        {
            rb->splash(0, true, "wrong extension");
            rb->button_get(true);
            rb->button_get(true);
        }
    }
    else
    {
        rb->splash(0, true, "name too long");
        rb->button_get(true);
        rb->button_get(true);

    }

}

/**
 * Copy bytes from src to dest while displaying a progressbar.
 * The files must be already open.
 */
static void copy_file(
    int dest,
    int src,
    unsigned int bytes,
    int prg_y,
    int prg_h)
{
    unsigned char *buffer;
    unsigned int i = 0;
    ssize_t bytes_read = 1; /* ensure the for loop is executed */
    unsigned int buffer_size;
    buffer = rb->plugin_get_buffer(&buffer_size);

    for (i = 0; i < bytes && bytes_read > 0; i += bytes_read)
    {
        ssize_t bytes_written;
        unsigned int bytes_to_read =
            bytes - i > buffer_size ? buffer_size : bytes - i;
        bytes_read = rb->read(src, buffer, bytes_to_read);
        bytes_written = rb->write(dest, buffer, bytes_read);

        rb->scrollbar(0, prg_y, LCD_WIDTH, prg_h, bytes, 0, i, HORIZONTAL);
        rb->lcd_update_rect(0, prg_y, LCD_WIDTH, prg_h);
    }
}

/**
 * Save the files, if the file_name is not NULL
 */
static int save(
    struct mp3entry *mp3,
    char *file_name1,
    char *file_name2,
    int splittime)
{
    int file1, file2, src_file;
    unsigned int end = 0;
    int retval = 0;

    /* Verify that file 1 doesn't exit yet */
    if (file_name1 != NULL)
    {
        file1 = rb->open(file_name1, O_RDONLY);
        if (file1 >= 0)
        {
            rb->close(file1);
            rb->splash(0, true, "File 1 exists. Please rename.");
            rb->button_get(true);
            rb->button_get(true);
            return -1;
        }
    }

    /* Verify that file 2 doesn't exit yet */
    if (file_name2 != NULL)
    {
        file2 = rb->open(file_name2, O_RDONLY);
        if (file2 >= 0)
        {
            rb->close(file2);
            rb->splash(0, true, "File 2 exists. Please rename.");
            rb->button_get(true);
            rb->button_get(true);
            return -2;
        }
    }

    /* find the file position of the split point */
    rb->mpeg_pause();
    rb->mpeg_ff_rewind(splittime);
    rb->yield();
    rb->yield();
    end = rb->mpeg_get_file_pos();

    /* open the source file */
    src_file = rb->open(mp3->path, O_RDONLY);
    if (src_file >= 0)
    {
        int close_stat = 0;
        int x, y;
        int offset;
        unsigned long last_header = rb->mpeg_get_last_header();

        rb->lcd_getstringsize("M", &x, &y);

        /* Find the next frame boundary */
        rb->lseek(src_file, end, SEEK_SET);
        rb->find_next_frame(src_file, &offset, 8000, last_header);
        rb->lseek(src_file, 0, SEEK_SET);
        end += offset;
        
        /* write the file 1 */
        if (file_name1 != NULL)
        {
            file1 = rb->open (file_name1, O_WRONLY | O_CREAT);
            if (file1 >= 0)
            {
                copy_file(file1, src_file, end, y*2 + 1, y -1);
                close_stat = rb->close(file1);

                if (close_stat != 0)
                {
                    rb->splash(0, true,
                        "failed closing file1: error %d", close_stat);
                    rb->button_get(true);
                    rb->button_get(true);
                }
            }
            else
            {
                rb->splash(0, true,
                    "Can't write File1: error %d", file1);
                rb->button_get(true);
                rb->button_get(true);
                retval = -1;
            }
        }
        /* if file1 hasn't been written we're not at the split point yet */
        else
        {
            if (rb->lseek(src_file, end, SEEK_SET) < (off_t)end)
            {
                rb->splash(0, true,
                    "Src file to short: error %d", src_file);
                rb->button_get(true);
                rb->button_get(true);
            }
        }

        if (file_name2 != NULL)
        {
            /* write file 2 */
            file2 = rb->open (file_name2, O_WRONLY | O_CREAT);
            if (file2 >= 0)
            {
                end = mp3->filesize - end;
                copy_file(file2, src_file, end, y * 5 + 1, y -1);
                close_stat = rb->close(file2);

                if (close_stat != 0)
                {
                    rb->splash(0, true,
                        "failed: closing file2: error %d", close_stat);
                    rb->button_get(true);
                    rb->button_get(true);
                }
            }
            else
            {
                rb->splash(0, true,
                    "Can't write File2: error %d", file2);
                rb->button_get(true);
                rb->button_get(true);
                retval = -2;
            }
        }

        close_stat = rb->close(src_file);
        if (close_stat != 0)
        {
            rb->splash(0, true,
                "failed: closing src: error %d", close_stat);
            rb->button_get(true);
            rb->button_get(true);
        }
    }
    else
    {
        rb->splash(0, true, "Source file not found");
        rb->button_get(true);
        rb->button_get(true);
        retval = -3;
    }

    rb->mpeg_resume();

    return retval;
}

/**
 * Let the user choose which file to save with which name
 */
static void save_editor(struct mp3entry *mp3, int splittime)
{
    bool exit_request = false;
    int choice = 0;
    int button = BUTTON_NONE;
    char part1_name [MAX_PATH];
    char part2_name [MAX_PATH];
    bool part1_save = true;
    bool part2_save = true;

    /* file name for left part */
    rb->strncpy(part1_name, mp3->path, MAX_PATH);
    generateFileName(part1_name, 1);

    /* file name for right part */
    rb->strncpy(part2_name, mp3->path, MAX_PATH);
    generateFileName(part2_name, 2);

    while (!exit_request)
    {
        int pos;
        rb->lcd_clear_display();

        /* Save file1? */
        rb->lcd_puts_style(0, 0, "Save part 1?", choice == SE_PART1_SAVE);
        rb->lcd_puts(13, 0, part1_save?"yes":"no");

        /* trim to display the filename without path */
        for (pos = rb->strlen(part1_name); pos > 0; pos--)
        {
            if (part1_name[pos] == '/')
                break;
        }
        pos++;

        /* File name 1 */
        rb->lcd_puts_scroll_style(0, 1,
            &part1_name[pos], choice == SE_PART1_NAME);

        /* Save file2? */
        rb->lcd_puts_style(0, 3, "Save part 2?", choice == SE_PART2_SAVE);
        rb->lcd_puts(13, 3, part2_save?"yes":"no");

        /* trim to display the filename without path */
        for (pos = rb->strlen(part2_name); pos > 0; pos --)
        {
            if (part2_name[pos] == '/')
                break;
        }
        pos++;

        /* File name 2 */
        rb->lcd_puts_scroll_style(0, 4,
            &part2_name[pos], choice == SE_PART2_NAME);

        /* Save */
        rb->lcd_puts_style(0, 6, "Save", choice == SE_SAVE);

        rb->lcd_update();


        button = rb->button_get(true);
        switch (button)
        {
        case BUTTON_UP:
            choice = (choice + SE_COUNT - 1) % SE_COUNT;
            break;

        case BUTTON_DOWN:
            choice = (choice + 1) % SE_COUNT;
            break;

        case SPLITEDIT_MENU_RUN:
            switch (choice)
            {
            int saved;

            case SE_PART1_SAVE:
                part1_save = !part1_save;
                break;

            case SE_PART1_NAME:
                rb->kbd_input(part1_name, MAX_PATH);
                break;

            case SE_PART2_SAVE:
                part2_save = !part2_save;
                break;

            case SE_PART2_NAME:
                rb->kbd_input(part2_name, MAX_PATH);
                break;

            case SE_SAVE:
                rb->lcd_stop_scroll();
                rb->lcd_clearrect(0, 6*8, LCD_WIDTH, LCD_HEIGHT);
                saved = save
                (
                    mp3,
                    part1_save?part1_name:NULL,
                    part2_save?part2_name:NULL,
                    splittime
                );

                /* if something failed the user may go on choosing */
                if (saved >= 0)
                {
                    exit_request = true;
                }
                break;
            }
            break;

        case SPLITEDIT_QUIT:
            exit_request = true;
            break;

        default:
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            {
                splitedit_exit_code = PLUGIN_USB_CONNECTED;
                exit_request = true;
            }
            break;
        }
    }
}

/**
 * The main loop of the editor
 */
unsigned long splitedit_editor(struct mp3entry * mp3_to_split,
                          unsigned int split_time,
                          unsigned int range)
{
    int button = BUTTON_NONE;
    int lastbutton = BUTTON_NONE;
    struct mp3entry *mp3 = mp3_to_split;
    unsigned int last_elapsed = 0;
    int lastx = OSCI_X + (OSCI_WIDTH / 2);
    int retval = -1;

    if (mp3 != NULL)
    {
        /*unsigned short scheme = SCHEME_SPLIT_EDITOR;*/
        bool exit_request = false;
        set_range_by_time(mp3, split_time, range);
        splitedit_set_loop_mode(LOOP_MODE_ALL);
        update_icons();

        /*while (scheme != SCHEME_RETURN) {*/
        while (!exit_request)
        {
            unsigned int elapsed ;
            int x ;

            /* get position */
            elapsed = mp3->elapsed;
            x = time_to_xpos(elapsed);

            /* are we still in the zoomed range? */
            if (elapsed > play_start && elapsed < play_end)
            {
                /* read volume info */
                unsigned short volume;
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
                volume = rb->mas_codec_readreg(0x0c);
                volume += rb->mas_codec_readreg(0x0d);
                volume = volume / 2;
                volume = rb->peak_meter_scale_value(volume, OSCI_HEIGHT);
#else
                volume = OSCI_HEIGHT / 2;
#endif

                /* update osci_buffer */
                if (osci_valid || lastx == x)
                {
                    int index = x - OSCI_X;
                    osci_buffer[index] = MAX(osci_buffer[index], volume);
                }
                else
                {
                    int i;
                    osci_buffer[x - OSCI_X] = volume;
                    for (i = lastx + 1; i < x; i++)
                    {
                        osci_buffer[i - OSCI_X] = 0;
                    }
                }

                /* make room */
                rb->lcd_clearrect(lastx + 1, OSCI_Y, x - lastx, OSCI_HEIGHT);
                /* draw a value */
                if (osci_buffer[x - OSCI_X] > 0)
                {
                    int i;
                    for (i = lastx +1; i <= x; i++)
                    {
                        rb->lcd_drawline
                            (
                            i, OSCI_Y + OSCI_HEIGHT - 1,
                            i, OSCI_Y + OSCI_HEIGHT - osci_buffer[i - OSCI_X]-1
                            );
                    }
                }

                /* mark the current position */
                if (lastx != x)
                {
                    rb->lcd_invertrect(lastx, OSCI_Y, 1, OSCI_HEIGHT);
                    rb->lcd_invertrect(x, OSCI_Y, 1, OSCI_HEIGHT);
                }

                /* mark the split point */
                if ((x > split_x - 2) && (lastx < split_x + 3))
                {
                    if ((lastx < split_x) && (x >= split_x))
                    {
                        rb->lcd_invertrect
                        (
                            split_x, OSCI_Y + 2,
                            1, OSCI_HEIGHT - 2
                        );
                    }
                    rb->lcd_drawline(split_x -2, OSCI_Y, split_x + 2, OSCI_Y);
                    rb->lcd_drawline(split_x-1, OSCI_Y+1, split_x +1,OSCI_Y+1);
                }

                /* make visible */
                if (lastx <= x)
                {
                    rb->lcd_update_rect(lastx, OSCI_Y, x-lastx+1, OSCI_HEIGHT);
                }
                else
                {
                    rb->lcd_update_rect
                    (
                        lastx, OSCI_Y,
                        OSCI_X + OSCI_WIDTH - lastx, OSCI_HEIGHT
                    );
                    rb->lcd_update_rect(0, OSCI_Y, x + 1, OSCI_HEIGHT);
                }

                lastx = x;
            }

            /* we're not in the zoom range -> rewind */
            else
            {
                if (elapsed >= play_end)
                {
                    switch (splitedit_get_loop_mode())
                    {
                    unsigned int range_width;

                    case LOOP_MODE_ALL:
                    case LOOP_MODE_TO:
                        rb->mpeg_pause();
                        rb->mpeg_ff_rewind(range_start);
#ifdef HAVE_MMC
/* MMC is slow - wait some time to allow track reload to finish */
                        rb->sleep(HZ/20);
                        if (mp3->elapsed > play_end) /* reload in progress */
                            rb->splash(10*HZ, true, "Wait - reloading");
#endif
                        rb->mpeg_resume();
                        break;

                    case LOOP_MODE_FROM:
                        rb->mpeg_pause();
                        rb->mpeg_ff_rewind(xpos_to_time(split_x));
#ifdef HAVE_MMC
/* MMC is slow - wait some time to allow track reload to finish */
                        rb->sleep(HZ/20);
                        if (mp3->elapsed > play_end) /* reload in progress */
                            rb->splash(10*HZ, true, "Wait - reloading");
#endif
                        rb->mpeg_resume();
                        break;

                    case LOOP_MODE_FREE:
                        range_width = range_end - range_start;
                        set_range_by_time(mp3,
                            range_end + range_width / 2, range_width);

                        /* play_end und play_start anpassen */
                        splitedit_set_loop_mode(LOOP_MODE_FREE);
                        rb->memset(osci_buffer, 0, sizeof osci_buffer);
                        update_osci();
                        rb->lcd_update();
                        break;
                    }
                }
            }

            button = rb->button_get(false);
            rb->yield();

            /* here the evaluation of the key scheme starts.
            All functions the user triggers are called from
            within execute_scheme */
            /* key_scheme_execute(button, &scheme); */
            switch (button)
            {
            case SPLITEDIT_PLAY:
#ifdef SPLITEDIT_PLAY_PRE
                if (lastbutton != SPLITEDIT_PLAY_PRE)
                    break;
#endif
                rb->mpeg_pause();
                rb->mpeg_ff_rewind(xpos_to_time(split_x));
                rb->mpeg_resume();
                break;

            case BUTTON_UP:
                splitedit_zoom_in(mp3);
                lastx = time_to_xpos(mp3->elapsed);
                break;

            case BUTTON_DOWN:
                splitedit_zoom_out(mp3);
                lastx = time_to_xpos(mp3->elapsed);
                break;

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
#if defined(SPLITEDIT_SPEED150) && defined(SPLITEDIT_SPEED100) && defined(SPLITEDIT_SPEED50)
            case SPLITEDIT_SPEED150:
                rb->mpeg_set_pitch(1500);
                splitedit_invalidate_osci();
                break;

            case SPLITEDIT_SPEED100:
                rb->mpeg_set_pitch(1000);
                splitedit_invalidate_osci();
                break;

            case SPLITEDIT_SPEED50:
                rb->mpeg_set_pitch(500);
                splitedit_invalidate_osci();
                break;
#endif
#endif

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (splitedit_get_split_x() > OSCI_X + 2)
                {
                    splitedit_set_split_x(splitedit_get_split_x() - 1);
                }
                else
                {
                    scroll(mp3);
                    lastx = time_to_xpos(mp3->elapsed);
                }
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (splitedit_get_split_x() < OSCI_X + OSCI_WIDTH-3)
                {
                    splitedit_set_split_x(splitedit_get_split_x() + 1);
                }
                else
                {
                    scroll(mp3);
                    lastx = time_to_xpos(mp3->elapsed);
                }
                break;

            case SPLITEDIT_SAVE:
                save_editor(mp3, xpos_to_time(split_x));
                rb->lcd_clear_display();
                update_osci();
                update_timebar(mp3);
                update_icons();
                break;

            case SPLITEDIT_LOOP_MODE:
                splitedit_set_loop_mode(splitedit_get_loop_mode() + 1);
                update_icons();
                break;

            case SPLITEDIT_SCALE:
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
                rb->peak_meter_set_use_dbfs(rb->peak_meter_get_use_dbfs() +1);
#endif
                splitedit_invalidate_osci();
                update_icons();
                break;

            case SPLITEDIT_QUIT:
                exit_request = true;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    splitedit_exit_code = PLUGIN_USB_CONNECTED;
                    exit_request = true;
                }
                break;

            }
            if (button != BUTTON_NONE)
                lastbutton = button;

            if (validation_start == ~(unsigned int)0)
            {
                if (elapsed < range_end && elapsed > range_start)
                {
                    validation_start = elapsed;
                }
                else
                {
                    int endx = time_to_xpos(range_end);
                    validation_start = xpos_to_time(endx - 2);
                }
                last_elapsed = elapsed + 1;
            }
            else
            {
                if ((last_elapsed <= validation_start) &&
                    (elapsed > validation_start))
                {
                    osci_valid = true;
                }

                last_elapsed = elapsed;
            }
            update_data();

            if (mp3 != rb->mpeg_current_track())
            {
                struct mp3entry *new_mp3 = rb->mpeg_current_track();
                if (rb->strncasecmp(path_mp3, new_mp3->path,
                                    sizeof (path_mp3)))
                {
                    rb->splash(0, true,"Abort due to file change");
                    rb->button_get(true);
                    rb->button_get(true);
                    exit_request = true;
                }
                else
                {
                    mp3 = new_mp3;
                    rb->mpeg_pause();
                    rb->mpeg_flush_and_reload_tracks();
                    rb->mpeg_ff_rewind(range_start);
                    rb->mpeg_resume();
                }
            }
        }
    }
    return retval;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    struct mp3entry* mp3;

    (void)parameter;
    rb = api;
    rb->lcd_clear_display();
    rb->lcd_update();
    mp3 = rb->mpeg_current_track();
    if (mp3 != NULL)
    {
        if (rb->mpeg_status() & MPEG_STATUS_PAUSE)
        {
            rb->mpeg_resume();
        }
        splitedit_editor(mp3, mp3->elapsed, MIN_RANGE_SIZE * 8);
    }
    else
    {
        rb->splash(0, true, "Play or pause a mp3 file first.");
        rb->button_get(true);
        rb->button_get(true);
    }
    return splitedit_exit_code;
}
#endif
#endif
