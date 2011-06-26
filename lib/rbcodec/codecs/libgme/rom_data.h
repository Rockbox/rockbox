// Common aspects of emulators which use rom data

// Game_Music_Emu 0.5.2
#ifndef ROM_DATA_H
#define ROM_DATA_H

#include "blargg_common.h"
#include "blargg_source.h"

// ROM data handler, used by several Classic_Emu derivitives. Loads file data
// with padding on both sides, allowing direct use in bank mapping. The main purpose
// is to allow all file data to be loaded with only one read() call (for efficiency).

extern const char gme_wrong_file_type []; // declared in gme.h

enum { pad_extra = 8 };
enum { max_bank_size = 0x4000 };
enum { max_pad_size = max_bank_size + pad_extra };
enum { max_rom_size = 2 * max_pad_size };

struct  Rom_Data {
	byte* file_data;
	unsigned file_size;
	
	int rom_addr;
	int bank_size;
	int rom_size;
	unsigned pad_size;
	int mask;
	int size; // TODO: eliminate
	int rsize_;
	
	// Unmapped space
	byte unmapped [max_rom_size];
};

// Initialize rom
static inline void Rom_init( struct Rom_Data* this, int bank_size )
{
	this->bank_size = bank_size;
	this->pad_size = this->bank_size + pad_extra;
	this->rom_size = 2 * this->pad_size;
}

// Load file data, using already-loaded header 'h' if not NULL. Copy header
// from loaded file data into *out and fill unmapped bytes with 'fill'.
blargg_err_t Rom_load( struct Rom_Data* this, const void* data, long size, int header_size, void* header_out, int fill );

// Set address that file data should start at
void Rom_set_addr( struct Rom_Data* this, int addr );

// Mask address to nearest power of two greater than size()
static inline int mask_addr( int addr, int mask )
{
	#ifdef check
		check( addr <= mask );
	#endif
	return addr & mask;
}

// Pointer to page starting at addr. Returns unmapped() if outside data.
static inline byte* Rom_at_addr( struct Rom_Data* this, int addr )
{
	unsigned offset = mask_addr( addr, this->mask ) - this->rom_addr;
	if ( offset > (unsigned) (this->rsize_ - this->pad_size) )
		offset = 0; // unmapped
			
	if ( offset < this->pad_size ) return &this->unmapped [offset];
	else return &this->file_data [offset - this->pad_size];
}


#ifndef GME_APU_HOOK
	#define GME_APU_HOOK( emu, addr, data ) ((void) 0)
#endif

#ifndef GME_FRAME_HOOK
	#define GME_FRAME_HOOK( emu ) ((void) 0)
#else
	#define GME_FRAME_HOOK_DEFINED 1
#endif

#endif
