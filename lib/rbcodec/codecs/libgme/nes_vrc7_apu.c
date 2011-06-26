
#include "nes_vrc7_apu.h"
#include "blargg_source.h"

int const period = 36; // NES CPU clocks per FM clock

void Vrc7_init( struct Nes_Vrc7_Apu* this )
{
	Synth_init( &this->synth );
	
	OPLL_new ( &this->opll, 3579545, 3579545 / 72 );
    OPLL_reset_patch( &this->opll, OPLL_VRC7_TONE );
	
	this->osc.output = 0;
	this->osc.last_amp = 0;
	this->mask = 0;

	Vrc7_volume( this, (int)FP_ONE_VOLUME );
	Vrc7_reset( this );
}

void Vrc7_reset( struct Nes_Vrc7_Apu* this )
{
	this->addr      = 0;
	this->next_time = 0;
	this->osc.last_amp = 0;

	OPLL_reset (&this->opll);	
	OPLL_setMask(&this->opll, this->mask);
}

void Vrc7_set_rate( struct Nes_Vrc7_Apu* this, int r )
{
	OPLL_set_quality( &this->opll, r < 44100 ? 0 : 1 );
}

void Vrc7_write_reg( struct Nes_Vrc7_Apu* this, int data )
{
	this->addr = data;
}

void Vrc7_run_until( struct Nes_Vrc7_Apu* this, blip_time_t end_time );
void Vrc7_write_data( struct Nes_Vrc7_Apu* this, blip_time_t time, int data )
{	
	if ( time > this->next_time )
		Vrc7_run_until( this, time );

	OPLL_writeIO( &this->opll, 0, this->addr );
	OPLL_writeIO( &this->opll, 1, data );
}

void Vrc7_end_frame( struct Nes_Vrc7_Apu* this, blip_time_t time )
{
	if ( time > this->next_time )
		Vrc7_run_until( this, time );
	
	this->next_time -= time;
	assert( this->next_time >= 0 );
	
	if ( this->osc.output )
		Blip_set_modified( this->osc.output );
}

void Vrc7_run_until( struct Nes_Vrc7_Apu* this, blip_time_t end_time )
{
	require( end_time > this->next_time );

	blip_time_t time = this->next_time;
	OPLL* opll = &this->opll; // cache
	struct Blip_Buffer* const output = this-> osc.output;
	if ( output )
	{
		do
		{
			int amp = OPLL_calc( opll ) << 1;
			int delta = amp - this->osc.last_amp;
			if ( delta )
			{
				this->osc.last_amp = amp;
				Synth_offset_inline( &this->synth, time, delta, output );
			}
			time += period;
		}
		while ( time < end_time );
	}

	this->next_time = time;
}
