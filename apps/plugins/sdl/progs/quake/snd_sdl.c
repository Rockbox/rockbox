
#include <stdio.h>
#include "SDL_audio.h"
#include "SDL_byteorder.h"
#include "SDL_mixer.h"
#include "quakedef.h"

static dma_t the_shm;
static int snd_inited;

extern int desired_speed;
extern int desired_bits;

// SDL hereby demands `len' samples in stream, *NOW*!
static void paint_audio(int chan, Uint8 *stream, int len, void *unused)
{
	if ( shm ) {
		shm->buffer = stream;
		shm->samplepos += len/(shm->samplebits/8)/2;
		// Check for samplepos overflow?
		S_PaintChannels (shm->samplepos);
	}
}

qboolean SNDDMA_Init(void)
{
	SDL_AudioSpec desired, obtained;

	snd_inited = 0;

	/* Set up the desired format */
	desired.freq = desired_speed;
	switch (desired_bits) {
		case 8:
			desired.format = AUDIO_U8;
			break;
		case 16:
			if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
				desired.format = AUDIO_S16MSB;
			else
				desired.format = AUDIO_S16LSB;
			break;
		default:
        		Con_Printf("Unknown number of audio bits: %d\n",
								desired_bits);
			return 0;
	}
	desired.channels = 2;
	desired.samples = 1024;
	//desired.callback = paint_audio;

        if( Mix_OpenAudio(desired_speed, desired.format, desired.channels, desired.samples) < 0 )
        {
            Con_Printf("Couldn't open SDL audio: %s\n", Mix_GetError());
            return 0;
        }

        Mix_RegisterEffect(0, paint_audio, NULL, NULL);

#if 0
	/* Open the audio device */
	if ( SDL_OpenAudio(&desired, &obtained) < 0 ) {
        	Con_Printf("Couldn't open SDL audio: %s\n", SDL_GetError());
		return 0;
	}
#endif

        void *blank_buf = (Uint8 *)malloc(4096);
	memset(blank_buf, 0, 4096);

	Mix_Chunk *blank = Mix_QuickLoad_RAW(blank_buf, 4096);

	Mix_PlayChannel(0, blank, -1);

	SDL_PauseAudio(0);

	/* Fill the audio DMA information block */
	shm = &the_shm;
	shm->splitbuffer = 0;
	shm->samplebits = (desired.format & 0xFF);
	shm->speed = desired.freq;
	shm->channels = desired.channels;
	shm->samples = desired.samples*shm->channels;
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = NULL;

	snd_inited = 1;
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	return shm->samplepos;
}

void SNDDMA_Shutdown(void)
{
	if (snd_inited)
	{
		SDL_CloseAudio();
		snd_inited = 0;
	}
}

