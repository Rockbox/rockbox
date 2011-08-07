// Combination of Downsampler and Blip_Buffer mixing. Used by Sega FM emulators.

// Game_Music_Emu 0.5.5
#ifndef RESAMPLER_H
#define RESAMPLER_H

#include "blargg_config.h"
#include "multi_buffer.h"

typedef short dsample_t;

enum { stereo = 2 };
enum { max_buf_size = 3960 };
enum { max_resampler_size = 5942 };
enum { write_offset = 8 * stereo };
enum { gain_bits = 14 };

struct Resampler {		
	int (*callback)( void*, blip_time_t, int, dsample_t* );
	void* callback_data;
	
	dsample_t sample_buf [max_buf_size];
	int sample_buf_size;
	int oversamples_per_frame;
	int buf_pos;
	int resampler_size;
	int gain_;
	
	// Internal resampler
	dsample_t buf [max_resampler_size];
	int buffer_size;
	
	int write_pos;
	double rate_;
	
	int pos;
	int step;
};

static inline void Resampler_init( struct Resampler* this )
{
	this->pos = 0;
	this->write_pos = 0;
	this->rate_     = 0;
}

blargg_err_t Resampler_reset( struct Resampler* this, int max_pairs );
void Resampler_resize( struct Resampler* this, int pairs_per_frame );
	
void Resampler_play( struct Resampler* this, long count, dsample_t* out, struct Stereo_Buffer* ) ICODE_ATTR;

static inline void Resampler_set_callback(struct Resampler* this, int (*func)( void*, blip_time_t, int, dsample_t* ), void* user_data )
{
	this->callback = func;
	this->callback_data = user_data;
}

blargg_err_t Resampler_setup( struct Resampler* this, double oversample, double rolloff, double gain );

static inline void Resampler_clear( struct Resampler* this )
{
	this->buf_pos = this->sample_buf_size;

	this->pos = 0;
	this->write_pos = 0;
}

#endif
