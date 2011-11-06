// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

#include "ym2413_emu.h"

void Ym2413_init( struct Ym2413_Emu* this )
{
	this->last_time = disabled_time; this->out = 0; 
}

int Ym2413_set_rate( struct Ym2413_Emu* this, int sample_rate, int clock_rate )
{	
	OPLL_new ( &this->opll, clock_rate, sample_rate );
    OPLL_reset_patch( &this->opll, OPLL_2413_TONE );
    
	Ym2413_reset( this );
	return 0;
}

void Ym2413_reset( struct Ym2413_Emu* this )
{
	OPLL_reset( &this->opll );
	OPLL_setMask( &this->opll, 0 );
}

void Ym2413_write( struct Ym2413_Emu* this, int addr, int data )
{
	OPLL_writeIO( &this->opll, 0, addr );
	OPLL_writeIO( &this->opll, 1, data );
}

void Ym2413_mute_voices( struct Ym2413_Emu* this, int mask )
{
	OPLL_setMask( &this->opll, mask );
}

void Ym2413_run( struct Ym2413_Emu* this, int pair_count, short* out )
{
	while ( pair_count-- )
	{
		int s = OPLL_calc( &this->opll ) << 1;
		out [0] = s;
		out [1] = s;
		out += 2;
	}
}
