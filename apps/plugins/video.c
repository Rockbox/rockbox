/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Plugin for video playback
* Reads raw image data + audio data from a file
*
* Copyright (C) 2003-2004 Jörg Hohensohn aka [IDC]Dragon
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
#include "plugin.h"
#include "sh7034.h"
#include "system.h"
#include "../apps/recorder/widgets.h" /* not in search path, booh */

#ifndef SIMULATOR /* not for simulator by now */
#ifdef HAVE_LCD_BITMAP /* and definitely not for the Player, haha */

#define SCREENSIZE (LCD_WIDTH*LCD_HEIGHT/8) /* in bytes */
#define FILEBUFSIZE (SCREENSIZE*4) /* must result in a multiple of 512 */
#define FPS 71 /* desired framerate */
#define WIND_MAX 9 /* max FF/FR speed */

#define FRAMETIME (FREQ/8/FPS) /* internal timer4 value */

/* globals */
static struct plugin_api* rb; /* here is a global api struct pointer */

static enum 
{
    playing,
    paused,
    stop,
    exit
} state = playing;

static int playstep = 1; /* for current speed and direction */
static int acceleration = 0;
static long time; /* to calculate the playing time */


/* test for button, returns relative frame and may change state */
int check_button(void)
{
    int button;
    int frame; /* result: relative frame */
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
            {
                acceleration--;
                playstep = acceleration/4; /* FR */
                if (playstep > -2)
                    playstep = -2;
                if (playstep < -WIND_MAX)
                    playstep = -WIND_MAX;
            }
            break;

        case BUTTON_RIGHT | BUTTON_REPEAT:
            if (state == paused)
                frame = 1; /* single step */
            else if (state == playing)
            {
                acceleration++;
                playstep = acceleration/4; /* FF */
                if (playstep < 2)
                    playstep = 2;
                if (playstep > WIND_MAX)
                    playstep = WIND_MAX;
            }
            break;

        case BUTTON_PLAY:
            if (state == playing && (playstep == 1 || playstep == -1))
                state = paused;
            else if (state == paused)
                state = playing;
            playstep = 1;
            acceleration = 0;
            break;

        case BUTTON_LEFT:
            if (state == paused)
                frame = -1; /* single step back */
            else if (state == playing)
                playstep = -1; /* rewind */
            acceleration = 0;
            break;
        
        case BUTTON_RIGHT:
            if (state == paused)
                frame = 1; /* single step */
            else if (state == playing)
                playstep = 1; /* forward */
            acceleration = 0;
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
        /* wait for frame to be due */
        while (TCNT4 < FRAMETIME) /* use our timer 4 */
            rb->yield(); /* yield to the other treads */
        TCNT4 -= FRAMETIME;
        
        rb->lcd_blit(p_current, 0, 0, LCD_WIDTH, LCD_HEIGHT/8, LCD_WIDTH);

        shown++;

        delta = check_button();
        p_current += delta * SCREENSIZE;
        if (p_current >= p_end || p_current < p_start)
            p_current = p_start; /* wrap */
    } while(state != stop && state != exit);

    return (state != exit) ? shown : -1;
}


/* play from file, exit if OFF is pressed */
int show_file(unsigned char* p_buffer, int fd)
{
	long tag[FILEBUFSIZE/512]; /* I treat the buffer as direct-mapped cache */
	long framepos = 0; /* position of frame in file */
	long filesize = rb->filesize(fd);
    long filepos = 0; /* my own counting */
    int shown = 0;

	long readfrom;
    long readto;
    int read; /* amount read from disk */
	long frame_offset; /* pos of frame in buffer */
    long writefrom; /* round down to sector */
	long sector; /* sector in frame buffer */
	long pos_aligned, orig_aligned; /* round down to sector */
    char buf[10];
    
    rb->memset(&tag, 0xFF, sizeof(tag)); /* invalidate cache */

	do
	{
		/* load the frame into memory */
		readfrom = readto = -1; /* invalidate */
		frame_offset = framepos % FILEBUFSIZE; /* pos of frame in buffer */
        writefrom = frame_offset & ~511; /* round down to sector */
		sector = frame_offset / 512; /* sector in frame buffer */
		orig_aligned = pos_aligned = framepos & ~511; /* down to sector */

		do
        {
			if (tag[sector] != pos_aligned) /* in cache? */
			{   /* not cached */
                tag[sector] = pos_aligned;
				if (readfrom == -1) /* not used yet? */
                {
					readfrom = pos_aligned; /* set start */
                    writefrom += pos_aligned - orig_aligned;
                }
                readto = pos_aligned; /* set stop */
			}
            pos_aligned += 512;
            sector++;
        } while (pos_aligned < framepos + SCREENSIZE);

        if (readfrom != -1)
        {   /* need to read from disk */
            if (filepos != readfrom)
            {   /* need to seek */
                filepos = rb->lseek(fd, readfrom, SEEK_SET);
            }
            read = readto - readfrom + 512;
            /* read the sector(s) */
            filepos += rb->read(fd, p_buffer + writefrom, read); 
        }
        else
            read = 0;

        /* wait for frame to be due */
        while (TCNT4 < FRAMETIME) /* use our timer 4 */
            rb->yield(); /* yield to the other treads */
        TCNT4 -= FRAMETIME;

        /* display OSD if FF/FR */
        if (playstep != 1 && playstep != -1)
        {
            int w,h;

            if (playstep > 0)
                rb->snprintf(buf, sizeof(buf), "%d>>", playstep);
            else
                rb->snprintf(buf, sizeof(buf), "%d<<", -playstep);

            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy(0, LCD_HEIGHT-h, buf);
            
            w++;
            rb->slidebar(w, LCD_HEIGHT-7, LCD_WIDTH-w, 7, 
                (100 * filepos)/filesize, Grow_Right);
            rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
            rb->lcd_blit(p_buffer + frame_offset, 0, 0, 
                LCD_WIDTH, LCD_HEIGHT/8 - 1, LCD_WIDTH);
        }
        else
        {   /* display the frame normally */
            rb->lcd_blit(p_buffer + frame_offset, 0, 0, LCD_WIDTH, 
                LCD_HEIGHT/8, LCD_WIDTH);
        }

		/* query keys to determine next frame */
		framepos += check_button() * SCREENSIZE;
        if (framepos <= 0)
        {
            state = paused;
            framepos = 0;
        }
        else if (framepos >= filesize)
        {
            if (state == playing)
            {
                if (playstep == 1)
                    state = stop; /* reached the end while playing normally */
                else
                {
                    state = paused; /* it may have been FF */
                    playstep = 1;
                }
            }

            framepos = filesize - SCREENSIZE;
            /* in case the file size is no integer multiple */
            framepos -= framepos % SCREENSIZE;
        }

        shown++;

	} while (state != stop && state != exit);

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

    /* init timer 4, crude code */
    IPRD = (IPRD & 0xFF0F); // disable interrupt
    and_b(~0x10, &TSTR); // Stop the timer 4
    and_b(~0x10, &TSNC); // No synchronization
    and_b(~0x10, &TMDR); // Operate normally
    TCR4 = 0x03; // no clear at GRA match, sysclock/8
    TCNT4 = 0; // start counting at 0

    file_size =  rb->filesize(fd);
    if (file_size <= buffer_size)
    {   /* we can read the whole file in advance */
        got_now = rb->read(fd, p_buffer, file_size);
        rb->close(fd);
        frames = got_now / (LCD_WIDTH*LCD_HEIGHT/8);
        or_b(0x10, &TSTR); // start timer 4
        time = *rb->current_tick;
        shown = show_buffer(p_buffer, frames);
        time = *rb->current_tick - time;
    }
    else
    {   /* we need to stream */
        or_b(0x10, &TSTR); // start timer 4
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

