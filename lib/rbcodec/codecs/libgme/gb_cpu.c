// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "gb_cpu.h"

#include "blargg_endian.h"

/* Copyright (C) 2003-2008 Shay Green. This module is free software; you
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

static inline void set_code_page( struct Gb_Cpu* this, int i, void* p )
{
	byte* p2 = STATIC_CAST(byte*,p) - GB_CPU_OFFSET( i * page_size );
	this->cpu_state_.code_map [i] = p2;
	this->cpu_state->code_map [i] = p2;
}

void Cpu_reset( struct Gb_Cpu* this, void* unmapped )
{
	check( this->cpu_state == &this->cpu_state_ );
	this->cpu_state = &this->cpu_state_;
	
	this->cpu_state_.time = 0;
	
	int i;
	for ( i = 0; i < page_count + 1; ++i )
		set_code_page( this, i, unmapped );
	
	memset( &this->r, 0, sizeof this->r );
	
	blargg_verify_byte_order();
}

void Cpu_map_code( struct Gb_Cpu* this, addr_t start, int size, void* data )
{
	// address range must begin and end on page boundaries
	require( start % page_size == 0 );
	require( size  % page_size == 0 );
	require( start + size <= mem_size );
	
	int offset;
	for ( offset = 0; offset < size; offset += page_size )
		set_code_page( this, (start + offset) >> page_bits, STATIC_CAST(char*,data) + offset );
}
