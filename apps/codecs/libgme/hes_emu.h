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

typedef short sample_t;

enum { buf_size = 2048 };

// HES file header
enum { header_size = 0x20 };
struct header_t
{
	byte tag [4];
	byte vers;
	byte first_track;
	byte init_addr [2];
	byte banks [8];
	byte data_tag [4];
	byte size [4];
	byte addr [4];
	byte unused [4];
};


struct timer_t {
	hes_time_t last_time;
	blargg_long count;
	blargg_long load;
	int raw_load;
	byte enabled;
	byte fired;
};
	
struct vdp_t {
	hes_time_t next_vbl;
	byte latch;
	byte control;
};
	
struct irq_t {
	hes_time_t timer;
	hes_time_t vdp;
	byte disables;
};


struct Hes_Emu {
	hes_time_t play_period;
	hes_time_t last_frame_hook;
	int timer_base;
	
	struct timer_t timer;
	struct vdp_t vdp;
	struct irq_t irq;
	
	// Sound
	long clock_rate_;
	long sample_rate_;
	unsigned buf_changed_count;
	int voice_count_;
	int tempo_;
	int gain_;
	
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
	
	// Larger files at the end
	// Header for currently loaded file
	struct header_t header;
	
	// M3u Playlist
	struct M3u_Playlist m3u;
	
	// Hes Cpu
	byte* write_pages [page_count + 1]; // 0 if unmapped or I/O space	
	struct Hes_Cpu cpu;
	
	struct Hes_Apu apu;
	struct Hes_Apu_Adpcm adpcm;

	struct Stereo_Buffer stereo_buf;
	sample_t buf [buf_size];
	
	// rom & ram
	struct Rom_Data rom;
	byte sgx [3 * page_size + cpu_padding];
};


// Basic functionality
// Initializes Hes_Emu structure
void Hes_init( struct Hes_Emu* this );

// Stops (clear) Hes_Emu structure
void Hes_stop( struct Hes_Emu* this );

// Loads a file from memory
blargg_err_t Hes_load( struct Hes_Emu* this, void* data, long size );

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Hes_set_sample_rate( struct Hes_Emu* this, long sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Hes_start_track( struct Hes_Emu* this, int );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Hes_play( struct Hes_Emu* this, long count, sample_t* buf );

// Track status/control
// Number of milliseconds (1000 msec = 1 second) played since ning of track
long Track_tell( struct Hes_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Hes_Emu* this, long msec );

// Skip n samples
blargg_err_t Track_skip( struct Hes_Emu* this, long n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Hes_Emu* this, long start_msec, long length_msec );

// Get track length in milliseconds
static inline long Track_get_length( struct Hes_Emu* this, int n )
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

int Cpu_read( struct Hes_Emu* this, hes_addr_t );
void Cpu_write( struct Hes_Emu* this, hes_addr_t, int );
void Cpu_write_vdp( struct Hes_Emu* this, int addr, int data );
int Cpu_done( struct Hes_Emu* this );

int Emu_cpu_read( struct Hes_Emu* this, hes_addr_t );
void Emu_cpu_write( struct Hes_Emu* this, hes_addr_t, int data );

static inline byte const* Emu_cpu_set_mmr( struct Hes_Emu* this, int page, int bank )
{
	this->write_pages [page] = 0;
	if ( bank < 0x80 )
		return Rom_at_addr( &this->rom, bank * (blargg_long) page_size );
	
	byte* data = 0;
	switch ( bank )
	{
		case 0xF8:
			data = this->cpu.ram;
			break;
		
		case 0xF9:
		case 0xFA:
		case 0xFB:
			data = &this->sgx [(bank - 0xF9) * page_size];
			break;
		
		default:
			if ( bank != 0xFF ) {
				dprintf( "Unmapped bank $%02X\n", bank );
			}
			return this->rom.unmapped;
	}
	
	this->write_pages [page] = data;
	return data;
}

#endif
