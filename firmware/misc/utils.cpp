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

void float_to_char(float number,char *text,int32_t width) {

  float original_number=number;

  uint32_t current=1000000;
  int position = 0;
  uint32_t int_part = number;
  for(;position<7;position++) {
    int v = int_part/current;
    text[position] = '0'+v;
    int_part = int_part - (v*current);
    current=current/10;
  }
  text[position]='.';
  position++;

  current=100;
  number=original_number;
  uint32_t fractional_part = (number*(float)1000)- ((float)(((uint32_t)number)*1000)) ;
  for(;position<12;position++) {
    int v = fractional_part/current;
    text[position] = '0'+v;
    fractional_part = fractional_part - (v*current);
    current=current/10;
  }
  text[position]=0;

  int last_leading_zero=-1;
  for(uint32_t n=0;n<50;n++) {
    if(text[n]=='0') {
      last_leading_zero=n;
    } else break;
  }

  if(text[last_leading_zero+1] == '.') {text[last_leading_zero]='0'; last_leading_zero--;}

  if(last_leading_zero != -1) {
    int m=0;
    for(int32_t n=last_leading_zero+1;n<=position;n++) {
      text[m] = text[n];
      m++;
    }
    position=m;
    text[position+1]=0;
  }

  if(width != -1) {

    text[width] = 0;
    for(int32_t n=0;n<width;n++) {
      if(text[n]==0) {
        for(int32_t i=n;i<width;i++) text[i] = ' ';
        break;
      }
    }
  }

}

void int_to_char(uint32_t number,char *text,int32_t width) {

  if(number == 0) {
    for(int32_t n=0;n<width;n++) text[n]=' ';
    text[0]='0';
    text[width]=0;
    return;
  }

  uint32_t current=1000000000;
  int position=0;
  for(;position<10;position++) {
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
    for(int32_t n=last_leading_zero+1;n<=position;n++) {
      text[m] = text[n];
      m++;
    }
    position=m;
    text[position+1]=0;
  }

  if(width != -1) {

    text[width] = 0;
    for(int32_t n=0;n<width;n++) {
      if(text[n]==0) {
        for(int32_t i=n;i<width;i++) text[i] = ' ';
        break;
      }
    }
  }

}

bool strcmpl(const char *i,const char *j,uint32_t len) {
  for(uint32_t n=0;n<len;n++) {
    if(i[n] != j[n]) return false;
    if((i[n] == 0) && (j[n] == 0)) return true;
  }
  return true;
}
