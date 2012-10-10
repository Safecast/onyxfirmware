/*
 * Copyright (c) 2004 Beeyond Software Holding B.V.
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

/*!\file hmacsha384.h
 * \brief HMAC-SHA-384 message authentication code, headers.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HMAC_m HMAC_sha384_m
 */

#ifndef _HMACSHA384_H
#define _HMACSHA384_H

#include "beecrypt/hmac.h"
#include "beecrypt/sha384.h"

/*!\ingroup HMAC_sha384_m
 */
typedef struct
{
	sha384Param sparam;
	byte kxi[128];
	byte kxo[128];
} hmacsha384Param;

#ifdef __cplusplus
extern "C" {
#endif

extern BEECRYPTAPI const keyedHashFunction hmacsha384;

BEECRYPTAPI
int hmacsha384Setup (hmacsha384Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha384Reset (hmacsha384Param*);
BEECRYPTAPI
int hmacsha384Update(hmacsha384Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha384Digest(hmacsha384Param*, byte*);

#ifdef __cplusplus
}
#endif

#endif
