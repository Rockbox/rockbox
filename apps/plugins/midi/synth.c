/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Stepan Moskovchenko
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

extern struct plugin_api * rb;

struct Event * getEvent(struct Track * tr, int evNum)
{
    return tr->dataBlock + (evNum*sizeof(struct Event));
}

void readTextBlock(int file, char * buf)
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



/* Filename is the name of the config file              */
/* The MIDI file should have been loaded at this point  */
int initSynth(struct MIDIfile * mf, char * filename, char * drumConfig)
{
    char patchUsed[128];
    char drumUsed[128];
    int a=0;
    for(a=0; a<MAX_VOICES; a++)
    {
        voices[a].cp=0;
        voices[a].vol=0;
        voices[a].ch=0;
        voices[a].isUsed=0;
        voices[a].note=0;
    }

    for(a=0; a<16; a++)
    {
        chVol[a]=100;            /* Default, not quite full blast.. */
            chPanLeft[a]=64;     /* Center                          */
        chPanRight[a]=64;        /* Center                          */
        chPat[a]=0;              /* Ac Gr Piano                     */
        chPW[a]=256;              /* .. not .. bent ?                */
    }
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
    for(a=0; a<mf->numTracks; a++)
    {
        unsigned int ts=0;

        if(mf->tracks[a] == NULL)
        {
            printf("\nNULL TRACK !!!");
            rb->splash(HZ*2, true, "Null Track in loader.");
            return -1;
        }

        for(ts=0; ts<mf->tracks[a]->numEvents; ts++)
        {

            if((getEvent(mf->tracks[a], ts)->status) == (MIDI_NOTE_ON+9))
                drumUsed[getEvent(mf->tracks[a], ts)->d1]=1;

            if( (getEvent(mf->tracks[a], ts)->status & 0xF0) == MIDI_PRGM)
            {
                if(patchUsed[getEvent(mf->tracks[a], ts)->d1]==0)
                    printf("\nI need to load patch %d.", getEvent(mf->tracks[a], ts)->d1);
                patchUsed[getEvent(mf->tracks[a], ts)->d1]=1;
            }
        }
    }

    int file = rb->open(filename, O_RDONLY);
    if(file == -1)
    {
        rb->splash(HZ*2, true, "Bad patch config.\nDid you install the patchset?");
        return -1;
    }

    char name[40];
    char fn[40];

    /* Scan our config file and load the right patches as needed    */
    int c = 0;
    rb->snprintf(name, 40, "");
    for(a=0; a<128; a++)
    {
        while(readChar(file)!=' ' && !eof(file));
        readTextBlock(file, name);

        rb->snprintf(fn, 40, "/.rockbox/patchset/%s.pat", name);
        printf("\nLOADING: <%s> ", fn);

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
    if(file == -1)
    {
        rb->splash(HZ*2, true, "Bad drum config.\nDid you install the patchset?");
        return -1;
    }

    /* Scan our config file and load the drum data  */
    int idx=0;
    char number[30];
    while(!eof(file))
    {
        readTextBlock(file, number);
        readTextBlock(file, name);
        rb->snprintf(fn, 40, "/.rockbox/patchset/%s.pat", name);

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



int currentVoice IDATA_ATTR;
struct SynthObject * so IDATA_ATTR;
struct GWaveform * wf IDATA_ATTR;
int s IDATA_ATTR;
short s1 IDATA_ATTR;
short s2 IDATA_ATTR;
short sample IDATA_ATTR;    /* For synthSample */
unsigned int cpShifted IDATA_ATTR;

unsigned char b1 IDATA_ATTR;
unsigned char b2 IDATA_ATTR;


inline int getSample(int s)
{
    /* Sign conversion moved to guspat.c */
    /* 8bit conversion NOT YET IMPLEMENTED in guspat.c */
    return ((short *) wf->data)[s];
}




inline void setPoint(struct SynthObject * so, int pt)
{
    if(so->ch==9) /* Drums, no ADSR */
    {
        so->curOffset = 1<<27;
        so->curRate = 1;
        return;
    }

    if(so->wf==NULL)
    {
        printf("\nCrap... null waveform...");
        exit(1);
    }
    if(so->wf->envRate==NULL)
    {
        printf("\nWaveform has no envelope set");
        exit(1);
    }

    so->curPoint = pt;

    int r=0;
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
     */
    so->curRate = so->curRate << 1;


    so->targetOffset = so->wf->envOffset[pt]<<(20);
    if(pt==0)
        so->curOffset = 0;
}


inline void stopVoice(struct SynthObject * so)
{
    if(so->state == STATE_RAMPDOWN)
        return;
    so->state = STATE_RAMPDOWN;
    so->decay = 255;

}


inline signed short int synthVoice(void)
{
    so = &voices[currentVoice];
    wf = so->wf;


    if(so->state != STATE_RAMPDOWN)
    {
        so->cp += so->delta;
    }

    cpShifted = so->cp >> 10;   //Was 10

    if( (cpShifted > (wf->numSamples) && (so->state != STATE_RAMPDOWN)))
    {
        stopVoice(so);
    }

        s2 = getSample((cpShifted)+1);

        /* LOOP_REVERSE|LOOP_PINGPONG  = 24  */
    if((wf->mode & (24)) && so->loopState == STATE_LOOPING && (cpShifted <= (wf->startLoop)))
    {
        if(wf->mode & LOOP_REVERSE)
        {
            so->cp = (wf->endLoop)<<10; //Was 10
            cpShifted = wf->endLoop;
            s2=getSample((cpShifted));
            } else
            {
                so->delta = -so->delta;
            so->loopDir = LOOPDIR_FORWARD;
        }
    }

    if((wf->mode & 28) && (cpShifted >= wf->endLoop))
    {
        so->loopState = STATE_LOOPING;
        if((wf->mode & (24)) == 0)
        {
            so->cp = (wf->startLoop)<<10; //Was 10
            cpShifted = wf->startLoop;
            s2=getSample((cpShifted));
        } else
        {
            so->delta = -so->delta;
            so->loopDir = LOOPDIR_REVERSE;
        }
    }

    /* Better, working, linear interpolation    */
    s1=getSample((cpShifted));              //\|/ Was 1023)) >> 10
    s = s1 + ((signed)((s2 - s1) * (so->cp & 1023))>>10);   //Was 10


/* ADSR COMMENT WOULD GO FROM HERE.........*/

    if(so->curRate == 0)
        stopVoice(so);


    if(so->ch != 9) /* Stupid ADSR code... and don't do ADSR for drums */
    {
        if(so->curOffset < so->targetOffset)
        {
            so->curOffset += (so->curRate);
            if(so -> curOffset > so->targetOffset && so->curPoint != 2)
            {
                if(so->curPoint != 5)
                    setPoint(so, so->curPoint+1);
                else
                    stopVoice(so);
            }
        } else
        {
            so->curOffset -= (so->curRate);
            if(so -> curOffset < so->targetOffset && so->curPoint != 2)
            {

                if(so->curPoint != 5)
                    setPoint(so, so->curPoint+1);
                else
                    stopVoice(so);

            }
        }
    }

    if(so->curOffset < 0)
        so->isUsed=0;  /* This is OK because offset faded it out already */


    s = (s * (so->curOffset >> 22) >> 8);

/* ............. TO HERE */


    if(so->state == STATE_RAMPDOWN)
    {
        so->decay--;
        if(so->decay == 0)
            so->isUsed=0;
        s = (s * so->decay) >> 8;
    }

    return s*((signed short int)so->vol*(signed short int)chVol[so->ch])>>14;
}


inline void synthSample(int * mixL, int * mixR)
{
    *mixL = 0;
    *mixR = 0;
    for(currentVoice=0; currentVoice<MAX_VOICES; currentVoice++)
    {
        if(voices[currentVoice].isUsed==1)
        {
            sample = synthVoice();
            *mixL += (sample*chPanLeft[voices[currentVoice].ch])>>7;
            *mixR += (sample*chPanRight[voices[currentVoice].ch])>>7;
        }
    }

    /* TODO: Automatic Gain Control, anyone? */
    /* Or, should this be implemented on the DSP's output volume instead? */

    return;  /* No more ghetto lowpass filter.. linear intrpolation works well. */
}
