// Sega Master System/Mark III, Sega Genesis/Mega Drive, BBC Micro VGM music file emulator

// Game_Music_Emu 0.6-pre
#ifndef VGM_EMU_H
#define VGM_EMU_H

#include "blargg_common.h"
#include "blargg_source.h"

#include "track_filter.h"
#include "resampler.h"
#include "multi_buffer.h"
#include "ym2413_emu.h"
#include "ym2612_emu.h"
#include "sms_apu.h"

typedef int vgm_time_t;
typedef int fm_time_t;

enum { fm_time_bits = 12 };
enum { blip_time_bits = 12 };

// VGM header format
enum { header_size = 0x40 };
struct header_t
{
	char tag [4];
	byte data_size [4];
	byte version [4];
	byte psg_rate [4];
	byte ym2413_rate [4];
	byte gd3_offset [4];
	byte track_duration [4];
	byte loop_offset [4];
	byte loop_duration [4];
	byte frame_rate [4];
	byte noise_feedback [2];
	byte noise_width;
	byte unused1;
	byte ym2612_rate [4];
	byte ym2151_rate [4];
	byte data_offset [4];
	byte unused2 [8];
};

enum { gme_max_field = 63 };
struct track_info_t
{
	/* times in milliseconds; -1 if unknown */
	int length;
	int intro_length;
	int loop_length;
	
	/* empty string if not available */
	char game      [64];
	char song      [96];
	char author    [64];
};

// Emulates VGM music using SN76489/SN76496 PSG, YM2612, and YM2413 FM sound chips.
// Supports custom sound buffer and frequency equalization when VGM uses just the PSG.
// FM sound chips can be run at their proper rates, or slightly higher to reduce
// aliasing on high notes. Currently YM2413 support requires that you supply a
// YM2413 sound chip emulator. I can provide one I've modified to work with the library.
struct Vgm_Emu {
	int fm_rate;
	int psg_rate;
	int vgm_rate;
	bool disable_oversampling;
	
	int fm_time_offset;
	int fm_time_factor;

	int blip_time_factor;
	
	byte const* file_begin;
	byte const* file_end;
	
	vgm_time_t vgm_time;
	byte const* loop_begin;
	byte const* pos;
	
	byte const* pcm_data;
	byte const* pcm_pos;
	int dac_amp;
	int dac_disabled; // -1 if disabled

	// general
	int current_track;
	int clock_rate_;
	unsigned buf_changed_count;
	int max_initial_silence;
	int voice_count;
	int const *voice_types;
	int mute_mask_;
	int tempo;
	int gain;
	
	int sample_rate;
	
	// larger items at the end
	struct track_info_t info;
	
	struct setup_t tfilter;
	struct Track_Filter track_filter;

	struct Ym2612_Emu ym2612;
	struct Ym2413_Emu ym2413;
	
	struct Sms_Apu psg;
	struct Blip_Synth pcm;
	struct Blip_Buffer blip_buf;
	
	struct Resampler resampler;
	struct Multi_Buffer stereo_buf;
};

void Vgm_init( struct Vgm_Emu* this );

// Disable running FM chips at higher than normal rate. Will result in slightly
// more aliasing of high notes.
static inline void Vgm_disable_oversampling( struct Vgm_Emu* this, bool disable ) { this->disable_oversampling = disable; }

// Header for currently loaded file
static inline struct header_t *header( struct Vgm_Emu* this ) { return (struct header_t*) this->file_begin; }

// Basic functionality (see Gme_File.h for file loading/track info functions)
blargg_err_t Vgm_load_mem( struct Vgm_Emu* this, byte const* new_data, long new_size, bool parse_info );

// True if any FM chips are used by file. Always false until init_fm()
// is called.
static inline bool uses_fm( struct Vgm_Emu* this ) { return Ym2612_enabled( &this->ym2612 ) || Ym2413_enabled( &this->ym2413 ); }

// Set output sample rate. Must be called only once before loading file.
blargg_err_t Vgm_set_sample_rate( struct Vgm_Emu* this, int sample_rate );

// Start a track, where 0 is the first track. Also clears warning string.
blargg_err_t Vgm_start_track( struct Vgm_Emu* this );

// Generate 'count' samples info 'buf'. Output is in stereo. Any emulation
// errors set warning string, and major errors also end track.
blargg_err_t Vgm_play( struct Vgm_Emu* this, int count, sample_t* buf );

// Track status/control

// Number of milliseconds (1000 msec = 1 second) played since beginning of track
int Track_tell( struct Vgm_Emu* this );

// Seek to new time in track. Seeking backwards or far forward can take a while.
blargg_err_t Track_seek( struct Vgm_Emu* this, int msec );

// Skip n samples
blargg_err_t Track_skip( struct Vgm_Emu* this, int n );

// Set start time and length of track fade out. Once fade ends track_ended() returns
// true. Fade time can be changed while track is playing.
void Track_set_fade( struct Vgm_Emu* this, int start_msec, int length_msec );

// True if a track has reached its end
static inline bool Track_ended( struct Vgm_Emu* this )
{
	return track_ended( &this->track_filter );
}

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void Track_ignore_silence( struct Vgm_Emu* this, bool disable )
{
	this->track_filter.silence_ignored_ = disable;
}

// Get track length in milliseconds
static inline int Track_get_length( struct Vgm_Emu* this )
{
	int length = this->info.length;
	if ( length <= 0 )
	{
		length = this->info.intro_length + 2 * this->info.loop_length; // intro + 2 loops
		if ( length <= 0 )
			length = 150 * 1000; // 2.5 minutes
	}
	
	return length;
}

// Sound customization

// Adjust song tempo, where 1.0 = normal, 0.5 = half speed, 2.0 = double speed.
// Track length as returned by track_info() assumes a tempo of 1.0.
void Sound_set_tempo( struct Vgm_Emu* this, int t );

// Mute/unmute voice i, where voice 0 is first voice
void Sound_mute_voice( struct Vgm_Emu* this, int index, bool mute );

// Set muting state of all voices at once using a bit mask, where -1 mutes them all,
// 0 unmutes them all, 0x01 mutes just the first voice, etc.
void Sound_mute_voices( struct Vgm_Emu* this, int mask );

// Change overall output amplitude, where 1.0 results in minimal clamping.
// Must be called before set_sample_rate().
static inline void Sound_set_gain( struct Vgm_Emu* this, int g )
{
	assert( !this->sample_rate ); // you must set gain before setting sample rate
	this->gain = g;
}

#endif
