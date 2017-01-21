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

#ifndef PSG_H
#define PSG_H

#define SND_MONO 0
#define SND_STEREO 1

struct sndconfig {
  int mode;
  int bits;
  int freq;
};

struct spsg {
  int regs[15];
  int env_kept;
  struct sndconfig output;
};

typedef struct spsg psg;

/**
 * PSG *psg_init(int mode, int bits, int freq)
 * Action:
 *  setup all internal variables.
 * Parameters:
 *  mode - mono/stereo
 *  bits - 8/16
 *  freq - frequency for audio output
 * Return:
 *  psg - empty struct for you to use
 */
psg *psg_init(int, int, int);

/**
 * psg_create_samples(void *output_buffer,
 *                    psg *psg_struct,
 *                    int usec);
 * Action:
 *  builds a sample of length 'usec' into 'output_buffer'
 *  from 'psg_struct'
 * Parameters:
 *  buffer - allocated output buffer of suitable size
 *  psg_struct - psg as returned from psg_init()
 *  samples - amount of samples to produce
 * Return:
 *  int - 0 if things go ok
 */
int psg_create_samples(void *buffer, psg *psgstruct, int samples);

void psg_audio_init(void);
unsigned int Giaccess(unsigned int data, unsigned int reg);
void Dosound(const unsigned char *buf);

#endif /* PSG_H */
