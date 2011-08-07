// Band-limited sound synthesis buffer

// Blip_Buffer 0.4.1
#ifndef BLIP_BUFFER_H
#define BLIP_BUFFER_H

#include <assert.h>

	// internal
	#include "blargg_common.h"
	#if INT_MAX >= 0x7FFFFFFF
		typedef int blip_long;
		typedef unsigned blip_ulong;
	#else
		typedef long blip_long;
		typedef unsigned long blip_ulong;
	#endif

// Time unit at source clock rate
typedef blip_long blip_time_t;

// Number of bits in resample ratio fraction. Higher values give a more accurate ratio
// but reduce maximum buffer size.
#ifndef BLIP_BUFFER_ACCURACY
	#define BLIP_BUFFER_ACCURACY 16
#endif

// Number bits in phase offset. Fewer than 6 bits (64 phase offsets) results in
// noticeable broadband noise when synthesizing high frequency square waves.
// Affects size of Blip_Synth objects since they store the waveform directly.
#ifndef BLIP_PHASE_BITS
	#define BLIP_PHASE_BITS 8
#endif

// Output samples are 16-bit signed, with a range of -32768 to 32767
typedef short blip_sample_t;
enum { blip_sample_max = 32767 };
enum { blip_widest_impulse_ = 16 };
enum { blip_buffer_extra_ = blip_widest_impulse_ + 2 };
enum { blip_res = 1 << BLIP_PHASE_BITS };
enum { blip_max_length = 0 };
enum { blip_default_length = 250 };

// Maximun buffer size (48Khz, 50 ms)
enum { blip_buffer_max = 2466 };
enum { blip_sample_bits = 30 };

typedef blip_time_t buf_t_;
/* typedef const char* blargg_err_t; */
typedef blip_ulong blip_resampled_time_t;

struct Blip_Buffer {
	blip_ulong factor_;
	blip_resampled_time_t offset_;
	buf_t_ buffer_ [blip_buffer_max];
	blip_long buffer_size_;
	blip_long reader_accum_;
	int bass_shift_;

	long sample_rate_;
	long clock_rate_;
	int bass_freq_;
	int length_;
	int modified_;
};

// not documented yet
void Blip_set_modified( struct Blip_Buffer* this ) ICODE_ATTR;
int Blip_clear_modified( struct Blip_Buffer* this ) ICODE_ATTR;
void Blip_remove_silence( struct Blip_Buffer* this, long count ) ICODE_ATTR;
blip_resampled_time_t Blip_resampled_duration( struct Blip_Buffer* this, int t ) ICODE_ATTR;
blip_resampled_time_t Blip_resampled_time( struct Blip_Buffer* this, blip_time_t t ) ICODE_ATTR;
blip_resampled_time_t Blip_clock_rate_factor( struct Blip_Buffer* this, long clock_rate ) ICODE_ATTR;

// Initializes Blip_Buffer structure
void Blip_init( struct Blip_Buffer* this );

// Stops (clear) Blip_Buffer structure
void Blip_stop( struct Blip_Buffer* this );

// Set output sample rate and buffer length in milliseconds (1/1000 sec, defaults
// to 1/4 second), then clear buffer. Returns NULL on success, otherwise if there
// isn't enough memory, returns error without affecting current buffer setup.
blargg_err_t Blip_set_sample_rate( struct Blip_Buffer* this, long samples_per_sec, int msec_length );

// Set number of source time units per second
static inline void Blip_set_clock_rate( struct Blip_Buffer* this, long cps )
{
	this->factor_ = Blip_clock_rate_factor( this, this->clock_rate_ = cps );
}

// End current time frame of specified duration and make its samples available
// (along with any still-unread samples) for reading with read_samples(). Begins
// a new time frame at the end of the current frame.
void Blip_end_frame( struct Blip_Buffer* this, blip_time_t time ) ICODE_ATTR;

// Read at most 'max_samples' out of buffer into 'dest', removing them from from
// the buffer. Returns number of samples actually read and removed. If stereo is
// true, increments 'dest' one extra time after writing each sample, to allow
// easy interleving of two channels into a stereo output buffer.
long Blip_read_samples( struct Blip_Buffer* this, blip_sample_t* dest, long max_samples, int stereo ) ICODE_ATTR;

// Additional optional features

// Current output sample rate
static inline long Blip_sample_rate( struct Blip_Buffer* this )
{
	return this->sample_rate_;
}

// Length of buffer, in milliseconds
static inline int  Blip_length( struct Blip_Buffer* this )
{
	return this->length_;
}

// Number of source time units per second
static inline long Blip_clock_rate( struct Blip_Buffer* this )
{
	return this->clock_rate_;
}


// Set frequency high-pass filter frequency, where higher values reduce bass more
void Blip_bass_freq( struct Blip_Buffer* this, int frequency );

// Number of samples delay from synthesis to samples read out
static inline int  Blip_output_latency( void )
{ 
	return blip_widest_impulse_ / 2;
}

// Remove all available samples and clear buffer to silence. If 'entire_buffer' is
// false, just clears out any samples waiting rather than the entire buffer.
void Blip_clear( struct Blip_Buffer* this, int entire_buffer );

// Number of samples available for reading with read_samples()
static inline long Blip_samples_avail( struct Blip_Buffer* this )
{ 
	return (long) (this->offset_ >> BLIP_BUFFER_ACCURACY);
}

// Remove 'count' samples from those waiting to be read
void Blip_remove_samples( struct Blip_Buffer* this, long count ) ICODE_ATTR;

// Experimental features

// Count number of clocks needed until 'count' samples will be available.
// If buffer can't even hold 'count' samples, returns number of clocks until
// buffer becomes full.
blip_time_t Blip_count_clocks( struct Blip_Buffer* this, long count ) ICODE_ATTR;

// Number of raw samples that can be mixed within frame of specified duration.
long Blip_count_samples( struct Blip_Buffer* this, blip_time_t duration ) ICODE_ATTR;

// Mix 'count' samples from 'buf' into buffer.
void Blip_mix_samples( struct Blip_Buffer* this, blip_sample_t const* buf, long count ) ICODE_ATTR;

// Range specifies the greatest expected change in amplitude. Calculate it
// by finding the difference between the maximum and minimum expected
// amplitudes (max - min).

struct Blip_Synth {
	struct Blip_Buffer* buf;
	int last_amp;
	int delta_factor;
};

// Initializes Blip_Synth structure
void Synth_init( struct Blip_Synth* this );

// Set overall volume of waveform
void Synth_volume( struct Blip_Synth* this, double v ) ICODE_ATTR;

// Get/set Blip_Buffer used for output
const struct Blip_Buffer* Synth_output( struct Blip_Synth* this ) ICODE_ATTR;

// Low-level interface

	#if defined (__GNUC__) || _MSC_VER >= 1100
		#define BLIP_RESTRICT __restrict
	#else
		#define BLIP_RESTRICT
	#endif

// Works directly in terms of fractional output samples. Contact author for more info.
static inline void Synth_offset_resampled( struct Blip_Synth* this, blip_resampled_time_t time,
	int delta, struct Blip_Buffer* blip_buf )
{
	// Fails if time is beyond end of Blip_Buffer, due to a bug in caller code or the
	// need for a longer buffer as set by set_sample_rate().
	assert( (blip_long) (time >> BLIP_BUFFER_ACCURACY) < blip_buf->buffer_size_ );
	delta *= this->delta_factor;
	blip_long* BLIP_RESTRICT buf = blip_buf->buffer_ + (time >> BLIP_BUFFER_ACCURACY);
	int phase = (int) (time >> (BLIP_BUFFER_ACCURACY - BLIP_PHASE_BITS) & (blip_res - 1));

	blip_long left = buf [0] + delta;

	// Kind of crappy, but doing shift after multiply results in overflow.
	// Alternate way of delaying multiply by delta_factor results in worse
	// sub-sample resolution.
	blip_long right = (delta >> BLIP_PHASE_BITS) * phase;
	left  -= right;
	right += buf [1];

	buf [0] = left;
	buf [1] = right;
}

// Update amplitude of waveform at given time. Using this requires a separate
// Blip_Synth for each waveform.
static inline void Synth_update( struct Blip_Synth* this, blip_time_t t, int amp )
{
	int delta = amp - this->last_amp;
	this->last_amp = amp;
	Synth_offset_resampled( this, t * this->buf->factor_ + this->buf->offset_, delta, this->buf );
}

// Add an amplitude transition of specified delta, optionally into specified buffer
// rather than the one set with output(). Delta can be positive or negative.
// The actual change in amplitude is delta * (volume / range)
static inline void Synth_offset( struct Blip_Synth* this, blip_time_t t, int delta, struct Blip_Buffer* buf )
{
	Synth_offset_resampled( this, t * buf->factor_ + buf->offset_, delta, buf );
}

// Same as offset(), except code is inlined for higher performance
static inline void Synth_offset_inline( struct Blip_Synth* this, blip_time_t t, int delta, struct Blip_Buffer* buf )
{
	Synth_offset_resampled( this, t * buf->factor_ + buf->offset_, delta, buf );
}

// Optimized reading from Blip_Buffer, for use in custom sample output

// Begin reading from buffer. Name should be unique to the current block.
#define BLIP_READER_BEGIN( name, blip_buffer ) \
	buf_t_* BLIP_RESTRICT name##_reader_buf = (blip_buffer).buffer_;\
	blip_long name##_reader_accum = (blip_buffer).reader_accum_

// Get value to pass to BLIP_READER_NEXT()
#define BLIP_READER_BASS( blip_buffer ) ((blip_buffer).bass_shift_)

// Current sample
#define BLIP_READER_READ( name )        (name##_reader_accum >> (blip_sample_bits - 16))

// Current raw sample in full internal resolution
#define BLIP_READER_READ_RAW( name )    (name##_reader_accum)

// Advance to next sample
#define BLIP_READER_NEXT( name, bass ) \
	(void) (name##_reader_accum += *name##_reader_buf++ - (name##_reader_accum >> (bass)))

// End reading samples from buffer. The number of samples read must now be removed
// using Blip_remove_samples().
#define BLIP_READER_END( name, blip_buffer ) \
	(void) ((blip_buffer).reader_accum_ = name##_reader_accum)
	
#define BLIP_READER_ADJ_( name, offset ) (name##_reader_buf += offset)

#define BLIP_READER_NEXT_IDX_( name, bass, idx ) {\
	name##_reader_accum -= name##_reader_accum >> (bass);\
	name##_reader_accum += name##_reader_buf [(idx)];\
}

//// BLIP_CLAMP

#if defined (_M_IX86) || defined (_M_IA64) || defined (__i486__) || \
		defined (__x86_64__) || defined (__ia64__) || defined (__i386__)
	#define BLIP_X86 1
	#define BLIP_CLAMP_( in ) in < -0x8000 || 0x7FFF < in
#else
	#define BLIP_CLAMP_( in ) (blip_sample_t) in != in
#endif

// Clamp sample to blip_sample_t range
#define BLIP_CLAMP( sample, out )\
	{ if ( BLIP_CLAMP_( (sample) ) ) (out) = ((sample) >> 31) ^ 0x7FFF; }

#endif
