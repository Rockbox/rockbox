/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#include "codeclib.h"
#include "spc_codec.h"
#include "spc_profiler.h"

/* lovingly ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */
/* DSP Based on Brad Martin's OpenSPC DSP emulator */
/* tag reading from sexyspc by John Brawn (John_Brawn@yahoo.com) and others */

struct cpu_ram_t ram CACHEALIGN_ATTR;

/**************** Timers ****************/

void Timer_run_( struct Timer* t, long time )
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

/**************** SPC emulator ****************/
/* 1.024 MHz clock / 32000 samples per second */

static void SPC_enable_rom( THIS, int enable )
{
    if ( this->rom_enabled != enable )
    {
        this->rom_enabled = enable;
        ci->memcpy( RAM + ROM_ADDR, (enable ? this->boot_rom : this->extra_ram), ROM_SIZE );
        /* TODO: ROM can still get overwritten when DSP writes to echo buffer */
    }
}

void SPC_Init( THIS )
{
    this->timer [0].shift = 4 + 3; /* 8 kHz */
    this->timer [1].shift = 4 + 3; /* 8 kHz */
    this->timer [2].shift = 4; /* 8 kHz */
    
    /* Put STOP instruction around memory to catch PC underflow/overflow. */
    ci->memset( ram.padding1, 0xFF, sizeof ram.padding1 );
    ci->memset( ram.padding2, 0xFF, sizeof ram.padding2 );
    
    /* A few tracks read from the last four bytes of IPL ROM */
    this->boot_rom [sizeof this->boot_rom - 2] = 0xC0;
    this->boot_rom [sizeof this->boot_rom - 1] = 0xFF;
    ci->memset( this->boot_rom, 0, sizeof this->boot_rom - 2 );

    /* Have DSP in a defined state in case EMU is run and hasn't loaded
     * a program yet */
    DSP_reset(&this->dsp);
}

static void SPC_load_state( THIS, struct cpu_regs_t const* cpu_state,
        const void* new_ram, const void* dsp_state )
{
    ci->memcpy(&(this->r),cpu_state,sizeof this->r);
        
    /* ram */
    ci->memcpy( RAM, new_ram, sizeof RAM );
    ci->memcpy( this->extra_ram, RAM + ROM_ADDR, sizeof this->extra_ram );
    
    /* boot rom (have to force enable_rom() to update it) */
    this->rom_enabled = !(RAM [0xF1] & 0x80);
    SPC_enable_rom( this, !this->rom_enabled );
    
    /* dsp */
    /* some SPCs rely on DSP immediately generating one sample */
    this->extra_cycles = 32; 
    DSP_reset( &this->dsp );
    int i;
    for ( i = 0; i < REGISTER_COUNT; i++ )
        DSP_write( &this->dsp, i, ((uint8_t const*) dsp_state) [i] );
    
    /* timers */
    for ( i = 0; i < TIMER_COUNT; i++ )
    {
        struct Timer* t = &this->timer [i];
        
        t->next_tick = -EXTRA_CLOCKS;
        t->enabled = (RAM [0xF1] >> i) & 1;
        if ( !t->enabled )
            t->next_tick = TIMER_DISABLED_TIME;
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
        ci->memset( RAM + addr, 0xFF, size );
    }
}

int SPC_load_spc( THIS, const void* data, long size )
{
    struct spc_file_t const* spc = (struct spc_file_t const*) data;
    struct cpu_regs_t regs;
    
    if ( size < SPC_FILE_SIZE )
        return -1;
    
    if ( ci->memcmp( spc->signature, "SNES-SPC700 Sound File Data", 27 ) != 0 )
        return -1;
    
    regs.pc     = spc->pc [1] * 0x100 + spc->pc [0];
    regs.a      = spc->a;
    regs.x      = spc->x;
    regs.y      = spc->y;
    regs.status = spc->status;
    regs.sp     = spc->sp;
    
    if ( (unsigned long) size >= sizeof *spc )
        ci->memcpy( this->boot_rom, spc->ipl_rom, sizeof this->boot_rom );
    
    SPC_load_state( this, &regs, spc->ram, spc->dsp );
    
    clear_echo(this);
    
    return 0;
}

/**************** DSP interaction ****************/
void SPC_run_dsp_( THIS, long time )
{
    /* divide by CLOCKS_PER_SAMPLE */
    int count = ((time - this->next_dsp) >> 5) + 1; 
    int32_t* buf = this->sample_buf;
    this->sample_buf = buf + count;
    this->next_dsp += count * CLOCKS_PER_SAMPLE;
    DSP_run( &this->dsp, count, buf );
}

int SPC_read( THIS, unsigned addr, long const time )
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

void SPC_write( THIS, unsigned addr, int data, long const time )
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
            if ( addr < ROM_ADDR )
            {
                RAM [addr] = (uint8_t) data;
            }
            else
            {
                this->extra_ram [addr - ROM_ADDR] = (uint8_t) data;
                if ( !this->rom_enabled )
                    RAM [addr] = (uint8_t) data;
            }
            break;
        
        /* DSP */
        /*case 0xF2:*/ /* mapped to RAM */
        case 0xF3: {
            SPC_run_dsp( this, time );
            int reg = RAM [0xF2];
            if ( reg < REGISTER_COUNT ) {
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
            for ( i = 0; i < TIMER_COUNT; i++ )
            {
                struct Timer * t = this->timer+i;
                if ( !(data & (1 << i)) )
                {
                    t->enabled = 0;
                    t->next_tick = TIMER_DISABLED_TIME;
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

/**************** Sample generation ****************/
int SPC_play( THIS, long count, int32_t* out )
{
    int i;
    assert( count % 2 == 0 ); /* output is always in pairs of samples */
    
    long start_time = -(count >> 1) * CLOCKS_PER_SAMPLE - EXTRA_CLOCKS;
    
    /* DSP output is made on-the-fly when DSP registers are read or written */
    this->sample_buf = out;
    this->next_dsp = start_time + CLOCKS_PER_SAMPLE;
    
    /* Localize timer next_tick times and run them to the present to prevent
       a running but ignored timer's next_tick from getting too far behind
       and overflowing. */
    for ( i = 0; i < TIMER_COUNT; i++ )
    {
        struct Timer* t = &this->timer [i];
        if ( t->enabled )
        {
            t->next_tick += start_time + EXTRA_CLOCKS;
            Timer_run( t, start_time );
        }
    }
    
    /* Run from start_time to 0, pre-advancing by extra cycles from last run */
    this->extra_cycles = CPU_run( this, start_time + this->extra_cycles ) +
                         EXTRA_CLOCKS;
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
    SPC_run_dsp( this, -EXTRA_CLOCKS );
#if 0
    EXIT_TIMER(cpu);
#endif
    assert( this->next_dsp == CLOCKS_PER_SAMPLE - EXTRA_CLOCKS );
    assert( this->sample_buf - out == count );
    
    return 0;
}
