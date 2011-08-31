// Private oscillators used by Nes_Apu

// Nes_Snd_Emu 0.1.8
#ifndef NES_OSCS_H
#define NES_OSCS_H

#include "blargg_common.h"
#include "blip_buffer.h"
#include "nes_cpu.h"

struct Nes_Apu;

struct Nes_Osc
{
	unsigned char regs [4];
	bool reg_written [4];
	struct Blip_Buffer* output;
	int length_counter;// length counter (0 if unused by oscillator)
	int delay;      // delay until next (potential) transition
	int last_amp;   // last amplitude oscillator was outputting
};

void Osc_clock_length( struct Nes_Osc* this, int halt_mask );
static inline int Osc_period( struct Nes_Osc* this ) 
{
	return (this->regs [3] & 7) * 0x100 + (this->regs [2] & 0xFF);
}

static inline void Osc_reset( struct Nes_Osc* this ) 
{
	this->delay = 0;
	this->last_amp = 0;
}

static inline int Osc_update_amp( struct Nes_Osc* this, int amp ) 
{
	int delta = amp - this->last_amp;
	this->last_amp = amp;
	return delta;
}

// Nes_Square

enum { negate_flag = 0x08 };
enum { shift_mask = 0x07 };
enum { square_phase_range = 8 };

typedef struct Blip_Synth Synth;

struct Nes_Square
{
	struct Nes_Osc osc;
	int envelope;
	int env_delay;
	int phase;
	int sweep_delay;
	
	Synth* synth; // shared between squares
};

static inline void Square_set_synth( struct Nes_Square* this, Synth* s ) { this->synth = s; }

void Square_clock_sweep( struct Nes_Square* this, int adjust );
void Square_run( struct Nes_Square* this, nes_time_t, nes_time_t );

static inline void Square_reset( struct Nes_Square* this ) 
{
	this->sweep_delay = 0;
	this->envelope = 0;
	this->env_delay = 0;
	Osc_reset( &this->osc );
}

void Square_clock_envelope( struct Nes_Square* this );
int Square_volume( struct Nes_Square* this );

// Nes_Triangle

enum { Triangle_phase_range = 16 };

struct Nes_Triangle
{
	struct Nes_Osc osc;
	
	int phase;
	int linear_counter;
	struct Blip_Synth synth;
};

void Triangle_run( struct Nes_Triangle* this, nes_time_t, nes_time_t );
void Triangle_clock_linear_counter( struct Nes_Triangle* this );

static inline void Triangle_reset( struct Nes_Triangle* this )
{
	this->linear_counter = 0;
	this->phase = 1;
	Osc_reset( &this->osc );
}

// Nes_Noise
struct Nes_Noise
{
	struct Nes_Osc osc;
	
	int envelope;
	int env_delay;
	int noise;
	struct Blip_Synth synth;
};

void Noise_clock_envelope( struct Nes_Noise* this );
int Noise_volume( struct Nes_Noise* this );
void Noise_run( struct Nes_Noise* this, nes_time_t, nes_time_t );

static inline void Noise_reset( struct Nes_Noise* this )
{
	this->noise = 1 << 14;
	this->envelope = 0;
	this->env_delay = 0;
	Osc_reset( &this->osc );
}

// Nes_Dmc

enum { loop_flag = 0x40 };

struct Nes_Dmc
{
	struct Nes_Osc osc;
	
	int address;    // address of next byte to read
	int period;
	int buf;
	int bits_remain;
	int bits;
	bool buf_full;
	bool silence;
	
	int dac;
	
	nes_time_t next_irq;
	bool irq_enabled;
	bool irq_flag;
	bool pal_mode;
	bool nonlinear;
	
 	int (*prg_reader)( void*, int ); // needs to be initialized to prg read function
	void* prg_reader_data;
	
	struct Nes_Apu* apu;
	
	struct Blip_Synth synth;
};

void Dmc_start( struct Nes_Dmc* this );
void Dmc_write_register( struct Nes_Dmc* this, int, int );
void Dmc_run( struct Nes_Dmc* this, nes_time_t, nes_time_t );
void Dmc_recalc_irq( struct Nes_Dmc* this );
void Dmc_fill_buffer( struct Nes_Dmc* this );
void Dmc_reset( struct Nes_Dmc* this );

int Dmc_count_reads( struct Nes_Dmc* this, nes_time_t, nes_time_t* );

#endif
