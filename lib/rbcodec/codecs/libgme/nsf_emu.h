// Nintendo NES/Famicom NSF music file emulator

// Game_Music_Emu 0.5.5
#ifndef NSF_EMU_H
#define NSF_EMU_H

#include "rom_data.h"
#include "multi_buffer.h"
#include "nes_apu.h"
#include "nes_cpu.h"
#include "nsfe_info.h"
#include "m3u_playlist.h"
#include "track_filter.h"

#ifndef NSF_EMU_APU_ONLY
	#include "nes_namco_apu.h"
	#include "nes_vrc6_apu.h"
	#include "nes_fme7_apu.h"
	#include "nes_fds_apu.h"
	#include "nes_mmc5_apu.h"
	#ifndef NSF_EMU_NO_VRC7
		#include "nes_vrc7_apu.h"
	#endif
#endif

// Sound chip flags
enum {
	vrc6_flag  = 1 << 0,
	vrc7_flag  = 1 << 1,
	fds_flag   = 1 << 2,
	mmc5_flag  = 1 << 3,
	namco_flag = 1 << 4,
	fme7_flag  = 1 << 5
};

enum { fds_banks    = 2 };
enum { bank_count   = fds_banks + 8 };

enum { rom_begin = 0x8000 };
enum { bank_select_addr = 0x5FF8 };
enum { mem_size  = 0x10000 };	
	
// cpu sits here when waiting for next call to play routine
enum { idle_addr = 0x5FF6 };
enum { banks_addr = idle_addr };
enum { badop_addr = bank_select_addr };

enum { low_ram_size = 0x800 };
enum { sram_size    = 0x2000 };
enum { fdsram_size  = 0x6000 };
enum { fdsram_offset = 0x2000 + page_size + 8 };
enum { sram_addr = 0x6000 };
enum { unmapped_size= page_size + 8 };
enum { max_voices = 32 };

// NSF file header
enum { header_size = 0x80 };
struct header_t
{
	char tag [5];
	byte vers;
	byte track_count;
	byte first_track;
	byte load_addr [2];
	byte init_addr [2];
	byte play_addr [2];
	char game [32];
	char author [32];
	char copyright [32];
	byte ntsc_speed [2];
	byte banks [8];
	byte pal_speed [2];
	byte speed_flags;
	byte chip_flags;
	byte unused [4];
};

struct Nsf_Emu {
	// Play routine timing
	nes_time_t next_play;
	nes_time_t play_period;
	int play_extra;
	int play_delay;
	struct registers_t saved_state; // of interrupted init routine

	// general
	int voice_count;
	int voice_types [32];
	int mute_mask_;
	int tempo;
	int gain;
	
	int sample_rate;
	
	// track-specific
	int track_count;
	int current_track;
	
	int clock_rate__;
	unsigned buf_changed_count;

	// M3u Playlist
	struct M3u_Playlist m3u;

	// Larger items at the end
	#ifndef NSF_EMU_APU_ONLY
		byte mmc5_mul [2];
		
		struct Nes_Fds_Apu   fds;
		struct Nes_Mmc5_Apu  mmc5;
		struct Nes_Namco_Apu namco;
		struct Nes_Vrc6_Apu  vrc6;
		struct Nes_Fme7_Apu  fme7;
		#ifndef NSF_EMU_NO_VRC7
			struct Nes_Vrc7_Apu  vrc7;
		#endif
	#endif
	
	struct Nes_Cpu cpu;
	struct Nes_Apu apu;
	
	// Header for currently loaded file
	struct header_t header;
	
	struct setup_t tfilter;
	struct Track_Filter track_filter;
	
	struct Multi_Buffer stereo_buf;
	struct Rom_Data rom;
	
	// Extended nsf info
	struct Nsfe_Info info;
	
	byte high_ram[fdsram_size + fdsram_offset];
	byte low_ram [low_ram_size];
};

// Basic functionality (see Gme_File.h for file loading/track info functions)

void Nsf_init( struct Nsf_Emu* this );
blargg_err_t Nsf_load_mem( struct Nsf_Emu* this, void* data, long size );
blargg_err_t Nsf_post_load( struct Nsf_Emu* this );

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Nsf_set_sample_rate( struct Nsf_Emu* this, int sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Nsf_start_track( struct Nsf_Emu* this , int );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Nsf_play( struct Nsf_Emu* this, int count, sample_t* buf );

void Nsf_clear_playlist( struct Nsf_Emu* this );
void Nsf_disable_playlist( struct Nsf_Emu* this, bool b ); // use clear_playlist()

// Track status/control

// Number of milliseconds (1000 msec = 1 second) played since beginning of track
int Track_tell( struct Nsf_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Nsf_Emu* this, int msec );

// Skip n samples
blargg_err_t Track_skip( struct Nsf_Emu* this, int n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Nsf_Emu* this, int start_msec, int length_msec );

// True if a track has reached its end
static inline bool Track_ended( struct Nsf_Emu* this )
{
	return track_ended( &this->track_filter );
}

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void Track_ignore_silence( struct Nsf_Emu* this, bool disable )
{
	this->track_filter.silence_ignored_ = disable;
}

// Get track length in milliseconds
int Track_length( struct Nsf_Emu* this, int n );

// Sound customization

// Adjust song tempo, where 1.0 = normal, 0.5 = half speed, 2.0 = double speed.
// Track length as returned by track_info() assumes a tempo of 1.0.
void Sound_set_tempo( struct Nsf_Emu* this, int t );

// Mute/unmute voice i, where voice 0 is first voice
void Sound_mute_voice( struct Nsf_Emu* this, int index, bool mute );

// Set muting state of all voices at once using a bit mask, where -1 mutes them all,
// 0 unmutes them all, 0x01 mutes just the first voice, etc.
void Sound_mute_voices( struct Nsf_Emu* this, int mask );

// Change overall output amplitude, where 1.0 results in minimal clamping.
// Must be called before set_sample_rate().
static inline void Sound_set_gain( struct Nsf_Emu* this, int g )
{
	assert( !this->sample_rate ); // you must set gain before setting sample rate
	this->gain = g;
}

// Emulation (You shouldn't touch these)

blargg_err_t run_clocks( struct Nsf_Emu* this, blip_time_t* duration, int );

void write_bank( struct Nsf_Emu* this, int index, int data );
int cpu_read( struct Nsf_Emu* this, addr_t );
void cpu_write( struct Nsf_Emu* this, addr_t, int );
addr_t get_addr( byte const [] );
bool run_cpu_until( struct Nsf_Emu* this, nes_time_t end );

// Sets clocks between calls to play routine to p + 1/2 clock
static inline void set_play_period( struct Nsf_Emu* this, int p ) { this->play_period = p; }

// Time play routine will next be called
static inline nes_time_t play_time( struct Nsf_Emu* this ) { return this->next_play; }

// Emulates to at least time t. Might emulate a few clocks extra.
void run_until( struct Nsf_Emu* this, nes_time_t t );

// Runs cpu to at least time t and returns false, or returns true
// if it encounters illegal instruction (halt).
bool run_cpu_until( struct Nsf_Emu* this, nes_time_t t );

// cpu calls through to these to access memory (except instructions)
int  read_mem(  struct Nsf_Emu* this, addr_t );
void write_mem( struct Nsf_Emu* this, addr_t, int );

// Address of play routine
static inline addr_t play_addr( struct Nsf_Emu* this ) { return get_addr( this->header.play_addr ); }

// Same as run_until, except emulation stops for any event (routine returned,
// play routine called, illegal instruction).
void run_once( struct Nsf_Emu* this, nes_time_t );

// Reads byte as cpu would when executing code. Only works for RAM/ROM,
// NOT I/O like sound chips.
int  read_code( struct Nsf_Emu* this, addr_t addr );

static inline byte* fdsram( struct Nsf_Emu* this )          { return &this->high_ram [fdsram_offset]; }
static inline byte* sram( struct Nsf_Emu* this )            { return this->high_ram; }
static inline byte* unmapped_code( struct Nsf_Emu* this )   { return &this->high_ram [sram_size]; }

#ifndef NSF_EMU_APU_ONLY
	static inline int fds_enabled( struct Nsf_Emu* this )   { return this->header.chip_flags & fds_flag;   }
	static inline int vrc6_enabled( struct Nsf_Emu* this )  { return this->header.chip_flags & vrc6_flag;  }
	#ifndef NSF_EMU_NO_VRC7
		static inline int vrc7_enabled( struct Nsf_Emu* this )  { return this->header.chip_flags & vrc7_flag;  }
	#endif
	static inline int mmc5_enabled( struct Nsf_Emu* this )  { return this->header.chip_flags & mmc5_flag;  }
	static inline int namco_enabled( struct Nsf_Emu* this ) { return this->header.chip_flags & namco_flag; }
	static inline int fme7_enabled( struct Nsf_Emu* this )  { return this->header.chip_flags & fme7_flag;  }
#endif
	
#endif
