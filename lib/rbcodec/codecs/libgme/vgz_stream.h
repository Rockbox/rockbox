// A gzipped VGM file, unpacked a little at a time as it is played

#ifndef VGZ_STREAM_H
#define VGZ_STREAM_H

#include "blargg_common.h"
#include "inflate/tinfl.h"

// Bytes run() may read from a command before checking again: the longest
// command, with room to spare.
enum { vgz_lookahead = 16 };

struct vgz_stream {
	tinfl_decompressor inflater;
	byte const* deflate;        // the compressed data, all in memory
	size_t deflate_size;
	size_t deflate_pos;         // how much of it has been read
	byte* dict;                 // tinfl's wrapping output, TINFL_LZ_DICT_SIZE
	size_t dict_pos;            // where it writes next
	size_t pending_pos;         // unpacked bytes in dict not yet in the window
	size_t pending_size;
	bool inflate_done;          // tinfl has finished, or failed
	byte* window;               // what run() reads from
	size_t window_size;
	size_t window_used;
	unsigned window_base;       // offset in the unpacked file of window [0]
};

// Starts on a gzip file of size bytes. The stream and its buffers come from
// codec_malloc(). Returns NULL if it is not a gzip file or there is no room.
struct vgz_stream* vgz_open( byte const* gz, size_t size );

// Goes to offset in the unpacked file, unpacking it again from the start if
// it is behind what has been unpacked. Returns a pointer into the window, or
// NULL if the file ends first.
byte const* vgz_seek( struct vgz_stream* s, unsigned offset );

// Moves the bytes from pos on to the start of the window and fills the rest.
// Returns the new pos.
byte const* vgz_fill( struct vgz_stream* s, byte const* pos );

// The end of what is in the window.
static inline byte const* vgz_end( struct vgz_stream const* s )
{
	return s->window + s->window_used;
}

// Offset in the unpacked file of pos, a pointer into the window.
static inline unsigned vgz_tell( struct vgz_stream const* s, byte const* pos )
{
	return s->window_base + (pos - s->window);
}

// Copies size bytes from pos on to out (or skips them, if out is NULL),
// filling the window as needed. Returns the new pos; *got is how many bytes
// there were.
byte const* vgz_read( struct vgz_stream* s, byte const* pos, byte* out,
		unsigned size, unsigned* got );

#endif
