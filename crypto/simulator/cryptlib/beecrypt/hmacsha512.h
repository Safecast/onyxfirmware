/*
 * Copyright (c) 2000, 2001, 2002 Virtual Unlimited B.V.
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

/*!\file hmacsha512.h
 * \brief HMAC-SHA-512 message authentication code, headers.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup HMAC_m HMAC_sha512_m
 */

#ifndef _HMACSHA512_H
#define _HMACSHA512_H

#include "beecrypt/hmac.h"
#include "beecrypt/sha512.h"

/*!\ingroup HMAC_sha512_m
 */
typedef struct
{
	sha512Param sparam;
	byte kxi[128];
	byte kxo[128];
} hmacsha512Param;

#ifdef __cplusplus
extern "C" {
#endif

extern BEECRYPTAPI const keyedHashFunction hmacsha512;

BEECRYPTAPI
int hmacsha512Setup (hmacsha512Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha512Reset (hmacsha512Param*);
BEECRYPTAPI
int hmacsha512Update(hmacsha512Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha512Digest(hmacsha512Param*, byte*);

#ifdef __cplusplus
}
#endif

#endif
