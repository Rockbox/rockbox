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

/* The DSP portion (awe!) */
#include "codeclib.h"
#include "spc_codec.h"
#include "spc_profiler.h"

#define CLAMP16( n ) clip_sample_16( n )

#if defined(CPU_ARM)
#if ARM_ARCH >= 6
#include "cpu/spc_dsp_armv6.c"
#elif ARM_ARCH >= 5
#include "cpu/spc_dsp_armv5.c"
#else
#include "cpu/spc_dsp_armv4.c"
#endif
#elif defined (CPU_COLDFIRE)
#include "cpu/spc_dsp_coldfire.c"
#endif

/* Above may still use generic implementations. Also defines final 
   function names. */
#include "spc_dsp_generic.c"

/* each rate divides exactly into 0x7800 without remainder */
static unsigned short const env_rates [0x20] ICONST_ATTR_SPC =
{
    0x0000, 0x000F, 0x0014, 0x0018, 0x001E, 0x0028, 0x0030, 0x003C,
    0x0050, 0x0060, 0x0078, 0x00A0, 0x00C0, 0x00F0, 0x0140, 0x0180,
    0x01E0, 0x0280, 0x0300, 0x03C0, 0x0500, 0x0600, 0x0780, 0x0A00,
    0x0C00, 0x0F00, 0x1400, 0x1800, 0x1E00, 0x2800, 0x3C00, 0x7800
};

#if !SPC_NOINTERP
/* Interleved gauss table (to improve cache coherency). */
/* gauss [i * 2 + j] = normal_gauss [(1 - j) * 256 + i] */
static int16_t gauss_table [512] IDATA_ATTR_SPC MEM_ALIGN_ATTR =
{
    370,1305, 366,1305, 362,1304, 358,1304,
    354,1304, 351,1304, 347,1304, 343,1303,
    339,1303, 336,1303, 332,1302, 328,1302,
    325,1301, 321,1300, 318,1300, 314,1299,
    311,1298, 307,1297, 304,1297, 300,1296,
    297,1295, 293,1294, 290,1293, 286,1292,
    283,1291, 280,1290, 276,1288, 273,1287,
    270,1286, 267,1284, 263,1283, 260,1282,
    257,1280, 254,1279, 251,1277, 248,1275,
    245,1274, 242,1272, 239,1270, 236,1269,
    233,1267, 230,1265, 227,1263, 224,1261,
    221,1259, 218,1257, 215,1255, 212,1253,
    210,1251, 207,1248, 204,1246, 201,1244,
    199,1241, 196,1239, 193,1237, 191,1234,
    188,1232, 186,1229, 183,1227, 180,1224,
    178,1221, 175,1219, 173,1216, 171,1213,
    168,1210, 166,1207, 163,1205, 161,1202,
    159,1199, 156,1196, 154,1193, 152,1190,
    150,1186, 147,1183, 145,1180, 143,1177,
    141,1174, 139,1170, 137,1167, 134,1164,
    132,1160, 130,1157, 128,1153, 126,1150,
    124,1146, 122,1143, 120,1139, 118,1136,
    117,1132, 115,1128, 113,1125, 111,1121,
    109,1117, 107,1113, 106,1109, 104,1106,
    102,1102, 100,1098,  99,1094,  97,1090,
     95,1086,  94,1082,  92,1078,  90,1074,
     89,1070,  87,1066,  86,1061,  84,1057,
     83,1053,  81,1049,  80,1045,  78,1040,
     77,1036,  76,1032,  74,1027,  73,1023,
     71,1019,  70,1014,  69,1010,  67,1005,
     66,1001,  65, 997,  64, 992,  62, 988,
     61, 983,  60, 978,  59, 974,  58, 969,
     56, 965,  55, 960,  54, 955,  53, 951,
     52, 946,  51, 941,  50, 937,  49, 932,
     48, 927,  47, 923,  46, 918,  45, 913,
     44, 908,  43, 904,  42, 899,  41, 894,
     40, 889,  39, 884,  38, 880,  37, 875,
     36, 870,  36, 865,  35, 860,  34, 855,
     33, 851,  32, 846,  32, 841,  31, 836,
     30, 831,  29, 826,  29, 821,  28, 816,
     27, 811,  27, 806,  26, 802,  25, 797,
     24, 792,  24, 787,  23, 782,  23, 777,
     22, 772,  21, 767,  21, 762,  20, 757,
     20, 752,  19, 747,  19, 742,  18, 737,
     17, 732,  17, 728,  16, 723,  16, 718,
     15, 713,  15, 708,  15, 703,  14, 698,
     14, 693,  13, 688,  13, 683,  12, 678,
     12, 674,  11, 669,  11, 664,  11, 659,
     10, 654,  10, 649,  10, 644,   9, 640,
      9, 635,   9, 630,   8, 625,   8, 620,
      8, 615,   7, 611,   7, 606,   7, 601,
      6, 596,   6, 592,   6, 587,   6, 582,
      5, 577,   5, 573,   5, 568,   5, 563,
      4, 559,   4, 554,   4, 550,   4, 545,
      4, 540,   3, 536,   3, 531,   3, 527,
      3, 522,   3, 517,   2, 513,   2, 508,
      2, 504,   2, 499,   2, 495,   2, 491,
      2, 486,   1, 482,   1, 477,   1, 473,
      1, 469,   1, 464,   1, 460,   1, 456,
      1, 451,   1, 447,   1, 443,   1, 439,
      0, 434,   0, 430,   0, 426,   0, 422,
      0, 418,   0, 414,   0, 410,   0, 405,
      0, 401,   0, 397,   0, 393,   0, 389,
      0, 385,   0, 381,   0, 378,   0, 374,
};
#endif /* !SPC_NOINTERP */

void DSP_write( struct Spc_Dsp* this, int i, int data )
{
    assert( (unsigned) i < REGISTER_COUNT );
    
    this->r.reg [i] = data;
    int high = i >> 4;
    int low  = i & 0x0F;
    if ( low < 2 ) /* voice volumes */
    {
        int left  = *(int8_t const*) &this->r.reg [i & ~1];
        int right = *(int8_t const*) &this->r.reg [i |  1];
        struct voice_t* v = this->voice_state + high;
        v->volume [0] = left;
        v->volume [1] = right;
    }
    else if ( low < 4 ) /* voice rates */
    {
        struct voice_t* v = this->voice_state + high;
        v->rate = GET_LE16A( this->r.voice[high].rate ) & 0x3fff;
    }
#if !SPC_NOECHO
    else if ( low == 0x0F ) /* fir coefficients */
    {
        this->fir.coeff [7 - high] = (int8_t) data; /* sign-extend */
    }
#endif /* !SPC_NOECHO */
}

/* Decode BRR block */
static inline void
decode_brr_block( struct voice_t* voice, unsigned start_addr, int16_t* out )
{
    /* header */
    start_addr += 9; /* point to next header */
    uint8_t const* addr = ram.ram + start_addr;
    unsigned block_header = addr[-9];
    voice->wave.block_header = block_header;
    voice->wave.start_addr = start_addr;

    /* previous samples */
    int smp2 = out [0];
    int smp1 = out [1];

    int offset = -BRR_BLOCK_SIZE * 4;

#if !SPC_BRRCACHE
    out [-(BRR_BLOCK_SIZE + 1)] = out [-1];

    /* if next block has end flag set,
       this block ends early (verified) */
    if ( (block_header & 3) != 3 && (*addr & 3) == 1 )
    {
        /* arrange for last 9 samples to be skipped */
        int const skip = 9;
        out [skip - (BRR_BLOCK_SIZE + 1)] = out [-1];
        out += (skip & 1);
        voice->wave.position += skip * 0x1000;
        offset = (-BRR_BLOCK_SIZE + (skip & ~1)) * 4;
        addr -= skip / 2;
        /* force sample to end on next decode */
        voice->wave.block_header = 1;
    }
#endif /* !SPC_BRRCACHE */

    int const filter = block_header & 0x0c;
    int const scale = block_header >> 4;

    if ( filter == 0x08 ) /* filter 2 (30-90% of the time) */
    {
        /* y[n] = x[n] + 61/32 * y[n-1] - 15/16 * y[n-2] */
        do /* decode and filter 16 samples */
        {
            /* Get nybble, sign-extend, then scale
               get byte, select which nybble, sign-extend, then shift
               based on scaling. */
            int delta = (int8_t)(addr [offset >> 3] << (offset & 4)) >> 4;
            delta = (delta << scale) >> 1;

            if (scale > 0xc)
                delta = (delta >> 17) << 11;

            out [offset >> 2] = smp2;

            delta -= smp2 >> 1;
            delta += smp2 >> 5;
            delta += smp1;
            delta += (-smp1 - (smp1 >> 1)) >> 5;

            delta = CLAMP16( delta );
            smp2 = smp1;
            smp1 = (int16_t) (delta * 2); /* sign-extend */
        }
        while ( (offset += 4) != 0 );
    }
    else if ( filter == 0x04 ) /* filter 1 */
    {
        /* y[n] = x[n] + 15/16 * y[n-1] */
        do /* decode and filter 16 samples */
        {
            /* Get nybble, sign-extend, then scale
               get byte, select which nybble, sign-extend, then shift
               based on scaling. */
            int delta = (int8_t)(addr [offset >> 3] << (offset & 4)) >> 4;
            delta = (delta << scale) >> 1;

            if (scale > 0xc)
                delta = (delta >> 17) << 11;

            out [offset >> 2] = smp2;

            delta += smp1 >> 1;
            delta += (-smp1) >> 5;

            delta = CLAMP16( delta );
            smp2 = smp1;
            smp1 = (int16_t) (delta * 2); /* sign-extend */
        }
        while ( (offset += 4) != 0 );
    }
    else if ( filter == 0x0c ) /* filter 3 */
    {
        /* y[n] = x[n] + 115/64 * y[n-1] - 13/16 * y[n-2] */
        do /* decode and filter 16 samples */
        {
            /* Get nybble, sign-extend, then scale
               get byte, select which nybble, sign-extend, then shift
               based on scaling. */
            int delta = (int8_t)(addr [offset >> 3] << (offset & 4)) >> 4;
            delta = (delta << scale) >> 1;

            if (scale > 0xc)
                delta = (delta >> 17) << 11;

            out [offset >> 2] = smp2;

            delta -= smp2 >> 1;
            delta += (smp2 + (smp2 >> 1)) >> 4;
            delta += smp1;
            delta += (-smp1 * 13) >> 7;

            delta = CLAMP16( delta );
            smp2 = smp1;
            smp1 = (int16_t) (delta * 2); /* sign-extend */
        }
        while ( (offset += 4) != 0 );
    }
    else /* filter 0 */
    {
        /* y[n] = x[n] */
        do /* decode and filter 16 samples */
        {
            /* Get nybble, sign-extend, then scale
               get byte, select which nybble, sign-extend, then shift
               based on scaling. */
            int delta = (int8_t)(addr [offset >> 3] << (offset & 4)) >> 4;
            delta = (delta << scale) >> 1;

            if (scale > 0xc)
                delta = (delta >> 17) << 11;

            out [offset >> 2] = smp2;

            smp2 = smp1;
            smp1 = delta * 2;
        }
        while ( (offset += 4) != 0 );
    }

#if SPC_BRRCACHE
    if ( !(block_header & 1) )
    {
        /* save to end of next block (for next call) */
        out [BRR_BLOCK_SIZE    ] = smp2;
        out [BRR_BLOCK_SIZE + 1] = smp1;
    }
    else
#endif /* SPC_BRRCACHE */
    {
        /* save to end of this block */
        out [0] = smp2;
        out [1] = smp1;
    }
}

#if SPC_BRRCACHE
static void NO_INLINE ICODE_ATTR_SPC
brr_decode_cache( struct Spc_Dsp* this, struct src_dir const* sd,
                  unsigned start_addr, struct voice_t* voice,
                  unsigned waveform, bool initial_point )
{
    /* a little extra for samples that go past end */
    static int16_t BRRcache [BRR_CACHE_SIZE] CACHEALIGN_ATTR;

    DEBUGF( "decode at %04x (wave #%u)\n", start_addr, waveform );

    struct cache_entry_t* const wave_entry = &this->wave_entry [waveform];
    wave_entry->start_addr = start_addr;

    unsigned const loop_addr = letoh16( sd [waveform].loop );
    int16_t* loop_start = NULL;

    DEBUGF( "loop addr at %04x (wave #%u)\n", loop_addr, waveform );

    int16_t* out = BRRcache + start_addr * 2;
    wave_entry->samples = out;

    /* BRR filter uses previous samples */
    if ( initial_point )
    {
        /* initialize filters */
        out [BRR_BLOCK_SIZE + 1] = 0;
        out [BRR_BLOCK_SIZE + 2] = 0;
    }

    *out++ = 0;

    unsigned block_header;

    do
    {
        if ( start_addr == loop_addr )
        {
            loop_start = out;
            DEBUGF( "loop found at %04x (wave #%u)\n", start_addr, waveform );
        }

        /* output position - preincrement */
        out += BRR_BLOCK_SIZE;

        decode_brr_block( voice, start_addr, out );

        block_header = voice->wave.block_header;
        start_addr = voice->wave.start_addr;

        /* if next block has end flag set, this block ends early */
        /* (verified) */
        if ( (block_header & 3) != 3 && (start_addr[ram.ram] & 3) == 1 )
        { 
            /* skip last 9 samples */
            DEBUGF( "block early end (wave #%u)\n", waveform );
            out -= 9;
            break;
        }
    }
    while ( !(block_header & 1) && start_addr < 0x10000 );

    wave_entry->end          = (out - 1 - wave_entry->samples) << 12;
    wave_entry->loop         = 0;
    wave_entry->loop_addr    = 0;
    wave_entry->block_header = block_header;

    if ( (block_header & 2) )
    {
        if ( loop_start )
        {
            wave_entry->loop = out - loop_start;
            wave_entry->end += 0x3000;

            out [2] = loop_start [2];
            out [3] = loop_start [3];
            out [4] = loop_start [4];
        }
        else
        {
            DEBUGF( "loop point outside initial wave\n" );
            /* Plan filter init for later decoding at loop point */
            int16_t* next = BRRcache + loop_addr * 2;
            next [BRR_BLOCK_SIZE + 1] = out [0];
            next [BRR_BLOCK_SIZE + 2] = out [1];
        }

        wave_entry->loop_addr = loop_addr;
    }

    DEBUGF( "end at %04x (wave #%u)\n\n", start_addr, waveform );

    /* add to cache */
    this->wave_entry_old [this->oldsize++] = *wave_entry;
}

/* see if in cache */
static inline bool
brr_probe_cache( struct Spc_Dsp* this, unsigned start_addr,
                 struct cache_entry_t* wave_entry )
{
    if ( wave_entry->start_addr == start_addr )
        return true;

    for ( unsigned i = 0; i < this->oldsize; i++ )
    {
        struct cache_entry_t* e = &this->wave_entry_old [i];

        if ( e->start_addr == start_addr )
        {
#if 0 /* do NOT want to see all the key down stuff for cached waves */
            DEBUGF( "found in wave_entry_old (oldsize=%u)\n",
                    this->oldsize );
#endif
            *wave_entry = *e;
            return true; /* Wave in cache */
        }
    }

    return false;
}

static NO_INLINE ICODE_ATTR_SPC void
brr_key_on( struct Spc_Dsp* this,  struct src_dir const* sd,
            struct voice_t* voice, struct raw_voice_t const* raw_voice,
            unsigned start_addr )
{
    bool initial_point = false;
    unsigned waveform = raw_voice->waveform;

    if (start_addr == (unsigned)-1)
    {
        initial_point = true;
        start_addr = letoh16( sd [waveform].start );
    }

    struct cache_entry_t* const wave_entry = &this->wave_entry [waveform];

    /* predecode BRR if not already */
    if ( !brr_probe_cache( this, start_addr, wave_entry ) )
    {
        /* actually decode it */
        brr_decode_cache( this, sd, start_addr, voice, waveform,
                          initial_point );
    }

    voice->wave.position     = 3 * 0x1000 - 1; /* 0x2fff */
    voice->wave.samples      = wave_entry->samples;
    voice->wave.end          = wave_entry->end;
    voice->wave.loop         = wave_entry->loop;
    voice->wave.start_addr   = wave_entry->start_addr;
    voice->wave.loop_addr    = wave_entry->loop_addr;
    voice->wave.block_header = wave_entry->block_header;
}

static inline int brr_decode( struct Spc_Dsp* this, struct src_dir const* sd,
                              struct voice_t* voice,
                              struct raw_voice_t const* raw_voice )
{
    if ( voice->wave.position < voice->wave.end )
        return 0;

    long loop_len = voice->wave.loop << 12;

    if ( !loop_len )
    {
        if ( !(voice->wave.block_header & 2 ) )
            return 2;

        /* "Loop" is outside initial waveform */
        brr_key_on( this, sd, voice, raw_voice, voice->wave.loop_addr );
    }

    voice->wave.position -= loop_len;
    return 1;

    (void)sd; (void)raw_voice;
}

#else /* !SPC_BRRCACHE */

static inline void
brr_key_on( struct Spc_Dsp* this,  struct src_dir const* sd,
            struct voice_t* voice, struct raw_voice_t const* raw_voice,
            unsigned start_addr )
{
    voice->wave.start_addr = letoh16( sd [raw_voice->waveform].start );
    /* BRR filter uses previous samples */
    voice->wave.samples [BRR_BLOCK_SIZE + 1] = 0;
    voice->wave.samples [BRR_BLOCK_SIZE + 2] = 0;
    /* force decode on next brr_decode call */
    voice->wave.position = (BRR_BLOCK_SIZE + 3) * 0x1000 - 1; /* 0x12fff */
    voice->wave.block_header = 0; /* "previous" BRR header */
    (void)this; (void)start_addr;
}

static inline int brr_decode( struct Spc_Dsp* this, struct src_dir const* sd,
                              struct voice_t* voice,
                              struct raw_voice_t const* raw_voice )
{
    if ( voice->wave.position < BRR_BLOCK_SIZE * 0x1000 )
        return 0;

    voice->wave.position -= BRR_BLOCK_SIZE * 0x1000;

    unsigned start_addr = voice->wave.start_addr;

    if ( start_addr >= 0x10000 )
        start_addr -= 0x10000;

    unsigned block_header = voice->wave.block_header;

    /* action based on previous block's header */
    int dec = 0;

    if ( block_header & 1 )
    {
        start_addr = letoh16( sd [raw_voice->waveform].loop );
        dec = 1;

        if ( !(block_header & 2) ) /* 1% of the time */
        {
            /* first block was end block;
               don't play anything (verified) */
            return 2;
        }
    }

    decode_brr_block( voice, start_addr,
                      &voice->wave.samples [1 + BRR_BLOCK_SIZE] );

    return dec;
    (void)this;
}
#endif /* SPC_BRRCACHE */

static void NO_INLINE ICODE_ATTR_SPC
key_on( struct Spc_Dsp* const this, struct voice_t* const voice,
        struct src_dir const* const sd,
        struct raw_voice_t const* const raw_voice,
        const int key_on_delay, const int vbit )
{
    voice->key_on_delay = key_on_delay;

    if ( key_on_delay == 0 )
    {
        this->keys_down |= vbit;
        voice->envx      = 0;
        voice->env_mode  = state_attack;
        voice->env_timer = ENV_RATE_INIT; /* TODO: inaccurate? */
        brr_key_on( this, sd, voice, raw_voice, -1 );
    }
}

void DSP_run_( struct Spc_Dsp* this, long count, int32_t* out_buf )
{
    #undef RAM
#if defined(CPU_ARM) && !SPC_BRRCACHE
    uint8_t* const ram_ = ram.ram;
    #define RAM ram_
#else
    #define RAM ram.ram
#endif
    EXIT_TIMER(cpu);
    ENTER_TIMER(dsp);

    /* Here we check for keys on/off.  Docs say that successive writes
       to KON/KOF must be separated by at least 2 Ts periods or risk
       being neglected.  Therefore DSP only looks at these during an
       update, and not at the time of the write.  Only need to do this
       once however, since the regs haven't changed over the whole
       period we need to catch up with. */
    
    {
        int key_ons  = this->r.g.key_ons;
        int key_offs = this->r.g.key_offs;
        /* keying on a voice resets that bit in ENDX */
        this->r.g.wave_ended &= ~key_ons;
        /* key_off bits prevent key_on from being acknowledged */
        this->r.g.key_ons = key_ons & key_offs;
        
        /* process key events outside loop, since they won't re-occur */
        struct voice_t* voice = this->voice_state + 8;
        int vbit = 0x80;
        do
        {
            --voice;
            if ( key_offs & vbit )
            {
                voice->env_mode     = state_release;
                voice->key_on_delay = 0;
            }
            else if ( key_ons & vbit )
            {
                voice->key_on_delay = 8;
            }
        }
        while ( (vbit >>= 1) != 0 );
    }

    struct src_dir const* const sd = 
        &ram.sd [this->r.g.wave_page * 0x100/sizeof(struct src_dir)];

#if !SPC_NOINTERP
    int const slow_gaussian = (this->r.g.pitch_mods >> 1) |
                              this->r.g.noise_enables;
#endif
#if !SPC_NOECHO
    int const echo_start = this->r.g.echo_page * 0x100;
    int const echo_delay = (this->r.g.echo_delay & 15) * 0x800;
#endif
    /* (g.flags & 0x40) ? 30 : 14 */
    int const global_muting = ((this->r.g.flags & 0x40) >> 2) + 14 - 8; 
    int const global_vol_0  = this->r.g.volume_0;
    int const global_vol_1  = this->r.g.volume_1;
    
    do /* one pair of output samples per iteration */
    {
        /* Noise */
        if ( this->r.g.noise_enables )
        {
            this->noise_count -= env_rates [this->r.g.flags & 0x1F];

            if ( this->noise_count <= 0 )
            {
                this->noise_count = ENV_RATE_INIT;
                int feedback = (this->noise << 13) ^ (this->noise << 14);
                this->noise = (feedback & 0x8000) ^ (this->noise >> 1 & ~1);
            }
        }
        
    #if !SPC_NOECHO
        int echo_0 = 0, echo_1 = 0;
    #endif /* !SPC_NOECHO */
        long prev_outx = 0; /* TODO: correct value for first channel? */
        int chans_0 = 0, chans_1 = 0;

        /* TODO: put raw_voice pointer in voice_t? */
        struct raw_voice_t * raw_voice = this->r.voice;
        struct voice_t* voice = this->voice_state;

        for (int vbit = 1; vbit < 0x100; vbit <<= 1, ++voice, ++raw_voice )
        {
            /* pregen involves checking keyon, etc */
            ENTER_TIMER(dsp_pregen);
            
            /* Key on events are delayed */
            int key_on_delay = voice->key_on_delay;

            if ( UNLIKELY ( --key_on_delay >= 0 ) ) /* <1% of the time */
                key_on( this, voice, sd, raw_voice, key_on_delay, vbit );
            
            if ( !(this->keys_down & vbit) ) /* Silent channel */
            {
            silent_chan:
                raw_voice->envx = 0;
                raw_voice->outx = 0;
                prev_outx = 0;
                continue;
            }
            
            /* Envelope */
            {
                int const ENV_RANGE = 0x800;
                int env_mode = voice->env_mode;
                int adsr0 = raw_voice->adsr [0];
                int env_timer;
                if ( LIKELY ( env_mode != state_release ) ) /* 99% of the time */
                {
                    env_timer = voice->env_timer;
                    if ( LIKELY ( adsr0 & 0x80 ) ) /* 79% of the time */
                    {
                        int adsr1 = raw_voice->adsr [1];
                        if ( LIKELY ( env_mode == state_sustain ) ) /* 74% of the time */
                        {
                            if ( (env_timer -= env_rates [adsr1 & 0x1F]) > 0 )
                                goto write_env_timer;
                            
                            int envx = voice->envx;
                            envx--; /* envx *= 255 / 256 */
                            envx -= envx >> 8;
                            voice->envx = envx;
                            /* TODO: should this be 8? */
                            raw_voice->envx = envx >> 4;
                            goto init_env_timer;
                        }
                        else if ( env_mode < 0 ) /* 25% state_decay */
                        {
                            int envx = voice->envx;
                            if ( (env_timer -=
                                env_rates [(adsr0 >> 3 & 0x0E) + 0x10]) <= 0 )
                            {
                                envx--; /* envx *= 255 / 256 */
                                envx -= envx >> 8;
                                voice->envx = envx;
                                /* TODO: should this be 8? */
                                raw_voice->envx = envx >> 4;
                                env_timer = ENV_RATE_INIT;
                            }
                            
                            int sustain_level = adsr1 >> 5;
                            if ( envx <= (sustain_level + 1) * 0x100 )
                                voice->env_mode = state_sustain;
                            
                            goto write_env_timer;
                        }
                        else /* state_attack */
                        {
                            int t = adsr0 & 0x0F;
                            if ( (env_timer -= env_rates [t * 2 + 1]) > 0 )
                                goto write_env_timer;
                            
                            int envx = voice->envx;
                            
                            int const step = ENV_RANGE / 64;
                            envx += step;
                            if ( t == 15 )
                                envx += ENV_RANGE / 2 - step;
                            
                            if ( envx >= ENV_RANGE )
                            {
                                envx = ENV_RANGE - 1;
                                voice->env_mode = state_decay;
                            }
                            voice->envx = envx;
                            /* TODO: should this be 8? */
                            raw_voice->envx = envx >> 4;
                            goto init_env_timer;
                        }
                    }
                    else /* gain mode */
                    {
                        int t = raw_voice->gain;
                        if ( t < 0x80 )
                        {
                            raw_voice->envx = t;
                            voice->envx = t << 4;
                            goto env_end;
                        }
                        else
                        {
                            if ( (env_timer -= env_rates [t & 0x1F]) > 0 )
                                goto write_env_timer;
                            
                            int envx = voice->envx;
                            int mode = t >> 5;
                            if ( mode <= 5 ) /* decay */
                            {
                                int step = ENV_RANGE / 64;
                                if ( mode == 5 ) /* exponential */
                                {
                                    envx--; /* envx *= 255 / 256 */
                                    step = envx >> 8;
                                }
                                if ( (envx -= step) < 0 )
                                {
                                    envx = 0;
                                    if ( voice->env_mode == state_attack )
                                        voice->env_mode = state_decay;
                                }
                            }
                            else /* attack */
                            {
                                int const step = ENV_RANGE / 64;
                                envx += step;
                                if ( mode == 7 &&
                                     envx >= ENV_RANGE * 3 / 4 + step )
                                    envx += ENV_RANGE / 256 - step;
                                
                                if ( envx >= ENV_RANGE )
                                    envx = ENV_RANGE - 1;
                            }
                            voice->envx = envx;
                            /* TODO: should this be 8? */
                            raw_voice->envx = envx >> 4;
                            goto init_env_timer;
                        }
                    }
                }
                else /* state_release */
                {
                    int envx = voice->envx;
                    if ( (envx -= ENV_RANGE / 256) > 0 )
                    {
                        voice->envx = envx;
                        raw_voice->envx = envx >> 8;
                        goto env_end;
                    }
                    else
                    {
                        /* bit was set, so this clears it */
                        this->keys_down ^= vbit;
                        voice->envx = 0;
                        goto silent_chan;
                    }
                }
            init_env_timer:
                env_timer = ENV_RATE_INIT;
            write_env_timer:
                voice->env_timer = env_timer;
            env_end:;
            }

            EXIT_TIMER(dsp_pregen);
            
            ENTER_TIMER(dsp_gen);

            switch ( brr_decode( this, sd, voice, raw_voice ) )
            {
            case 2:
                /* bit was set, so this clears it */
                this->keys_down ^= vbit; 

                /* since voice->envx is 0,
                   samples and position don't matter */
                raw_voice->envx = 0;
                voice->envx = 0;
            case 1:
                this->r.g.wave_ended |= vbit;
            }

            /* Get rate (with possible modulation) */
            int rate = voice->rate;
            if ( this->r.g.pitch_mods & vbit )
                rate = (rate * (prev_outx + 32768)) >> 15;

            uint32_t position = voice->wave.position;
            voice->wave.position += rate;

            int output;
            int amp_0, amp_1;

        #if !SPC_NOINTERP
            /* Gaussian interpolation using most recent 4 samples */
           
            /* Only left half of gaussian kernel is in table, so we must mirror
               for right half */
            int offset = ( position >> 4 ) & 0xFF;
            int16_t const* fwd = gauss_table       + offset * 2;
            int16_t const* rev = gauss_table + 510 - offset * 2;
            
            /* Use faster gaussian interpolation when exact result isn't needed
               by pitch modulator of next channel */
            if ( LIKELY ( !(slow_gaussian & vbit) ) ) /* 99% of the time */
            {
                /* Main optimization is lack of clamping. Not a problem since
                   output never goes more than +/- 16 outside 16-bit range and
                   things are clamped later anyway. Other optimization is to
                   preserve fractional accuracy, eliminating several masks. */
                output = gaussian_fast_interp( voice->wave.samples, position,
                                               fwd, rev );
                output = gaussian_fast_amp( voice, output, &amp_0, &amp_1 );
            }
            else /* slow gaussian */
        #endif /* !SPC_NOINTERP (else two-point linear interpolation) */
            {
                output = *(int16_t *)&this->noise;

                if ( !(this->r.g.noise_enables & vbit) )
                    output = interp( voice->wave.samples, position, fwd, rev );

                /* Apply envelope and volume */
                output = apply_amp( voice, output, &amp_0, &amp_1 );
            }

            prev_outx = output;
            raw_voice->outx = output >> 8;
        
            EXIT_TIMER(dsp_gen);
            
            ENTER_TIMER(dsp_mix);

            chans_0 += amp_0;
            chans_1 += amp_1;
        #if !SPC_NOECHO
            if ( this->r.g.echo_ons & vbit )
            {
                echo_0 += amp_0;
                echo_1 += amp_1;
            }
        #endif /* !SPC_NOECHO */

            EXIT_TIMER(dsp_mix);
        }
        /* end of voice loop */
        
        /* Generate output */
        int amp_0, amp_1;
    #if !SPC_NOECHO
        /* Read feedback from echo buffer */
        int echo_pos = this->echo_pos;
        uint8_t* const echo_ptr = RAM + ((echo_start + echo_pos) & 0xFFFF);

        echo_pos += 4;

        if ( echo_pos >= echo_delay )
            echo_pos = 0;

        this->echo_pos = echo_pos;

        /* Apply FIR */
        int fb_0, fb_1;
        echo_apply( this, echo_ptr, &fb_0, &fb_1 );

        if ( !(this->r.g.flags & 0x20) )
        {
            /* Feedback into echo buffer */
            echo_feedback( this, echo_ptr, echo_0, echo_1, fb_0, fb_1 );
        }
    #endif /* !SPC_NOECHO */

        mix_output( this, global_muting, global_vol_0, global_vol_1, 
                    chans_0, chans_1, fb_0, fb_1, &amp_0, &amp_1 );

        out_buf [             0] = amp_0;
        out_buf [WAV_CHUNK_SIZE] = amp_1;
        out_buf ++;
    }
    while ( --count );

    EXIT_TIMER(dsp);
    ENTER_TIMER(cpu);
}

void DSP_reset( struct Spc_Dsp* this )
{
    this->keys_down   = 0;
    this->noise_count = 0;
    this->noise       = 2;
    
    this->r.g.flags   = 0xE0; /* reset, mute, echo off */
    this->r.g.key_ons = 0;
    
    ci->memset( this->voice_state, 0, sizeof this->voice_state );
    
    for ( int i = VOICE_COUNT; --i >= 0; )
    {
        struct voice_t* v = this->voice_state + i;
        v->env_mode = state_release;
        v->wave.start_addr = 0;
    }
    
#if SPC_BRRCACHE
    this->oldsize = 0;
    for ( int i = 0; i < 256; i++ )
        this->wave_entry [i].start_addr = 0xffff;
#endif /* SPC_BRRCACHE */

#if !SPC_NOINTERP && GAUSS_TABLE_SCALE
    if (gauss_table[0] == 370)
    {
        /* Not yet scaled */
        for ( int i = 0; i < 512; i++)
            gauss_table[i] <<= GAUSS_TABLE_SCALE;
    }
#endif /* !SPC_NOINTERP && GAUSS_TABLE_SCALE */

#if !SPC_NOECHO
    this->echo_pos = 0;
    echo_init(this);
#endif /* SPC_NOECHO */

    assert( offsetof (struct globals_t,unused9 [2]) == REGISTER_COUNT );
    assert( sizeof (this->r.voice) == REGISTER_COUNT );
}
