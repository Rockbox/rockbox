/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Peter D'Hoye
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
#include "plugin.h"

PLUGIN_HEADER

/* temp byte buffer */
uint8_t samples[10 * 1024]; /* read 10KB at the time */

static struct wav_header
{
    int8_t   chunkid[4];
    int32_t  chunksize;
    int8_t   format[4];
    int8_t   formatchunkid[4];
    int32_t  formatchunksize;
    int16_t  audioformat;
    int16_t  numchannels;
    uint32_t samplerate;
    uint32_t byterate;
    uint16_t blockalign;
    uint16_t bitspersample;
    int8_t   datachunkid[4];
    int32_t  datachunksize;
} header;

#define WAV_HEADER_FORMAT "4L44LSSLLSS4L"

struct peakstruct
{
    int lmin;
    int lmax;
    int rmin;
    int rmax;
};

/* TO DO: limit used LCD height, so the waveform isn't streched vertically? */

#define LEFTZERO    (LCD_HEIGHT / 4)
#define RIGHTZERO   (3 * (LCD_HEIGHT / 4))
#define YSCALE      ((0x8000 / (LCD_HEIGHT / 4)) + 1)

/* global vars */
static char *audiobuf;
static ssize_t audiobuflen;
static uint32_t mempeakcount = 0;
static uint32_t filepeakcount = 0;
static uint32_t fppmp = 0; /* file peaks per mem peaks */
static uint32_t zoomlevel = 1;
static uint32_t leftmargin = 0;
static uint32_t center = 0;
static uint32_t ppp = 1;

/* helper function copied from libwavpack bits.c */
void little_endian_to_native (void *data, char *format)
{
    unsigned char *cp = (unsigned char *) data;

    while (*format) {
        switch (*format) {
            case 'L':
                *(long *)cp = letoh32(*(long *)cp);
                cp += 4;
                break;

            case 'S':
                *(short *)cp = letoh16(*(short *)cp);
                cp += 2;
                break;

            default:
                if (*format >= '0' && *format <= '9')
                    cp += *format - '0';

                break;
        }
        format++;
    }
}
/* --- */

/*  read WAV file
    display some info about it
    store peak info in aufiobuf for display routine */
static int readwavpeaks(const char *filename)
{
    register uint32_t bytes_read;
    register uint32_t fppmp_count;
    register int16_t sampleval;
    register uint16_t* sampleshort = NULL;

    int file;
    uint32_t total_bytes_read = 0;
    char tstr[128];
    int hours;
    int minutes;
    int seconds;
    uint32_t peakcount = 0;
    struct peakstruct* peak = NULL;

    if(rb->strcasecmp (filename + rb->strlen (filename) - 3, "wav"))
    {
        rb->splash(HZ*2, "Only for wav files!");
        return -1;
    }

    file = rb->open(filename, O_RDONLY);

    if(file < 0)
    {
        rb->splash(HZ*2, "Could not open file!");
        return -1;
    }

    if(rb->read(file, &header, sizeof (header)) != sizeof (header))
    {
        rb->splash(HZ*2, "Could not read file!");
        rb->close (file);
        return -1;
    }

    total_bytes_read += sizeof (header);
    little_endian_to_native(&header, WAV_HEADER_FORMAT);

	if (rb->strncmp(header.chunkid, "RIFF", 4) ||
	    rb->strncmp(header.formatchunkid, "fmt ", 4) ||
	    rb->strncmp(header.datachunkid, "data", 4) ||
        (header.bitspersample != 16) ||
        header.audioformat != 1)
    {
            rb->splash(HZ*2, "Incompatible wav file!");
            rb->close (file);
            return -1;
    }

    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Viewing file:");
    rb->lcd_puts_scroll(0, 1, (unsigned char *)filename);
    rb->lcd_update();

    rb->snprintf(tstr,127, "Channels: %s",
                           header.numchannels == 1 ? "mono" : "stereo");
    rb->lcd_puts(0, 2, tstr);

    rb->snprintf(tstr,127, "Bits/sample: %d", header.bitspersample);
    rb->lcd_puts(0, 3, tstr);

    rb->snprintf(tstr,127, "Samplerate: %d Hz", (int)(header.samplerate));
    rb->lcd_puts(0, 4, tstr);

    seconds = header.datachunksize / header.byterate;
    hours = seconds / 3600;
    seconds -= hours * 3600;
    minutes = seconds / 60;
    seconds -= minutes * 60;
    rb->snprintf(tstr,127, "Length (hh:mm:ss): %02d:%02d:%02d", hours,
                                                                minutes,
                                                                seconds);
    rb->lcd_puts(0, 5, tstr);
    rb->lcd_puts(0, 6, "Searching for peaks... ");
    rb->lcd_update();

    /* calculate room for peaks */
    filepeakcount = header.datachunksize /
                    (header.numchannels * (header.bitspersample / 8));
    mempeakcount = audiobuflen / sizeof(struct peakstruct);
    fppmp = (filepeakcount / mempeakcount) + 1;
    peak = (struct peakstruct*)audiobuf;

    fppmp_count = fppmp;
    mempeakcount = 0;
    while(total_bytes_read < (header.datachunksize +
                              sizeof(struct wav_header)))
    {
        bytes_read = rb->read(file, &samples, sizeof(samples));
        total_bytes_read += bytes_read;

        if(0 == bytes_read)
        {
            rb->splash(HZ*2, "File read error!");
            rb->close (file);
            return -1;
        }
        if(((bytes_read/4)*4) != bytes_read)
        {
            rb->splashf(HZ*2, "bytes_read/*4 err: %ld",(long int)bytes_read);
            rb->close (file);
            return -1;
        }

        sampleshort = (int16_t*)samples;
        sampleval = letoh16(*sampleshort);
        peak->lmin = sampleval;
        peak->lmax = sampleval;
        sampleval = letoh16(*(sampleshort+1));
        peak->rmin = sampleval;
        peak->rmax = sampleval;
    
        while(bytes_read)
        {
            sampleval = letoh16(*sampleshort++);
            if(sampleval < peak->lmin)
                peak->lmin = sampleval;
            else if (sampleval > peak->lmax)
                peak->lmax = sampleval;

            sampleval = letoh16(*sampleshort++);
            if(sampleval < peak->rmin)
                peak->rmin = sampleval;
            else if (sampleval > peak->rmax)
                peak->rmax = sampleval;

            bytes_read -= 4;
            peakcount++;
            fppmp_count--;
            if(!fppmp_count)
            {
                peak++;
                mempeakcount++;
                fppmp_count = fppmp;
                sampleval = letoh16(*sampleshort);
                peak->lmin = sampleval;
                peak->lmax = sampleval;
                sampleval = letoh16(*(sampleshort+1));
                peak->rmin = sampleval;
                peak->rmax = sampleval;
            }
        }

        /* update progress */
        rb->snprintf(tstr,127, "Searching for peaks... %d%%",(int)
            (total_bytes_read / ((header.datachunksize +
                                 sizeof(struct wav_header)) / 100)));
        rb->lcd_puts(0, 6, tstr);
        rb->lcd_update();

        /* allow user to abort */
        if(ACTION_KBD_ABORT == rb->get_action(CONTEXT_KEYBOARD,TIMEOUT_NOBLOCK))
        {
            rb->splash(HZ*2, "ABORTED");
            rb->close (file);
            return -1;
        }
    }

    rb->lcd_puts(0, 6, "Searching for peaks... done");
    rb->lcd_update();

    rb->close (file);

    return 0;
}

int displaypeaks(void)
{
    register int x = 0;
    register int lymin = INT_MAX;
    register int lymax = INT_MIN;
    register int rymin = INT_MAX;
    register int rymax = INT_MIN;
    register unsigned int peakcount = 0;
    struct peakstruct* peak = (struct peakstruct*)audiobuf + leftmargin;

#if LCD_DEPTH > 1
    unsigned org_forecolor = rb->lcd_get_foreground();
    rb->lcd_set_foreground(LCD_LIGHTGRAY);
#endif

    if(!zoomlevel) zoomlevel = 1;
    ppp = (mempeakcount / LCD_WIDTH) / zoomlevel;  /* peaks per pixel */

    rb->lcd_clear_display();

    rb->lcd_hline(0, LCD_WIDTH-1, LEFTZERO - (0x8000 / YSCALE));
    rb->lcd_hline(0, LCD_WIDTH-1, LEFTZERO);
    rb->lcd_hline(0, LCD_WIDTH-1, LEFTZERO + (0x8000 / YSCALE));
    rb->lcd_hline(0, LCD_WIDTH-1, RIGHTZERO - (0x8000 / YSCALE));
    rb->lcd_hline(0, LCD_WIDTH-1, RIGHTZERO);
    rb->lcd_hline(0, LCD_WIDTH-1, RIGHTZERO + (0x8000 / YSCALE));

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_BLACK);
#endif

    /* draw zoombar */
    rb->lcd_hline(leftmargin / (mempeakcount / LCD_WIDTH),
                  (leftmargin / (mempeakcount / LCD_WIDTH)) +
                        (LCD_WIDTH / zoomlevel),
                  LCD_HEIGHT / 2);

    while((x < LCD_WIDTH) && (peakcount < mempeakcount))
    {
        if(peak->lmin < lymin)
            lymin = peak->lmin;
        if(peak->lmax > lymax)
            lymax = peak->lmax;
        if(peak->rmin < rymin)
            rymin = peak->rmin;
        if(peak->rmax > rymax)
            rymax = peak->rmax;
        peak++;
        if(0 == (peakcount % ppp))
        {
            /* drawing time */
            rb->lcd_vline(x, LEFTZERO - (lymax / YSCALE),
                          LEFTZERO - (lymin / YSCALE));
            rb->lcd_vline(x, RIGHTZERO - (rymax / YSCALE),
                          RIGHTZERO - (rymin / YSCALE));
            lymin = INT_MAX;
            lymax = INT_MIN;
            rymin = INT_MAX;
            rymax = INT_MIN;
            x++;
            rb->lcd_update();
        }
        peakcount++;
    }

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(org_forecolor);
#endif

    return 0;
}

void show_help(void)
{
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "WAVVIEW USAGE:");
    rb->lcd_puts(0, 2, "up/down: zoom out/in");
    rb->lcd_puts(0, 3, "left/right: pan left/right");
    rb->lcd_puts(0, 4, "select: refresh/continue");
    rb->lcd_puts(0, 5, "stop/off: quit");
    rb->lcd_update();
}

enum plugin_status plugin_start(const void *parameter)
{
    unsigned int quit = 0;
    unsigned int action = 0;
    unsigned int dodisplay = 1;
    int retval;

    if (!parameter)
        return PLUGIN_ERROR;

    audiobuf = rb->plugin_get_audio_buffer((size_t *)&audiobuflen);

    if (!audiobuf)
    {
        rb->splash(HZ*2, "unable to get audio buffer!");
        return PLUGIN_ERROR;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    retval = readwavpeaks(parameter); /* read WAV file and create peaks array */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    if(retval)
        return 0;

    /* press any key to continue */
    while(1)
    {
        retval = rb->get_action(CONTEXT_KEYBOARD,TIMEOUT_BLOCK);
        if(ACTION_KBD_ABORT == retval)
            return 0;
        else if(ACTION_KBD_SELECT == retval)
            break;
    }

    /* start with the overview */
    zoomlevel = 1;
    leftmargin = 0;

    while(!quit)
    {
        if(!center)
            center = mempeakcount / 2;
        if(zoomlevel <= 1)
        {
            zoomlevel = 1;
            leftmargin = 0;
        }
        else
        {
            if(center < (mempeakcount / (zoomlevel * 2)))
                center = mempeakcount / (zoomlevel * 2);
            if(center > (((zoomlevel * 2) - 1) * (mempeakcount /
                                                  (zoomlevel * 2))))
                center = ((zoomlevel * 2) - 1) * (mempeakcount /
                                                  (zoomlevel * 2));
            leftmargin = center - (mempeakcount / (zoomlevel * 2));
        }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
#endif
        if(dodisplay)
            displaypeaks();
        dodisplay = 1;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif

        action = rb->get_action(CONTEXT_KEYBOARD, TIMEOUT_BLOCK);
        switch(action)
        {
        case ACTION_KBD_UP:
            /* zoom out */
            if(zoomlevel > 1)
                zoomlevel /= 2;
            rb->splashf(HZ/2, "ZOOM: %dx",(int)zoomlevel);
            break;
        case ACTION_KBD_DOWN:
            if(zoomlevel < (mempeakcount / LCD_WIDTH / 2))
                zoomlevel *= 2;
            rb->splashf(HZ/2, "ZOOM: %dx",(int)zoomlevel);
            break;
        case ACTION_KBD_LEFT:
            center -= 10 * (mempeakcount / LCD_WIDTH) / zoomlevel;
            break;
        case ACTION_KBD_RIGHT:
            center += 10 * (mempeakcount / LCD_WIDTH) / zoomlevel;
            break;
        case ACTION_KBD_ABORT:
            quit = 1;
            break;
        case ACTION_KBD_SELECT:
            /* refresh */
            break;
        case ACTION_KBD_PAGE_FLIP:
            /* menu key shows help */
            show_help();
            while(1)
            {
                retval = rb->get_action(CONTEXT_KEYBOARD,TIMEOUT_BLOCK);
                if((ACTION_KBD_SELECT == retval) ||
                   (ACTION_KBD_ABORT == retval))
                   break;
            }
            break;
        default:
            /* eat it */
            dodisplay = 0;
            break;
        }
    }

    return 0;
}
