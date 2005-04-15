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

#define SAMPLE_RATE 48000
#define MAX_VOICES 100

/*
#if defined(SIMULATOR)
//	This is for writing to the DSP directly from the Simulator
#include <stdio.h>
#include <stdlib.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#endif
*/


#include "../../plugin.h"
#include "midi/midiutil.c"
#include "midi/guspat.h"
#include "midi/guspat.c"
#include "midi/sequencer.c"
#include "midi/midifile.c"
#include "midi/synth.c"



//#include "lib/xxx2wav.h"

int fd=-1; //File descriptor, for opening /dev/dsp and writing to it

extern long tempo; //The sequencer keeps track of this


struct plugin_api * rb;




enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
	TEST_PLUGIN_API(api);
	rb = api;
    	TEST_PLUGIN_API(api);
    	(void)parameter;
    	rb = api;

    	if(parameter == NULL)
    	{
    		rb->splash(HZ*2, true, " Play .MID file ");
    		return PLUGIN_OK;
    	}
    	rb->splash(HZ, true, parameter);
    	if(midimain(parameter) == -1)
    	{
    		return PLUGIN_ERROR;
    	}
    	rb->splash(HZ*3, true, "FINISHED PLAYING");
    	return PLUGIN_OK;
}


int midimain(void * filename)
{

	printf("\nHello.\n");

	rb->splash(HZ/5, true, "LOADING MIDI");

	struct MIDIfile * mf = loadFile(filename);
	long bpm, nsmp, l;

	int bp=0;

	rb->splash(HZ/5, true, "LOADING PATCHES");
	if (initSynth(mf, "/.rockbox/patchset/patchset.cfg", "/.rockbox/patchset/drums.cfg") == -1)
	{
		return -1;
	}

	fd=rb->open("/dsp.raw", O_WRONLY|O_CREAT);

/*
//This lets you hear the music through the sound card if you are on Simulator
//Make a symlink, archos/dsp.raw and make it point to /dev/dsp or whatever
//your sound device is.

#if defined(SIMULATOR)
        int arg, status;
        int bit, samp, ch;

        arg = 16;          // sample size
        status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);
        status = ioctl(fd, SOUND_PCM_READ_BITS, &arg);
        bit=arg;


        arg = 2;        //Number of channels, 1=mono
        status = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg);
        status = ioctl(fd, SOUND_PCM_READ_CHANNELS, &arg);
        ch=arg;

        arg = SAMPLE_RATE; //Yeah. sampling rate
        status = ioctl(fd, SOUND_PCM_WRITE_RATE, &arg);
        status = ioctl(fd, SOUND_PCM_READ_RATE, &arg);
        samp=arg;
#endif
*/


	rb->splash(HZ/5, true, "  START PLAYING  ");




	signed char buf[3000];

	// tick() will do one MIDI clock tick. Then, there's a loop here that
	// will generate the right number of samples per MIDI tick. The whole
	// MIDI playback is timed in terms of this value.. there are no forced
	// delays or anything. It just produces enough samples for each tick, and
	// the playback of these samples is what makes the timings right.
	//
	// This seems to work quite well.


	printf("\nOkay, starting sequencing");

	//Tick() will return 0 if there are no more events left to play
	while(tick(mf))
	{

		//Some annoying math to compute the number of samples
		//to syntehsize per each MIDI tick.
		bpm=mf->div*1000000/tempo;
		nsmp=SAMPLE_RATE/bpm;

		//Yes we need to do this math each time because the tempo
		//could have changed.

		// On second thought, this can be moved to the event that
		//recalculates the tempo, to save a little bit of CPU time.
		for(l=0; l<nsmp; l++)
		{
			int s1, s2;

			synthSample(&s1, &s2);


			//16-bit audio because, well, it's better
			// But really because ALSA's OSS emulation sounds extremely
			//noisy and distorted when in 8-bit mode. I still do not know
			//why this happens.
			buf[bp]=s1&0XFF; // Low byte first
			bp++;
			buf[bp]=s1>>8;   //High byte second
			bp++;

			buf[bp]=s2&0XFF; // Low byte first
			bp++;
			buf[bp]=s2>>8;   //High byte second
			bp++;


			//As soon as we produce 2000 bytes of sound,
			//write it to the sound card. Why 2000? I have
			//no idea. It's 1 AM and I am dead tired.
			if(bp>=2000)
			{
				rb->write(fd, buf, 2000);
				bp=0;
			}
		}
	}

//	unloadFile(mf);
	printf("\n");
	rb->close(fd);
	return 0;
}
