// NES FDS sound chip emulator

// Game_Music_Emu 0.6-pre
#ifndef NES_FDS_APU_H
#define NES_FDS_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"

enum { lfo_base_tempo = 8 };
enum { fds_osc_count = 1 };

enum { fds_io_addr = 0x4040 };
enum { fds_io_size = 0x53 };

enum { fds_wave_size       = 0x40 };
enum { fds_master_vol_max  =   10 };
enum { fds_vol_max         = 0x20 };
enum { fds_wave_sample_max = 0x3F };

struct Nes_Fds_Apu {
	unsigned char regs_ [fds_io_size];// last written value to registers
	int lfo_tempo; // normally 8; adjusted by set_tempo()   
	
	int env_delay;
	int env_speed;
	int env_gain;
	
	int sweep_delay;
	int sweep_speed;
	int sweep_gain;
	
	int wave_pos;
	int last_amp;
	blip_time_t wave_fract;
	
	int mod_fract;
	int mod_pos;
	int mod_write_pos;
	unsigned char mod_wave [fds_wave_size];
	
	// synthesis
	blip_time_t last_time;
	struct Blip_Buffer* output_;
	struct Blip_Synth synth;
};

// init
void Fds_init( struct Nes_Fds_Apu* this );
// setup
void Fds_set_tempo( struct Nes_Fds_Apu* this, int t );
	
// emulation
void Fds_reset( struct Nes_Fds_Apu* this );

static inline void Fds_volume( struct Nes_Fds_Apu* this, int v )
{
	Synth_volume( &this->synth, (v*14) / 100 / fds_master_vol_max / fds_vol_max / fds_wave_sample_max );
}

static inline void Fds_set_output( struct Nes_Fds_Apu* this, int i, struct Blip_Buffer* b )
{
#if defined(ROCKBOX)
	(void) i;
#endif

	assert( (unsigned) i < fds_osc_count );
	this->output_ = b;
}

void Fds_run_until( struct Nes_Fds_Apu* this, blip_time_t );
static inline void Fds_end_frame( struct Nes_Fds_Apu* this, blip_time_t end_time )
{
	if ( end_time > this->last_time )
		Fds_run_until( this, end_time );
	this->last_time -= end_time;
	assert( this->last_time >= 0 );
}

void Fds_write_( struct Nes_Fds_Apu* this, unsigned addr, int data );
static inline void Fds_write( struct Nes_Fds_Apu* this, blip_time_t time, unsigned addr, int data )
{
	Fds_run_until( this, time );
	Fds_write_( this, addr, data );
}

static inline int Fds_read( struct Nes_Fds_Apu* this, blip_time_t time, unsigned addr )
{
	Fds_run_until( this, time );
	
	int result = 0xFF;
	switch ( addr )
	{
	case 0x4090:
		result = this->env_gain;
		break;
	
	case 0x4092:
		result = this->sweep_gain;
		break;
	
	default:
		{
			unsigned i = addr - fds_io_addr;
			if ( i < fds_wave_size )
				result = this->regs_ [i];
		}
	}
	
	return result | 0x40;
}

// allow access to registers by absolute address (i.e. 0x4080)
static inline unsigned char* regs_nes( struct Nes_Fds_Apu* this, unsigned addr ) { return &this->regs_ [addr - fds_io_addr]; }

#endif
