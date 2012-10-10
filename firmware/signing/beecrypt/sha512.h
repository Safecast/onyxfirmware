/*
 * Copyright (c) 2004 Beeyond Software Holding BV
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

/*!\file sha512.h
 * \brief SHA-512 hash function, headers.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HASH_m HASH_sha512_m
 */

#ifndef _SHA512_H
#define _SHA512_H

#include "beecrypt/beecrypt.h"

/*!\brief Holds all the parameters necessary for the SHA-512 algorithm.
 * \ingroup HASH_sha512_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI sha512Param
#else
struct _sha512Param
#endif
{
	/*!\var h
	 */
	uint64_t h[8];
	/*!\var data
	 */
	uint64_t data[80];
	/*!\var length
	 * \brief Multi-precision integer counter for the bits that have been
	 *  processed so far.
	 */
	#if (MP_WBITS == 64)
	mpw length[2];
	#elif (MP_WBITS == 32)
	mpw length[4];
	#else
	# error
	#endif
	/*!\var offset
	 * \brief Offset into \a data; points to the place where new data will be
	 *  copied before it is processed.
	 */
	uint64_t offset;
};

#ifndef __cplusplus
typedef struct _sha512Param sha512Param;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var sha512
 * \brief Holds the full API description of the SHA-512 algorithm.
 */
extern BEECRYPTAPI const hashFunction sha512;

/*!\fn void sha512Process(sha512Param* sp)
 * \brief This function performs the core of the SHA-512 hash algorithm; it
 *  processes a block of 128 bytes.
 * \param sp The hash function's parameter block.
 */
BEECRYPTAPI
void sha512Process(sha512Param* sp);

/*!\fn int sha512Reset(sha512Param* sp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param sp The hash function's parameter block.
 * \retval 0 on success.
 */
BEECRYPTAPI
int  sha512Reset  (sha512Param* sp);

/*!\fn int sha512Update(sha512Param* sp, const byte* data, size_t size)
 * \brief This function should be used to pass successive blocks of data
 *  to be hashed.
 * \param sp The hash function's parameter block.
 * \param data
 * \param size
 * \retval 0 on success.
 */
BEECRYPTAPI
int  sha512Update (sha512Param* sp, const byte* data, size_t size);

/*!\fn int sha512Digest(sha512Param* sp, byte* digest)
 * \brief This function finishes the current hash computation and copies
 *  the digest value into \a digest.
 * \param sp The hash function's parameter block.
 * \param digest The place to store the 64-byte digest.
 * \retval 0 on success.
 */
BEECRYPTAPI
int  sha512Digest (sha512Param* sp, byte* digest);

#ifdef __cplusplus
}
#endif

#endif
