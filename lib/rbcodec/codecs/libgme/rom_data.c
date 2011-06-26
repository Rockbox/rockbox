// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "rom_data.h"

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include <string.h>
#include "blargg_source.h"

// Rom_Data

blargg_err_t Rom_load( struct Rom_Data* this, const void* data, long size,
		int header_size, void* header_out, int fill )
{
	int file_offset = this->pad_size;
	
	this->rom_addr = 0;
	this->mask     = 0;
	this->size    = 0;
	
	if ( size <= header_size ) // <= because there must be data after header
		return gme_wrong_file_type;
	
	// Read header
	memcpy( header_out, data, header_size );
	
	this->file_size = size - header_size;
	this->file_data = (byte*) data + header_size;
	
	memset( this->unmapped, fill, this->rom_size );
	memcpy( &this->unmapped [file_offset], this->file_data, 
		this->file_size < this->pad_size ? this->file_size : this->pad_size );
	
	return 0;
}

void Rom_set_addr( struct Rom_Data* this, int addr )
{
	this->rom_addr = addr - this->bank_size - pad_extra;
	
	int rounded = (addr + this->file_size + this->bank_size - 1) / this->bank_size * this->bank_size;
	if ( rounded <= 0 )
	{
		rounded = 0;
	}
	else
	{
		int shift = 0;
		unsigned int max_addr = (unsigned int) (rounded - 1);
		while ( max_addr >> shift )
			shift++;
		this->mask = (1L << shift) - 1;
	}
	
	if ( addr < 0 )
		addr = 0;
	this->size = rounded;
	this->rsize_ = rounded - this->rom_addr + pad_extra;
}
