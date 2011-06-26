// Sega/Game Gear/Coleco SGC music file emulator

// Game_Music_Emu 0.6-pre
#ifndef SGC_EMU_H
#define SGC_EMU_H

#include "blargg_common.h"
#include "multi_buffer.h"

#include "rom_data.h"
#include "z80_cpu.h"
#include "sms_fm_apu.h"
#include "sms_apu.h"
#include "m3u_playlist.h"
#include "track_filter.h"

typedef struct Z80_Cpu Sgc_Cpu;

// SGC file header
enum { header_size = 0xA0 };
struct header_t
{
	char tag       [4]; // "SGC\x1A"
	byte vers;          // 0x01
	byte rate;          // 0=NTSC 1=PAL
	byte reserved1 [2];
	byte load_addr [2];
	byte init_addr [2];
	byte play_addr [2];
	byte stack_ptr [2];
	byte reserved2 [2];
	byte rst_addrs [7*2];
	byte mapping   [4]; // Used by Sega only
	byte first_song;    // Song to start playing first
	byte song_count;
	byte first_effect;
	byte last_effect;
	byte system;        // 0=Master System 1=Game Gear 2=Colecovision
	byte reserved3 [23];
	char game      [32]; // strings can be 32 chars, NOT terminated
	char author    [32];
	char copyright [32];
};

// True if header has valid file signature
static inline bool valid_tag( struct header_t* h )
{
	return 0 == memcmp( h->tag, "SGC\x1A", 4 );	
}

static inline int effect_count( struct header_t* h ) { return h->last_effect ? h->last_effect - h->first_effect + 1 : 0; }

struct Sgc_Emu {
	bool fm_accessed;

	cpu_time_t play_period;
	cpu_time_t next_play;
	void const* bank2;      // ROM selected for bank 2, in case RAM is currently hiding it
	addr_t vectors_addr;    // RST vectors start here
	addr_t idle_addr;       // return address for init/play routines
	void* coleco_bios;
	
	// general
	int voice_count;
	int const* voice_types;
	int mute_mask_;
	int tempo;
	int gain;
	
	int sample_rate;
	
	// track-specific
	int current_track;
	int track_count;

	int clock_rate_;
	unsigned buf_changed_count;
	
	// M3u Playlist
	struct M3u_Playlist m3u;
	struct header_t header;
	
	struct setup_t tfilter;
	struct Track_Filter track_filter;
	
	struct Multi_Buffer stereo_buf;
	
	struct Sms_Apu apu;
	struct Sms_Fm_Apu fm_apu;
	
	Sgc_Cpu cpu;
	
	// large items
	struct Rom_Data rom;
	byte vectors [page_size + page_padding];
	byte unmapped_write [0x4000];
	byte ram [0x2000 + page_padding];
	byte ram2 [0x4000 + page_padding];
};

// Basic functionality (see Gme_File.h for file loading/track info functions)

void Sgc_init( struct Sgc_Emu* this );

blargg_err_t Sgc_load_mem( struct Sgc_Emu* this, const void* data, long size );

static inline int clock_rate( struct Sgc_Emu* this ) { return this->header.rate ? 3546893 : 3579545; }

// 0x2000 bytes
static inline void set_coleco_bios( struct Sgc_Emu* this, void* p ) { this->coleco_bios = p; }

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Sgc_set_sample_rate( struct Sgc_Emu* this, int sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Sgc_start_track( struct Sgc_Emu* this, int track );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Sgc_play( struct Sgc_Emu* this, int count, sample_t* buf );

// Track status/control

// Number of milliseconds (1000 msec = 1 second) played since beginning of track
int Track_tell( struct Sgc_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Sgc_Emu* this, int msec );

// Skip n samples
blargg_err_t Track_skip( struct Sgc_Emu* this, int n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Sgc_Emu* this, int start_msec, int length_msec );

// True if a track has reached its end
static inline bool Track_ended( struct Sgc_Emu* this )
{
	return track_ended( &this->track_filter );
}

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void Track_ignore_silence( struct Sgc_Emu* this, bool disable )
{
	this->track_filter.silence_ignored_ = disable;
}

// Get track length in milliseconds
static inline int Track_get_length( struct Sgc_Emu* this, int n )
{
	int length = 120 * 1000;  /* 2 minutes */ 
	if ( (this->m3u.size > 0) && (n < this->m3u.size) ) {
		struct entry_t* entry = &this->m3u.entries [n];
		length = entry->length;
	} 

	return length;
}

// Sound customization

// Adjust song tempo, where 1.0 = normal, 0.5 = half speed, 2.0 = double speed.
// Track length as returned by track_info() assumes a tempo of 1.0.
void Sound_set_tempo( struct Sgc_Emu* this, int t );

// Mute/unmute voice i, where voice 0 is first voice
void Sound_mute_voice( struct Sgc_Emu* this, int index, bool mute );

// Set muting state of all voices at once using a bit mask, where -1 mutes them all,
// 0 unmutes them all, 0x01 mutes just the first voice, etc.
void Sound_mute_voices( struct Sgc_Emu* this, int mask );

// Change overall output amplitude, where 1.0 results in minimal clamping.
// Must be called before set_sample_rate().
static inline void Sound_set_gain( struct Sgc_Emu* this, int g )
{
	assert( !this->sample_rate ); // you must set gain before setting sample rate
	this->gain = g;
}

// True if Master System or Game Gear
static inline bool sega_mapping( struct Sgc_Emu* this )
{
	return this->header.system <= 1;
}

// Emulation (You shouldn't touch these)

bool run_cpu( struct Sgc_Emu* this, cpu_time_t end_time );
void cpu_out( struct Sgc_Emu* this, cpu_time_t time, addr_t addr, int data );
void cpu_write( struct Sgc_Emu* this, addr_t addr, int data );
void jsr( struct Sgc_Emu* this, byte addr [2] );

#endif
