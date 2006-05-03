/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Karl Kurbjun based on midi2wav by Stepan Moskovchenko
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "../../plugin.h"

PLUGIN_HEADER

#define FRACTSIZE 10
#define SAMPLE_RATE 22050  // 44100 22050 11025
#define MAX_VOICES 13   // Note: 24 midi channels is the minimum general midi
                         // spec implementation
#define BUF_SIZE 512
#define NBUF   2

#undef SYNC
struct MIDIfile * mf IBSS_ATTR;

int numberOfSamples IBSS_ATTR;
long bpm IBSS_ATTR;

#include "midi/midiutil.c"
#include "midi/guspat.h"
#include "midi/guspat.c"
#include "midi/sequencer.c"
#include "midi/midifile.c"
#include "midi/synth.c"

short gmbuf[BUF_SIZE*NBUF] IBSS_ATTR;

int quit=0;
struct plugin_api * rb;

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int retval = 0;
    rb = api;

    if(parameter == NULL)
    {
        rb->splash(HZ*2, true, " Play .MID file ");
        return PLUGIN_OK;
    }
    rb->lcd_setfont(0);

#ifdef USE_IRAM
    rb->memcpy(iramstart, iramcopy, iramend-iramstart);
    rb->memset(iedata, 0, iend - iedata);
#endif

#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

    printf("\n%s", parameter);
    /*   rb->splash(HZ, true, parameter); */

#ifdef RB_PROFILE
    rb->profile_thread();
#endif

    retval = midimain(parameter);

#ifdef RB_PROFILE
    rb->profstop();
#endif

#ifndef SIMULATOR
    rb->pcm_play_stop();
    rb->pcm_set_frequency(44100); // 44100
#endif

#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif

    rb->splash(HZ, true, "FINISHED PLAYING");

    if(retval == -1)
        return PLUGIN_ERROR;
    return PLUGIN_OK;
}

bool swap=0;
bool lastswap=1;

inline void synthbuf(void)
{
   short *outptr;
   register int i;
   static int currentSample=0;
   int synthtemp[2];

#ifndef SYNC
   if(lastswap==swap) return;
   lastswap=swap;

   outptr=(swap ? gmbuf : gmbuf+BUF_SIZE);
#else
   outptr=gmbuf;
#endif

   for(i=0; i<BUF_SIZE/2; i++)
   {
      synthSample(&synthtemp[0], &synthtemp[1]);
      currentSample++;
      *outptr=synthtemp[0]&0xFFFF;
      outptr++;
      *outptr=synthtemp[1]&0xFFFF;
      outptr++;
      if(currentSample==numberOfSamples)
      {
         if( tick() == 0 ) quit=1;
         currentSample=0;
      }
   }
}

void get_more(unsigned char** start, size_t* size)
{
#ifndef SYNC
    if(lastswap!=swap)
    {
        printf("Buffer miss!"); // Comment out the printf to make missses less noticable.
    }

#else
    synthbuf();  // For some reason midiplayer crashes when an update is forced
#endif

    *size = BUF_SIZE*sizeof(short);
#ifndef SYNC
    *start = (unsigned char*)((swap ? gmbuf : gmbuf + BUF_SIZE));
    swap=!swap;
#else
    *start = (unsigned char*)(gmbuf);
#endif
}

int midimain(void * filename)
{
    int button;

    /*   rb->splash(HZ/5, true, "LOADING MIDI"); */
    printf("\nLoading file");
    mf= loadFile(filename);

    /*   rb->splash(HZ/5, true, "LOADING PATCHES"); */
    if (initSynth(mf, "/.rockbox/patchset/patchset.cfg", "/.rockbox/patchset/drums.cfg") == -1)
        return -1;

#ifndef SIMULATOR
    rb->pcm_play_stop();
    rb->pcm_set_frequency(SAMPLE_RATE); // 44100 22050 11025
#endif

    /*
        * tick() will do one MIDI clock tick. Then, there's a loop here that
        * will generate the right number of samples per MIDI tick. The whole
        * MIDI playback is timed in terms of this value.. there are no forced
        * delays or anything. It just produces enough samples for each tick, and
        * the playback of these samples is what makes the timings right.
        *
        * This seems to work quite well. On a laptop, anyway.
        */

    printf("\nOkay, starting sequencing");

    bpm=mf->div*1000000/tempo;
    numberOfSamples=SAMPLE_RATE/bpm;

    tick();

    synthbuf();
#ifndef SIMULATOR
    rb->pcm_play_data(&get_more, NULL, 0);
#endif

    button=rb->button_status();

    while(!quit)
    {
    #ifndef SYNC
        synthbuf();
    #endif
        rb->yield();
        if(rb->button_status()!=button) quit=1;
    }

    return 0;
}
