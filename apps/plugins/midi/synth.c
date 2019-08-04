/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Stepan Moskovchenko
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
#include "guspat.h"
#include "midiutil.h"
#include "synth.h"

static void readTextBlock(int file, char * buf)
{
    char c = 0;
    do
    {
        c = readChar(file);
    } while(c == '\n' || c == ' ' || c=='\t');

    rb->lseek(file, -1, SEEK_CUR);
    int cp = 0;
    do
    {
        c = readChar(file);
        buf[cp] = c;
        cp++;
    } while (c != '\n' && c != ' ' && c != '\t' && !eof(file));
    buf[cp-1]=0;
    rb->lseek(file, -1, SEEK_CUR);
}

void resetControllers()
{
    int a=0;
    for(a=0; a<MAX_VOICES; a++)
    {
        voices[a].cp=0;
        voices[a].vol=0;
        voices[a].ch=0;
        voices[a].isUsed=false;
        voices[a].note=0;
    }

    for(a=0; a<16; a++)
    {
        chVol[a]=100;            /* Default, not quite full blast.. */
        chPan[a]=64;             /* Center                          */
        chPat[a]=0;              /* Ac Gr Piano                     */
        chPW[a]=256;             /* .. not .. bent ?                */
        chPBDepth[a]=2;          /* Default bend value is 2 */
        chPBNoteOffset[a]=0;     /* No offset */
        chPBFractBend[a]=65536; /* Center.. no bend */
        chLastCtrlMSB[a]=0;     /* Set to pitch bend depth */
        chLastCtrlLSB[a]=0;     /* Set to pitch bend depth */
    }
}

/* Filename is the name of the config file              */
/* The MIDI file should have been loaded at this point  */
int initSynth(struct MIDIfile * mf, char * filename, char * drumConfig)
{
    char patchUsed[128];
    char drumUsed[128];
    int a=0;

    resetControllers();

    for(a=0; a<128; a++)
    {
        patchSet[a]=NULL;
        drumSet[a]=NULL;
        patchUsed[a]=0;
        drumUsed[a]=0;
    }

    /*
     * Always load the piano.
     * Some files will assume its loaded without specifically
     * issuing a Patch command... then we wonder why we can't hear anything
     */
    patchUsed[0]=1;

    /* Scan the file to see what needs to be loaded */
    if(mf != NULL)
    {
        for(a=0; a<mf->numTracks; a++)
        {
            unsigned int ts=0;

            if(mf->tracks[a] == NULL)
            {
                midi_debug("NULL TRACK !!!");
                rb->splash(HZ*2, "Null Track in loader.");
                return -1;
            }

            for(ts=0; ts<mf->tracks[a]->numEvents; ts++)
            {

                if((getEvent(mf->tracks[a], ts)->status) == (MIDI_NOTE_ON+9))
                    drumUsed[getEvent(mf->tracks[a], ts)->d1]=1;

                if( (getEvent(mf->tracks[a], ts)->status & 0xF0) == MIDI_PRGM)
                    patchUsed[getEvent(mf->tracks[a], ts)->d1]=1;
            }
        }
    } else
    {
        /* Initialize the whole drum set */
        for(a=0; a<128; a++)
            drumUsed[a]=1;

    }

    int file = rb->open(filename, O_RDONLY);
    if(file < 0)
    {
        midi_debug("");
        midi_debug("No MIDI patchset found.");
        midi_debug("Please install the instruments.");
        midi_debug("See Rockbox page for more info.");

        rb->splash(HZ*2, "No Instruments");
        return -1;
    }

    char name[40];
    char fn[40];

    /* Scan our config file and load the right patches as needed    */
    int c = 0;
    name[0] = '\0';
    midi_debug("Loading instruments");
    for(a=0; a<128; a++)
    {
        while(readChar(file)!=' ' && !eof(file));
        readTextBlock(file, name);

        rb->snprintf(fn, 40, ROCKBOX_DIR "/patchset/%s.pat", name);
/*        midi_debug("\nLOADING: <%s> ", fn); */

        if(patchUsed[a]==1)
        {
            patchSet[a]=gusload(fn);

            if(patchSet[a] == NULL) /* There was an error loading it */
                return -1;
        }

        while((c != '\n'))
            c = readChar(file);
    }
    rb->close(file);

    file = rb->open(drumConfig, O_RDONLY);
    if(file < 0)
    {
        rb->splash(HZ*2, "Bad drum config. Did you install the patchset?");
        return -1;
    }

    /* Scan our config file and load the drum data  */
    int idx=0;
    char number[30];
    midi_debug("Loading drums");
    while(!eof(file))
    {
        readTextBlock(file, number);
        readTextBlock(file, name);
        rb->snprintf(fn, 40, ROCKBOX_DIR "/patchset/%s.pat", name);

        idx = rb->atoi(number);
        if(idx == 0)
            break;

        if(drumUsed[idx]==1)
        {
            drumSet[idx]=gusload(fn);
            if(drumSet[idx] == NULL)    /* Error loading patch */
                return -1;
        }

        while((c != '\n') && (c != 255) && (!eof(file)))
            c = readChar(file);
    }
    rb->close(file);
    return 0;
}

void setPoint(struct SynthObject * so, int pt) ICODE_ATTR;
void setPoint(struct SynthObject * so, int pt)
{
    if(so->ch==9) /* Drums, no ADSR */
    {
        so->curOffset = 1<<27;
        so->curRate = 1;
        return;
    }

    if(so->wf==NULL)
    {
        midi_debug("Crap... null waveform...");
        exit(1);
    }
    if(so->wf->envRate==NULL)
    {
        midi_debug("Waveform has no envelope set");
        exit(1);
    }

    so->curPoint = pt;

    int r;
    int rate = so->wf->envRate[pt];

    r=3-((rate>>6) & 0x3);      /* Some blatant Timidity code for rate conversion... */
    r*=3;
    r = (rate & 0x3f) << r;

    /*
     * Okay. This is the rate shift. Timidity defaults to 9, and sets
     * it to 10 if you use the fast decay option. Slow decay sounds better
     * on some files, except on some other files... you get chords that aren't
     * done decaying yet.. and they dont harmonize with the next chord and it
     * sounds like utter crap. Yes, even Timitidy does that. So I'm going to
     * default this to 10, and maybe later have an option to set it to 9
     * for longer decays.
     */
    so->curRate = r<<10;

    /*
     * Do this here because the patches assume a 44100 sampling rate
     * We've halved our sampling rate, ergo the ADSR code will be
     * called half the time. Ergo, double the rate to keep stuff
     * sounding right.
     *
     * Or just move the 1 up one line to optimize a tiny bit.
     */
    /* so->curRate = so->curRate << 1; */


    so->targetOffset = so->wf->envOffset[pt]<<(20);
    if(pt==0)
        so->curOffset = 0;
}

static inline void synthVoice(struct SynthObject * so, int32_t * out, unsigned int samples)
{
    struct GWaveform * wf;
    register int s1;
    register int s2;

    register unsigned int cp_temp = so->cp;

    wf = so->wf;
    const int16_t *sample_data = wf->data;

    const unsigned int pan = chPan[so->ch];
    const int volscale = so->volscale;

    const int mode_mask24 = wf->mode&24;
    const int mode_mask28 = wf->mode&28;
    const int mode_mask_looprev = wf->mode&LOOP_REVERSE;

    const unsigned int num_samples = (wf->numSamples-1) << FRACTSIZE;

    const unsigned int end_loop = wf->endLoop << FRACTSIZE;
    const unsigned int start_loop = wf->startLoop << FRACTSIZE;
    const int diff_loop = end_loop-start_loop;

    bool rampdown = (so->state == STATE_RAMPDOWN);
    const bool ch_9 = (so->ch == 9);

    while(LIKELY(samples-- > 0))
    {
        /* Is voice being ramped? */
        if(UNLIKELY(rampdown))
        {
            if(so->decay != 0)  /* Ramp has been started */
            {
                so->decay = so->decay / 2;

                if(so->decay < 10 && so->decay > -10)
                    so->isUsed = false;

                s1 = so->decay;
                s2 = s1 * pan;
                s1 = (s1 << 7) -s2;
                *(out++) += s1;
                *(out++) += s2;
                continue;
            }
        } else  /* OK to advance voice */
        {
            cp_temp += so->delta;
        }

        s2 = sample_data[(cp_temp >> FRACTSIZE)+1];

        if(LIKELY(mode_mask28))
        {
            /* LOOP_REVERSE|LOOP_PINGPONG  = 24  */
            if(UNLIKELY(mode_mask24 && so->loopState == STATE_LOOPING && (cp_temp < start_loop)))
            {
                if(UNLIKELY(mode_mask_looprev))
                {
                    cp_temp += diff_loop;
                    s2 = sample_data[cp_temp >> FRACTSIZE];
                }
                else
                {
                    so->delta = -so->delta; /* At this point cp_temp is wrong. We need to take a step */
                }
            }

            if(UNLIKELY(cp_temp >= end_loop))
            {
                so->loopState = STATE_LOOPING;
                if(UNLIKELY(!mode_mask24))
                {
                    cp_temp -= diff_loop;
                    s2 = sample_data[cp_temp >> FRACTSIZE];
                }
                else
                {
                    so->delta = -so->delta;
                }
            }
        }

        /* Have we overrun? */
        if(UNLIKELY(cp_temp >= num_samples))
        {
            cp_temp -= so->delta;
            s2 = sample_data[(cp_temp >> FRACTSIZE)+1];

            if (!rampdown) /* stop voice */
            {
                rampdown = true;
                so->decay = 0;
            }
        }

        /* Better, working, linear interpolation    */
        s1 = sample_data[cp_temp >> FRACTSIZE];

        s1 +=((signed)((s2 - s1) * (cp_temp & ((1<<FRACTSIZE)-1)))>>FRACTSIZE);

        if(UNLIKELY(so->curRate == 0))
        {
            if (!rampdown) /* stop voice */
            {
                rampdown = true;
                so->decay = 0;
            }
//          so->isUsed = false;
        }

        if(LIKELY(!ch_9 && !rampdown)) /* Stupid ADSR code... and don't do ADSR for drums */
        {
            if(UNLIKELY(so->curOffset < so->targetOffset))
            {
                so->curOffset += (so->curRate);
                if(UNLIKELY(so -> curOffset > so->targetOffset && so->curPoint != 2))
                {
                    if(UNLIKELY(so->curPoint != 5))
                    {
                        setPoint(so, so->curPoint+1);
                    }
                    else if (!rampdown) /* stop voice */
                    {
                        rampdown = true;
                        so->decay = 0;
                    }
                }
            } else
            {
                so->curOffset -= (so->curRate);
                if(UNLIKELY(so -> curOffset < so->targetOffset && so->curPoint != 2))
                {
                    if(UNLIKELY(so->curPoint != 5))
                    {
                        setPoint(so, so->curPoint+1);
                    }
                    else if (!rampdown) /* stop voice */
                    {
                        rampdown = true;
                        so->decay = 0;
                    }

                }
            }
        }

        if(UNLIKELY(so->curOffset < 0))
        {
            so->curOffset = so->targetOffset;
            if (!rampdown)
            {
                rampdown = true;
                so->decay = 0;
            }
        }

        s1 = s1 * (so->curOffset >> 22) >> 8;

        /* Scaling by channel volume and note volume is done in sequencer.c */
        /* That saves us some multiplication and pointer operations         */
        s1 = s1 * volscale >> 14;

        /* need to set ramp beginning */
        if(UNLIKELY(rampdown && so->decay == 0))
        {
            so->decay = s1;
            if(so->decay == 0)
                so->decay = 1;  /* stupid junk.. */
        }

        s2 = s1 * pan;
        s1 = (s1 << 7) - s2;
        *(out++) += s1;
        *(out++) += s2;
    }

    /* store these again */
    if (rampdown)
        so->state = STATE_RAMPDOWN;
    so->cp = cp_temp;
    return;
}

/* synth num_samples samples and write them to the */
/* buffer pointed to by buf_ptr                    */
size_t synthSamples(int32_t *buf_ptr, size_t num_samples) ICODE_ATTR;
size_t synthSamples(int32_t *buf_ptr, size_t num_samples)
{
    unsigned int i;
    struct SynthObject *voicept;
    size_t nsamples = MIN(num_samples, MAX_SAMPLES);

    rb->memset(buf_ptr, 0, nsamples * 2 * sizeof(int32_t));

    for(i=0; i < MAX_VOICES; i++)
    {
        voicept=&voices[i];
        if(voicept->isUsed)
        {
            synthVoice(voicept, buf_ptr, nsamples);
        }
    }

    /* TODO: Automatic Gain Control, anyone? */
    /* Or, should this be implemented on the DSP's output volume instead? */

    return nsamples; /* No more ghetto lowpass filter. Linear interpolation works well. */
}

