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

#define SAMPLE_RATE 22050
#define MAX_VOICES 100


/* Only define LOCAL_DSP on Simulator or else we're asking for trouble  */
#if defined(SIMULATOR)
    /*Enable this to write to the soundcard via a /dsv/dsp symlink in */
    //#define LOCAL_DSP
#endif


#if defined(LOCAL_DSP)
/*  This is for writing to the DSP directly from the Simulator */
#include <stdio.h>
#include <stdlib.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#endif

#include "../../firmware/export/system.h"

#include "../../plugin.h"

//#include "../codecs/lib/xxx2wav.h"

int numberOfSamples IDATA_ATTR;
long bpm;

#include "midi/midiutil.c"
#include "midi/guspat.h"
#include "midi/guspat.c"
#include "midi/sequencer.c"
#include "midi/midifile.c"
#include "midi/synth.c"




int fd=-1; /* File descriptor where the output is written */

extern long tempo; /* The sequencer keeps track of this */


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
    /* Return PLUGIN_USB_CONNECTED to force a file-tree refresh */
    return PLUGIN_USB_CONNECTED;
}

signed char outputBuffer[3000] IDATA_ATTR; /* signed char.. gonna run out of iram ... ! */


int currentSample IDATA_ATTR;
int outputBufferPosition IDATA_ATTR;
int outputSampleOne IDATA_ATTR;
int outputSampleTwo IDATA_ATTR;


int midimain(void * filename)
{

    printf("\nHello.\n");

    rb->splash(HZ/5, true, "LOADING MIDI");

    struct MIDIfile * mf = loadFile(filename);

    rb->splash(HZ/5, true, "LOADING PATCHES");
    if (initSynth(mf, "/.rockbox/patchset/patchset.cfg", "/.rockbox/patchset/drums.cfg") == -1)
    {
        return -1;
    }

/*
 *  This lets you hear the music through the sound card if you are on Simulator
 *  Make a symlink, archos/dsp.raw and make it point to /dev/dsp or whatever
 *  your sound device is.
 */

#if defined(LOCAL_DSP)
    fd=rb->open("/dsp.raw", O_WRONLY);
    int arg, status;
    int bit, samp, ch;

    arg = 16;          /* sample size */
    status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);
    status = ioctl(fd, SOUND_PCM_READ_BITS, &arg);
    bit=arg;


    arg = 2;        /* Number of channels, 1=mono */
    status = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg);
    status = ioctl(fd, SOUND_PCM_READ_CHANNELS, &arg);
    ch=arg;

    arg = SAMPLE_RATE; /* Yeah. sampling rate */
    status = ioctl(fd, SOUND_PCM_WRITE_RATE, &arg);
    status = ioctl(fd, SOUND_PCM_READ_RATE, &arg);
    samp=arg;
#else

/* xxx2wav stuff, removed for now, will move to the real way of outputting sound soon */
/*
    file_info_struct file_info;
    file_info.samplerate = SAMPLE_RATE;
    file_info.infile = fd;
    file_info.channels = 2;
    file_info.bitspersample = 16;
    local_init("/miditest.tmp", "/miditest.wav", &file_info, rb);
    fd = file_info.outfile;
*/
#endif


    rb->splash(HZ/5, true, "  I hope this works...  ");




    /*
     * tick() will do one MIDI clock tick. Then, there's a loop here that
     * will generate the right number of samples per MIDI tick. The whole
     * MIDI playback is timed in terms of this value.. there are no forced
     * delays or anything. It just produces enough samples for each tick, and
     * the playback of these samples is what makes the timings right.
     *
     * This seems to work quite well.
     */

    printf("\nOkay, starting sequencing");


    currentSample=0;            /* Sample counting variable */
    outputBufferPosition = 0;


    bpm=mf->div*1000000/tempo;
    numberOfSamples=SAMPLE_RATE/bpm;



    /* Tick() will return 0 if there are no more events left to play */
    while(tick(mf))
    {
        /*
         * Tempo recalculation moved to sequencer.c to be done on a tempo event only
         *
         */
        for(currentSample=0; currentSample<numberOfSamples; currentSample++)
        {

            synthSample(&outputSampleOne, &outputSampleTwo);


            /*
             * 16-bit audio because, well, it's better
             * But really because ALSA's OSS emulation sounds extremely
             * noisy and distorted when in 8-bit mode. I still do not know
             * why this happens.
             */

            outputBuffer[outputBufferPosition]=outputSampleOne&0XFF; // Low byte first
            outputBufferPosition++;
            outputBuffer[outputBufferPosition]=outputSampleOne>>8;   //High byte second
            outputBufferPosition++;

            outputBuffer[outputBufferPosition]=outputSampleTwo&0XFF; // Low byte first
            outputBufferPosition++;
            outputBuffer[outputBufferPosition]=outputSampleTwo>>8;   //High byte second
            outputBufferPosition++;


            /*
             * As soon as we produce 2000 bytes of sound,
             * write it to the sound card. Why 2000? I have
             * no idea. It's 1 AM and I am dead tired.
             */
            if(outputBufferPosition>=2000)
            {
                rb->write(fd, outputBuffer, 2000);
                outputBufferPosition=0;
            }
        }
    }

    printf("\n");

#if !defined(LOCAL_DSP)

/* again, xxx2wav stuff, removed for now */

    /* close_wav(&file_info); */
#else
    rb->close(fd);
#endif
    return 0;
}
