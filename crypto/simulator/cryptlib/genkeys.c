// chumby key generator
// bunnie@chumby.com

#include <stdio.h>

#include "beecrypt/sha1.h"
#include "beecrypt/aes.h"
#include <string.h>
#include "beecrypt/rsa.h"
#include "beecrypt/fips186.h"
#include "beecrypt/entropy.h"

#include "simpleCrypt.h"


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

int main() {
  FILE *rng;
  unsigned long rnum;
  int i, j;
  randomGeneratorContext rngc;
  byte keyTest[32];
  char keyHex[80];
  rsakp keypair;
  unsigned char mac[6];
  byte digest[20];
  char digestHex[41];
  sha1Param sha1param;

  randomGeneratorContextInit(&rngc, randomGeneratorDefault());
  rsakpInit(&keypair);

  fscanf( stdin, "%02X:%02X:%02X:%02X:%02X:%02X", &(mac[0]),&(mac[1]),&(mac[2]),&(mac[3]),&(mac[4]),&(mac[5]) );
  sha1Reset( &sha1param );
  sha1Update( &sha1param, mac, 6 );  // put the MAC into the SHA-1
  rngc.rng->next(rngc.param, (byte*) keyTest, 16);
  sha1Update( &sha1param, keyTest, 16 ); // then add some random numbers for good measure
  sha1Digest( &sha1param, digest );
  toHex(digest, digestHex, 16); // and there you have our putative ID
  // just in case we are a bit too deterministic, we push around
  // the RNG some amount that is linked to the MAC...not really a permanent
  // solution, but it should help avoid collisions.
  for( i = 0; i < digest[0]; i++ ) {
    rngc.rng->next(rngc.param, (byte*) keyTest, 16);
  }
  for( i = 0; i < digest[12]; i++ ) {
    rngc.rng->next(rngc.param, (byte*) keyTest, 16);
  }
  for( i = 0; i < digest[7]; i++ ) {
    rngc.rng->next(rngc.param, (byte*) keyTest, 16);
  }
  for( i = 0; i < digest[17]; i++ ) {
    rngc.rng->next(rngc.param, (byte*) keyTest, 16);
  }

  fprintf(stdout, "# Indeed, these are crypto keys. The overall system is designed to tolerate people finding these keys. Please note that modifying or copying these keys may cause us to be unable to recover your account information.\n" );
  for( i = 0; i < MAX_KEY_INDEX; i++ ) {
    rngc.rng->next(rngc.param, (byte*) keyTest, 16);
    toHex(keyTest, keyHex, 16);
    fprintf(stdout, "AES:%d:", i );
    fprintf(stdout, "%s\n", keyHex );
    fflush(stdout);
  }
  fprintf( stderr, "Generating 2048-bit RSA key pair..." );
  rsakpMake(&keypair, &rngc, 2048);
  fprintf( stderr, "Done.\n" );
  fflush( stderr );

  for( j = 0; j < MAX_KEY_INDEX; j++ ) {
    // create the putative index
    //rngc.rng->next(rngc.param, (byte*) keyTest, 16);
    //toHex(keyTest, keyHex, 16);
    fprintf(stdout, "PKI_I:%d:", j );
    //    fprintf(stdout, "%s", keyHex );
    fprintf(stdout, "%s", digestHex );
    // save the public key
    fprintf(stdout, "\nPKI_N:%d:", j );
    for( i = 0; i < keypair.n.size; i++ )
      fprintf(stdout, "%08X", keypair.n.modl[i] );
    fprintf(stdout, "\nPKI_E:%d:", j );
    for( i = 0; i < keypair.e.size; i++ ) 
      fprintf(stdout, "%08X", keypair.e.data[i]);
    // now save the private key components used in the CRT
    fprintf(stdout, "\nPKI_P:%d:", j );
    for( i = 0; i < keypair.p.size; i++ )
      fprintf(stdout, "%08X", keypair.p.modl[i] );
    fprintf(stdout, "\nPKI_Q:%d:", j );
    for( i = 0; i < keypair.q.size; i++ )
      fprintf(stdout, "%08X", keypair.q.modl[i] );
    fprintf(stdout, "\nPKI_DP:%d:", j );
    for( i = 0; i < keypair.dp.size; i++ ) 
      fprintf(stdout, "%08X", keypair.dp.data[i]);
    fprintf(stdout, "\nPKI_DQ:%d:", j );
    for( i = 0; i < keypair.dq.size; i++ ) 
      fprintf(stdout, "%08X", keypair.dq.data[i]);
    fprintf(stdout, "\nPKI_QI:%d:", j );
    for( i = 0; i < keypair.qi.size; i++ ) 
      fprintf(stdout, "%08X", keypair.qi.data[i]);
    fprintf(stdout, "\n" );
  }
  
  return 0;
}
