// TurboGrafx-16/PC Engine HES music file emulator

// Game_Music_Emu 0.5.2
#ifndef HES_EMU_H
#define HES_EMU_H

#include "blargg_source.h"

#include "multi_buffer.h"
#include "rom_data.h"
#include "hes_apu.h"
#include "hes_apu_adpcm.h"
#include "hes_cpu.h"
#include "m3u_playlist.h"
#include "track_filter.h"

// HES file header
enum { info_offset = 0x20 };
enum { header_size = 0x20 };
struct header_t
{
	byte tag       [4];
	byte vers;
	byte first_track;
	byte init_addr [2];
	byte banks     [8];
	byte data_tag  [4];
	byte size      [4];
	byte addr      [4];
	byte unused    [4];
};


struct timer_t {
	hes_time_t last_time;
	int        count;
	int        load;
	int        raw_load;
	byte       enabled;
	byte       fired;
};
	
struct vdp_t {
	hes_time_t next_vbl;
	byte       latch;
	byte       control;
};
	
struct irq_t {
	hes_time_t timer;
	hes_time_t vdp;
	byte       disables;
};

struct Hes_Emu {
	hes_time_t play_period;
	int timer_base;
	
	struct timer_t timer;
	struct vdp_t   vdp;
	struct irq_t   irq;
	
	// Sound
	int clock_rate_;
	int sample_rate_;
	unsigned buf_changed_count;
	int voice_count_;
	int const* voice_types_;
	int mute_mask_;
	int tempo_;
	int gain_;
	
	// track-specific
	byte track_count;
	int current_track_;
	
	// Larger files at the end
	// Header for currently loaded file
	struct header_t header;
	
	// M3u Playlist
	struct M3u_Playlist m3u;
	
	struct setup_t tfilter;
	struct Track_Filter track_filter;
	
	// Hes Cpu
	struct Hes_Cpu cpu;
	struct Rom_Data rom;
	
	struct Hes_Apu apu;
	struct Hes_Apu_Adpcm adpcm;

	struct Multi_Buffer stereo_buf;
	
	// rom & ram
	byte*   write_pages [page_count + 1]; // 0 if unmapped or I/O space	
	byte    ram [page_size];
	byte    sgx [3 * page_size + cpu_padding];
};


// Basic functionality
// Initializes Hes_Emu structure
void Hes_init( struct Hes_Emu* this );

// Stops (clear) Hes_Emu structure
void Hes_stop( struct Hes_Emu* this );

// Loads a file from memory
blargg_err_t Hes_load_mem( struct Hes_Emu* this, void* data, long size );

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Hes_set_sample_rate( struct Hes_Emu* this, int sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Hes_start_track( struct Hes_Emu* this, int );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Hes_play( struct Hes_Emu* this, int count, sample_t* buf );

// Track status/control
// Number of milliseconds (1000 msec = 1 second) played since ning of track
int Track_tell( struct Hes_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Hes_Emu* this, int msec );

// Skip n samples
blargg_err_t Track_skip( struct Hes_Emu* this, int n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Hes_Emu* this, int start_msec, int length_msec );

// True if a track has reached its end
static inline bool Track_ended( struct Hes_Emu* this )
{
	return track_ended( &this->track_filter );
}

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void Track_ignore_silence( struct Hes_Emu* this, bool disable )
{
	this->track_filter.silence_ignored_ = disable;
}

// Get track length in milliseconds
static inline int Track_get_length( struct Hes_Emu* this, int n )
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
void Sound_set_tempo( struct Hes_Emu* this, int );

// Mute/unmute voice i, where voice 0 is first voice
void Sound_mute_voice( struct Hes_Emu* this, int index, bool mute );

// Set muting state of all voices at once using a bit mask, where -1 mutes them all,
// 0 unmutes them all, 0x01 mutes just the first voice, etc.
void Sound_mute_voices( struct Hes_Emu* this, int mask );

// Change overall output amplitude, where 1.0 results in minimal clamping.
// Must be called before set_sample_rate().
static inline void Sound_set_gain( struct Hes_Emu* this, int g )
{
	assert( !this->sample_rate_ ); // you must set gain before setting sample rate
	this->gain_ = g;
}

// Emulation (You shouldn't touch these)

void irq_changed( struct Hes_Emu* this );
void run_until( struct Hes_Emu* this, hes_time_t );
bool run_cpu( struct Hes_Emu* this, hes_time_t end );
int  read_mem_( struct Hes_Emu* this, hes_addr_t );
int  read_mem( struct Hes_Emu* this, hes_addr_t );
void write_mem_( struct Hes_Emu* this, hes_addr_t, int data );
void write_mem( struct Hes_Emu* this, hes_addr_t, int );
void write_vdp( struct Hes_Emu* this, int addr, int data );
void set_mmr( struct Hes_Emu* this, int reg, int bank );
int  cpu_done( struct Hes_Emu* this );

#endif
