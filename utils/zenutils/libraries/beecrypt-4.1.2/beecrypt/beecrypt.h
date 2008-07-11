/*
 * Copyright (c) 1999, 2000, 2001, 2002 Virtual Unlimited B.V.
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

/*!\file beecrypt.h
 * \brief BeeCrypt API, headers.
 *
 * These API functions provide an abstract way for using most of
 * the various algorithms implemented by the library.
 *
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup ES_m PRNG_m HASH_m HMAC_m BC_m
 */

#ifndef _BEECRYPT_H
#define _BEECRYPT_H

#include "beecrypt/api.h"

#include "beecrypt/memchunk.h"
#include "beecrypt/mpnumber.h"

/*
 * Entropy Sources
 */

/*!\typedef entropyNext
 * \brief Prototype definition for an entropy-generating function.
 * \ingroup ES_m
 */
typedef int (*entropyNext)(byte*, size_t);

/*!\brief This struct holds information and pointers to code specific to each
 *  source of entropy.
 * \ingroup ES_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI entropySource
#else
struct _entropySource
#endif
{
	/*!\var name
	 * \brief The entropy source's name.
	 */
	const char*			name;
	/*!\var next
	 * \brief Points to the function which produces the entropy.
	 */
	const entropyNext	next;
};

#ifndef __cplusplus
typedef struct _entropySource entropySource;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\fn int entropySourceCount()
 * \brief This function returns the number of entropy sources implemented by
 *  the library.
 * \return The number of implemented entropy sources.
 */
BEECRYPTAPI
int						entropySourceCount(void);

/*!\fn const entropySource* entropySourceGet(int n)
 * \brief This function returns the \a n -th entropy source implemented by
 *  the library.
 * \param n Index of the requested entropy source; legal values are 0
 *  through entropySourceCount() - 1.
 * \return A pointer to an entropy source or null, if the index was out of
 *  range.
 */
BEECRYPTAPI
const entropySource*	entropySourceGet(int n);

/*!\fn const entropySource* entropySourceFind(const char* name)
 * \brief This function returns the entropy source specified by the given name.
 * \param name Name of the requested entropy source.
 * \return A pointer to an entropy source or null, if the name wasn't found.
 */
BEECRYPTAPI
const entropySource*	entropySourceFind(const char* name);

/*!\fn const entropySource* entropySourceDefault()
 * \brief This functions returns the default entropy source; the default value
 *  can be specified by setting environment variable BEECRYPT_ENTROPY.
 * \return A pointer to an entropy source or null, in case an error occured.
 */
BEECRYPTAPI
const entropySource*	entropySourceDefault(void);

/*!\fn int entropyGatherNext(byte* data, size_t size)
 * \brief This function gathers \a size bytes of entropy into \a data.
 *
 * Unless environment variable BEECRYPT_ENTROPY is set, this function will
 * try each successive entropy source to gather up the requested amount.
 *
 * \param data Points to where the entropy should be stored.
 * \param size Indicates how many bytes of entropy should be gathered.
 * \retval 0 On success.
 * \retval -1 On failure.
 */
BEECRYPTAPI
int						entropyGatherNext(byte*, size_t);

#ifdef __cplusplus
}
#endif

/*
 * Pseudo-random Number Generators
 */

typedef void randomGeneratorParam;

typedef int (*randomGeneratorSetup  )(randomGeneratorParam*);
typedef int (*randomGeneratorSeed   )(randomGeneratorParam*, const byte*, size_t);
typedef int (*randomGeneratorNext   )(randomGeneratorParam*, byte*, size_t);
typedef int (*randomGeneratorCleanup)(randomGeneratorParam*);

/*
 * The struct 'randomGenerator' holds information and pointers to code specific
 * to each random generator. Each specific random generator MUST be written to
 * be multithread safe.
 *
 * WARNING: each randomGenerator, when used in cryptographic applications, MUST
 * be guaranteed to be of suitable quality and strength (i.e. don't use the
 * random() function found in most UN*X-es).
 *
 * Multiple instances of each randomGenerator can be used (even concurrently),
 * provided they each use their own randomGeneratorParam parameters, a chunk
 * of memory which must be at least as large as indicated by the paramsize
 * field.
 *
 */

/*!\brief This struct holds information and pointers to code specific to each
 *  pseudo-random number generator.
 * \ingroup PRNG_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI randomGenerator
#else
struct _randomGenerator
#endif
{
	/*!\var name
	 * \brief The random generator's name.
	 */
	const char*						name;
	/*!\var paramsize
	 * \brief The size of the random generator's parameters.
	 * \note The implementor should set this by using sizeof(<struct holding
     *  random generator's parameters>).
	 */
	const size_t					paramsize;
	/*!\var setup
	 * \brief Points to the setup function.
	 */
	const randomGeneratorSetup		setup;
	/*!\var seed
	 * \brief Points to the seeding function.
	 */
	const randomGeneratorSeed		seed;
	/*!\var seed
	 * \brief Points to the function which generates the random data.
	 */
	const randomGeneratorNext		next;
	/*!\var seed
	 * \brief Points to the cleanup function.
	 */
	const randomGeneratorCleanup	cleanup;
};

#ifndef __cplusplus
typedef struct _randomGenerator randomGenerator;
#endif

/*
 * You can use the following functions to find random generators implemented by
 * the library:
 *
 * randomGeneratorCount returns the number of generators available.
 *
 * randomGeneratorGet returns the random generator with a given index (starting
 * at zero, up to randomGeneratorCount() - 1), or NULL if the index was out of
 * bounds.
 *
 * randomGeneratorFind returns the random generator with the given name, or
 * NULL if no random generator exists with that name.
 */

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int						randomGeneratorCount(void);
BEECRYPTAPI
const randomGenerator*	randomGeneratorGet(int);
BEECRYPTAPI
const randomGenerator*	randomGeneratorFind(const char*);
BEECRYPTAPI
const randomGenerator*	randomGeneratorDefault(void);

#ifdef __cplusplus
}
#endif

/*
 * The struct 'randomGeneratorContext' is used to contain both the functional
 * part (the randomGenerator), and its parameters.
 */

#ifdef __cplusplus
struct BEECRYPTAPI randomGeneratorContext
#else
struct _randomGeneratorContext
#endif
{
	const randomGenerator* rng;
	randomGeneratorParam* param;

	#ifdef __cplusplus
	randomGeneratorContext();
	randomGeneratorContext(const randomGenerator*);
	~randomGeneratorContext();
	#endif
};

#ifndef __cplusplus
typedef struct _randomGeneratorContext randomGeneratorContext;
#endif

/*
 * The following functions can be used to initialize and free a
 * randomGeneratorContext. Initializing will allocate a buffer of the size
 * required by the randomGenerator, freeing will deallocate that buffer.
 */

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int randomGeneratorContextInit(randomGeneratorContext*, const randomGenerator*);
BEECRYPTAPI
int randomGeneratorContextFree(randomGeneratorContext*);
BEECRYPTAPI
int randomGeneratorContextNext(randomGeneratorContext*, byte*, size_t);
BEECRYPTAPI
int randomGeneratorContextSeed(randomGeneratorContext*, const byte*, size_t);

#ifdef __cplusplus
}
#endif

/*
 * Hash Functions
 */

/*!typedef void hashFunctionParam
 * \ingroup HASH_m
 */
typedef void hashFunctionParam;

typedef int (*hashFunctionReset )(hashFunctionParam*);
typedef int (*hashFunctionUpdate)(hashFunctionParam*, const byte*, size_t);
typedef int (*hashFunctionDigest)(hashFunctionParam*, byte*);

/*
 * The struct 'hashFunction' holds information and pointers to code specific
 * to each hash function. Specific hash functions MAY be written to be
 * multithread-safe.
 *
 * NOTE: data MUST have a size (in bytes) of at least 'digestsize' as described
 * in the hashFunction struct.
 * NOTE: for safety reasons, after calling digest, each specific implementation
 * MUST reset itself so that previous values in the parameters are erased.
 */
#ifdef __cplusplus
struct BEECRYPTAPI hashFunction
#else
struct _hashFunction
#endif
{
	const char*					name;
	const size_t				paramsize;	/* in bytes */
	const size_t				blocksize;	/* in bytes */
	const size_t				digestsize;	/* in bytes */
	const hashFunctionReset		reset;
	const hashFunctionUpdate	update;
	const hashFunctionDigest	digest;
};

#ifndef __cplusplus
typedef struct _hashFunction hashFunction;
#endif

/*
 * You can use the following functions to find hash functions implemented by
 * the library:
 *
 * hashFunctionCount returns the number of hash functions available.
 *
 * hashFunctionGet returns the hash function with a given index (starting
 * at zero, up to hashFunctionCount() - 1), or NULL if the index was out of
 * bounds.
 *
 * hashFunctionFind returns the hash function with the given name, or
 * NULL if no hash function exists with that name.
 */

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int					hashFunctionCount(void);
BEECRYPTAPI
const hashFunction*	hashFunctionGet(int);
BEECRYPTAPI
const hashFunction*	hashFunctionFind(const char*);
BEECRYPTAPI
const hashFunction*	hashFunctionDefault(void);

#ifdef __cplusplus
}
#endif

/*
 * The struct 'hashFunctionContext' is used to contain both the functional
 * part (the hashFunction), and its parameters.
 */
#ifdef __cplusplus
struct BEECRYPTAPI hashFunctionContext
#else
struct _hashFunctionContext
#endif
{
	const hashFunction* algo;
	hashFunctionParam* param;

	#ifdef __cplusplus
	hashFunctionContext();
	hashFunctionContext(const hashFunction*);
	~hashFunctionContext();
	#endif
};

#ifndef __cplusplus
typedef struct _hashFunctionContext hashFunctionContext;
#endif

/*
 * The following functions can be used to initialize and free a
 * hashFunctionContext. Initializing will allocate a buffer of the size
 * required by the hashFunction, freeing will deallocate that buffer.
 */

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int hashFunctionContextInit(hashFunctionContext*, const hashFunction*);
BEECRYPTAPI
int hashFunctionContextFree(hashFunctionContext*);
BEECRYPTAPI
int hashFunctionContextReset(hashFunctionContext*);
BEECRYPTAPI
int hashFunctionContextUpdate(hashFunctionContext*, const byte*, size_t);
BEECRYPTAPI
int hashFunctionContextUpdateMC(hashFunctionContext*, const memchunk*);
BEECRYPTAPI
int hashFunctionContextUpdateMP(hashFunctionContext*, const mpnumber*);
BEECRYPTAPI
int hashFunctionContextDigest(hashFunctionContext*, byte*);
BEECRYPTAPI
int hashFunctionContextDigestMP(hashFunctionContext*, mpnumber*);
BEECRYPTAPI
int hashFunctionContextDigestMatch(hashFunctionContext*, const mpnumber*);

#ifdef __cplusplus
}
#endif

/*
 * Keyed Hash Functions, a.k.a. Message Authentication Codes
 */

/*!\typedef void keyedHashFunctionParam
 * \ingroup HMAC_m
 */
typedef void keyedHashFunctionParam;

typedef int (*keyedHashFunctionSetup  )(keyedHashFunctionParam*, const byte*, size_t);
typedef int (*keyedHashFunctionReset  )(keyedHashFunctionParam*);
typedef int (*keyedHashFunctionUpdate )(keyedHashFunctionParam*, const byte*, size_t);
typedef int (*keyedHashFunctionDigest )(keyedHashFunctionParam*, byte*);

/*
 * The struct 'keyedHashFunction' holds information and pointers to code
 * specific to each keyed hash function. Specific keyed hash functions MAY be
 * written to be multithread-safe.
 *
 * The struct field 'keybitsmin' contains the minimum number of bits a key
 * must contains, 'keybitsmax' the maximum number of bits a key may contain,
 * 'keybitsinc', the increment in bits that may be used between min and max.
 * 
 * NOTE: data must be at least have a bytesize of 'digestsize' as described
 * in the keyedHashFunction struct.
 * NOTE: for safety reasons, after calling digest, each specific implementation
 * MUST reset itself so that previous values in the parameters are erased.
 */
#ifdef __cplusplus
struct BEECRYPTAPI keyedHashFunction
#else
struct _keyedHashFunction
#endif
{
	const char*						name;
	const size_t					paramsize;	/* in bytes */
	const size_t					blocksize;	/* in bytes */
	const size_t					digestsize;	/* in bytes */
	const size_t					keybitsmin;	/* in bits */
	const size_t					keybitsmax;	/* in bits */
	const size_t					keybitsinc;	/* in bits */
	const keyedHashFunctionSetup	setup;
	const keyedHashFunctionReset	reset;
	const keyedHashFunctionUpdate	update;
	const keyedHashFunctionDigest	digest;
};

#ifndef __cplusplus
typedef struct _keyedHashFunction keyedHashFunction;
#endif

/*
 * You can use the following functions to find keyed hash functions implemented
 * by the library:
 *
 * keyedHashFunctionCount returns the number of keyed hash functions available.
 *
 * keyedHashFunctionGet returns the keyed hash function with a given index
 * (starting at zero, up to keyedHashFunctionCount() - 1), or NULL if the index
 * was out of bounds.
 *
 * keyedHashFunctionFind returns the keyed hash function with the given name,
 * or NULL if no keyed hash function exists with that name.
 */

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int							keyedHashFunctionCount(void);
BEECRYPTAPI
const keyedHashFunction*	keyedHashFunctionGet(int);
BEECRYPTAPI
const keyedHashFunction*	keyedHashFunctionFind(const char*);
BEECRYPTAPI
const keyedHashFunction*	keyedHashFunctionDefault(void);

#ifdef __cplusplus
}
#endif

/*
 * The struct 'keyedHashFunctionContext' is used to contain both the functional
 * part (the keyedHashFunction), and its parameters.
 */
#ifdef __cplusplus
struct BEECRYPTAPI keyedHashFunctionContext
#else
struct _keyedHashFunctionContext
#endif
{
	const keyedHashFunction*	algo;
	keyedHashFunctionParam*		param;

	#ifdef __cplusplus
	keyedHashFunctionContext();
	keyedHashFunctionContext(const keyedHashFunction*);
	~keyedHashFunctionContext();
	#endif
};

#ifndef __cplusplus
typedef struct _keyedHashFunctionContext keyedHashFunctionContext;
#endif

/*
 * The following functions can be used to initialize and free a
 * keyedHashFunctionContext. Initializing will allocate a buffer of the size
 * required by the keyedHashFunction, freeing will deallocate that buffer.
 */

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int keyedHashFunctionContextInit(keyedHashFunctionContext*, const keyedHashFunction*);
BEECRYPTAPI
int keyedHashFunctionContextFree(keyedHashFunctionContext*);
BEECRYPTAPI
int keyedHashFunctionContextSetup(keyedHashFunctionContext*, const byte*, size_t);
BEECRYPTAPI
int keyedHashFunctionContextReset(keyedHashFunctionContext*);
BEECRYPTAPI
int keyedHashFunctionContextUpdate(keyedHashFunctionContext*, const byte*, size_t);
BEECRYPTAPI
int keyedHashFunctionContextUpdateMC(keyedHashFunctionContext*, const memchunk*);
BEECRYPTAPI
int keyedHashFunctionContextUpdateMP(keyedHashFunctionContext*, const mpnumber*);
BEECRYPTAPI
int keyedHashFunctionContextDigest(keyedHashFunctionContext*, byte*);
BEECRYPTAPI
int keyedHashFunctionContextDigestMP(keyedHashFunctionContext*, mpnumber*);
BEECRYPTAPI
int keyedHashFunctionContextDigestMatch(keyedHashFunctionContext*, const mpnumber*);

#ifdef __cplusplus
}
#endif

/*
 * Block ciphers
 */

/*!\enum cipherOperation
 * \brief Specifies whether to perform encryption or decryption.
 * \ingroup BC_m
 */
typedef enum
{
	NOCRYPT,
	ENCRYPT,
	DECRYPT
} cipherOperation;

/*!\typedef void blockCipherParam
 * \brief Placeholder type definition for blockcipher parameters.
 * \sa aesParam, blowfishParam.
 * \ingroup BC_m
 */
typedef void blockCipherParam;

/*!\brief Prototype definition for a setup function.
 * \ingroup BC_m
 */
typedef int (*blockCipherSetup  )(blockCipherParam*, const byte*, size_t, cipherOperation);

/*!\typedef int (*blockCipherSetIV)(blockCipherParam* bp, const byte* iv)
 * \brief Prototype definition for an initialization vector setup function.
 * \param bp The blockcipher's parameters.
 * \param iv The blockciphers' IV value.
 * \note iv length must be equal to the cipher's block size.
 * \retval 0 on success.
 * \retval -1 on failure.
 * \ingroup BC_m
 */
typedef int (*blockCipherSetIV  )(blockCipherParam*, const byte*);

/*!\typedef int (*blockCipherRawcrypt)(blockCipherParam* bp, uint32_t* dst, const uint32_t* src)
 * \brief Prototype for a \e raw encryption or decryption function.
 * \param bp The blockcipher's parameters.
 * \param dst The ciphertext address; must be aligned on 32-bit boundary.
 * \param src The cleartext address; must be aligned on 32-bit boundary.
 * \retval 0 on success.
 * \retval -1 on failure.
 * \ingroup BC_m
 */
typedef int (*blockCipherRawcrypt)(blockCipherParam*, uint32_t*, const uint32_t*);

/*!\typedef int (*blockCipherModcrypt)(blockCipherParam* bp, uint32_t* dst, const uint32_t* src, unsigned int nblocks)
 * \brief Prototype for a \e encryption or decryption function which operates
 *        on multiple blocks in a certain mode.
 * \param bp The blockcipher's parameters.
 * \param dst The ciphertext address; must be aligned on 32-bit boundary.
 * \param src The cleartext address; must be aligned on 32-bit boundary.
 * \param nblocks The number of blocks to process.
 * \retval 0 on success.
 * \retval -1 on failure.
 * \ingroup BC_m
 */
typedef int (*blockCipherModcrypt)(blockCipherParam*, uint32_t*, const uint32_t*, unsigned int);

typedef uint32_t* (*blockCipherFeedback)(blockCipherParam*);

typedef struct
{
	const blockCipherRawcrypt encrypt;
	const blockCipherRawcrypt decrypt;
} blockCipherRaw;

typedef struct
{
	const blockCipherModcrypt encrypt;
	const blockCipherModcrypt decrypt;
} blockCipherMode;

/*!\brief Holds information and pointers to code specific to each cipher.
 *
 * Specific block ciphers \e may be written to be multithread-safe.
 *
 * \ingroup BC_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI blockCipher
#else
struct _blockCipher
#endif
{
	/*!\var name
	 * \brief The blockcipher's name.
	 */
	const char*					name;
	/*!\var paramsize
	 * \brief The size of the parameters required by this cipher, in bytes.
	 */
	const size_t				paramsize;
	/*!\var blocksize
	 * \brief The size of one block of data, in bytes.
	 */
	const size_t				blocksize;
	/*!\var keybitsmin
	 * \brief The minimum number of key bits.
	 */
	const size_t				keybitsmin;
	/*!\var keybitsmax
	 * \brief The maximum number of key bits.
	 */
	const size_t				keybitsmax;
	/*!\var keybitsinc
	 * \brief The allowed increment in key bits between min and max.
	 * \see keybitsmin and keybitsmax.
	 */
	const size_t				keybitsinc;
	/*!\var setup
	 * \brief Pointer to the cipher's setup function.
	 */
	const blockCipherSetup		setup;
	/*!\var setiv
	 * \brief Pointer to the cipher's initialization vector setup function.
	 */
	const blockCipherSetIV		setiv;
	/*!\var raw
	 * \brief The cipher's raw functions.
	 */
	const blockCipherRaw		raw;
	/*!\var ecb
	 * \brief The cipher's ECB functions.
	 */
	const blockCipherMode		ecb;
	const blockCipherMode		cbc;
	/*!\var getfb
	 * \brief Pointer to the cipher's feedback-returning function.
	 */
	const blockCipherFeedback	getfb;
};

#ifndef __cplusplus
typedef struct _blockCipher blockCipher;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\fn int blockCipherCount()
 * \brief This function returns the number of blockciphers implemented
 *  by the library.
 * \return The number of implemented blockciphers.
 */
BEECRYPTAPI
int						blockCipherCount(void);

/*!\fn const blockCipher* blockCipherGet(int n)
 * \brief This function returns the \a n -th blockcipher implemented by
 *  the library.
 * \param n Index of the requested blockcipher; legal values are 0
 *  through blockCipherCount() - 1.
 * \return A pointer to a blockcipher or null, if the index was out of
 *  range.
 */
BEECRYPTAPI
const blockCipher*		blockCipherGet(int);

/*!\fn const blockCipher* blockCipherFind(const char* name)
 * \brief This function returns the blockcipher specified by the given name.
 * \param name Name of the requested blockcipher.
 * \return A pointer to a blockcipher or null, if the name wasn't found.
 */
BEECRYPTAPI
const blockCipher*		blockCipherFind(const char*);

/*!\fn const blockCipher* blockCipherDefault()
 * \brief This functions returns the default blockcipher; the default value
 *  can be specified by setting environment variable BEECRYPT_CIPHER.
 * \return A pointer to a blockcipher or null, in case an error occured.
 */
BEECRYPTAPI
const blockCipher*		blockCipherDefault(void);

#ifdef __cplusplus
}
#endif

/*!\brief Holds a pointer to a blockcipher as well as its parameters.
 * \warning A context can be used by only one thread at the same time.
 * \ingroup BC_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI blockCipherContext
#else
struct _blockCipherContext
#endif
{
	/*!\var algo
	 * \brief Pointer to a blockCipher.
	 */
	const blockCipher*	algo;
	/*!\var param
	 * \brief Pointer to the parameters used by algo.
	 */
	blockCipherParam*	param;
	/*!\var op
	 */
	cipherOperation		op;

	#ifdef __cplusplus
	blockCipherContext();
	blockCipherContext(const blockCipher*);
	~blockCipherContext();
	#endif
};

#ifndef __cplusplus
typedef struct _blockCipherContext blockCipherContext;
#endif

/*
 * The following functions can be used to initialize and free a
 * blockCipherContext. Initializing will allocate a buffer of the size
 * required by the blockCipher, freeing will deallocate that buffer.
 */

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int blockCipherContextInit(blockCipherContext*, const blockCipher*);

BEECRYPTAPI
int blockCipherContextSetup(blockCipherContext*, const byte*, size_t, cipherOperation);

BEECRYPTAPI
int blockCipherContextSetIV(blockCipherContext*, const byte*);

BEECRYPTAPI
int blockCipherContextFree(blockCipherContext*);

BEECRYPTAPI
int blockCipherContextECB(blockCipherContext*, uint32_t*, const uint32_t*, int);

BEECRYPTAPI
int blockCipherContextCBC(blockCipherContext*, uint32_t*, const uint32_t*, int);

BEECRYPTAPI
int blockCipherContextValidKeylen(blockCipherContext*, size_t);

#ifdef __cplusplus
}
#endif

#endif
