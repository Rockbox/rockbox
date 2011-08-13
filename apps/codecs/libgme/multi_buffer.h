// Multi-channel sound buffer interface, and basic mono and stereo buffers

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
enum { buf_count = 3 };
	
struct Stereo_Buffer {
	struct Blip_Buffer bufs [buf_count];
	struct channel_t chan;
	int stereo_added;
	int was_stereo;
	
	unsigned channels_changed_count_;
	long sample_rate_;
	int length_;
	int samples_per_frame_;
};

// Initializes Stereo_Buffer structure
void Buffer_init( struct Stereo_Buffer* this );

blargg_err_t Buffer_set_sample_rate( struct Stereo_Buffer* this, long, int msec );
void Buffer_clock_rate( struct Stereo_Buffer* this, long );
void Buffer_bass_freq( struct Stereo_Buffer* this, int );
void Buffer_clear( struct Stereo_Buffer* this );
struct channel_t Buffer_channel( struct Stereo_Buffer* this );
void Buffer_end_frame( struct Stereo_Buffer* this, blip_time_t );

long Buffer_read_samples( struct Stereo_Buffer* this, blip_sample_t*, long );

// Count of changes to channel configuration. Incremented whenever
// a change is made to any of the Blip_Buffers for any channel.
unsigned Buffer_channels_changed_count( struct Stereo_Buffer* this );
void Buffer_channels_changed( struct Stereo_Buffer* this );

void Buffer_mix_stereo_no_center( struct Stereo_Buffer* this, blip_sample_t*, blargg_long );
void Buffer_mix_stereo( struct Stereo_Buffer* this, blip_sample_t*, blargg_long );
void Buffer_mix_mono( struct Stereo_Buffer* this, blip_sample_t*, blargg_long );

// Number of samples per output frame (1 = mono, 2 = stereo)
static inline int Buffer_samples_per_frame( struct Stereo_Buffer* this )
{
	return this->samples_per_frame_;
}

// See Blip_Buffer.h
static inline long Buffer_sample_rate( struct Stereo_Buffer* this )
{
	return this->sample_rate_;
}

// Length of buffer, in milliseconds
static inline int Buffer_length( struct Stereo_Buffer* this )
{
	return this->length_;
}

#endif
