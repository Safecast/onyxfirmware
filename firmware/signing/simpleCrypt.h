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

typedef unsigned char octet;

struct privKeyInFlash {
  octet  header_n[11]; 
  octet  n[256];
  octet  header_e[2];
  octet  e[4];
  octet  header_d[2];
  octet  d[256];
  octet  header_p[2];
  octet  p[128];
  octet  header_q[2];
  octet  q[128];
  octet  spacer[134];
  char   keyname[16];
};

#define KEYDATABASE  (struct privKeyInFlash *)(0x08000800)
#define PUBDATABASE  (char *)(0x1000)
#define GUIDDATABASE (char *)(0x17F0)
