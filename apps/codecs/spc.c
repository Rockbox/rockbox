/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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

#include "codeclib.h"
#include "inttypes.h"
#include "system.h"

/* rather than comment out asserts, just define NDEBUG */
#define NDEBUG
#include <assert.h>
#undef check
#define check assert

CODEC_HEADER

#ifdef CPU_ARM
    #undef  ICODE_ATTR
    #define ICODE_ATTR

    #undef  IDATA_ATTR
    #define IDATA_ATTR
#endif

/* TGB is the only target fast enough for gaussian and realtime BRR decode */
/* echo is almost fast enough but not quite */
#ifndef TOSHIBA_GIGABEAT_F
    /* Cache BRR waves */
    #define SPC_BRRCACHE 1

    /* Disable gaussian interpolation */
    #define SPC_NOINTERP 1

#ifndef CPU_COLDFIRE
    /* Disable echo processing */
    #define SPC_NOECHO 1
#else
    /* Enable echo processing */
    #define SPC_NOECHO 0
#endif
#else
    /* Don't cache BRR waves */
    #define SPC_BRRCACHE 0 
    
    /* Allow gaussian interpolation */
    #define SPC_NOINTERP 0
    
    /* Allow echo processing */
    #define SPC_NOECHO 0
#endif

/* Samples per channel per iteration */
#ifdef CPU_COLDFIRE
#define WAV_CHUNK_SIZE 1024
#else
#define WAV_CHUNK_SIZE 2048
#endif

/* simple profiling with USEC_TIMER */
/*#define SPC_PROFILE*/

#include "spc/spc_profiler.h"

#define THIS struct Spc_Emu* const this

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

static struct
{
    union {
        uint8_t padding1 [0x100];
        uint16_t align;
    } padding1 [1];
    uint8_t ram      [0x10000];
    uint8_t padding2 [0x100];
} ram;

#include "spc/Spc_Dsp.h"

#undef RAM
#define RAM ram.ram

/**************** Timers ****************/

enum { timer_count = 3 };

struct Timer
{
    long next_tick;
    int period;
    int count;
    int shift;
    int enabled;
    int counter;
};

static void Timer_run_( struct Timer* t, long time ) ICODE_ATTR;
static void Timer_run_( struct Timer* t, long time )
{
    /* when disabled, next_tick should always be in the future */
    assert( t->enabled ); 
    
    int elapsed = ((time - t->next_tick) >> t->shift) + 1;
    t->next_tick += elapsed << t->shift;
    
    elapsed += t->count;
    if ( elapsed >= t->period ) /* avoid unnecessary division */
    {
        int n = elapsed / t->period;
        elapsed -= n * t->period;
        t->counter = (t->counter + n) & 15;
    }
    t->count = elapsed;
}

static inline void Timer_run( struct Timer* t, long time )
{
    if ( time >= t->next_tick )
        Timer_run_( t, time );
}

/**************** SPC emulator ****************/
/* 1.024 MHz clock / 32000 samples per second */
enum { clocks_per_sample = 32 }; 

enum { extra_clocks = clocks_per_sample / 2 };

/* using this disables timer (since this will always be in the future) */
enum { timer_disabled_time = 127 };

enum { rom_size = 64 };
enum { rom_addr = 0xFFC0 };

struct cpu_regs_t
{
    long pc; /* more than 16 bits to allow overflow detection */
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t status;
    uint8_t sp;
};


struct Spc_Emu
{
    uint8_t cycle_table [0x100];
    struct cpu_regs_t r;
    
    int32_t* sample_buf;
    long next_dsp;
    int rom_enabled;
    int extra_cycles;
    
    struct Timer timer [timer_count];
    
    /* large objects at end */
    struct Spc_Dsp dsp;
    uint8_t extra_ram [rom_size];
    uint8_t boot_rom  [rom_size];
};

static void SPC_enable_rom( THIS, int enable )
{
    if ( this->rom_enabled != enable )
    {
        this->rom_enabled = enable;
        memcpy( RAM + rom_addr, (enable ? this->boot_rom : this->extra_ram), rom_size );
        /* TODO: ROM can still get overwritten when DSP writes to echo buffer */
    }
}

static void SPC_Init( THIS )
{
    this->timer [0].shift = 4 + 3; /* 8 kHz */
    this->timer [1].shift = 4 + 3; /* 8 kHz */
    this->timer [2].shift = 4; /* 8 kHz */
    
    /* Put STOP instruction around memory to catch PC underflow/overflow. */
    memset( ram.padding1, 0xFF, sizeof ram.padding1 );
    memset( ram.padding2, 0xFF, sizeof ram.padding2 );
    
    /* A few tracks read from the last four bytes of IPL ROM */
    this->boot_rom [sizeof this->boot_rom - 2] = 0xC0;
    this->boot_rom [sizeof this->boot_rom - 1] = 0xFF;
    memset( this->boot_rom, 0, sizeof this->boot_rom - 2 );
}

static void SPC_load_state( THIS, struct cpu_regs_t const* cpu_state,
        const void* new_ram, const void* dsp_state )
{
    memcpy(&(this->r),cpu_state,sizeof this->r);
        
    /* ram */
    memcpy( RAM, new_ram, sizeof RAM );
    memcpy( this->extra_ram, RAM + rom_addr, sizeof this->extra_ram );
    
    /* boot rom (have to force enable_rom() to update it) */
    this->rom_enabled = !(RAM [0xF1] & 0x80);
    SPC_enable_rom( this, !this->rom_enabled );
    
    /* dsp */
    /* some SPCs rely on DSP immediately generating one sample */
    this->extra_cycles = 32; 
    DSP_reset( &this->dsp );
    int i;
    for ( i = 0; i < register_count; i++ )
        DSP_write( &this->dsp, i, ((uint8_t const*) dsp_state) [i] );
    
    /* timers */
    for ( i = 0; i < timer_count; i++ )
    {
        struct Timer* t = &this->timer [i];
        
        t->next_tick = -extra_clocks;
        t->enabled = (RAM [0xF1] >> i) & 1;
        if ( !t->enabled )
            t->next_tick = timer_disabled_time;
        t->count = 0;
        t->counter = RAM [0xFD + i] & 15;
        
        int p = RAM [0xFA + i];
        if ( !p )
            p = 0x100;
        t->period = p;
    }
    
    /* Handle registers which already give 0 when read by
       setting RAM and not changing it.
       Put STOP instruction in registers which can be read,
       to catch attempted execution. */
    RAM [0xF0] = 0;
    RAM [0xF1] = 0;
    RAM [0xF3] = 0xFF;
    RAM [0xFA] = 0;
    RAM [0xFB] = 0;
    RAM [0xFC] = 0;
    RAM [0xFD] = 0xFF;
    RAM [0xFE] = 0xFF;
    RAM [0xFF] = 0xFF;
}

static void clear_echo( THIS )
{
    if ( !(DSP_read( &this->dsp, 0x6C ) & 0x20) )
    {
        unsigned addr = 0x100 * DSP_read( &this->dsp, 0x6D );
        size_t   size = 0x800 * DSP_read( &this->dsp, 0x7D );
        size_t max_size = sizeof RAM - addr;
        if ( size > max_size )
            size = sizeof RAM - addr;
        memset( RAM + addr, 0xFF, size );
    }
}

enum { spc_file_size = 0x10180 };

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

static int SPC_load_spc( THIS, const void* data, long size )
{
    struct spc_file_t const* spc = (struct spc_file_t const*) data;
    struct cpu_regs_t regs;
    
    if ( size < spc_file_size )
        return -1;
    
    if ( memcmp( spc->signature, "SNES-SPC700 Sound File Data", 27 ) != 0 )
        return -1;
    
    regs.pc     = spc->pc [1] * 0x100 + spc->pc [0];
    regs.a      = spc->a;
    regs.x      = spc->x;
    regs.y      = spc->y;
    regs.status = spc->status;
    regs.sp     = spc->sp;
    
    if ( (unsigned long) size >= sizeof *spc )
        memcpy( this->boot_rom, spc->ipl_rom, sizeof this->boot_rom );
    
    SPC_load_state( this, &regs, spc->ram, spc->dsp );
    
    clear_echo(this);
    
    return 0;
}

/**************** DSP interaction ****************/

static void SPC_run_dsp_( THIS, long time ) ICODE_ATTR;
static void SPC_run_dsp_( THIS, long time )
{
    /* divide by clocks_per_sample */
    int count = ((time - this->next_dsp) >> 5) + 1; 
    int32_t* buf = this->sample_buf;
    this->sample_buf = buf + count;
    this->next_dsp += count * clocks_per_sample;
    DSP_run( &this->dsp, count, buf );
}

static inline void SPC_run_dsp( THIS, long time )
{
    if ( time >= this->next_dsp )
        SPC_run_dsp_( this, time );
}

static int SPC_read( THIS, unsigned addr, long const time ) ICODE_ATTR;
static int SPC_read( THIS, unsigned addr, long const time )
{
    int result = RAM [addr];
    
    if ( ((unsigned) (addr - 0xF0)) < 0x10 )
    {
        assert( 0xF0 <= addr && addr <= 0xFF );
        
        /* counters */
        int i = addr - 0xFD;
        if ( i >= 0 )
        {
            struct Timer* t = &this->timer [i];
            Timer_run( t, time );
            result = t->counter;
            t->counter = 0;
        }
        /* dsp */
        else if ( addr == 0xF3 )
        {
            SPC_run_dsp( this, time );
            result = DSP_read( &this->dsp, RAM [0xF2] & 0x7F );
        }
    }
    return result;
}

static void SPC_write( THIS, unsigned addr, int data, long const time )
    ICODE_ATTR;
static void SPC_write( THIS, unsigned addr, int data, long const time )
{
    /* first page is very common */
    if ( addr < 0xF0 )
    {
        RAM [addr] = (uint8_t) data;
    }
    else switch ( addr )
    {
        /* RAM */
        default:
            if ( addr < rom_addr )
            {
                RAM [addr] = (uint8_t) data;
            }
            else
            {
                this->extra_ram [addr - rom_addr] = (uint8_t) data;
                if ( !this->rom_enabled )
                    RAM [addr] = (uint8_t) data;
            }
            break;
        
        /* DSP */
        /*case 0xF2:*/ /* mapped to RAM */
        case 0xF3: {
            SPC_run_dsp( this, time );
            int reg = RAM [0xF2];
            if ( reg < register_count ) {
                DSP_write( &this->dsp, reg, data );
            }
            else {
                /*dprintf( "DSP write to $%02X\n", (int) reg ); */
            }
            break;
        }
        
        case 0xF0: /* Test register */
            /*dprintf( "Wrote $%02X to $F0\n", (int) data ); */
            break;
        
        /* Config */
        case 0xF1:
        {
            int i;
            /* timers */
            for ( i = 0; i < timer_count; i++ )
            {
                struct Timer * t = this->timer+i;
                if ( !(data & (1 << i)) )
                {
                    t->enabled = 0;
                    t->next_tick = timer_disabled_time;
                }
                else if ( !t->enabled )
                {
                    /* just enabled */
                    t->enabled = 1;
                    t->counter = 0;
                    t->count = 0;
                    t->next_tick = time;
                }
            }
            
            /* port clears */
            if ( data & 0x10 )
            {
                RAM [0xF4] = 0;
                RAM [0xF5] = 0;
            }
            if ( data & 0x20 )
            {
                RAM [0xF6] = 0;
                RAM [0xF7] = 0;
            }
            
            SPC_enable_rom( this, (data & 0x80) != 0 );
            break;
        }
        
        /* Ports */
        case 0xF4:
        case 0xF5:
        case 0xF6:
        case 0xF7:
            /* to do: handle output ports */
            break;

        /* verified on SNES that these are read/write (RAM) */
        /*case 0xF8: */
        /*case 0xF9: */
        
        /* Timers */
        case 0xFA:
        case 0xFB:
        case 0xFC: {
            int i = addr - 0xFA;
            struct Timer* t = &this->timer [i];
            if ( (t->period & 0xFF) != data )
            {
                Timer_run( t, time );
                this->timer[i].period = data ? data : 0x100;
            }
            break;
        }
        
        /* Counters (cleared on write) */
        case 0xFD:
        case 0xFE:
        case 0xFF:
            /*dprintf( "Wrote to counter $%02X\n", (int) addr ); */
            this->timer [addr - 0xFD].counter = 0;
            break;
    }
}

#include "spc/Spc_Cpu.h"

/**************** Sample generation ****************/

static int SPC_play( THIS, long count, int32_t* out ) ICODE_ATTR;
static int SPC_play( THIS, long count, int32_t* out )
{
    int i;
    assert( count % 2 == 0 ); /* output is always in pairs of samples */
    
    long start_time = -(count >> 1) * clocks_per_sample - extra_clocks;
    
    /* DSP output is made on-the-fly when DSP registers are read or written */
    this->sample_buf = out;
    this->next_dsp = start_time + clocks_per_sample;
    
    /* Localize timer next_tick times and run them to the present to prevent
       a running but ignored timer's next_tick from getting too far behind
       and overflowing. */
    for ( i = 0; i < timer_count; i++ )
    {
        struct Timer* t = &this->timer [i];
        if ( t->enabled )
        {
            t->next_tick += start_time + extra_clocks;
            Timer_run( t, start_time );
        }
    }
    
    /* Run from start_time to 0, pre-advancing by extra cycles from last run */
    this->extra_cycles = CPU_run( this, start_time + this->extra_cycles ) +
                         extra_clocks;
    if ( this->extra_cycles < 0 )
    {
        /*dprintf( "Unhandled instruction $%02X, pc = $%04X\n",
                (int) CPU_read( r.pc ), (unsigned) r.pc ); */
        
        return -1;
    }
    
    /* Catch DSP up to present */
#if 0
    ENTER_TIMER(cpu);
#endif
    SPC_run_dsp( this, -extra_clocks );
#if 0
    EXIT_TIMER(cpu);
#endif
    assert( this->next_dsp == clocks_per_sample - extra_clocks );
    assert( this->sample_buf - out == count );
    
    return 0;
}

/**************** ID666 parsing ****************/

struct {
    unsigned char isBinary;
    char song[32];
    char game[32];
    char dumper[16];
    char comments[32];
    int day,month,year;
    unsigned long length;
    unsigned long fade;
    char artist[32];
    unsigned char muted;
    unsigned char emulator;
} ID666;

static int LoadID666(unsigned char *buf) {
    unsigned char *ib=buf;
    int isbinary = 1;
    int i;
  
    memcpy(ID666.song,ib,32);
    ID666.song[31]=0;
    ib+=32;

    memcpy(ID666.game,ib,32);
    ID666.game[31]=0;
    ib+=32;

    memcpy(ID666.dumper,ib,16);
    ID666.dumper[15]=0;
    ib+=16;

    memcpy(ID666.comments,ib,32);
    ID666.comments[31]=0;
    ib+=32;

    /* Ok, now comes the fun part. */
   
    /* Date check */
    if(ib[2] == '/'  && ib[5] == '/' )
        isbinary = 0;
  
    /* Reserved bytes check */
    if(ib[0xD2 - 0x2E - 112] >= '0' &&
       ib[0xD2 - 0x2E - 112] <= '9' &&
       ib[0xD3 - 0x2E - 112] == 0x00)
        isbinary = 0;
    
    /* is length & fade only digits? */
    for (i=0;i<8 && ( 
        (ib[0xA9 - 0x2E - 112+i]>='0'&&ib[0xA9 - 0x2E - 112+i]<='9') ||
        ib[0xA9 - 0x2E - 112+i]=='\0');
        i++);
    if (i==8) isbinary=0;
    
    ID666.isBinary = isbinary;
  
    if(isbinary) {
        DEBUGF("binary tag detected\n");
        ID666.year=*ib;
        ib++;
        ID666.year|=*ib<<8;
        ib++;
        ID666.month=*ib;
        ib++;    
        ID666.day=*ib;
        ib++;

        ib+=7;

        ID666.length=*ib;
        ib++;
    
        ID666.length|=*ib<<8;
        ib++;
    
        ID666.length|=*ib<<16;
        ID666.length*=1000;
        ib++;
    
        ID666.fade=*ib;
        ib++;
        ID666.fade|=*ib<<8; 
        ib++;
        ID666.fade|=*ib<<16;
        ib++;
        ID666.fade|=*ib<<24;
        ib++;

        memcpy(ID666.artist,ib,32);
        ID666.artist[31]=0;
        ib+=32;

        ID666.muted=*ib;
        ib++;

        ID666.emulator=*ib;
        ib++;    
    } else {
        unsigned long tmp;
        char buf[64];
        
        DEBUGF("text tag detected\n");
        
        memcpy(buf, ib, 2);
        buf[2] = 0; 
        tmp = 0;
        for (i=0;i<2 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.month = tmp;
        ib+=3;
        
        memcpy(buf, ib, 2);
        buf[2] = 0; 
        tmp = 0;
        for (i=0;i<2 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.day = tmp;
        ib+=3;
        
        memcpy(buf, ib, 4);
        buf[4] = 0; 
        tmp = 0;
        for (i=0;i<4 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.year = tmp;
        ib+=5;
    
        memcpy(buf, ib, 3);
        buf[3] = 0; 
        tmp = 0;
        for (i=0;i<3 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.length = tmp * 1000;
        ib+=3;

        memcpy(buf, ib, 5);
        buf[5] = 0;
        tmp = 0;
        for (i=0;i<5 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.fade = tmp;
        ib+=5;

        memcpy(ID666.artist,ib,32);
        ID666.artist[31]=0;
        ib+=32;

        /*I have no idea if this is right or not.*/
        ID666.muted=*ib;
        ib++;

        memcpy(buf, ib, 1);
        buf[1] = 0;
        tmp = 0;
        ib++;
    }
    return 1;
}

/**************** Codec ****************/

static int32_t samples[WAV_CHUNK_SIZE*2] IBSS_ATTR;

static struct Spc_Emu spc_emu IDATA_ATTR;

enum {sample_rate = 32000};

/* The main decoder loop */
static int play_track( void )
{
    int sampleswritten=0;
    
    unsigned long fadestartsample = ID666.length*(long long) sample_rate/1000;
    unsigned long fadeendsample = (ID666.length+ID666.fade)*(long long) sample_rate/1000;
    int fadedec = 0;
    int fadevol = 0x7fffffffl;
    
    if (fadeendsample>fadestartsample)
        fadedec=0x7fffffffl/(fadeendsample-fadestartsample)+1;
        
    ENTER_TIMER(total);
    
    while ( 1 )
    {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }

        if (ci->seek_time) {
            int curtime = sampleswritten*1000LL/sample_rate;
            DEBUGF("seek to %ld\ncurrently at %d\n",ci->seek_time,curtime);
            if (ci->seek_time < curtime) {
                DEBUGF("seek backwards = reset\n");
                ci->seek_complete();
                return 1;
            }
            ci->seek_complete();
        }
        
        ENTER_TIMER(render);
        /* fill samples buffer */
        if ( SPC_play(&spc_emu,WAV_CHUNK_SIZE*2,samples) )
            assert( false );
        EXIT_TIMER(render);
        
        sampleswritten+=WAV_CHUNK_SIZE;

        /* is track timed? */
        if (ci->global_settings->repeat_mode!=REPEAT_ONE && ci->id3->length) {
            unsigned long curtime = sampleswritten*1000LL/sample_rate;
            unsigned long lasttimesample = (sampleswritten-WAV_CHUNK_SIZE);

            /* fade? */
            if (curtime>ID666.length)
            {
#ifdef CPU_COLDFIRE
                /* Have to switch modes to do this */
                long macsr = coldfire_get_macsr();
                coldfire_set_macsr(EMAC_SATURATE | EMAC_FRACTIONAL | EMAC_ROUND);
#endif
                int i;
                for (i=0;i<WAV_CHUNK_SIZE;i++) {
                    if (lasttimesample+i>fadestartsample) {
                        if (fadevol>0) {
                            samples[i] = FRACMUL(samples[i], fadevol);
                            samples[i+WAV_CHUNK_SIZE] = FRACMUL(samples[i+WAV_CHUNK_SIZE], fadevol);
                        } else samples[i]=samples[i+WAV_CHUNK_SIZE]=0;
                        fadevol-=fadedec;
                    }
                }
#ifdef CPU_COLDFIRE
               coldfire_set_macsr(macsr);
#endif
            }
            /* end? */
            if (lasttimesample>=fadeendsample)
                break;
        }

        ci->pcmbuf_insert(samples, samples+WAV_CHUNK_SIZE, WAV_CHUNK_SIZE);

        if (ci->global_settings->repeat_mode!=REPEAT_ONE)
        ci->set_elapsed(sampleswritten*1000LL/sample_rate);
        else
            ci->set_elapsed(0);
    }
    
    EXIT_TIMER(total);
    
    return 0;
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    memcpy( spc_emu.cycle_table, cycle_table, sizeof cycle_table );

#ifdef CPU_COLDFIRE
    coldfire_set_macsr(EMAC_SATURATE);
#endif

    do
    {
        DEBUGF("SPC: next_track\n");
        if (codec_init()) {
            return CODEC_ERROR;
        }
        DEBUGF("SPC: after init\n");

        ci->configure(DSP_SET_SAMPLE_DEPTH, 24);
        ci->configure(DSP_SET_FREQUENCY, sample_rate);
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);

        /* wait for track info to load */
        while (!*ci->taginfo_ready && !ci->stop_codec)
            ci->sleep(1);

        codec_set_replaygain(ci->id3);

        /* Read the entire file */
        DEBUGF("SPC: request initial buffer\n");
        ci->configure(CODEC_SET_FILEBUF_WATERMARK, ci->filesize);

        ci->seek_buffer(0);
        size_t buffersize;
        uint8_t* buffer = ci->request_buffer(&buffersize, ci->filesize);
        if (!buffer) {
            return CODEC_ERROR;
        }

        DEBUGF("SPC: read size = 0x%x\n",buffersize);
        do
        {
            SPC_Init(&spc_emu);
            if (SPC_load_spc(&spc_emu,buffer,buffersize)) {
                DEBUGF("SPC load failure\n");
                return CODEC_ERROR;
            }

            LoadID666(buffer+0x2e);
            
            if (ci->global_settings->repeat_mode!=REPEAT_ONE && ID666.length==0) {
                ID666.length=3*60*1000; /* 3 minutes */
                ID666.fade=5*1000; /* 5 seconds */
            }
            ci->id3->length = ID666.length+ID666.fade;
            ci->id3->title = ID666.song;
            ci->id3->album = ID666.game;
            ci->id3->artist = ID666.artist;
            ci->id3->year = ID666.year;
            ci->id3->comment = ID666.comments;

            reset_profile_timers();
        }
        
        while ( play_track() );

        print_timers(ci->id3->path);
    }
    while ( ci->request_next_track() );
    
    return CODEC_OK;
}
