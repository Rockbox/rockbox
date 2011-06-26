#include "sms_fm_apu.h"

#include "blargg_source.h"

void Fm_apu_create( struct Sms_Fm_Apu* this )
{ 
	Synth_init( &this->synth );
	Ym2413_init( &this->apu );
}

blargg_err_t Fm_apu_init( struct Sms_Fm_Apu* this, int clock_rate, int sample_rate )
{
	this->period_ = (blip_time_t) (clock_rate / sample_rate);
	CHECK_ALLOC( !Ym2413_set_rate( &this->apu, sample_rate, clock_rate ) );
	
	Fm_apu_set_output( this, 0 );
	Fm_apu_volume( this, (int)FP_ONE_VOLUME );
	Fm_apu_reset( this );
	return 0;
}

void Fm_apu_reset( struct Sms_Fm_Apu* this )
{
	this->addr      = 0;
	this->next_time = 0;
	this->last_amp  = 0;
	
	Ym2413_reset( &this->apu );
}

void fm_run_until( struct Sms_Fm_Apu* this, blip_time_t end_time );
void Fm_apu_write_data( struct Sms_Fm_Apu* this, blip_time_t time, int data )
{
	if ( time > this->next_time )
		fm_run_until( this, time );
	
	Ym2413_write( &this->apu, this->addr, data );
}

void fm_run_until( struct Sms_Fm_Apu* this, blip_time_t end_time )
{
	assert( end_time > this->next_time );
	
	struct Blip_Buffer* const output = this->output_;
	if ( !output )
	{
		this->next_time = end_time;
		return;
	}
	
	blip_time_t time = this->next_time;
	struct Ym2413_Emu* emu = &this->apu;
	do
	{
		short samples [2];
		Ym2413_run( emu, 1, samples );
		int amp = (samples [0] + samples [1]) >> 1;
		
		int delta = amp - this->last_amp;
		if ( delta )
		{
			this->last_amp = amp;
			Synth_offset_inline( &this->synth, time, delta, output );
		}
		time += this->period_;
	}
	while ( time < end_time );
	
	this->next_time = time;
}

void Fm_apu_end_frame( struct Sms_Fm_Apu* this, blip_time_t time )
{
	if ( time > this->next_time )
		fm_run_until( this, time );
	
	this->next_time -= time;
	assert( this->next_time >= 0 );
	
	if ( this->output_ )
		Blip_set_modified( this->output_ );
}
