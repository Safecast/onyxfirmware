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

  randomGeneratorContextInit(&rngc, randomGeneratorDefault());
  rsakpInit(&keypair);

  for( i = 0; i < MAX_KEY_INDEX; i++ ) {
    rngc.rng->next(rngc.param, (byte*) keyTest, 16);
    toHex(keyTest, keyHex, 16);
    fprintf(stdout, "\nAES:%d\n", i );
    fprintf(stdout, "%s", keyHex );
    fflush(stdout);
  }
  fprintf( stderr, "." );
  rsakpMake(&keypair, &rngc, 2048);
  fprintf( stderr, "." );
  fflush( stderr );

  for( j = 0; j < MAX_KEY_INDEX; j++ ) {
    fprintf(stdout, "\nPKI_N:%d\n", j );
    for( i = 0; i < keypair.n.size; i++ )
      fprintf(stdout, "%08X", keypair.n.modl[i] );
    fprintf(stdout, "\nPKI_E:%d\n", j );
    for( i = 0; i < keypair.e.size; i++ ) 
      fprintf(stdout, "%08X", keypair.e.data[i]);
    fprintf(stdout, "\nPKI_P:%d\n", j );
    for( i = 0; i < keypair.p.size; i++ )
      fprintf(stdout, "%08X", keypair.p.modl[i] );
    fprintf(stdout, "\nPKI_Q:%d\n", j );
    for( i = 0; i < keypair.q.size; i++ )
      fprintf(stdout, "%08X", keypair.q.modl[i] );
    fprintf(stdout, "\nPKI_DP:%d\n", j );
    for( i = 0; i < keypair.dp.size; i++ ) 
      fprintf(stdout, "%08X", keypair.dp.data[i]);
    fprintf(stdout, "\nPKI_DQ:%d\n", j );
    for( i = 0; i < keypair.dq.size; i++ ) 
      fprintf(stdout, "%08X", keypair.dq.data[i]);
    fprintf(stdout, "\nPKI_QI:%d\n", j );
    for( i = 0; i < keypair.qi.size; i++ ) 
      fprintf(stdout, "%08X", keypair.qi.data[i]);
  }
  
  fprintf(stdout, "\n" );
  return 0;
}
