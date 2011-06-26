// Normal cpu for NSF emulator

// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "nsf_emu.h"

#include "blargg_endian.h"

#ifdef BLARGG_DEBUG_H
	//#define CPU_LOG_START 1000000
	//#include "nes_cpu_log.h"
	#undef LOG_MEM
#endif

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

#ifndef LOG_MEM
	#define LOG_MEM( addr, str, data ) data
#endif

int read_mem( struct Nsf_Emu* this, addr_t addr )
{
	int result = this->low_ram [addr & (low_ram_size-1)]; // also handles wrap-around
	if ( addr & 0xE000 )
	{
		result = *Cpu_get_code( &this->cpu, addr );
		if ( addr < sram_addr )
		{
			if ( addr == apu_status_addr )
				result = Apu_read_status( &this->apu, Cpu_time( &this->cpu ) );
			else
				result = cpu_read( this, addr );
		}
	}
	return LOG_MEM( addr, ">", result );
}

void write_mem( struct Nsf_Emu* this, addr_t addr, int data )
{
	(void) LOG_MEM( addr, "<", data );
	
	int offset = addr - sram_addr;
	if ( (unsigned) offset < sram_size )
	{
		sram( this ) [offset] = data;
	}
	else
	{
		// after sram because cpu handles most low_ram accesses internally already
		int temp = addr & (low_ram_size-1); // also handles wrap-around
		if ( !(addr & 0xE000) )
		{
			this->low_ram [temp] = data;
		}
		else
		{
			int bank = addr - banks_addr;
			if ( (unsigned) bank < bank_count )
			{
				write_bank( this, bank, data );
			}
			else if ( (unsigned) (addr - apu_io_addr) < apu_io_size )
			{
				Apu_write_register( &this->apu, Cpu_time( &this->cpu ), addr, data );
			}
			else
			{
			#ifndef NSF_EMU_APU_ONLY
				// 0x8000-0xDFFF is writable
				int i = addr - 0x8000;
				if ( fds_enabled( this ) && (unsigned) i < fdsram_size )
					fdsram( this ) [i] = data;
				else
			#endif
				cpu_write( this, addr, data );
			}
		}
	}
}

#define READ_LOW(  addr       ) (LOG_MEM( addr, ">", this->low_ram [addr] ))
#define WRITE_LOW( addr, data ) (LOG_MEM( addr, "<", this->low_ram [addr] = data ))

#define CAN_WRITE_FAST( addr )  (addr < low_ram_size)
#define WRITE_FAST              WRITE_LOW

// addr < 0x2000 || addr >= 0x8000
#define CAN_READ_FAST( addr )   ((addr ^ 0x8000) < 0xA000)
#define READ_FAST( addr, out  ) (LOG_MEM( addr, ">", out = READ_CODE( addr ) ))

#define READ_MEM(  addr       ) read_mem(  this, addr )
#define WRITE_MEM( addr, data ) write_mem( this, addr, data )

#define CPU_BEGIN \
bool run_cpu_until( struct Nsf_Emu* this, nes_time_t end ) \
{ \
	struct Nes_Cpu* cpu = &this->cpu; \
	Cpu_set_end_time( cpu, end ); \
	if ( *Cpu_get_code( cpu, cpu->r.pc ) != halt_opcode ) \
	{
		#include "nes_cpu_run.h"
	}
	return Cpu_time_past_end( cpu ) < 0;
}
