#include <stdio.h>

#include "beecrypt/sha1.h"
#include "beecrypt/aes.h"
#include <string.h>
#include "beecrypt/rsa.h"
#include "beecrypt/fips186.h"
#include "beecrypt/entropy.h"

//////////// entropy

extern int entropy_dev_urandom(byte* data, size_t size);
extern int randomGeneratorContextInit(randomGeneratorContext* ctxt, const randomGenerator* rng);
extern const randomGenerator* randomGeneratorDefault();

//////////// SHA1
struct vector
{
	int input_size;
	byte* input;
	byte* expect;
};

struct vector table[2] = {
	{  3, (byte*) "abc",
	      (byte*) "\xA9\x99\x3E\x36\x47\x06\x81\x6A\xBA\x3E\x25\x71\x78\x50\xC2\x6C\x9C\xD0\xD8\x9D" },
	{ 56, (byte*) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
		  (byte*) "\x84\x98\x3E\x44\x1C\x3B\xD2\x6E\xBA\xAE\x4A\xA1\xF9\x51\x29\xE5\xE5\x46\x70\xF1" }
};

///////////////////////  AES
struct vectorAES
{
	char*			key;
	char*			input;
	char*			expect;
	cipherOperation	op;
};

#define NVECTORS 6

struct vectorAES tableAES[NVECTORS] = {
	{ "000102030405060708090a0b0c0d0e0f",
	  "00112233445566778899aabbccddeeff",
	  "69c4e0d86a7b0430d8cdb78070b4c55a",
	  ENCRYPT },
	{ "000102030405060708090a0b0c0d0e0f",
	  "69c4e0d86a7b0430d8cdb78070b4c55a",
	  "00112233445566778899aabbccddeeff",
	  DECRYPT },
	{ "000102030405060708090a0b0c0d0e0f1011121314151617",
	  "00112233445566778899aabbccddeeff",
	  "dda97ca4864cdfe06eaf70a0ec0d7191",
	  ENCRYPT },
	{ "000102030405060708090a0b0c0d0e0f1011121314151617",
	  "dda97ca4864cdfe06eaf70a0ec0d7191",
	  "00112233445566778899aabbccddeeff",
	  DECRYPT },
	{ "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	  "00112233445566778899aabbccddeeff",
	  "8ea2b7ca516745bfeafc49904b496089",
	  ENCRYPT },
	{ "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	  "8ea2b7ca516745bfeafc49904b496089",
	  "00112233445566778899aabbccddeeff",
	  DECRYPT }
};

//////////////// RSA
// public bits
// n is modulus
static const char* rsa_n  = "bbf82f090682ce9c2338ac2b9da871f7368d07eed41043a440d6b6f07454f51fb8dfbaaf035c02ab61ea48ceeb6fcd4876ed520d60e1ec4619719d8a5b8b807fafb8e0a3dfc737723ee6b4b7d93a2584ee6a649d060953748834b2454598394ee0aab12d7b61a51f527a9a41f6c1687fe2537298ca2a8f5946f8e5fd091dbdcb";
// e is exponent
static const char* rsa_e  = "11"; // 11
// private bits
// p is secret prime factor 1
static const char* rsa_p  = "eecfae81b1b9b3c908810b10a1b5600199eb9f44aef4fda493b81a9e3d84f632124ef0236e5d1e3b7e28fae7aa040a2d5b252176459d1f397541ba2a58fb6599";
// q is secret prime factor 2
static const char* rsa_q  = "c97fb1f027f453f6341233eaaad1d9353f6c42d08866b1d05a0f2035028b9d869840b41666b42e92ea0da3b43204b5cfce3352524d0416a5a441e700af461503";
// dp (d1) is is first prime coefficient
static const char* rsa_d1 = "54494ca63eba0337e4e24023fcd69a5aeb07dddc0183a4d0ac9b54b051f2b13ed9490975eab77414ff59c1f7692e9a2e202b38fc910a474174adc93c1f67c981";
// dq (d2) is the second prime coefficient
static const char* rsa_d2 = "471e0290ff0af0750351b7f878864ca961adbd3a8a7e991c5c0556a94c3146a7f9803f8f6f8ae342e931fd8ae47a220d1b99a495849807fe39f9245a9836da3d";
// qi (c) is the crt coefficient
static const char* rsa_c  = "b06c4fdabb6301198d265bdbae9423b380f271f73453885093077fcd39e2119fc98632154f5883b167a967bf402b4e9e2e0f9656e698ea3666edfb25798039f7";

static const char* rsa_m  = "d436e99569fd32a7c8a05bbc90d32c49";


int fromhex(byte* data, const char* hexdata)
{
	int length = strlen(hexdata);
	int count = 0, index = 0;
	byte b = 0;
	char ch;

	if (length & 1)
		count = 1;

	while (index++ < length)
	{
		ch = *(hexdata++);

		b <<= 4;
		if (ch >= '0' && ch <= '9')
			b += (ch - '0');
		else if (ch >= 'A' && ch <= 'F')
			b += (ch - 'A') + 10;
		else if (ch >= 'a' && ch <= 'f')
			b += (ch - 'a') + 10;

		count++;
		if (count == 2)
		{
			*(data++) = b;
			b = 0;
			count = 0;
		}
	}
	return (length+1) >> 1;
}

int testSha1() {
  int i, failures = 0;
  byte digest[20];
  sha1Param param;

  for (i = 0; i < 2; i++) {
    if (sha1Reset(&param))
      return -1;
    if (sha1Update(&param, table[i].input, table[i].input_size))
      return -1;
    if (sha1Digest(&param, digest))
      return -1;

    if (memcmp(digest, table[i].expect, 20)) {
      printf("failed test vector %d\n", i+1);
      failures++;
    }
  }
  return failures;
}

int testAES() {
  int i, failures = 0;
  aesParam param;
  byte key[32];
  byte src[16];
  byte dst[16];
  byte chk[16];
  size_t keybits;

  for (i = 0; i < NVECTORS; i++) {
      keybits = fromhex(key, tableAES[i].key) << 3;
      
      if (aesSetup(&param, key, keybits, tableAES[i].op))
	return -1;
      
      fromhex(src, tableAES[i].input);
      fromhex(chk, tableAES[i].expect);
      
      switch (tableAES[i].op) {
      case ENCRYPT:
	if (aesEncrypt(&param, (uint32_t*) dst, (const uint32_t*) src))
	  return -1;
	break;
      case DECRYPT:
	if (aesDecrypt(&param, (uint32_t*) dst, (const uint32_t*) src))
	  return -1;
	break;
      }
      
      if (memcmp(dst, chk, 16)) {
	printf("failed vector %d\n", i+1);
	failures++;
      }
  }
  return failures;
}

int testRSA() {
  int failures = 0;

  rsakp keypair;
  mpnumber m, cipher, decipher;
  randomGeneratorContext rngc;

  /* First we do the fixed value verification */
  rsakpInit(&keypair);

  mpbsethex(&keypair.n, rsa_n);
  mpnsethex(&keypair.e, rsa_e);
  mpbsethex(&keypair.p, rsa_p);
  mpbsethex(&keypair.q, rsa_q);
  mpnsethex(&keypair.dp, rsa_d1);
  mpnsethex(&keypair.dq, rsa_d2);
  mpnsethex(&keypair.qi, rsa_c);

  mpnzero(&m);
  mpnzero(&cipher);
  mpnzero(&decipher);

  mpnsethex(&m, rsa_m);

  /* it's safe to cast the keypair to a public key */
  if (rsapub(&keypair.n, &keypair.e, &m, &cipher))
    failures++;
  
  if (rsapricrt(&keypair.n, &keypair.p, &keypair.q, &keypair.dp, &keypair.dq, &keypair.qi, &cipher, &decipher))
    failures++;
    
  if (mpnex(m.size, m.data, decipher.size, decipher.data))
    failures++;
    
  mpnfree(&decipher);
  mpnfree(&cipher);
  mpnfree(&m);
  
  rsakpFree(&keypair);

  mpnzero(&m);
  mpnzero(&cipher);
  mpnzero(&decipher);

  if (randomGeneratorContextInit(&rngc, randomGeneratorDefault()) == 0) {
    printf( "making prime...\n" );
    /* Now we generate a keypair and do some tests on it */
    rsakpMake(&keypair, &rngc, 2048);
    printf( "done.\n" );

    /* generate a random m in the range 0 < m < n */
    mpbnrnd(&keypair.n, &rngc, &m);

    /* it's safe to cast the keypair to a public key */
    if (rsapub(&keypair.n, &keypair.e, &m, &cipher))
      failures++;

    if (rsapricrt(&keypair.n, &keypair.p, &keypair.q, &keypair.dp, &keypair.dq, &keypair.qi, &cipher, &decipher))
      failures++;
    
    if (mpnex(m.size, m.data, decipher.size, decipher.data))
      failures++;

    rsakpFree(&keypair);
  }
  return failures;
}
int main() {
  
  if(testSha1() != 0)
    printf( "SHA1 has problems.\n");
  
  if(testAES() != 0 ) 
    printf( "AES has problems.\n");

  if(testRSA() != 0 )
    printf( "RSA has problems.\n");
}
