// Nintendo Game Boy GBS music file emulator

// Game_Music_Emu 0.5.2
#ifndef GBS_EMU_H
#define GBS_EMU_H

#include "rom_data.h"
#include "multi_buffer.h"
#include "gb_apu.h"
#include "gb_cpu.h"
#include "m3u_playlist.h"

/* typedef uint8_t byte; */
typedef short sample_t;

enum { joypad_addr = 0xFF00 };
enum { ram_addr = 0xA000 };
enum { hi_page = 0xFF00 - ram_addr };
enum { io_base = 0xFF00 };
enum { buf_size = 2048 };

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
	long clock_rate_;
	long sample_rate_;
	unsigned buf_changed_count;
	int voice_count_;
	double gain_;
	int tempo_;
	
	// track-specific
	byte track_count;
	volatile bool track_ended;
	int current_track_;
	blargg_long out_time;  // number of samples played since start of track
	blargg_long emu_time;  // number of samples emulator has generated since start of track
	bool emu_track_ended_; // emulator has reached end of track
	
	// fading
	blargg_long fade_start;
	int fade_step;
	
	// silence detection
	// Disable automatic end-of-track detection and skipping of silence at beginning
	bool ignore_silence;

	int max_initial_silence;
	int mute_mask_;
	int silence_lookahead; // speed to run emulator when looking ahead for silence
	long silence_time;     // number of samples where most recent silence began
	long silence_count;    // number of samples of silence to play before using buf
	long buf_remain;       // number of samples left in silence buffer
	
	// Larger items at the end
	// Header for currently loaded file
	struct header_t header;
	
	// M3u Playlist
	struct M3u_Playlist m3u;
	
	struct Gb_Apu apu;
	struct Gb_Cpu cpu;
	struct Stereo_Buffer stereo_buf;
	
	sample_t buf [buf_size];
	
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
blargg_err_t Gbs_load( struct Gbs_Emu* this, void* data, long size );

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Gbs_set_sample_rate( struct Gbs_Emu* this, long sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Gbs_start_track( struct Gbs_Emu* this, int );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Gbs_play( struct Gbs_Emu* this, long count, sample_t* buf ) ICODE_ATTR;

// Track status/control
// Number of milliseconds (1000 msec = 1 second) played since beginning of track
long Track_tell( struct Gbs_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Gbs_Emu* this, long msec );

// Skip n samples
blargg_err_t Track_skip( struct Gbs_Emu* this, long n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Gbs_Emu* this, long start_msec, long length_msec );

// Get track length in milliseconds
static inline long Track_get_length( struct Gbs_Emu* this, int n )
{
	long length = 120 * 1000;  /* 2 minutes */ 
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
static inline void Sound_set_gain( struct Gbs_Emu* this, double g )
{
	assert( !this->sample_rate_ ); // you must set gain before setting sample rate
	this->gain_ = g;
}


// Emulation (You shouldn't touch these)

blargg_err_t Run_clocks( struct Gbs_Emu* this, blip_time_t duration ) ICODE_ATTR;
void Set_bank( struct Gbs_Emu* this, int ) ICODE_ATTR;
void Update_timer( struct Gbs_Emu* this ) ICODE_ATTR;

// Runs CPU until time becomes >= 0
void Run_cpu( struct Gbs_Emu* this ) ICODE_ATTR;

// Reads/writes memory and I/O
int  Read_mem( struct Gbs_Emu* this, addr_t addr ) ICODE_ATTR;
void Write_mem( struct Gbs_Emu* this, addr_t addr, int data ) ICODE_ATTR;

// Current time
static inline blip_time_t Time( struct Gbs_Emu* this ) 
{
	return Cpu_time( &this->cpu ) + this->end_time; 
}

void Jsr_then_stop( struct Gbs_Emu* this, byte const [] ) ICODE_ATTR;
void Write_io_inline( struct Gbs_Emu* this, int offset, int data, int base ) ICODE_ATTR;
void Write_io_( struct Gbs_Emu* this, int offset, int data ) ICODE_ATTR;
int  Read_io(  struct Gbs_Emu* this, int offset ) ICODE_ATTR;
void Write_io( struct Gbs_Emu* this, int offset, int data ) ICODE_ATTR;

#endif
