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


/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define BTN_QUIT     BUTTON_OFF
#define BTN_RIGHT    BUTTON_RIGHT
#define BTN_UP       BUTTON_UP
#define BTN_DOWN     BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define BTN_QUIT         BUTTON_OFF
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BTN_QUIT         BUTTON_OFF
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define BTN_QUIT         (BUTTON_SELECT | BUTTON_MENU)
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_SCROLL_FWD
#define BTN_DOWN         BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_VOL_UP
#define BTN_DOWN         BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#endif



#define FRACTSIZE 10
#define SAMPLE_RATE 22050  // 44100 22050 11025
#define MAX_VOICES 14   // Note: 24 midi channels is the minimum general midi
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
    int notesUsed = 0;
    int a=0;
    printf("\nLoading file");
    mf= loadFile(filename);

    if(mf == NULL)
    {
        printf("\nError loading file.");
        return -1;
    }

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



    /* Skip over any junk in the beginning of the file, so start playing */
    /* after the first note event */
    do
    {
        notesUsed = 0;
        for(a=0; a<MAX_VOICES; a++)
            if(voices[a].isUsed == 1)
                notesUsed++;
        tick();
    } while(notesUsed == 0);

    synthbuf();
#ifndef SIMULATOR
    rb->pcm_play_data(&get_more, NULL, 0);
#endif

    int vol=0;

    while(!quit)
    {
    #ifndef SYNC
        synthbuf();
    #endif
        rb->yield();

        /* Code taken from Oscilloscope plugin */
        switch(rb->button_get(false))
        {
                case BTN_UP:
                case BTN_UP | BUTTON_REPEAT:
                    vol = rb->global_settings->volume;
                    if (vol < rb->sound_max(SOUND_VOLUME))
                    {
                        vol++;
                        rb->sound_set(SOUND_VOLUME, vol);
                        rb->global_settings->volume = vol;
                    }
                    break;

                case BTN_DOWN:
                case BTN_DOWN | BUTTON_REPEAT:
                    vol = rb->global_settings->volume;
                    if (vol > rb->sound_min(SOUND_VOLUME))
                    {
                        vol--;
                        rb->sound_set(SOUND_VOLUME, vol);
                        rb->global_settings->volume = vol;
                    }
                    break;

                case BTN_RIGHT:
                {
                    /* Skip 3 seconds */
                    /* Should skip length be retrieved from the RB settings? */
                    int samp = 3*SAMPLE_RATE;
                    int tickCount = samp / numberOfSamples;
                    int a=0;
                    for(a=0; a<tickCount; a++)
                        tick();
                    break;
                }

                case BTN_QUIT:
                    quit=1;
        }


    }

    return 0;
}
