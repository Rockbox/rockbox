// MSX computer KSS music file emulator

// Game_Music_Emu 0.5.5
#ifndef KSS_EMU_H
#define KSS_EMU_H

#include "gme.h"
#include "blargg_common.h"

#include "rom_data.h"
#include "multi_buffer.h"

#include "kss_scc_apu.h"
#include "z80_cpu.h"
#include "sms_apu.h"
#include "ay_apu.h"
#include "opl_apu.h"
#include "m3u_playlist.h"
#include "track_filter.h"

typedef int kss_time_t;
typedef int kss_addr_t;
typedef struct Z80_Cpu Kss_Cpu;

// Sound chip flags
enum {
	sms_psg_flag   = 1 << 0,
	sms_fm_flag    = 1 << 1,
	msx_psg_flag   = 1 << 2,
	msx_scc_flag   = 1 << 3,
	msx_music_flag = 1 << 4,
	msx_audio_flag = 1 << 5
};

enum { idle_addr = 0xFFFF };
enum { scc_enabled_true = 0xC000 };
enum { mem_size = 0x10000 };

// KSS file header
enum { header_size = 0x20 };
enum { header_base_size = 0x10 };
enum { header_ext_size = header_size - header_base_size };

struct header_t
{
	byte tag [4];
	byte load_addr [2];
	byte load_size [2];
	byte init_addr [2];
	byte play_addr [2];
	byte first_bank;
	byte bank_mode;
	byte extra_header;
	byte device_flags;
	
	// KSSX extended data, if extra_header==0x10
	byte data_size [4];
	byte unused [4];
	byte first_track [2];
	byte last_track [2]; // if no extended data, we set this to 0xFF
	byte psg_vol;
	byte scc_vol;
	byte msx_music_vol;
	byte msx_audio_vol;
};

struct sms_t {
	struct Sms_Apu psg;
	struct Opl_Apu fm;
};

struct msx_t {
	struct Ay_Apu  psg;
	struct Scc_Apu scc;
	struct Opl_Apu music;
	struct Opl_Apu audio;
};

struct Kss_Emu {
	struct header_t header;
	
	int chip_flags;
	bool scc_accessed;
	bool gain_updated;
	
	unsigned scc_enabled; // 0 or 0xC000
	int bank_count;
	
	blip_time_t play_period;
	blip_time_t next_play;
	int ay_latch;
	
	// general
	int voice_count;
	int const* voice_types;
	int mute_mask_;
	int tempo;
	int gain;
	
	int sample_rate;
	
	// track-specific
	int track_count;
	int current_track;
	
	int clock_rate_;
	unsigned buf_changed_count;
	
	// M3u Playlist
	struct M3u_Playlist m3u;
	
	struct setup_t tfilter;
	struct Track_Filter track_filter;
	
	struct sms_t sms;
	struct msx_t msx;

	Kss_Cpu cpu;
	struct Multi_Buffer stereo_buf; // NULL if using custom buffer
	struct Rom_Data rom;
	
	byte unmapped_read  [0x100];
	byte unmapped_write [page_size];
	byte ram [mem_size + cpu_padding];
};

// Basic functionality (see Gme_File.h for file loading/track info functions)

void Kss_init( struct Kss_Emu* this );
blargg_err_t Kss_load_mem( struct Kss_Emu* this, const void* data, long size );
blargg_err_t end_frame( struct Kss_Emu* this, kss_time_t );

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Kss_set_sample_rate( struct Kss_Emu* this, int sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Kss_start_track( struct Kss_Emu* this, int track );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Kss_play( struct Kss_Emu* this, int count, sample_t* buf );

// Track status/control

// Number of milliseconds (1000 msec = 1 second) played since beginning of track
int Track_tell( struct Kss_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Kss_Emu* this, int msec );

// Skip n samples
blargg_err_t Track_skip( struct Kss_Emu* this, int n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Kss_Emu* this, int start_msec, int length_msec );

// True if a track has reached its end
static inline bool Track_ended( struct Kss_Emu* this )
{
	return track_ended( &this->track_filter );
}

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void Track_ignore_silence( struct Kss_Emu* this, bool disable )
{
	this->track_filter.silence_ignored_ = disable;
}

// Get track length in milliseconds
static inline int Track_get_length( struct Kss_Emu* this, int n )
{
	int length = 0;
	
	if ( (this->m3u.size > 0) && (n < this->m3u.size) ) {
		struct entry_t* entry = &this->m3u.entries [n];
		length = entry->length;
	} 
	
	if ( length <= 0 )
		length = 120 * 1000;  /* 2 minutes */ 

	return length;
}

// Sound customization

// Adjust song tempo, where 1.0 = normal, 0.5 = half speed, 2.0 = double speed.
// Track length as returned by track_info() assumes a tempo of 1.0.
void Sound_set_tempo( struct Kss_Emu* this, int t );

// Mute/unmute voice i, where voice 0 is first voice
void Sound_mute_voice( struct Kss_Emu* this, int index, bool mute );

// Set muting state of all voices at once using a bit mask, where -1 mutes them all,
// 0 unmutes them all, 0x01 mutes just the first voice, etc.
void Sound_mute_voices( struct Kss_Emu* this, int mask );

// Change overall output amplitude, where 1.0 results in minimal clamping.
// Must be called before set_sample_rate().
static inline void Sound_set_gain( struct Kss_Emu* this, int g )
{
	assert( !this->sample_rate ); // you must set gain before setting sample rate
	this->gain = g;
}

// Emulation (You shouldn't touch these
void cpu_write( struct Kss_Emu* this, kss_addr_t, int );
int  cpu_in( struct Kss_Emu* this, kss_time_t, kss_addr_t );
void cpu_out( struct Kss_Emu* this, kss_time_t, kss_addr_t, int );

void cpu_write_( struct Kss_Emu* this, kss_addr_t addr, int data );
bool run_cpu( struct Kss_Emu* this, kss_time_t end );
void jsr( struct Kss_Emu* this, byte const addr [] );

static inline int sms_psg_enabled( struct Kss_Emu* this )   { return this->chip_flags & sms_psg_flag;  }
static inline int sms_fm_enabled( struct Kss_Emu* this )    { return this->chip_flags & sms_fm_flag;   }
static inline int msx_psg_enabled( struct Kss_Emu* this )   { return this->chip_flags & msx_psg_flag;  }
static inline int msx_scc_enabled( struct Kss_Emu* this )   { return this->chip_flags & msx_scc_flag;  }
static inline int msx_music_enabled( struct Kss_Emu* this ) { return this->chip_flags & msx_music_flag;}
static inline int msx_audio_enabled( struct Kss_Emu* this ) { return this->chip_flags & msx_audio_flag;}

#endif
