// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "nes_fds_apu.h"

/* Copyright (C) 2006 Shay Green. This module is free software; you
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

int const fract_range = 65536;

void Fds_init( struct Nes_Fds_Apu* this )
{
	Synth_init( &this->synth );
		
	this->lfo_tempo = lfo_base_tempo;
	Fds_set_output( this, 0, NULL );
	Fds_volume( this, (int)FP_ONE_VOLUME );
	Fds_reset( this );
}

void Fds_reset( struct Nes_Fds_Apu* this )
{
	memset( this->regs_, 0, sizeof this->regs_ );
	memset( this->mod_wave, 0, sizeof this->mod_wave );
	
	this->last_time     = 0;
	this->env_delay     = 0;
	this->sweep_delay   = 0;
	this->wave_pos      = 0;
	this->last_amp      = 0;
	this->wave_fract    = fract_range;
	this->mod_fract     = fract_range;
	this->mod_pos       = 0;
	this->mod_write_pos = 0;
	
	static byte const initial_regs [0x0B] = {
		0x80,       // disable envelope
		0, 0, 0xC0, // disable wave and lfo
		0x80,       // disable sweep
		0, 0, 0x80, // disable modulation
		0, 0, 0xFF  // LFO period // TODO: use 0xE8 as FDS ROM does?
	};
	int i;
	for ( i = 0; i < (int) sizeof initial_regs; i++ )
	{
		// two writes to set both gain and period for envelope registers
		Fds_write_( this, fds_io_addr + fds_wave_size + i, 0 );
		Fds_write_( this, fds_io_addr + fds_wave_size + i, initial_regs [i] );
	}
}

void Fds_write_( struct Nes_Fds_Apu* this, unsigned addr, int data )
{
	unsigned reg = addr - fds_io_addr;
	if ( reg < fds_io_size )
	{
		if ( reg < fds_wave_size )
		{
			if ( *regs_nes (this, 0x4089) & 0x80 )
				this->regs_ [reg] = data & fds_wave_sample_max;
		}
		else
		{
			this->regs_ [reg] = data;
			switch ( addr )
			{
			case 0x4080:
				if ( data & 0x80 )
					this->env_gain = data & 0x3F;
				else
					this->env_speed = (data & 0x3F) + 1;
				break;
			
			case 0x4084:
				if ( data & 0x80 )
					this->sweep_gain = data & 0x3F;
				else
					this->sweep_speed = (data & 0x3F) + 1;
				break;
			
			case 0x4085:
				this->mod_pos = this->mod_write_pos;
				*regs_nes (this, 0x4085) = data & 0x7F;
				break;
			
			case 0x4088:
				if ( *regs_nes (this, 0x4087) & 0x80 )
				{
					int pos = this->mod_write_pos;
					data &= 0x07;
					this->mod_wave [pos    ] = data;
					this->mod_wave [pos + 1] = data;
					this->mod_write_pos = (pos     + 2) & (fds_wave_size - 1);
					this->mod_pos       = (this->mod_pos + 2) & (fds_wave_size - 1);
				}
				break;
			}
		}
	}
}

void Fds_set_tempo( struct Nes_Fds_Apu* this, int t )
{
	this->lfo_tempo = lfo_base_tempo;
	if ( t != (int)FP_ONE_TEMPO )
	{
		this->lfo_tempo = (int) ((lfo_base_tempo * FP_ONE_TEMPO) / t);
		if ( this->lfo_tempo <= 0 )
			this->lfo_tempo = 1;
	}
}

void Fds_run_until( struct Nes_Fds_Apu* this, blip_time_t final_end_time )
{
	int const wave_freq = (*regs_nes (this, 0x4083) & 0x0F) * 0x100 + *regs_nes (this, 0x4082);
	struct Blip_Buffer* const output_ = this->output_;
	if ( wave_freq && output_ && !((*regs_nes (this, 0x4089) | *regs_nes (this, 0x4083)) & 0x80) )
	{
		Blip_set_modified( output_ );
		
		// master_volume
		#define MVOL_ENTRY( percent ) (fds_master_vol_max * percent + 50) / 100
		static unsigned char const master_volumes [4] = {
			MVOL_ENTRY( 100 ), MVOL_ENTRY( 67 ), MVOL_ENTRY( 50 ), MVOL_ENTRY( 40 )
		};
		int const master_volume = master_volumes [*regs_nes (this, 0x4089) & 0x03];
		
		// lfo_period
		blip_time_t lfo_period = *regs_nes (this, 0x408A) * this->lfo_tempo;
		if ( *regs_nes (this, 0x4083) & 0x40 )
			lfo_period = 0;
		
		// sweep setup
		blip_time_t sweep_time = this->last_time + this->sweep_delay;
		blip_time_t const sweep_period = lfo_period * this->sweep_speed;
		if ( !sweep_period || *regs_nes (this, 0x4084) & 0x80 )
			sweep_time = final_end_time;
		
		// envelope setup
		blip_time_t env_time = this->last_time + this->env_delay;
		blip_time_t const env_period = lfo_period * this->env_speed;
		if ( !env_period || *regs_nes (this, 0x4080) & 0x80 )
			env_time = final_end_time;
		
		// modulation
		int mod_freq = 0;
		if ( !(*regs_nes (this, 0x4087) & 0x80) )
			mod_freq = (*regs_nes (this, 0x4087) & 0x0F) * 0x100 + *regs_nes (this, 0x4086);
		
		blip_time_t end_time = this->last_time;
		do
		{
			// sweep
			if ( sweep_time <= end_time )
			{
				sweep_time += sweep_period;
				int mode = *regs_nes (this, 0x4084) >> 5 & 2;
				int new_sweep_gain = this->sweep_gain + mode - 1;
				if ( (unsigned) new_sweep_gain <= (unsigned) 0x80 >> mode )
					this->sweep_gain = new_sweep_gain;
				else
					*regs_nes (this, 0x4084) |= 0x80; // optimization only
			}
			
			// envelope
			if ( env_time <= end_time )
			{
				env_time += env_period;
				int mode = *regs_nes (this, 0x4080) >> 5 & 2;
				int new_env_gain = this->env_gain + mode - 1;
				if ( (unsigned) new_env_gain <= (unsigned) 0x80 >> mode )
					this->env_gain = new_env_gain;
				else
					*regs_nes (this, 0x4080) |= 0x80; // optimization only
			}
			
			// new end_time
			blip_time_t const start_time = end_time;
			end_time = final_end_time;
			if ( end_time > env_time   ) end_time = env_time;
			if ( end_time > sweep_time ) end_time = sweep_time;
			
			// frequency modulation
			int freq = wave_freq;
			if ( mod_freq )
			{
				// time of next modulation clock
				blip_time_t mod_time = start_time + (this->mod_fract + mod_freq - 1) / mod_freq;
				if ( end_time > mod_time )
					end_time = mod_time;
				
				// run modulator up to next clock and save old sweep_bias
				int sweep_bias = *regs_nes (this, 0x4085);
				this->mod_fract -= (end_time - start_time) * mod_freq;
				if ( this->mod_fract <= 0 )
				{
					this->mod_fract += fract_range;
					check( (unsigned) this->mod_fract <= fract_range );
					
					static short const mod_table [8] = { 0, +1, +2, +4, 0, -4, -2, -1 };
					int mod = this->mod_wave [this->mod_pos];
					this->mod_pos = (this->mod_pos + 1) & (fds_wave_size - 1);
					int new_sweep_bias = (sweep_bias + mod_table [mod]) & 0x7F;
					if ( mod == 4 )
						new_sweep_bias = 0;
					*regs_nes (this, 0x4085) = new_sweep_bias;
				}
				
				// apply frequency modulation
				sweep_bias = (sweep_bias ^ 0x40) - 0x40;
				int factor = sweep_bias * this->sweep_gain;
				int extra = factor & 0x0F;
				factor >>= 4;
				if ( extra )
				{
					factor--;
					if ( sweep_bias >= 0 )
						factor += 3;
				}
				if ( factor > 193 ) factor -= 258;
				if ( factor < -64 ) factor += 256;
				freq += (freq * factor) >> 6;
				if ( freq <= 0 )
					continue;
			}
			
			// wave
			int wave_fract = this->wave_fract;
			blip_time_t delay = (wave_fract + freq - 1) / freq;
			blip_time_t time = start_time + delay;
			
			if ( time <= end_time )
			{
				// at least one wave clock within start_time...end_time
				
				blip_time_t const min_delay = fract_range / freq;
				int wave_pos = this->wave_pos;
				
				int volume = this->env_gain;
				if ( volume > fds_vol_max )
					volume = fds_vol_max;
				volume *= master_volume;
				
				int const min_fract = min_delay * freq;
				
				do
				{
					// clock wave
					int amp = this->regs_ [wave_pos] * volume;
					wave_pos = (wave_pos + 1) & (fds_wave_size - 1);
					int delta = amp - this->last_amp;
					if ( delta )
					{
						this->last_amp = amp;
						Synth_offset_inline( &this->synth, time, delta, output_ );
					}
					
					wave_fract += fract_range - delay * freq;
					check( unsigned (fract_range - wave_fract) < freq );
					
					// delay until next clock
					delay = min_delay;
					if ( wave_fract > min_fract )
						delay++;
					check( delay && delay == (wave_fract + freq - 1) / freq );
					
					time += delay;
				}
				while ( time <= end_time ); // TODO: using < breaks things, but <= is wrong
				
				this->wave_pos = wave_pos;
			}
			this->wave_fract = wave_fract - (end_time - (time - delay)) * freq;
			check( this->wave_fract > 0 );
		}
		while ( end_time < final_end_time );
		
		this->env_delay   = env_time   - final_end_time; check( env_delay >= 0 );
		this->sweep_delay = sweep_time - final_end_time; check( sweep_delay >= 0 );
	}
	this->last_time = final_end_time;
}
