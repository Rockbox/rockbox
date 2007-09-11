/*
   arc4.h - based on matrixssl-1-8-3-open

*/

/*
 *  Copyright (c) PeerSec Networks, 2002-2007. All Rights Reserved.
 *  The latest version of this code is available at http://www.matrixssl.org
 *
 *  This software is open source; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This General Public License does NOT permit incorporating this software 
 *  into proprietary programs.  If you are unable to comply with the GPL, a 
 *  commercial license for this software may be purchased from PeerSec Networks
 *  at http://www.peersec.com
 *  
 *  This program is distributed in WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  See the GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  http://www.gnu.org/copyleft/gpl.html
 */
/*****************************************************************************/

#ifndef _ARC4_H

#include <stdint.h>

struct rc4_key_t
{
    unsigned char   state[256];
    uint32_t  byteCount;
    unsigned char   x;
    unsigned char   y;
};

void matrixArc4Init(struct rc4_key_t *ctx, unsigned char *key, int32_t keylen);
int32_t matrixArc4(struct rc4_key_t *ctx, unsigned char *in,
                   unsigned char *out, int32_t len);

#endif
