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
int initSynth(struct MIDIfile * mf, char * filename, char * drumConfig);
signed short int synthVoice(struct SynthObject * so);
void setPoint(struct SynthObject * so, int pt);

static inline void synthSample(int * mixL, int * mixR)
{
    int i;
    register int dL=0;
    register int dR=0;
    register short sample = 0;
    register struct SynthObject *voicept=voices;

    for(i=MAX_VOICES/2; i > 0; i--)
    {
        if(voicept->isUsed==1)
        {
            sample = synthVoice(voicept);
            dL += (sample*chPanLeft[voicept->ch])>>7;
            dR += (sample*chPanRight[voicept->ch])>>7;
        }
        voicept++;
        if(voicept->isUsed==1)
        {
            sample = synthVoice(voicept);
            dL += (sample*chPanLeft[voicept->ch])>>7;
            dR += (sample*chPanRight[voicept->ch])>>7;
        }
        voicept++;
    }

    for(i=MAX_VOICES%2; i > 0; i--)
    {
        if(voicept->isUsed==1)
        {
            sample = synthVoice(voicept);
            dL += (sample*chPanLeft[voicept->ch])>>7;
            dR += (sample*chPanRight[voicept->ch])>>7;
        }
        voicept++;
    }

   *mixL=dL;
   *mixR=dR;

    /* TODO: Automatic Gain Control, anyone? */
    /* Or, should this be implemented on the DSP's output volume instead? */

    return;  /* No more ghetto lowpass filter.. linear intrpolation works well. */
}

static inline struct Event * getEvent(struct Track * tr, int evNum)
{
    return tr->dataBlock + (evNum*sizeof(struct Event));
}

