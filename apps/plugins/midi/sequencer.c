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


void findDelta(struct SynthObject * so, int ch, int note)
{

    struct GWaveform * wf = patchSet[chPat[ch]]->waveforms[patchSet[chPat[ch]]->noteTable[note]];
    so->wf=wf;
    so->delta = (((gustable[note]<<10) / (wf->rootFreq)) * wf->sampRate / (SAMPLE_RATE));
    so->delta = (so->delta * pitchTbl[chPW[ch]])>> 16;
}

void setPW(int ch, int msb)
{
    printf("\npitchw[%d]  %d ==> %d", ch, chPW[ch], msb);
    chPW[ch] = msb;

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
        setPW((ev->status & 0xF), ev->d2);
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
