// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

#include "nsf_emu.h"

#include "blargg_endian.h"
#include <string.h>

/* Copyright (C) 2005-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

void Info_init( struct Nsfe_Info* this )
{ 
	this->playlist_disabled = false; 
}

void Info_unload( struct Nsfe_Info* this )
{
	memset(this->playlist, 0, 256);
	memset(this->track_times, 0, 256 * sizeof(int32_t));
	
	this->playlist_size = 0;
	this->track_times_size = 0;
}

// TODO: if no playlist, treat as if there is a playlist that is just 1,2,3,4,5... ?
void Info_disable_playlist( struct Nsfe_Info* this, bool b )
{
	this->playlist_disabled = b;
	this->track_count = this->playlist_size;
	if ( !this->track_count || this->playlist_disabled )
		this->track_count = this->actual_track_count_;
}

int Info_remap_track( struct Nsfe_Info* this, int track )
{
	if ( !this->playlist_disabled && (unsigned) track < (unsigned) this->playlist_size )
		track = this->playlist [track];
	return track;
}

const char eof_error [] = "Unexpected end of file";

// Read n bytes from memory buffer
static blargg_err_t in_read( void* dst, long bytes, void* data, long* offset, long size )
{
	if ((*offset + bytes) > size) return eof_error;
	
	memcpy(dst, (char*) data + *offset, bytes);
	*offset += bytes;
	return 0;
}

static blargg_err_t in_skip( long bytes, long *offset, long size )
{
	if ((*offset + bytes) > size) return eof_error;
	
	*offset += bytes;
	return 0;
}

// Skip n bytes from memory buffer

// Read multiple strings and separate into individual strings
static int read_strs( void* data, long bytes, long* offset, long size,
		const char* strs [4] )
{
	char* chars = (char*) data + *offset;
	chars [bytes - 1] = 0; // in case last string doesn't have terminator
	
	if ( in_skip( bytes, offset, size) )
		return -1;

	int count = 0, i;
	for ( i = 0; i < bytes; i++ )
	{
		strs [count] = &chars [i];
		while ( i < bytes && chars [i] )
			i++;
		
		count++;
		if (count >= 4) 
			break;
	}
	
	return count;
}

struct nsfe_info_t
{
	byte load_addr [2];
	byte init_addr [2];
	byte play_addr [2];
	byte speed_flags;
	byte chip_flags;
	byte track_count;
	byte first_track;
	byte unused [6];
};

blargg_err_t Info_load( struct Nsfe_Info* this, void* data, long size, struct Nsf_Emu* nsf_emu )
{
	long offset = 0;
	int const nsfe_info_size = 16;
	assert( offsetof (struct nsfe_info_t,unused [6]) == nsfe_info_size );
	
	// check header
	byte signature [4];
	blargg_err_t err = in_read( signature, sizeof signature, data, &offset, size );
	if ( err )
		return (err == eof_error ? gme_wrong_file_type : err);
	if ( memcmp( signature, "NSFE", 4 ) ) {
	}
	
	// free previous info
	/* TODO: clear track_names */
	memset(this->playlist, 0, 256);
	memset(this->track_times, 0, 256 * sizeof(int32_t));
	
	this->playlist_size = 0;
	this->track_times_size = 0;
	
	// default nsf header
	static const struct header_t base_header =
	{
		{'N','E','S','M','\x1A'},// tag
		1,                  // version
		1, 1,               // track count, first track
		{0,0},{0,0},{0,0},  // addresses
		"","","",           // strings
		{0x1A, 0x41},       // NTSC rate
		{0,0,0,0,0,0,0,0},  // banks
		{0x20, 0x4E},       // PAL rate
		0, 0,               // flags
		{0,0,0,0}           // unused
	};
	
	memcpy( &nsf_emu->header, &base_header, sizeof base_header );
	
	// parse tags
	int phase = 0;
	while ( phase != 3 )
	{
		// read size and tag
		byte block_header [2] [4];
		RETURN_ERR( in_read( block_header, sizeof block_header, data, &offset, size ) );
		
		int chunk_size = get_le32( block_header [0] );
		int tag  = get_le32( block_header [1] );
		
		switch ( tag )
		{
			case BLARGG_4CHAR('O','F','N','I'): {
				check( phase == 0 );
				if ( chunk_size < 8 )
					return "Corrupt file";
				
				struct nsfe_info_t finfo;
				finfo.track_count = 1;
				finfo.first_track = 0;
				
				RETURN_ERR( in_read( &finfo, min( chunk_size, nsfe_info_size ), 
					(char*) data, &offset, size ) );
				
				if ( chunk_size > nsfe_info_size )
					RETURN_ERR( in_skip( chunk_size - nsfe_info_size, &offset, size ) );
					
				phase = 1;
				nsf_emu->header.speed_flags = finfo.speed_flags;
				nsf_emu->header.chip_flags  = finfo.chip_flags;
				nsf_emu->header.track_count = finfo.track_count;
				this->actual_track_count_ = finfo.track_count;
				nsf_emu->header.first_track = finfo.first_track;
				memcpy( nsf_emu->header.load_addr, finfo.load_addr, 2 * 3 );
				break;
			}
			
			case BLARGG_4CHAR('K','N','A','B'):
				if ( chunk_size > (int) sizeof nsf_emu->header.banks )
					return "Corrupt file";
				RETURN_ERR( in_read( nsf_emu->header.banks, chunk_size, data, &offset, size ) );
				break;
			
			case BLARGG_4CHAR('h','t','u','a'): {
				const char* strs [4];
				int n = read_strs( data, chunk_size, &offset, size, strs );
				if ( n < 0 ) 
					return eof_error;
				break;
			}
			
			case BLARGG_4CHAR('e','m','i','t'):
				this->track_times_size = chunk_size / 4;
				RETURN_ERR( in_read( this->track_times, this->track_times_size * 4, data, &offset, size ) );
				break;
			
			case BLARGG_4CHAR('l','b','l','t'):
				RETURN_ERR( in_skip( chunk_size, &offset, size ) );
				break;
			
			case BLARGG_4CHAR('t','s','l','p'):
				this->playlist_size = chunk_size;
				RETURN_ERR( in_read( &this->playlist [0], chunk_size, data, &offset, size ) );
				break;
			
			case BLARGG_4CHAR('A','T','A','D'): {
				check( phase == 1 );
				phase = 2;
				if ( !nsf_emu )
				{
					RETURN_ERR( in_skip( chunk_size, &offset, size ) );
				}
				else
				{
					// Avoid unexpected end of file
					if ( (offset + chunk_size) > size )
						return eof_error;
						
					RETURN_ERR( Rom_load( &nsf_emu->rom, (char*) data + offset, chunk_size, 0, 0, 0 ) );
					RETURN_ERR( Nsf_post_load( nsf_emu ) );
					offset += chunk_size;
				}
				break;
			}
			
			case BLARGG_4CHAR('D','N','E','N'):
				check( phase == 2 );
				phase = 3;
				break;
			
			default:
				// tags that can be skipped start with a lowercase character
				check( islower( (tag >> 24) & 0xFF ) );
				RETURN_ERR( in_skip( chunk_size, &offset, size ) );
				break;
		}
	}
	
	return 0;
}

int Track_length( struct Nsf_Emu* this, int n )
{
	int length = 0; 
	if ( (this->m3u.size > 0) && (n < this->m3u.size) ) {
		struct entry_t* entry = &this->m3u.entries [n];
		length = entry->length;
	} 
	else if ( (this->info.playlist_size > 0) && (n < this->info.playlist_size) ) {
		int remapped = Info_remap_track( &this->info, n );
		if ( (unsigned) remapped < (unsigned) this->info.track_times_size )
			length = (int32_t) get_le32( &this->info.track_times [remapped] );
	}
	else if( (unsigned) n < (unsigned) this->info.track_times_size )
		length = (int32_t) get_le32( &this->info.track_times [n] );
	
	/* Length will be 2,30 minutes for one track songs,
		and 1,45 minutes for multitrack songs */
	if ( length <= 0 )
		length = (this->track_count > 1 ? 105 : 150) * 1000;
	
	return length;
}
