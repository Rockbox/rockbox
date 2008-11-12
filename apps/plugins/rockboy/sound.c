/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * The following code is rewrite of the C++ code provided in VisualBoyAdvance.
 * There are also portions of the original GNUboy code.
 * Copyright (C) 2001 the GNUboy development team
 *
 * VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
 * Copyright (C) 1999-2003 Forgotten
 * Copyright (C) 2004 Forgotten and the VBA development team
 *
 * VisualBoyAdvance conversion from C++ to C by Karl Kurbjun July, 2007
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 */

#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"
#include "sound.h"
#include "cpu-gb.h"
#include "hw.h"
#include "regs.h"

static const byte soundWavePattern[4][32] = {
  {0x01,0x01,0x01,0x01,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff},
  {0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff},
  {0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff},
  {0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0x01,0x01,0x01,0x01,
   0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff}
};

int soundFreqRatio[8] ICONST_ATTR= {
  1048576, // 0
  524288,  // 1
  262144,  // 2
  174763,  // 3
  131072,  // 4
  104858,  // 5
  87381,   // 6
  74898    // 7
};

int soundShiftClock[16] ICONST_ATTR= {
      2, // 0
      4, // 1
      8, // 2
     16, // 3
     32, // 4
     64, // 5
    128, // 6
    256, // 7
    512, // 8
   1024, // 9
   2048, // 10
   4096, // 11
   8192, // 12
  16384, // 13
  1,     // 14
  1      // 15
};

struct snd snd IBSS_ATTR;

#define RATE (snd.rate)
#define WAVE (ram.hi+0x30)
#define S1 (snd.ch[0])
#define S2 (snd.ch[1])
#define S3 (snd.ch[2])
#define S4 (snd.ch[3])

#define SOUND_MAGIC   0x60000000
#define SOUND_MAGIC_2 0x30000000
#define NOISE_MAGIC 5

static void gbSoundChannel1(int *r, int *l)
{
    int vol = S1.envol;

    int freq = 0;

    int value = 0;

    if(S1.on && (S1.len || !S1.cont))
    {
        S1.pos += snd.quality*S1.skip;
        S1.pos &= 0x1fffffff;

        value = ((signed char)S1.wave[S1.pos>>24]) * vol;
    }

    if (snd.balance & 1) *r += value;
    if (snd.balance & 16) *l += value;

    if(S1.on)
    {
        if(S1.len) 
        {
            S1.len-=snd.quality;

            if(S1.len <=0 && S1.cont) 
            {
                R_NR52 &= 0xfe;
                S1.on = 0;
            }
        }

        if(S1.enlen)
        {
            S1.enlen-=snd.quality;

            if(S1.enlen<=0) 
            {
                if(S1.endir)
                {
                    if(S1.envol < 15)
                        S1.envol++;
                }
                else 
                {
                    if(S1.envol)
                        S1.envol--;
                }

                S1.enlen += S1.enlenreload;
            }
        }

        if(S1.swlen)
        {
            S1.swlen-=snd.quality;

            if(S1.swlen<=0)
            {
                freq = (((int)(R_NR14&7) << 8) | R_NR13);

                int updown = 1;
        
                if(S1.swdir)
                    updown = -1;

                int newfreq = 0;
                if(S1.swsteps)
                {
                    newfreq = freq + updown * freq / (1 << S1.swsteps);
                    if(newfreq == freq)
                        newfreq = 0;
                }
                else
                    newfreq = freq;

                if(newfreq < 0)
                {
                    S1.swlen += S1.swlenreload;
                }
                else if(newfreq > 2047)
                {
                    S1.swlen = 0;
                    S1.on = 0;
                    R_NR52 &= 0xfe;
                } 
                else 
                {
                    S1.swlen += S1.swlenreload;
                    S1.skip = SOUND_MAGIC/(2048 - newfreq);

                    R_NR13 = newfreq & 0xff;
                    R_NR14 = (R_NR14 & 0xf8) |((newfreq >> 8) & 7);
                }
            }
        }
    }
}

static void gbSoundChannel2(int *r, int *l)
{
    int vol = S2.envol;
  
    int value = 0;
    
    if(S2.on && (S2.len || !S2.cont))
    {
        S2.pos += snd.quality*S2.skip;
        S2.pos &= 0x1fffffff;
    
        value = ((signed char)S2.wave[S2.pos>>24]) * vol;
    }

    if (snd.balance & 2) *r += value;
    if (snd.balance & 32) *l += value;

    if(S2.on) {
        if(S2.len) {
            S2.len-=snd.quality;
            
            if(S2.len <= 0 && S2.cont) {
                R_NR52 &= 0xfd;
                S2.on = 0;
            }
        }
        
        if(S2.enlen) {
            S2.enlen-=snd.quality;
          
            if(S2.enlen <= 0) {
                if(S2.endir) {
                    if(S2.envol < 15)
                        S2.envol++;
                } else {
                    if(S2.envol)
                        S2.envol--;
                }
                S2.enlen += S2.enlenreload;
            }
        }
    }
}

static void gbSoundChannel3(int *r, int *l)
{
    int s;
    if (S3.on && (S3.len || !S3.cont))
    {
        S3.pos += S3.skip*snd.quality;
        S3.pos &= 0x1fffffff;
        s=ram.hi[0x30 + (S3.pos>>25)];
        if (S3.pos & 0x01000000)
            s &= 0x0f;
        else
            s >>= 4;

        s -= 8;

        switch(S3.outputlevel)
        {
            case 0:
                s=0;
                break;
            case 1:
                break;
            case 2:
                s=s>>1;
                break;
            case 3:
                s=s>>2;
                break;
        }

        if (snd.balance & 4) *r += s;
        if (snd.balance & 64) *l += s;
    }

    if(S3.on)
    {
        if(S3.len)
        {
            S3.len-=snd.quality;
            if(S3.len<=0 && S3.cont)
            {
                R_NR52 &= 0xFB;
                S3.on=0;
            }
        }
    }
}

static void gbSoundChannel4(int *r, int *l)
{
    int vol = S4.envol;
  
    int value = 0;
  
    if(S4.clock <= 0x0c)
    {
        if(S4.on && (S4.len || !S4.cont))
        {
            S4.pos += snd.quality*S4.skip;
            S4.shiftpos += snd.quality*S4.shiftskip;
      
            if(S4.nsteps)
            {
                while(S4.shiftpos > 0x1fffff) {
                    S4.shiftright = (((S4.shiftright << 6) ^
                        (S4.shiftright << 5)) & 0x40) | (S4.shiftright >> 1);
                    S4.shiftpos -= 0x200000;
                }
            } 
            else 
            {
                while(S4.shiftpos > 0x1fffff)
                {
                    S4.shiftright = (((S4.shiftright << 14) ^
                        (S4.shiftright << 13)) & 0x4000) | (S4.shiftright >> 1);
                    S4.shiftpos -= 0x200000;
                }
            }
      
            S4.pos &= 0x1fffff;
            S4.shiftpos &= 0x1fffff;
          
            value = ((S4.shiftright & 1)*2-1) * vol;
        } 
        else
        {
            value = 0;
        }
    }
  
    if (snd.balance & 8) *r += value;
    if (snd.balance & 128) *l += value;
  
    if(S4.on) {
        if(S4.len) {
            S4.len-=snd.quality;
            
            if(S4.len <= 0 && S4.cont) {
                R_NR52 &= 0xfd;
                S4.on = 0;
            }
        }
        
        if(S4.enlen) {
            S4.enlen-=snd.quality;
            
            if(S4.enlen <= 0)
            {
                if(S4.endir)
                {
                    if(S4.envol < 15)
                        S4.envol++;
                } 
                else 
                {
                    if(S4.envol)
                        S4.envol--;
                }
                S4.enlen += S4.enlenreload;
            }
        }
    }
}

void sound_mix(void)
{
    int l, r;

    if (!RATE || cpu.snd < RATE) return;

    for (; cpu.snd >= RATE; cpu.snd -= RATE)
    {
        l = r = 0;

        gbSoundChannel1(&r,&l);

        gbSoundChannel2(&r,&l);

        gbSoundChannel3(&r,&l);

        gbSoundChannel4(&r,&l);

        if(snd.gbDigitalSound)
        {
            l = snd.level1<<8;
            r = snd.level2<<8;
        }
        else
        {
            l *= snd.level1*60;
            r *= snd.level2*60;
        }

        if(l > 32767)
            l = 32767;
        if(l < -32768)
            l = -32768;
            
        if(r > 32767)
            r = 32767;
        if(r < -32768)
            r = -32768;

        if (pcm.buf)
        {
            if (pcm.pos >= pcm.len)
                pcm_submit();
            if (pcm.stereo)
            {
                pcm.buf[pcm.pos++] = l;
                pcm.buf[pcm.pos++] = r;
            }
            else pcm.buf[pcm.pos++] = ((l+r)>>1);
        }
    }
    R_NR52 = (R_NR52&0xf0) | S1.on | (S2.on<<1) | (S3.on<<2) | (S4.on<<3);
}

byte sound_read(byte r)
{
    if(!options.sound) return 0;
    sound_mix();
    /* printf("read %02X: %02X\n", r, REG(r)); */
    return REG(r);
}

void sound_write(byte r, byte b)
{
    int freq=0;
    ram.hi[r]=b;

    if(!options.sound)
        return;

    sound_mix();

    switch (r)
    {
    case RI_NR10:
        S1.swlen = S1.swlenreload = 344 * ((b >> 4) & 7);
        S1.swsteps = b & 7;
        S1.swdir = b & 0x08;
        S1.swstep = 0;
        break;
    case RI_NR11:
        S1.len = 172 * (64 - (b & 0x3f));
        S1.wave = soundWavePattern[b >> 6];
        break;
    case RI_NR12:
        S1.envol = b >> 4;
        S1.endir = b & 0x08;
        S1.enlenreload = S1.enlen = 689 * (b & 7);
        break;
    case RI_NR13:
        freq = (((int)(R_NR14 & 7)) << 8) | b;
        S1.len = 172 * (64 - (R_NR11 & 0x3f));
        freq = 2048 - freq;
        if(freq)
        {
            S1.skip = SOUND_MAGIC / freq;
        }
        else
        {
            S1.skip = 0;
        }
        break;
    case RI_NR14:
        freq = (((int)(b&7) << 8) | R_NR13);
        freq = 2048 - freq;
        S1.len = 172 * (64 - (R_NR11 & 0x3f));
        S1.cont = b & 0x40;
        if(freq) 
        {
            S1.skip = SOUND_MAGIC / freq;
        } 
        else
        {
            S1.skip = 0;
        }
        if(b & 0x80)
        {
            R_NR52 |= 1;
            S1.envol = R_NR12 >> 4;
            S1.endir = R_NR12 & 0x08;
            S1.len = 172 * (64 - (R_NR11 & 0x3f));
            S1.enlenreload = S1.enlen = 689 * (R_NR12 & 7);
            S1.swlen = S1.swlenreload = 344 * ((R_NR10 >> 4) & 7);
            S1.swsteps = R_NR10 & 7;
            S1.swdir = R_NR10 & 0x08;
            S1.swstep = 0;
  
            S1.pos = 0;
            S1.on = 1;
        }
        break;
    case RI_NR21:
        S2.wave = soundWavePattern[b >> 6];
        S2.len = 172 * (64 - (b & 0x3f));
        break;
    case RI_NR22:
        S2.envol = b >> 4;
        S2.endir = b & 0x08;
        S2.enlenreload = S2.enlen = 689 * (b & 7);
        break;
    case RI_NR23:
        freq = (((int)(R_NR24 & 7)) << 8) | b;
        S2.len = 172 * (64 - (R_NR21 & 0x3f));
        freq = 2048 - freq;
        if(freq)
        {
            S2.skip = SOUND_MAGIC / freq;
        } 
        else
        {
            S2.skip = 0;
        }
        break;
    case RI_NR24:
        freq = (((int)(b&7) << 8) | R_NR23);
        freq = 2048 - freq;
        S2.len = 172 * (64 - (R_NR21 & 0x3f));
        S2.cont = b & 0x40;
        if(freq) {
            S2.skip = SOUND_MAGIC / freq;
        } else
            S2.skip = 0;
        if(b & 0x80) {
            R_NR52 |= 2;
            S2.envol = R_NR22 >> 4;
            S2.endir = R_NR22 & 0x08;
            S2.len = 172 * (64 - (R_NR21 & 0x3f));
            S2.enlenreload = S2.enlen = 689 * (R_NR22 & 7);

            S2.pos = 0;
            S2.on = 1;
        }
        break;
    case RI_NR30:
        if (!(b & 0x80)){
            R_NR52 &= 0xfb;
            S3.on = 0;
        }
        break;
    case RI_NR31:
        S3.len = (256-R_NR31) * 172;
        break;
    case RI_NR32:
        S3.outputlevel = (b >> 5) & 3;
        break;
    case RI_NR33:
        freq = 2048 - (((int)(R_NR34&7) << 8) | b);
        if(freq)
            S3.skip = SOUND_MAGIC_2 / freq;
        else
            S3.skip = 0;
        break;
    case RI_NR34:
        freq = 2048 - (((b&7)<<8) | R_NR33);

        if(freq)
            S3.skip = SOUND_MAGIC_2 / freq;
        else
            S3.skip = 0;

        S3.cont=b & 0x40;
        if((b & 0x80) && (R_NR30 & 0x80))
        {
            R_NR52 |= 4;
            S3.len = 172 * (256 - R_NR31);
            S3.pos = 0;
            S3.on = 1;
        }
        break;
    case RI_NR41:
        S4.len = 172 * (64 - (b & 0x3f));
        break;
    case RI_NR42:
        S4.envol = b >> 4;
        S4.endir = b & 0x08;
        S4.enlenreload = S4.enlen = 689 * (b & 7);
        break;
    case RI_NR43:
        freq = soundFreqRatio[b & 7];

        S4.nsteps = b & 0x08;
        S4.skip = (freq << 8) / NOISE_MAGIC;
        S4.clock = b >> 4;

        freq = freq / soundShiftClock[S4.clock];
        S4.shiftskip = (freq << 8) / NOISE_MAGIC;
        break;
    case RI_NR44:
        S4.cont = b & 0x40;
        if(b & 0x80)
        {
            R_NR52 |= 8;
            S4.envol = R_NR42 >> 4;
            S4.endir = R_NR42 & 0x08;
            S4.len = 172 * (64 - (R_NR41 & 0x3f));
            S4.enlenreload = S4.enlen = 689 * (R_NR42 & 7);

            S4.on = 1;
            
            S4.pos = 0;
            S4.shiftpos = 0;
            
            freq = soundFreqRatio[R_NR43 & 7];
      
            S4.shiftpos = (freq << 8) / NOISE_MAGIC;

            S4.nsteps = R_NR43 & 0x08;
            
            freq = freq / soundShiftClock[R_NR43 >> 4];
      
            S4.shiftskip = (freq << 8) / NOISE_MAGIC;
            if(S4.nsteps)
            {
                S4.shiftright = 0x7fff;
            }
            else
            {
                S4.shiftright = 0x7f;
            }
        }
        break;
    case RI_NR50:
        snd.level1 = b & 7;
        snd.level2 = (b >> 4) & 7;
        break;
    case RI_NR51:
        snd.balance = b;
        break;
    case RI_NR52:
        if (!(b & 0x80))
        {
            S1.on=0;
            S2.on=0;
            S3.on=0;
            S4.on=0;
        }
        break;
    }
    
    snd.gbDigitalSound = true;

    if(S1.on && S1.envol != 0)
        snd.gbDigitalSound = false;
    if(S2.on && S2.envol != 0)
        snd.gbDigitalSound = false;
    if(S3.on && S3.outputlevel != 0)
        snd.gbDigitalSound = false;
    if(S4.on && S4.envol != 0)
        snd.gbDigitalSound = false;
}

void sound_reset(void)
{
    snd.level1 = 7;
    snd.level2 = 7;
    S1.on           = S2.on     = S3.on     = S4.on = 0;
    S1.len          = S2.len    = S3.len    = S4.len = 0;
    S1.skip         = S2.skip   = S3.skip   = S4.skip = 0;
    S1.pos          = S2.pos    = S3.pos    = S4.pos = 0;
    S1.cont         = S2.cont   = S3.cont   = S4.cont = 0;
    S1.envol        = S2.envol              = S4.envol = 0;
    S1.enlen        = S2.enlen              = S4.enlen = 0;
    S1.endir        = S2.endir              = S4.endir = 0;
    S1.enlenreload  = S2.enlenreload        = S4.enlenreload = 0;
    S1.swlen = 0;
    S1.swlenreload = 0;
    S1.swsteps = 0;
    S1.swdir = 0;
    S1.swstep = 0;
    S1.wave         = S2.wave = soundWavePattern[2];

    S3.outputlevel = 0;

    S4.clock = 0;
    S4.shiftright = 0x7f;
    S4.nsteps = 0;

    sound_write(0x10, 0x80);
    sound_write(0x11, 0xbf);
    sound_write(0x12, 0xf3);
    sound_write(0x14, 0xbf);
    sound_write(0x16, 0x3f);
    sound_write(0x17, 0x00);
    sound_write(0x19, 0xbf);
  
    sound_write(0x1a, 0x7f);
    sound_write(0x1b, 0xff);
    sound_write(0x1c, 0xbf);
    sound_write(0x1e, 0xbf);
  
    sound_write(0x20, 0xff);
    sound_write(0x21, 0x00);
    sound_write(0x22, 0x00);
    sound_write(0x23, 0xbf);
    sound_write(0x24, 0x77);
    sound_write(0x25, 0xf3);
  
    sound_write(0x26, 0xf0);
  
    S1.on = 0;
    S2.on = 0;
    S3.on = 0;
    S4.on = 0;

    int addr = 0x30;
	while(addr < 0x40) 
    {
        ram.hi[addr++] = 0x00;
        ram.hi[addr++] = 0xff;
    }

	if (pcm.hz)
    {
        snd.rate = (1<<21) / pcm.hz;
        snd.quality=44100 / pcm.hz;
    }
	else snd.rate = 0;
}

void sound_dirty(void)
{
    sound_write(RI_NR10, R_NR10);
    sound_write(RI_NR11, R_NR11);
    sound_write(RI_NR12, R_NR12);
    sound_write(RI_NR13, R_NR13);
    sound_write(RI_NR14, R_NR14);
    
    sound_write(RI_NR21, R_NR21);
    sound_write(RI_NR22, R_NR22);
    sound_write(RI_NR23, R_NR23);
    sound_write(RI_NR24, R_NR24);
    
    sound_write(RI_NR30, R_NR30);
    sound_write(RI_NR31, R_NR31);
    sound_write(RI_NR32, R_NR32);
    sound_write(RI_NR33, R_NR33);
    sound_write(RI_NR34, R_NR34);
    
    sound_write(RI_NR42, R_NR42);
    sound_write(RI_NR43, R_NR43);
    sound_write(RI_NR44, R_NR44);
    
    sound_write(RI_NR50, R_NR50);
    sound_write(RI_NR51, R_NR51);
    sound_write(RI_NR52, R_NR52);
}
