/*
 * Copyright (c) 1999, 2000, 2002 Virtual Unlimited B.V.
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

/*!\file blowfish.h
 * \brief Blowfish block cipher.
 *
 * For more information on this blockcipher, see:
 * "Applied Cryptography", second edition
 *  Bruce Schneier
 *  Wiley & Sons
 *
 * Also see http://www.counterpane.com/blowfish.html
 *
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup BC_m BC_blowfish_m
 */

#ifndef _BLOWFISH_H
#define _BLOWFISH_H

#include "beecrypt/beecrypt.h"
#include "beecrypt/blowfishopt.h"

#define BLOWFISHROUNDS	16
#define BLOWFISHPSIZE	(BLOWFISHROUNDS+2)

/*!\brief Holds all the parameters necessary for the Blowfish cipher.
 * \ingroup BC_blowfish_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI blowfishParam
#else
struct _blowfishParam
#endif
{
	/*!\var p
	 * \brief Holds the key expansion.
	 */
	uint32_t p[BLOWFISHPSIZE];
	/*!\var s
	 * \brief Holds the s-boxes.
	 */
	uint32_t s[1024];
	/*!\var fdback
	 * \brief Buffer to be used by block chaining or feedback modes.
	 */
	uint32_t fdback[2];
};

#ifndef __cplusplus
typedef struct _blowfishParam blowfishParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var blowfish
 * \brief Holds the full API description of the Blowfish algorithm.
 */
extern const BEECRYPTAPI blockCipher blowfish;

/*!\fn int blowfishSetup(blowfishParam* bp, const byte* key, size_t keybits, cipherOperation
 op)
 * \brief The function performs the cipher's key expansion.
 * \param bp The cipher's parameter block.
 * \param key The key value.
 * \param keybits The number of bits in the key; legal values are: 32 to 448,
 *  in multiples of 8.
 * \param op ENCRYPT or DECRYPT.
 * \retval 0 on success.
 * \retval -1 on failure.
 */
BEECRYPTAPI
int		blowfishSetup   (blowfishParam*, const byte*, size_t, cipherOperation);

/*!\fn int blowfishSetIV(blowfishParam* bp, const byte* iv)
 * \brief This function sets the Initialization Vector.
 * \note This function is only useful in block chaining or feedback modes.
 * \param bp The cipher's parameter block.
 * \param iv The initialization vector; may be null.
 * \retval 0 on success.
 */
BEECRYPTAPI
int		blowfishSetIV   (blowfishParam*, const byte*);

/*!\fn blowfishEncrypt(blowfishParam* bp, uint32_t* dst, const uint32_t* src)
 * \brief This function performs the Blowfish encryption; it encrypts one block
 *  of 64 bits.
 * \param bp The cipher's parameter block.
 * \param dst The ciphertext; should be aligned on 32-bit boundary.
 * \param src The cleartext; should be aligned on 32-bit boundary.
 * \retval 0 on success.
 */
BEECRYPTAPI
int		blowfishEncrypt (blowfishParam*, uint32_t*, const uint32_t*);

/*!\fn blowfishDecrypt(blowfishParam* bp, uint32_t* dst, const uint32_t* src)
 * \brief This function performs the Blowfish decryption; it Rderypts one block
 *  of 64 bits.
 * \param bp The cipher's parameter block.
 * \param dst The cleartext; should be aligned on 32-bit boundary.
 * \param src The ciphertext; should be aligned on 32-bit boundary.
 * \retval 0 on success.
 */
BEECRYPTAPI
int		blowfishDecrypt (blowfishParam*, uint32_t*, const uint32_t*);

BEECRYPTAPI
uint32_t*	blowfishFeedback(blowfishParam*);

#ifdef __cplusplus
}
#endif

#endif
