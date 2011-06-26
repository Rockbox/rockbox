// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "ay_emu.h"

#include "blargg_endian.h"
//#include "z80_cpu_log.h"

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

void cpu_out( struct Ay_Emu* this, cpu_time_t time, addr_t addr, int data )
{
	if ( (addr & 0xFF) == 0xFE )
	{
		check( !cpc_mode );
		this->spectrum_mode = !this->cpc_mode;
		
		// beeper_mask and last_beeper are 0 if (cpc_mode || !beeper_output)
		if ( (data &= this->beeper_mask) != this->last_beeper )
		{
			this->last_beeper = data;
			int delta = -this->beeper_delta;
			this->beeper_delta = delta;
			struct Blip_Buffer* bb = this->beeper_output;
			Blip_set_modified( bb );
			Synth_offset( &this->apu.synth_, time, delta, bb );
		}
	}
	else
	{
		cpu_out_( this, time, addr, data );
	}
}

#define OUT_PORT( addr, data )  cpu_out( this, TIME(), addr, data )
#define IN_PORT( addr )  0xFF // cpu in
#define FLAT_MEM                mem

#define CPU_BEGIN \
bool run_cpu( struct Ay_Emu* this, cpu_time_t end_time ) \
{\
	struct Z80_Cpu* cpu = &this->cpu; \
	Z80_set_end_time( cpu, end_time ); \
	byte* const mem = this->mem.ram; // cache
	
	#include "z80_cpu_run.h"
	
	return warning;
}
