// NES 2A03 APU sound chip emulator

// Nes_Snd_Emu 0.2.0-pre
#ifndef NES_APU_H
#define NES_APU_H

#include "blargg_common.h"
#include "nes_oscs.h"

enum { apu_status_addr = 0x4015 };
enum { apu_osc_count = 5 };
enum { apu_no_irq = INT_MAX/2 + 1 };
enum { apu_irq_waiting = 0 };

enum { apu_io_addr = 0x4000 };
enum { apu_io_size = 0x18 };

struct apu_state_t;

struct Nes_Apu {
	int tempo_;
	nes_time_t last_time; // has been run until this time in current frame
	nes_time_t last_dmc_time;
	nes_time_t earliest_irq_;
	nes_time_t next_irq;
	int frame_period;
	int frame_delay; // cycles until frame counter runs next
	int frame; // current frame (0-3)
	int osc_enables;
	int frame_mode;
	bool irq_flag;
	
	void (*irq_notifier_)( void* user_data );
	void* irq_data;
	
	Synth square_synth; // shared by squares
	
	struct Nes_Osc*            oscs [apu_osc_count];
	struct Nes_Square          square1;
	struct Nes_Square          square2;
	struct Nes_Noise           noise;
	struct Nes_Triangle        triangle;
	struct Nes_Dmc             dmc;
};

// Init Nes apu
void Apu_init( struct Nes_Apu* this );

// Set buffer to generate all sound into, or disable sound if NULL
void Apu_output( struct Nes_Apu* this, struct Blip_Buffer* );

// All time values are the number of cpu clock cycles relative to the
// beginning of the current time frame. Before resetting the cpu clock
// count, call end_frame( last_cpu_time ).

// Write to register (0x4000-0x4017, except 0x4014 and 0x4016)
void Apu_write_register( struct Nes_Apu* this, nes_time_t, addr_t, int data );

// Read from status register at 0x4015
int Apu_read_status( struct Nes_Apu* this, nes_time_t );

// Run all oscillators up to specified time, end current time frame, then
// start a new time frame at time 0. Time frames have no effect on emulation
// and each can be whatever length is convenient.
void Apu_end_frame( struct Nes_Apu* this, nes_time_t );

// Additional optional features (can be ignored without any problem)

// Reset internal frame counter, registers, and all oscillators.
// Use PAL timing if pal_timing is true, otherwise use NTSC timing.
// Set the DMC oscillator's initial DAC value to initial_dmc_dac without
// any audible click.
void Apu_reset( struct Nes_Apu* this, bool pal_mode, int initial_dmc_dac );

// Adjust frame period
void Apu_set_tempo( struct Nes_Apu* this, int );

// Set overall volume (default is 1.0)
void Apu_volume( struct Nes_Apu* this, int );

// Run DMC until specified time, so that any DMC memory reads can be
// accounted for (i.e. inserting cpu wait states).
void Apu_run_until( struct Nes_Apu* this, nes_time_t );

// Set sound output of specific oscillator to buffer. If buffer is NULL,
// the specified oscillator is muted and emulation accuracy is reduced.
// The oscillators are indexed as follows: 0) Square 1, 1) Square 2,
// 2) Triangle, 3) Noise, 4) DMC.
static inline void Apu_osc_output( struct Nes_Apu* this, int osc, struct Blip_Buffer* buf )
{
	assert( (unsigned) osc < apu_osc_count );
	this->oscs [osc]->output = buf;
}

// Set memory reader callback used by DMC oscillator to fetch samples.
// When callback is invoked, 'user_data' is passed unchanged as the
// first parameter.
static inline void Apu_dmc_reader( struct Nes_Apu* this, int (*func)( void*, addr_t ), void* user_data )
{
	this->dmc.prg_reader_data = user_data;
	this->dmc.prg_reader = func;
}

// Set IRQ time callback that is invoked when the time of earliest IRQ
// may have changed, or NULL to disable. When callback is invoked,
// 'user_data' is passed unchanged as the first parameter.
static inline void Apu_irq_notifier( struct Nes_Apu* this, void (*func)( void* user_data ), void* user_data )
{
	this->irq_notifier_ = func;
	this->irq_data = user_data;
}

// Count number of DMC reads that would occur if 'run_until( t )' were executed.
// If last_read is not NULL, set *last_read to the earliest time that
// 'count_dmc_reads( time )' would result in the same result.
static inline int Apu_count_dmc_reads( struct Nes_Apu* this, nes_time_t time, nes_time_t* last_read )
{
	return Dmc_count_reads( &this->dmc, time, last_read );
}

static inline nes_time_t Dmc_next_read_time( struct Nes_Dmc* this )
{
	if ( this->osc.length_counter == 0 )
		return apu_no_irq; // not reading
	
	return this->apu->last_dmc_time + this->osc.delay + (this->bits_remain - 1) * this->period;
}

// Time when next DMC memory read will occur
static inline nes_time_t Apu_next_dmc_read_time( struct Nes_Apu* this ) { return Dmc_next_read_time( &this->dmc ); }
void Apu_irq_changed( struct Nes_Apu* this );

#if 0
// Experimental
void Apu_enable_nonlinear_( struct Nes_Apu* this, int sq, int tnd );
#endif
#endif
