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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "midiutil.h"
#include "guspat.h"
#include "synth.h"

extern struct plugin_api * rb;
extern int playingTime IBSS_ATTR;
extern int samplesThisSecond IBSS_ATTR;

long tempo=375000;

/* From the old patch config.... each patch is scaled.
 * Should be moved into patchset.cfg
 * But everyone would need a new config file.
 *
 * While this really does need to go into the patch config,
 * I doubt anyone has made their own custom rockbox patchset
 * (if you have, please send a copy my way :) )
 */
static const unsigned char patchScale[]=
{
    125,115,115,100,100,80,115,100,100,100,100,80,100,100,100,100,
    100,100,100,100,60,100,100,100,150,155,145,100,125,86,125,85,
    161,160,133,160,135,133,100,128,150,100,100,150,100,130,100,200,
    100,100,125,125,100,100,100,100,124,110,111,100,139,113,115,115,
    125,115,95,140,100,100,105,100,90,100,80,80,100,125,100,80,
    100,100,100,250,130,100,100,100,115,100,100,120,200,100,100,80,
    130,100,100,150,100,100,100,100,100,100,200,100,100,100,100,100,
    100,100,113,100,200,100,100,100,30,100,100,100,100,100,100,100
};

/* Sets the volume scaling by channel volume and note volume */
/* This way we can do the multiplication/indexing once per   */
/* MIDI event at the most, instead of once per sample.       */
static inline void setVolScale(int a)
{
    struct SynthObject * so = &voices[a];
    int ch = so->ch;

    so->volscale = so->vol * chVol[ch]*patchScale[chPat[ch]] / 100;
    //((signed short int)so->vol*(signed short int)chVol[ch])*patchScale[chPat[ch]];
}

static inline void setVol(int ch, int vol)
{
    int a=0;
    chVol[ch]=vol;

    /* If channel volume changes, we need to recalculate the volume scale */
    /* factor for all voices active on this channel                       */
    for(a=0; a<MAX_VOICES; a++)
        if(voices[a].ch == ch)
            setVolScale(a);
}

static inline void setPatch(int ch, int pat)
{
    chPat[ch]=pat;
}



/*
    This is the new pitch bend table. There are 512 entries.
    The middle entry is exactly 65536 - no bending.

    The range of the table is one semitone /in either direction/
    Ie, one semitone from the center.

    Bends beyond this can be achieved by first offsetting the index
    into the GUS frequency table by the appropriate number of semitones,
    and then using this table to bend the rest of the way.

    Generated using Matlab code:
    for i=0:512, fprintf('%d,', round(2^16*2^((i-256)/1536))); end
*/

const uint32_t pitchTbl[] ICONST_ATTR={
    61858,61872,61886,61900,61914,61928,61942,61956,61970,61983,61997,62011,
    62025,62039,62053,62067,62081,62095,62109,62124,62138,62152,62166,62180,
    62194,62208,62222,62236,62250,62264,62278,62292,62306,62320,62334,62348,
    62362,62376,62390,62404,62419,62433,62447,62461,62475,62489,62503,62517,
    62531,62545,62560,62574,62588,62602,62616,62630,62644,62658,62673,62687,
    62701,62715,62729,62743,62757,62772,62786,62800,62814,62828,62843,62857,
    62871,62885,62899,62913,62928,62942,62956,62970,62984,62999,63013,63027,
    63041,63056,63070,63084,63098,63112,63127,63141,63155,63169,63184,63198,
    63212,63227,63241,63255,63269,63284,63298,63312,63326,63341,63355,63369,
    63384,63398,63412,63427,63441,63455,63470,63484,63498,63512,63527,63541,
    63555,63570,63584,63599,63613,63627,63642,63656,63670,63685,63699,63713,
    63728,63742,63757,63771,63785,63800,63814,63829,63843,63857,63872,63886,
    63901,63915,63929,63944,63958,63973,63987,64002,64016,64030,64045,64059,
    64074,64088,64103,64117,64132,64146,64161,64175,64190,64204,64219,64233,
    64248,64262,64277,64291,64306,64320,64335,64349,64364,64378,64393,64407,
    64422,64436,64451,64465,64480,64494,64509,64524,64538,64553,64567,64582,
    64596,64611,64626,64640,64655,64669,64684,64699,64713,64728,64742,64757,
    64772,64786,64801,64815,64830,64845,64859,64874,64889,64903,64918,64933,
    64947,64962,64976,64991,65006,65020,65035,65050,65065,65079,65094,65109,
    65123,65138,65153,65167,65182,65197,65211,65226,65241,65256,65270,65285,
    65300,65315,65329,65344,65359,65374,65388,65403,65418,65433,65447,65462,
    65477,65492,65506,65521,65536,65551,65566,65580,65595,65610,65625,65640,
    65654,65669,65684,65699,65714,65729,65743,65758,65773,65788,65803,65818,
    65832,65847,65862,65877,65892,65907,65922,65936,65951,65966,65981,65996,
    66011,66026,66041,66056,66071,66085,66100,66115,66130,66145,66160,66175,
    66190,66205,66220,66235,66250,66265,66280,66294,66309,66324,66339,66354,
    66369,66384,66399,66414,66429,66444,66459,66474,66489,66504,66519,66534,
    66549,66564,66579,66594,66609,66624,66639,66654,66670,66685,66700,66715,
    66730,66745,66760,66775,66790,66805,66820,66835,66850,66865,66880,66896,
    66911,66926,66941,66956,66971,66986,67001,67016,67032,67047,67062,67077,
    67092,67107,67122,67137,67153,67168,67183,67198,67213,67228,67244,67259,
    67274,67289,67304,67320,67335,67350,67365,67380,67395,67411,67426,67441,
    67456,67472,67487,67502,67517,67532,67548,67563,67578,67593,67609,67624,
    67639,67655,67670,67685,67700,67716,67731,67746,67761,67777,67792,67807,
    67823,67838,67853,67869,67884,67899,67915,67930,67945,67961,67976,67991,
    68007,68022,68037,68053,68068,68083,68099,68114,68129,68145,68160,68176,
    68191,68206,68222,68237,68252,68268,68283,68299,68314,68330,68345,68360,
    68376,68391,68407,68422,68438,68453,68468,68484,68499,68515,68530,68546,
    68561,68577,68592,68608,68623,68639,68654,68670,68685,68701,68716,68732,
    68747,68763,68778,68794,68809,68825,68840,68856,68871,68887,68902,68918,
    68933,68949,68965,68980,68996,69011,69027,69042,69058,69074,69089,69105,
    69120,69136,69152,69167,69183,69198,69214,69230,69245,69261,69276,69292,
    69308,69323,69339,69355,69370,69386,69402,69417,69433
};


static void findDelta(struct SynthObject * so, int ch, int note)
{

    struct GWaveform * wf = patchSet[chPat[ch]]->waveforms[patchSet[chPat[ch]]->noteTable[note]];
    so->wf=wf;
    unsigned int delta= 0;
    delta = (((gustable[note+chPBNoteOffset[ch]]<<FRACTSIZE) / (wf->rootFreq)) * wf->sampRate / (SAMPLE_RATE));
    delta = (delta * chPBFractBend[ch])>> 16;

    so->delta = delta;
}

static inline void setPW(int ch, int msb, int lsb)
{
    chPW[ch] = msb<<2|lsb>>5;

    int totalBend = (chPW[ch]-256) * chPBDepth[ch];
    chPBNoteOffset[ch] = totalBend >> 8;
    chPBFractBend[ch] = pitchTbl[(totalBend & 0xFF) + 256];

    int a=0;
    for(a = 0; a<MAX_VOICES; a++)
    {
        if(voices[a].isUsed==1 && voices[a].ch == ch)
        {
            findDelta(&voices[a], ch, voices[a].note);
        }
    }
}

inline void pressNote(int ch, int note, int vol)
{
    static int lastKill = 0;
/* Silences all channels but one, for easy debugging, for me. */
/*
    if(ch == 0) return;
    if(ch == 1) return;
    if(ch == 2) return;
    if(ch == 3) return;
    if(ch == 4) return;
    if(ch == 5) return;
    if(ch == 6) return;
    if(ch == 7) return;
    if(ch == 8) return;
    if(ch == 9) return;
    if(ch == 10) return;
    if(ch == 11) return;
    if(ch == 12) return;
    if(ch == 13) return;
    if(ch == 14) return;
    if(ch == 15) return;
*/
    int a=0;
    for(a=0; a<MAX_VOICES; a++)
    {
        if(voices[a].ch == ch && voices[a].note == note)
            break;

        if(voices[a].isUsed==0)
            break;
    }
    if(a==MAX_VOICES)
    {
//        printf("\nVoice kill");
//        printf("\nToo many voices playing at once. No more left");
//        printf("\nVOICE DUMP: ");
//        for(a=0; a<48; a++)
//            printf("\n#%d  Ch=%d  Note=%d  curRate=%d   curOffset=%d   curPoint=%d   targetOffset=%d", a, voices[a].ch, voices[a].note, voices[a].curRate, voices[a].curOffset, voices[a].curPoint, voices[a].targetOffset);
        lastKill++;
        if(lastKill == MAX_VOICES)
            lastKill = 0;
        a = lastKill;
//        return; /* None available */
    }
    voices[a].ch=ch;
    voices[a].note=note;
    voices[a].vol=vol;
    voices[a].cp=0;
    voices[a].state=STATE_ATTACK;
    voices[a].decay=255;

    setVolScale(a);

    voices[a].loopState=STATE_NONLOOPING;
    /*
     * OKAY. Gt = Gus Table value
     * rf = Root Frequency of wave
     * SR = sound sampling rate
     * sr = WAVE sampling rate
     */

    if(ch!=9)
    {
        findDelta(&voices[a], ch, note);
        /* Turn it on */
        voices[a].isUsed=1;
        setPoint(&voices[a], 0);
    } else
    {
        if(drumSet[note]!=NULL)
        {
            if(note<35)
                printf("NOTE LESS THAN 35, AND A DRUM PATCH EXISTS FOR THIS? WHAT THE HELL?");

            struct GWaveform * wf = drumSet[note]->waveforms[0];
            voices[a].wf=wf;
            voices[a].delta = (((gustable[note]<<10) / wf->rootFreq) * wf->sampRate / SAMPLE_RATE);
            if(wf->mode & 28)
//                printf("\nWoah, a drum patch has a loop. Stripping the loop...");
            wf->mode = wf->mode & (255-28);

            /* Turn it on */
            voices[a].isUsed=1;
            setPoint(&voices[a], 0);

        } else
        {
/*            printf("\nWarning: drum %d does not have a patch defined... Ignoring it", note); */
        }
    }
}

static void releaseNote(int ch, int note)
{

    if(ch==9)
        return;

    int a=0;
    for(a=0; a<MAX_VOICES; a++)
    {
        if(voices[a].ch == ch && voices[a].note == note)
        {
            if((voices[a].wf->mode & 28))
            {
                setPoint(&voices[a], 3);
            }
        }
    }
}

static void sendEvent(struct Event * ev)
{
    const unsigned char status_low = ev->status & 0x0F;
    const unsigned char d1 = ev->d1;
    const unsigned char d2 = ev->d2;
    switch(ev->status & 0xF0)
    {
        case MIDI_CONTROL:
            switch(d1)
            {
                case CTRL_VOLUME:
                {
                    setVol((status_low), d2);
                    return;
                }
                case CTRL_PANNING:
                {
                    chPan[status_low]=d2;
                    return;
                }
                case CTRL_DATAENT_MSB:
                {
                    /* TODO: Update all deltas. Is this really needed? */
                    if(chLastCtrlMSB[status_low] == REG_PITCHBEND_MSB &&
                       chLastCtrlLSB[status_low] == REG_PITCHBEND_LSB)
                    {
//                         printf("Pitch bend depth set to %d\n", d2);
                        chPBDepth[status_low] = d2;
                    }
                    return;
                }

                case CTRL_NONREG_LSB:
                {
                    chLastCtrlLSB[status_low] = 0xFF;   /* Ignore nonregistered writes */
                    return;
                }

                case CTRL_NONREG_MSB:
                {
                    chLastCtrlMSB[status_low] = 0xFF;   /* Ignore nonregistered writes */
                    return;
                }

                case CTRL_REG_LSB:
                {
                    chLastCtrlLSB[status_low] = d2;
                    return;
                }

                case CTRL_REG_MSB:
                {
                    chLastCtrlMSB[status_low] = d2;
                    return;
                }

            }
            break;

        case MIDI_PITCHW:
            setPW((status_low), d2, d1);
            return;

        case MIDI_NOTE_ON:
            switch(d2)
            {
                case 0: /* Release by vol=0 */
                    releaseNote(status_low, d1);
                    return;

                default:
                    pressNote(status_low, d1, d2);
                    return;
            }

        case MIDI_NOTE_OFF:
            releaseNote(status_low, d1);
            return;

        case MIDI_PRGM:
            if((status_low) != 9)
                setPatch(status_low, d1);
    }
}

void rewindFile()
{
    int i=0;
    for(i=0; i<mf->numTracks; i++)
    {
        mf->tracks[i]->delta = 0;
        mf->tracks[i]->pos = 0;
    }
}

int tick(void) ICODE_ATTR;
int tick(void)
{
    if(mf==NULL)
        return 0;

    int a=0;
    int tracksAdv=0;
    for(a=0; a<mf->numTracks; a++)
    {
        struct Track * tr = mf->tracks[a];

        if(tr == NULL)
            printf("NULL TRACK: %d", a);


        //BIG DEBUG STATEMENT
        //printf("\nTrack %2d,  Event = %4d of %4d,   Delta = %5d,    Next = %4d", a, tr->pos, tr->numEvents, tr->delta, getEvent(tr, tr->pos)->delta);


        if(tr != NULL && (tr->pos < tr->numEvents))
        {
            tr->delta++;
            tracksAdv++;
            while(getEvent(tr, tr->pos)->delta <= tr->delta)
            {
                struct Event * e = getEvent(tr, tr->pos);

                if(e->status != 0xFF)
                {
                    sendEvent(e);
                    if(((e->status&0xF0) == MIDI_PRGM))
                    {
/*                        printf("\nPatch Event, patch[%d] ==> %d", e->status&0xF, e->d1); */
                    }
                }
                else
                {
                    if(e->d1 == 0x51)
                    {
                        tempo = (((short)e->evData[0])<<16)|(((short)e->evData[1])<<8)|(e->evData[2]);
/*                        printf("\nMeta-Event: Tempo Set = %d", tempo); */
                        bpm=mf->div*1000000/tempo;
                        numberOfSamples=SAMPLE_RATE/bpm;

                    }
                }
                tr->delta = 0;
                tr->pos++;
                if(tr->pos>=(tr->numEvents-1))
                    break;
            }
        }
    }

    samplesThisSecond += numberOfSamples;

    while(samplesThisSecond >= SAMPLE_RATE)
    {
        samplesThisSecond -= SAMPLE_RATE;
        playingTime++;
//         printf("Time: %d sec\n", playingTime);
    }

    if(tracksAdv != 0)
        return 1;
    else
        return 0;
}

