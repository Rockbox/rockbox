// Gb_Snd_Emu 0.1.4. http://www.slack.net/~ant/

#include "gb_apu.h"

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

int const cgb_02 = 0; // enables bug in early CGB units that causes problems in some games
int const cgb_05 = 0; // enables CGB-05 zombie behavior

int const trigger_mask   = 0x80;
int const length_enabled = 0x40;

void Osc_reset( struct Gb_Osc* this )
{
	this->output   = NULL;
	this->last_amp = 0;
	this->delay    = 0;
	this->phase    = 0;
	this->enabled  = false;
}

static inline void Osc_update_amp( struct Gb_Osc* this, blip_time_t time, int new_amp )
{
	Blip_set_modified( this->output );
	int delta = new_amp - this->last_amp;
	if ( delta )
	{
		this->last_amp = new_amp;
		Synth_offset( this->synth, time, delta, this->output );
	}
}

// Units

void Osc_clock_length( struct Gb_Osc* this )
{
	if ( (this->regs [4] & length_enabled) && this->length_ctr )
	{
		if ( --this->length_ctr <= 0 )
			this->enabled = false;
	}
}

void Noise_clock_envelope( struct Gb_Noise* this )
{
	if ( this->env_enabled && --this->env_delay <= 0 && Noise_reload_env_timer( this ) )
	{
		int v = this->volume + (this->osc.regs [2] & 0x08 ? +1 : -1);
		if ( 0 <= v && v <= 15 )
			this->volume = v;
		else
			this->env_enabled = false;
	}
}

void Square_clock_envelope( struct Gb_Square* this )
{
	if ( this->env_enabled && --this->env_delay <= 0 && Square_reload_env_timer( this ) )
	{
		int v = this->volume + (this->osc.regs [2] & 0x08 ? +1 : -1);
		if ( 0 <= v && v <= 15 )
			this->volume = v;
		else
			this->env_enabled = false;
	}
}

static inline void reload_sweep_timer( struct Gb_Square* this )
{
	this->sweep_delay = (this->osc.regs [0] & period_mask) >> 4;
	if ( !this->sweep_delay )
		this->sweep_delay = 8;
}

static void calc_sweep( struct Gb_Square* this, bool update )
{
	struct Gb_Osc* osc = &this->osc;
	int const shift = osc->regs [0] & shift_mask;
	int const delta = this->sweep_freq >> shift;
	this->sweep_neg = (osc->regs [0] & 0x08) != 0;
	int const freq = this->sweep_freq + (this->sweep_neg ? -delta : delta);
	
	if ( freq > 0x7FF )
	{
		osc->enabled = false;
	}
	else if ( shift && update )
	{
		this->sweep_freq = freq;
		
		osc->regs [3] = freq & 0xFF;
		osc->regs [4] = (osc->regs [4] & ~0x07) | (freq >> 8 & 0x07);
	}
}

void clock_sweep( struct Gb_Square* this )
{
	if ( --this->sweep_delay <= 0 )
	{
		reload_sweep_timer( this );
		if ( this->sweep_enabled && (this->osc.regs [0] & period_mask) )
		{
			calc_sweep( this, true  );
			calc_sweep( this, false );
		}
	}
}

int wave_access( struct Gb_Wave* this, int addr )
{
	if ( this->osc.enabled )
	{
		addr = this->osc.phase & (wave_bank_size - 1);
		if ( this->osc.mode == mode_dmg )
		{
			addr++;
			if ( this->osc.delay > clk_mul )
				return -1; // can only access within narrow time window while playing
		}
		addr >>= 1;
	}
	return addr & 0x0F;
}

// write_register

static int write_trig( struct Gb_Osc* this, int frame_phase, int max_len, int old_data )
{
	int data = this->regs [4];
	
	if ( (frame_phase & 1) && !(old_data & length_enabled) && this->length_ctr )
	{
		if ( (data & length_enabled) || cgb_02 )
			this->length_ctr--;
	}
	
	if ( data & trigger_mask )
	{
		this->enabled = true;
		if ( !this->length_ctr )
		{
			this->length_ctr = max_len;
			if ( (frame_phase & 1) && (data & length_enabled) )
				this->length_ctr--;
		}
	}
	
	if ( !this->length_ctr )
		this->enabled = false;
	
	return data & trigger_mask;
}

static inline void Noise_zombie_volume( struct Gb_Noise* this, int old, int data )
{
	int v = this->volume;
	if ( this->osc.mode == mode_agb || cgb_05 )
	{
		// CGB-05 behavior, very close to AGB behavior as well
		if ( (old ^ data) & 8 )
		{
			if ( !(old & 8) )
			{
				v++;
				if ( old & 7 )
					v++;
			}
			
			v = 16 - v;
		}
		else if ( (old & 0x0F) == 8 )
		{
			v++;
		}
	}
	else
	{
		// CGB-04&02 behavior, very close to MGB behavior as well
		if ( !(old & 7) && this->env_enabled )
			v++;
		else if ( !(old & 8) )
			v += 2;
		
		if ( (old ^ data) & 8 )
			v = 16 - v;
	}
	this->volume = v & 0x0F;
}

static inline void Square_zombie_volume( struct Gb_Square* this, int old, int data )
{
	int v = this->volume;
	if ( this->osc.mode == mode_agb || cgb_05 )
	{
		// CGB-05 behavior, very close to AGB behavior as well
		if ( (old ^ data) & 8 )
		{
			if ( !(old & 8) )
			{
				v++;
				if ( old & 7 )
					v++;
			}
			
			v = 16 - v;
		}
		else if ( (old & 0x0F) == 8 )
		{
			v++;
		}
	}
	else
	{
		// CGB-04&02 behavior, very close to MGB behavior as well
		if ( !(old & 7) && this->env_enabled )
			v++;
		else if ( !(old & 8) )
			v += 2;
		
		if ( (old ^ data) & 8 )
			v = 16 - v;
	}
	this->volume = v & 0x0F;
}

static bool Square_write_register( struct Gb_Square* this, int frame_phase, int reg, int old_data, int data )
{
	int const max_len = 64;
	
	switch ( reg )
	{
	case 1:
		this->osc.length_ctr = max_len - (data & (max_len - 1));
		break;
	
	case 2:
		if ( !Square_dac_enabled( this ) )
			this->osc.enabled = false;
		
		Square_zombie_volume( this, old_data, data );
		
		if ( (data & 7) && this->env_delay == 8 )
		{
			this->env_delay = 1;
			Square_clock_envelope( this ); // TODO: really happens at next length clock
		}
		break;
	
	case 4:
		if ( write_trig( &this->osc, frame_phase, max_len, old_data ) )
		{
			this->volume = this->osc.regs [2] >> 4;
			Square_reload_env_timer( this );
			this->env_enabled = true;
			if ( frame_phase == 7 )
				this->env_delay++;
			if ( !Square_dac_enabled( this ) )
				this->osc.enabled = false;
			this->osc.delay = (this->osc.delay & (4 * clk_mul - 1)) + Square_period( this );
			return true;
		}
	}
	
	return false;
}

static inline void Noise_write_register( struct Gb_Noise* this, int frame_phase, int reg, int old_data, int data )
{
	int const max_len = 64;
	
	switch ( reg )
	{
	case 1:
		this->osc.length_ctr = max_len - (data & (max_len - 1));
		break;
	
	case 2:
		if ( !Noise_dac_enabled( this ) )
			this->osc.enabled = false;
		
		Noise_zombie_volume( this, old_data, data );
		
		if ( (data & 7) && this->env_delay == 8 )
		{
			this->env_delay = 1;
			Noise_clock_envelope( this ); // TODO: really happens at next length clock
		}
		break;
	
	case 4:
		if ( write_trig( &this->osc, frame_phase, max_len, old_data ) )
		{
			this->volume = this->osc.regs [2] >> 4;
			Noise_reload_env_timer( this );
			this->env_enabled = true;
			if ( frame_phase == 7 )
				this->env_delay++;
			if ( !Noise_dac_enabled( this ) )
				this->osc.enabled = false;
				
			this->osc.phase = 0x7FFF;
			this->osc.delay += 8 * clk_mul;
		}
	}
}

static inline void Sweep_write_register( struct Gb_Square* this, int frame_phase, int reg, int old_data, int data )
{
	if ( reg == 0 && this->sweep_enabled && this->sweep_neg && !(data & 0x08) )
		this->osc.enabled = false; // sweep negate disabled after used
	
	if ( Square_write_register( this, frame_phase, reg, old_data, data ) )
	{
		this->sweep_freq = Osc_frequency( &this->osc );
		this->sweep_neg = false;
		reload_sweep_timer( this );
		this->sweep_enabled = (this->osc.regs [0] & (period_mask | shift_mask)) != 0;
		if ( this->osc.regs [0] & shift_mask )
			calc_sweep( this, false );
	}
}

static void corrupt_wave( struct Gb_Wave* this )
{
	int pos = ((this->osc.phase + 1) & (wave_bank_size - 1)) >> 1;
	if ( pos < 4 )
		this->wave_ram [0] = this->wave_ram [pos];
	else {
		int i;
		for ( i = 4; --i >= 0; )
			this->wave_ram [i] = this->wave_ram [(pos & ~3) + i];
	}
}

static inline void Wave_write_register( struct Gb_Wave* this, int frame_phase, int reg, int old_data, int data )
{
	int const max_len = 256;
	
	switch ( reg )
	{
	case 0:
		if ( !Wave_dac_enabled( this ) )
			this->osc.enabled = false;
		break;
	
	case 1:
		this->osc.length_ctr = max_len - data;
		break;
	
	case 4:
		{
			bool was_enabled = this->osc.enabled;
			if ( write_trig( &this->osc, frame_phase, max_len, old_data ) )
			{
				if ( !Wave_dac_enabled( this ) )
					this->osc.enabled = false;
				else if ( this->osc.mode == mode_dmg && was_enabled &&
						(unsigned) (this->osc.delay - 2 * clk_mul) < 2 * clk_mul )
					corrupt_wave( this );
			
				this->osc.phase = 0;
				this->osc.delay = Wave_period( this ) + 6 * clk_mul;
			}
		}
	}
}

void write_osc( struct Gb_Apu* this, int reg, int old_data, int data )
{
	int index = (reg * 3 + 3) >> 4; // avoids divide
	assert( index == reg / 5 );
	reg -= index * 5;
	switch ( index )
	{
	case 0: Sweep_write_register ( &this->square1, this->frame_phase, reg, old_data, data ); break;
	case 1: Square_write_register( &this->square2, this->frame_phase, reg, old_data, data ); break;
	case 2: Wave_write_register  ( &this->wave, this->frame_phase, reg, old_data, data ); break;
	case 3: Noise_write_register ( &this->noise, this->frame_phase, reg, old_data, data ); break;
	}
}

// Synthesis

void Square_run( struct Gb_Square* this, blip_time_t time, blip_time_t end_time )
{
	// Calc duty and phase
	static byte const duty_offsets [4] = { 1, 1, 3, 7 };
	static byte const duties       [4] = { 1, 2, 4, 6 };

	struct Gb_Osc* osc = &this->osc;
	int const duty_code = osc->regs [1] >> 6;
	int duty_offset = duty_offsets [duty_code];
	int duty = duties [duty_code];
	if ( osc->mode == mode_agb )
	{
		// AGB uses inverted duty
		duty_offset -= duty;
		duty = 8 - duty;
	}
	int ph = (osc->phase + duty_offset) & 7;
	
	// Determine what will be generated
	int vol = 0;
	struct Blip_Buffer* const out = osc->output;
	if ( out )
	{
		int amp = osc->dac_off_amp;
		if ( Square_dac_enabled( this ) )
		{
			if ( osc->enabled )
				vol = this->volume;
			
			amp = -dac_bias;
			if ( osc->mode == mode_agb )
				amp = -(vol >> 1);
			
			// Play inaudible frequencies as constant amplitude
			if ( Osc_frequency( osc ) >= 0x7FA && osc->delay < 32 * clk_mul )
			{
				amp += (vol * duty) >> 3;
				vol = 0;
			}
			
			if ( ph < duty )
			{
				amp += vol;
				vol = -vol;
			}
		}
		Osc_update_amp( osc, time, amp );
	}
	
	// Generate wave
	time += osc->delay;
	if ( time < end_time )
	{
		int const per = Square_period( this );
		if ( !vol )
		{
			#ifdef GB_APU_FAST
				time = end_time;
			#else
				// Maintain phase when not playing
				int count = (end_time - time + per - 1) / per;
				ph += count; // will be masked below
				time += (blip_time_t) count * per;
			#endif
		}
		else
		{
			// Output amplitude transitions
			int delta = vol;
			do
			{
				ph = (ph + 1) & 7;
				if ( ph == 0 || ph == duty )
				{
					Synth_offset_inline( osc->synth, time, delta, out );
					delta = -delta;
				}
				time += per;
			}
			while ( time < end_time );
			
			if ( delta != vol )
				osc->last_amp -= delta;
		}
		osc->phase = (ph - duty_offset) & 7;
	}
	osc->delay = time - end_time;
}

#ifndef GB_APU_FAST
// Quickly runs LFSR for a large number of clocks. For use when noise is generating
// no sound.
static unsigned run_lfsr( unsigned s, unsigned mask, int count )
{
	bool const optimized = true; // set to false to use only unoptimized loop in middle
	
	// optimization used in several places:
	// ((s & (1 << b)) << n) ^ ((s & (1 << b)) << (n + 1)) = (s & (1 << b)) * (3 << n)
	
	if ( mask == 0x4000 && optimized )
	{
		if ( count >= 32767 )
			count %= 32767;
		
		// Convert from Fibonacci to Galois configuration,
		// shifted left 1 bit
		s ^= (s & 1) * 0x8000;
		
		// Each iteration is equivalent to clocking LFSR 255 times
		while ( (count -= 255) > 0 )
			s ^= ((s & 0xE) << 12) ^ ((s & 0xE) << 11) ^ (s >> 3);
		count += 255;
		
		// Each iteration is equivalent to clocking LFSR 15 times
		// (interesting similarity to single clocking below)
		while ( (count -= 15) > 0 )
			s ^= ((s & 2) * (3 << 13)) ^ (s >> 1);
		count += 15;
		
		// Remaining singles
		while ( --count >= 0 )
			s = ((s & 2) * (3 << 13)) ^ (s >> 1);
		
		// Convert back to Fibonacci configuration
		s &= 0x7FFF;
	}
	else if ( count < 8 || !optimized )
	{
		// won't fully replace upper 8 bits, so have to do the unoptimized way
		while ( --count >= 0 )
			s = (s >> 1 | mask) ^ (mask & -((s - 1) & 2));
	}
	else
	{
		if ( count > 127 )
		{
			count %= 127;
			if ( !count )
				count = 127; // must run at least once
		}
		
		// Need to keep one extra bit of history
		s = s << 1 & 0xFF;
		
		// Convert from Fibonacci to Galois configuration,
		// shifted left 2 bits
		s ^= (s & 2) * 0x80;
		
		// Each iteration is equivalent to clocking LFSR 7 times
		// (interesting similarity to single clocking below)
		while ( (count -= 7) > 0 )
			s ^= ((s & 4) * (3 << 5)) ^ (s >> 1);
		count += 7;
		
		// Remaining singles
		while ( --count >= 0 )
			s = ((s & 4) * (3 << 5)) ^ (s >> 1);
		
		// Convert back to Fibonacci configuration and
		// repeat last 8 bits above significant 7
		s = (s << 7 & 0x7F80) | (s >> 1 & 0x7F);
	}
	
	return s;
}
#endif

void Noise_run( struct Gb_Noise* this, blip_time_t time, blip_time_t end_time )
{
	// Determine what will be generated
	int vol = 0;
	struct Gb_Osc* osc = &this->osc;
	struct Blip_Buffer* const out = osc->output;
	if ( out )
	{
		int amp = osc->dac_off_amp;
		if ( Noise_dac_enabled( this ) )
		{
			if ( osc->enabled )
				vol = this->volume;
			
			amp = -dac_bias;
			if ( osc->mode == mode_agb )
				amp = -(vol >> 1);
			
			if ( !(osc->phase & 1) )
			{
				amp += vol;
				vol = -vol;
			}
		}
		
		// AGB negates final output
		if ( osc->mode == mode_agb )
		{
			vol = -vol;
			amp    = -amp;
		}
		
		Osc_update_amp( osc, time, amp );
	}
	
	// Run timer and calculate time of next LFSR clock
	static byte const period1s [8] = { 1, 2, 4, 6, 8, 10, 12, 14 };
	int const period1 = period1s [osc->regs [3] & 7] * clk_mul;
	
	#ifdef GB_APU_FAST
		time += delay;
	#else
		{
			int extra = (end_time - time) - osc->delay;
			int const per2 = period2( this, 8 );
			time += osc->delay + ((this->divider ^ (per2 >> 1)) & (per2 - 1)) * period1;
			
			int count = (extra < 0 ? 0 : (extra + period1 - 1) / period1);
			this->divider = (this->divider - count) & period2_mask;
			osc->delay = count * period1 - extra;
		}
	#endif
	
	// Generate wave
	if ( time < end_time )
	{
		unsigned const mask = lfsr_mask( this );
		unsigned bits = osc->phase;
		
		int per = period2( this, period1 * 8 );
		#ifdef GB_APU_FAST
			// Noise can be THE biggest time hog; adjust as necessary
			int const min_period = 24;
			if ( per < min_period )
				per = min_period;
		#endif
		if ( period2_index( this ) >= 0xE )
		{
			time = end_time;
		}
		else if ( !vol )
		{
			#ifdef GB_APU_FAST
				time = end_time;
			#else
				// Maintain phase when not playing
				int count = (end_time - time + per - 1) / per;
				time += (blip_time_t) count * per;
				bits = run_lfsr( bits, ~mask, count );
			#endif
		}
		else
		{
			struct Blip_Synth* synth = osc->synth; // cache
			
			// Output amplitude transitions
			int delta = -vol;
			do
			{
				unsigned changed = bits + 1;
				bits = bits >> 1 & mask;
				if ( changed & 2 )
				{
					bits |= ~mask;
					delta = -delta;
					Synth_offset_inline( synth, time, delta, out );
				}
				time += per;
			}
			while ( time < end_time );
			
			if ( delta == vol )
				osc->last_amp += delta;
		}
		osc->phase = bits;
	}
	
	#ifdef GB_APU_FAST
		osc->delay = time - end_time;
	#endif
}

void Wave_run( struct Gb_Wave* this, blip_time_t time, blip_time_t end_time )
{
	// Calc volume
#ifdef GB_APU_NO_AGB
	static byte const shifts [4] = { 4+4, 0+4, 1+4, 2+4 };
	int const volume_idx = this->regs [2] >> 5 & 3;
	int const volume_shift = shifts [volume_idx];
	int const volume_mul = 1;
#else
	static byte const volumes [8] = { 0, 4, 2, 1, 3, 3, 3, 3 };
	int const volume_shift = 2 + 4;
	int const volume_idx = this->osc.regs [2] >> 5 & (this->agb_mask | 3); // 2 bits on DMG/CGB, 3 on AGB
	int const volume_mul = volumes [volume_idx];
#endif

	// Determine what will be generated
	int playing = false;
	struct Gb_Osc* osc = &this->osc;
	struct Blip_Buffer* out = osc->output;
	if ( out )
	{
		int amp = osc->dac_off_amp;
		if ( Wave_dac_enabled( this ) )
		{
			// Play inaudible frequencies as constant amplitude
			amp = 8 << 4; // really depends on average of all samples in wave
			
			// if delay is larger, constant amplitude won't start yet
			if ( Osc_frequency( osc ) <= 0x7FB || osc->delay > 15 * clk_mul )
			{
				if ( volume_mul && volume_shift != 4+4 )
					playing = (int) osc->enabled;
				
				amp = (this->sample_buf << (osc->phase << 2 & 4) & 0xF0) * playing;
			}
			
			amp = ((amp * volume_mul) >> volume_shift) - dac_bias;
		}
		Osc_update_amp( osc, time, amp );
	}
	
	// Generate wave
	time += osc->delay;
	if ( time < end_time )
	{
		byte const* wave = this->wave_ram;
		
		// wave size and bank
	#ifdef GB_APU_NO_AGB
		int const wave_mask = 0x1F;
		int const swap_banks = 0;
	#else
		int const size20_mask = 0x20;
		int const flags = osc->regs [0] & this->agb_mask;
		int const wave_mask = (flags & size20_mask) | 0x1F;
		int swap_banks = 0;
		if ( flags & bank40_mask )
		{
			swap_banks = flags & size20_mask;
			wave += wave_bank_size/2 - (swap_banks >> 1);
		}
	#endif
		
		int ph = osc->phase ^ swap_banks;
		ph = (ph + 1) & wave_mask; // pre-advance
		
		int const per = Wave_period( this );
		if ( !playing )
		{
			#ifdef GB_APU_FAST
				time = end_time;
			#else
				// Maintain phase when not playing
				int count = (end_time - time + per - 1) / per;
				ph += count; // will be masked below
				time += (blip_time_t) count * per;
			#endif
		}
		else
		{
			struct Blip_Synth* synth = osc->synth; // cache
			
			// Output amplitude transitions
			int lamp = osc->last_amp + dac_bias;
			do
			{
				// Extract nibble
				int nibble = wave [ph >> 1] << (ph << 2 & 4) & 0xF0;
				ph = (ph + 1) & wave_mask;
				
				// Scale by volume
				int amp = (nibble * volume_mul) >> volume_shift;
				
				int delta = amp - lamp;
				if ( delta )
				{
					lamp = amp;
					Synth_offset_inline( synth, time, delta, out );
				}
				time += per;
			}
			while ( time < end_time );
			osc->last_amp = lamp - dac_bias;
		}
		ph = (ph - 1) & wave_mask; // undo pre-advance and mask position
		
		// Keep track of last byte read
		if ( osc->enabled )
			this->sample_buf = wave [ph >> 1];
		
		osc->phase = ph ^ swap_banks; // undo swapped banks
	}
	osc->delay = time - end_time;
}
