/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2008 Michael Sevakis (jhMikeS)
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
 * Copyright (C) 2004-2007 Shay Green (blargg)
 * Copyright (C) 2002 Brad Martin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* lovingly ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */
/* DSP Based on Brad Martin's OpenSPC DSP emulator */
/* tag reading from sexyspc by John Brawn (John_Brawn@yahoo.com) and others */

#ifndef _SPC_CODEC_H_
#define _SPC_CODEC_H_

/* rather than comment out asserts, just define NDEBUG */
#ifndef NDEBUG
#define NDEBUG
#endif
#include <assert.h>

/** Basic configuration options **/

#ifndef ARM_ARCH
#define ARM_ARCH 0
#endif

#if NUM_CORES == 1
#define SPC_DUAL_CORE 0
#else
#define SPC_DUAL_CORE 1
#endif

/* Only some targets are too slow for gaussian and realtime BRR decode */
#if defined(CPU_COLDFIRE)
    /* Cache BRR waves */
    #define SPC_BRRCACHE 1 
    /* Disable gaussian interpolation */
    #define SPC_NOINTERP 1
#elif defined (CPU_PP)
    /* Cache BRR waves */
    #define SPC_BRRCACHE 1 
    /* Disable gaussian interpolation */
    #define SPC_NOINTERP 1
#if !SPC_DUAL_CORE
    /* Disable echo processing */
    #define SPC_NOECHO 1
#endif
#endif /* CPU_* */

/** Turn on, by default, all the good stuff **/
#ifndef SPC_BRRCACHE
    /* Don't cache BRR waves */
    #define SPC_BRRCACHE 0
#endif
#ifndef SPC_NOINTERP
    /* Allow gaussian interpolation */
    #define SPC_NOINTERP 0
#endif
#ifndef SPC_NOECHO
    /* Allow echo processing */
    #define SPC_NOECHO 0
#endif

#if   (CONFIG_CPU == MCF5250)
#define IBSS_ATTR_SPC               IBSS_ATTR
#define ICODE_ATTR_SPC              ICODE_ATTR
#define ICONST_ATTR_SPC             ICONST_ATTR
#define IDATA_ATTR_SPC              IDATA_ATTR
/* Not enough IRAM available to move further data to it. */
#define IBSS_ATTR_SPC_LARGE_IRAM

#elif (CONFIG_CPU == PP5020)
/* spc is slower on PP5020 when moving data to IRAM. */
#define IBSS_ATTR_SPC
#define ICODE_ATTR_SPC
#define ICONST_ATTR_SPC
#define IDATA_ATTR_SPC
/* Not enough IRAM available to move further data to it. */
#define IBSS_ATTR_SPC_LARGE_IRAM

#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024) || (CONFIG_CPU == S5L8702)
#define IBSS_ATTR_SPC               IBSS_ATTR
#define ICODE_ATTR_SPC              ICODE_ATTR
#define ICONST_ATTR_SPC             ICONST_ATTR
#define IDATA_ATTR_SPC              IDATA_ATTR
/* Not enough IRAM available to move further data to it. */
#define IBSS_ATTR_SPC_LARGE_IRAM

#elif (CONFIG_CPU == S5L8700) || (CONFIG_CPU == S5L8701)
#define IBSS_ATTR_SPC               IBSS_ATTR
#define ICODE_ATTR_SPC              ICODE_ATTR
#define ICONST_ATTR_SPC             ICONST_ATTR
#define IDATA_ATTR_SPC              IDATA_ATTR
/* Very large IRAM. Move even more data to it. */
#define IBSS_ATTR_SPC_LARGE_IRAM    IBSS_ATTR

#else
#define IBSS_ATTR_SPC               IBSS_ATTR
#define ICODE_ATTR_SPC              ICODE_ATTR
#define ICONST_ATTR_SPC             ICONST_ATTR
#define IDATA_ATTR_SPC              IDATA_ATTR
/* Not enough IRAM available to move further data to it. */
#define IBSS_ATTR_SPC_LARGE_IRAM
#endif

#if SPC_DUAL_CORE
    #undef SHAREDBSS_ATTR
    #define SHAREDBSS_ATTR __attribute__ ((section(".ibss")))
    #undef SHAREDDATA_ATTR
    #define SHAREDDATA_ATTR __attribute__((section(".idata")))
#endif

/* Samples per channel per iteration */
#if defined(CPU_PP) && NUM_CORES == 1
#define WAV_CHUNK_SIZE 2048
#else
#define WAV_CHUNK_SIZE 1024
#endif

/**************** Little-endian handling ****************/

static inline unsigned get_le16( void const* p )
{
    return  ((unsigned char const*) p) [1] * 0x100u +
            ((unsigned char const*) p) [0];
}

static inline int get_le16s( void const* p )
{
    return  ((signed char const*) p) [1] * 0x100 +
            ((unsigned char const*) p) [0];
}

static inline void set_le16( void* p, unsigned n )
{
    ((unsigned char*) p) [1] = (unsigned char) (n >> 8);
    ((unsigned char*) p) [0] = (unsigned char) n;
}

#define GET_LE16( addr )        get_le16( addr )
#define GET_LE16A( addr )       get_le16( addr )
#define SET_LE16( addr, data )  set_le16( addr, data )
#define INT16A( addr ) (*(uint16_t*) (addr))
#define INT16SA( addr ) (*(int16_t*) (addr))

#ifdef ROCKBOX_LITTLE_ENDIAN
    #define GET_LE16SA( addr )      (*( int16_t*) (addr))
    #define SET_LE16A( addr, data ) (void) (*(uint16_t*) (addr) = (data))
#else
    #define GET_LE16SA( addr )      get_le16s( addr )
    #define SET_LE16A( addr, data ) set_le16 ( addr, data )
#endif

struct Spc_Emu;
#define THIS struct Spc_Emu* const this

/* The CPU portion (shock!) */

struct cpu_regs_t
{
    long pc; /* more than 16 bits to allow overflow detection */
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t status;
    uint8_t sp;
};

struct src_dir
{
    uint16_t start;
    uint16_t loop;
};

struct cpu_ram_t
{
    union {
        uint8_t padding1 [0x100];
        uint16_t align;
    } padding1 [1];
    union {
        uint8_t ram       [0x10000];
        struct src_dir sd [0x10000/sizeof(struct src_dir)];
    };
    uint8_t padding2 [0x100];
};

#undef RAM
#define RAM ram.ram
extern struct cpu_ram_t ram;

long CPU_run( THIS, long start_time )
    ICODE_ATTR_SPC;

void CPU_Init( THIS );

/* The DSP portion (awe!) */
enum { VOICE_COUNT = 8 };
enum { REGISTER_COUNT = 128 };

struct raw_voice_t
{
    int8_t  volume [2];
    uint8_t rate [2];
    uint8_t waveform;
    uint8_t adsr [2];   /* envelope rates for attack, decay, and sustain */
    uint8_t gain;       /* envelope gain (if not using ADSR) */
    int8_t  envx;       /* current envelope level */
    int8_t  outx;       /* current sample */
    int8_t  unused [6];
};
    
struct globals_t
{
    int8_t  unused1 [12];
    int8_t  volume_0;         /* 0C   Main Volume Left (-.7) */
    int8_t  echo_feedback;    /* 0D   Echo Feedback (-.7) */
    int8_t  unused2 [14];
    int8_t  volume_1;         /* 1C   Main Volume Right (-.7) */
    int8_t  unused3 [15];
    int8_t  echo_volume_0;    /* 2C   Echo Volume Left (-.7) */
    uint8_t pitch_mods;       /* 2D   Pitch Modulation on/off for each voice */
    int8_t  unused4 [14];
    int8_t  echo_volume_1;    /* 3C   Echo Volume Right (-.7) */
    uint8_t noise_enables;    /* 3D   Noise output on/off for each voice */
    int8_t  unused5 [14];
    uint8_t key_ons;          /* 4C   Key On for each voice */
    uint8_t echo_ons;         /* 4D   Echo on/off for each voice */
    int8_t  unused6 [14];
    uint8_t key_offs;         /* 5C   key off for each voice
                                      (instantiates release mode) */
    uint8_t wave_page;        /* 5D   source directory (wave table offsets) */
    int8_t  unused7 [14];
    uint8_t flags;            /* 6C   flags and noise freq */
    uint8_t echo_page;        /* 6D */
    int8_t  unused8 [14];
    uint8_t wave_ended;       /* 7C */
    uint8_t echo_delay;       /* 7D   ms >> 4 */
    char    unused9 [2];
};

enum { ENV_RATE_INIT = 0x7800 };
enum state_t
{  /* -1, 0, +1 allows more efficient if statements */
    state_decay   = -1,
    state_sustain = 0,
    state_attack  = +1,
    state_release = 2
};

enum { BRR_BLOCK_SIZE = 16 };

#if SPC_BRRCACHE
struct cache_entry_t
{
    int16_t const* samples; /* decoded samples (cached) */
    unsigned end;          /* past-the-end position (cached) */
    unsigned loop;         /* number of samples in loop (cached) */
    uint16_t start_addr;   /* RAM start address */
    uint16_t loop_addr;    /* RAM loop address */
    uint8_t  block_header; /* final wave block header */
};

enum { BRR_CACHE_SIZE = 0x20000 + 32};

struct voice_wave_t
{
    int16_t const* samples; /* decoded samples in cache */
    long position;         /* position in samples buffer, 12-bit frac */
    long end;              /* end position in samples buffer */
    int loop;              /* length of looping area */
    unsigned block_header; /* header byte from current BRR block */
    unsigned start_addr;   /* BRR waveform address in RAM */
    unsigned loop_addr;    /* Loop address in RAM */
};
#else /* !SPC_BRRCACHE */
struct voice_wave_t
{
    int16_t samples [3 + BRR_BLOCK_SIZE + 1]; /* last decoded block */
    int32_t position;      /* position in samples buffer, 12-bit frac */
    unsigned block_header; /* header byte from current BRR block */
    unsigned start_addr;   /* BRR waveform address in RAM */
};
#endif /* SPC_BRRCACHE */

struct voice_t
{
    struct voice_wave_t wave;
    short volume [2];
    short envx;
    short env_mode;
    short env_timer;
    short key_on_delay;
    short rate;
};

#if !SPC_NOECHO
enum { FIR_BUF_HALF = 8 };
#endif

struct Spc_Dsp;

/* These must go before the definition of struct Spc_Dsp because a
   definition of struct echo_filter is required. Only declarations
   are created unless SPC_DSP_C is defined before including these. */
#if defined(CPU_ARM)
#if ARM_ARCH >= 6
#include "cpu/spc_dsp_armv6.h"
#elif ARM_ARCH >= 5
#include "cpu/spc_dsp_armv5.h"
#else
#include "cpu/spc_dsp_armv4.h"
#endif
#elif defined (CPU_COLDFIRE)
#include "cpu/spc_dsp_coldfire.h"
#endif

/* Above may still use generic implementations. Also defines final 
   function names. */
#include "spc_dsp_generic.h"

#if !SPC_NOINTERP && !defined (GAUSS_TABLE_SCALE)
#define GAUSS_TABLE_SCALE 0
#endif

struct Spc_Dsp
{
    union
    {
        struct raw_voice_t voice [VOICE_COUNT];
        uint8_t reg [REGISTER_COUNT];
        struct globals_t g;
        int16_t align;
    } r;
    
    int keys_down;
    int noise_count;
    uint16_t noise; /* also read as int16_t */
    struct voice_t voice_state [VOICE_COUNT];

#if !SPC_NOECHO
    unsigned echo_pos;
    struct echo_filter fir;
#endif /* !SPC_NOECHO */
    
#if SPC_BRRCACHE
    unsigned oldsize;
    struct cache_entry_t wave_entry     [256];
    struct cache_entry_t wave_entry_old [256];
#endif
};

void DSP_run_( struct Spc_Dsp* this, long count, int32_t* out_buf )
    ICODE_ATTR_SPC;

void DSP_reset( struct Spc_Dsp* this );

static inline void DSP_run( struct Spc_Dsp* this, long count, int32_t* out )
{
    /* Should we just fill the buffer with silence? Flags won't be cleared */
    /* during this run so it seems it should keep resetting every sample. */
    if ( this->r.g.flags & 0x80 )
        DSP_reset( this );
    
    DSP_run_( this, count, out );
}

/**************** SPC emulator ****************/
/* 1.024 MHz clock / 32000 samples per second */
enum { CLOCKS_PER_SAMPLE = 32 }; 

enum { EXTRA_CLOCKS = CLOCKS_PER_SAMPLE / 2 };

/* using this disables timer (since this will always be in the future) */
enum { TIMER_DISABLED_TIME = 127 };

enum { ROM_SIZE = 64 };
enum { ROM_ADDR = 0xFFC0 };

enum { TIMER_COUNT = 3 };

struct Timer
{
    long next_tick;
    int period;
    int count;
    int shift;
    int enabled;
    int counter;
};

struct Spc_Emu
{
    uint8_t cycle_table [0x100];
    struct cpu_regs_t r;
    
    int32_t* sample_buf;
    long next_dsp;
    int rom_enabled;
    int extra_cycles;
    
    struct Timer timer [TIMER_COUNT];
    
    /* large objects at end */
    struct Spc_Dsp dsp;
    uint8_t extra_ram [ROM_SIZE];
    uint8_t boot_rom  [ROM_SIZE];
};

enum { SPC_FILE_SIZE = 0x10180 };

struct spc_file_t
{
    char    signature [27];
    char    unused [10];
    uint8_t pc [2];
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t status;
    uint8_t sp;
    char    unused2 [212];
    uint8_t ram [0x10000];
    uint8_t dsp [128];
    uint8_t ipl_rom [128];
};

void SPC_Init( THIS );

int SPC_load_spc( THIS, const void* data, long size );

/**************** DSP interaction ****************/
void DSP_write( struct Spc_Dsp* this, int i, int data )
    ICODE_ATTR_SPC;

static inline int DSP_read( struct Spc_Dsp* this, int i )
{
    assert( (unsigned) i < REGISTER_COUNT );
    return this->r.reg [i];
}

int SPC_read( THIS, unsigned addr, long const time )
    ICODE_ATTR_SPC;

void SPC_write( THIS, unsigned addr, int data, long const time )
    ICODE_ATTR_SPC;

/**************** Sample generation ****************/
int SPC_play( THIS, long count, int32_t* out )
    ICODE_ATTR_SPC;

#endif /* _SPC_CODEC_H_ */
