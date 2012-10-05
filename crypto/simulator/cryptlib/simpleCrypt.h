typedef struct {
  char *rsa_n; // public
  char *rsa_e; // public
  char *rsa_p; // private
  char *rsa_q; // private
  char *rsa_dp;  // private
  char *rsa_dq;  // private
  char *rsa_qi;  // private
} RSAkey;

typedef struct {
  byte key[32];
} AESkey;

/* keyfile format

 [keyindex]:[key in ASCII hex digits][\n]

 each key can be of variable length, delimited by the \n

 ultimately these keys will be stored in local (on-chip) memory so the file format is temporary
 
 the memory storage format will be :

 array of 128-bit entries for symmetric keys
 array of struct RSArec for public keys

*/

/* i/o format

 [d/e]:[keypair index]:[cipher]:[hexdigits]\n

 responds with:

 [hexdigits]\n

 [keypair index] can take the form of:
 0-9. each cipher (AES or RSA) has its own key index
 
 [cipher] can be any of these values:
 AES -- performs a simple AES encryption
 SGN -- hashes and signs the message with the private key for Pn (enc mode)
        encrypts a message with the public key for Pn (dec mode)
 VRF -- decrypts message with private key for Pn

 ==future commands==
 ST1 -- decrypts message with private key for Pn, but does not print cleartext message
 ST2 -- use previously decrypted message to set symmetric key specified by keypair index in this command

*/

typedef enum { CH_ENCRYPT = 0, CH_DECRYPT } opTypeEnum;
typedef enum { CH_SYM = 0, CH_PK } keyTypeEnum;
typedef enum { CH_AES = 0, CH_SGN, CH_VRF, CH_SHA } cipherTypeEnum;
typedef enum { CH_OPTYPE = 0, CH_KEYTYPE, CH_KEYINDEX, CH_CTYPE, CH_DATA } syntaxEnum;

#define MAX_KEY_INDEX 9

typedef struct {
  opTypeEnum     opType;
  unsigned int keyIndex;
  keyTypeEnum    keyType;
  cipherTypeEnum cipherType;
  unsigned int dataLen;
  unsigned char *data;
}  opRec;

