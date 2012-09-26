#ifndef QR_UTILS_H
#define QR_UTILS_H
#include <stdint.h>

bool IsKanjiData(unsigned char c1, unsigned char c2);
bool IsNumeralData(unsigned char c);
bool IsAlphabetData(unsigned char c);
int GetBitLength(uint8_t nMode, int ncData, int nVerGroup);
uint8_t AlphabetToBinary(unsigned char c);
uint16_t KanjiToBinary(uint16_t wc);
#endif
