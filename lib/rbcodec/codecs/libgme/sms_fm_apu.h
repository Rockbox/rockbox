#ifndef SMS_FM_APU_H
#define SMS_FM_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"
#include "ym2413_emu.h"

enum { fm_apu_osc_count = 1 };

struct Sms_Fm_Apu {
	struct Blip_Buffer* output_;
	blip_time_t next_time;
	int last_amp;
	int addr;
	
	int clock_;
	int rate_;
	blip_time_t period_;
	
	struct Blip_Synth synth;
	struct Ym2413_Emu apu;
};

void Fm_apu_create( struct Sms_Fm_Apu* this );

static inline bool Fm_apu_supported( void ) { return Ym2413_supported(); }
blargg_err_t Fm_apu_init( struct Sms_Fm_Apu* this, int clock_rate, int sample_rate );

static inline void Fm_apu_set_output( struct Sms_Fm_Apu* this, struct Blip_Buffer* b )
{
	this->output_ = b;
}

static inline void Fm_apu_volume( struct Sms_Fm_Apu* this, int v ) { Synth_volume( &this->synth, (v*2) / 5 / 4096 ); }

void Fm_apu_reset( struct Sms_Fm_Apu* this );

static inline void Fm_apu_write_addr( struct Sms_Fm_Apu* this, int data )                 { this->addr = data; }
void Fm_apu_write_data( struct Sms_Fm_Apu* this, blip_time_t, int data );

void Fm_apu_end_frame( struct Sms_Fm_Apu* this, blip_time_t t );

#endif
