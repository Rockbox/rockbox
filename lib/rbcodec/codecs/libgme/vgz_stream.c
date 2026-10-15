// A gzipped VGM file, unpacked a little at a time as it is played.
//
// The whole compressed file is in memory (the codec asks for all of it), so
// only the output is streamed. tinfl writes into its own 32 KB wrapping
// dictionary, as deflate needs; what it writes is copied into a small linear
// window that the VGM command loop reads straight from, and refilled when
// that loop gets near its end. Going back, for a loop or a seek, means
// unpacking from the start again.

#include "blargg_common.h"
#include "blargg_endian.h"
#include "vgz_stream.h"

enum { window_size = 4096 };

// gzip header flags
enum { gz_fhcrc = 2, gz_fextra = 4, gz_fname = 8, gz_fcomment = 16 };

// Where the deflate data starts in a gzip file, or 0 if it is not one.
static size_t gzip_header_size( byte const* gz, size_t size )
{
	size_t p = 10;
	int flags;

	if ( size < 18 || gz [0] != 0x1F || gz [1] != 0x8B || gz [2] != 8 )
		return 0;
	flags = gz [3];
	if ( flags & gz_fextra )
		p += 2 + get_le16( gz + p );
	if ( flags & gz_fname )
		while ( p < size && gz [p++] ) { }
	if ( flags & gz_fcomment )
		while ( p < size && gz [p++] ) { }
	if ( flags & gz_fhcrc )
		p += 2;
	return p < size ? p : 0;
}

static void rewind_stream( struct vgz_stream* s )
{
	tinfl_init( &s->inflater );
	s->deflate_pos  = 0;
	s->dict_pos     = 0;
	s->pending_size = 0;
	s->inflate_done = false;
	s->window_used  = 0;
	s->window_base  = 0;
}

// Unpacks more into the dictionary, as pending bytes. Returns false at the
// end of the file, or of what could be unpacked of a damaged one.
static bool inflate_more( struct vgz_stream* s )
{
	while ( !s->inflate_done )
	{
		size_t in  = s->deflate_size - s->deflate_pos;
		size_t out = TINFL_LZ_DICT_SIZE - s->dict_pos;
		// All the input is there, but without TINFL_FLAG_HAS_MORE_INPUT
		// tinfl would go on past the end of a cut file as if it were
		// followed by zeros. With it, it stops there.
		tinfl_status status = tinfl_decompress( &s->inflater,
				s->deflate + s->deflate_pos, &in,
				s->dict, s->dict + s->dict_pos, &out,
				TINFL_FLAG_HAS_MORE_INPUT );

		s->deflate_pos += in;
		s->pending_pos  = s->dict_pos;
		s->pending_size = out;
		s->dict_pos     = (s->dict_pos + out) & (TINFL_LZ_DICT_SIZE - 1);
		if ( status != TINFL_STATUS_HAS_MORE_OUTPUT )
			s->inflate_done = true; // finished, cut short or bad
		if ( out )
			return true;
	}
	return false;
}

// Tops the window up from the dictionary, unpacking more as needed. The
// pending bytes are copied out before tinfl is called again, so it never
// writes over them.
static void fill_window( struct vgz_stream* s )
{
	while ( s->window_used < s->window_size )
	{
		size_t n;

		if ( !s->pending_size && !inflate_more( s ) )
			break;
		n = s->window_size - s->window_used;
		if ( n > s->pending_size )
			n = s->pending_size;
		memcpy( s->window + s->window_used, s->dict + s->pending_pos, n );
		s->window_used  += n;
		s->pending_pos  += n;
		s->pending_size -= n;
	}
}

struct vgz_stream* vgz_open( byte const* gz, size_t size )
{
	struct vgz_stream* s;
	size_t start = gzip_header_size( gz, size );

	if ( !start )
		return NULL;

	s = (struct vgz_stream*) codec_malloc( sizeof *s );
	if ( !s )
		return NULL;
	s->dict   = (byte*) codec_malloc( TINFL_LZ_DICT_SIZE );
	s->window = (byte*) codec_malloc( window_size );
	if ( !s->dict || !s->window )
		return NULL;

	s->deflate      = gz + start;
	s->deflate_size = size - start;
	s->window_size  = window_size;
	rewind_stream( s );
	fill_window( s );
	return s;
}

byte const* vgz_fill( struct vgz_stream* s, byte const* pos )
{
	size_t keep = vgz_end( s ) - pos;

	memmove( s->window, pos, keep );
	s->window_base += pos - s->window;
	s->window_used  = keep;
	fill_window( s );
	return s->window;
}

byte const* vgz_seek( struct vgz_stream* s, unsigned offset )
{
	byte const* pos;

	if ( offset < s->window_base )
		rewind_stream( s );

	if ( offset >= s->window_base + s->window_used )
	{
		// Skip to it without copying what lies between into the window
		s->window_base += s->window_used;
		s->window_used  = 0;
		while ( offset - s->window_base >= s->pending_size )
		{
			s->window_base += s->pending_size;
			s->pending_size = 0;
			if ( !inflate_more( s ) )
				return NULL;
		}
		s->pending_pos  += offset - s->window_base;
		s->pending_size -= offset - s->window_base;
		s->window_base   = offset;
		fill_window( s );
	}

	pos = s->window + (offset - s->window_base);
	if ( vgz_end( s ) - pos < vgz_lookahead )
		pos = vgz_fill( s, pos );
	return pos;
}

byte const* vgz_read( struct vgz_stream* s, byte const* pos, byte* out,
		unsigned size, unsigned* got )
{
	unsigned done = 0;

	while ( done < size )
	{
		size_t n = vgz_end( s ) - pos;

		if ( !n )
		{
			pos = vgz_fill( s, pos );
			if ( pos == vgz_end( s ) )
				break; // the file ends here
			continue;
		}
		if ( n > size - done )
			n = size - done;
		if ( out )
			memcpy( out + done, pos, n );
		pos  += n;
		done += n;
	}
	*got = done;

	if ( vgz_end( s ) - pos < vgz_lookahead )
		pos = vgz_fill( s, pos );
	return pos;
}
