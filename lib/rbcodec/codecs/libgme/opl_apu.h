#ifndef OPL_APU_H
#define OPL_APU_H

#include "blargg_common.h"
#include "blargg_source.h"
#include "blip_buffer.h"

#include "emu8950.h"
#include "emu2413.h"

enum opl_type_t { type_opll = 0x10, type_msxmusic = 0x11, type_smsfmunit = 0x12,
			type_vrc7 = 0x13, type_msxaudio = 0x21 };

enum { opl_osc_count = 1 };

struct Opl_Apu {
	struct Blip_Buffer* output_;
	enum opl_type_t type_;

	blip_time_t next_time;
	int last_amp;
	int addr;
	
	long clock_;
	long rate_;
	blip_time_t period_;
	
	struct Blip_Synth synth;

	// OPL chips
	struct Y8950 opl;
	OPLL opll;
	
	unsigned char regs[ 0x100 ];
	unsigned char opl_memory[ 32768 ];
};

blargg_err_t Opl_init( struct Opl_Apu* this, long clock, long rate, blip_time_t period, enum opl_type_t type );
void Opl_shutdown( struct Opl_Apu* this );

void Opl_reset( struct Opl_Apu* this );
static inline void Opl_volume( struct Opl_Apu* this, int v ) { Synth_volume( &this->synth, v / (4096 * 6) ); }

static inline void Opl_osc_output( struct Opl_Apu* this, int i, struct Blip_Buffer* buf )
{
#if defined(ROCKBOX)
	(void) i;
#endif
	assert( (unsigned) i < opl_osc_count );
	this->output_ = buf;
}

static inline void Opl_set_output( struct Opl_Apu* this, struct Blip_Buffer* buf ) { Opl_osc_output( this, 0, buf ); }
void Opl_end_frame( struct Opl_Apu* this, blip_time_t );

static inline void Opl_write_addr( struct Opl_Apu* this, int data ) { this->addr = data; }
void Opl_write_data( struct Opl_Apu* this, blip_time_t, int data );

int Opl_read( struct Opl_Apu* this, blip_time_t, int port );

static inline bool Opl_supported( void ) { return true; }

#endif
