/*****************************************************************************
 * vlc_md5.h: MD5 hash
 *****************************************************************************
 * Copyright (C) 2004-2005 the VideoLAN team
 * $Id: 46f10f01439edbcc8bad45673cc302039b76dd5b $
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
 *          Sam Hocevar <sam@zoy.org>
 *
 * Adapted to Rockbox by: Antoine Cellerier <dionoea at videolan dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef _VLC_MD5_H
# define _VLC_MD5_H

/*****************************************************************************
 * md5_s: MD5 message structure
 *****************************************************************************
 * This structure stores the static information needed to compute an MD5
 * hash. It has an extra data buffer to allow non-aligned writes.
 *****************************************************************************/
struct md5_s
{
    uint64_t i_bits;      /* Total written bits */
    uint32_t p_digest[4]; /* The MD5 digest */
    uint32_t p_data[16];  /* Buffer to cache non-aligned writes */
};

void InitMD5( struct md5_s * );
void AddMD5( struct md5_s *, const void *, size_t );
void EndMD5( struct md5_s * );

/**
 * Returns a char representation of the md5 hash, as shown by UNIX md5 or
 * md5sum tools.
 */
#define MD5_STRING_LENGTH 32
void psz_md5_hash( char *, struct md5_s * );

#endif
