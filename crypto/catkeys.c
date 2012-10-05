#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#define SECLEN  2048
#define PUBLEN  2032

int main() {
  FILE *secret, *pub, *out;
  int len = 0;
  uuid_t uuid;

  secret = fopen("./secring.gpg", "rb");
  if( secret == NULL ) {
    printf( "Can't open secret key file, failing.\n" );
    exit(0);
  }
  pub = fopen( "./pubkey.txt", "r" );
  if( pub == NULL ) {
    printf( "Can't open public key file, failing.\n" );
    exit(0);
  }
  out = fopen( "./secblock.bin", "wb" );
  if( out == NULL ) {
    printf( "Can't open binary output file, failing.\n" );
    exit(0);
  }

  len = 0;
  while( !feof(secret) ){
    len ++;
    fputc(fgetc(secret), out);
  }
  if( len >= SECLEN ) {
    printf( "Warning! Secret key input larger than 2k.\n" );
  }
  while( len < SECLEN ) {
    len++;
    fputc(0, out); // pad 0's to 2k length
  }
  
  while( !feof(pub) ) {
    len++;
    fputc(fgetc(pub), out);
  }
  if( len >= (SECLEN + PUBLEN)) {
    printf( "Warning! public key length too long.\n" );
  }
  while( len < (SECLEN + PUBLEN) ) {
    len++;
    fputc(0, out); // pad 0's to almost 4k length
  }
  
  uuid_generate(uuid);

  fwrite( uuid, sizeof(uuid), 1, out );

  fclose(out);
  fclose(secret);
  fclose(pub);
}
