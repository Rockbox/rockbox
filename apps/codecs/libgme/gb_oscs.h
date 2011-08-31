// Private oscillators used by Gb_Apu

// Gb_Snd_Emu 0.1.4
#ifndef GB_OSCS_H
#define GB_OSCS_H

#include "blargg_common.h"
#include "blip_buffer.h"

#ifndef GB_APU_OVERCLOCK
	#define GB_APU_OVERCLOCK 1
#endif

#if GB_APU_OVERCLOCK & (GB_APU_OVERCLOCK - 1)
	#error "GB_APU_OVERCLOCK must be a power of 2"
#endif

enum { clk_mul  = GB_APU_OVERCLOCK };
enum { dac_bias = 7 };

struct Gb_Osc {
	struct Blip_Buffer*    outputs [4];// NULL, right, left, center
	struct Blip_Buffer*    output;     // where to output sound
	uint8_t* regs;       // osc's 5 registers
	int             mode;       // mode_dmg, mode_cgb, mode_agb
	int             dac_off_amp;// amplitude when DAC is off
	int             last_amp;   // current amplitude in Blip_Buffer

	struct Blip_Synth* synth;
	
	int         delay;      // clocks until frequency timer expires
	int         length_ctr; // length counter
	unsigned    phase;      // waveform phase (or equivalent)
	bool        enabled;    // internal enabled flag	
};

// 11-bit frequency in NRx3 and NRx4
static inline int Osc_frequency( struct Gb_Osc* this ) { return (this->regs [4] & 7) * 0x100 + this->regs [3]; }

void Osc_clock_length( struct Gb_Osc* this );
void Osc_reset( struct Gb_Osc* this );

// Square

enum { period_mask = 0x70 };
enum { shift_mask  = 0x07 };

struct Gb_Square {
	struct Gb_Osc osc;
	
	int  env_delay;
	int  volume;
	bool env_enabled;
	
	// Sweep square
	int  sweep_freq;
	int  sweep_delay;
	bool sweep_enabled;
	bool sweep_neg;
};

void Square_run( struct Gb_Square* this, blip_time_t, blip_time_t );
void Square_clock_envelope( struct Gb_Square* this );
	 
static inline void Square_reset( struct Gb_Square* this )
{
	this->env_delay = 0;
	this->volume    = 0;
	Osc_reset( &this->osc );
	this->osc.delay = 0x40000000; // TODO: something less hacky (never clocked until first trigger)
}
// Frequency timer period
static inline int Square_period( struct Gb_Square* this ) { return (2048 - Osc_frequency( &this->osc )) * (4 * clk_mul); }
static inline int Square_dac_enabled( struct Gb_Square* this) { return this->osc.regs [2] & 0xF8; }
static inline int Square_reload_env_timer( struct Gb_Square* this )
{
	int raw = this->osc.regs [2] & 7;
	this->env_delay = (raw ? raw : 8);
	return raw;
}

// Sweep square

void clock_sweep( struct Gb_Square* this );
	
static inline void Sweep_reset( struct Gb_Square* this )
{
	this->sweep_freq    = 0;
	this->sweep_delay   = 0;
	this->sweep_enabled = false;
	this->sweep_neg     = false;
	
	this->env_delay = 0;
	this->volume    = 0;
	Osc_reset( &this->osc );
	this->osc.delay = 0x40000000; // TODO: something less hacky (never clocked until first trigger)
}

// Noise 

enum { period2_mask = 0x1FFFF };
	
struct Gb_Noise {
	struct Gb_Osc osc;
	
	int  env_delay;
	int  volume;
	bool env_enabled;
	
	int divider; // noise has more complex frequency divider setup
};

void Noise_run( struct Gb_Noise* this, blip_time_t, blip_time_t );
	
static inline void Noise_reset( struct Gb_Noise* this )
{
	this->divider = 0;
	
	this->env_delay = 0;
	this->volume    = 0;
	Osc_reset( &this->osc );
	this->osc.delay = 4 * clk_mul; // TODO: remove?
}
	
void Noise_clock_envelope( struct Gb_Noise* this );
	
// Non-zero if DAC is enabled
static inline int Noise_dac_enabled( struct Gb_Noise* this) { return this->osc.regs [2] & 0xF8; }
static inline int Noise_reload_env_timer( struct Gb_Noise* this )
{
	int raw = this->osc.regs [2] & 7;
	this->env_delay = (raw ? raw : 8);
	return raw;
}

static inline int period2_index( struct Gb_Noise* this ) { return this->osc.regs [3] >> 4; }
static inline int period2( struct Gb_Noise* this, int base ) { return base << period2_index( this ); }
static inline unsigned lfsr_mask( struct Gb_Noise* this ) { return (this->osc.regs [3] & 0x08) ? ~0x4040 : ~0x4000; }

// Wave

enum { bank40_mask = 0x40 };
enum { wave_bank_size   = 32 };
	
struct Gb_Wave {
	struct Gb_Osc osc;
	
	int sample_buf;      // last wave RAM byte read (hardware has this as well)
	
	int agb_mask;        // 0xFF if AGB features enabled, 0 otherwise
	uint8_t* wave_ram;   // 32 bytes (64 nybbles), stored in APU
};

void Wave_run( struct Gb_Wave* this, blip_time_t, blip_time_t );

static inline void Wave_reset( struct Gb_Wave* this )
{
	this->sample_buf = 0;
	Osc_reset( &this->osc );
}

// Frequency timer period
static inline int Wave_period( struct Gb_Wave* this ) { return (2048 - Osc_frequency( &this->osc )) * (2 * clk_mul); }
	
// Non-zero if DAC is enabled
static inline int Wave_dac_enabled( struct Gb_Wave* this ) { return this->osc.regs [0] & 0x80; }
	
static inline uint8_t* wave_bank( struct Gb_Wave* this ) { return &this->wave_ram [(~this->osc.regs [0] & bank40_mask) >> 2 & this->agb_mask]; }
	
// Wave index that would be accessed, or -1 if no access would occur
int wave_access( struct Gb_Wave* this, int addr );

// Reads/writes wave RAM		
static inline int Wave_read( struct Gb_Wave* this, int addr )
{
	int index = wave_access( this, addr );
	return (index < 0 ? 0xFF : wave_bank( this ) [index]);
}

static inline void Wave_write( struct Gb_Wave* this, int addr, int data )
{
	int index = wave_access( this, addr );
	if ( index >= 0 )
		wave_bank( this ) [index] = data;;
}

#endif
