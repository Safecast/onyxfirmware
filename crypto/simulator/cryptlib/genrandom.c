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

  rngc.rng->next(rngc.param, (byte*) keyTest, 8);
  toHex(keyTest, keyHex, 8);
  fprintf(stdout, "%s", keyHex );
  fflush(stdout);

  return 0;
}
