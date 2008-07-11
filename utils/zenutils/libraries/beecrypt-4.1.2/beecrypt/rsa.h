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

/*!\file rsa.h
 * \brief RSA algorithm.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup IF_m IF_rsa_m
 */

#ifndef _RSA_H
#define _RSA_H

#include "beecrypt/rsakp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!\fn int rsapub(const mpbarrett* n, const mpnumber* e, const mpnumber* m, mpnumber* c)
 * \brief This function performs a raw RSA public key operation.
 *
 * This function can be used for encryption and verifying.
 *
 * It performs the following operation:
 * \li \f$c=m^{e}\ \textrm{mod}\ n\f$
 *
 * \param n The RSA modulus.
 * \param e The RSA public exponent.
 * \param m The message.
 * \param c The ciphertext.
 * \retval 0 on success.
 * \retval -1 on failure.
 */
BEECRYPTAPI
int rsapub(const mpbarrett* n, const mpnumber* e,
           const mpnumber* m, mpnumber* c);

/*!\fn int rsapri(const mpbarrett* n, const mpnumber* d, const mpnumber* c, mpnumber* m)
 * \brief This function performs a raw RSA private key operation.
 *
 * This function can be used for decryption and signing.
 *
 * It performs the operation:
 * \li \f$m=c^{d}\ \textrm{mod}\ n\f$
 *
 * \param n The RSA modulus.
 * \param d The RSA private exponent.
 * \param c The ciphertext.
 * \param m The message.
 * \retval 0 on success.
 * \retval -1 on failure.
 */
BEECRYPTAPI
int rsapri(const mpbarrett* n, const mpnumber* d,
           const mpnumber* c, mpnumber* m);

/*!\fn int rsapricrt(const mpbarrett* n, const mpbarrett* p, const mpbarrett* q, const mpnumber* dp, const mpnumber* dq, const mpnumber* qi, const mpnumber* c, mpnumber* m)
 *
 * \brief This function performs a raw RSA private key operation, with
 *  application of the Chinese Remainder Theorem.
 *
 * It performs the operation:
 * \li \f$j_1=c^{dp}\ \textrm{mod}\ p\f$
 * \li \f$j_2=c^{dq}\ \textrm{mod}\ q\f$
 * \li \f$h=qi \cdot (j_1-j_2)\ \textrm{mod}\ p\f$
 * \li \f$m=j_2+hq\f$
 *
 * \param n The RSA modulus.
 * \param p The first RSA prime factor.
 * \param q The second RSA prime factor.
 * \param dp
 * \param dq
 * \param qi
 * \param c The ciphertext.
 * \param m The message.
 * \retval 0 on success.
 * \retval -1 on failure.
 */
BEECRYPTAPI
int rsapricrt(const mpbarrett* n, const mpbarrett* p, const mpbarrett* q,
              const mpnumber* dp, const mpnumber* dq, const mpnumber* qi,
              const mpnumber* c, mpnumber* m);

/*!\fn int rsavrfy(const mpbarrett* n, const mpnumber* e, const mpnumber* m, const mpnumber* c)
 * \brief This function performs a raw RSA verification.
 *
 * It verifies if ciphertext \a c was encrypted from cleartext \a m
 * with the private key matching the given public key \a (n, e).
 *
 * \param n The RSA modulus.
 * \param e The RSA public exponent.
 * \param m The cleartext message.
 * \param c The ciphertext message.
 * \retval 1 on success.
 * \retval 0 on failure.
 */
BEECRYPTAPI
int rsavrfy(const mpbarrett* n, const mpnumber* e,
            const mpnumber* m, const mpnumber* c);

#ifdef __cplusplus
}
#endif

#endif
