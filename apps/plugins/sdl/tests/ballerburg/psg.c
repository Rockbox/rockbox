/*
 * libpsg - a simple YM-2149 soundchip emulation library.
 *
 * Copyright (C) 2000  Stefan Berndtsson (NoCrew)
 * Copyright (C) 2011  Thomas Huth
 * 
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <SDL.h>

#define M_PI 3.1415926535897932384626433832

#include "psg.h"


psg psg_struct;

//#define FIXED_MATH 1

#ifdef FIXED_MATH
#  define FIXED_TYPE int
#  define FIXED(x) ((int)((x) * 256))
#  define DIV(x, y) ((((x) + ((y) >> 1)) << 8) / (y))
#  define MOD(x, y) ((x) % (y))
#  define FIXED_TO_INT(x) (((x) + 128) >> 8)
#else
#  define FIXED_TYPE double
#  define FIXED(x) x
#  define DIV(x, y) ((x) / (y))
#  define MOD(x, y) fmod (x, y)
#  define FIXED_TO_INT(x) (x)
#endif

#define INLINE static __inline__

#define TYPE_SQUARE 0

#if 0 /* undefined at the moment */
#  define TYPE_SINUS 1
#  define TYPE_PSGSQR 2
#endif

#define MFP_FREQUENCY 2457600
#define PSG_FREQUENCY 2000000
#define QUALITY (1<<14)
#define SAMPLE_TYPE TYPE_SQUARE
#define SAMPLE_MAX 0x7fff
#define SAMPLE_MIN 0x8000
#define NOISE_MAX 0xffff

struct frame
{
	int wave_period[3];
	int rfreq[3];
	int rvol[3];
	int tone[3];
	int noise[3];
	int env[3];
	int env_mode;
	int env_period;
	double env_freq;
	int sample_freq;
};


#define ONE FIXED (1)
#define PI FIXED (M_PI)

INLINE FIXED_TYPE env_wave_00XX(FIXED_TYPE in)
{
	if(in < PI)
		return ONE - DIV (in, PI);
	else
		return 0;
}

INLINE FIXED_TYPE env_wave_01XX(FIXED_TYPE in)
{
	if(in < PI)
		return DIV (in, PI);
	else
		return 0;
}

INLINE FIXED_TYPE env_wave_1000(FIXED_TYPE in)
{
	return ONE - DIV (MOD (in, PI), PI);
}

INLINE FIXED_TYPE env_wave_1001(FIXED_TYPE in)
{
	if(in < PI)
		return ONE - DIV (in, PI);
	else
		return 0;
}

INLINE FIXED_TYPE env_wave_1010(FIXED_TYPE in)
{
	FIXED_TYPE tmp;
	tmp = MOD (in, PI*2);
	if(tmp < PI)
		return ONE - DIV (tmp, PI);
	else
		return DIV (tmp - PI, PI);
}

INLINE FIXED_TYPE env_wave_1011(FIXED_TYPE in)
{
	if(in < PI)
		return ONE - DIV (in, PI);
	else
		return ONE;
}

INLINE FIXED_TYPE env_wave_1100(FIXED_TYPE in)
{
	return DIV (MOD (in, PI), PI);
}

INLINE FIXED_TYPE env_wave_1101(FIXED_TYPE in)
{
	if(in < PI)
		return DIV (in, PI);
	else
		return ONE;
}

INLINE FIXED_TYPE env_wave_1110(FIXED_TYPE in)
{
	FIXED_TYPE tmp;
	tmp = MOD (in, PI*2);
	if(tmp < PI)
		return DIV (tmp, PI);
	else
		return ONE - DIV (tmp - PI, PI);
}

INLINE FIXED_TYPE env_wave_1111(FIXED_TYPE in)
{
	if(in < PI)
		return DIV (in, PI);
	else
		return ONE;
}

static FIXED_TYPE env_wave(FIXED_TYPE in, int mode)
{
	switch(mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		return env_wave_00XX(in);
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		return env_wave_01XX(in);
		break;
	case 8:
		return env_wave_1000(in);
		break;
	case 9:
		return env_wave_1001(in);
		break;
	case 10:
		return env_wave_1010(in);
		break;
	case 11:
		return env_wave_1011(in);
		break;
	case 12:
		return env_wave_1100(in);
		break;
	case 13:
		return env_wave_1101(in);
		break;
	case 14:
		return env_wave_1110(in);
		break;
	case 15:
		return env_wave_1111(in);
		break;
	}
	return 0;
}



/* Period counter for seamless frames */
struct frame tdata;

/* Values stolen from STonX */
static int amps[16]= {0,1,1,2,3,4,5,7,12,20,28,44,70,110,165,255};

static int16_t _clip(int input)
{
	if(input < -32768) return (int16_t)-32768;
	if(input > 32767) return (int16_t) 32767;
	return input;
}

psg *psg_init(int mode, int bits, int freq)
{
	int i;

	if((bits != 16))
	{
		fprintf(stderr, "Unsupported bitvalue\n");
		return (psg *)NULL;
	}

	if((mode < 0) || (mode > 1))
	{
		fprintf(stderr, "Unsupported mode\n");
		return (psg *)NULL;
	}

	if(freq != 44100 && freq != 22050)
	{
		fprintf(stderr, "Unsupported frequency\n");
		return (psg *)NULL;
	}

	psg_struct.output.mode = mode;
	psg_struct.output.bits = bits;
	psg_struct.output.freq = freq;
	tdata.sample_freq = freq;

	for(i=0; i<3; i++)
	{
		tdata.wave_period[i] = 0;
	}

	return &psg_struct;
}

static
int16_t _get_one_tonesample(int offset)
{
	if((offset&(QUALITY-1)) < (QUALITY/2))
		return SAMPLE_MAX;
	else
		return SAMPLE_MIN;
}

static
int16_t _get_one_noisesample(int offset)
{
	return (NOISE_MAX&rand())-SAMPLE_MAX;
}

static
int _get_real_freq(int psgfreq, int sample_freq)
{
	int denominator;

	if(psgfreq)
	{
		denominator = sample_freq * psgfreq;
		return (QUALITY *(PSG_FREQUENCY/16.0) + (denominator >> 1)) / denominator;
	}
	else
		return 0;
}

static void psg_update_values(psg *psg)
{
	int i, tmp;

	for(i=0; i<3; i++)
	{
		if(psg->regs[8+i] == 16)
		{
			tdata.rvol[i] = 0;
			tdata.env[i] = 1;
			if(psg->regs[13] != 0xff)
			{
				//tdata.env_period = 0;
				tdata.env_mode = psg->regs[13];
			}
		}
		else
		{
			tdata.rvol[i] = psg->regs[8+i];
			tdata.env[i] = 0;
		}
		tmp = psg->regs[11] + (psg->regs[12] << 8);
		if(tmp)
			tdata.env_freq = ((double)PSG_FREQUENCY/(256.0 * tmp));
		else
			tdata.env_freq = 0;
		tdata.rfreq[i] = _get_real_freq((psg->regs[1+i*2]<<8) + psg->regs[0+i*2], psg->output.freq);
		tdata.tone[i] = !(psg->regs[7] & (1<<i));
		tdata.noise[i] = !(psg->regs[7] & (8<<i));
	}
}


static
int16_t _get_one_sample(void)
{
	int i;
	int32_t tmp;

	tmp = 0;

	for(i=0; i<3; i++)
	{
		// printf("%i: noise=%i tone=%i env=%i\n",i,tdata.noise[i], tdata.tone[i], tdata.env[i]);
		if(tdata.tone[i] && !tdata.noise[i])
		{
			if(tdata.env[i])
			{
				tmp = _clip(tmp + ((_get_one_tonesample(tdata.wave_period[i])*
				                    amps[(int)
				                         (15*env_wave(tdata.env_period*M_PI*2/QUALITY,
				                                      tdata.env_mode))]>>8)));
			}
			else
			{
				tmp = _clip(tmp + ((_get_one_tonesample(tdata.wave_period[i])*
				                    amps[tdata.rvol[i]])>>8));
			}
			tdata.wave_period[i] += tdata.rfreq[i];
		}
		if(tdata.noise[i] && !tdata.tone[i])
		{
			if(tdata.env[i])
			{
				tmp = _clip(tmp + ((_get_one_noisesample(tdata.wave_period[i])*
				                    amps[(int)
				                         (15*env_wave(tdata.env_period*M_PI*2/QUALITY,
				                                      tdata.env_mode))]>>8)));
			}
			else
			{
				tmp = _clip(tmp + ((_get_one_noisesample(tdata.wave_period[i])*
				                    amps[tdata.rvol[i]])>>8));
			}
			//      tdata.wave_period[i] += tdata.rfreq[i];
		}
		if(tdata.noise[i] && tdata.tone[i])
		{
			if(tdata.env[i])
			{
				tmp = _clip(tmp + ((_clip(_get_one_tonesample(tdata.wave_period[i])+
				                          _get_one_noisesample(tdata.wave_period[i]))*
				                    amps[(int)
				                         (15*env_wave(tdata.env_period*M_PI*2/QUALITY,
				                                      tdata.env_mode))]>>8)));
			}
			else
			{
				tmp = _clip(tmp + (_clip(_get_one_tonesample(tdata.wave_period[i])+
				                         _get_one_noisesample(tdata.wave_period[i]))*
				                   amps[tdata.rvol[i]]>>8));
			}
			tdata.wave_period[i] += tdata.rfreq[i];
		}
	}

	if(tdata.env_freq)
		tdata.env_period += 2*(QUALITY/(tdata.sample_freq * tdata.env_freq));

	return tmp;
}


int psg_create_samples(void *buffer, psg *psg, int samples)
{
	int16_t *tmp;
	int i, tval;

	tmp = (int16_t *) buffer;

	for(i=0; i<samples; i++)
	{
		tval = _get_one_sample();
		*tmp++ = tval;
		if(psg->output.mode)
			*tmp++ = tval;
	}


	return 0;
}


/**
 * SDL audio callback function - copy emulation sound to audio system.
 */
static void Audio_CallBack(void *userdata, Uint8 *stream, int len)
{
	psg_create_samples(stream, &psg_struct, len/4);
}


unsigned int Giaccess(unsigned int data, unsigned int reg)
{
	if (reg & 0x80)
	{
		psg_struct.regs[reg&0x0f] = data;
		psg_update_values(&psg_struct);
		if ((reg&0x0f) == 13)
			tdata.env_period = 0;
		return 0;
	}
	else
	{
		return psg_struct.regs[reg&0x0f];
	}
}


void Dosound(const unsigned char *buf)
{
	int i = 0;
	while (buf[i] != 0xff)
	{
		if (buf[i] >= 16)
		{
			printf("Dosound: Unsupported opcode 0x%02x\n", buf[i]);
		}
		else
		{
			psg_struct.regs[buf[i]] = buf[i+1];
			if (buf[i] == 13)
				tdata.env_period = 0;
		}
		i += 2;
	}

	psg_update_values(&psg_struct);
}


/**
 * Initialize the audio subsystem. Return true if all OK.
 */
void psg_audio_init(void)
{
	SDL_AudioSpec desiredAudioSpec;    /* We fill in the desired SDL audio options here */

	/* Init the SDL's audio subsystem: */
	if (SDL_WasInit(SDL_INIT_AUDIO) == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		{
			fprintf(stderr, "Could not init audio: %s\n", SDL_GetError() );
			return;
		}
	}

	psg_init(1, 16, 44100);

	/* Set up SDL audio: */
	desiredAudioSpec.freq = 22050;
	desiredAudioSpec.format = AUDIO_S16SYS;		/* 16-Bit signed */
	desiredAudioSpec.channels = 2;			/* stereo */
	desiredAudioSpec.callback = Audio_CallBack;
	desiredAudioSpec.userdata = NULL;
	desiredAudioSpec.samples = 512;			/* buffer size in samples */

	if (SDL_OpenAudio(&desiredAudioSpec, NULL))	/* Open audio device */
	{
		fprintf(stderr, "Can't use audio: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return;
	}

	SDL_PauseAudio(0);
}
