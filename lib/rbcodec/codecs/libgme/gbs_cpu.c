// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "gbs_emu.h"
#include "blargg_endian.h"

/* Copyright (C) 2003-2009 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

#ifndef LOG_MEM
	#define LOG_MEM( addr, str, data ) data
#endif

int read_mem( struct Gbs_Emu* this, addr_t addr )
{
	int result = *Cpu_get_code( &this->cpu, addr );
	if ( (unsigned) (addr - io_addr) < io_size )
		result = Apu_read_register( &this->apu, Time( this ), addr );

	return LOG_MEM( addr, ">", result );
}

static inline void write_io_inline( struct Gbs_Emu* this, int offset, int data, int base )
{
	if ( (unsigned) (offset - (io_addr - base)) < io_size )
		Apu_write_register( &this->apu, Time( this ), offset + base, data & 0xFF );
	else if ( (unsigned) (offset - (0xFF06 - base)) < 2 )
		update_timer( this );
	else if ( offset == io_base - base )
		this->ram [base - ram_addr + offset] = 0; // keep joypad return value 0
	else
		this->ram [base - ram_addr + offset] = 0xFF;
}

void write_mem( struct Gbs_Emu* this, addr_t addr, int data )
{
	(void) LOG_MEM( addr, "<", data );
	
	int offset = addr - ram_addr;
	if ( (unsigned) offset < 0x10000 - ram_addr )
	{
		this->ram [offset] = data;
		
		offset -= 0xE000 - ram_addr;
		if ( (unsigned) offset < 0x1F80 )
			write_io_inline( this, offset, data, 0xE000 );
	}
	else if ( (unsigned) (offset - (0x2000 - ram_addr)) < 0x2000 )
	{
		set_bank( this, data & 0xFF );
	}
#ifndef NDEBUG
	else if ( unsigned (addr - 0x8000) < 0x2000 || unsigned (addr - 0xE000) < 0x1F00 )
	{
		/* dprintf( "Unmapped write $%04X\n", (unsigned) addr ); */
	}
#endif
}

static void write_io_( struct Gbs_Emu* this, int offset, int data )
{
	write_io_inline( this, offset, data, io_base );
}

static inline void write_io( struct Gbs_Emu* this, int offset, int data )
{
	(void) LOG_MEM( offset + io_base, "<", data );
	
	this->ram [io_base - ram_addr + offset] = data;
	if ( (unsigned) offset < 0x80 )
		write_io_( this, offset, data );
}

static int read_io( struct Gbs_Emu* this, int offset )
{
	int const io_base = 0xFF00;
	int result = this->ram [io_base - ram_addr + offset];
	
	if ( (unsigned) (offset - (io_addr - io_base)) < io_size )
	{
		result = Apu_read_register( &this->apu, Time( this ), offset + io_base );
		(void) LOG_MEM( offset + io_base, ">", result );
	}
	else
	{
		check( result == read_mem( offset + io_base ) );
	}
	return result;
}

#define READ_FAST( emu, addr, out ) \
{\
	out = READ_CODE( addr );\
	if ( (unsigned) (addr - io_addr) < io_size )\
		out = LOG_MEM( addr, ">", Apu_read_register( &emu->apu, TIME() + emu->end_time, addr ) );\
	else\
		check( out == Read_mem( emu, addr ) );\
}

#define READ_MEM(  emu, addr       ) read_mem( emu, addr )
#define WRITE_MEM( emu, addr, data ) write_mem( emu, addr, data )

#define WRITE_IO( emu, addr, data )  write_io( emu, addr, data )
#define READ_IO( emu, addr, out )    out = read_io( emu, addr )

#define CPU_BEGIN \
void run_cpu( struct Gbs_Emu* this )\
{ \
	struct Gb_Cpu* cpu = &this->cpu;
	#include "gb_cpu_run.h"
}
