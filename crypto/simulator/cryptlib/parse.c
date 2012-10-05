#include <stdio.h>
#include <string.h>
#include "beecrypt/beecrypt.h"

#include "simpleCrypt.h"

#define isNum(x) ( (x >= '0') && (x <= '9') )

void parseError(syntaxEnum syntaxPt, char *s) {
  printf( "parseString(): Parse error at syntax point %d, %s\n", (int) syntaxPt, s );
}

#define PARSE_MAXNUM 16

// returns 1 on success
int parseString(char *cmd, opRec *oprec) {
  int parsePt = 0;
  syntaxEnum syntaxPt = CH_OPTYPE;
  int i, readLen;
  char numTmp[PARSE_MAXNUM];
  
  // discard all case information
  i = 0;
  while( cmd[i] != '\0' ) {
    cmd[i] = tolower(cmd[i]);
    i++;
  }
  
  while( cmd[parsePt] != '\0' ) {
    switch( syntaxPt ) {
    case CH_OPTYPE:
      if( cmd[parsePt] == 'e' ) oprec->opType = CH_ENCRYPT;
      else if( cmd[parsePt] == 'd' ) oprec->opType = CH_DECRYPT;
      else {
	parseError(syntaxPt, "unrecognized enc/dec token");
	return -1;
      }
      parsePt++;
      syntaxPt = CH_KEYINDEX;
      break;
      
      // this gets skipped now actually
    case CH_KEYTYPE:
      if( cmd[parsePt] == 's' ) oprec->keyType = CH_SYM;
      else if( cmd[parsePt] == 'p' ) oprec->keyType = CH_PK;
      else {
	parseError(syntaxPt, "unrecognized keytype token");
	return -1;
      }
      parsePt++;
      syntaxPt = CH_KEYINDEX;
      break;
      
    case CH_KEYINDEX:
      for( i = 0; i < PARSE_MAXNUM; i++ )
	numTmp[i] = '\0';
      i = 0;
      while( isNum(cmd[parsePt + i] ) ) {
	numTmp[i] = cmd[parsePt + i];
	i++;
      }
      numTmp[i] = '\0';
      oprec->keyIndex = atoi(numTmp);
      if( oprec->keyIndex > MAX_KEY_INDEX ) {
	parseError(syntaxPt, "keyIndex is out of bound" );
	return -1;
      }
      parsePt++;
      syntaxPt = CH_CTYPE;
      break;
      
    case CH_CTYPE:
      for( i = 0; i < 3; i++ ) {
	numTmp[i] = cmd[parsePt++];
      }
      numTmp[i] = '\0';
      if( strcmp( numTmp, "aes" ) == 0 ) { 
	oprec->cipherType = CH_AES; oprec->keyType = CH_SYM; 
      } else if( strcmp( numTmp, "sgn" ) == 0 ) { 
	oprec->cipherType = CH_SGN; oprec->keyType = CH_PK; 
      } else if( strcmp( numTmp, "vrf" ) == 0 ) { 
	oprec->cipherType = CH_VRF; oprec->keyType = CH_PK; 
      } else if( strcmp( numTmp, "sha" ) == 0 ) {
	oprec->cipherType = CH_SHA; oprec->keyType = CH_SYM;
      } else {
	parseError(syntaxPt, "cipher type is unrecognized");
	return -1;
      }
      syntaxPt = CH_DATA;
      break;
      
    case CH_DATA:
      readLen = 0;
      while( (cmd[parsePt] != '\0') && (cmd[parsePt] != '\n') ) {
	if( readLen > oprec->dataLen ) {
	  parseError(syntaxPt, "data length exceeds available buffer space");
	  return -1;
	}
	// check if this is a hex digit
	if( !isNum(cmd[parsePt]) && !(cmd[parsePt] <= 'f' && cmd[parsePt] >= 'a') ) {
	  parseError(syntaxPt, "non hex digits in data");
	  return -1;
	}
	(oprec->data)[readLen] = cmd[parsePt];
	parsePt++; readLen++;
      } // while
      (oprec->data)[readLen] = '\0';
      oprec->dataLen = readLen;
      return 1;
      break;

    default:
      fprintf(stderr, "Error in parseString()\n" );
      return(-1);
    } // switch syntaxPt
    if( (cmd[parsePt] != ':') ) {
      parseError(syntaxPt, "missing delimiter" );
      return -1;
    }
    parsePt++;
  }

  // we should never reach this point, a successful parse leaves function at CH_DATA case
  parseError(syntaxPt, "unknown error, late failure in function");
  return -1;
}

