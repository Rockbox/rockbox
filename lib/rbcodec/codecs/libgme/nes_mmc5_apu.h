// NES MMC5 sound chip emulator

// Nes_Snd_Emu 0.2.0-pre
#ifndef NES_MMC5_APU_H
#define NES_MMC5_APU_H

#include "blargg_common.h"
#include "nes_apu.h"

enum { mmc5_regs_addr = 0x5000 };
enum { mmc5_regs_size = 0x16 };
enum { mmc5_osc_count  = 3 };
enum { mmc5_exram_size = 1024 };

struct Nes_Mmc5_Apu {
	struct Nes_Apu apu;
	unsigned char exram [mmc5_exram_size];
};

static inline void Mmc5_init( struct Nes_Mmc5_Apu* this )
{
	Apu_init( &this->apu );
}

static inline void Mmc5_set_output( struct Nes_Mmc5_Apu* this, int i, struct Blip_Buffer* b )
{
	// in: square 1, square 2, PCM
	// out: square 1, square 2, skipped, skipped, PCM
	if ( i > 1 )
		i += 2;
	Apu_osc_output( &this->apu, i, b );
}

static inline void Mmc5_write_register( struct Nes_Mmc5_Apu* this, blip_time_t time, unsigned addr, int data )
{
	switch ( addr )
	{
	case 0x5015: // channel enables
		data &= 0x03; // enable the square waves only
		// fall through
	case 0x5000: // Square 1
	case 0x5002:
	case 0x5003:
	case 0x5004: // Square 2
	case 0x5006:
	case 0x5007:
	case 0x5011: // DAC
		Apu_write_register( &this->apu, time, addr - 0x1000, data );
		break;
	
	case 0x5010: // some things write to this for some reason
		break;
	
#ifdef BLARGG_DEBUG_H
	default:
			dprintf( "Unmapped MMC5 APU write: $%04X <- $%02X\n", addr, data );
#endif
	}
}

#endif
