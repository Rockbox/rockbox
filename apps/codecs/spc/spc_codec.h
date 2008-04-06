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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#define NDEBUG
#include <assert.h>

/** Basic configuration options **/

#define SPC_DUAL_CORE 1

#if !defined(SPC_DUAL_CORE) || NUM_CORES == 1
#undef  SPC_DUAL_CORE
#define SPC_DUAL_CORE 0
#endif

/* TGB is the only target fast enough for gaussian and realtime BRR decode */
/* echo is almost fast enough but not quite */
#if defined(TOSHIBA_GIGABEAT_F) || defined(SIMULATOR)
    /* Don't cache BRR waves */
    #define SPC_BRRCACHE 0 
    
    /* Allow gaussian interpolation */
    #define SPC_NOINTERP 0

    /* Allow echo processing */
    #define SPC_NOECHO 0
#elif defined(CPU_COLDFIRE)
    /* Cache BRR waves */
    #define SPC_BRRCACHE 1 
    
    /* Disable gaussian interpolation */
    #define SPC_NOINTERP 1

    /* Allow echo processing */
    #define SPC_NOECHO 0
#elif defined (CPU_PP) && SPC_DUAL_CORE
    /* Cache BRR waves */
    #define SPC_BRRCACHE 1 
    
    /* Disable gaussian interpolation */
    #define SPC_NOINTERP 1

    /* Allow echo processing */
    #define SPC_NOECHO 0
#else
    /* Cache BRR waves */
    #define SPC_BRRCACHE 1 
    
    /* Disable gaussian interpolation */
    #define SPC_NOINTERP 1

    /* Disable echo processing */
    #define SPC_NOECHO 1
#endif

#ifdef CPU_ARM

#if CONFIG_CPU != PP5002
    #undef  ICODE_ATTR
    #define ICODE_ATTR

    #undef  IDATA_ATTR
    #define IDATA_ATTR

    #undef  ICONST_ATTR
    #define ICONST_ATTR

    #undef  IBSS_ATTR
    #define IBSS_ATTR
#endif

#if SPC_DUAL_CORE
    #undef SHAREDBSS_ATTR
    #define SHAREDBSS_ATTR __attribute__ ((section(".ibss")))
    #undef SHAREDDATA_ATTR
    #define SHAREDDATA_ATTR __attribute__((section(".idata")))
#endif
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
#define SET_LE16( addr, data )  set_le16( addr, data )
#define INT16A( addr ) (*(uint16_t*) (addr))
#define INT16SA( addr ) (*(int16_t*) (addr))

#ifdef ROCKBOX_LITTLE_ENDIAN
    #define GET_LE16A( addr )       (*(uint16_t*) (addr))
    #define GET_LE16SA( addr )      (*( int16_t*) (addr))
    #define SET_LE16A( addr, data ) (void) (*(uint16_t*) (addr) = (data))
#else
    #define GET_LE16A( addr )       get_le16 ( addr )
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

struct cpu_ram_t
{
    union {
        uint8_t padding1 [0x100];
        uint16_t align;
    } padding1 [1];
    uint8_t ram      [0x10000];
    uint8_t padding2 [0x100];
};

#undef RAM
#define RAM ram.ram
extern struct cpu_ram_t ram;

long CPU_run( THIS, long start_time ) ICODE_ATTR;
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

enum state_t
{  /* -1, 0, +1 allows more efficient if statements */
    state_decay   = -1,
    state_sustain = 0,
    state_attack  = +1,
    state_release = 2
};

struct cache_entry_t
{
    int16_t const* samples;
    unsigned end; /* past-the-end position */
    unsigned loop; /* number of samples in loop */
    unsigned start_addr;
};

enum { BRR_BLOCK_SIZE = 16 };
enum { BRR_CACHE_SIZE = 0x20000 + 32} ;

struct voice_t
{
#if SPC_BRRCACHE
    int16_t const* samples;
    long wave_end;
    int wave_loop;
#else
    int16_t samples [3 + BRR_BLOCK_SIZE + 1];
    int block_header; /* header byte from current block */
#endif
    uint8_t const* addr;
    short volume [2];
    long position;/* position in samples buffer, with 12-bit fraction */
    short envx;
    short env_mode;
    short env_timer;
    short key_on_delay;
};

#if SPC_BRRCACHE
/* a little extra for samples that go past end */
extern int16_t BRRcache [BRR_CACHE_SIZE];
#endif

enum { FIR_BUF_HALF = 8 };

#if defined(CPU_COLDFIRE)
/* global because of the large aligment requirement for hardware masking -
 * L-R interleaved 16-bit samples for easy loading and mac.w use.
 */
enum
{
    FIR_BUF_CNT   = FIR_BUF_HALF,
    FIR_BUF_SIZE  = FIR_BUF_CNT * sizeof ( int32_t ),
    FIR_BUF_ALIGN = FIR_BUF_SIZE * 2,
    FIR_BUF_MASK  = ~((FIR_BUF_ALIGN / 2) | (sizeof ( int32_t ) - 1))
};
#elif defined (CPU_ARM)
enum
{
    FIR_BUF_CNT   = FIR_BUF_HALF * 2 * 2,
    FIR_BUF_SIZE  = FIR_BUF_CNT * sizeof ( int32_t ),
    FIR_BUF_ALIGN = FIR_BUF_SIZE,
    FIR_BUF_MASK  = ~((FIR_BUF_ALIGN / 2) | (sizeof ( int32_t ) * 2 - 1))
};
#endif /* CPU_* */

struct Spc_Dsp
{
    union
    {
        struct raw_voice_t voice [VOICE_COUNT];
        uint8_t reg [REGISTER_COUNT];
        struct globals_t g;
        int16_t align;
    } r;
    
    unsigned echo_pos;
    int keys_down;
    int noise_count;
    uint16_t noise; /* also read as int16_t */
    
#if defined(CPU_COLDFIRE)
    /* circularly hardware masked address */
    int32_t *fir_ptr;
    /* wrapped address just behind current position -
       allows mac.w to increment and mask fir_ptr */
    int32_t *last_fir_ptr;
    /* copy of echo FIR constants as int16_t for use with mac.w */
    int16_t fir_coeff [VOICE_COUNT];
#elif defined (CPU_ARM)
   /* fir_buf [i + 8] == fir_buf [i], to avoid wrap checking in FIR code */
    int32_t *fir_ptr;
    /* copy of echo FIR constants as int32_t, for faster access */
    int32_t fir_coeff [VOICE_COUNT]; 
#else
    /* fir_buf [i + 8] == fir_buf [i], to avoid wrap checking in FIR code */
    int fir_pos; /* (0 to 7) */
    int fir_buf [FIR_BUF_HALF * 2] [2];
    /* copy of echo FIR constants as int, for faster access */
    int fir_coeff [VOICE_COUNT]; 
#endif
    
    struct voice_t voice_state [VOICE_COUNT];
    
#if SPC_BRRCACHE
    uint8_t oldsize;
    struct cache_entry_t wave_entry     [256];
    struct cache_entry_t wave_entry_old [256];
#endif
};

struct src_dir
{
    char start [2];
    char loop  [2];
};

void DSP_run_( struct Spc_Dsp* this, long count, int32_t* out_buf ) ICODE_ATTR;
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

void Timer_run_( struct Timer* t, long time ) ICODE_ATTR;

static inline void Timer_run( struct Timer* t, long time )
{
    if ( time >= t->next_tick )
        Timer_run_( t, time );
}

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
void DSP_write( struct Spc_Dsp* this, int i, int data ) ICODE_ATTR;

static inline int DSP_read( struct Spc_Dsp* this, int i )
{
    assert( (unsigned) i < REGISTER_COUNT );
    return this->r.reg [i];
}

void SPC_run_dsp_( THIS, long time ) ICODE_ATTR;

static inline void SPC_run_dsp( THIS, long time )
{
    if ( time >= this->next_dsp )
        SPC_run_dsp_( this, time );
}

int SPC_read( THIS, unsigned addr, long const time ) ICODE_ATTR;
void SPC_write( THIS, unsigned addr, int data, long const time ) ICODE_ATTR;

/**************** Sample generation ****************/
int SPC_play( THIS, long count, int32_t* out ) ICODE_ATTR;

#endif /* _SPC_CODEC_H_ */
