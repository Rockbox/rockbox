// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "hes_emu.h"

#include "blargg_endian.h"

//#include "hes_cpu_log.h"

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
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
#define PAGE HES_CPU_PAGE

int read_mem( struct Hes_Emu* this, hes_addr_t addr )
{
	check( addr < 0x10000 );
	int result = *Cpu_get_code( &this->cpu, addr );
	if ( this->cpu.mmr [PAGE( addr )] == 0xFF )
		result = read_mem_( this, addr );
	return result;
}

void write_mem( struct Hes_Emu* this, hes_addr_t addr, int data )
{
	check( addr < 0x10000 );
	byte* out = this->write_pages [PAGE( addr )];
	if ( out )
		out [addr & (page_size - 1)] = data;
	else if ( this->cpu.mmr [PAGE( addr )] == 0xFF )
		write_mem_( this, addr, data );
}

void set_mmr( struct Hes_Emu* this, int page, int bank )
{
	this->write_pages [page] = 0;
	byte* data = Rom_at_addr( &this->rom, bank * page_size );
	if ( bank >= 0x80 )
	{
		data = 0;
		switch ( bank )
		{
		case 0xF8:
			data = this->ram;
			break;
		
		case 0xF9:
		case 0xFA:
		case 0xFB:
			data = &this->sgx [(bank - 0xF9) * page_size];
			break;
		
		default:
			/* if ( bank != 0xFF )
				dprintf( "Unmapped bank $%02X\n", bank ); */
			data = this->rom.unmapped;
			goto end;
		}
		
		this->write_pages [page] = data;
	}
end:
	Cpu_set_mmr( &this->cpu, page, bank, data );
}

#define READ_FAST( addr, out ) \
{\
	out = READ_CODE( addr );\
	if ( cpu->mmr [PAGE( addr )] == 0xFF )\
	{\
		FLUSH_TIME();\
		out = read_mem_( this, addr );\
		CACHE_TIME();\
	}\
}

#define WRITE_FAST( addr, data ) \
{\
	int page = PAGE( addr );\
	byte* out = this->write_pages [page];\
	addr &= page_size - 1;\
	if ( out )\
	{\
		out [addr] = data;\
	}\
	else if ( cpu->mmr [page] == 0xFF )\
	{\
		FLUSH_TIME();\
		write_mem_( this, addr, data );\
		CACHE_TIME();\
	}\
}

#define READ_LOW(  addr )           (this->ram [addr])
#define WRITE_LOW( addr, data )     (this->ram [addr] = data)
#define READ_MEM(  addr )           read_mem(  this, addr )
#define WRITE_MEM( addr, data )     write_mem( this, addr, data )
#define WRITE_VDP( addr, data )     write_vdp( this, addr, data )
#define CPU_DONE( result_out )      { FLUSH_TIME(); result_out = cpu_done( this ); CACHE_TIME(); }
#define SET_MMR( reg, bank )        set_mmr( this, reg, bank )

#define IDLE_ADDR   idle_addr

#define CPU_BEGIN \
bool run_cpu( struct Hes_Emu* this, hes_time_t end_time )\
{\
	struct Hes_Cpu* cpu = &this->cpu;\
	Cpu_set_end_time( cpu, end_time );
	
	#include "hes_cpu_run.h"
	
	return illegal_encountered;
}
