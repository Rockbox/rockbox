// Multi-channel sound buffer interface, stereo and effects buffers

// Blip_Buffer 0.4.1
#ifndef MULTI_BUFFER_H
#define MULTI_BUFFER_H

#include "blargg_common.h"
#include "blip_buffer.h"

// Get indexed channel, from 0 to channel count - 1
struct channel_t {
	struct Blip_Buffer* center;
	struct Blip_Buffer* left;
	struct Blip_Buffer* right;
};

enum { type_index_mask = 0xFF };
enum { wave_type = 0x100, noise_type = 0x200, mixed_type = wave_type | noise_type };
enum { stereo = 2 };
enum { bufs_size = 3 };

// Tracked_Blip_Buffer
struct Tracked_Blip_Buffer {
	struct Blip_Buffer blip;
	int last_non_silence;
};

void Tracked_init( struct Tracked_Blip_Buffer* this );
unsigned Tracked_non_silent( struct Tracked_Blip_Buffer* this );
void Tracked_remove_all_samples( struct Tracked_Blip_Buffer * this );
int Tracked_read_samples( struct Tracked_Blip_Buffer* this, blip_sample_t [], int );
void Tracked_remove_silence( struct Tracked_Blip_Buffer* this, int );
void Tracked_remove_samples( struct Tracked_Blip_Buffer* this, int );
void Tracked_clear( struct Tracked_Blip_Buffer* this );
void Tracked_end_frame( struct Tracked_Blip_Buffer* this, blip_time_t );

static inline delta_t unsettled( struct Blip_Buffer* this )
{
	return this->reader_accum_ >> delta_bits;
}

// Stereo Mixer
struct Stereo_Mixer {
	struct Tracked_Blip_Buffer* bufs [3];
	int samples_read;
};

void Mixer_init( struct Stereo_Mixer* this );
void Mixer_read_pairs( struct Stereo_Mixer* this, blip_sample_t out [], int count );

typedef struct Tracked_Blip_Buffer buf_t;

// Multi_Buffer
struct Multi_Buffer {
	unsigned channels_changed_count_;
	int sample_rate_;
	int length_;
	int channel_count_;
	int samples_per_frame_;
	int const *channel_types_;
	bool immediate_removal_;

	buf_t bufs [bufs_size];
	struct Stereo_Mixer mixer;
	struct channel_t chan;
};

blargg_err_t Buffer_set_channel_count( struct Multi_Buffer* this, int n, int const* types );

// Buffers used for all channels
static inline struct Blip_Buffer* center( struct Multi_Buffer* this ) { return &this->bufs [2].blip; }
static inline struct Blip_Buffer* left( struct Multi_Buffer* this )   { return &this->bufs [0].blip; }
static inline struct Blip_Buffer* right( struct Multi_Buffer* this )  { return &this->bufs [1].blip; }

// Initializes Multi_Buffer structure
void Buffer_init( struct Multi_Buffer* this );

blargg_err_t Buffer_set_sample_rate( struct Multi_Buffer* this, int, int msec );
void Buffer_clock_rate( struct Multi_Buffer* this, int );
void Buffer_bass_freq( struct Multi_Buffer* this, int );
void Buffer_clear( struct Multi_Buffer* this );
void Buffer_end_frame( struct Multi_Buffer* this, blip_time_t ) ICODE_ATTR;

static inline int Buffer_length( struct Multi_Buffer* this )
{
	return this->length_;
}

// Count of changes to channel configuration. Incremented whenever
// a change is made to any of the Blip_Buffers for any channel.
static inline unsigned Buffer_channels_changed_count( struct Multi_Buffer* this )
{
	return this->channels_changed_count_;
}

static inline void Buffer_disable_immediate_removal( struct Multi_Buffer* this )
{
	this->immediate_removal_ = false;
}

static inline int Buffer_sample_rate( struct Multi_Buffer* this )
{
	return this->sample_rate_;
}

static inline int Buffer_samples_avail( struct Multi_Buffer* this )
{
	return (Blip_samples_avail(&this->bufs [0].blip) - this->mixer.samples_read) * 2;
}

int Buffer_read_samples( struct Multi_Buffer* this, blip_sample_t*, int ) ICODE_ATTR;
struct channel_t Buffer_channel( struct Multi_Buffer* this, int i );

#endif
