#include <stdio.h>

#include "beecrypt/sha1.h"
#include "beecrypt/aes.h"
#include <string.h>
#include "beecrypt/rsa.h"
#include "beecrypt/fips186.h"
#include "beecrypt/entropy.h"

#include "simpleCrypt.h"

int bytesFromMpn(unsigned int mpnSize) {
  // compute bytes to read based on mpnSize
  return (mpnSize + 7) / 8;
}

void big16read(unsigned short *s, FILE *f) {
  *s = 0;
  *s |= fgetc(f) << 8;
  *s |= fgetc(f);
}

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

int main( int argc, char **argv ) {
  FILE *secblock;

  unsigned char packetType;
  unsigned short packetLen;
  unsigned char buffer[4096];
  unsigned char hexrep[8192];

  // for CRT computation
  mpbarrett psubone, qsubone;

  // for testing
  mpnumber m, cipher, decipher;

  rsakp keypair;

  size_t bits = 2048;
  size_t pbits = (bits+1) >> 1;
  size_t qbits = (bits - pbits);
  size_t psize = MP_BITS_TO_WORDS(pbits+MP_WBITS-1);
  size_t qsize = MP_BITS_TO_WORDS(qbits+MP_WBITS-1);
  size_t pqsize = psize+qsize;
  mpw* temp = (mpw*) malloc((16*pqsize+6)*sizeof(mpw));

  if( argc < 2 ) {
    printf( "usage: %s <secblock>\n", argv[0] );
    exit( 1 );
  }
  
  mpbzero(&psubone);
  mpbzero(&qsubone);

  secblock = fopen(argv[1], "rb");
  if( secblock == NULL ) {
    printf( "Can't open %s\n", argv[1] );
    exit(0);
  }

  packetType = fgetc(secblock);
  packetLen = 0;
  //big endianness...
  big16read(&packetLen, secblock);
  
  printf( "Packet type: 0x%02X\n", packetType );
  printf( "Packet length: %04d\n", (int) packetLen );

  // skip ahead six bytes, this includes key generation time and other attributes
  fread( buffer, 6, 1, secblock);

  rsakpInit(&keypair);

  big16read(&packetLen, secblock);
  printf( "n Packet length: %02d bits, %02d bytes\n", (int) packetLen, (int) bytesFromMpn(packetLen) );
  fread( buffer, bytesFromMpn(packetLen), 1, secblock );
  mpbsetbin(&keypair.n, buffer, bytesFromMpn(packetLen));
  mpprintln(packetLen/32, keypair.n.modl);

  big16read(&packetLen, secblock);
  printf( "e Packet length: %02d bits, %02d bytes\n", (int) packetLen, (int) bytesFromMpn(packetLen) );
  fread( buffer, bytesFromMpn(packetLen), 1, secblock );
  mpnsetbin(&keypair.e, buffer, bytesFromMpn(packetLen));
  mpprintln(keypair.e.size, keypair.e.data);

  packetType = fgetc(secblock);
  if( packetType == 0 ) {
    printf( "secret data is plaintext\n" );
  } else {
    printf( "secret data is encrypted\n" );
  }
  
  big16read(&packetLen, secblock);
  printf( "d Packet length: %02d bits, %02d bytes\n", (int) packetLen, (int) bytesFromMpn(packetLen) );
  fread( buffer, bytesFromMpn(packetLen), 1, secblock );
  mpnsetbin(&keypair.d, buffer, bytesFromMpn(packetLen));
  mpprintln(keypair.d.size, keypair.d.data);
  
  big16read(&packetLen, secblock);
  printf( "p Packet length: %02d bits, %02d bytes\n", (int) packetLen, (int) bytesFromMpn(packetLen) );
  fread( buffer, bytesFromMpn(packetLen), 1, secblock );
  mpbsetbin(&keypair.p, buffer, bytesFromMpn(packetLen));
  mpprintln(packetLen/32, keypair.p.modl);
  
  big16read(&packetLen, secblock);
  printf( "q Packet length: %02d bits, %02d bytes\n", (int) packetLen, (int) bytesFromMpn(packetLen) );
  fread( buffer, bytesFromMpn(packetLen), 1, secblock );
  mpbsetbin(&keypair.q, buffer, bytesFromMpn(packetLen));
  mpprintln(packetLen/32, keypair.q.modl);

#ifdef USE_CRT
  // compute CRT elements
  /* compute p-1 */
  mpbsubone(&keypair.p, temp);
  mpbset(&psubone, psize, temp);

  /* compute q-1 */
  mpbsubone(&keypair.q, temp);
  mpbset(&qsubone, qsize, temp);

  /* compute dp = d mod (p-1) */
  mpnsize(&keypair.dp, psize);
  mpbmod_w(&psubone, keypair.d.data, keypair.dp.data, temp);

  /* compute dq = d mod (q-1) */
  mpnsize(&keypair.dq, qsize);
  mpbmod_w(&qsubone, keypair.d.data, keypair.dq.data, temp);

  /* compute qi = inv(q) mod p */
  mpninv(&keypair.qi, (mpnumber*) &keypair.q, (mpnumber*) &keypair.p);
#endif

  // now test
  mpnzero(&m);
  mpnzero(&cipher);
  mpnzero(&decipher);

  mpnsethex(&m, "d436e99569fd32a7c8a05bbc90d32c49");
  printf( "Original: " );
  mpprintln(m.size, m.data);
  
  rsapub(&keypair.n, &keypair.e, &m, &cipher);

  printf( "Encrypted: " );
  mpprintln(cipher.size, cipher.data);

#ifdef USE_CRT
  rsapricrt(&keypair.n, &keypair.p, &keypair.q, &keypair.dp, &keypair.dq, &keypair.qi, &cipher, &decipher);
#else
  rsapri(&keypair.n, &keypair.d, &cipher, &decipher);
#endif

  printf( "Recovered: " );
  mpprintln(decipher.size, decipher.data);

  if (mpnex(m.size, m.data, decipher.size, decipher.data))
    printf ( "results don't match\n" );
  else
    printf ( "before and after encyrption sizes match\n" );

  free(temp);

  return 0;
}
