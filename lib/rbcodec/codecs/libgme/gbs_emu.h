// Nintendo Game Boy GBS music file emulator

// Game_Music_Emu 0.5.2
#ifndef GBS_EMU_H
#define GBS_EMU_H

#include "rom_data.h"
#include "multi_buffer.h"
#include "gb_apu.h"
#include "gb_cpu.h"
#include "m3u_playlist.h"
#include "track_filter.h"

enum { joypad_addr = 0xFF00 };
enum { ram_addr = 0xA000 };
enum { hi_page = 0xFF00 - ram_addr };
enum { io_base = 0xFF00 };

// Selects which sound hardware to use. AGB hardware is cleaner than the
// others. Doesn't take effect until next start_track().
enum sound_t {
	sound_dmg = mode_dmg,   // Game Boy monochrome
	sound_cgb = mode_cgb,   // Game Boy Color
	sound_agb = mode_agb,   // Game Boy Advance
	sound_gbs               // Use DMG/CGB based on GBS (default)
};

// GBS file header
enum { header_size = 112 };
struct header_t
{
	char tag [3];
	byte vers;
	byte track_count;
	byte first_track;
	byte load_addr [2];
	byte init_addr [2];
	byte play_addr [2];
	byte stack_ptr [2];
	byte timer_modulo;
	byte timer_mode;
	char game [32];
	char author [32];
	char copyright [32];
};

struct Gbs_Emu {
	enum sound_t sound_hardware;
	
	int tempo;
	
	// timer
	blip_time_t cpu_time;
	blip_time_t end_time;
	blip_time_t play_period;
	blip_time_t next_play;

	// Sound 
	int clock_rate_;
	int sample_rate_;
	unsigned buf_changed_count;
	int voice_count_;
	int const* voice_types_;
	int mute_mask_;
	int gain_;
	int tempo_;
	
	// track-specific
	byte track_count;
	int current_track_;

	// Larger items at the end
	// Header for currently loaded file
	struct header_t header;
	
	// M3u Playlist
	struct M3u_Playlist m3u;
	
	struct setup_t tfilter;
	struct Track_Filter track_filter;
	
	struct Gb_Apu apu;
	struct Gb_Cpu cpu;
	struct Multi_Buffer stereo_buf;
	
	// rom & ram
	struct Rom_Data rom; 
	byte ram [0x4000 + 0x2000 + cpu_padding];
};


// Basic functionality
// Initializes Gbs_Emu structure
void Gbs_init( struct Gbs_Emu* this );

// Stops (clear) Gbs_Emu structure
void Gbs_stop( struct Gbs_Emu* this );

// Loads a file from memory
blargg_err_t Gbs_load_mem( struct Gbs_Emu* this, void* data, long size );

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Gbs_set_sample_rate( struct Gbs_Emu* this, int sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Gbs_start_track( struct Gbs_Emu* this, int );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Gbs_play( struct Gbs_Emu* this, int count, sample_t* buf );

// Track status/control
// Number of milliseconds (1000 msec = 1 second) played since beginning of track
int Track_tell( struct Gbs_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Gbs_Emu* this, int msec );

// Skip n samples
blargg_err_t Track_skip( struct Gbs_Emu* this, int n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Gbs_Emu* this, int start_msec, int length_msec );

// True if a track has reached its end
static inline bool Track_ended( struct Gbs_Emu* this )
{
	return track_ended( &this->track_filter );
}

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void Track_ignore_silence( struct Gbs_Emu* this, bool disable )
{
	this->track_filter.silence_ignored_ = disable;
}

// Get track length in milliseconds
static inline int Track_get_length( struct Gbs_Emu* this, int n )
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
void Sound_set_tempo( struct Gbs_Emu* this, int );

// Mute/unmute voice i, where voice 0 is first voice
void Sound_mute_voice( struct Gbs_Emu* this, int index, bool mute );

// Set muting state of all voices at once using a bit mask, where -1 mutes them all,
// 0 unmutes them all, 0x01 mutes just the first voice, etc.
void Sound_mute_voices( struct Gbs_Emu* this, int mask );

// Change overall output amplitude, where 1.0 results in minimal clamping.
// Must be called before set_sample_rate().
static inline void Sound_set_gain( struct Gbs_Emu* this, int g )
{
	assert( !this->sample_rate_ ); // you must set gain before setting sample rate
	this->gain_ = g;
}

// Emulation (You shouldn't touch these)

blargg_err_t run_clocks( struct Gbs_Emu* this, blip_time_t duration );
void set_bank( struct Gbs_Emu* this, int );
void update_timer( struct Gbs_Emu* this );

// Runs CPU until time becomes >= 0
void run_cpu( struct Gbs_Emu* this );

// Reads/writes memory and I/O
int  read_mem( struct Gbs_Emu* this, addr_t addr );
void write_mem( struct Gbs_Emu* this, addr_t addr, int data );

// Current time
static inline blip_time_t Time( struct Gbs_Emu* this ) 
{
	return Cpu_time( &this->cpu ) + this->end_time; 
}

void jsr_then_stop( struct Gbs_Emu* this, byte const [] );

#endif
