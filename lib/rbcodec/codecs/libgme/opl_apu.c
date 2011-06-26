#include "opl_apu.h"

#include "blargg_source.h"

/* NOTE: Removed unused chips ~ gama */

blargg_err_t Opl_init( struct Opl_Apu* this, long clock, long rate, blip_time_t period, enum opl_type_t type )
{
	Synth_init( &this->synth );

	this->type_ = type;
	this->clock_ = clock;
	this->rate_ = rate;
	this->period_ = period;
	Opl_set_output( this, 0 );
	Opl_volume( this, (int)FP_ONE_VOLUME );

	switch (type)
	{
	case type_opll:
	case type_msxmusic:
	case type_smsfmunit:
		OPLL_new ( &this->opll, clock, rate );
    	OPLL_reset_patch( &this->opll, OPLL_2413_TONE );
		break;
	case type_vrc7:
		OPLL_new ( &this->opll, clock, rate );
		OPLL_reset_patch( &this->opll, OPLL_VRC7_TONE );
		break;
	case type_msxaudio:
		OPL_init( &this->opl, this->opl_memory, sizeof this->opl_memory );
		OPL_setSampleRate( &this->opl, rate, clock );
		OPL_setInternalVolume(&this->opl, 1 << 13);
		break;
	}

	Opl_reset( this );
	return 0;
}

void Opl_shutdown( struct Opl_Apu* this )
{
	switch (this->type_)
	{
	case type_opll:
	case type_msxmusic:
	case type_smsfmunit:
	case type_vrc7:
		OPLL_delete( &this->opll );
		break;
	case type_msxaudio: break;
	}
}

void Opl_reset( struct Opl_Apu* this )
{
	this->addr = 0;
	this->next_time = 0;
	this->last_amp = 0;

	switch (this->type_)
	{
	case type_opll:
	case type_msxmusic:
	case type_smsfmunit:
	case type_vrc7:
		OPLL_reset( &this->opll );
		OPLL_setMask( &this->opll, 0 );
		break;
	case type_msxaudio:
		OPL_reset( &this->opl );
		break;
	}
}

static void run_until( struct Opl_Apu* this, blip_time_t end_time );
void Opl_write_data( struct Opl_Apu* this, blip_time_t time, int data )
{
	run_until( this, time );
	switch (this->type_)
	{
	case type_opll:
	case type_msxmusic:
	case type_smsfmunit:
	case type_vrc7:
		OPLL_writeIO( &this->opll, 0, this->addr );
		OPLL_writeIO( &this->opll, 1, data );
		break;
	case type_msxaudio:
		OPL_writeReg( &this->opl, this->addr, data );
		break;
	}
}

int Opl_read( struct Opl_Apu* this, blip_time_t time, int port )
{
	run_until( this, time );
	switch (this->type_)
	{
	case type_opll:
	case type_msxmusic:
	case type_smsfmunit:
	case type_vrc7:
		return OPLL_read( &this->opll, port );
	case type_msxaudio:
		return OPL_readStatus( &this->opl );
	}

	return 0;
}

void Opl_end_frame( struct Opl_Apu* this, blip_time_t time )
{
	run_until( this, time );
	this->next_time -= time;

	if ( this->output_ )
		Blip_set_modified( this->output_ );
}

static void run_until( struct Opl_Apu* this, blip_time_t end_time )
{
	if ( end_time > this->next_time )
	{
		blip_time_t time_delta = end_time - this->next_time;
		blip_time_t time = this->next_time;
		unsigned count = time_delta / this->period_ + 1;
		switch (this->type_)
		{
		case type_opll:
		case type_msxmusic:
		case type_smsfmunit:
		case type_vrc7:
			{
				OPLL* opll = &this->opll; // cache
				struct Blip_Buffer* const output = this->output_;
				while ( count > 0 )
				{
					unsigned todo = count;
					if ( todo > 1024 ) todo = 1024;
					short *buffer = OPLL_update_buffer(opll, todo);
					
					if ( output && buffer )
					{
						int last_amp = this->last_amp;
						unsigned i;
						for ( i = 0; i < todo; i++ )
						{
							int amp = buffer [i];
							int delta = amp - last_amp;
							if ( delta )
							{
								last_amp = amp;
								Synth_offset_inline( &this->synth, time, delta, output );
							}
							time += this->period_;
						}
						this->last_amp = last_amp;
					}
					count -= todo;
				}
			}
			break;
		case type_msxaudio:
			{
				struct Y8950* opl = &this->opl;
				struct Blip_Buffer* const output = this->output_;
				while ( count > 0 )
				{
					unsigned todo = count;
					if ( todo > 1024 ) todo = 1024;
					int *buffer = OPL_updateBuffer(opl, todo);

					if ( output && buffer )
					{
						int last_amp = this->last_amp;
						unsigned i;
						for ( i = 0; i < todo; i++ )
						{
							int amp = buffer [i];
							int delta = amp - last_amp;
							if ( delta )
							{
								last_amp = amp;
								Synth_offset_inline( &this->synth, time, delta, output );
							}
							time += this->period_;
						}
						this->last_amp = last_amp;
					}
					count -= todo;
				}
			}
			break;
		}
		this->next_time = time;
	}
}
