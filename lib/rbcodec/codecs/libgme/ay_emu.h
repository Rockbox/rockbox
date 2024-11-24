// Sinclair Spectrum AY music file emulator

// Game_Music_Emu 0.6-pre
#ifndef AY_EMU_H
#define AY_EMU_H

#include "blargg_source.h"

#include "multi_buffer.h"
#include "z80_cpu.h"
#include "ay_apu.h"
#include "m3u_playlist.h"
#include "track_filter.h"

// 64K memory to load code and data into before starting track. Caller
// must parse the AY file.
enum { mem_size = 0x10000 };
enum { ram_addr = 0x4000 }; // where official RAM starts

// AY file header
enum { header_size = 0x14 };
struct header_t
{
	byte tag        [8];
	byte vers;
	byte player;
	byte unused     [2];
	byte author     [2];
	byte comment    [2];
	byte max_track;
	byte first_track;
	byte track_info [2];
};

struct file_t {
	struct header_t const* header;
	byte const* tracks;
	byte const* end;    // end of file data
};

struct mem_t {
	uint8_t padding1 [0x100];
	uint8_t ram      [mem_size + 0x100];
};

struct Ay_Emu {
	struct file_t file;

	struct Blip_Buffer* beeper_output;
	int          beeper_delta;
	int          last_beeper;
	int          beeper_mask;
	
	addr_t play_addr;
	cpu_time_t play_period;
	cpu_time_t next_play;
	
	int  cpc_latch;
	bool spectrum_mode;
	bool cpc_mode;
	
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
	
	// large items
	struct setup_t tfilter;
	struct Track_Filter track_filter;
	
	struct Ay_Apu apu;
	struct Multi_Buffer stereo_buf; // NULL if using custom buffer
	struct Z80_Cpu cpu;
	struct mem_t mem;
};

// Basic functionality (see Gme_File.h for file loading/track info functions)
void Ay_init( struct Ay_Emu* this );

blargg_err_t Ay_load_mem( struct Ay_Emu* this, byte const in [], long size );

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Ay_set_sample_rate( struct Ay_Emu* this, int sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Ay_start_track( struct Ay_Emu* this, int track );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Ay_play( struct Ay_Emu* this, int count, sample_t* buf );


// Track status/control

// Number of milliseconds (1000 msec = 1 second) played since beginning of track
int Track_tell( struct Ay_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Ay_Emu* this, int msec );

// Skip n samples
blargg_err_t Track_skip( struct Ay_Emu* this, int n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Ay_Emu* this, int start_msec, int length_msec );

// True if a track has reached its end
static inline bool Track_ended( struct Ay_Emu* this )
{
	return track_ended( &this->track_filter );
}

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void Track_ignore_silence( struct Ay_Emu* this, bool disable )
{
	this->track_filter.silence_ignored_ = disable;
}

// Get track length in milliseconds
int Track_get_length( struct Ay_Emu* this, int n );

// Sound customization

// Adjust song tempo, where 1.0 = normal, 0.5 = half speed, 2.0 = double speed.
// Track length as returned by track_info() assumes a tempo of 1.0.
void Sound_set_tempo( struct Ay_Emu* this, int t );

// Mute/unmute voice i, where voice 0 is first voice
void Sound_mute_voice( struct Ay_Emu* this, int index, bool mute );

// Set muting state of all voices at once using a bit mask, where -1 mutes them all,
// 0 unmutes them all, 0x01 mutes just the first voice, etc.
void Sound_mute_voices( struct Ay_Emu* this, int mask );

// Change overall output amplitude, where 1.0 results in minimal clamping.
// Must be called before set_sample_rate().
static inline void Sound_set_gain( struct Ay_Emu* this, int g )
{
	assert( !this->sample_rate ); // you must set gain before setting sample rate
	this->gain = g;
}

// Emulation (You shouldn't touch these)
void cpu_out( struct Ay_Emu* this, cpu_time_t, addr_t, int data );
void cpu_out_( struct Ay_Emu* this, cpu_time_t, addr_t, int data );
bool run_cpu( struct Ay_Emu* this, cpu_time_t end );

static inline void disable_beeper( struct Ay_Emu *this )
{
	this->beeper_mask = 0;
	this->last_beeper = 0;
}

#endif
