/*
 * Copyright (c) 2003 Bob Deblier
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
 *
 */

/*!\file mpnumber.h
 * \brief Multi-precision numbers, headers.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup MP_m
 */

#ifndef _MPNUMBER_H
#define _MPNUMBER_H

#include "beecrypt/mp.h"

#ifdef __cplusplus
# include <iostream>
#endif

#ifdef __cplusplus
struct BEECRYPTAPI mpnumber
#else
struct _mpnumber
#endif
{
	size_t	size;
	mpw*	data;

#ifdef __cplusplus
	static const mpnumber ZERO;
	static const mpnumber ONE;

	mpnumber();
	mpnumber(unsigned int);
	mpnumber(const mpnumber&);
	~mpnumber();

	const mpnumber& operator=(const mpnumber&);
	bool operator==(const mpnumber&) const throw ();
	bool operator!=(const mpnumber&) const throw ();

	void wipe();

	size_t bitlength() const throw ();
#endif
};

#ifndef __cplusplus
typedef struct _mpnumber mpnumber;
#else
BEECRYPTAPI
std::ostream& operator<<(std::ostream&, const mpnumber&);
/*
BEECRYPTAPI
std::istream& operator>>(std::istream&, mpnumber&);
*/
#endif

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
void mpnzero(mpnumber*);
BEECRYPTAPI
void mpnsize(mpnumber*, size_t);
BEECRYPTAPI
void mpninit(mpnumber*, size_t, const mpw*);
BEECRYPTAPI
void mpnfree(mpnumber*);
BEECRYPTAPI
void mpncopy(mpnumber*, const mpnumber*);
BEECRYPTAPI
void mpnwipe(mpnumber*);

BEECRYPTAPI
void mpnset   (mpnumber*, size_t, const mpw*);
BEECRYPTAPI
void mpnsetw  (mpnumber*, mpw);

BEECRYPTAPI
int mpnsetbin(mpnumber*, const byte*, size_t);
BEECRYPTAPI
int mpnsethex(mpnumber*, const char*);

BEECRYPTAPI
int  mpninv(mpnumber*, const mpnumber*, const mpnumber*);

/*!\brief Truncate the mpnumber to the specified number of (least significant) bits.
 */
BEECRYPTAPI
size_t mpntrbits(mpnumber*, size_t);
BEECRYPTAPI
size_t mpnbits(const mpnumber*);

#ifdef __cplusplus
}
#endif

#endif
