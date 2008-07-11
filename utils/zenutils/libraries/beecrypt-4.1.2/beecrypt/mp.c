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

/*!\file mp.c
 * \brief Multi-precision integer routines.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup MP_m
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/mp.h"
#include "beecrypt/mpopt.h"

#ifndef ASM_MPZERO
void mpzero(size_t size, mpw* data)
{
	while (size--)
		*(data++) = 0;
}
#endif

#ifndef ASM_MPFILL
void mpfill(size_t size, mpw* data, mpw fill)
{
	while (size--)
		*(data++) = fill;
}
#endif

#ifndef ASM_MPODD
int mpodd(size_t size, const mpw* data)
{
	return (int)(data[size-1] & 0x1);
}
#endif

#ifndef ASM_MPEVEN
int mpeven(size_t size, const mpw* data)
{
	return !(int)(data[size-1] & 0x1);
}
#endif

#ifndef ASM_MPZ
int mpz(size_t size, const mpw* data)
{
	while (size--)
		if (*(data++))
			return 0;
	return 1;
}
#endif

#ifndef ASM_MPNZ
int mpnz(size_t size, const mpw* data)
{
	while (size--)
		if (*(data++))
			return 1;
	return 0;
}
#endif

#ifndef ASM_MPEQ
int mpeq(size_t size, const mpw* xdata, const mpw* ydata)
{
	while (size--)
	{
		if (*xdata == *ydata)
		{
			xdata++;
			ydata++;
		}
		else
			return 0;
	}
	return 1;
}
#endif

#ifndef ASM_MPEQX
int mpeqx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpeq(ysize, xdata+diff, ydata) && mpz(diff, xdata);
	}
	else if (xsize < ysize)
	{
		register size_t diff = ysize - xsize;
		return mpeq(xsize, ydata+diff, xdata) && mpz(diff, ydata);
	}
	else
		return mpeq(xsize, xdata, ydata);
}
#endif

#ifndef ASM_MPNE
int mpne(size_t size, const mpw* xdata, const mpw* ydata)
{
	while (size--)
	{
		if (*xdata == *ydata)
		{
			xdata++;
			ydata++;
		}
		else
			return 1;
	}
	return 0;
}
#endif

#ifndef ASM_MPNEX
int mpnex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpnz(diff, xdata) || mpne(ysize, xdata+diff, ydata);
	}
	else if (xsize < ysize)
	{
		register size_t diff = ysize - xsize;
		return mpnz(diff, ydata) || mpne(xsize, ydata+diff, xdata);
	}
	else
		return mpne(xsize, xdata, ydata);
}
#endif

#ifndef ASM_MPGT
int mpgt(size_t size, const mpw* xdata, const mpw* ydata)
{
	while (size--)
	{
		if (*xdata < *ydata)
			return 0;
		if (*xdata > *ydata)
			return 1;
		xdata++; ydata++;
	}
	return 0;
}
#endif

#ifndef ASM_MPGTX
int mpgtx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpnz(diff, xdata) || mpgt(ysize, xdata + diff, ydata);
	}
	else if (xsize < ysize)
	{
		register size_t diff = ysize - xsize;
		return mpz(diff, ydata) && mpgt(xsize, xdata, ydata + diff);
	}
	else
		return mpgt(xsize, xdata, ydata);
}
#endif

#ifndef ASM_MPLT
int mplt(size_t size, const mpw* xdata, const mpw* ydata)
{
	while (size--)
	{
		if (*xdata > *ydata)
			return 0;
		if (*xdata < *ydata)
			return 1;
		xdata++; ydata++;
	}
	return 0;
}
#endif

#ifndef ASM_MPLTX
int mpltx(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpz(diff, xdata) && mplt(ysize, xdata+diff, ydata);
	}
	else if (xsize < ysize)
	{
		register size_t diff = ysize - xsize;
		return mpnz(diff, ydata) || mplt(xsize, xdata, ydata+diff);
	}
	else
		return mplt(xsize, xdata, ydata);
}
#endif

#ifndef ASM_MPGE
int mpge(size_t size, const mpw* xdata, const mpw* ydata)
{
	while (size--)
	{
		if (*xdata < *ydata)
			return 0;
		if (*xdata > *ydata)
			return 1;
		xdata++; ydata++;
	}
	return 1;
}
#endif

#ifndef ASM_MPGEX
int mpgex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpnz(diff, xdata) || mpge(ysize, xdata+diff, ydata);
	}
	else if (xsize < ysize)
	{
		register size_t diff = ysize - xsize;
		return mpz(diff, ydata) && mpge(xsize, xdata, ydata+diff);
	}
	else
		return mpge(xsize, xdata, ydata);
}
#endif

#ifndef ASM_MPLE
int mple(size_t size, const mpw* xdata, const mpw* ydata)
{
	while (size--)
	{
		if (*xdata < *ydata)
			return 1;
		if (*xdata > *ydata)
			return 0;
		xdata++; ydata++;
	}
	return 1;
}
#endif

#ifndef ASM_MPLEX
int mplex(size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpz(diff, xdata) && mple(ysize, xdata+ diff, ydata);
	}
	else if (xsize < ysize)
	{
		register size_t diff = ysize - xsize;
		return mpnz(diff, ydata) || mple(xsize, xdata, ydata+diff);
	}
	else
		return mple(xsize, xdata, ydata);
}
#endif

#ifndef ASM_MPISONE
int mpisone(size_t size, const mpw* data)
{
	data += size;
	if (*(--data) == 1)
	{
		while (--size)
			if (*(--data))
				return 0;
		return 1;
	}
	return 0;
}
#endif

#ifndef ASM_MPISTWO
int mpistwo(size_t size, const mpw* data)
{
	data += size;
	if (*(--data) == 2)
	{
		while (--size)
			if (*(--data))
				return 0;
		return 1;
	}
	return 0;
}
#endif

#ifndef ASM_MPEQMONE
int mpeqmone(size_t size, const mpw* xdata, const mpw* ydata)
{
    xdata += size;
    ydata += size;

    if (*(--xdata)+1 == *(--ydata))
    {
        while (--size)
            if (*(--xdata) != *(--ydata))
                return 0;
        return 1;
    }
    return 0;
}
#endif

#ifndef ASM_MPLEONE
int mpleone(size_t size, const mpw* data)
{
	data += size;
	if (*(--data) > 1)
		return 0;
	else
	{
		while (--size)
			if (*(--data))
				return 0;
		return 1;
	}
}
#endif

#ifndef ASM_MPMSBSET
int mpmsbset(size_t size, const mpw* data)
{
	return (int)((*data) >> (MP_WBITS-1));
}
#endif

#ifndef ASM_MPLSBSET
int mplsbset(size_t size, const mpw* data)
{
    return (int)(data[size-1] & 0x1);
}
#endif

#ifndef ASM_MPSETMSB
void mpsetmsb(size_t size, mpw* data)
{
	*data |= MP_MSBMASK;
}
#endif

#ifndef ASM_MPSETLSB
void mpsetlsb(size_t size, mpw* data)
{
	data[size-1] |= MP_LSBMASK;
}
#endif

#ifndef ASM_MPCLRMSB
void mpclrmsb(size_t size, mpw* data)
{
	*data &= ~ MP_MSBMASK;
}
#endif

#ifndef ASM_MPCLRLSB
void mpclrlsb(size_t size, mpw* data)
{
    data[size-1] &= ~ MP_LSBMASK;
}
#endif

#ifndef ASM_MPAND
void mpand(size_t size, mpw* xdata, const mpw* ydata)
{
	while (size--)
		xdata[size] &= ydata[size];
}
#endif

#ifndef ASM_MPOR
void mpor(size_t size, mpw* xdata, const mpw* ydata)
{
	while (size--)
		xdata[size] |= ydata[size];
}
#endif

#ifndef ASM_MPXOR
void mpxor(size_t size, mpw* xdata, const mpw* ydata)
{
	while (size--)
		xdata[size] ^= ydata[size];
}
#endif

#ifndef ASM_MPNOT
void mpnot(size_t size, mpw* data)
{
	while (size--)
		data[size] = ~data[size];
}
#endif

#ifndef ASM_MPSETW
void mpsetw(size_t size, mpw* xdata, mpw y)
{
	while (--size)
		*(xdata++) = 0;
	*(xdata++) = y;
}
#endif

#ifndef ASM_MPSETX
void mpsetx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata)
{
	while (xsize > ysize)
	{
		xsize--;
		*(xdata++) = 0;
	}
	while (ysize > xsize)
	{
		ysize--;
		ydata++;
	}
	while (xsize--)
		*(xdata++) = *(ydata++);
}
#endif

#ifndef ASM_MPADDW
int mpaddw(size_t size, mpw* xdata, mpw y)
{
	register mpw load, temp;
	register int carry = 0;

	xdata += size-1;

	load = *xdata;
	temp = load + y;
	*(xdata--) = temp;
	carry = (load > temp);

	while (--size && carry)
	{
		load = *xdata;
		temp = load + 1;
		*(xdata--) = temp;
		carry = (load > temp);	
	}
	return carry;
}
#endif

#ifndef ASM_MPADD
int mpadd(size_t size, mpw* xdata, const mpw* ydata)
{
	register mpw load, temp;
	register int carry = 0;

	xdata += size-1;
	ydata += size-1;

	while (size--)
	{
		temp = *(ydata--);
		load = *xdata;
		temp = carry ? (load + temp + 1) : (load + temp);
		*(xdata--) = temp;
		carry = carry ? (load >= temp) : (load > temp);
	}
	return carry;
}
#endif

#ifndef ASM_MPADDX
int mpaddx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpaddw(diff, xdata, (mpw) mpadd(ysize, xdata+diff, ydata));
	}
	else
	{
		register size_t diff = ysize - xsize;
		return mpadd(xsize, xdata, ydata+diff);
	}
}
#endif

#ifndef ASM_MPSUBW
int mpsubw(size_t size, mpw* xdata, mpw y)
{
	register mpw load, temp;
	register int carry = 0;

	xdata += size-1;

	load = *xdata;
	temp = load - y;
	*(xdata--) = temp;
	carry = (load < temp);

	while (--size && carry)
	{
		load = *xdata;
		temp = load - 1;
		*(xdata--) = temp;
		carry = (load < temp);	
	}
	return carry;
}
#endif

#ifndef ASM_MPSUB
int mpsub(size_t size, mpw* xdata, const mpw* ydata)
{
	register mpw load, temp;
	register int carry = 0;

	xdata += size-1;
	ydata += size-1;

	while (size--)
	{
		temp = *(ydata--);
		load = *xdata;
		temp = carry ? (load - temp - 1) : (load - temp);
		*(xdata--) = temp;
		carry = carry ? (load <= temp) : (load < temp);
	}
	return carry;
}
#endif

#ifndef ASM_MPSUBX
int mpsubx(size_t xsize, mpw* xdata, size_t ysize, const mpw* ydata)
{
	if (xsize > ysize)
	{
		register size_t diff = xsize - ysize;
		return mpsubw(diff, xdata, (mpw) mpsub(ysize, xdata+diff, ydata));
	}
	else
	{
		register size_t diff = ysize - xsize;
		return mpsub(xsize, xdata, ydata+diff);
	}
}
#endif

#ifndef ASM_MPNEG
void mpneg(size_t size, mpw* data)
{
	mpnot(size, data);
	mpaddw(size, data, 1);
}
#endif

#ifndef ASM_MPSETMUL
mpw mpsetmul(size_t size, mpw* result, const mpw* data, mpw y)
{
	#if HAVE_MPDW
	register mpdw temp;
	register mpw  carry = 0;

	data += size;
	result += size;

	while (size--)
	{
		temp = *(--data);
		temp *= y;
		temp += carry;
		*(--result) = (mpw) temp;
		carry = (mpw)(temp >> MP_WBITS);
	}
	#else
	register mpw temp, load, carry = 0;
	register mphw ylo, yhi;

	ylo = (mphw)  y;
	yhi = (mphw) (y >> MP_HWBITS);

	data += size;
	result += size;

	while (size--)
	{
		register mphw xlo, xhi;
		register mpw rlo, rhi;

		xlo = (mphw) (temp = *(--data));
		xhi = (mphw) (temp >> MP_HWBITS);

		rlo = (mpw) xlo * ylo;
		rhi = (mpw) xhi * yhi;
		load = rlo;
		temp = (mpw) xhi * ylo;
		rlo += (temp << MP_HWBITS);
		rhi += (temp >> MP_HWBITS) + (load > rlo);
		load = rlo;
		temp = (mpw) xlo * yhi;
		rlo += (temp << MP_HWBITS);
		rhi += (temp >> MP_HWBITS) + (load > rlo);
		load = rlo;
		temp = rlo + carry;
		carry = rhi + (load > temp);
		*(--result) = temp;
	}
	#endif
	return carry;
}
#endif

#ifndef ASM_MPADDMUL
mpw mpaddmul(size_t size, mpw* result, const mpw* data, mpw y)
{
	#if HAVE_MPDW
	register mpdw temp;
	register mpw  carry = 0;

	data += size;
	result += size;

	while (size--)
	{
		temp = *(--data);
		temp *= y;
		temp += carry;
		temp += *(--result);
		*result = (mpw) temp;
		carry = (mpw)(temp >> MP_WBITS);
	}
	#else
	register mpw temp, load, carry = 0;
	register mphw ylo, yhi;

	ylo = (mphw)  y;
	yhi = (mphw) (y >> MP_HWBITS);

	data += size;
	result += size;

	while (size--)
	{
		register mphw xlo, xhi;
		register mpw rlo, rhi;

		xlo = (mphw) (temp = *(--data));
		xhi = (mphw) (temp >> MP_HWBITS);

		rlo = (mpw) xlo * ylo;
		rhi = (mpw) xhi * yhi;
		load = rlo;
		temp = (mpw) xhi * ylo;
		rlo += (temp << MP_HWBITS);
		rhi += (temp >> MP_HWBITS) + (load > rlo);
		load = rlo;
		temp = (mpw) xlo * yhi;
		rlo += (temp << MP_HWBITS);
		rhi += (temp >> MP_HWBITS) + (load > rlo);
		load = rlo;
		rlo += carry;
		temp = (load > rlo);
		load = rhi;
		rhi += temp;
		carry = (load > rhi);
		load = rlo;
		rlo += *(--result);
		*result = rlo;
		carry += rhi + (load > rlo);
	}
	#endif
	return carry;
}
#endif

#ifndef ASM_MPMUL
void mpmul(mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata)
{
	/* preferred passing of parameters is x the larger of the two numbers */
	if (xsize >= ysize)
	{
		register mpw rc;

		result += ysize;
		ydata += ysize;

		rc = mpsetmul(xsize, result, xdata, *(--ydata));
		*(--result) = rc;

		while (--ysize)
		{
			rc = mpaddmul(xsize, result, xdata, *(--ydata));
			*(--result) = rc;
		}
	}
	else
	{
		register mpw rc;

		result += xsize;
		xdata += xsize;

		rc = mpsetmul(ysize, result, ydata, *(--xdata));
		*(--result) = rc;

		while (--xsize)
		{
			rc = mpaddmul(ysize, result, ydata, *(--xdata));
			*(--result) = rc;
		}
	}
}
#endif

#ifndef ASM_MPADDSQRTRC
void mpaddsqrtrc(size_t size, mpw* result, const mpw* data)
{
	#if HAVE_MPDW
	register mpdw temp;
	register mpw  load, carry = 0;

	result += (size << 1);

	while (size--)
	{
		temp = load = data[size];
		temp *= load;
		temp += carry;
		temp += *(--result);
		*result = (mpw) temp;
		temp >>= MP_WBITS;
		temp += *(--result);
		*result = (mpw) temp;
		carry = (mpw)(temp >> MP_WBITS);
	}
	#else
	register mpw temp, load, carry = 0;

	result += (size << 1);

	while (size--)
	{
		register mphw xlo, xhi;
		register mpw rlo, rhi;

		xlo = (mphw) (temp = data[size]);
		xhi = (mphw) (temp >> MP_HWBITS);

		rlo = (mpw) xlo * xlo;
		rhi = (mpw) xhi * xhi;
		temp = (mpw) xhi * xlo;
		load = rlo;
		rlo += (temp << MP_HWBITS);
		rhi += (temp >> MP_HWBITS) + (load > rlo);
		load = rlo;
		rlo += (temp << MP_HWBITS);
		rhi += (temp >> MP_HWBITS) + (load > rlo);
		load = rlo;
		rlo += carry;
		rhi += (load > rlo);
		load = rlo;
		rlo += *(--result);
		*result = rlo;
		temp = (load > rlo);
		load = rhi;
		rhi += temp;
		carry = (load > rhi);
		load = rhi;
		rhi += *(--result);
		*result = rhi;
		carry += (load > rhi);
	}
	#endif
}
#endif

#ifndef ASM_MPSQR
void mpsqr(mpw* result, size_t size, const mpw* data)
{
	register mpw rc;
	register size_t n = size-1;

	result += size;
	result[n] = 0;

	if (n)
	{
		rc = mpsetmul(n, result, data, data[n]);
		*(--result) = rc;
		while (--n)
		{
			rc = mpaddmul(n, result, data, data[n]);
			*(--result) = rc;
		}
	}

	*(--result) = 0;

	mpmultwo(size << 1, result);

	mpaddsqrtrc(size, result, data);
}
#endif

#ifndef ASM_MPSIZE
size_t mpsize(size_t size, const mpw* data)
{
	while (size)
	{
		if (*data)
			return size;
		data++;
		size--;
	}
	return 0;
}
#endif

#ifndef ASM_MPBITS
size_t mpbits(size_t size, const mpw* data)
{
	return MP_WORDS_TO_BITS(size) - mpmszcnt(size, data); 
}
#endif

#ifndef ASM_MPNORM
size_t mpnorm(size_t size, mpw* data)
{
	register size_t shift = mpmszcnt(size, data);
	mplshift(size, data, shift);
	return shift;
}
#endif

#ifndef ASM_MPDIVTWO
void mpdivtwo(size_t size, mpw* data)
{
	register mpw temp, carry = 0;

	while (size--)
	{
		temp = *data;
		*(data++) = (temp >> 1) | carry;
		carry = (temp << (MP_WBITS-1));
	}
}
#endif

#ifndef ASM_MPSDIVTWO
void mpsdivtwo(size_t size, mpw* data)
{
	int carry = mpmsbset(size, data);
	mpdivtwo(size, data);
	if (carry)
		mpsetmsb(size, data);
}
#endif

#ifndef ASM_MPMULTWO
int mpmultwo(size_t size, mpw* data)
{
	register mpw temp, carry = 0;

	data += size;
	while (size--)
	{
		temp = *(--data);
		*data = (temp << 1) | carry;
		carry = (temp >> (MP_WBITS-1));
	}
	return (int) carry;
}
#endif

#ifndef ASM_MPMSZCNT
size_t mpmszcnt(size_t size, const mpw* data)
{
	register size_t zbits = 0;
	register size_t i = 0;

	while (i < size)
	{
		register mpw temp = data[i++];
		if (temp)
		{
			while (!(temp & MP_MSBMASK))
			{
				zbits++;
				temp <<= 1;
			}
			break;
		}
		else
			zbits += MP_WBITS;
	}
	return zbits;
}
#endif

#ifndef ASM_MPLSZCNT
size_t mplszcnt(size_t size, const mpw* data)
{
	register size_t zbits = 0;

	while (size--)
	{
		register mpw temp = data[size];
		if (temp)
		{
			while (!(temp & MP_LSBMASK))
			{
				zbits++;
				temp >>= 1;
			}
			break;
		}
		else
			zbits += MP_WBITS;
	}
	return zbits;
}
#endif

#ifndef ASM_MPLSHIFT
void mplshift(size_t size, mpw* data, size_t count)
{
	register size_t words = MP_BITS_TO_WORDS(count);

	if (words < size)
	{
		register short lbits = (short) (count & (MP_WBITS-1));

		/* first do the shifting, then do the moving */
		if (lbits)
		{
			register mpw temp, carry = 0;
			register short rbits = MP_WBITS - lbits;
			register size_t i = size;

			while (i > words)
			{
				temp = data[--i];
				data[i] = (temp << lbits) | carry;
				carry = (temp >> rbits);
			}
		}
		if (words)
		{
			mpmove(size-words, data, data+words);
			mpzero(words, data+size-words);
		}
	}
	else
		mpzero(size, data);
}
#endif

#ifndef ASM_MPRSHIFT
void mprshift(size_t size, mpw* data, size_t count)
{
	register size_t words = MP_BITS_TO_WORDS(count);

	if (words < size)
	{
		register short rbits = (short) (count & (MP_WBITS-1));

		/* first do the shifting, then do the moving */
		if (rbits)
		{
			register mpw temp, carry = 0;
			register short lbits = MP_WBITS - rbits;
			register size_t i = 0;

			while (i < size-words)
			{
				temp = data[i];
				data[i++] = (temp >> rbits) | carry;
				carry = (temp << lbits);
			}
		}
		if (words)
		{
			mpmove(size-words, data+words, data);
			mpzero(words, data);
		}
	}
	else
		mpzero(size, data);
}
#endif

#ifndef ASM_MPRSHIFTLSZ
size_t mprshiftlsz(size_t size, mpw* data)
{
	register mpw* slide = data+size-1;
	register size_t  zwords = 0; /* counter for 'all zero bit' words */
	register short   lbits, rbits = 0; /* counter for 'least significant zero' bits */
	register mpw  temp, carry = 0;

	data = slide;

	/* count 'all zero' words and move src pointer */
	while (size--)
	{
		/* test if we have a non-zero word */
		if ((carry = *(slide--)))
		{
			/* count 'least signification zero bits and set zbits counter */
			while (!(carry & MP_LSBMASK))
			{
				carry >>= 1;
				rbits++;
			}
			break;
		}
		zwords++;
	}

	if ((rbits == 0) && (zwords == 0))
		return 0;

	/* prepare right-shifting of data */
	lbits = MP_WBITS - rbits;

	/* shift data */
	while (size--)
	{
		temp = *(slide--);
		*(data--) = (temp << lbits) | carry;
		carry = (temp >> rbits);
	}

	/* store the final carry */
	*(data--) = carry;

	/* store the return value in size */
	size = MP_WORDS_TO_BITS(zwords) + rbits;

	/* zero the (zwords) most significant words */
	while (zwords--)
		*(data--) = 0;

	return size;
}
#endif

/* try an alternate version here, with descending sizes */
/* also integrate lszcnt and rshift properly into one function */
#ifndef ASM_MPGCD_W
/*
 * mpgcd_w
 *  need workspace of (size) words
 */
void mpgcd_w(size_t size, const mpw* xdata, const mpw* ydata, mpw* result, mpw* wksp)
{
	register size_t shift, temp;

	if (mpge(size, xdata, ydata))
	{
		mpcopy(size, wksp, xdata);
		mpcopy(size, result, ydata);
	}
	else
	{
		mpcopy(size, wksp, ydata);
		mpcopy(size, result, xdata);
	}
		
	/* get the smallest returned values, and set shift to that */

	shift = mprshiftlsz(size, wksp);
	temp = mprshiftlsz(size, result);

	if (shift > temp)
		shift = temp;

	while (mpnz(size, wksp))
	{
		mprshiftlsz(size, wksp);
		mprshiftlsz(size, result);

		if (mpge(size, wksp, result))
			mpsub(size, wksp, result);
		else
			mpsub(size, result, wksp);

		/* slide past zero words in both operands by increasing pointers and decreasing size */
		if ((*wksp == 0) && (*result == 0))
		{
			size--;
			wksp++;
			result++;
		}
	}

	/* figure out if we need to slide the result pointer back */
	if ((temp = MP_BITS_TO_WORDS(shift)))
	{
		size += temp;
		result -= temp;
	}

	mplshift(size, result, shift);
}
#endif

#ifndef ASM_MPEXTGCD_W
/* needs workspace of (6*size+6) words */
/* used to compute the modular inverse */
int mpextgcd_w(size_t size, const mpw* xdata, const mpw* ydata, mpw* result, mpw* wksp)
{
	/*
	 * For computing a modular inverse, pass the modulus as xdata and the number
	 * to be inverted as ydata.
	 *
	 * Fact: if a element of Zn, then a is invertible if and only if gcd(a,n) = 1
	 * Hence: if n is even, then a must be odd, otherwise the gcd(a,n) >= 2
	 *
	 * The calling routine must guarantee this condition.
	 */

	register size_t sizep = size+1;
	register int full;

	mpw* udata = wksp;
	mpw* vdata = udata+sizep;
	mpw* adata = vdata+sizep;
	mpw* bdata = adata+sizep;
	mpw* cdata = bdata+sizep;
	mpw* ddata = cdata+sizep;

	mpsetx(sizep, udata, size, xdata);
	mpsetx(sizep, vdata, size, ydata);
	mpzero(sizep, bdata);
	mpsetw(sizep, ddata, 1);

	if ((full = mpeven(sizep, udata)))
	{
		mpsetw(sizep, adata, 1);
		mpzero(sizep, cdata);
	}

	while (1)
	{
		while (mpeven(sizep, udata))
		{
			mpdivtwo(sizep, udata);

			if (mpodd(sizep, bdata) || (full && mpodd(sizep, adata)))
			{
				if (full) mpaddx(sizep, adata, size, ydata);
				mpsubx(sizep, bdata, size, xdata);
			}

			if (full) mpsdivtwo(sizep, adata);
			mpsdivtwo(sizep, bdata);
		}
		while (mpeven(sizep, vdata))
		{
			mpdivtwo(sizep, vdata);

			if (mpodd(sizep, ddata) || (full && mpodd(sizep, cdata)))
			{
				if (full) mpaddx(sizep, cdata, size, ydata);
				mpsubx(sizep, ddata, size, xdata);
			}

			if (full) mpsdivtwo(sizep, cdata);
			mpsdivtwo(sizep, ddata);
		}
		if (mpge(sizep, udata, vdata))
		{
			mpsub(sizep, udata, vdata);
			if (full) mpsub(sizep, adata, cdata);
			mpsub(sizep, bdata, ddata);
		}
		else
		{
			mpsub(sizep, vdata, udata);
			if (full) mpsub(sizep, cdata, adata);
			mpsub(sizep, ddata, bdata);
		}
		if (mpz(sizep, udata))
		{
			if (mpisone(sizep, vdata))
			{
				if (result)
				{
					if (*ddata & MP_MSBMASK)
					{
						/* keep adding the modulus until we get a carry */
						while (!mpaddx(sizep, ddata, size, xdata));
					} 
					else
					{
						/* in some computations, d ends up > x, hence:
						 * keep subtracting n from d until d < x
						 */
						while (mpgtx(sizep, ddata, size, xdata))
							mpsubx(sizep, ddata, size, xdata);
					}
					mpsetx(size, result, sizep, ddata);
				}
				return 1; 
			}   
			return 0;
		}
	}
}
#endif

#ifndef ASM_MPPNDIV
mpw mppndiv(mpw xhi, mpw xlo, mpw y)
{
	register mpw result = 0;
	register short count = MP_WBITS;
	register int carry = 0;

	while (count--)
	{
		if (carry | (xhi >= y))
		{
			xhi -= y;
			result++;
		}
		carry = (xhi >> (MP_WBITS-1));
		xhi <<= 1;
		xhi |= (xlo >> (MP_WBITS-1));
		xlo <<= 1;
		result <<= 1;
	}
	if (carry | (xhi >= y))
	{
		xhi -= y;
		result++;
	}
	return result;
}
#endif

#ifndef ASM_MPMOD
void mpmod(mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata, mpw* workspace)
{
	/* result size xsize, workspace size 2*ysize+1 */
	mpw q, msw;
	mpw* rdata = result;
	mpw* ynorm = workspace+ysize+1;
	size_t shift, qsize = xsize-ysize;

	mpcopy(ysize, ynorm, ydata);
	shift = mpnorm(ysize, ynorm);
	msw = *ynorm;
	mpcopy(xsize, rdata, xdata);
	if (mpge(ysize, rdata, ynorm))
		mpsub(ysize, rdata, ynorm);

	while (qsize--)
	{
		q = mppndiv(rdata[0], rdata[1], msw);

		*workspace = mpsetmul(ysize, workspace+1, ynorm, q);

		while (mplt(ysize+1, rdata, workspace))
		{
			mpsubx(ysize+1, workspace, ysize, ynorm);
			q--;
		}
		mpsub(ysize+1, rdata, workspace);
		rdata++;
	}
	/* de-normalization steps */
	while (shift--)
	{
		mpdivtwo(ysize, ynorm);
		if (mpge(ysize, rdata, ynorm))
			mpsub(ysize, rdata, ynorm);
	}
}
#endif

#ifndef ASM_MPNDIVMOD
void mpndivmod(mpw* result, size_t xsize, const mpw* xdata, size_t ysize, const mpw* ydata, register mpw* workspace)
{
	/* result must be xsize+1 in length */
	/* workspace must be ysize+1 in length */
	/* expect ydata to be normalized */
	mpw q;
	mpw msw = *ydata;
	size_t qsize = xsize-ysize;

	*result = (mpge(ysize, xdata, ydata) ? 1 : 0);
	mpcopy(xsize, result+1, xdata);

	if (*result)
		(void) mpsub(ysize, result+1, ydata);

	result++;

	while (qsize--)
	{
		q = mppndiv(result[0], result[1], msw);

		*workspace = mpsetmul(ysize, workspace+1, ydata, q);

		while (mplt(ysize+1, result, workspace))
		{
			mpsubx(ysize+1, workspace, ysize, ydata);
			q--;
		}
		mpsub(ysize+1, result, workspace);
		*(result++) = q;
	}
}
#endif

void mpprint(size_t size, const mpw* data)
{
	mpfprint(stdout, size, data);
}

void mpprintln(size_t size, const mpw* data)
{
	mpfprintln(stdout, size, data);
}

void mpfprint(FILE* f, size_t size, const mpw* data)
{
	if (data == (mpw*) 0)
		return;

	if (f == (FILE*) 0)
		return;

	while (size--)
	{
		#if (MP_WBITS == 32)
		fprintf(f, "%08x", (unsigned) *(data++));
		#elif (MP_WBITS == 64)
		# if WIN32
		fprintf(f, "%016I64x", *(data++));
		# elif SIZEOF_UNSIGNED_LONG == 8
		fprintf(f, "%016lx", *(data++));
		# else
		fprintf(f, "%016llx", *(data++));
		# endif
		#else
		# error
		#endif
	}
	fflush(f);
}

void mpfprintln(FILE* f, size_t size, const mpw* data)
{
	if (data == (mpw*) 0)
		return;

	if (f == (FILE*) 0)
		return;

	while (size--)
	{
		#if (MP_WBITS == 32)
		fprintf(f, "%08x", *(data++));
		#elif (MP_WBITS == 64)
		# if WIN32
		fprintf(f, "%016I64x", *(data++));
		# elif SIZEOF_UNSIGNED_LONG == 8
		fprintf(f, "%016lx", *(data++));
		# else
		fprintf(f, "%016llx", *(data++));
		# endif
		#else
		# error
		#endif
	}
	fprintf(f, "\n");
	fflush(f);
}

int i2osp(byte *osdata, size_t ossize, const mpw* idata, size_t isize)
{
	#if WORDS_BIGENDIAN
	size_t max_bytes = MP_WORDS_TO_BYTES(isize);
	#endif
	size_t significant_bytes = (mpbits(isize, idata) + 7) >> 3;

	/* verify that ossize is large enough to contain the significant bytes */
	if (ossize >= significant_bytes)
	{
		/* looking good; check if we have more space than significant bytes */
		if (ossize > significant_bytes)
		{	/* fill most significant bytes with zero */
			memset(osdata, 0, ossize - significant_bytes);
			osdata += ossize - significant_bytes;
		}
		if (significant_bytes)
		{	/* fill remaining bytes with endian-adjusted data */
			#if !WORDS_BIGENDIAN
			mpw w = idata[--isize];
			byte shift = 0;

			/* fill right-to-left; much easier than left-to-right */
			do	
			{
				osdata[--significant_bytes] = (byte)(w >> shift);
				shift += 8;
				if (shift == MP_WBITS)
				{
					shift = 0;
					w = idata[--isize];
				}
			} while (significant_bytes);
			#else
			/* just copy data past zero bytes */
			memcpy(osdata, ((byte*) idata) + (max_bytes - significant_bytes), significant_bytes);
			#endif
		}
		return 0;
	}
	return -1;
}

int os2ip(mpw* idata, size_t isize, const byte* osdata, size_t ossize)
{
	size_t required;

	/* skip non-significant leading zero bytes */
	while (!(*osdata) && ossize)
	{
		osdata++;
		ossize--;
	}

	required = MP_BYTES_TO_WORDS(ossize + MP_WBYTES - 1);

	if (isize >= required)
	{
		/* yes, we have enough space and can proceed */
		mpw w = 0;
		/* adjust counter so that the loop will start by skipping the proper
		 * amount of leading bytes in the first significant word
		 */
		byte b = (ossize % MP_WBYTES);

		if (isize > required)
		{	/* fill initials words with zero */
			mpzero(isize-required, idata);
			idata += isize-required;
		}

		if (b == 0)
			b = MP_WBYTES;

		while (ossize--)
		{
			w <<= 8;
			w |= *(osdata++);
			b--;

			if (b == 0)
			{
				*(idata++) = w;
				w = 0;
				b = MP_WBYTES;
			}
		}

		return 0;
	}
	return -1;
}

int hs2ip(mpw* idata, size_t isize, const char* hsdata, size_t hssize)
{
	size_t required = MP_NIBBLES_TO_WORDS(hssize + MP_WNIBBLES - 1);

	if (isize >= required)
	{
		register size_t i;


		if (isize > required)
		{	/* fill initial words with zero */
			for (i = required; i < isize; i++)
				*(idata++) = 0;
		}
		while (hssize)
		{
			register mpw w = 0;
			register size_t chunk = hssize & (MP_WNIBBLES - 1);
			register char ch;

			if (chunk == 0) chunk = MP_WNIBBLES;

			for (i = 0; i < chunk; i++)
			{
				ch = *(hsdata++);
				w <<= 4;
				if (ch >= '0' && ch <= '9')
					w += (ch - '0');
				else if (ch >= 'A' && ch <= 'F')
					w += (ch - 'A') + 10;
				else if (ch >= 'a' && ch <= 'f')
					w += (ch - 'a') + 10;
			}
			*(idata++) = w;
			hssize -= chunk;
		}
		return 0;
	}
	return -1;
}
