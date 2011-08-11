// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "z80_cpu.h"

/* Copyright (C) 2006-2008 Shay Green. This module is free software; you
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

// flags, named with hex value for clarity
int const S80 = 0x80;
int const Z40 = 0x40;
int const F20 = 0x20;
int const H10 = 0x10;
int const F08 = 0x08;
int const V04 = 0x04;
int const P04 = 0x04;
int const N02 = 0x02;
int const C01 = 0x01;

void Z80_init( struct Z80_Cpu* this ) 
{
	this->cpu_state = &this->cpu_state_;
	
	int i;
	for ( i = 0x100; --i >= 0; )
	{
		int p, even = 1;
		for ( p = i; p; p >>= 1 )
			even ^= p;
		int n = (i & (S80 | F20 | F08)) | ((even & 1) * P04);
		this->szpc [i] = n;
		this->szpc [i + 0x100] = n | C01;
	}
	this->szpc [0x000] |= Z40;
	this->szpc [0x100] |= Z40;
}

static inline void set_page( struct Z80_Cpu* this, int i, void* write, void const* read )
{
	int offset = Z80_CPU_OFFSET( i * page_size );
	byte      * write2 = STATIC_CAST(byte      *,write) - offset;
	byte const* read2  = STATIC_CAST(byte const*,read ) - offset;
	this->cpu_state_.write [i] = write2;
	this->cpu_state_.read  [i] = read2;
	this->cpu_state->write [i] = write2;
	this->cpu_state->read  [i] = read2;
}

void Z80_reset( struct Z80_Cpu* this, void* unmapped_write, void const* unmapped_read )
{
	check( this->cpu_state == &this->cpu_state_ );
	this->cpu_state = &this->cpu_state_;
	this->cpu_state_.time = 0;
	this->cpu_state_.base = 0;
	this->end_time_   = 0;
	
	int i;
	for ( i = 0; i < page_count + 1; i++ )
		set_page( this, i, unmapped_write, unmapped_read );
	
	memset( &this->r, 0, sizeof this->r );
}

void Z80_map_mem( struct Z80_Cpu* this, addr_t start, int size, void* write, void const* read )
{
	// address range must begin and end on page boundaries
	require( start % page_size == 0 );
	require( size  % page_size == 0 );
	require( start + size <= 0x10000 );
	
	int offset;
	for ( offset = 0; offset < size; offset += page_size )
		set_page( this, (start + offset) >> page_bits,
				STATIC_CAST(char      *,write) + offset,
				STATIC_CAST(char const*,read ) + offset );
}
