/*
 * Copyright (c) 2002, 2003 Bob Deblier
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

/*!\file mp.h
 * \brief Multi-precision integer routines.
 *
 * The routines declared here are all low-level operations, most of them
 * suitable to be implemented in assembler. Prime candidates are in order
 * of importance (according to gprof):
 * <ul>
 *  <li>mpaddmul
 *  <li>mpsetmul 
 *  <li>mpaddsqrtrc
 *  <li>mpsub
 *  <li>mpadd
 * </ul>
 *
 * With some smart use of available assembler instructions, it's possible
 * to speed these routines up by a factor of 2 to 4.
 *
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup MP_m
 */

#ifndef _MP_H
#define _MP_H

#include "beecrypt/api.h"
#include "beecrypt/mpopt.h"

#define MP_HWBITS	(MP_WBITS >> 1)
#define MP_WBYTES	(MP_WBITS >> 3)
#define MP_WNIBBLES	(MP_WBITS >> 2)

#if (MP_WBITS == 64)
# define MP_WORDS_TO_BITS(x)	((x) << 6)
# define MP_WORDS_TO_NIBBLES(x)	((x) << 4)
# define MP_WORDS_TO_BYTES(x)	((x) << 3)
# define MP_BITS_TO_WORDS(x)	((x) >> 6)
# define MP_NIBBLES_TO_WORDS(x)	((x) >> 4)
# define MP_BYTES_TO_WORDS(x)	((x) >> 3)
#elif (MP_WBITS == 32)
# define MP_WORDS_TO_BITS(x)	((x) << 5)
# define MP_WORDS_TO_NIBBLES(x)	((x) << 3)
# define MP_WORDS_TO_BYTES(x)	((x) << 2)
# define MP_BITS_TO_WORDS(x)	((x) >> 5) 
# define MP_NIBBLES_TO_WORDS(x)	((x) >> 3)
# define MP_BYTES_TO_WORDS(x)	((x) >> 2)
#else
# error
#endif

#define MP_MSBMASK	(((mpw) 0x1) << (MP_WBITS-1))
#define MP_LSBMASK	 ((mpw) 0x1)
#define MP_ALLMASK	~((mpw) 0x0)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ASM_MPCOPY
# define mpcopy(size, dst, src) memcpy(dst, src, MP_WORDS_TO_BYTES(size))
#else
BEECRYPTAPI
void mpcopy(size_t size, mpw* dest, const mpw* src);
#endif

#ifndef ASM_MPMOVE
# define mpmove(size, dst, src) memmove(dst, src, MP_WORDS_TO_BYTES(size))
#else
BEECRYPTAPI
void mpmove(size_t size, mpw* dest, const mpw* src);
#endif

/*!\fn void mpzero(size_t size, mpw* data)
 * \brief This function zeroes a multi-precision integer of a given size.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
void mpzero(size_t size, mpw* data);

/*!\fn void mpfill(size_t size, mpw* data, mpw fill)
 * \brief This function fills each word of a multi-precision integer with a
 *  given value.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \param fill The value fill the data with.
 */
BEECRYPTAPI
void mpfill(size_t size, mpw* data, mpw fill);

/*!\fn int mpodd(size_t size, const mpw* data)
 * \brief This functions tests if a multi-precision integer is odd.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if odd
 * \retval 0 if even
 */
BEECRYPTAPI
int mpodd (size_t size, const mpw* data);

/*!\fn int mpeven(size_t size, const mpw* data)
 * \brief This function tests if a multi-precision integer is even.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if even
 * \retval 0 if odd
 */
BEECRYPTAPI
int mpeven(size_t size, const mpw* data);

/*!\fn int mpz(size_t size, const mpw* data)
 * \brief This function tests if a multi-precision integer is zero.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if zero
 * \retval 0 if not zero
 */
BEECRYPTAPI
int mpz  (size_t size, const mpw* data);

/*!\fn int mpnz(size_t size, const mpw* data)
 * \brief This function tests if a multi-precision integer is not zero.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if not zero
 * \retval 0 if zero
 */
BEECRYPTAPI
int mpnz (size_t size, const mpw* data);

/*!\fn int mpeq(size_t size, const mpw* xdata, const mpw* ydata)
 * \brief This function tests if two multi-precision integers of the same size
 *  are equal.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if equal
 * \retval 0 if not equal
 */
BEECRYPTAPI
int mpeq (size_t size, const mpw* xdata, const mpw* ydata);

/*!\fn int mpne(size_t size, const mpw* xdata, const mpw* ydata)
 * \brief This function tests if two multi-precision integers of the same size
 *  differ.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if not equal
 * \retval 0 if equal
 */
BEECRYPTAPI
int mpne (size_t size, const mpw* xdata, const mpw* ydata);

/*!\fn int mpgt(size_t size, const mpw* xdata, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of the same size is greater than the second.
 * \note The comparison treats the arguments as unsigned.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if greater
 * \retval 0 if less or equal
 */
BEECRYPTAPI
int mpgt (size_t size, const mpw* xdata, const mpw* ydata);

/*!\fn int mplt(size_t size, const mpw* xdata, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of the same size is less than the second.
 * \note The comparison treats the arguments as unsigned.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if less
 * \retval 0 if greater or equal
 */
BEECRYPTAPI
int mplt (size_t size, const mpw* xdata, const mpw* ydata);

/*!\fn int mpge(size_t size, const mpw* xdata, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of the same size is greater than or equal to the second.
 * \note The comparison treats the arguments as unsigned.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if greater or equal
 * \retval 0 if less
 */
BEECRYPTAPI
int mpge (size_t size, const mpw* xdata, const mpw* ydata);

/*!\fn int mple(size_t size, const mpw* xdata, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of the same size is less than or equal to the second.
 * \note The comparison treats the arguments as unsigned.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if less or equal
 * \retval 0 if greater
 */
BEECRYPTAPI
int mple (size_t size, const mpw* xdata, const mpw* ydata);

/*!\fn int mpeqx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function tests if two multi-precision integers of different
 *  size are equal.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if equal
 * \retval 0 if not equal
 */
BEECRYPTAPI
int mpeqx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mpnex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function tests if two multi-precision integers of different
 *  size are equal.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if equal
 * \retval 0 if not equal
*/
BEECRYPTAPI
int mpnex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mpgtx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of different size is greater than the second.
 * \note The comparison treats the arguments as unsigned.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the second multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if greater
 * \retval 0 if less or equal
 */
BEECRYPTAPI
int mpgtx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mpltx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of different size is less than the second.
 * \note The comparison treats the arguments as unsigned.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the second multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if less
 * \retval 0 if greater or equal
 */
BEECRYPTAPI
int mpltx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mpgex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of different size is greater than or equal to the second.
 * \note The comparison treats the arguments as unsigned.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the second multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if greater or equal
 * \retval 0 if less
 */
BEECRYPTAPI
int mpgex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mplex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function tests if the first of two multi-precision integers
 *  of different size is less than or equal to the second.
 * \note The comparison treats the arguments as unsigned.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the second multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if less or equal
 * \retval 0 if greater
 */
BEECRYPTAPI
int mplex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mpisone(size_t size, const mpw* data)
 * \brief This functions tests if the value of a multi-precision integer is
 *  equal to one.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if one
 * \retval 0 if not one
 */
BEECRYPTAPI
int mpisone(size_t size, const mpw* data);

/*!\fn int mpistwo(size_t size, const mpw* data)
 * \brief This function tests if the value of a multi-precision integer is
 *  equal to two.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if two
 * \retval 0 if not two
 */
BEECRYPTAPI
int mpistwo(size_t size, const mpw* data);

/*!\fn int mpleone(size_t size, const mpw* data);
 * \brief This function tests if the value of a multi-precision integer is
 *  less than or equal to one.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if less than or equal to one.
 * \retval 0 if greater than one.
 */
BEECRYPTAPI
int mpleone(size_t size, const mpw* data);

/*!\fn int mpeqmone(size_t size, const mpw* xdata, const mpw* ydata);
 * \brief This function tests if multi-precision integer x is equal to y
 *  minus one.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \retval 1 if less than or equal to one.
 * \retval 0 if greater than one.
 */
BEECRYPTAPI
int mpeqmone(size_t size, const mpw* xdata, const mpw* ydata);

/*!\fn int mpmsbset(size_t size, const mpw* data)
 * \brief This function tests if the most significant bit of a multi-precision
 *  integer is set.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if set
 * \retval 0 if not set
 */
BEECRYPTAPI
int mpmsbset(size_t size, const mpw* data);

/*!\fn int mplsbset(size_t size, const mpw* data)
 * \brief This function tests if the leiast significant bit of a multi-precision
 *  integer is set.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 * \retval 1 if set
 * \retval 0 if not set
 */
BEECRYPTAPI
int mplsbset(size_t size, const mpw* data);

/*!\fn void mpsetmsb(size_t size, mpw* data)
 * \brief This function sets the most significant bit of a multi-precision
 *  integer.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
void mpsetmsb(size_t size, mpw* data);

/*!\fn void mpsetlsb(size_t size, mpw* data)
 * \brief This function sets the least significant bit of a multi-precision
 *  integer.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
void mpsetlsb(size_t size, mpw* data);

/*!\fn void mpclrmsb(size_t size, mpw* data)
 * \brief This function clears the most significant bit of a multi-precision
 *  integer.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
void mpclrmsb(size_t size, mpw* data);

/*!\fn void mpclrlsb(size_t size, mpw* data)
 * \brief This function clears the least significant bit of a multi-precision
 *  integer.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
void mpclrlsb(size_t size, mpw* data);

/*!\fn mpand(size_t size, mpw* xdata, const mpw* ydata)
 * \brief This function computes the bit-wise AND of two multi-precision
 *  integers. Modifies xdata.
 * \param size The size of the multi-precision integers.
 * \param xdata The multi-precision integer data.
 * \param ydata The multi-precision integer data.
 */
BEECRYPTAPI
void mpand(size_t size, mpw* xdata, const mpw* ydata);

/*!\fn void mpor(size_t size, mpw* xdata, const mpw* ydata) 
 * \brief This function computes the bit-wise OR of two multi-precision
 *  integers. Modifies xdata.
 * \param size The size of the multi-precision integer.
 * \param xdata The multi-precision integer data.
 * \param ydata The multi-precision integer data.
 */
BEECRYPTAPI
void mpor(size_t size, mpw* xdata, const mpw* ydata);

/*!\fn void mpxor(size_t size, mpw* xdata, const mpw* ydata) 
 * \brief This function computes the bit-wise XOR of two multi-precision
 *  integers. Modifies xdata.
 * \param size The size of the multi-precision integer.
 * \param xdata The multi-precision integer data.
 * \param ydata The multi-precision integer data.
 */
BEECRYPTAPI
void mpxor(size_t size, mpw* xdata, const mpw* ydata);

/*!\fn mpnot(size_t size, mpw* data)
 * \brief This function flips all bits of a multi-precision integer.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
void mpnot(size_t size, mpw* data);

/*!\fn void mpsetw(size_t size, mpw* xdata, mpw y)
 * \brief This function sets the value of a multi-precision integer to the
 *  given word. The given value is copied into the least significant word,
 *  while the most significant words are zeroed.
 * \param size The size of the multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param y The multi-precision word.
 */
BEECRYPTAPI
void mpsetw(size_t size, mpw* xdata, mpw y);

/*!\fn void mpsetx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function set the value of the first multi-precision integer
 *  to the second, truncating the most significant words if ysize > xsize, or
 *  zeroing the most significant words if ysize < xsize.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the second multi-precision integer.
 * \param ydata The second multi-precision integer.
 */
void mpsetx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mpaddw(size_t size, mpw* xdata, mpw y)
 * \brief This function adds one word to a multi-precision integer.
 *  The performed operation is in pseudocode: x += y.
 * \param size The size of the multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param y The multi-precision word.
 * \return The carry-over value of the operation; this value is either 0 or 1.
 */
BEECRYPTAPI
int mpaddw(size_t size, mpw* xdata, mpw y);

/*!\fn int mpadd(size_t size, mpw* xdata, const mpw* ydata)
 * \brief This function adds two multi-precision integers of equal size.
 *  The performed operation is in pseudocode: x += y.
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \return The carry-over value of the operation; this value is either 0 or 1.
 */
BEECRYPTAPI
int mpadd (size_t size, mpw* xdata, const mpw* ydata);

/*!\fn int mpaddx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function adds two multi-precision integers of different size.
 *  The performed operation in pseudocode: x += y.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the second multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \return The carry-over value of the operation; this value is either 0 or 1.
 */
BEECRYPTAPI
int mpaddx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn int mpsubw(size_t size, mpw* xdata, mpw y)
 * \brief This function subtracts one word to a multi-precision integer.
 *  The performed operation in pseudocode: x -= y
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param y The multi-precision word.
 * \return The carry-over value of the operation; this value is either 0 or 1.
 */
BEECRYPTAPI
int mpsubw(size_t size, mpw* xdata, mpw y);

/*!\fn int mpsub(size_t size, mpw* xdata, const mpw* ydata)
 * \brief This function subtracts two multi-precision integers of equal size.
 *  The performed operation in pseudocode: x -= y
 * \param size The size of the multi-precision integers.
 * \param xdata The first multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \return The carry-over value of the operation; this value is either 0 or 1.
 */
BEECRYPTAPI
int mpsub (size_t size, mpw* xdata, const mpw* ydata);

/*!\fn int mpsubx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function subtracts two multi-precision integers of different
 *  size. The performed operation in pseudocode: x -= y.
 * \param xsize The size of the first multi-precision integer.
 * \param xdata The first multi-precision integer.
 * \param ysize The size of the second multi-precision integer.
 * \param ydata The second multi-precision integer.
 * \return The carry-over value of the operation; this value is either 0 or 1.
 */
BEECRYPTAPI
int mpsubx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata);

BEECRYPTAPI
int mpmultwo(size_t size, mpw* data);

/*!\fn void mpneg(size_t size, mpw* data)
 * \brief This function negates a multi-precision integer.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
void mpneg(size_t size, mpw* data);

/*!\fn size_t mpsize(size_t size, const mpw* data)
 * \brief This function returns the true size of a multi-precision
 *  integer, after stripping leading zero words.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
size_t mpsize(size_t size, const mpw* data);

/*!\fn size_t mpbits(size_t size, const mpw* data)
 * \brief This function returns the number of significant bits
 *  in a multi-precision integer.
 * \param size The size of the multi-precision integer.
 * \param data The multi-precision integer data.
 */
BEECRYPTAPI
size_t mpbits(size_t size, const mpw* data);

BEECRYPTAPI
size_t mpmszcnt(size_t size, const mpw* data);

BEECRYPTAPI
size_t mplszcnt(size_t size, const mpw* data);

BEECRYPTAPI
void mplshift(size_t size, mpw* data, size_t count);

BEECRYPTAPI
void mprshift(size_t size, mpw* data, size_t count);

BEECRYPTAPI
size_t mprshiftlsz(size_t size, mpw* data);

BEECRYPTAPI
size_t mpnorm(size_t size, mpw* data);

BEECRYPTAPI
void mpdivtwo (size_t size, mpw* data);

BEECRYPTAPI
void mpsdivtwo(size_t size, mpw* data);

/*!\fn mpw mpsetmul(size_t size, mpw* result, const mpw* data, mpw y)
 * \brief This function performs a multi-precision multiply-setup.
 *
 * This function is used in the computation of a full multi-precision
 * multiplication. By using it we can shave off a few cycles; otherwise we'd
 * have to zero the least significant half of the result first and use
 * another call to the slightly slower mpaddmul function.
 *
 * \param size The size of multi-precision integer multiplier.
 * \param result The place where result will be accumulated.
 * \param data The multi-precision integer multiplier.
 * \param y The multiplicand.
 * \return The carry-over multi-precision word.
 */
BEECRYPTAPI
mpw mpsetmul   (size_t size, mpw* result, const mpw* data, mpw y);

/*!\fn mpw mpaddmul(size_t size, mpw* result, const mpw* data, mpw y)
 * \brief This function performs a mult-precision multiply-accumulate.
 *
 * This function is used in the computation of a full multi-precision
 * multiplication. It computes the product-by-one-word and accumulates it with
 * the previous result.
 *
 * \param size The size of multi-precision integer multiplier.
 * \param result The place where result will be accumulated.
 * \param data The multi-precision integer multiplier.
 * \param y The multiplicand.
 * \retval The carry-over multi-precision word.
 */
BEECRYPTAPI
mpw mpaddmul   (size_t size, mpw* result, const mpw* data, mpw y);

/*!\fn void mpaddsqrtrc(size_t size, mpw* result, const mpw* data)
 * \brief This function is used in the calculation of a multi-precision
 * squaring.
 */
BEECRYPTAPI
void mpaddsqrtrc(size_t size, mpw* result, const mpw* data);

/*!\fn void mpmul(mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
 * \brief This function computes a full multi-precision product.
 */
BEECRYPTAPI
void mpmul(mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata);

/*!\fn void mpsqr(mpw* result, size_t size, const mpw* data)
 * \brief This function computes a full multi-precision square.
 */
BEECRYPTAPI
void mpsqr(mpw* result, size_t size, const mpw* data);

BEECRYPTAPI
void mpgcd_w(size_t size, const mpw* xdata, const mpw* ydata, mpw* result, mpw* wksp);

BEECRYPTAPI
int  mpextgcd_w(size_t size, const mpw* xdata, const mpw* ydata, mpw* result, mpw* wksp);

BEECRYPTAPI
mpw mppndiv(mpw xhi, mpw xlo, mpw y);

BEECRYPTAPI
void mpmod (mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw*ydata, mpw* wksp);

BEECRYPTAPI
void mpndivmod(mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata, mpw* wksp);

/*
 * Output Routines
 */

BEECRYPTAPI
void mpprint(size_t size, const mpw* data);

BEECRYPTAPI
void mpprintln(size_t size, const mpw* data);

BEECRYPTAPI
void mpfprint(FILE* f, size_t size, const mpw* data);

BEECRYPTAPI
void mpfprintln(FILE* f, size_t size, const mpw* data);

/*
 * Conversion Routines
 */

BEECRYPTAPI
int os2ip(mpw* idata, size_t isize, const byte* osdata, size_t ossize);

BEECRYPTAPI
int i2osp(byte* osdata, size_t ossize, const mpw* idata, size_t isize);

BEECRYPTAPI
int hs2ip(mpw* idata, size_t isize, const char* hsdata, size_t hssize);

#ifdef __cplusplus
}
#endif

#endif
