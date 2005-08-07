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

long tempo=375000;


void setVol(int ch, int vol)
{
    printf("\nvolume[%d]  %d ==> %d", ch, chVol[ch], vol);
    chVol[ch]=vol;
}

void setPan(int ch, int pan)
{
    printf("\npanning[%d]  %d ==> %d", ch, chPanRight[ch], pan);

    chPanLeft[ch]=128-pan;
    chPanRight[ch]=pan;
}


void setPatch(int ch, int pat)
{
    chPat[ch]=pat;
}


/*
 * Pitch Bend table, Computed by
 * for i=0:127, fprintf('%d,', round(2^16*2^((i-64)/384))); end
 * (When typed into Matlab)
 * 16 bit pitch bend table
*/
/*
long pitchTbl[]=
{
    58386,58491,58597,58703,58809,58915,59022,59128,59235,59342,59449,59557,59664,59772,59880,59988,60097,60205,
    60314,60423,60532,60642,60751,60861,60971,61081,61191,61302,61413,61524,61635,61746,61858,61970,62081,62194,
    62306,62419,62531,62644,62757,62871,62984,63098,63212,63326,63441,63555,63670,63785,63901,64016,64132,64248,
    64364,64480,64596,64713,64830,64947,65065,65182,65300,65418,65536,65654,65773,65892,66011,66130,66250,66369,
    66489,66609,66730,66850,66971,67092,67213,67335,67456,67578,67700,67823,67945,68068,68191,68314,68438,68561,
    68685,68809,68933,69058,69183,69308,69433,69558,69684,69810,69936,70062,70189,70316,70443,70570,70698,70825,
    70953,71082,71210,71339,71468,71597,71726,71856,71985,72115,72246,72376,72507,72638,72769,72901,73032,73164,
    73297,73429
};
*/
long pitchTbl[]={
58386,58412,58439,58465,58491,58518,58544,58571,58597,58624,58650,58676,58703,58729,58756,58782,58809,58836,58862,58889,58915,58942,58968,58995,59022,59048,59075,59102,59128,59155,59182,59208,59235,59262,59289,59315,59342,59369,59396,59423,59449,59476,59503,59530,59557,59584,59611,59638,59664,59691,59718,59745,59772,59799,59826,59853,59880,59907,59934,59961,59988,60015,60043,60070,60097,60124,60151,60178,60205,60233,60260,60287,60314,60341,60369,60396,60423,60450,60478,60505,60532,60560,60587,60614,60642,60669,60696,60724,60751,60779,60806,60833,60861,60888,60916,60943,60971,60998,61026,61054,61081,61109,61136,61164,61191,61219,61247,61274,61302,61330,61357,61385,61413,61440,61468,61496,61524,61551,61579,61607,61635,61663,61690,61718,61746,61774,61802,61830,61858,61886,61914,61942,61970,61997,62025,62053,62081,62109,62138,62166,62194,62222,62250,62278,62306,62334,62362,62390,62419,62447,62475,62503,62531,62560,62588,62616,62644,62673,62701,62729,62757,62786,62814,62843,62871,62899,62928,62956,62984,63013,63041,63070,63098,63127,63155,63184,63212,63241,63269,63298,63326,63355,63384,63412,63441,63470,63498,63527,63555,63584,63613,63642,63670,63699,63728,63757,63785,63814,63843,63872,63901,63929,63958,63987,64016,64045,64074,64103,64132,64161,64190,64219,64248,64277,64306,64335,64364,64393,64422,64451,64480,64509,64538,64567,64596,64626,64655,64684,64713,64742,64772,64801,64830,64859,64889,64918,64947,64976,65006,65035,65065,65094,65123,65153,65182,65211,65241,65270,65300,65329,65359,65388,65418,65447,65477,65506,65536,65566,65595,65625,65654,65684,65714,65743,65773,65803,65832,65862,65892,65922,65951,65981,66011,66041,66071,66100,66130,66160,66190,66220,66250,66280,66309,66339,66369,66399,66429,66459,66489,66519,66549,66579,66609,66639,66670,66700,66730,66760,66790,66820,66850,66880,66911,66941,66971,67001,67032,67062,67092,67122,67153,67183,67213,67244,67274,67304,67335,67365,67395,67426,67456,67487,67517,67548,67578,67609,67639,67670,67700,67731,67761,67792,67823,67853,67884,67915,67945,67976,68007,68037,68068,68099,68129,68160,68191,68222,68252,68283,68314,68345,68376,68407,68438,68468,68499,68530,68561,68592,68623,68654,68685,68716,68747,68778,68809,68840,68871,68902,68933,68965,68996,69027,69058,69089,69120,69152,69183,69214,69245,69276,69308,69339,69370,69402,69433,69464,69496,69527,69558,69590,69621,69653,69684,69716,69747,69778,69810,69841,69873,69905,69936,69968,69999,70031,70062,70094,70126,70157,70189,70221,70252,70284,70316,70348,70379,70411,70443,70475,70507,70538,70570,70602,70634,70666,70698,70730,70762,70793,70825,70857,70889,70921,70953,70985,71017,71049,71082,71114,71146,71178,71210,71242,71274,71306,71339,71371,71403,71435,71468,71500,71532,71564,71597,71629,71661,71694,71726,71758,71791,71823,71856,71888,71920,71953,71985,72018,72050,72083,72115,72148,72181,72213,72246,72278,72311,72344,72376,72409,72442,72474,72507,72540,72573,72605,72638,72671,72704,72736,72769,72802,72835,72868,72901,72934,72967,72999,73032,73065,73098,73131,73164,73197,73230,73264,73297,73330,73363,73396,73429,73462,73495,73528
};

void findDelta(struct SynthObject * so, int ch, int note)
{

    struct GWaveform * wf = patchSet[chPat[ch]]->waveforms[patchSet[chPat[ch]]->noteTable[note]];
    so->wf=wf;                  // \|/ was 10
    so->delta = (((gustable[note]<<10) / (wf->rootFreq)) * wf->sampRate / (SAMPLE_RATE));
    so->delta = (so->delta * pitchTbl[chPW[ch]])>> 16;
}

void setPW(int ch, int msb, int lsb)
{
    printf("\npitchw[%d]  %d ==> %d", ch, chPW[ch], msb);
    chPW[ch] = msb<<2|lsb>>5;

    int a=0;
    for(a = 0; a<MAX_VOICES; a++)
    {
        if(voices[a].isUsed==1 && voices[a].ch == ch)
        {
            findDelta(&voices[a], ch, voices[a].note);
        }
    }
}

void pressNote(int ch, int note, int vol)
{

//Silences all channels but one, for easy debugging, for me.
/*
    if(ch == 0) return;
    if(ch == 1) return;
    if(ch == 2) return;
    if(ch == 3) return;
//  if(ch == 4) return;
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
    if(a==MAX_VOICES-1)
    {
        printf("\nOVERFLOW: Too many voices playing at once. No more left");
        printf("\nVOICE DUMP: ");
        for(a=0; a<48; a++)
            printf("\n#%d  Ch=%d  Note=%d  curRate=%d   curOffset=%d   curPoint=%d   targetOffset=%d", a, voices[a].ch, voices[a].note, voices[a].curRate, voices[a].curOffset, voices[a].curPoint, voices[a].targetOffset);
        return; /* None available */
    }
    voices[a].ch=ch;
    voices[a].note=note;
    voices[a].vol=vol;
    voices[a].cp=0;
    voices[a].state=STATE_ATTACK;
    voices[a].decay=255;


    voices[a].loopState=STATE_NONLOOPING;
    voices[a].loopDir = LOOPDIR_FORWARD;
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
                printf("\nNOTE LESS THAN 35, AND A DRUM PATCH EXISTS FOR THIS? WHAT THE HELL?");

            struct GWaveform * wf = drumSet[note]->waveforms[0];
            voices[a].wf=wf;
            voices[a].delta = (((gustable[note]<<10) / wf->rootFreq) * wf->sampRate / SAMPLE_RATE);
            if(wf->mode & 28)
                printf("\nWoah, a drum patch has a loop. Stripping the loop...");
            wf->mode = wf->mode & (255-28);

            /* Turn it on */
            voices[a].isUsed=1;
            setPoint(&voices[a], 0);

        } else
        {
            printf("\nWarning: drum %d does not have a patch defined... Ignoring it", note);
        }
    }
}


void releaseNote(int ch, int note)
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

void sendEvent(struct Event * ev)
{
    if( ((ev->status & 0xF0) == MIDI_CONTROL) && (ev->d1 == CTRL_VOLUME) )
    {
        setVol((ev->status & 0xF), ev->d2);
        return;
    }

    if( ((ev->status & 0xF0) == MIDI_CONTROL) && (ev->d1 == CTRL_PANNING))
    {
        setPan((ev->status & 0xF), ev->d2);
        return;
    }

    if(((ev->status & 0xF0) == MIDI_PITCHW))
    {
        setPW((ev->status & 0xF), ev->d2, ev->d1);
        return;
    }

    if(((ev->status & 0xF0) == MIDI_NOTE_ON) && (ev->d2 != 0))
    {
        pressNote(ev->status & 0x0F, ev->d1, ev->d2);
        return;
    }

    if(((ev->status & 0xF0) == MIDI_NOTE_ON) && (ev->d2 == 0)) /* Release by vol=0 */
    {
        releaseNote(ev->status & 0x0F, ev->d1);
        return;
    }


    if((ev->status & 0xF0) == MIDI_NOTE_OFF)
    {
        releaseNote(ev->status & 0x0F, ev->d1);
        return;
    }

    if((ev->status & 0xF0) == MIDI_PRGM)
    {
        if((ev->status & 0x0F) == 9)
            printf("\nNOT PATCHING: Someone tried patching Channel 9 onto something?");
        else
            setPatch(ev->status & 0x0F, ev->d1);
    }
}





int tick(struct MIDIfile * mf)
{
    if(mf==NULL)
        return 0;

    int a=0;
    int tracksAdv=0;
    for(a=0; a<mf->numTracks; a++)
    {
        struct Track * tr = mf->tracks[a];

        if(tr == NULL)
            printf("\nNULL TRACK: %d", a);


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
                        printf("\nPatch Event, patch[%d] ==> %d", e->status&0xF, e->d1);
                    }
                }
                else
                {
                    if(e->d1 == 0x51)
                    {
                        tempo = (((short)e->evData[0])<<16)|(((short)e->evData[1])<<8)|(e->evData[2]);
                        printf("\nMeta-Event: Tempo Set = %d", tempo);
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

    if(tracksAdv != 0)
        return 1;
    else
        return 0;
}
