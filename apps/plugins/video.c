/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Experimental plugin for halftoning
* Reads raw image data from a file
*
* Copyright (C) 2003 Jörg Hohensohn [IDC]Dragon
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
#include "plugin.h"
#include "system.h"

#ifndef SIMULATOR /* not for simulator by now */
#ifdef HAVE_LCD_BITMAP /* and definitely not for the Player, haha */

#define INT_MAX ((int)(~(unsigned)0 >> 1))
#define INT_MIN (-INT_MAX-1)

#define SCREENSIZE (LCD_WIDTH*LCD_HEIGHT/8) /* in bytes */
#define FILEBUFSIZE SCREENSIZE*4 /* must result in a multiple of 512 */

#define WIND_STEP 5


/* globals */
static struct plugin_api* rb; /* here is a global api struct pointer */

static enum 
{
    playing,
    paused,
    stop,
    exit
} state = playing;

static int playstep = 1; /* forcurrent speed and direction */
static long time; /* to calculate the playing time */


/* test for button, returns relative frame and may change state */
int check_button(void)
{
    int button;
    int frame;
    bool loop;
    
    frame = playstep; /* default */

    do
    {
        loop = false;
        if (state == playing)
            button = rb->button_get(false);
        else
        {
            long current = *rb->current_tick;
            button = rb->button_get(true); /* block */
            time += *rb->current_tick - current; /* don't time while waiting */
        }

        switch(button)
        {
        case BUTTON_NONE:
            break; /* quick exit */

        case BUTTON_LEFT | BUTTON_REPEAT:
            if (state == paused)
                frame = -1; /* single step back */
            else if (state == playing)
                playstep = -WIND_STEP; /* FR */
            break;

        case BUTTON_RIGHT | BUTTON_REPEAT:
            if (state == paused)
                frame = 1; /* single step */
            else if (state == playing)
                playstep = WIND_STEP; /* FF */
            break;

        case BUTTON_PLAY:
            if (state == playing && playstep == 1)
                state = paused;
            else if (state == paused)
                state = playing;
            playstep = 1;
            break;

        case BUTTON_LEFT:
            if (state == paused)
                frame = -1; /* single step back */
            else if (state == playing)
                playstep = -1; /* rewind */
            break;
        
        case BUTTON_RIGHT:
            if (state == paused)
                frame = 1; /* single step */
            else if (state == playing)
                playstep = 1; /* forward */
            break;

        case BUTTON_UP:
            if (state == paused)
                frame = INT_MAX; /* end of clip */
            break;

        case BUTTON_DOWN:
            frame = INT_MIN; /* start of clip */
            break;

        case BUTTON_OFF:
            state = stop;
            break;

        case SYS_USB_CONNECTED:
            state = exit;
            break;

        default:
            if (state != playing)
                loop = true;
        }
    }
    while (loop);

    return frame;
}


int WaitForButton(void)
{
    int button;
    
    do
    {
        button = rb->button_get(true);
    } while ((button & BUTTON_REL) && button != SYS_USB_CONNECTED);
    
    return button;
}


/* play from memory, loop until OFF is pressed */
int show_buffer(unsigned char* p_start, int frames)
{
    unsigned char* p_current = p_start;
    unsigned char* p_end = p_start + SCREENSIZE * frames;
    int shown = 0;
    int delta; /* next frame */

    do
    {
        rb->lcd_blit(p_current, 0, 0, LCD_WIDTH, LCD_HEIGHT/8, LCD_WIDTH);

        rb->yield(); /* yield to the other treads */
        shown++;

        delta = check_button();
        if (delta == INT_MIN)
            p_current = p_start;
        else if (delta == INT_MAX)
            p_current = p_end - SCREENSIZE;
        else
        {
            p_current += delta * SCREENSIZE;
            if (p_current >= p_end || p_current < p_start)
                p_current = p_start; /* wrap */
        }

    } while(state != stop && state != exit);

    return (state != exit) ? shown : -1;
}


/* play from file, exit if OFF is pressed */
int show_file(unsigned char* p_buffer, int fd)
{
    int shown = 0;
    int delta; /* next frame */
    /* tricky buffer handling to read only whole sectors, then no copying needed */
    unsigned char* p_file = p_buffer; /* file read */
    unsigned char* p_screen = p_buffer; /* display */
    unsigned char* p_end = p_buffer + FILEBUFSIZE; /* for wraparound test */
    int needed; /* read how much data to complete a frame */
    int got_now; /* how many gotten for this frame */
    int read_now; /* size to read for this frame, 512 or 1024 */

    do
    {
        needed = SCREENSIZE - (p_file - p_screen); /* minus what we have */
        read_now = (needed + (SECTOR_SIZE-1)) & ~(SECTOR_SIZE-1); /* round up to whole sectors */

        got_now = rb->read(fd, p_file, read_now); /* read the sector(s) */
        rb->lcd_blit(p_screen, 0, 0, LCD_WIDTH, LCD_HEIGHT/8, LCD_WIDTH);

        p_screen += SCREENSIZE;
        if (p_screen >= p_end)
            p_screen = p_buffer; /* wrap */

        p_file += got_now;
        if (p_file >= p_end)
            p_file = p_buffer; /* wrap */

        delta = check_button();

        if (read_now < SCREENSIZE) /* below average? time to do something else */
            rb->yield(); /* yield to the other treads */

        if (delta != 1)
        {   /* need to seek */
            long newpos;
            
            rb->yield(); /* yield in case we didn't do it above */

            if (delta == INT_MIN)
                newpos = 0;
            else if (delta == INT_MAX)
                newpos = rb->filesize(fd) - SCREENSIZE;
            else
            {
                bool pause = false;
                newpos = rb->lseek(fd, 0, SEEK_CUR) - (p_file - p_screen) + (delta-1) * SCREENSIZE;
            
                if (newpos < 0)
                {
                    newpos = 0;
                    pause = true; /* go in paused mode */
                }

                if (newpos > rb->filesize(fd) - SCREENSIZE)
                {
                    newpos = rb->filesize(fd) - SCREENSIZE;
                    pause = true; /* go in paused mode */
                }

                if (pause && state == playing)
                    state = paused;
            }

            newpos -= newpos % SCREENSIZE; /* should be "even", just to make shure */

            rb->lseek(fd, newpos & ~(SECTOR_SIZE-1), SEEK_SET); /* round down to sector */
            newpos %= FILEBUFSIZE; /* make it an offset within buffer */ 
            p_screen = p_buffer + newpos;
            p_file = p_buffer + (newpos & ~(SECTOR_SIZE-1));
        }

        shown++;
    } while (got_now >= needed && state != stop && state != exit);

    return (state != exit) ? shown : -1;
}


int main(char* filename)
{
    char buf[32];
    int buffer_size, file_size;
    unsigned char* p_buffer;
    int fd; /* file descriptor handle */
    int got_now; /* how many bytes read from file */
    int frames, shown;
    int button;
    int fps;

    p_buffer = rb->plugin_get_buffer(&buffer_size);
    if (buffer_size < FILEBUFSIZE)
        return PLUGIN_ERROR; /* not enough memory */
    
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return PLUGIN_ERROR;

    file_size =  rb->filesize(fd);
    if (file_size <= buffer_size)
    {   /* we can read the whole file in advance */
        got_now = rb->read(fd, p_buffer, file_size);
        rb->close(fd);
        frames = got_now / (LCD_WIDTH*LCD_HEIGHT/8);
        time = *rb->current_tick;
        shown = show_buffer(p_buffer, frames);
        time = *rb->current_tick - time;
    }
    else
    {   /* we need to stream */
        time = *rb->current_tick;
        shown = show_file(p_buffer, fd);
        time = *rb->current_tick - time;
        rb->close(fd);
    }

    if (shown == -1) /* exception */
        return PLUGIN_USB_CONNECTED;

    fps = (shown * HZ *100) / time; /* 100 times fps */

    rb->lcd_clear_display();
    rb->snprintf(buf, sizeof(buf), "%d frames shown", shown);
    rb->lcd_puts(0, 0, buf);
    rb->snprintf(buf, sizeof(buf), "%d.%02d seconds", time/HZ, time%HZ);
    rb->lcd_puts(0, 1, buf);
    rb->snprintf(buf, sizeof(buf), "%d.%02d fps", fps/100, fps%100);
    rb->lcd_puts(0, 2, buf);
    rb->snprintf(buf, sizeof(buf), "file: %d bytes", file_size);
    rb->lcd_puts(0, 6, buf);
    rb->snprintf(buf, sizeof(buf), "buffer: %d bytes", buffer_size);
    rb->lcd_puts(0, 7, buf);
    rb->lcd_update();
    button = WaitForButton();
    return (button == SYS_USB_CONNECTED) ? PLUGIN_USB_CONNECTED : PLUGIN_OK;


}


/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int ret;
    /* this macro should be called as the first thing you do in the plugin.
    it test that the api version and model the plugin was compiled for
    matches the machine it is running on */
    TEST_PLUGIN_API(api);
    
    rb = api; /* copy to global api pointer */
    
    if (parameter == NULL)
    {
        rb->splash(HZ*2, true, "Play .rvf file!");
        return PLUGIN_ERROR;
    }

    /* now go ahead and have fun! */
    ret = main((char*) parameter);
    if (ret==PLUGIN_USB_CONNECTED)
        rb->usb_screen();
    return ret;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
#endif /* #ifndef SIMULATOR */
