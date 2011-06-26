// Namco 106 sound chip emulator

// Nes_Snd_Emu 0.1.8
#ifndef NES_NAMCO_APU_H
#define NES_NAMCO_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"

struct namco_state_t;

enum { namco_osc_count = 8 };
enum { namco_addr_reg_addr = 0xF800 };
enum { namco_data_reg_addr = 0x4800 };
enum { namco_reg_count = 0x80 };

struct Namco_Osc {
	int delay;
	struct Blip_Buffer* output;
	short last_amp;
	short wave_pos;
};

struct Nes_Namco_Apu {
	struct Namco_Osc oscs [namco_osc_count];
	
	blip_time_t last_time;
	int addr_reg;
	
	uint8_t reg [namco_reg_count];

	struct Blip_Synth synth;
};

// See Nes_Apu.h for reference.
void Namco_init( struct Nes_Namco_Apu* this );
void Namco_output( struct Nes_Namco_Apu* this, struct Blip_Buffer* );
	
void Namco_reset( struct Nes_Namco_Apu* this );
void Namco_end_frame( struct Nes_Namco_Apu* this, blip_time_t );
	
static inline uint8_t* namco_access( struct Nes_Namco_Apu* this )
{
	int addr = this->addr_reg & 0x7F;
	if ( this->addr_reg & 0x80 )
		this->addr_reg = (addr + 1) | 0x80;
	return &this->reg [addr];
}

static inline void Namco_volume( struct Nes_Namco_Apu* this, int v ) { Synth_volume( &this->synth, v / 10 / namco_osc_count / 15 ); }

// Write-only address register is at 0xF800
static inline void Namco_write_addr( struct Nes_Namco_Apu* this, int v ) { this->addr_reg = v; }

static inline int Namco_read_data( struct Nes_Namco_Apu* this ) { return *namco_access( this ); }

static inline void Namco_osc_output( struct Nes_Namco_Apu* this, int i, struct Blip_Buffer* buf )
{
	assert( (unsigned) i < namco_osc_count );
	this->oscs [i].output = buf;
}

// Read/write data register is at 0x4800
void Namco_run_until( struct Nes_Namco_Apu* this, blip_time_t );
static inline void Namco_write_data( struct Nes_Namco_Apu* this, blip_time_t time, int data )
{
	Namco_run_until( this, time );
	*namco_access( this ) = data;
}

#endif
