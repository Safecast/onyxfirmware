#include <stdio.h>
#include <string.h>
#include "simpleCrypt.h"

void dumpRec(opRec *oprec) {
  if( oprec->opType == CH_ENCRYPT ) 
    printf( "opType is CH_ENCRYPT\n" );
  if( oprec->opType == CH_DECRYPT ) 
    printf( "opType is CH_DECRYPT\n" );
  
  printf( "key index is %d\n", oprec->keyIndex );
  
  if( oprec->keyType == CH_SYM ) 
    printf( "keyType is CH_SYM\n" );
  if( oprec->keyType == CH_PK ) 
    printf( "keyType is CH_PK\n" );
  
  if( oprec->cipherType == CH_AES ) 
    printf( "cipherType is CH_AES\n" );
  if( oprec->cipherType == CH_SGN ) 
    printf( "cipherType is CH_SGN\n" );
  if( oprec->cipherType == CH_VRF ) 
    printf( "cipherType is CH_VRF\n" );
  
  printf( "data field is %d digits long with string: \n%s\n\n", oprec->dataLen, oprec->data );

}

void testParse() {
  opRec oprec;
  char dataRec[200];
  char string1[] = "e:4:AES:4567890123defabc\n\0";
  char string2[] = "d:0:SGN:000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f\n\0";

  
  oprec.data = dataRec;
  oprec.dataLen = 200;

  if( parseString(string1, &oprec) != 1) 
    printf( "failure on string1\n" );
  
  dumpRec(&oprec);
  
  oprec.dataLen = 200;
  if( parseString(string2, &oprec) != 1) 
    printf( "failure on string2\n" );
  
  dumpRec(&oprec);
  
}

int main() {
  testParse();
  return(0);
}
