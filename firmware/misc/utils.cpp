#include "utils.h"
#include <stdint.h>

uint32_t str_to_uint(const char *text) {

  uint8_t digits[10];

  uint32_t size=0;
  for(size=0;size<10;size++) {
    if((text[size] >= '0') && (text[size] <= '9')) {
      digits[size] = text[size]-'0';
    } else break;
  }
  if(size != 0) size--;

  uint32_t place=1;
  uint32_t value=0;
  for(int32_t i=size;i>=0;i--) {
    value += digits[i]*place;
    place=place*10;
  }

  return value;
}

bool strcmpl(const char *i,const char *j,uint32_t len) {
  for(uint32_t n=0;n<len;n++) {
    if(i[n] != j[n]) return false;
    if((i[n] == 0) && (j[n] == 0)) return true;
  }
  return true;
}
