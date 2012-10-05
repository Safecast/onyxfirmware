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

/*!\file sha_k.h
 * \brief SHA-512 and SHA-384 shared constants, headers.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HASH_sha512_m HASH_sha384_m
 */

#ifndef _SHA_K_H
#define _SHA_K_H

#include "beecrypt/beecrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

extern BEECRYPTAPI const uint64_t SHA2_64BIT_K[80];

#ifdef __cplusplus
}
#endif

#endif
