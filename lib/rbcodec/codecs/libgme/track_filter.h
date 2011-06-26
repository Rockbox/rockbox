// Removes silence from beginning of track, fades end of track. Also looks ahead
// for excessive silence, and if found, ends track.

// Game_Music_Emu 0.6-pre
#ifndef TRACK_FILTER_H
#define TRACK_FILTER_H

#include "blargg_common.h"

typedef short sample_t;
typedef int sample_count_t;

enum { indefinite_count = INT_MAX/2 + 1 };
enum { buf_size = 2048 };

struct setup_t {
    sample_count_t max_initial; // maximum silence to strip from beginning of track
    sample_count_t max_silence; // maximum silence in middle of track without it ending
    int lookahead;   // internal speed when looking ahead for silence (2=200% etc.)
};

struct Track_Filter {
    void* emu_;
    struct setup_t setup_;
    const char* emu_error;
    bool silence_ignored_;
    
    // Timing
    int out_time;  // number of samples played since start of track
    int emu_time;  // number of samples emulator has generated since start of track
    int emu_track_ended_; // emulator has reached end of track
    volatile int track_ended_;
    
    // Fading
    int fade_start;
    int fade_step;
    
    // Silence detection
    int silence_time;   // absolute number of samples where most recent silence began
    int silence_count;  // number of samples of silence to play before using buf
    int buf_remain;     // number of samples left in silence buffer
    sample_t buf [buf_size];
};

// Initializes filter. Must be done once before using object.
blargg_err_t track_init( struct Track_Filter* this, void* );
void track_create( struct Track_Filter* this );

// Gets/sets setup
static inline struct setup_t const* track_get_setup( struct Track_Filter* this ) { return &this->setup_; }
static inline void track_setup( struct Track_Filter* this, struct setup_t const* s ) { this->setup_ = *s; }

// Disables automatic end-of-track detection and skipping of silence at beginning
static inline void track_ignore_silence( struct Track_Filter* this, bool disable ) { this->silence_ignored_ = disable; }

// Clears state and skips initial silence in track
blargg_err_t track_start( struct Track_Filter* this );
    
// Sets time that fade starts, and how long until track ends.
void track_set_fade( struct Track_Filter* this, sample_count_t start, sample_count_t length );

// Generates n samples into buf
blargg_err_t track_play( struct Track_Filter* this, int n, sample_t buf [] );

// Skips n samples
blargg_err_t track_skip( struct Track_Filter* this, int n );

// Number of samples played/skipped since start_track()
static inline int track_sample_count( struct Track_Filter* this ) { return this->out_time; }

// True if track ended. Causes are end of source samples, end of fade,
// or excessive silence.
static inline bool track_ended( struct Track_Filter* this ) { return this->track_ended_; }

// Clears state
void track_stop( struct Track_Filter* this );
    
// For use by callbacks

// Sets internal "track ended" flag and stops generation of further source samples
static inline void track_set_end( struct Track_Filter* this ) { this->emu_track_ended_ = true; }

// For use by skip_() callback
blargg_err_t skippy_( struct Track_Filter* this, int count );
void fill_buf( struct Track_Filter* this );

// Skip and play callbacks
blargg_err_t skip_( void* emu, int count );
blargg_err_t play_( void* emu, int count, sample_t out [] );
#endif
