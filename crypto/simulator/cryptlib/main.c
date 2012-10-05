/// chumby crypto routines
/// bunnie@chumby.com

#include <stdio.h>

#include "beecrypt/sha1.h"
#include "beecrypt/aes.h"
#include <string.h>
#include "beecrypt/rsa.h"
#include "beecrypt/fips186.h"
#include "beecrypt/entropy.h"

#include "simpleCrypt.h"

#define TESTING 0

AESkey AESkeyTable[MAX_KEY_INDEX + 1];
RSAkey RSAkeyTable[MAX_KEY_INDEX + 1];
char RSAkeyIdx[MAX_KEY_INDEX + 1][33];

char hexToAscii(char c) {
  return(c < 0xA ? c + '0' : c + 'A' - 0xA);
}

int toHex(byte *digest, char *digestHex, int numBytes) {
  int i = 0;

  // assume MSB first for digest
  for( i = 0; i < numBytes; i++ ) {
    digestHex[2*i] = hexToAscii((digest[i] & 0xF0) >> 4);
    digestHex[2*i + 1] = hexToAscii((digest[i] & 0xF));
  }
  digestHex[2*i] = '\0';
  
  return(numBytes * 2 + 1);
}

int fromhex(byte* data, const char* hexdata) {
  int length = strlen(hexdata);
  int count = 0, index = 0;
  byte b = 0;
  char ch;

  if (length & 1)
    count = 1;
  
  while (index++ < length) {
    ch = *(hexdata++);
    
    b <<= 4;
    if (ch >= '0' && ch <= '9')
      b += (ch - '0');
    else if (ch >= 'A' && ch <= 'F')
      b += (ch - 'A') + 10;
    else if (ch >= 'a' && ch <= 'f')
      b += (ch - 'a') + 10;

    count++;
    if (count == 2) {
      *(data++) = b;
      b = 0;
      count = 0;
    }
  }
  return (length+1) >> 1;
}


#define MAX_INPUT_LEN 2048
#if TESTING
//char str[] = "e:0:AES:00112233445566778899aabbccddeeff\n\0";
//char str[] = "d:0:AES:69c4e0d86a7b0430d8cdb78070b4c55a\n\0";
char str[] = "e:0:SGN:69c4e0d86a7b0430d8cdb78070b4c55a\n\0";

char *getString() {
  static int first = 1;
  
  if( first ) {
    first = 0;
  } else {
    exit(0);
  }
  
  return str;
}
#else
// put the REAL getString here...
char *getString() {
  char *str;
  int i;
  
  str = malloc(sizeof(char) * MAX_INPUT_LEN);
  i = 0;
  do {
    str[i] = getchar();
  } while( (str[i++] != '\n') && (i < (MAX_INPUT_LEN - 1)) );
  
  str[i] = '\0';

  return( str );
}
#endif

#if TESTING
//////////////// RSA test keys
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

int initKeys() {
  int i;
  
  // all the same AES keys for now
  // eventually, this function goes away and key access is simply mapped to internal secure ROM
  for( i = 0; i < MAX_KEY_INDEX; i++ ) {
    fromhex(AESkeyTable[i].key, "000102030405060708090a0b0c0d0e0f");
  }
  for( i = 0; i < MAX_KEY_INDEX; i++ ) {
    RSAkeyTable[i].rsa_n = (char *)rsa_n;
    RSAkeyTable[i].rsa_e = (char *)rsa_e;
    RSAkeyTable[i].rsa_p = (char *)rsa_p;
    RSAkeyTable[i].rsa_q = (char *)rsa_q;
    RSAkeyTable[i].rsa_dp = (char *)rsa_d1;
    RSAkeyTable[i].rsa_dq = (char *)rsa_d2;
    RSAkeyTable[i].rsa_qi = (char *)rsa_c;
  }
  return 0;
}
#else
// TEMPORARY ROUTINE FOR "REAL" USE
// NEED METHOD FOR GENERATING AND REPORTING KEYS

//////////////// RSA test keys
// public bits
// n is modulus
static char rsa_n[520];
// e is exponent
static char rsa_e[16];
// private bits
// p is secret prime factor 1
static char rsa_p[260];
// q is secret prime factor 2
static char rsa_q[260];
// dp (d1) is is first prime coefficient
static char rsa_d1[260];
// dq (d2) is the second prime coefficient
static char rsa_d2[260];
// qi (c) is the crt coefficient
static char rsa_c[260];

int initKeys(char *fname) {
  FILE *keyFile;
  char typeStr[600];
  char keyStr[600];
  int idx;
  
  keyFile = fopen(fname, "r");
  if( keyFile == NULL )
    return -1;
  
  while( !feof(keyFile) ) {
    fscanf(keyFile, "%[0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_#,.!? ]:", typeStr);
    if( typeStr[0] == '#' ) {
      //      printf("found comment.\n" );
      fgetc(keyFile); // eat the \n
      continue; // ignore comments
    }
    fscanf(keyFile, "%d:%[0123456789ABCDEFabcdef]\n", &idx, keyStr);
    //    printf( "got %s:%d:%s\n", typeStr, idx, keyStr );
    //    fflush(stdout);

    if( idx >= MAX_KEY_INDEX )
      continue; // ignore out of range index requests
    
    if( strcmp( "AES", typeStr ) == 0 ) {
      fromhex(AESkeyTable[idx].key, keyStr );
    } else if( strcmp( "PKI_I", typeStr ) == 0 ) {
      strncpy(RSAkeyIdx[idx], keyStr, 33); // indices are 32 characters + a NULL
    } else if( strcmp( "PKI_N", typeStr ) == 0 ) {
      strncpy(rsa_n, keyStr, 513 );
      RSAkeyTable[idx].rsa_n = rsa_n;   // note that we constantly bash this, so only the last key entered sticks
    } else if( strcmp( "PKI_E", typeStr ) == 0 ) {
      strncpy(rsa_e, keyStr, 9 );
      RSAkeyTable[idx].rsa_e = rsa_e;
    } else if( strcmp( "PKI_P", typeStr ) == 0 ) {
      strncpy(rsa_p, keyStr, 257 );
      RSAkeyTable[idx].rsa_p = rsa_p;
    } else if( strcmp( "PKI_Q", typeStr ) == 0 ) {
      strncpy(rsa_q, keyStr, 257 );
      RSAkeyTable[idx].rsa_q = rsa_q;
    } else if( strcmp( "PKI_DP", typeStr ) == 0 ) {
      strncpy(rsa_d1, keyStr, 257 );
      RSAkeyTable[idx].rsa_dp = rsa_d1;
    } else if( strcmp( "PKI_DQ", typeStr ) == 0 ) {
      strncpy(rsa_d2, keyStr, 257 );
      RSAkeyTable[idx].rsa_dq = rsa_d2;
    } else if( strcmp( "PKI_QI", typeStr ) == 0 ) {
      strncpy(rsa_c, keyStr, 257 );
      RSAkeyTable[idx].rsa_qi = rsa_c;
    }
    // else we just didn't hear you.
  }
  return 0;
}

#endif

#define MAXDATLEN 600
void doInteraction() {
  char *cmd;
  // i/o
  opRec oprec;
  unsigned char dataStr[256];
  // aes
  aesParam aesparam;
  byte aesSrc[16];
  byte aesDst[16];
  // rsa
  rsakp keypair;
  mpnumber m, cipher, signature;
  // sha1
  byte digest[20];
  char digestHex[41];
  sha1Param sha1param;
  
  int i;

  oprec.data = dataStr;
  oprec.dataLen = MAXDATLEN;

  while(1) {
    // reset the data string
    for( i = 0; i < MAXDATLEN; i++ ) {
      oprec.data[i] = '0';
    }
    oprec.dataLen = MAXDATLEN;

    // grab the string and parse it
    cmd = getString();
    if(parseString(cmd, &oprec) != 1) {
      oprec.dataLen = MAXDATLEN;
      continue;
    }
    
    switch(oprec.cipherType) {
    case CH_AES:
      for( i = 0; i < 16; i++ ) {
	aesSrc[i] = 0;
      }
      if(aesSetup(&aesparam, AESkeyTable[oprec.keyIndex].key, 128, oprec.opType == CH_ENCRYPT ? ENCRYPT : DECRYPT ))
	continue;
      fromhex(aesSrc, oprec.data);
      if( oprec.opType == CH_ENCRYPT ) {
	if( aesEncrypt(&aesparam, (uint32_t *)aesDst, (uint32_t *)aesSrc) )
	  continue;
      } else {
	if( aesDecrypt(&aesparam, (uint32_t *)aesDst, (uint32_t *)aesSrc) )
	  continue;
      }
      for( i = 0; i < 16; i++ ) {
	printf("%02X", aesDst[i] );
      }
      printf( "\n" );
      break;
    case CH_SGN:
      // init sha1
      if( sha1Reset( &sha1param ) )
	continue;
      if( sha1Update( &sha1param, oprec.data, oprec.dataLen ))
	continue;
      if( sha1Digest( &sha1param, digest ) )
	continue;
      
      // digest now contains the 160-bit message we want to sign
      toHex(digest, digestHex, 20);
      // digestHex now has the correct large number representation of the message
#if TESTING
      fprintf( stderr, "sha1 of message: %s\n", digestHex );
#endif
      // init rsa
      rsakpInit(&keypair);
      
      mpbsethex(&keypair.n, RSAkeyTable[oprec.keyIndex].rsa_n);
      mpnsethex(&keypair.e, RSAkeyTable[oprec.keyIndex].rsa_e);
      mpbsethex(&keypair.p, RSAkeyTable[oprec.keyIndex].rsa_p);
      mpbsethex(&keypair.q, RSAkeyTable[oprec.keyIndex].rsa_q);
      mpnsethex(&keypair.dp, RSAkeyTable[oprec.keyIndex].rsa_dp);
      mpnsethex(&keypair.dq, RSAkeyTable[oprec.keyIndex].rsa_dq);
      mpnsethex(&keypair.qi, RSAkeyTable[oprec.keyIndex].rsa_qi);

      mpnzero(&m);
      mpnzero(&cipher);
      mpnzero(&signature);

      mpnsethex(&m, digestHex);
      
      // we are now all set to do the signing
      // need to:
      // write signing alg here
      // make test case
      // this link is very helpful in writing this code:
      // http://tools.ietf.org/html/rfc3447#page-12
      rsapricrt(&keypair.n, &keypair.p, &keypair.q, &keypair.dp, &keypair.dq, &keypair.qi, &m, &signature);
      for( i = 0; i < signature.size; i++ ) {
	printf("%08X", signature.data[i] );
      }
      printf( "\n" );

#if TESTING
      mpnfree(&m);
      mpnzero(&m);
      rsapub(&keypair.n, &keypair.e, &signature, &m);
      for( i = 0; i < m.size; i++ ) {
	printf("%08X", m.data[i] );
      }
      printf( "\n" );
#endif

      rsakpFree(&keypair);
      break;
    case CH_VRF:
      rsakpInit(&keypair);
      
      mpbsethex(&keypair.n, RSAkeyTable[oprec.keyIndex].rsa_n);
      mpnsethex(&keypair.e, RSAkeyTable[oprec.keyIndex].rsa_e);
      mpbsethex(&keypair.p, RSAkeyTable[oprec.keyIndex].rsa_p);
      mpbsethex(&keypair.q, RSAkeyTable[oprec.keyIndex].rsa_q);
      mpnsethex(&keypair.dp, RSAkeyTable[oprec.keyIndex].rsa_dp);
      mpnsethex(&keypair.dq, RSAkeyTable[oprec.keyIndex].rsa_dq);
      mpnsethex(&keypair.qi, RSAkeyTable[oprec.keyIndex].rsa_qi);

      mpnzero(&m);
      mpnzero(&cipher);
      mpnzero(&signature);

      mpnsethex(&m, oprec.data);
      rsapub(&keypair.n, &keypair.e, &m, &cipher);

      for( i = 0; i < cipher.size; i++ ) 
	printf("%08X", cipher.data[i]);
      printf( "\n" );
      break;
      
    case CH_SHA:
      // init sha1
      if( sha1Reset( &sha1param ) )
	continue;
      if( sha1Update( &sha1param, oprec.data, oprec.dataLen ))
	continue;
      if( sha1Digest( &sha1param, digest ) )
	continue;
      
      // digest now contains the 160-bit message we want to sign
      toHex(digest, digestHex, 20);
      printf( "%s\n", digestHex );
      break;
      
    default:
      fprintf( stderr, "unknown cipher type caught.\n" );
    } // switch
    
    // prevent the leak!
#if (TESTING == 0)
    if( cmd != NULL )
      free(cmd);
#endif
  } // while
  
}

void printkeys() {
  int i, j;

  for( i = 0; i < MAX_KEY_INDEX; i++ ) {
    for( j = 0; j < 16; j++ ) {
      printf( "%02X", AESkeyTable[i].key[j] & 0xFF );
    }
    printf( "\n" );
  }
  printf( "%s\n", RSAkeyTable[0].rsa_n );
  printf( "%s\n", RSAkeyTable[0].rsa_e );
  printf( "%s\n", RSAkeyTable[0].rsa_p );
  printf( "%s\n", RSAkeyTable[0].rsa_q );
  printf( "%s\n", RSAkeyTable[0].rsa_dp );
  printf( "%s\n", RSAkeyTable[0].rsa_dq );
  printf( "%s\n", RSAkeyTable[0].rsa_qi );
}

int main( int argc, char **argv ) {
  if( argc < 2 ) {
    printf( "usage: %s <keyfile>\n", argv[0] );
    exit( 1 );
  }
  
  if( initKeys(argv[1]) ) {
    printf( "Error in reading in key file, aborting.\n" );
    exit(1);
  }

  //printkeys();

  doInteraction();

  return 0;
}
