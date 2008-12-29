/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * MOD Codec for rockbox
 *
 * Written from scratch by Rainer Sinsch
 *         exclusivly for Rockbox in February 2008
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

 /**************
  * This version supports large files directly from internal memory management.
  * There is a drawback however: It may happen that a song is not completely
  * loaded when the internal rockbox-ringbuffer (approx. 28MB) is filled up
  * As a workaround make sure you don't have directories with mods larger
  * than a total of 28MB
  *************/

#include "debug.h"
#include "codeclib.h"
#include <inttypes.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


CODEC_HEADER

#define CHUNK_SIZE (1024*2)


/* This codec supports MOD Files:
 *
 */

static int32_t samples[CHUNK_SIZE] IBSS_ATTR;   /* The sample buffer */

/* Instrument Data */
struct s_instrument {
    /* Sample name / description */
    /*char description[22];*/

    /* Sample length in bytes */
    unsigned short length;

    /* Sample finetuning (-8 - +7) */
    signed char finetune;

    /* Sample volume (0 - 64) */
    signed char volume;

    /* Sample Repeat Position */
    unsigned short repeatoffset;

    /* Sample Repeat Length  */
    unsigned short repeatlength;

    /* Offset to sample data */
    unsigned int sampledataoffset;
};

/* Song Data */
struct s_song {
    /* Song name / title description */
    /*char szTitle[20];*/

    /* No. of channels in song */
    unsigned char noofchannels;

    /* No. of instruments used (either 15 or 31) */
    unsigned char noofinstruments;

    /* How many patterns are beeing played? */
    unsigned char songlength;

    /* Where to jump after the song end? */
    unsigned char songendjumpposition;

    /* Pointer to the Pattern Order Table */
    unsigned char *patternordertable;

    /* Pointer to the pattern data  */
    void *patterndata;

    /* Pointer to the sample buffer */
    signed char *sampledata;

    /* Instrument data  */
    struct s_instrument instrument[31];
};

struct s_modchannel {
    /* Current Volume */
    signed char volume;

    /* Current Offset to period in PeriodTable of notebeeing played
       (can be temporarily negative) */
    short periodtableoffset;

    /* Current Period beeing played */
    short period;

    /* Current effect */
    unsigned char effect;

    /* Current parameters of effect */
    unsigned char effectparameter;

    /* Current Instrument beeing played */
    unsigned char instrument;

    /* Current Vibrato Speed */
    unsigned char vibratospeed;

    /* Current Vibrato Depth */
    unsigned char vibratodepth;

    /* Current Position for Vibrato in SinTable */
    unsigned char vibratosinpos;

    /* Current Tremolo Speed */
    unsigned char tremolospeed;

    /* Current Tremolo Depth */
    unsigned char tremolodepth;

    /* Current Position for Tremolo in SinTable */
    unsigned char tremolosinpos;

    /* Current Speed of Effect "Slide Note up" */
    unsigned char slideupspeed;

    /* Current Speed of Effect "Slide Note down" */
    unsigned char slidedownspeed;

    /* Current Speed of the "Slide to Note" effect */
    unsigned char slidetonotespeed;

    /* Current Period of the "Slide to Note" effect */
    unsigned short slidetonoteperiod;
};

struct s_modplayer {
    /* Ticks per Line */
    unsigned char ticksperline;

    /* Beats per Minute */
    unsigned char bpm;

    /* Position of the Song in the Pattern Table (0-127) */
    unsigned char patterntableposition;

    /* Current Line (may be temporarily < 0) */
    signed char currentline;

    /* Current Tick */
    signed char currenttick;

    /* How many samples are required to calculate for each tick? */
    unsigned int samplespertick;

    /* Information about the channels */
    struct s_modchannel modchannel[8];

    /* The Amiga Period Table
       (+1 because we use index 0 for period 0 = no new note) */
    unsigned short periodtable[37*8+1];

    /* The sinus table [-255,255] */
    signed short sintable[0x40];

    /* Is the glissando effect enabled? */
    bool glissandoenabled;

    /* Is the Amiga Filter enabled? */
    bool amigafilterenabled;

    /* The pattern-line where the loop is carried out (set with e6 command) */
    unsigned char loopstartline;

    /* Number of times to loop */
    unsigned char looptimes;
};

struct s_channel {
    /* Panning (0 = left, 16 = right) */
    unsigned char panning;

    /* Sample frequency of the channel */
    unsigned short frequency;

    /* Position of the sample currently played */
    unsigned int samplepos;

    /* Fractual Position of the sample currently player */
    unsigned int samplefractpos;

    /* Loop Sample */
    bool loopsample;

    /* Loop Position Start */
    unsigned int loopstart;

    /* Loop Position End */
    unsigned int loopend;

    /* Is The channel beeing played? */
    bool channelactive;

    /* The Volume (0..64) */
    signed char volume;

    /* The last sampledata beeing played (required for interpolation) */
    signed short lastsampledata;
};

struct s_mixer {
    /* The channels */
    struct s_channel channel[32];
};

struct s_song modsong IDATA_ATTR;               /* The Song */
struct s_modplayer modplayer IDATA_ATTR;        /* The Module Player */
struct s_mixer mixer IDATA_ATTR;

const unsigned short mixingrate = 44100;

STATICIRAM void mixer_playsample(int channel, int instrument) ICODE_ATTR;
void mixer_playsample(int channel, int instrument)
{
    struct s_channel *p_channel = &mixer.channel[channel];
    struct s_instrument *p_instrument = &modsong.instrument[instrument];

    p_channel->channelactive = true;
    p_channel->samplepos = p_instrument->sampledataoffset;
    p_channel->samplefractpos = 0;
    p_channel->loopsample = (p_instrument->repeatlength > 2) ? true : false;
    if (p_channel->loopsample) {
        p_channel->loopstart = p_instrument->repeatoffset +
            p_instrument->sampledataoffset;
        p_channel->loopend = p_channel->loopstart +
            p_instrument->repeatlength;
    }
    else p_channel->loopend = p_instrument->length +
            p_instrument->sampledataoffset;

    /* Remember the instrument */
    modplayer.modchannel[channel].instrument = instrument;
}

static inline void mixer_stopsample(int channel)
{
    mixer.channel[channel].channelactive = false;
}

static inline void mixer_continuesample(int channel)
{
    mixer.channel[channel].channelactive = true;
}

static inline void mixer_setvolume(int channel, int volume)
{
    mixer.channel[channel].volume = volume;
}

static inline void mixer_setpanning(int channel, int panning)
{
    mixer.channel[channel].panning = panning;
}

static inline void mixer_setamigaperiod(int channel, int amigaperiod)
{
    /* Just to make sure we don't devide by zero
     * amigaperiod shouldn't 0 anyway - if it is the case
     * then something terribly went wrong */
    if (amigaperiod == 0)
        return;

    mixer.channel[channel].frequency = 3579546 / amigaperiod;
}

/* Initialize the MOD Player with default values and precalc tables */
STATICIRAM void initmodplayer(void) ICODE_ATTR;
void initmodplayer(void)
{
    unsigned int i,c;

    /* Calculate Amiga Period Values
     * Start with Period 907 (= C-1 with Finetune -8) and work upwards */
    double f = 907.0f;
    /* Index 0 stands for no note (and therefore no period) */
    modplayer.periodtable[0] = 0;
    for (i=1;i<297;i++)
    {
        modplayer.periodtable[i] = (unsigned short) f;
        f /= 1.0072464122237039; /* = pow(2.0f, 1.0f/(12.0f*8.0f)); */
    }

    /*
     * This is a more accurate but also time more consuming approach
     * to calculate the amiga period table
     * Commented out for speed purposes
    const int finetuning = 8;
    const int octaves = 3;
    for (int halftone=0;halftone<=finetuning*octaves*12+7;halftone++)
        {
            float e = pow(2.0f, halftone/(12.0f*8.0f));
            float f = 906.55f/e;
            modplayer.periodtable[halfetone+1] = (int)(f+0.5f);
        }
    */

    /* Calculate Protracker Vibrato sine table
     * The routine makes use of the Harmonical Oscillator Approach
     * for calculating sine tables
     * (see http://membres.lycos.fr/amycoders/tutorials/sintables.html)
     * The routine presented here calculates a complete sine wave
     * with 64 values in range [-255,255]
     */
    float a, b, d, dd;

    d = 0.09817475f; /* = 2*PI/64 */
    dd = d*d;
    a = 0;
    b = d;

    for (i=0;i<0x40;i++)
    {
        modplayer.sintable[i] = (int)(255*a);

        a = a+b;
        b = b-dd*a;
    }

    /* Set Default Player Values */
    modplayer.currentline = 0;
    modplayer.currenttick = 0;
    modplayer.patterntableposition = 0;
    modplayer.bpm = 125;
    modplayer.ticksperline = 6;
    modplayer.glissandoenabled = false;         /* Disable glissando */
    modplayer.amigafilterenabled = false;       /* Disable the Amiga Filter */

    /* Default Panning Values */
    int panningvalues[8] = {4,12,12,4,4,12,12,4};
    for (c=0;c<8;c++)
    {
        /* Set Default Panning */
        mixer_setpanning(c, panningvalues[c]);
        /* Reset channels in the MOD Player */
        memset(&modplayer.modchannel[c], 0, sizeof(struct s_modchannel));
        /* Don't play anything */
        mixer.channel[c].channelactive = false;
    }

}

/* Load the MOD File from memory */
STATICIRAM bool loadmod(void *modfile) ICODE_ATTR;
bool loadmod(void *modfile)
{
    int i;
    unsigned char *periodsconverted;

    /* We don't support PowerPacker 2.0 Files */
    if (memcmp((char*) modfile, "PP20", 4) == 0) return false;

    /* Get the File Format Tag */
    char *fileformattag = (char*)modfile + 1080;

    /* Find out how many channels and instruments are used */
    if (memcmp(fileformattag, "2CHN", 4) == 0)
        {modsong.noofchannels = 2; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "M.K.", 4) == 0)
        {modsong.noofchannels = 4; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "M!K!", 4) == 0)
        {modsong.noofchannels = 4; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "4CHN", 4) == 0)
        {modsong.noofchannels = 4; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "FLT4", 4) == 0)
        {modsong.noofchannels = 4; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "6CHN", 4) == 0)
        {modsong.noofchannels = 6; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "8CHN", 4) == 0)
        {modsong.noofchannels = 8; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "OKTA", 4) == 0)
        {modsong.noofchannels = 8; modsong.noofinstruments = 31;}
    else if (memcmp(fileformattag, "CD81", 4) == 0)
        {modsong.noofchannels = 8; modsong.noofinstruments = 31;}
    else {
        /* The file has no format tag, so most likely soundtracker */
        modsong.noofchannels = 4;
        modsong.noofinstruments = 15;
    }

    /* Get the Song title
     * Skipped here
     * strncpy(modsong.szTitle, (char*)pMODFile, 20); */

    /* Get the Instrument information */
    for (i=0;i<modsong.noofinstruments;i++)
    {
        struct s_instrument *instrument = &modsong.instrument[i];
        unsigned char *p = (unsigned char *)modfile + 20 + i*30;

        /*strncpy(instrument->description, (char*)p, 22); */
        p += 22;
        instrument->length = (((p[0])<<8) + p[1]) << 1; p+=2;
        instrument->finetune = *p++ & 0x0f;
        /* Treat finetuning as signed nibble */
        if (instrument->finetune > 7) instrument->finetune -= 16;
        instrument->volume = *p++;
        instrument->repeatoffset = (((p[0])<<8) + p[1]) << 1; p+= 2;
        instrument->repeatlength = (((p[0])<<8) + p[1]) << 1;
    }

    /* Get the pattern information */
    unsigned char *p = (unsigned char *)modfile + 20 +
        modsong.noofinstruments*30;
    modsong.songlength = *p++;
    modsong.songendjumpposition = *p++;
    modsong.patternordertable = p;

    /* Find out how many patterns are used within this song */
    int maxpatterns = 0;
    for (i=0;i<128;i++)
        if (modsong.patternordertable[i] > maxpatterns)
            maxpatterns = modsong.patternordertable[i];
    maxpatterns++;

    /* use 'restartposition' (historically set to 127) which is not used here
       as a marker that periods have already been converted */
       
    periodsconverted = (char*)modfile + 20 + modsong.noofinstruments*30 + 1; 

    /* Get the pattern data; ST doesn't have fileformattag, so 4 bytes less */
    modsong.patterndata = periodsconverted + 
                          (modsong.noofinstruments==15 ? 129 : 133); 
 
    /* Convert the period values in the mod file to offsets
     * in our periodtable (but only, if we haven't done this yet) */
    p = (unsigned char *) modsong.patterndata;
    if (*periodsconverted != 0xfe)
    {
        int note, note2, channel;
        for (note=0;note<maxpatterns*64;note++)
            for (channel=0;channel<modsong.noofchannels;channel++)
            {
                int period = ((p[0] & 0x0f) << 8) | p[1];
                int periodoffset = 0;

                /* Find the offset of the current period */
                for (note2 = 1; note2 < 12*3+1; note2++)
                    if (abs(modplayer.periodtable[note2*8+1]-period) < 4)
                    {
                        periodoffset = note2*8+1;
                        break;
                    }
                /* Write back the period offset */
                p[0] = (periodoffset >> 8) | (p[0] & 0xf0);
                p[1] = periodoffset & 0xff;
                p += 4;
            }
        /* Remember that we already converted the periods,
         * in case the file gets reloaded by rewinding 
         * with 0xfe (arbitary magic value > 127) */
        *periodsconverted = 0xfe;  
    }

    /* Get the samples
     * Calculation: The Samples come after the pattern data
     * We know that there are nMaxPatterns and each pattern requires
     * 4 bytes per note and per channel.
     * And of course there are always lines in each channel */
    modsong.sampledata = (signed char*) modsong.patterndata +
                          maxpatterns*4*modsong.noofchannels*64;
    int sampledataoffset = 0;
    for (i=0;i<modsong.noofinstruments;i++)
    {
        modsong.instrument[i].sampledataoffset = sampledataoffset;
        sampledataoffset += modsong.instrument[i].length;
    }

    return true;
}

/* Apply vibrato to channel */
STATICIRAM void vibrate(int channel) ICODE_ATTR;
void vibrate(int channel)
{
    struct s_modchannel *p_modchannel = &modplayer.modchannel[channel];

    /* Apply Vibrato
     * >> 7 is used in the original protracker source code */
    mixer_setamigaperiod(channel, p_modchannel->period+
        ((p_modchannel->vibratodepth *
        modplayer.sintable[p_modchannel->vibratosinpos])>>7));

    /* Foward in Sine Table */
    p_modchannel->vibratosinpos += p_modchannel->vibratospeed;
    p_modchannel->vibratosinpos &= 0x3f;
}

/* Apply tremolo to channel
 * (same as vibrato, but only apply on volume instead of pitch) */
STATICIRAM void tremolo(int channel) ICODE_ATTR;
void tremolo(int channel)
{
    struct s_modchannel *p_modchannel = &modplayer.modchannel[channel];

    /* Apply Tremolo
     * >> 6 is used in the original protracker source code */
    int volume = (p_modchannel->volume *
        modplayer.sintable[p_modchannel->tremolosinpos])>>6;
    if (volume > 64) volume = 64;
    else if (volume < 0) volume = 0;
    mixer_setvolume(channel, volume);

    /* Foward in Sine Table */
    p_modchannel->tremolosinpos += p_modchannel->tremolosinpos;
    p_modchannel->tremolosinpos &= 0x3f;
}

/* Apply Slide to Note effect to channel */
STATICIRAM void slidetonote(int channel) ICODE_ATTR;
void slidetonote(int channel)
{
    struct s_modchannel *p_modchannel = &modplayer.modchannel[channel];

    /* If there hasn't been any slide-to note set up, then return */
    if (p_modchannel->slidetonoteperiod == 0) return;

    /* Slide note up */
    if (p_modchannel->slidetonoteperiod > p_modchannel->period)
    {
        p_modchannel->period += p_modchannel->slidetonotespeed;
        if (p_modchannel->period > p_modchannel->slidetonoteperiod)
            p_modchannel->period = p_modchannel->slidetonoteperiod;
    }
    /* Slide note down */
    else if (p_modchannel->slidetonoteperiod < p_modchannel->period)
    {
        p_modchannel->period -= p_modchannel->slidetonotespeed;
        if (p_modchannel->period < p_modchannel->slidetonoteperiod)
            p_modchannel->period = p_modchannel->slidetonoteperiod;
    }
    mixer_setamigaperiod(channel, p_modchannel->period);
}

/* Apply Slide to Note effect on channel,
 * but this time with glissando enabled */
STATICIRAM void slidetonoteglissando(int channel) ICODE_ATTR;
void slidetonoteglissando(int channel)
{
    struct s_modchannel *p_modchannel = &modplayer.modchannel[channel];

    /* Slide note up */
    if (p_modchannel->slidetonoteperiod > p_modchannel->period)
    {
        p_modchannel->period =
            modplayer.periodtable[p_modchannel->periodtableoffset+=8];
        if (p_modchannel->period > p_modchannel->slidetonoteperiod)
            p_modchannel->period = p_modchannel->slidetonoteperiod;
    }
    /* Slide note down */
    else
    {
        p_modchannel->period =
            modplayer.periodtable[p_modchannel->periodtableoffset-=8];
        if (p_modchannel->period < p_modchannel->slidetonoteperiod)
            p_modchannel->period = p_modchannel->slidetonoteperiod;
    }
    mixer_setamigaperiod(channel, p_modchannel->period);
}

/* Apply Volume Slide */
STATICIRAM void volumeslide(int channel, int effectx, int effecty) ICODE_ATTR;
void volumeslide(int channel, int effectx, int effecty)
{
    struct s_modchannel *p_modchannel = &modplayer.modchannel[channel];

    /* If both X and Y Parameters are non-zero, then the y value is ignored */
    if (effectx > 0) {
        p_modchannel->volume += effectx;
        if (p_modchannel->volume > 64) p_modchannel->volume = 64;
    }
    else {
        p_modchannel->volume -= effecty;
        if (p_modchannel->volume < 0) p_modchannel->volume = 0;
    }

    mixer_setvolume(channel, p_modchannel->volume);
}

/* Play the current line (at tick 0) */
STATICIRAM void playline(int pattern, int line) ICODE_ATTR;
void playline(int pattern, int line)
{
    int c;

    /* Get pointer to the current pattern */
    unsigned char *p_line = (unsigned char*)modsong.patterndata;
    p_line += pattern*64*4*modsong.noofchannels;
    p_line += line*4*modsong.noofchannels;

    /* Only allow one Patternbreak Commando per Line */
    bool patternbreakdone = false;

    for (c=0;c<modsong.noofchannels;c++)
    {
        struct s_modchannel *p_modchannel = &modplayer.modchannel[c];
        unsigned char *p_note = p_line + c*4;
        unsigned char samplenumber = (p_note[0] & 0xf0) | (p_note[2] >> 4);
        short periodtableoffset = ((p_note[0] & 0x0f) << 8) | p_note[1];

        p_modchannel->effect = p_note[2] & 0x0f;
        p_modchannel->effectparameter = p_note[3];

        /* Remember Instrument and set Volume if new Instrument triggered */
        if (samplenumber > 0)
        {
            /* And trigger new sample, if new instrument was set */
            if (samplenumber-1 != p_modchannel->instrument)
            {
                /* Advance the new sample to the same offset
                 * the old sample was beeing played */
                int oldsampleoffset = mixer.channel[c].samplepos -
                    modsong.instrument[
                    p_modchannel->instrument].sampledataoffset;
                mixer_playsample(c, samplenumber-1);
                mixer.channel[c].samplepos += oldsampleoffset;
            }

            /* Remember last played instrument on channel */
            p_modchannel->instrument = samplenumber-1;

            /* Set Volume to standard instrument volume,
             * if not overwritten by volume effect */
            if (p_modchannel->effect != 0x0c)
            {
                p_modchannel->volume = modsong.instrument[
                    p_modchannel->instrument].volume;
                mixer_setvolume(c, p_modchannel->volume);
            }
        }
        /* Trigger new sample if note available */
        if (periodtableoffset > 0)
        {
            /* Restart instrument only when new sample triggered */
            if (samplenumber != 0)
                mixer_playsample(c, (samplenumber > 0) ?
                    samplenumber-1 : p_modchannel->instrument);

            /* Set the new amiga period
             * (but only, if there is no slide to note effect) */
            if ((p_modchannel->effect != 0x3) &&
                (p_modchannel->effect != 0x5))
            {
                /* Apply finetuning to sample */
                p_modchannel->periodtableoffset = periodtableoffset +
                    modsong.instrument[p_modchannel->instrument].finetune;
                p_modchannel->period = modplayer.periodtable[
                    p_modchannel->periodtableoffset];
                mixer_setamigaperiod(c, p_modchannel->period);
                /* When a new note is played without slide to note setup,
                 * then disable slide to note */
                modplayer.modchannel[c].slidetonoteperiod =
                    p_modchannel->period;
            }
        }
        int effectx = p_modchannel->effectparameter>>4;
        int effecty = p_modchannel->effectparameter&0x0f;

        switch (p_modchannel->effect)
        {
            /* Effect 0: Arpeggio */
            case 0x00:
                /* Set the base period on tick 0 */
                if (p_modchannel->effectparameter > 0)
                    mixer_setamigaperiod(c,
                        modplayer.periodtable[
                            p_modchannel->periodtableoffset]);
                break;
            /* Slide up (Portamento up) */
            case 0x01:
                if (p_modchannel->effectparameter > 0)
                    p_modchannel->slideupspeed =
                        p_modchannel->effectparameter;
                break;

            /* Slide down (Portamento down) */
            case 0x02:
                if (p_modchannel->effectparameter > 0)
                    p_modchannel->slidedownspeed =
                        p_modchannel->effectparameter;
                break;

            /* Slide to Note */
            case 0x03:
                if (p_modchannel->effectparameter > 0)
                    p_modchannel->slidetonotespeed =
                        p_modchannel->effectparameter;
                /* Get the slide to note directly from the pattern buffer */
                if (periodtableoffset > 0)
                    p_modchannel->slidetonoteperiod =
                        modplayer.periodtable[periodtableoffset +
                        modsong.instrument[
                        p_modchannel->instrument].finetune];
                /* If glissando is enabled apply the effect directly here */
                if (modplayer.glissandoenabled)
                    slidetonoteglissando(c);
                break;

            /* Set Vibrato */
            case 0x04:
                if (effectx > 0) p_modchannel->vibratospeed = effectx;
                if (effecty > 0) p_modchannel->vibratodepth = effecty;
                break;

            /* Effect 0x06: Slide to note */
            case 0x05:
                /* Get the slide to note directly from the pattern buffer */
                if (periodtableoffset > 0)
                    p_modchannel->slidetonoteperiod =
                        modplayer.periodtable[periodtableoffset +
                            modsong.instrument[
                            p_modchannel->instrument].finetune];
                break;

            /* Effect 0x06 is "Continue Effects" */
            /* It is not processed on tick 0 */
            case 0x06:
                break;

            /* Set Tremolo */
            case 0x07:
                if (effectx > 0) p_modchannel->tremolodepth = effectx;
                if (effecty > 0) p_modchannel->tremolospeed = effecty;
                break;

            /* Set fine panning */
            case 0x08:
                /* Internal panning goes from 0..15
                 * Scale the fine panning value to that range */
                mixer.channel[c].panning = p_modchannel->effectparameter>>4;
                break;

            /* Set Sample Offset */
            case 0x09:
                {
                    struct s_instrument *p_instrument =
                        &modsong.instrument[p_modchannel->instrument];
                    int sampleoffset = p_instrument->sampledataoffset;
                    if (sampleoffset > p_instrument->length)
                        sampleoffset = p_instrument->length;
                    /* Forward the new offset to the mixer */
                    mixer.channel[c].samplepos =
                        p_instrument->sampledataoffset +
                        (p_modchannel->effectparameter<<8);
                    mixer.channel[c].samplefractpos = 0;
                break;
                }

            /* Effect 0x0a (Volume slide) is not processed on tick 0 */

            /* Position Jump */
            case 0x0b:
                modplayer.currentline = -1;
                modplayer.patterntableposition = (effectx<<4)+effecty;
                break;

            /* Set Volume */
            case 0x0c:
                p_modchannel->volume = p_modchannel->effectparameter;
                mixer_setvolume(c, p_modchannel->volume);
                break;

            /* Pattern break */
            case 0x0d:
                modplayer.currentline = effectx*10 + effecty - 1;
                if (!patternbreakdone)
                {
                    patternbreakdone = true;
                    modplayer.patterntableposition++;
                }
                break;

            /* Extended Effects */
            case 0x0e:
                switch (effectx)
                {
                    /* Set Filter */
                    case 0x0:
                        modplayer.amigafilterenabled =
                            (effecty>0) ? false : true;
                        break;
                    /* Fineslide up */
                    case 0x1:
                        mixer_setamigaperiod(c, p_modchannel->period -=
                            effecty);
                        if (p_modchannel->period <
                            modplayer.periodtable[37*8]) p_modchannel->period = 100;
                        /* Find out the new offset in the period table */
                        if (p_modchannel->periodtableoffset < 36*8)
                            while (modplayer.periodtable[
                                p_modchannel->periodtableoffset+8] >= p_modchannel->period)
                                    p_modchannel->periodtableoffset+=8;
                        break;
                    /* Fineslide down */
                    case 0x2:
                        mixer_setamigaperiod(c,
                            p_modchannel->period += effecty);
                        if (p_modchannel->periodtableoffset > 8)
                            while (modplayer.periodtable[
                                p_modchannel->periodtableoffset-8]
                                <= p_modchannel->period)
                            p_modchannel->periodtableoffset-=8;
                        break;
                    /* Set glissando on/off */
                    case 0x3:
                        modplayer.glissandoenabled =
                            (effecty > 0) ? true:false;
                        break;
                    /* Set Vibrato waveform */
                    case 0x4:
                        /* Currently not implemented */
                        break;
                    /* Set Finetune value */
                    case 0x5:
                        /* Treat as signed nibble */
                        if (effecty > 7) effecty -= 16;

                        p_modchannel->periodtableoffset +=
                            effecty -
                                modsong.instrument[
                                p_modchannel->instrument].finetune;
                        p_modchannel->period =
                            modplayer.periodtable[
                            p_modchannel->periodtableoffset];
                        modsong.instrument[
                            p_modchannel->instrument].finetune = effecty;
                        break;
                    /* Pattern loop */
                    case 0x6:
                        if (effecty == 0)
                            modplayer.loopstartline = line-1;
                        else
                        {
                            if (modplayer.looptimes == 0)
                            {
                                modplayer.currentline =
                                    modplayer.loopstartline;
                                modplayer.looptimes = effecty;
                            }
                            else modplayer.looptimes--;
                            if (modplayer.looptimes > 0)
                                modplayer.currentline =
                                    modplayer.loopstartline;
                        }
                        break;
                    /* Set Tremolo waveform */
                    case 0x7:
                        /* Not yet implemented */
                        break;
                    /* Enhanced Effect 8 is not used */
                    case 0x8:
                        break;
                    /* Retrigger sample */
                    case 0x9:
                        /* Only processed on subsequent ticks */
                        break;
                    /* Fine volume slide up */
                    case 0xa:
                        p_modchannel->volume += effecty;
                        if (p_modchannel->volume > 64)
                            p_modchannel->volume = 64;
                        mixer_setvolume(c, p_modchannel->volume);
                        break;
                    /* Fine volume slide down */
                    case 0xb:
                        p_modchannel->volume -= effecty;
                        if (p_modchannel->volume < 0)
                            p_modchannel->volume = 0;
                        mixer_setvolume(c, p_modchannel->volume);
                        break;
                    /* Cut sample */
                    case 0xc:
                        /* Continue sample */
                        mixer_continuesample(c);
                        break;
                    /* Note delay (Usage: $ED + ticks to delay note.) */
                    case 0xd:
                        /* We stop the sample here on tick 0
                         * and restart it later in the effect */
                        if (effecty > 0)
                            mixer.channel[c].channelactive = false;
                        break;
                }
                break;

            /* Set Speed */
            case 0x0f:
                if (p_modchannel->effectparameter < 32)
                    modplayer.ticksperline = p_modchannel->effectparameter;
                else
                    modplayer.bpm = p_modchannel->effectparameter;
                break;
        }
    }
}

/* Play the current effect of the note (ticks 1..speed) */
STATICIRAM void playeffect(int currenttick) ICODE_ATTR;
void playeffect(int currenttick)
{
    int c;

    for (c=0;c<modsong.noofchannels;c++)
    {
        struct s_modchannel *p_modchannel = &modplayer.modchannel[c];

        /* If there is no note active then there are no effects to play */
        if (p_modchannel->period == 0) continue;

        unsigned char effectx = p_modchannel->effectparameter>>4;
        unsigned char effecty = p_modchannel->effectparameter&0x0f;

        switch (p_modchannel->effect)
        {
            /* Effect 0: Arpeggio */
            case 0x00:
                if (p_modchannel->effectparameter > 0)
                {
                    unsigned short newperiodtableoffset;
                    switch (currenttick % 3)
                    {
                        case 0:
                            mixer_setamigaperiod(c,
                                modplayer.periodtable[
                                    p_modchannel->periodtableoffset]);
                            break;
                        case 1:
                            newperiodtableoffset =
                                p_modchannel->periodtableoffset+(effectx<<3);
                            if (newperiodtableoffset < 37*8)
                                mixer_setamigaperiod(c,
                                    modplayer.periodtable[
                                        newperiodtableoffset]);
                            break;
                        case 2:
                            newperiodtableoffset =
                                p_modchannel->periodtableoffset+(effecty<<3);
                            if (newperiodtableoffset < 37*8)
                                mixer_setamigaperiod(c,
                                    modplayer.periodtable[
                                        newperiodtableoffset]);
                            break;
                    }
                }
                break;

            /* Effect 1: Slide Up */
            case 0x01:
                mixer_setamigaperiod(c,
                    p_modchannel->period -= p_modchannel->slideupspeed);
                /* Find out the new offset in the period table */
                if (p_modchannel->periodtableoffset <= 37*8)
                    while (modplayer.periodtable[
                        p_modchannel->periodtableoffset] >
                        p_modchannel->period)
                    {
                        p_modchannel->periodtableoffset++;
                        /* Make sure we don't go out of range */
                        if (p_modchannel->periodtableoffset > 37*8)
                        {
                            p_modchannel->periodtableoffset = 37*8;
                            break;
                        }
                    }
                break;

            /* Effect 2: Slide Down */
            case 0x02:
                mixer_setamigaperiod(c, p_modchannel->period +=
                    p_modchannel->slidedownspeed);
                /* Find out the new offset in the period table */
                if (p_modchannel->periodtableoffset > 8)
                    while (modplayer.periodtable[
                        p_modchannel->periodtableoffset] <
                        p_modchannel->period)
                    {
                        p_modchannel->periodtableoffset--;
                        /* Make sure we don't go out of range */
                        if (p_modchannel->periodtableoffset < 1)
                        {
                            p_modchannel->periodtableoffset = 1;
                            break;
                        }
                    }
                break;

            /* Effect 3: Slide to Note */
            case 0x03:
                /* Apply smooth sliding, if no glissando is enabled */
                if (modplayer.glissandoenabled == 0)
                    slidetonote(c);
                break;

            /* Effect 4: Vibrato */
            case 0x04:
                vibrate(c);
                break;

            /* Effect 5: Continue effect 3:'Slide to note',
             *           but also do Volume slide */
            case 0x05:
                slidetonote(c);
                volumeslide(c, effectx, effecty);
                break;

            /* Effect 6: Continue effect 4:'Vibrato',
             *           but also do Volume slide */
            case 0x06:
                vibrate(c);
                volumeslide(c, effectx, effecty);
                break;

            /* Effect 7: Tremolo */
            case 0x07:
                tremolo(c);
                break;

            /* Effect 8 (Set fine panning) is only processed at tick 0 */
            /* Effect 9 (Set sample offset) is only processed at tick 0 */

            /* Effect A: Volume slide */
            case 0x0a:
                volumeslide(c, effectx, effecty);
                break;

            /* Effect B (Position jump) is only processed at tick 0 */
            /* Effect C (Set Volume) is only processed at tick 0 */
            /* Effect D (Pattern Preak) is only processed at tick 0 */
            /* Effect E (Enhanced Effect) */
            case 0x0e:
                switch (effectx)
                {
                    /* Retrigger sample ($E9 + Tick to Retrig note at) */
                    case 0x9:
                        /* Don't device by zero */
                        if (effecty == 0) effecty = 1;
                        /* Apply retrig */
                        if (currenttick % effecty == 0)
                            mixer_playsample(c, p_modchannel->instrument);
                        break;
                    /* Cut note (Usage: $EC + Tick to Cut note at) */
                    case 0xc:
                        if (currenttick == effecty)
                        mixer_stopsample(c);
                        break;
                    /* Delay note (Usage: $ED + ticks to delay note) */
                    case 0xd:
                        /* If this is the correct tick,
                         * we start playing the sample now */
                        if (currenttick == effecty)
                            mixer.channel[c].channelactive = true;
                        break;

                }
                break;
            /* Effect F (Set Speed) is only processed at tick 0 */

        }
    }
}

static inline int clip(int i)
{
    if (i > 32767) return(32767);
    else if (i < -32768) return(-32768);
    else return(i);
}

STATICIRAM void synthrender(void *renderbuffer, int samplecount) ICODE_ATTR;
void synthrender(void *renderbuffer, int samplecount)
{
    /* 125bpm equals to 50Hz (= 0.02s)
     * => one tick = mixingrate/50,
     * samples passing in one tick:
     * mixingrate/(bpm/2.5) = 2.5*mixingrate/bpm */

    int *p_left = (int *) renderbuffer; /* int in rockbox */
    int *p_right = p_left+1;
    signed short s;
    int qf_distance, qf_distance2;

    int i;

    int c, left, right;

    for (i=0;i<samplecount;i++)
    {
        /* New Tick? */
        if ((modplayer.samplespertick-- <= 0) &&
            (modplayer.patterntableposition < 127))
        {
            if (modplayer.currenttick == 0)
                playline(modsong.patternordertable[
                    modplayer.patterntableposition], modplayer.currentline);
            else playeffect(modplayer.currenttick);

            modplayer.currenttick++;

            if (modplayer.currenttick >= modplayer.ticksperline)
            {
                modplayer.currentline++;
                modplayer.currenttick = 0;
                if (modplayer.currentline == 64)
                {
                    modplayer.patterntableposition++;
                    if (modplayer.patterntableposition >= modsong.songlength)
                        /* This is for Noise Tracker
                         * modplayer.patterntableposition =
                         *    modsong.songendjumpposition;
                         * More compatible approach is restart from 0 */
                        modplayer.patterntableposition=0;
                    modplayer.currentline = 0;
                }
            }

            modplayer.samplespertick = (20*mixingrate/modplayer.bpm)>>3;
        }
        /* Mix buffers from here
         * Walk through all channels */
        left=0, right=0;

        /* If song has not stopped playing */
        if (modplayer.patterntableposition < 127)
            /* Loop through all channels */
            for (c=0;c<modsong.noofchannels;c++)
            {
                /* Only mix the sample,
                 * if channel there is something played on the channel */
                if (!mixer.channel[c].channelactive) continue;

                /* Loop the sample, if requested? */
                if (mixer.channel[c].samplepos >= mixer.channel[c].loopend)
                {
                    if (mixer.channel[c].loopsample)
                        mixer.channel[c].samplepos -=
                            (mixer.channel[c].loopend-
                            mixer.channel[c].loopstart);
                    else mixer.channel[c].channelactive = false;
                }

                /* If the sample has stopped playing don't mix it */
                if (!mixer.channel[c].channelactive) continue;

                /* Get the sample */
                s = (signed short)(modsong.sampledata[
                    mixer.channel[c].samplepos]*mixer.channel[c].volume);

                /* Interpolate if the sample-frequency is lower
                 * than the mixing rate
                 * If you don't want interpolation simply skip this part */
                if (mixer.channel[c].frequency < mixingrate)
                {
                    /* Low precision linear interpolation
                     * (fast integer based) */
                    qf_distance = mixer.channel[c].samplefractpos<<16 /
                                    mixingrate;
                    qf_distance2 = (1<<16)-qf_distance;
                    s = (qf_distance*s + qf_distance2*
                        mixer.channel[c].lastsampledata)>>16;
                }

                /* Save the last played sample for interpolation purposes */
                mixer.channel[c].lastsampledata = s;

                /* Pan the sample */
                left += s*(16-mixer.channel[c].panning)>>3;
                right += s*mixer.channel[c].panning>>3;

                /* Advance sample */
                mixer.channel[c].samplefractpos += mixer.channel[c].frequency;
                while (mixer.channel[c].samplefractpos > mixingrate)
                {
                    mixer.channel[c].samplefractpos -= mixingrate;
                    mixer.channel[c].samplepos++;
                }
            }
        /* If we have more than 4 channels
         * we have to make sure that we apply clipping */
        if (modsong.noofchannels > 4) {
            *p_left = clip(left)<<13;
            *p_right = clip(right)<<13;
        }
        else {
            *p_left = left<<13;
            *p_right = right<<13;
        }
        p_left+=2;
        p_right+=2;
    }
}


enum codec_status codec_main(void)
{
    size_t n;
    unsigned char *modfile;
    int old_patterntableposition;

    int bytesdone;

    ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);


next_track:
    if (codec_init()) {
        return CODEC_ERROR;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);

    /* Load MOD file */
    /*
     * This is the save way

    size_t bytesfree;
    unsigned int filesize;

    p = modfile;
    bytesfree=sizeof(modfile);
    while ((n = ci->read_filebuf(p, bytesfree)) > 0) {
        p += n;
        bytesfree -= n;
        if (bytesfree == 0)
           return CODEC_ERROR;
    }
    filesize = p-modfile;

    if (filesize == 0)
        return CODEC_ERROR;
    */

    /* Directly use mod in buffer */
    ci->seek_buffer(0);
    modfile = ci->request_buffer(&n, ci->filesize);
    if (!modfile || n < (size_t)ci->filesize) {
        return CODEC_ERROR;
    }

    initmodplayer();
    loadmod(modfile);

    /* Make use of 44.1khz */
    ci->configure(DSP_SET_FREQUENCY, 44100);
    /* Sample depth is 28 bit host endian */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 28);
    /* Stereo output */
    ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);

    /* The main decoder loop */
    ci->set_elapsed(0);
    bytesdone = 0;
    old_patterntableposition = 0;

    while (1) {
        ci->yield();
        if (ci->stop_codec || ci->new_track)
            break;

        if (ci->seek_time) {
            /* New time is ready in ci->seek_time */
            modplayer.patterntableposition = ci->seek_time/1000;
            modplayer.currentline = 0;
            ci->seek_complete();
        }

        if(old_patterntableposition != modplayer.patterntableposition) {
          ci->set_elapsed(modplayer.patterntableposition*1000+500);
          old_patterntableposition=modplayer.patterntableposition;
        }

        synthrender(samples, CHUNK_SIZE/2);

        bytesdone += CHUNK_SIZE;

        ci->pcmbuf_insert(samples, NULL, CHUNK_SIZE/2);

    }

    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;
}
