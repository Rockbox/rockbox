/*
 * Copyright (c) 2000, 2002 Virtual Unlimited B.V.
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

/*!\file blockmode.c
 * \brief Blockcipher operation modes.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup BC_m
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/blockmode.h"

int blockEncryptECB(const blockCipher* bc, blockCipherParam* bp, uint32_t* dst, const uint32_t* src, unsigned int nblocks)
{
	register const unsigned int blockwords = bc->blocksize >> 2;

	while (nblocks > 0)
	{
		bc->raw.encrypt(bp, dst, src);

		dst += blockwords;
		src += blockwords;

		nblocks--;
	}

	return 0;
}

int blockDecryptECB(const blockCipher* bc, blockCipherParam* bp, uint32_t* dst, const uint32_t* src, unsigned int nblocks)
{
	register const unsigned int blockwords = bc->blocksize >> 2;

	while (nblocks > 0)
	{
		bc->raw.decrypt(bp, dst, src);

		dst += blockwords;
		src += blockwords;

		nblocks--;
	}

	return 0;
}

int blockEncryptCBC(const blockCipher* bc, blockCipherParam* bp, uint32_t* dst, const uint32_t* src, unsigned int nblocks)
{
	register const unsigned int blockwords = bc->blocksize >> 2;
	register uint32_t* fdback = bc->getfb(bp);

	if (nblocks > 0)
	{
		register unsigned int i;

		for (i = 0; i < blockwords; i++)
			dst[i] = src[i] ^ fdback[i];

		bc->raw.encrypt(bp, dst, dst);

		nblocks--;

		while (nblocks > 0)
		{
			for (i = 0; i < blockwords; i++)
				dst[i+blockwords] = src[i+blockwords] ^ dst[i];

			dst += blockwords;

			bc->raw.encrypt(bp, dst, dst);

			src += blockwords;

			nblocks--;
		}

		for (i = 0; i < blockwords; i++)
			fdback[i] = dst[i];
	}

	return 0;
}

int blockDecryptCBC(const blockCipher* bc, blockCipherParam* bp, uint32_t* dst, const uint32_t* src, unsigned int nblocks)
{
	register const unsigned int blockwords = bc->blocksize >> 2;
	register uint32_t* fdback = bc->getfb(bp);
	register uint32_t* buf = (uint32_t*) malloc(blockwords * sizeof(uint32_t));

	if (buf)
	{
		while (nblocks > 0)
		{
			register uint32_t tmp;
			register unsigned int i;

			bc->raw.decrypt(bp, buf, src);

			for (i = 0; i < blockwords; i++)
			{
				tmp = src[i];
				dst[i] = buf[i] ^ fdback[i];
				fdback[i] = tmp;
			}

			dst += blockwords;
			src += blockwords;

			nblocks--;
		}
		free(buf);
		return 0;
	}

	return -1;
}
