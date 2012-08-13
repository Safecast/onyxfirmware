#include "utils.h"
#include <stdint.h>

void int_to_char(uint32_t number,char *text,uint32_t width) {

  if(number == 0) {
    for(uint32_t n=0;n<width;n++) text[n]=' ';
    text[0]='0';
    text[width]=0;
    return;
  }

  uint32_t current=1000000;
  int position=0;
  for(;position<7;position++) {
    int v = number/current;
    text[position] = '0'+v;
    number = number - (v*current);
    current=current/10;
  }
  text[position]=0;

  int last_leading_zero=-1;
  for(uint32_t n=0;n<50;n++) {
    if(text[n]=='0') {
      last_leading_zero=n;
    } else break;
  }

  if(last_leading_zero != -1) {
    int m=0;
    for(uint32_t n=last_leading_zero+1;n<=position;n++) {
      text[m] = text[n];
      m++;
    }
    position=m;
    text[position+1]=0;
  }

  if(width != -1) {

    text[width] = 0;
    for(uint32_t n=0;n<width;n++) {
      if(text[n]==0) {
        for(uint32_t i=n;i<width;i++) text[i] = ' ';
        break;
      }
    }
  }

}

bool strcmp(const char *i,const char *j) {

  for(uint32_t n=0;;n++) {
    if(i[n] != j[n]) return false;
    if((i[n] == 0) && (j[n] == 0)) return true;
  }
}

int32_t strlen(const char *i) {

  for(uint32_t n=0;n<100;n++) {
    if(i[n] == 0) return n;
  }
  return 0;
}
