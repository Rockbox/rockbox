// Nes_Snd_Emu 0.1.8. http://www.slack.net/~ant/

#include "nes_apu.h"

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

// Nes_Osc

void Osc_clock_length( struct Nes_Osc* this, int halt_mask )
{
	if ( this->length_counter && !(this->regs [0] & halt_mask) )
		this->length_counter--;
}

// Nes_Square

void Square_clock_envelope( struct Nes_Square* this )
{
	struct Nes_Osc* osc = &this->osc;
	int period = osc->regs [0] & 15;
	if ( osc->reg_written [3] ) {
		osc->reg_written [3] = false;
		this->env_delay = period;
		this->envelope = 15;
	}
	else if ( --this->env_delay < 0 ) {
		this->env_delay = period;
		if ( this->envelope | (osc->regs [0] & 0x20) )
			this->envelope = (this->envelope - 1) & 15;
	}
}

int Square_volume( struct Nes_Square* this )
{
	struct Nes_Osc* osc = &this->osc;
	return osc->length_counter == 0 ? 0 : (osc->regs [0] & 0x10) ? (osc->regs [0] & 15) : this->envelope;
}

void Square_clock_sweep( struct Nes_Square* this, int negative_adjust )
{
	struct Nes_Osc* osc = &this->osc;
	int sweep = osc->regs [1];
	
	if ( --this->sweep_delay < 0 )
	{
		osc->reg_written [1] = true;
		
		int period = Osc_period( osc );
		int shift = sweep & shift_mask;
		if ( shift && (sweep & 0x80) && period >= 8 )
		{
			int offset = period >> shift;
			
			if ( sweep & negate_flag )
				offset = negative_adjust - offset;
			
			if ( period + offset < 0x800 )
			{
				period += offset;
				// rewrite period
				osc->regs [2] = period & 0xFF;
				osc->regs [3] = (osc->regs [3] & ~7) | ((period >> 8) & 7);
			}
		}
	}
	
	if ( osc->reg_written [1] ) {
		osc->reg_written [1] = false;
		this->sweep_delay = (sweep >> 4) & 7;
	}
}

// TODO: clean up
static inline nes_time_t Square_maintain_phase( struct Nes_Square* this, nes_time_t time, nes_time_t end_time,
		nes_time_t timer_period )
{
	nes_time_t remain = end_time - time;
	if ( remain > 0 )
	{
		int count = (remain + timer_period - 1) / timer_period;
		this->phase = (this->phase + count) & (square_phase_range - 1);
		time += count * timer_period;
	}
	return time;
}

void Square_run( struct Nes_Square* this, nes_time_t time, nes_time_t end_time )
{
	struct Nes_Osc* osc = &this->osc;
	const int period = Osc_period( osc );
	const int timer_period = (period + 1) * 2;
	
	if ( !osc->output )
	{
		osc->delay = Square_maintain_phase( this, time + osc->delay, end_time, timer_period ) - end_time;
		return;
	}
	
	int offset = period >> (osc->regs [1] & shift_mask);
	if ( osc->regs [1] & negate_flag )
		offset = 0;
	
	const int volume = Square_volume( this );
	if ( volume == 0 || period < 8 || (period + offset) >= 0x800 )
	{
		if ( osc->last_amp ) {
			Blip_set_modified( osc->output );
			Synth_offset( this->synth, time, -osc->last_amp, osc->output );
			osc->last_amp = 0;
		}
		
		time += osc->delay;
		time = Square_maintain_phase( this, time, end_time, timer_period );
	}
	else
	{
		// handle duty select
		int duty_select = (osc->regs [0] >> 6) & 3;
		int duty = 1 << duty_select; // 1, 2, 4, 2
		int amp = 0;
		if ( duty_select == 3 ) {
			duty = 2; // negated 25%
			amp = volume;
		}
		if ( this->phase < duty )
			amp ^= volume;
		
		Blip_set_modified( osc->output );
		{
			int delta = Osc_update_amp( osc, amp );
			if ( delta )
				Synth_offset( this->synth, time, delta, osc->output );
		}
		
		time += osc->delay;
		if ( time < end_time )
		{
			struct Blip_Buffer* const output = osc->output;
			Synth* synth = this->synth;
			int delta = amp * 2 - volume;
			int phase = this->phase;
			
			do {
				phase = (phase + 1) & (square_phase_range - 1);
				if ( phase == 0 || phase == duty ) {
					delta = -delta;
					Synth_offset_inline( synth, time, delta, output );
				}
				time += timer_period;
			}
			while ( time < end_time );
			
			osc->last_amp = (delta + volume) >> 1;
			this->phase = phase;
		}
	}
	
	osc->delay = time - end_time;
}

// Nes_Triangle

void Triangle_clock_linear_counter( struct Nes_Triangle* this )
{
	struct Nes_Osc* osc = &this->osc;
	if ( osc->reg_written [3] )
		this->linear_counter = osc->regs [0] & 0x7F;
	else if ( this->linear_counter )
		this->linear_counter--;
	
	if ( !(osc->regs [0] & 0x80) )
		osc->reg_written [3] = false;
}

static inline int Triangle_calc_amp( struct Nes_Triangle* this )
{
	int amp = Triangle_phase_range - this->phase;
	if ( amp < 0 )
		amp = this->phase - (Triangle_phase_range + 1);
	return amp;
}

// TODO: clean up
static inline nes_time_t Triangle_maintain_phase( struct Nes_Triangle* this, nes_time_t time, nes_time_t end_time,
		nes_time_t timer_period )
{
	nes_time_t remain = end_time - time;
	if ( remain > 0 )
	{
		int count = (remain + timer_period - 1) / timer_period;
		this->phase = ((unsigned) this->phase + 1 - count) & (Triangle_phase_range * 2 - 1);
		this->phase++;
		time += count * timer_period;
	}
	return time;
}

void Triangle_run( struct Nes_Triangle* this, nes_time_t time, nes_time_t end_time )
{
	struct Nes_Osc* osc = &this->osc;
	const int timer_period = Osc_period( osc ) + 1;
	if ( !osc->output )
	{
		time += osc->delay;
		osc->delay = 0;
		if ( osc->length_counter && this->linear_counter && timer_period >= 3 )
			osc->delay = Triangle_maintain_phase( this, time, end_time, timer_period ) - end_time;
		return;
	}
	
	// to do: track phase when period < 3
	// to do: Output 7.5 on dac when period < 2? More accurate, but results in more clicks.
	
	int delta = Osc_update_amp( osc, Triangle_calc_amp( this ) );
	if ( delta )
	{
		Blip_set_modified( osc->output );
		Synth_offset( &this->synth, time, delta, osc->output );
	}
	
	time += osc->delay;
	if ( osc->length_counter == 0 || this->linear_counter == 0 || timer_period < 3 )
	{
		time = end_time;
	}
	else if ( time < end_time )
	{
		struct Blip_Buffer* const output = osc->output;
		
		int phase = this->phase;
		int volume = 1;
		if ( phase > Triangle_phase_range ) {
			phase -= Triangle_phase_range;
			volume = -volume;
		}
		Blip_set_modified( osc->output );
		
		do {
			if ( --phase == 0 ) {
				phase = Triangle_phase_range;
				volume = -volume;
			}
			else 
			{
				Synth_offset_inline( &this->synth, time, volume, output );
			}
			
			time += timer_period;
		}
		while ( time < end_time );
		
		if ( volume < 0 )
			phase += Triangle_phase_range;
		this->phase = phase;
		osc->last_amp = Triangle_calc_amp( this );
 	}
	osc->delay = time - end_time;
}

// Nes_Dmc

void Dmc_reset( struct Nes_Dmc* this )
{
	this->address = 0;
	this->dac = 0;
	this->buf = 0;
	this->bits_remain = 1;
	this->bits = 0;
	this->buf_full = false;
	this->silence = true;
	this->next_irq = apu_no_irq;
	this->irq_flag = false;
	this->irq_enabled = false;
	
	Osc_reset( &this->osc );
	this->period = 0x1AC;
}

void Dmc_recalc_irq( struct Nes_Dmc* this )
{
	struct Nes_Osc* osc = &this->osc;
	nes_time_t irq = apu_no_irq;
	if ( this->irq_enabled && osc->length_counter )
		irq = this->apu->last_dmc_time + osc->delay +
				((osc->length_counter - 1) * 8 + this->bits_remain - 1) * (nes_time_t) (this->period) + 1;
	if ( irq != this->next_irq ) {
		this->next_irq = irq;
		Apu_irq_changed( this->apu );
	}
}

int Dmc_count_reads( struct Nes_Dmc* this, nes_time_t time, nes_time_t* last_read )
{
	struct Nes_Osc* osc = &this->osc;
	if ( last_read )
		*last_read = time;
	
	if ( osc->length_counter == 0 )
		return 0; // not reading
	
	nes_time_t first_read = Dmc_next_read_time( this );
	nes_time_t avail = time - first_read;
	if ( avail <= 0 )
		return 0;
	
	int count = (avail - 1) / (this->period * 8) + 1;
	if ( !(osc->regs [0] & loop_flag) && count > osc->length_counter )
		count = osc->length_counter;
	
	if ( last_read )
	{
		*last_read = first_read + (count - 1) * (this->period * 8) + 1;
		check( *last_read <= time );
		check( count == count_reads( *last_read, NULL ) );
		check( count - 1 == count_reads( *last_read - 1, NULL ) );
	}
	
	return count;
}

static short const dmc_period_table [2] [16] = {
	{428, 380, 340, 320, 286, 254, 226, 214, // NTSC
	190, 160, 142, 128, 106,  84,  72,  54},

	{398, 354, 316, 298, 276, 236, 210, 198, // PAL
	176, 148, 132, 118,  98,  78,  66,  50}
};

static inline void Dmc_reload_sample( struct Nes_Dmc* this )
{
	this->address = 0x4000 + this->osc.regs [2] * 0x40;
	this->osc.length_counter = this->osc.regs [3] * 0x10 + 1;
}

static int const dmc_table [128] =
{
   0,  24,  48,  71,  94, 118, 141, 163, 186, 209, 231, 253, 275, 297, 319, 340,
 361, 383, 404, 425, 445, 466, 486, 507, 527, 547, 567, 587, 606, 626, 645, 664,
 683, 702, 721, 740, 758, 777, 795, 813, 832, 850, 867, 885, 903, 920, 938, 955,
 972, 989,1006,1023,1040,1056,1073,1089,1105,1122,1138,1154,1170,1185,1201,1217,
1232,1248,1263,1278,1293,1308,1323,1338,1353,1368,1382,1397,1411,1425,1440,1454,
1468,1482,1496,1510,1523,1537,1551,1564,1578,1591,1604,1618,1631,1644,1657,1670,
1683,1695,1708,1721,1733,1746,1758,1771,1783,1795,1807,1819,1831,1843,1855,1867,
1879,1890,1902,1914,1925,1937,1948,1959,1971,1982,1993,2004,2015,2026,2037,2048,
};

static inline int update_amp_nonlinear( struct Nes_Dmc* this, int in )
{
	if ( !this->nonlinear )
		in = dmc_table [in];
	int delta = in - this->osc.last_amp;
	this->osc.last_amp = in;
	return delta;
}

void Dmc_write_register( struct Nes_Dmc* this, int addr, int data )
{
	if ( addr == 0 )
	{
		this->period = dmc_period_table [this->pal_mode] [data & 15];
		this->irq_enabled = (data & 0xC0) == 0x80; // enabled only if loop disabled
		this->irq_flag &= this->irq_enabled;
		Dmc_recalc_irq( this );
	}
	else if ( addr == 1 )
	{
		this->dac = data & 0x7F;
	}
}

void Dmc_start( struct Nes_Dmc* this )
{
	Dmc_reload_sample( this );
	Dmc_fill_buffer( this );
	Dmc_recalc_irq( this );
}

void Dmc_fill_buffer( struct Nes_Dmc* this )
{
	if ( !this->buf_full && this->osc.length_counter )
	{
		require( this->prg_reader ); // prg_reader must be set
		this->buf = this->prg_reader( this->prg_reader_data, 0x8000u + this->address );
		this->address = (this->address + 1) & 0x7FFF;
		this->buf_full = true;
		if ( --this->osc.length_counter == 0 )
		{
			if ( this->osc.regs [0] & loop_flag ) {
				Dmc_reload_sample( this );
			}
			else {
				this->apu->osc_enables &= ~0x10;
				this->irq_flag = this->irq_enabled;
				this->next_irq = apu_no_irq;
				Apu_irq_changed( this->apu );
			}
		}
	}
}

void Dmc_run( struct Nes_Dmc* this, nes_time_t time, nes_time_t end_time )
{
	struct Nes_Osc* osc = &this->osc;
	int delta = update_amp_nonlinear( this, this->dac );
	if ( !osc->output )
	{
		this->silence = true;
	}
	else if ( delta )
	{
		Blip_set_modified( osc->output );
		Synth_offset( &this->synth, time, delta, osc->output );
	}
	
	time += osc->delay;
	if ( time < end_time )
	{
		int bits_remain = this->bits_remain;
		if ( this->silence && !this->buf_full )
		{
			int count = (end_time - time + this->period - 1) / this->period;
			bits_remain = (bits_remain - 1 + 8 - (count % 8)) % 8 + 1;
			time += count * this->period;
		}
		else
		{
			struct Blip_Buffer* const output = osc->output;
			const int period = this->period;
			int bits = this->bits;
			int dac = this->dac;
			if ( output )
				Blip_set_modified( output );
			
			do
			{
				if ( !this->silence )
				{
					int step = (bits & 1) * 4 - 2;
					bits >>= 1;
					if ( (unsigned) (dac + step) <= 0x7F ) {
						dac += step;
						Synth_offset_inline( &this->synth, time, update_amp_nonlinear( this, dac ), output );
					}
				}
				
				time += period;
				
				if ( --bits_remain == 0 )
				{
					bits_remain = 8;
					if ( !this->buf_full ) {
						this->silence = true;
					}
					else
					{
						this->silence = false;
						bits = this->buf;
						this->buf_full = false;
						if ( !output )
							this->silence = true;
						Dmc_fill_buffer( this );
					}
				}
			}
			while ( time < end_time );
			
			this->dac = dac;
			//osc->last_amp = dac;
			this->bits = bits;
		}
		this->bits_remain = bits_remain;
	}
	osc->delay = time - end_time;
}

// Nes_Noise

static short const noise_period_table [16] = {
	0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
	0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4
};

void Noise_clock_envelope( struct Nes_Noise* this )
{
	struct Nes_Osc* osc = &this->osc;
	int period = osc->regs [0] & 15;
	if ( osc->reg_written [3] ) {
		osc->reg_written [3] = false;
		this->env_delay = period;
		this->envelope = 15;
	}
	else if ( --this->env_delay < 0 ) {
		this->env_delay = period;
		if ( this->envelope | (osc->regs [0] & 0x20) )
			this->envelope = (this->envelope - 1) & 15;
	}
}

int Noise_volume( struct Nes_Noise* this )
{
	struct Nes_Osc* osc = &this->osc;
	return osc->length_counter == 0 ? 0 : (osc->regs [0] & 0x10) ? (osc->regs [0] & 15) : this->envelope;
}

void Noise_run( struct Nes_Noise* this, nes_time_t time, nes_time_t end_time )
{
	struct Nes_Osc* osc = &this->osc;
	int period = noise_period_table [osc->regs [2] & 15];
	
	if ( !osc->output )
	{
		// TODO: clean up
		time += osc->delay;
		osc->delay = time + (end_time - time + period - 1) / period * period - end_time;
		return;
	}
	
	const int volume = Noise_volume( this );
	int amp = (this->noise & 1) ? volume : 0;
	{
		int delta = Osc_update_amp( osc, amp );
		if ( delta )
		{
			Blip_set_modified( osc->output );
			Synth_offset( &this->synth, time, delta, osc->output );
		}
	}
	
	time += osc->delay;
	if ( time < end_time )
	{
		const int mode_flag = 0x80;
		
		if ( !volume )
		{
			// round to next multiple of period
			time += (end_time - time + period - 1) / period * period;
			
			// approximate noise cycling while muted, by shuffling up noise register
			// to do: precise muted noise cycling?
			if ( !(osc->regs [2] & mode_flag) ) {
				int feedback = (this->noise << 13) ^ (this->noise << 14);
				this->noise = (feedback & 0x4000) | (this->noise >> 1);
			}
		}
		else
		{
			struct Blip_Buffer* const output = osc->output;
			
			// using resampled time avoids conversion in synth.offset()
			blip_resampled_time_t rperiod = Blip_resampled_duration( output, period );
			blip_resampled_time_t rtime = Blip_resampled_time( output, time );
			
			int noise = this->noise;
			int delta = amp * 2 - volume;
			const int tap = (osc->regs [2] & mode_flag ? 8 : 13);
			Blip_set_modified( output );
			
			do {
				int feedback = (noise << tap) ^ (noise << 14);
				time += period;
				
				if ( (noise + 1) & 2 ) {
					// bits 0 and 1 of noise differ
					delta = -delta;
					Synth_offset_resampled( &this->synth, rtime, delta, output );
				}
				
				rtime += rperiod;
				noise = (feedback & 0x4000) | (noise >> 1);
			}
			while ( time < end_time );
			
			osc->last_amp = (delta + volume) >> 1;
			this->noise = noise;
		}
	}
	
	osc->delay = time - end_time;
}

