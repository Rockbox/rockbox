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
* !!!!!!!!!! Code Police free zone !!!!!!!!!!
*
* Copyright (C) 2003-2004 J�g Hohensohn aka [IDC]Dragon
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


/****************** imports ******************/

#include "plugin.h"
#include "sh7034.h"
#include "system.h"
#include "helper.h"

#ifndef SIMULATOR /* not for simulator by now */
#ifdef HAVE_LCD_BITMAP /* and definitely not for the Player, haha */

PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define VIDEO_STOP_SEEK BUTTON_PLAY
#define VIDEO_RESUME BUTTON_PLAY
#define VIDEO_DEBUG BUTTON_F1
#define VIDEO_CONTRAST_DOWN BUTTON_F2
#define VIDEO_CONTRAST_UP BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD
#define VIDEO_STOP_SEEK_PRE BUTTON_MENU
#define VIDEO_STOP_SEEK (BUTTON_MENU | BUTTON_REL)
#define VIDEO_RESUME BUTTON_RIGHT
#define VIDEO_CONTRAST_DOWN (BUTTON_MENU | BUTTON_DOWN)
#define VIDEO_CONTRAST_UP (BUTTON_MENU | BUTTON_UP)

#endif
/****************** constants ******************/

#define SCREENSIZE (LCD_WIDTH*LCD_HEIGHT/8) /* in bytes */
#define FPS 68 /* default fps for headerless (old video-only) file */
#define MAX_ACC 20 /* maximum FF/FR speedup */
#define FF_TICKS 3000; /* experimentally found nice */

/* trigger levels, we need about 80 kB/sec */
#define SPINUP_INIT 5000 /* from what level on to refill, in milliseconds */
#define SPINUP_SAFETY 700 /* how much on top of the measured spinup time */
#define CHUNK (1024*32) /* read size */


/****************** prototypes ******************/
void timer4_isr(void); /* IMIA4 ISR */
int check_button(void); /* determine next relative frame */


/****************** data types ******************/

/* plugins don't introduce headers, so structs are repeated from rvf_format.h */

#define HEADER_MAGIC 0x52564668 /* "RVFh" at file start  */
#define AUDIO_MAGIC  0x41756446 /* "AudF" for each audio block */
#define FILEVERSION  100 /* 1.00 */

/* format type definitions */
#define VIDEOFORMAT_NO_VIDEO       0
#define VIDEOFORMAT_RAW            1
#define AUDIOFORMAT_NO_AUDIO       0
#define AUDIOFORMAT_MP3            1
#define AUDIOFORMAT_MP3_BITSWAPPED 2

/* bit flags */
#define FLAG_LOOP 0x00000001 /* loop the playback, e.g. for stills */

typedef struct /* contains whatever might be useful to the player */
{
    /* general info (16 entries = 64 byte) */
    unsigned long magic; /* HEADER_MAGIC */
    unsigned long version; /* file version */
    unsigned long flags; /* combination of FLAG_xx */
    unsigned long blocksize; /* how many bytes per block (=video frame) */
    unsigned long bps_average; /* bits per second of the whole stream */
    unsigned long bps_peak; /* max. of above (audio may be VBR) */
    unsigned long resume_pos; /* file position to resume to */
    unsigned long reserved[9]; /* reserved, should be zero */

    /* video info (16 entries = 64 byte) */
    unsigned long video_format; /* one of VIDEOFORMAT_xxx */
    unsigned long video_1st_frame; /* byte position of first video frame */
    unsigned long video_duration; /* total length of video part, in ms */
    unsigned long video_payload_size; /* total amount of video data, in bytes */
    unsigned long video_bitrate; /* derived from resolution and frame time, in bps */
    unsigned long video_frametime; /* frame interval in 11.0592 MHz clocks */
    long video_preroll; /* video is how much ahead, in 11.0592 MHz clocks */
    unsigned long video_width; /* in pixels */
    unsigned long video_height; /* in pixels */
    unsigned long video_reserved[7]; /* reserved, should be zero */

    /* audio info (16 entries = 64 byte) */
    unsigned long audio_format; /* one of AUDIOFORMAT_xxx */
    unsigned long audio_1st_frame; /* byte position of first video frame */
    unsigned long audio_duration; /* total length of audio part, in ms */
    unsigned long audio_payload_size; /* total amount of audio data, in bytes */
    unsigned long audio_avg_bitrate; /* average audio bitrate, in bits per second */
    unsigned long audio_peak_bitrate; /* maximum bitrate */
    unsigned long audio_headersize; /* offset to payload in audio frames */
    long audio_min_associated; /* minimum offset to video frame, in bytes */
    long audio_max_associated; /* maximum offset to video frame, in bytes */
    unsigned long audio_reserved[7]; /* reserved, should be zero */

    /* more to come... ? */

    /* Note: padding up to 'blocksize' with zero following this header */
} tFileHeader;

typedef struct /* the little header for all audio blocks */
{
    unsigned long magic; /* AUDIO_MAGIC indicates an audio block */
    unsigned char previous_block; /* previous how many blocks backwards */
    unsigned char next_block; /* next how many blocks forward */
    short associated_video; /* offset to block with corresponding video */
    unsigned short frame_start; /* offset to first frame starting in this block */
    unsigned short frame_end; /* offset to behind last frame ending in this block */
} tAudioFrameHeader;



/****************** globals ******************/

static const struct plugin_api* rb; /* here is a global api struct pointer */
static char gPrint[32]; /* a global printf buffer, saves stack */


/* playstate */
static struct 
{
    enum 
    {
        paused,
        playing,
    } state;
    bool bAudioUnderrun;
    bool bVideoUnderrun;
    bool bHasAudio;
    bool bHasVideo;
    int nTimeOSD; /* OSD should stay for this many frames */
    bool bDirtyOSD; /* OSD needs redraw */
    bool bRefilling; /* set if refilling buffer */
    bool bSeeking;
    int nSeekAcc; /* accelleration value for seek */
    int nSeekPos; /* current file position for seek */
    bool bDiskSleep; /* disk is suspended */
#if FREQ == 12000000 /* Ondio speed kludge */
    int nFrameTimeAdjusted;
#endif
} gPlay;

/* buffer information */
static struct
{
    ssize_t bufsize;
    int granularity; /* common multiple of block and sector size */
    unsigned char* pBufStart; /* start of ring buffer */
    unsigned char* pBufEnd; /* end of ring buffer */
    unsigned char* pOSD; /* OSD memory (112 bytes for 112*8 pixels) */

    int vidcount; /* how many video blocks are known in a row */
    unsigned char* pBufFill; /* write pointer for disk, owned by main task */
    unsigned char* pReadVideo; /* video readout, maintained by timer ISR */
    unsigned char* pReadAudio; /* audio readout, maintained by demand ISR */
    bool bEOF; /* flag for end of file */
    int low_water; /* reload threshold  */
    int high_water; /* end of reload threshold */
    int spinup_safety; /* safety margin when recalculating low_water */
    int nReadChunk; /* how much data for normal buffer fill */
    int nSeekChunk; /* how much data while seeking */
} gBuf;

/* statistics */
static struct
{
    int minAudioAvail;
    int minVideoAvail;
    int nAudioUnderruns;
    int nVideoUnderruns;
    long minSpinup;
    long maxSpinup;
} gStats;

tFileHeader gFileHdr; /* file header */

/****************** implementation ******************/

/* tool function: return how much playable audio/video is left */
int Available(unsigned char* pSnapshot)
{
    if (pSnapshot <= gBuf.pBufFill)
        return gBuf.pBufFill - pSnapshot;
    else
        return gBuf.bufsize - (pSnapshot - gBuf.pBufFill);
}

/* debug function to draw buffer indicators */
void DrawBuf(void)
{
    int fill, video, audio;

    rb->memset(gBuf.pOSD, 0x10, LCD_WIDTH); /* draw line */
    gBuf.pOSD[0] = gBuf.pOSD[LCD_WIDTH-1] = 0xFE; /* ends */

    /* calculate new tick positions */
    fill = 1 + ((gBuf.pBufFill - gBuf.pBufStart) * (LCD_WIDTH-2)) / gBuf.bufsize;
    video = 1 + ((gBuf.pReadVideo - gBuf.pBufStart) * (LCD_WIDTH-2)) / gBuf.bufsize;
    audio = 1 + ((gBuf.pReadAudio - gBuf.pBufStart) * (LCD_WIDTH-2)) / gBuf.bufsize;

    gBuf.pOSD[fill] |= 0x20; /* below the line, two pixels */
    gBuf.pOSD[video] |= 0x08; /* one above */
    gBuf.pOSD[audio] |= 0x04; /* two above */

    if (gPlay.state == paused) /* we have to draw ourselves */
        rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
    else
        gPlay.bDirtyOSD = true; /* redraw it with next timer IRQ */
}


/* helper function to draw a position indicator */
void DrawPosition(int pos, int total)
{
    int w,h;
    int sec; /* estimated seconds */


    /* print the estimated position */   
    sec = pos / (gFileHdr.bps_average/8);
    if (sec < 100*60) /* fits into mm:ss format */
        rb->snprintf(gPrint, sizeof(gPrint), "%02d:%02dm", sec/60, sec%60);
    else /* a very long clip, hh:mm format */
        rb->snprintf(gPrint, sizeof(gPrint), "%02d:%02dh", sec/3600, (sec/60)%60);
    rb->lcd_puts(0, 7, gPrint);

    /* draw a slider over the rest of the line */
    rb->lcd_getstringsize(gPrint, &w, &h);
    w++;
    rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],w, LCD_HEIGHT-7, LCD_WIDTH-w,
                             7, total, 0, pos, HORIZONTAL);

    if (gPlay.state == paused) /* we have to draw ourselves */
        rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
    else /* let the display time do it */
    {
        gPlay.nTimeOSD = 70;
        gPlay.bDirtyOSD = true; /* redraw it with next timer IRQ */
    }
}


/* helper function to change the volume by a certain amount, +/- */
void ChangeVolume(int delta)
{
    int minvol = rb->sound_min(SOUND_VOLUME);
    int maxvol = rb->sound_max(SOUND_VOLUME);
    int vol = rb->global_settings->volume + delta;

    if (vol > maxvol) vol = maxvol;
    else if (vol < minvol) vol = minvol;
    if (vol != rb->global_settings->volume)
    {
        rb->sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
        rb->snprintf(gPrint, sizeof(gPrint), "Vol: %d dB", vol);
        rb->lcd_puts(0, 7, gPrint);
        if (gPlay.state == paused) /* we have to draw ourselves */
            rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
        else /* let the display time do it */
        {
            gPlay.nTimeOSD = 50; /* display it for 50 frames */
            gPlay.bDirtyOSD = true; /* let the refresh copy it to LCD */
        }
    }
}


/* helper function to change the LCD contrast by a certain amount, +/- */
void ChangeContrast(int delta)
{
    static int mycontrast = -1; /* the "permanent" value while running */
    int contrast; /* updated value */

    if (mycontrast == -1)
        mycontrast = rb->global_settings->contrast;

    contrast = mycontrast + delta;
    if (contrast > 63) contrast = 63;
    else if (contrast < 5) contrast = 5;
    if (contrast != mycontrast)
    {
        rb->lcd_set_contrast(contrast);
        mycontrast = contrast;
        rb->snprintf(gPrint, sizeof(gPrint), "Contrast: %d", contrast);
        rb->lcd_puts(0, 7, gPrint);
        if (gPlay.state == paused) /* we have to draw ourselves */
            rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
        else /* let the display time do it */
        {
            gPlay.nTimeOSD = 50; /* display it for 50 frames */
            gPlay.bDirtyOSD = true; /* let the refresh copy it to LCD */
        }
    }
}


/* sync the video to the current audio */
void SyncVideo(void)
{
    tAudioFrameHeader* pAudioBuf;

    pAudioBuf = (tAudioFrameHeader*)(gBuf.pReadAudio);
    if (pAudioBuf->magic == AUDIO_MAGIC)
    {
        gBuf.vidcount = 0; /* nothing known */
        /* sync the video position */
        gBuf.pReadVideo = gBuf.pReadAudio + 
            (long)pAudioBuf->associated_video * (long)gFileHdr.blocksize;
    
        /* handle possible wrap */
        if (gBuf.pReadVideo >= gBuf.pBufEnd)
            gBuf.pReadVideo -= gBuf.bufsize;
        else if (gBuf.pReadVideo < gBuf.pBufStart)
            gBuf.pReadVideo += gBuf.bufsize;
    }
}


/* timer interrupt handler to display a frame */
void timer4_isr(void)
{
    int available;
    tAudioFrameHeader* pAudioBuf;
    int height; /* height to display */

    /* reduce height if we have OSD on */
    height = gFileHdr.video_height/8;        
    if (gPlay.nTimeOSD > 0)
    {
        gPlay.nTimeOSD--;
        height = MIN(LCD_HEIGHT/8-1, height); /* reserve bottom line */
        if (gPlay.bDirtyOSD)
        {   /* OSD to bottom line */
            rb->lcd_blit_mono(gBuf.pOSD, 0, LCD_HEIGHT/8-1,
                LCD_WIDTH, 1, LCD_WIDTH);
            gPlay.bDirtyOSD = false;
        }
    }

    rb->lcd_blit_mono(gBuf.pReadVideo, 0, 0,
        gFileHdr.video_width, height, gFileHdr.video_width);

    available = Available(gBuf.pReadVideo);

    /* loop to skip audio frame(s) */
    while(1)
    {
        /* just for the statistics */
        if (!gBuf.bEOF && available < gStats.minVideoAvail)
            gStats.minVideoAvail = available;

        if (available <= (int)gFileHdr.blocksize)
        {   /* no data for next frame */

            if (gBuf.bEOF && (gFileHdr.flags & FLAG_LOOP))
            {   /* loop now, assuming the looped clip fits in memory */
                gBuf.pReadVideo = gBuf.pBufStart + gFileHdr.video_1st_frame;
                /* FixMe: pReadVideo is incremented below */
            }
            else
            {
                gPlay.bVideoUnderrun = true;
                rb->timer_unregister(); /* disable ourselves */
                return; /* no data available */
            }
        }
        else /* normal advance for next time */
        {
            gBuf.pReadVideo += gFileHdr.blocksize;
            if (gBuf.pReadVideo >= gBuf.pBufEnd)
                gBuf.pReadVideo -= gBuf.bufsize; /* wraparound */
            available -= gFileHdr.blocksize;
        }

        if (!gPlay.bHasAudio)
            break; /* no need to skip any audio */
        
        if (gBuf.vidcount)
        {   
            /* we know the next is a video frame */
            gBuf.vidcount--;
            break; /* exit the loop */
        }
        
        pAudioBuf = (tAudioFrameHeader*)(gBuf.pReadVideo);
        if (pAudioBuf->magic == AUDIO_MAGIC)
        {   /* we ran into audio, can happen after seek */
            gBuf.vidcount = pAudioBuf->next_block;
            if (gBuf.vidcount)
                gBuf.vidcount--; /* minus the audio block */
        }
    } /* while */
}


/* ISR function to get more mp3 data */
void GetMoreMp3(unsigned char** start, size_t* size)
{
    int available;
    int advance;

    tAudioFrameHeader* pAudioBuf = (tAudioFrameHeader*)(gBuf.pReadAudio);

    advance = pAudioBuf->next_block * gFileHdr.blocksize;

    available = Available(gBuf.pReadAudio);

    /* just for the statistics */
    if (!gBuf.bEOF && available < gStats.minAudioAvail)
        gStats.minAudioAvail = available;
    
    if (available < advance + (int)gFileHdr.blocksize || advance == 0)
    {
        gPlay.bAudioUnderrun = true;
        return; /* no data available */
    }

    gBuf.pReadAudio += advance;
    if (gBuf.pReadAudio >= gBuf.pBufEnd)
        gBuf.pReadAudio -= gBuf.bufsize; /* wraparound */

    *start = gBuf.pReadAudio + gFileHdr.audio_headersize;
    *size = gFileHdr.blocksize - gFileHdr.audio_headersize;
}


int WaitForButton(void)
{
    int button;
    
    do
    {
        button = rb->button_get(true);
        rb->default_event_handler(button);
    } while ((button & BUTTON_REL) && button != SYS_USB_CONNECTED);
    
    return button;
}


bool WantResume(int fd)
{
    int button;

    rb->lcd_puts(0, 0, "Resume to this");
    rb->lcd_puts(0, 1, "last position?");
    rb->lcd_puts(0, 2, "PLAY = yes");
    rb->lcd_puts(0, 3, "Any Other = no");
    rb->lcd_puts(0, 4, " (plays from start)");
    DrawPosition(gFileHdr.resume_pos, rb->filesize(fd));
    rb->lcd_update();

    button = WaitForButton();
    return (button == VIDEO_RESUME);
}


int SeekTo(int fd, int nPos)
{
    int read_now, got_now;

    if (gPlay.bHasAudio)
        rb->mp3_play_stop(); /* stop audio ISR */
    if (gPlay.bHasVideo)
        rb->timer_unregister(); /* stop the timer */

    rb->lseek(fd, nPos, SEEK_SET);

    gBuf.pBufFill = gBuf.pBufStart; /* all empty */
    gBuf.pReadVideo = gBuf.pReadAudio = gBuf.pBufStart;

    read_now = gBuf.low_water - 1; /* less than low water, so loading will continue */
    read_now -= read_now % gBuf.granularity; /* round down to granularity */
    got_now = rb->read(fd, gBuf.pBufFill, read_now);
    gBuf.bEOF = (read_now != got_now);
    gBuf.pBufFill += got_now;

    if (nPos == 0)
    {   /* we seeked to the start */
        if (gPlay.bHasVideo)
            gBuf.pReadVideo += gFileHdr.video_1st_frame;

        if (gPlay.bHasAudio)
            gBuf.pReadAudio += gFileHdr.audio_1st_frame;
    }
    else
    {   /* we have to search for the positions */
        if (gPlay.bHasAudio) /* prepare audio playback, if contained */
        {
            /* search for audio frame */
            while (((tAudioFrameHeader*)(gBuf.pReadAudio))->magic != AUDIO_MAGIC)
                gBuf.pReadAudio += gFileHdr.blocksize;
            
            if (gPlay.bHasVideo)
                SyncVideo(); /* pick the right video for that */
        }
    }

    /* synchronous start */
    gPlay.state = playing;
    if (gPlay.bHasAudio)
    {
        gPlay.bAudioUnderrun = false;
        rb->mp3_play_data(gBuf.pReadAudio + gFileHdr.audio_headersize,
                gFileHdr.blocksize - gFileHdr.audio_headersize, GetMoreMp3);
        rb->mp3_play_pause(true); /* kickoff audio */
    }
    if (gPlay.bHasVideo)
    {
        gPlay.bVideoUnderrun = false;
        /* start display interrupt */
#if FREQ == 12000000 /* Ondio speed kludge */
        rb->timer_register(1, NULL, gPlay.nFrameTimeAdjusted, 1,
                           timer4_isr IF_COP(, CPU));
#else
        rb->timer_register(1, NULL, gFileHdr.video_frametime, 1,
                           timer4_isr IF_COP(, CPU));
#endif
    }

    return 0;
}

/* called from default_event_handler_ex() or at end of playback */
void Cleanup(void *fd)
{
    rb->close(*(int*)fd); /* close the file */

    if (gPlay.bHasVideo)
        rb->timer_unregister(); /* stop video ISR, now I can use the display again */

    if (gPlay.bHasAudio)
        rb->mp3_play_stop(); /* stop audio ISR */

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

    /* restore normal contrast */
    rb->lcd_set_contrast(rb->global_settings->contrast);
}

/* returns >0 if continue, =0 to stop, <0 to abort (USB) */
int PlayTick(int fd)
{
    int button;
    static int lastbutton = 0;
    int avail_audio = -1, avail_video = -1;
    int retval = 1;
    int filepos;

    /* check buffer level */
    
    if (gPlay.bHasAudio)
        avail_audio = Available(gBuf.pReadAudio);
    if (gPlay.bHasVideo)
        avail_video = Available(gBuf.pReadVideo);

    if ((gPlay.bHasAudio && avail_audio < gBuf.low_water)
     || (gPlay.bHasVideo && avail_video < gBuf.low_water))
    {
        gPlay.bRefilling = true; /* go to refill mode */
    }

    if ((!gPlay.bHasAudio || gPlay.bAudioUnderrun)
     && (!gPlay.bHasVideo || gPlay.bVideoUnderrun)
     && gBuf.bEOF)
    {
        if (gFileHdr.resume_pos)
        {   /* we played til the end, clear resume position */
            gFileHdr.resume_pos = 0;
            rb->lseek(fd, 0, SEEK_SET); /* save resume position */
            rb->write(fd, &gFileHdr, sizeof(gFileHdr));
        }
        Cleanup(&fd);
        return 0; /* all expired */
    }

    if (!gPlay.bRefilling || gBuf.bEOF)
    {   /* nothing to do */
        button = rb->button_get_w_tmo(HZ/10);
    }
    else
    {   /* refill buffer */
        int read_now, got_now;
        int buf_free;
        long spinup; /* measure the spinup time */
        
        /* how much can we reload, don't fill completely, would appear empty */
        buf_free = gBuf.bufsize - MAX(avail_audio, avail_video) - gBuf.high_water;
        if (buf_free < 0)
            buf_free = 0; /* just for safety */
        buf_free -= buf_free % gBuf.granularity; /* round down to granularity */

        /* in one piece max. up to buffer end (wrap after that) */
        read_now = MIN(buf_free, gBuf.pBufEnd - gBuf.pBufFill);

        /* load piecewise, to stay responsive */
        read_now = MIN(read_now, gBuf.nReadChunk);

        if (read_now == buf_free)
            gPlay.bRefilling = false; /* last piece requested */

        spinup = *rb->current_tick; /* in case this is interesting below */
        
        got_now = rb->read(fd, gBuf.pBufFill, read_now);
        if (got_now != read_now || read_now == 0)
        {
            gBuf.bEOF = true;
            gPlay.bRefilling = false;
        }

        if (gPlay.bDiskSleep) /* statistics about the spinup time */
        {
            spinup = *rb->current_tick - spinup;
            gPlay.bDiskSleep = false;
            if (spinup > gStats.maxSpinup)
                gStats.maxSpinup = spinup;
            if (spinup < gStats.minSpinup)
                gStats.minSpinup = spinup;

            /* recalculate the low water mark from real measurements */
            gBuf.low_water = (gStats.maxSpinup + gBuf.spinup_safety) 
                             * gFileHdr.bps_peak / 8 / HZ;
        }

        if (!gPlay.bRefilling
#ifndef HAVE_FLASH_STORAGE
            && rb->global_settings->disk_spindown < 20  /* condition for test only */
#endif
            )
        {
            rb->ata_sleep(); /* no point in leaving the disk run til timeout */
            gPlay.bDiskSleep = true;
        }

        gBuf.pBufFill += got_now;
        if (gBuf.pBufFill >= gBuf.pBufEnd)
            gBuf.pBufFill = gBuf.pBufStart; /* wrap */

        rb->yield(); /* have mercy with the other threads */
        button = rb->button_get(false);
    }

    /* check keypresses */

    if (button != BUTTON_NONE)
    {
        filepos = rb->lseek(fd, 0, SEEK_CUR);

        if (gPlay.bHasVideo) /* video position is more accurate */
            filepos -= Available(gBuf.pReadVideo); /* take video position */
        else
            filepos -= Available(gBuf.pReadAudio); /* else audio */
            
        switch (button) {   /* set exit conditions */
        case BUTTON_OFF:
            if (gFileHdr.magic == HEADER_MAGIC /* only if file has header */
                && !(gFileHdr.flags & FLAG_LOOP)) /* not for stills */
            {
                gFileHdr.resume_pos = filepos;
                rb->lseek(fd, 0, SEEK_SET); /* save resume position */
                rb->write(fd, &gFileHdr, sizeof(gFileHdr));
            }
            Cleanup(&fd);
            retval = 0; /* signal "stopped" to caller */
            break;
        case VIDEO_STOP_SEEK:
#ifdef VIDEO_STOP_SEEK_PRE
            if (lastbutton != VIDEO_STOP_SEEK_PRE)
                break;
#endif
            if (gPlay.bSeeking)
            {
                gPlay.bSeeking = false;
                gPlay.state = playing;
                SeekTo(fd, gPlay.nSeekPos);
            }
            else if (gPlay.state == playing)
            {
                gPlay.state = paused;
                if (gPlay.bHasAudio)
                    rb->mp3_play_pause(false); /* pause audio */
                if (gPlay.bHasVideo)
                    rb->timer_unregister(); /* stop the timer */
            }
            else if (gPlay.state == paused)
            {
                gPlay.state = playing;
                if (gPlay.bHasAudio)
                {
                    if (gPlay.bHasVideo)
                        SyncVideo();
                    rb->mp3_play_pause(true); /* play audio */
                }
                if (gPlay.bHasVideo)
                {   /* start the video */
#if FREQ == 12000000 /* Ondio speed kludge */
                    rb->timer_register(1, NULL,
                        gPlay.nFrameTimeAdjusted, 1, timer4_isr);
#else
                    rb->timer_register(1, NULL,
                        gFileHdr.video_frametime, 1, timer4_isr);
#endif
                }
            }
            break;
        case BUTTON_UP:
        case BUTTON_UP | BUTTON_REPEAT:
            if (gPlay.bHasAudio)
                ChangeVolume(1);
            break;
        case BUTTON_DOWN:
        case BUTTON_DOWN | BUTTON_REPEAT:
            if (gPlay.bHasAudio)
                ChangeVolume(-1);
            break;
        case BUTTON_LEFT:
        case BUTTON_LEFT | BUTTON_REPEAT:
            if (!gPlay.bSeeking) /* prepare seek */
            {
                gPlay.nSeekPos = filepos;
                gPlay.bSeeking = true;
                gPlay.nSeekAcc = 0;
            }
            else if (gPlay.nSeekAcc > 0) /* other direction, stop sliding */
                gPlay.nSeekAcc = 0;
            else
                gPlay.nSeekAcc--;
            break;
        case BUTTON_RIGHT:
        case BUTTON_RIGHT | BUTTON_REPEAT:
            if (!gPlay.bSeeking) /* prepare seek */
            {
                gPlay.nSeekPos = filepos;
                gPlay.bSeeking = true;
                gPlay.nSeekAcc = 0;
            }
            else if (gPlay.nSeekAcc < 0) /* other direction, stop sliding */
                gPlay.nSeekAcc = 0;
            else
                gPlay.nSeekAcc++;
            break;
#ifdef VIDEO_DEBUG
        case VIDEO_DEBUG: /* debug key */
        case VIDEO_DEBUG | BUTTON_REPEAT:
            DrawBuf(); /* show buffer status */
            gPlay.nTimeOSD = 30;
            gPlay.bDirtyOSD = true;
            break;
#endif
        case VIDEO_CONTRAST_DOWN: /* contrast down */
        case VIDEO_CONTRAST_DOWN | BUTTON_REPEAT:
            if (gPlay.bHasVideo)
                ChangeContrast(-1);
            break;
        case VIDEO_CONTRAST_UP: /* contrast up */
        case VIDEO_CONTRAST_UP | BUTTON_REPEAT:
            if (gPlay.bHasVideo)
                ChangeContrast(1);
            break;
        default:
            if (rb->default_event_handler_ex(button, Cleanup, &fd)
                == SYS_USB_CONNECTED)
                retval = -1; /* signal "aborted" to caller */
            break;
        }

        lastbutton = button;
    } /*  if (button != BUTTON_NONE) */

    
    /* handle seeking */
    
    if (gPlay.bSeeking) /* seeking? */
    {
        if (gPlay.nSeekAcc < -MAX_ACC)
            gPlay.nSeekAcc = -MAX_ACC;
        else if (gPlay.nSeekAcc > MAX_ACC)
            gPlay.nSeekAcc = MAX_ACC;
        
        gPlay.nSeekPos += gPlay.nSeekAcc * gBuf.nSeekChunk;
        if (gPlay.nSeekPos < 0)
            gPlay.nSeekPos = 0;
        if (gPlay.nSeekPos > rb->filesize(fd) - gBuf.granularity)
        {
            gPlay.nSeekPos = rb->filesize(fd);
            gPlay.nSeekPos -= gPlay.nSeekPos % gBuf.granularity;
        }
        DrawPosition(gPlay.nSeekPos, rb->filesize(fd));
    }


    /* check + recover underruns */
    
    if ((gPlay.bAudioUnderrun || gPlay.bVideoUnderrun) && !gBuf.bEOF)
    {
        gBuf.spinup_safety += HZ/2; /* add extra spinup time for the future */
        filepos = rb->lseek(fd, 0, SEEK_CUR);

        if (gPlay.bHasVideo && gPlay.bVideoUnderrun)
        {
            gStats.nVideoUnderruns++;
            filepos -= Available(gBuf.pReadVideo); /* take video position */
            SeekTo(fd, filepos);
        }
        else if (gPlay.bHasAudio && gPlay.bAudioUnderrun)
        {
            gStats.nAudioUnderruns++;
            filepos -= Available(gBuf.pReadAudio); /* else audio */
            SeekTo(fd, filepos);
        }
    }

    return retval;
}


int main(char* filename)
{
    int file_size;
    int fd; /* file descriptor handle */
    int read_now, got_now;
    int button = 0;
    int retval;

    /* try to open the file */
    fd = rb->open(filename, O_RDWR);
    if (fd < 0)
        return PLUGIN_ERROR;
    file_size =  rb->filesize(fd);
    
    /* reset pitch value to ensure synchronous playback */
    rb->sound_set_pitch(1000);

    /* init statistics */
    rb->memset(&gStats, 0, sizeof(gStats));
    gStats.minAudioAvail = gStats.minVideoAvail = INT_MAX;
    gStats.minSpinup = INT_MAX;

    /* init playback state */
    rb->memset(&gPlay, 0, sizeof(gPlay));

    /* init buffer */
    rb->memset(&gBuf, 0, sizeof(gBuf));
    gBuf.pOSD = rb->lcd_framebuffer + LCD_WIDTH*7; /* last screen line */
    gBuf.pBufStart = rb->plugin_get_audio_buffer((size_t *)&gBuf.bufsize);
    /*gBuf.bufsize = 1700*1024; // test, like 2MB version!!!! */
    gBuf.pBufFill = gBuf.pBufStart; /* all empty */

    /* load file header */
    read_now = sizeof(gFileHdr);
    got_now = rb->read(fd, &gFileHdr, read_now);
    rb->lseek(fd, 0, SEEK_SET); /* rewind to restart sector-aligned */
    if (got_now != read_now)
    {
        rb->close(fd);
        return (PLUGIN_ERROR);
    }

    /* check header */
    if (gFileHdr.magic != HEADER_MAGIC)
    {   /* old file, use default info */
        rb->memset(&gFileHdr, 0, sizeof(gFileHdr));
        gFileHdr.blocksize = SCREENSIZE;
        if (file_size < SCREENSIZE * FPS) /* less than a second */
            gFileHdr.flags |= FLAG_LOOP;
        gFileHdr.video_format = VIDEOFORMAT_RAW;
        gFileHdr.video_width = LCD_WIDTH;
        gFileHdr.video_height = LCD_HEIGHT;
        gFileHdr.video_frametime = 11059200 / FPS;
        gFileHdr.bps_peak = gFileHdr.bps_average = LCD_WIDTH * LCD_HEIGHT * FPS;
    }

#if FREQ == 12000000 /* Ondio speed kludge, 625 / 576 == 12000000 / 11059200 */
    gPlay.nFrameTimeAdjusted = (gFileHdr.video_frametime * 625) / 576;
#endif

    /* continue buffer init: align the end, calc low water, read sizes */
    gBuf.granularity = gFileHdr.blocksize;
    while (gBuf.granularity % 512) /* common multiple of sector size */
        gBuf.granularity *= 2;
    gBuf.bufsize -= gBuf.bufsize % gBuf.granularity; /* round down */
    gBuf.pBufEnd = gBuf.pBufStart + gBuf.bufsize;
    gBuf.low_water = SPINUP_INIT * gFileHdr.bps_peak / 8000;
    gBuf.spinup_safety = SPINUP_SAFETY * HZ / 1000; /* in time ticks */
    if (gFileHdr.audio_min_associated < 0)
        gBuf.high_water = 0 - gFileHdr.audio_min_associated;
    else
        gBuf.high_water = 1; /* never fill buffer completely, would appear empty */
    gBuf.nReadChunk = (CHUNK + gBuf.granularity - 1); /* round up */
    gBuf.nReadChunk -= gBuf.nReadChunk % gBuf.granularity;/* and align */
    gBuf.nSeekChunk = rb->filesize(fd) / FF_TICKS;
    gBuf.nSeekChunk += gBuf.granularity - 1; /* round up */
    gBuf.nSeekChunk -= gBuf.nSeekChunk % gBuf.granularity; /* and align */

    /* prepare video playback, if contained */
    if (gFileHdr.video_format == VIDEOFORMAT_RAW)
    {
        gPlay.bHasVideo = true;
        /* Turn off backlight timeout */
        backlight_force_on(rb); /* backlight control in lib/helper.c */
    }

    /* prepare audio playback, if contained */
    if (gFileHdr.audio_format == AUDIOFORMAT_MP3_BITSWAPPED)
    {
        gPlay.bHasAudio = true;
    }

    /* start playback by seeking to zero or resume position */
    if (gFileHdr.resume_pos && WantResume(fd)) /* ask the user */
        SeekTo(fd, gFileHdr.resume_pos);
    else
        SeekTo(fd, 0);

    /* all that's left to do is keep the buffer full */
    do /* the main loop */
    {
        retval = PlayTick(fd);
    } while (retval > 0);

    if (retval < 0) /* aborted? */
    {
        return PLUGIN_USB_CONNECTED;
    }

#ifndef DEBUG /* for release compilations, only display the stats in case of error */
    if (gStats.nAudioUnderruns || gStats.nVideoUnderruns)
#endif
    {
        /* display statistics */
        rb->lcd_clear_display();
        rb->snprintf(gPrint, sizeof(gPrint), "%d Audio Underruns", gStats.nAudioUnderruns);
        rb->lcd_puts(0, 0, gPrint);
        rb->snprintf(gPrint, sizeof(gPrint), "%d Video Underruns", gStats.nVideoUnderruns);
        rb->lcd_puts(0, 1, gPrint);
        rb->snprintf(gPrint, sizeof(gPrint), "%d MinAudio bytes", gStats.minAudioAvail);
        rb->lcd_puts(0, 2, gPrint);
        rb->snprintf(gPrint, sizeof(gPrint), "%d MinVideo bytes", gStats.minVideoAvail);
        rb->lcd_puts(0, 3, gPrint);
        rb->snprintf(gPrint, sizeof(gPrint), "MinSpinup %ld.%02ld", gStats.minSpinup/HZ, gStats.minSpinup%HZ);
        rb->lcd_puts(0, 4, gPrint);
        rb->snprintf(gPrint, sizeof(gPrint), "MaxSpinup %ld.%02ld", gStats.maxSpinup/HZ, gStats.maxSpinup%HZ);
        rb->lcd_puts(0, 5, gPrint);
        rb->snprintf(gPrint, sizeof(gPrint), "LowWater: %d", gBuf.low_water);
        rb->lcd_puts(0, 6, gPrint);
        rb->snprintf(gPrint, sizeof(gPrint), "HighWater: %d", gBuf.high_water);
        rb->lcd_puts(0, 7, gPrint);
 
        rb->lcd_update();
        button = WaitForButton();
    }
    return (button == SYS_USB_CONNECTED) ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}


/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    rb = api; /* copy to global api pointer */
    
    if (parameter == NULL)
    {
        rb->splash(HZ*2, "Play .rvf file!");
        return PLUGIN_ERROR;
    }

    /* now go ahead and have fun! */
    return main((char*) parameter);
}

#endif /* #ifdef HAVE_LCD_BITMAP */
#endif /* #ifndef SIMULATOR */

