#include "beecrypt/sha1.h"
#include "beecrypt/aes.h"
#include "beecrypt/rsa.h"
#include "beecrypt/fips186.h"
#include "beecrypt/entropy.h"
#include "simpleCrypt.h"
#include "ctype.h"

#include "usart.h"

void serial_write_string(const char *str) {
  unsigned int n;
  for(n=0; str[n] != 0 ;n++) {
    usart_putc(USART1, str[n]);
  }
}

void print_mp(size_t size, const mpw* data)
{
  char stemp[64];
  int i;

  if (data == (mpw*) 0)
    return;
  if( size == 0 )
    return;

  i = 0;
  while (size--)
    {
      if( (i % 16 == 0) && (i != 0))
        serial_write_string("\r\n");
      sprintf(stemp, "%08x", *(data++));
      serial_write_string(stemp);
      i++;
    }
  serial_write_string("\r\n");
}

void print_mp_nocrlf(size_t size, const mpw* data)
{
  char stemp[64];
  int i;

  if (data == (mpw*) 0)
    return;
  if( size == 0 )
    return;

  i = 0;
  while (size--)
    {
      sprintf(stemp, "%08x", *(data++));
      serial_write_string(stemp);
      i++;
    }
}

void signing_test() {
  sha1Param param;
  char testStr[] = "Good evening gentlemen.\n";
  byte  hash_str[20];
  mpnumber hash_mpn, signed_hash, decipher;
  rsakp keypair;
  struct privKeyInFlash *kdat = KEYDATABASE;

  // just a quick routine to exercise the RSA signing abilities and SHA-1 hashing
  serial_write_string("Initializing PK system.\r\n" );

  mpnzero(&hash_mpn);
  rsakpInit(&keypair);
  if( sha1Reset(&param) ) { serial_write_string( "FAIL" ); goto cleanup; }

  serial_write_string("Computing SHA-1 hash of: " );
  serial_write_string(testStr);
  serial_write_string("\r\n");
  if( sha1Update(&param, (byte *) testStr, 24 ) ) { serial_write_string( "FAIL" ); goto cleanup; }
  if( sha1Digest(&param, hash_str) ) { serial_write_string( "FAIL" ); goto cleanup; }
  if(mpnsetbin(&hash_mpn, (byte *) hash_str, (size_t) 20) != 0) { serial_write_string( "FAIL" ); goto cleanup; }
  if( sha1Reset(&param) ) { serial_write_string( "FAIL" ); goto cleanup; }

  serial_write_string("Hashed result to sign: " );
  print_mp(hash_mpn.size, hash_mpn.data);

  serial_write_string("Initializing key parameters for 2048-bit RSA.\r\n" );
  //  if( mpnsetbin(&keypair.e, kdat->e, 4) != 0 ) { serial_write_string( "FAIL" ); goto cleanup; }
  if( mpbsetbin(&keypair.n, kdat->n, 256) != 0) { serial_write_string( "FAIL" ); goto cleanup; }
  serial_write_string("n:\r\n" ); // n & e are the pubkey so okay to print this
  print_mp(256/4, keypair.n.modl);
  if( mpnsetbin(&keypair.d, kdat->d, 256) != 0 ) { serial_write_string( "FAIL" ); goto cleanup; }
  ///// don't print the private key for production!
  //  serial_write_string("d:\r\n" );
  //  print_mp(keypair.d.size, keypair.d.data);
  /////
  //  if( mpbsetbin(&keypair.p, kdat->p, 128) != 0) { serial_write_string( "FAIL" ); goto cleanup; }
  //  if( mpbsetbin(&keypair.q, kdat->q, 128) != 0) { serial_write_string( "FAIL" ); goto cleanup; }

  serial_write_string("Signing hash...\r\n" );
  mpnzero(&signed_hash);
  rsapri(&keypair.n, &keypair.d, &hash_mpn, &signed_hash);

  serial_write_string("Signature is (in hex):\r\n" );
  // at this point, signed_hash should be returned to the console as the 'signature'...we'll figure that out later
  print_mp(signed_hash.size, signed_hash.data);

  serial_write_string("Decrypting test hash...\r\n" );
  mpnzero(&decipher);
  rsapub(&keypair.n, &keypair.e, &signed_hash, &decipher);

  serial_write_string("Decrypted hash (should match original hash):\r\n" );
  print_mp(decipher.size, decipher.data);

  if (mpnex(hash_mpn.size, hash_mpn.data, decipher.size, decipher.data))
    serial_write_string ( "uu_pktest FAIL\r\n" );
  else
    serial_write_string ( "uu_pktest PASS\r\n" );

  serial_write_string("Done.\r\n" );

 cleanup: // dealloc anything that could have been alloc'd...
  mpnfree(&hash_mpn);
  mpnfree(&signed_hash);
  mpnfree(&decipher);
  rsakpFree(&keypair);
  return;
}

int signing_isKeyValid() {
  struct privKeyInFlash *kdat = KEYDATABASE;
  char validKey[] = "safecast per-device unique signing key";
  char invalidKey[] = "safecast TEST signing key (invalid for production use)"; // not used, but here for reference
  
  if( strncmp(validKey, kdat->keyname, 16) == 0 ) {
    return 1;
  } else {
    return 0;
  }
}

void signing_printPubKey() {
  serial_write_string(PUBDATABASE);
}

void signing_printGUID() {
  unsigned char *bin_guid = (unsigned char *)GUIDDATABASE;
  mpnumber guid;
  
  mpnzero(&guid);
  mpnsetbin(&guid, bin_guid, 16);
  print_mp_nocrlf(guid.size, guid.data);
  mpnfree(&guid);
}

// to do:
// function to do hash and private key signing using internal keys

void signing_hashLog() {
  sha1Param param;
  char *logBuf[5120];
  byte  hash_str[20];
  mpnumber hash_mpn, signed_hash;
  rsakp keypair;
  struct privKeyInFlash *kdat = KEYDATABASE;
  int i, len;

  unsigned char *bin_guid = (unsigned char *)GUIDDATABASE;
  mpnumber guid;
  int j;
  char *tbuf;

  if( signing_isKeyValid() == 0 ) {
    serial_write_string("WARNING: System uses a test private key. This signature is worthless and should be disregarded.\r\n");
  }
  // just a quick routine to exercise the RSA signing abilities and SHA-1 hashing
  mpnzero(&hash_mpn);
  rsakpInit(&keypair);
  if( sha1Reset(&param) ) { serial_write_string( "FAIL" ); goto cleanup; }

  // setup the log
  log_read_start();

  for( i = 0; i < 5120; i ++ ) {
    logBuf[i] = (char) 0;
  }
  // keep feeding data into the SHA-1 stream
  while( (len = log_read_block(logBuf)) != 0 ) {
    if( sha1Update(&param, (byte *)logBuf, len ) ) { serial_write_string( "FAIL" ); goto cleanup; }

    for( i = 0; i < 5120; i ++ ) {
      logBuf[i] = (char) 0;
    }
  };

  // compute the final SHA-1 value (the 'digest')
  if( sha1Digest(&param, hash_str) ) { serial_write_string( "FAIL" ); goto cleanup; }
  if(mpnsetbin(&hash_mpn, (byte *) hash_str, (size_t) 20) != 0) { serial_write_string( "FAIL" ); goto cleanup; }
  if( sha1Reset(&param) ) { serial_write_string( "FAIL" ); goto cleanup; }

  // print the hash for diagnostic purposes
  serial_write_string("{\"hash\":\"");
  print_mp_nocrlf(hash_mpn.size, hash_mpn.data);
  serial_write_string("\",\r\n");

  // set private key params and encrypt the hash
  if( mpbsetbin(&keypair.n, kdat->n, 256) != 0) { serial_write_string( "FAIL" ); goto cleanup; }
  if( mpnsetbin(&keypair.d, kdat->d, 256) != 0 ) { serial_write_string( "FAIL" ); goto cleanup; }

  mpnzero(&signed_hash);
  rsapri(&keypair.n, &keypair.d, &hash_mpn, &signed_hash);

  // print the encrypted hash
  serial_write_string("\"signature\":\"");
  print_mp_nocrlf(signed_hash.size, signed_hash.data);
  serial_write_string("\",\r\n");

  serial_write_string("\"guid\":\"");
  mpnzero(&guid);
  mpnsetbin(&guid, bin_guid, 16);
  print_mp_nocrlf(guid.size, guid.data);
  serial_write_string("\",\r\n");
  mpnfree(&guid);

  serial_write_string("\"pubkey\":\"");
  j = 0;
  tbuf = PUBDATABASE;
  while( tbuf[j] != '\0' ) {
    if( tbuf[j] != '\n' && isascii(tbuf[j]) )
      usart_putc(USART1, tbuf[j]);
    else
      serial_write_string("\\n");
    j++;
  }
  serial_write_string("\"}\r\n");

 cleanup: // dealloc anything that could have been alloc'd...
  mpnfree(&hash_mpn);
  mpnfree(&signed_hash);
  rsakpFree(&keypair);
  return;

}
