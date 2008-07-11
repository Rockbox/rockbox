/*
 * Copyright (c) 2001 Virtual Unlimited B.V.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*!\file memchunk.h
 * \author Bob Deblier <bob.deblier@pandora.be>
 */

#ifndef _MEMCHUNK_H
#define _MEMCHUNK_H

#include "beecrypt/api.h"

typedef struct
{
	size_t	size;
	byte*	data;
} memchunk;

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
memchunk*	memchunkAlloc(size_t);
BEECRYPTAPI
void		memchunkWipe(memchunk*);
BEECRYPTAPI
void		memchunkFree(memchunk*);
BEECRYPTAPI
memchunk*	memchunkResize(memchunk*, size_t);
BEECRYPTAPI
memchunk*	memchunkClone(const memchunk*);

#ifdef __cplusplus
}
#endif

#endif
