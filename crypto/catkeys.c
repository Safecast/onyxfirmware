#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#define SECLEN  2048
#define PUBLEN  2032

void print_uuid(uuid_t *uuid) {
  unsigned char *data;
  int i;
  
  data = (char *)uuid;
  for( i = 0; i < 16; i++ ) {
    printf( "%02x", data[i] );
  }
  printf( "\n" );
}

int main(int argc, char *argv[]) {
  FILE *secret, *pub, *out;
  int len = 0;
  uuid_t uuid;

  if( argc != 4 ) {
    printf( "Usage: catkeys <secret key file> <pubkey file> <outputfile>\n" );
  }
  secret = fopen(argv[1], "rb");
  if( secret == NULL ) {
    printf( "Can't open secret key file %s, failing.\n", argv[1] );
    exit(0);
  }
  pub = fopen( argv[2], "r" );
  if( pub == NULL ) {
    printf( "Can't open public key file %s, failing.\n", argv[2] );
    exit(0);
  }
  out = fopen( argv[3], "wb" );
  if( out == NULL ) {
    printf( "Can't open binary output file %s, failing.\n", argv[3] );
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
  
  print_uuid(&uuid);

  fclose(out);
  fclose(secret);
  fclose(pub);
}
