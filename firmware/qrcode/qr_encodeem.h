#ifndef QRCODEEM_H
#define QRCODEEM_H

bool qr_encode_data(int nLevel, int nVersion,bool bAutoExtent, int nMaskingNo, const uint8_t * lpsSource, int ncSource,uint8_t *outputdata,int *outputdata_len,int *width);

void qr_dumpimage(uint8_t *image,int width);
int qr_getmodule(uint8_t *outputdata,int width,int x,int y);

#endif
