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
#ifdef HAVE_LCD_BITMAP

#include "plugin.h"
#include "system.h"

#define DEFAULT_FILENAME "/default.rvf"
#define SCREENSIZE (LCD_WIDTH*LCD_HEIGHT/8) /* in bytes */
#define FILEBUFSIZE SCREENSIZE*4 /* must result in a multiple of 512 */

static struct plugin_api* rb; /* here is a global api struct pointer */

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
    int button;

    do
    {
        rb->lcd_blit(p_current, 0, 0, LCD_WIDTH, LCD_HEIGHT/8, LCD_WIDTH);
        p_current += SCREENSIZE;
        if (p_current >= p_end)
            p_current = p_start; /* wrap */

        rb->yield(); /* yield to the other treads */
        shown++;

        button = rb->button_get(false);
    } while(button != BUTTON_OFF && button != SYS_USB_CONNECTED);

    return (button != SYS_USB_CONNECTED) ? shown : SYS_USB_CONNECTED;
}


/* play from file, exit if OFF is pressed */
int show_file(unsigned char* p_buffer, int fd)
{
    int got_now; /* how many gotten for this frame */
    int shown = 0;
    int button = BUTTON_NONE;
    /* tricky buffer handling to read only whole sectors, then no copying needed */
    unsigned char* p_file = p_buffer; /* file read */
    unsigned char* p_screen = p_buffer; /* display */
    unsigned char* p_end = p_buffer + FILEBUFSIZE; /* for wraparound test */
    int needed; /* read how much data to complete a frame */
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

        if (read_now < SCREENSIZE) /* below average? time to do something else */
        {
            rb->yield(); /* yield to the other treads */
            button = rb->button_get(false);
        }

        shown++;

    } while (got_now >= needed 
          && button != BUTTON_OFF 
          && button != SYS_USB_CONNECTED);

    return (button != SYS_USB_CONNECTED) ? shown : SYS_USB_CONNECTED;
}


int main(char* filename)
{
    char buf[32];
    int buffer_size, file_size;
    unsigned char* p_buffer;
    int fd; /* file descriptor handle */
    int got_now; /* how many bytes read from file */
    int frames, shown;
    long time;
    int button;

    p_buffer = rb->plugin_get_buffer(&buffer_size);
    if (buffer_size < FILEBUFSIZE)
        return PLUGIN_ERROR; /* not enough memory */
    
    /* compose filename if none given */
    if (filename == NULL)
    {
        filename = DEFAULT_FILENAME;
    }

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

    rb->close(fd);
    and_b(~0x40, &PBDRL); /* hack workaround to get the LED off */

    if (shown == SYS_USB_CONNECTED) /* exception */
        return PLUGIN_USB_CONNECTED;

    rb->lcd_clear_display();
    rb->snprintf(buf, sizeof(buf), "%d frames", shown);
    rb->lcd_puts(0, 0, buf);
    rb->snprintf(buf, sizeof(buf), "%d.%02d seconds", time/HZ, time%HZ);
    rb->lcd_puts(0, 1, buf);
    rb->snprintf(buf, sizeof(buf), "%d fps", (shown * HZ + time/2) / time);
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
    /* this macro should be called as the first thing you do in the plugin.
    it test that the api version and model the plugin was compiled for
    matches the machine it is running on */
    TEST_PLUGIN_API(api);
    
    rb = api; /* copy to global api pointer */
    
    /* now go ahead and have fun! */
    return main((char*) parameter);
}

#endif // #ifdef HAVE_LCD_BITMAP