// Band-limited sound synthesis buffer

// Blip_Buffer 0.4.1
#ifndef BLIP_BUFFER_H
#define BLIP_BUFFER_H

#include "blargg_common.h"

typedef unsigned blip_resampled_time_t;
typedef int blip_time_t;
typedef int clocks_t;

// Output samples are 16-bit signed, with a range of -32768 to 32767
typedef short blip_sample_t;

static int const blip_default_length = 1000 / 4;   // Default Blip_Buffer length (1/4 second)

#ifndef BLIP_MAX_QUALITY
	#define BLIP_MAX_QUALITY 2
#endif

#ifndef BLIP_BUFFER_ACCURACY
	#define BLIP_BUFFER_ACCURACY 16
#endif

// linear interpolation needs 8 bits
#ifndef BLIP_PHASE_BITS
	#define BLIP_PHASE_BITS 8
#endif

static int const blip_res           = 1 << BLIP_PHASE_BITS;
static int const blip_buffer_extra_ = BLIP_MAX_QUALITY + 2;

// Properties of fixed-point sample position
typedef unsigned ufixed_t; // unsigned for more range, optimized shifts
enum { fixed_bits = BLIP_BUFFER_ACCURACY };             // bits in fraction
enum { fixed_unit = 1 << fixed_bits };  // 1.0 samples

// Deltas in buffer are fixed-point with this many fraction bits.
// Less than 16 for extra range.
enum { delta_bits = 14 };

// Pointer to first committed delta sample
typedef int delta_t;

// Maximun buffer size (48Khz, 50 ms)
enum { blip_buffer_max = 2466 };

struct Blip_Buffer {
	unsigned factor_;
	ufixed_t  offset_;
	delta_t* buffer_center_;
	int      buffer_size_;
	int      reader_accum_;
	int      bass_shift_;
	int      bass_freq_;
	int      sample_rate_;
	int      clock_rate_;
	int      length_;
	bool     modified;

	delta_t  buffer_ [blip_buffer_max];
};

// Blip_Buffer_ implementation
static inline ufixed_t to_fixed( struct Blip_Buffer *this, clocks_t t )
{
	return t * this->factor_ + this->offset_;
}

static inline delta_t* delta_at( struct Blip_Buffer *this, ufixed_t f )
{
	assert( (f >> fixed_bits) < (unsigned) this->buffer_size_ );
	return this->buffer_center_ + (f >> fixed_bits);
}

// Number of samples available for reading with read_samples()
static inline int Blip_samples_avail( struct Blip_Buffer* this )
{
	return (int) (this->offset_ >> BLIP_BUFFER_ACCURACY);
}

static inline void Blip_remove_silence( struct Blip_Buffer* this, int count )
{
	assert( count <= Blip_samples_avail( this ) ); // tried to remove more samples than available
	this->offset_ -= (blip_resampled_time_t) count << BLIP_BUFFER_ACCURACY;
}

// Initializes Blip_Buffer structure
void Blip_init( struct Blip_Buffer* this );

// Sets output sample rate and resizes and clears sample buffer
blargg_err_t Blip_set_sample_rate( struct Blip_Buffer* this, int samples_per_sec, int msec_length );

// Current output sample rate
static inline int Blip_sample_rate( struct Blip_Buffer* this )
{
	return this->sample_rate_;
}

// Sets number of source time units per second
blip_resampled_time_t Blip_clock_rate_factor( struct Blip_Buffer* this, int clock_rate );
static inline void Blip_set_clock_rate( struct Blip_Buffer* this, int clocks_per_sec )
{
	this->factor_ = Blip_clock_rate_factor( this, this->clock_rate_ = clocks_per_sec );
}

// Number of source time units per second
static inline int Blip_clock_rate( struct Blip_Buffer* this )
{
	return this->clock_rate_;
}

static inline int Blip_length( struct Blip_Buffer* this )
{
	return this->length_;
}

// Clears buffer and removes all samples
void Blip_clear( struct Blip_Buffer* this );

// Use Blip_Synth to add waveform to buffer

// Resamples to time t, then subtracts t from current time. Appends result of resampling
// to buffer for reading.
void Blip_end_frame( struct Blip_Buffer* this, blip_time_t time ) ICODE_ATTR;


// Reads at most n samples to out [0 to n-1] and returns number actually read. If stereo
// is true, writes to out [0], out [2], out [4] etc. instead.
int Blip_read_samples( struct Blip_Buffer* this, blip_sample_t out [], int n, bool stereo ) ICODE_ATTR;


// More features

// Sets flag that tells some Multi_Buffer types that sound was added to buffer,
// so they know that it needs to be mixed in. Only needs to be called once
// per time frame that sound was added. Not needed if not using Multi_Buffer.
static inline void Blip_set_modified( struct Blip_Buffer* this ) { this->modified = true; }

// Set frequency high-pass filter frequency, where higher values reduce bass more
void Blip_bass_freq( struct Blip_Buffer* this, int frequency );


// Low-level features

// Removes the first n samples
void Blip_remove_samples( struct Blip_Buffer* this, int n ) ICODE_ATTR;

// Returns number of clocks needed until n samples will be available.
// If buffer cannot even hold n samples, returns number of clocks
// until buffer becomes full.
blip_time_t Blip_count_clocks( struct Blip_Buffer* this, int count ) ICODE_ATTR;

// Number of samples that should be mixed before calling Blip_end_frame( t )
int Blip_count_samples( struct Blip_Buffer* this, blip_time_t t ) ICODE_ATTR;

// Mixes n samples into buffer
void Blip_mix_samples( struct Blip_Buffer* this, blip_sample_t const in [], int n ) ICODE_ATTR;


// Resampled time (sorry, poor documentation right now)

// Resampled time is fixed-point, in terms of output samples.

// Converts clock count to resampled time
static inline blip_resampled_time_t Blip_resampled_duration( struct Blip_Buffer* this, int t )
{
	return t * this->factor_;
}

// Converts clock time since beginning of current time frame to resampled time
static inline blip_resampled_time_t Blip_resampled_time( struct Blip_Buffer* this, blip_time_t t )
{
	return t * this->factor_ + this->offset_;
}


// Range specifies the greatest expected change in amplitude. Calculate it
// by finding the difference between the maximum and minimum expected
// amplitudes (max - min).

typedef char coeff_t;

struct Blip_Synth {
	int delta_factor;
	int last_amp;
	struct Blip_Buffer* buf;
};

// Blip_Synth_
void volume_unit( struct Blip_Synth* this, int new_unit );

// Initializes Blip_Synth structure
void Synth_init( struct Blip_Synth* this );

// Sets volume of amplitude delta unit
static inline void Synth_volume( struct Blip_Synth* this, int v )
{
	volume_unit( this, v ); // new_unit = 1 / range * v
}


// Low-level interface

// (in >> sh & mask) * mul
#define BLIP_SH_AND_MUL( in, sh, mask, mul ) \
((int) (in) / ((1U << (sh)) / (mul)) & (unsigned) ((mask) * (mul)))

// (T*) ptr + (off >> sh)
#define BLIP_PTR_OFF_SH( T, ptr, off, sh ) \
	((T*) (BLIP_SH_AND_MUL( off, sh, -1, sizeof (T) ) + (char*) (ptr)))

// Works directly in terms of fractional output samples. Use resampled time functions in Blip_Buffer
// to convert clock counts to resampled time.
static inline void Synth_offset_resampled( struct Blip_Synth* this, blip_resampled_time_t time,
	int delta, struct Blip_Buffer* blip_buf )
{
	int const half_width = 1;

	delta_t* BLARGG_RESTRICT buf = delta_at( blip_buf, time );
	delta *= this->delta_factor;

	int const phase_shift = BLIP_BUFFER_ACCURACY - BLIP_PHASE_BITS;
	int const phase = (half_width & (half_width - 1)) ?
		(int) BLIP_SH_AND_MUL( time, phase_shift, blip_res - 1, sizeof (coeff_t) ) * half_width :
		(int) BLIP_SH_AND_MUL( time, phase_shift, blip_res - 1, sizeof (coeff_t) * half_width );

	int left = buf [0] + delta;

	// Kind of crappy, but doing shift after multiply results in overflow.
	// Alternate way of delaying multiply by delta_factor results in worse
	// sub-sample resolution.
	int right = (delta >> BLIP_PHASE_BITS) * phase;
	#ifdef BLIP_BUFFER_NOINTERP
		// TODO: remove? (just a hack to see how it sounds)
		right = 0;
	#endif
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
	Synth_offset_resampled( this, to_fixed(this->buf, t), delta, this->buf );
}

// Adds amplitude transition at time t. Delta can be positive or negative.
// The actual change in amplitude is delta * volume.
static inline void Synth_offset_inline( struct Blip_Synth* this, blip_time_t t, int delta, struct Blip_Buffer* buf )
{
	Synth_offset_resampled( this, to_fixed(buf, t), delta, buf );
}

#define Synth_offset( synth, time, delta, buf ) Synth_offset_inline( synth, time, delta, buf )

// Number of bits in raw sample that covers normal output range. Less than 32 bits to give
// extra amplitude range. That is,
// +1 << (blip_sample_bits-1) = +1.0
// -1 << (blip_sample_bits-1) = -1.0
static int const blip_sample_bits = 30;

// Optimized reading from Blip_Buffer, for use in custom sample output

// Begin reading from buffer. Name should be unique to the current block.
#define BLIP_READER_BEGIN( name, blip_buffer ) \
	const delta_t* BLARGG_RESTRICT name##_reader_buf = (blip_buffer).buffer_;\
	int name##_reader_accum = (blip_buffer).reader_accum_

// Get value to pass to BLIP_READER_NEXT()
#define BLIP_READER_BASS( blip_buffer ) ((blip_buffer).bass_shift_)

// Constant value to use instead of BLIP_READER_BASS(), for slightly more optimal
// code at the cost of having no bass_freq() functionality
static int const blip_reader_default_bass = 9;

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

#define BLIP_READER_NEXT_RAW_IDX_( name, bass, idx ) {\
	name##_reader_accum -= name##_reader_accum >> (bass);\
	name##_reader_accum +=\
	*(delta_t const*) ((char const*) name##_reader_buf + (idx));\
}

//// BLIP_CLAMP

#if defined(CPU_ARM) && (ARM_ARCH >= 6)
	#define BLIP_CLAMP( sample, out ) \
		({ \
		asm ("ssat %0, #16, %1" \
				: "=r" ( out ) : "r"( sample ) ); \
		out; \
		})
#else
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

#endif
