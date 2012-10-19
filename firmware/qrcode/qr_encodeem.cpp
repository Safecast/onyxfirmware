#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "qr_encodeem.h"
#include "qr_utils.h"
#include <iostream>

using namespace std;

#define MAX_INPUTDATA  1024 //3096 // maximum input data size
#define MAX_QRCODESIZE 1024 //4096 // (177*177)/8

#define MAX_CODEBLOCK   153 // ブロックデータコードワード数最大値(ＲＳコードワードを含む)
#define MAX_MODULESIZE    177 // 一辺モジュール数最大値

bool qr_encode_source_data(const uint8_t* lpsSource,uint8_t *m_byDataCodeWord,int *outputdata_len,int ncLength, int nVerGroup);
int qr_encode_with_version(int nVersion,int level,const uint8_t* lpsSource, int ncLength,uint8_t *outputdata,int *outputdata_len);
int SetBitStream(uint8_t *codestream, int nIndex, uint16_t wData, int ncData);
void GetRSCodeWord(uint8_t *lpbyRSWork, int ncDataCodeWord, int ncRSCodeWord);
void SetFinderPattern(uint8_t *image,int width,int x, int y);
void FormatModule(uint8_t *image,int width,uint8_t *input_data,int input_data_len,int m_nMaskingNo,int version,int level);
void SetFunctionModule(uint8_t *image,int width,int version);
void SetCodeWordPattern(uint8_t *image,int width,uint8_t *encoded_data,int encoded_data_size,int version);
void SetMaskingPattern(uint8_t *image,int width,int nPatternNo,int version);
void SetFormatInfoPattern(uint8_t *image,int width,int nPatternNo,int level);
void SetVersionPattern(uint8_t *image,int width);
void SetAlignmentPattern(uint8_t *image,int width, int x, int y);
void SetVersionPattern(uint8_t *image,int width,int version);
//int CountPenalty(uint8_t *image,int width);
bool is_on_finder_pattern(int width,int x,int y);
bool is_on_deadarea(int width,int x,int y);
bool is_on_timing(int width,int x,int y);

void qr_setmodule(uint8_t *image,int width,int x,int y,int value) {
  int bitpos = ((y*width)+x);

  int byte = bitpos/8;
  int bit  = bitpos%8;

  if(value != 0) image[byte] = image[byte] | (1<<bit);
            else image[byte] = image[byte] & (~(1<<bit));
}

int qr_getmodule(uint8_t *outputdata,int width,int x,int y) {

 // if(x>width) return 0;
 // if(y>width) return 0;

  int bitpos = ((y*width)+x);

  int byte = bitpos/8;
  int bit  = bitpos%8;

  if(outputdata[byte] & (1<<bit)) {return 1;}
                             else {return 0;}
}

int qr_getmoduleC(uint8_t *outputdata,int width,int x,int y) {
  if(is_on_timing(width,x,y)) return 1;
  if(is_on_deadarea(width,x,y)) return 1;
  if(is_on_finder_pattern(width,x,y)) return 1;
  return 0;
}

// qr_encode_data
// 用  途：データエンコード
// 引  数：誤り訂正レベル、型番(0=自動)、型番自動拡張フラグ、マスキング番号(-1=自動)、エンコードデータ、エンコードデータ長
// 戻り値：エンコード成功時=true、データなし、または容量オーバー時=false
// nLevel    : Level, the ammount of redundancy in the QR code
// nVersion  : Specifies the size of the QR code, 0 means auto select.
// nMaskingNo: Something to do with data masking (see wikipedia)
// lpsSource : Source data
// ncSource  : Source data length, if 0 then assume NULL terminated.
bool qr_encode_data(int nLevel, int nVersion,bool bAutoExtent, int nMaskingNo, const uint8_t * lpsSource, int ncSource,uint8_t *outputdata,int *outputdata_len,int *width) {

  // If negative masking number, we need to find the mask with the best penalty
/*
  if(nMaskingNo == -1) {
    int min_penalty = 100000000;
		for(int n=0;n<=7;n++) {
			uint8_t outputdata[4096];
			int width;
			bool ok = qr_encode_data(nLevel,nVersion,bAutoExtent,n,lpsSource,ncSource,outputdata,outputdata_len,&width);
			
      int penalty = CountPenalty(outputdata,width);
      if(penalty < min_penalty) { min_penalty = penalty; nMaskingNo = n;}
		}
  }
*/

	int i, j;
  
  uint8_t m_byDataCodeWord[MAX_INPUTDATA];
  int     m_ncDataCodeWordBit;

	int m_nMaskingNo = nMaskingNo;

	// データ長が指定されていない場合は lstrlen によって取得
	int ncLength;
  if(ncSource > 0) { ncLength = ncSource; }
             else  { ncLength = strlen((const char *)lpsSource); }

	if (ncLength == 0) {
		return 1; // データなし
  }

  // Version Check
	// バージョン(型番)チェック
	//int nEncodeVersion = GetEncodeVersion(nVersion, lpsSource, ncLength);
  int nEncodeVersion = qr_encode_with_version(nVersion,nLevel,lpsSource,ncLength,m_byDataCodeWord,&m_ncDataCodeWordBit);

	if (nEncodeVersion == 0) {
		return 2; // 容量オーバー
  }
  int m_nVersion;
	if (nVersion == 0)
	{
		// 型番自動
		m_nVersion = nEncodeVersion;
	}
	else
	{
		if (nEncodeVersion <= nVersion)
		{
			m_nVersion = nVersion;
		}
		else
		{
			if (bAutoExtent)
				m_nVersion = nEncodeVersion; // バージョン(型番)自動拡張
			else
				return 3; // 容量オーバー
		}
	}

  // Terminator Code "0000"
	// ターミネータコード"0000"付加
	int ncDataCodeWord = QR_VersionInfo[m_nVersion].ncDataCodeWord[nLevel];

	int ncTerminater = std::min(4, (ncDataCodeWord * 8) - m_ncDataCodeWordBit);

	if (ncTerminater > 0)
		m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, 0, ncTerminater);

	// パディングコード"11101100, 00010001"付加
	uint8_t byPaddingCode = 0xec;

	for (i = (m_ncDataCodeWordBit + 7) / 8; i < ncDataCodeWord; ++i)
	{
		m_byDataCodeWord[i] = byPaddingCode;

		byPaddingCode = (uint8_t)(byPaddingCode == 0xec ? 0x11 : 0xec);
	}

	// 総コードワード算出エリアクリア
  int m_ncAllCodeWord; // 総コードワード数(ＲＳ誤り訂正データを含む)
  uint8_t m_byAllCodeWord[MAX_INPUTDATA]; // 総コードワード算出エリア

	m_ncAllCodeWord = QR_VersionInfo[m_nVersion].ncAllCodeWord;
  memset(m_byAllCodeWord,0,m_ncAllCodeWord);

	int nDataCwIndex = 0; // データコードワード処理位置

	// データブロック分割数
	int ncBlock1 = QR_VersionInfo[m_nVersion].RS_BlockInfo1[nLevel].ncRSBlock;
	int ncBlock2 = QR_VersionInfo[m_nVersion].RS_BlockInfo2[nLevel].ncRSBlock;
	int ncBlockSum = ncBlock1 + ncBlock2;

	int nBlockNo = 0; // 処理中ブロック番号

	// ブロック別データコードワード数
	int ncDataCw1 = QR_VersionInfo[m_nVersion].RS_BlockInfo1[nLevel].ncDataCodeWord;
	int ncDataCw2 = QR_VersionInfo[m_nVersion].RS_BlockInfo2[nLevel].ncDataCodeWord;

	// データコードワードインターリーブ配置
	for (i = 0; i < ncBlock1; ++i)
	{
		for (j = 0; j < ncDataCw1; ++j)
		{
			m_byAllCodeWord[(ncBlockSum * j) + nBlockNo] = m_byDataCodeWord[nDataCwIndex++];
		}

		++nBlockNo;
	}

	for (i = 0; i < ncBlock2; ++i)
	{
		for (j = 0; j < ncDataCw2; ++j)
		{
			if (j < ncDataCw1)
			{
				m_byAllCodeWord[(ncBlockSum * j) + nBlockNo] = m_byDataCodeWord[nDataCwIndex++];
			}
			else
			{
				// ２種目ブロック端数分配置
				m_byAllCodeWord[(ncBlockSum * ncDataCw1) + i]  = m_byDataCodeWord[nDataCwIndex++];
			}	
		}

		++nBlockNo;
	}

	// ブロック別ＲＳコードワード数(※現状では同数)
	int ncRSCw1 = QR_VersionInfo[m_nVersion].RS_BlockInfo1[nLevel].ncAllCodeWord - ncDataCw1;
	int ncRSCw2 = QR_VersionInfo[m_nVersion].RS_BlockInfo2[nLevel].ncAllCodeWord - ncDataCw2;

	/////////////////////////////////////////////////////////////////////////
	// ＲＳコードワード算出

	nDataCwIndex = 0;
	nBlockNo = 0;

  uint8_t m_byRSWork[MAX_CODEBLOCK]; // ＲＳコードワード算出ワーク
	for (i = 0; i < ncBlock1; ++i)
	{

		memset(m_byRSWork,0,sizeof(m_byRSWork));

		memmove(m_byRSWork, m_byDataCodeWord + nDataCwIndex, ncDataCw1);

		GetRSCodeWord(m_byRSWork, ncDataCw1, ncRSCw1);

		// ＲＳコードワード配置
		for (j = 0; j < ncRSCw1; ++j)
		{
			m_byAllCodeWord[ncDataCodeWord + (ncBlockSum * j) + nBlockNo] = m_byRSWork[j];
		}

		nDataCwIndex += ncDataCw1;
		++nBlockNo;
	}

	for (i = 0; i < ncBlock2; ++i)
	{
		memset(m_byRSWork,0,sizeof(m_byRSWork));

		memmove(m_byRSWork, m_byDataCodeWord + nDataCwIndex, ncDataCw2);

		GetRSCodeWord(m_byRSWork, ncDataCw2, ncRSCw2);

		// ＲＳコードワード配置
		for (j = 0; j < ncRSCw2; ++j)
		{
			m_byAllCodeWord[ncDataCodeWord + (ncBlockSum * j) + nBlockNo] = m_byRSWork[j];
		}

		nDataCwIndex += ncDataCw2;
		++nBlockNo;
	}

	*width = m_nVersion * 4 + 17;


  // Up until here we've just been reorganising the input data. We now start drawing the QRCode image.

	// モジュール配置
	FormatModule(outputdata,*width,m_byAllCodeWord,m_ncAllCodeWord,nMaskingNo,m_nVersion,nLevel);

	return 0;
}


// GetEncodeVersion
// qr_encode_with_version, this does the basic encoding, with a specified version (if possible)
// 用  途：エンコード時バージョン(型番)取得
// 引  数：調査開始バージョン、エンコードデータ、エンコードデータ長
// 戻り値：バージョン番号（容量オーバー時=0）
int qr_encode_with_version(int nVersion,int level,const uint8_t* lpsSource, int ncLength,uint8_t *outputdata,int *outputdata_len) {

  int &m_ncDataCodeWordBit = *outputdata_len;
  int m_nLevel = level;

	int nVerGroup = nVersion >= 27 ? QR_VRESION_L : (nVersion >= 10 ? QR_VRESION_M : QR_VRESION_S);
	int i, j;

  // try different versions in order?
	for (i = nVerGroup; i <= QR_VRESION_L; ++i)
	{
		if (qr_encode_source_data(lpsSource,outputdata,outputdata_len, ncLength, i))
		{
			if (i == QR_VRESION_S)
			{
				for (j = 1; j <= 9; ++j)
				{
					if ((m_ncDataCodeWordBit + 7) / 8 <= QR_VersionInfo[j].ncDataCodeWord[m_nLevel])
						return j;
				}
			}
			else if (i == QR_VRESION_M)
			{
				for (j = 10; j <= 26; ++j)
				{
					if ((m_ncDataCodeWordBit + 7) / 8 <= QR_VersionInfo[j].ncDataCodeWord[m_nLevel])
						return j;
				}
			}
			else if (i == QR_VRESION_L)
			{
				for (j = 27; j <= 40; ++j)
				{
					if ((m_ncDataCodeWordBit + 7) / 8 <= QR_VersionInfo[j].ncDataCodeWord[m_nLevel])
						return j;
				}
			}
		}
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetBitStream
// 用  途：ビットセット
// 引  数：挿入位置、ビット配列データ、データビット長(最大16)
// 戻り値：次回挿入位置(バッファオーバー時=-1)
// 備  考：m_byDataCodeWord に結果をセット(要ゼロ初期化)

int SetBitStream(uint8_t *codestream, int nIndex, uint16_t wData, int ncData) {
	int i;

	if (nIndex == -1 || nIndex + ncData > MAX_INPUTDATA * 8)
		return -1;

	for (i = 0; i < ncData; ++i)
	{
		if (wData & (1 << (ncData - i - 1)))
		{
			codestream[(nIndex + i) / 8] |= 1 << (7 - ((nIndex + i) % 8));
		}
	}

	return nIndex + ncData;
}



/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::EncodeSourceData
// 用  途：入力データエンコード
// 引  数：入力データ、入力データ長、バージョン(型番)グループ
// 戻り値：エンコード成功時=true

// This actually does the main data encoding.
bool qr_encode_source_data(const uint8_t* lpsSource,uint8_t *m_byDataCodeWord,int *outputdata_len,int ncLength, int nVerGroup) {
  int &m_ncDataCodeWordBit = *outputdata_len; // データコードワードビット長 (data code bit)

  int32_t m_nBlockLength[MAX_INPUTDATA];
  uint8_t m_byBlockMode [MAX_INPUTDATA];
//	uint8_t m_byDataCodeWord[MAX_INPUTDATA]; // 入力データエンコードエリア data encode area

	int m_ncDataBlock;

	memset(m_nBlockLength,0,sizeof(m_nBlockLength));
	memset(m_byBlockMode  ,5,sizeof(m_byBlockMode));

	int i, j;

	// どのモードが何文字(バイト)継続しているかを調査
	for (m_ncDataBlock = i = 0; i < ncLength; ++i) {
		uint8_t byMode;

		if (i < ncLength - 1 && IsKanjiData(lpsSource[i], lpsSource[i + 1]))
			byMode = QR_MODE_KANJI;
		else if (IsNumeralData(lpsSource[i]))
			byMode = QR_MODE_NUMERAL;
		else if (IsAlphabetData(lpsSource[i]))
			byMode = QR_MODE_ALPHABET;
		else
			byMode = QR_MODE_8BIT;

		if (i == 0) { m_byBlockMode[0] = byMode; }
 
		if (m_byBlockMode[m_ncDataBlock] != byMode)
			m_byBlockMode[++m_ncDataBlock] = byMode;
 
		++m_nBlockLength[m_ncDataBlock];

		if (byMode == QR_MODE_KANJI)
		{
			// 漢字は文字数ではなく	数で記録
			++m_nBlockLength[m_ncDataBlock];
			++i;
		}
	}

	++m_ncDataBlock;

	/////////////////////////////////////////////////////////////////////////
	// 隣接する英数字モードブロックと数字モードブロックの並びをを条件により結合

	int ncSrcBits, ncDstBits; // 元のビット長と単一の英数字モードブロック化した場合のビット長

	int nBlock = 0;

	while (nBlock < m_ncDataBlock - 1)
	{
		int ncJoinFront, ncJoinBehind; // 前後８ビットバイトモードブロックと結合した場合のビット長
		int nJoinPosition = 0; // ８ビットバイトモードブロックとの結合：-1=前と結合、0=結合しない、1=後ろと結合

		// 「数字−英数字」または「英数字−数字」の並び
		if ((m_byBlockMode[nBlock] == QR_MODE_NUMERAL  && m_byBlockMode[nBlock + 1] == QR_MODE_ALPHABET) ||
			(m_byBlockMode[nBlock] == QR_MODE_ALPHABET && m_byBlockMode[nBlock + 1] == QR_MODE_NUMERAL))
		{
			// 元のビット長と単一の英数字モードブロック化した場合のビット長を比較
			ncSrcBits = GetBitLength(m_byBlockMode[nBlock], m_nBlockLength[nBlock], nVerGroup) +
						GetBitLength(m_byBlockMode[nBlock + 1], m_nBlockLength[nBlock + 1], nVerGroup);

			ncDstBits = GetBitLength(QR_MODE_ALPHABET, m_nBlockLength[nBlock] + m_nBlockLength[nBlock + 1], nVerGroup);

			if (ncSrcBits > ncDstBits)
			{
				// 前後に８ビットバイトモードブロックがある場合、それらとの結合が有利かどうかをチェック
				if (nBlock >= 1 && m_byBlockMode[nBlock - 1] == QR_MODE_8BIT)
				{
					// 前に８ビットバイトモードブロックあり
					ncJoinFront = GetBitLength(QR_MODE_8BIT, m_nBlockLength[nBlock - 1] + m_nBlockLength[nBlock], nVerGroup) +
								  GetBitLength(m_byBlockMode[nBlock + 1], m_nBlockLength[nBlock + 1], nVerGroup);

					if (ncJoinFront > ncDstBits + GetBitLength(QR_MODE_8BIT, m_nBlockLength[nBlock - 1], nVerGroup))
						ncJoinFront = 0; // ８ビットバイトモードブロックとは結合しない
				}
				else
					ncJoinFront = 0;

				if (nBlock < m_ncDataBlock - 2 && m_byBlockMode[nBlock + 2] == QR_MODE_8BIT)
				{
					// 後ろに８ビットバイトモードブロックあり
					ncJoinBehind = GetBitLength(m_byBlockMode[nBlock], m_nBlockLength[nBlock], nVerGroup) +
								   GetBitLength(QR_MODE_8BIT, m_nBlockLength[nBlock + 1] + m_nBlockLength[nBlock + 2], nVerGroup);

					if (ncJoinBehind > ncDstBits + GetBitLength(QR_MODE_8BIT, m_nBlockLength[nBlock + 2], nVerGroup))
						ncJoinBehind = 0; // ８ビットバイトモードブロックとは結合しない
				}
				else
					ncJoinBehind = 0;

				if (ncJoinFront != 0 && ncJoinBehind != 0)
				{
					// 前後両方に８ビットバイトモードブロックがある場合はデータ長が短くなる方を優先
					nJoinPosition = (ncJoinFront < ncJoinBehind) ? -1 : 1;
				}
				else
				{
					nJoinPosition = (ncJoinFront != 0) ? -1 : ((ncJoinBehind != 0) ? 1 : 0);
				}

				if (nJoinPosition != 0)
				{
					// ８ビットバイトモードブロックとの結合
					if (nJoinPosition == -1)
					{
						m_nBlockLength[nBlock - 1] += m_nBlockLength[nBlock];

						// 後続をシフト
						for (i = nBlock; i < m_ncDataBlock - 1; ++i)
						{
							m_byBlockMode[i]  = m_byBlockMode[i + 1];
							m_nBlockLength[i] = m_nBlockLength[i + 1];
						}
					}
					else
					{
						m_byBlockMode[nBlock + 1] = QR_MODE_8BIT;
						m_nBlockLength[nBlock + 1] += m_nBlockLength[nBlock + 2];

						// 後続をシフト
						for (i = nBlock + 2; i < m_ncDataBlock - 1; ++i)
						{
							m_byBlockMode[i]  = m_byBlockMode[i + 1];
							m_nBlockLength[i] = m_nBlockLength[i + 1];
						}
					}

					--m_ncDataBlock;
				}
				else
				{
					// 英数字と数字の並びを単一の英数字モードブロックに統合

					if (nBlock < m_ncDataBlock - 2 && m_byBlockMode[nBlock + 2] == QR_MODE_ALPHABET)
					{
						// 結合しようとするブロックの後ろに続く英数字モードブロックを結合
						m_nBlockLength[nBlock + 1] += m_nBlockLength[nBlock + 2];

						// 後続をシフト
						for (i = nBlock + 2; i < m_ncDataBlock - 1; ++i)
						{
							m_byBlockMode[i]  = m_byBlockMode[i + 1];
							m_nBlockLength[i] = m_nBlockLength[i + 1];
						}

						--m_ncDataBlock;
					}

					m_byBlockMode[nBlock] = QR_MODE_ALPHABET;
					m_nBlockLength[nBlock] += m_nBlockLength[nBlock + 1];

					// 後続をシフト
					for (i = nBlock + 1; i < m_ncDataBlock - 1; ++i)
					{
						m_byBlockMode[i]  = m_byBlockMode[i + 1];
						m_nBlockLength[i] = m_nBlockLength[i + 1];
					}

					--m_ncDataBlock;

					if (nBlock >= 1 && m_byBlockMode[nBlock - 1] == QR_MODE_ALPHABET)
					{
						// 結合したブロックの前の英数字モードブロックを結合
						m_nBlockLength[nBlock - 1] += m_nBlockLength[nBlock];

						// 後続をシフト
						for (i = nBlock; i < m_ncDataBlock - 1; ++i)
						{
							m_byBlockMode[i]  = m_byBlockMode[i + 1];
							m_nBlockLength[i] = m_nBlockLength[i + 1];
						}

						--m_ncDataBlock;
					}
				}

				continue; // 現在位置のブロックを再調査
			}
		}

		++nBlock; // 次ブロックを調査
	}

	/////////////////////////////////////////////////////////////////////////
	// 連続する短いモードブロックを８ビットバイトモードブロック化

	nBlock = 0;

	while (nBlock < m_ncDataBlock - 1)
	{
		ncSrcBits = GetBitLength(m_byBlockMode[nBlock], m_nBlockLength[nBlock], nVerGroup)
					+ GetBitLength(m_byBlockMode[nBlock + 1], m_nBlockLength[nBlock + 1], nVerGroup);

		ncDstBits = GetBitLength(QR_MODE_8BIT, m_nBlockLength[nBlock] + m_nBlockLength[nBlock + 1], nVerGroup);

		// 前に８ビットバイトモードブロックがある場合、重複するインジケータ分を減算
		if (nBlock >= 1 && m_byBlockMode[nBlock - 1] == QR_MODE_8BIT)
			ncDstBits -= (4 + nIndicatorLen8Bit[nVerGroup]);

		// 後ろに８ビットバイトモードブロックがある場合、重複するインジケータ分を減算
		if (nBlock < m_ncDataBlock - 2 && m_byBlockMode[nBlock + 2] == QR_MODE_8BIT)
			ncDstBits -= (4 + nIndicatorLen8Bit[nVerGroup]);

		if (ncSrcBits > ncDstBits)
		{
			if (nBlock >= 1 && m_byBlockMode[nBlock - 1] == QR_MODE_8BIT)
			{
				// 結合するブロックの前にある８ビットバイトモードブロックを結合
				m_nBlockLength[nBlock - 1] += m_nBlockLength[nBlock];

				// 後続をシフト
				for (i = nBlock; i < m_ncDataBlock - 1; ++i)
				{
					m_byBlockMode[i]  = m_byBlockMode[i + 1];
					m_nBlockLength[i] = m_nBlockLength[i + 1];
				}

				--m_ncDataBlock;
				--nBlock;
			}

			if (nBlock < m_ncDataBlock - 2 && m_byBlockMode[nBlock + 2] == QR_MODE_8BIT)
			{
				// 結合するブロックの後ろにある８ビットバイトモードブロックを結合
				m_nBlockLength[nBlock + 1] += m_nBlockLength[nBlock + 2];

				// 後続をシフト
				for (i = nBlock + 2; i < m_ncDataBlock - 1; ++i)
				{
					m_byBlockMode[i]  = m_byBlockMode[i + 1];
					m_nBlockLength[i] = m_nBlockLength[i + 1];
				}

				--m_ncDataBlock;
			}

			m_byBlockMode[nBlock] = QR_MODE_8BIT;
			m_nBlockLength[nBlock] += m_nBlockLength[nBlock + 1];

			// 後続をシフト
			for (i = nBlock + 1; i < m_ncDataBlock - 1; ++i)
			{
				m_byBlockMode[i]  = m_byBlockMode[i + 1];
				m_nBlockLength[i] = m_nBlockLength[i + 1];
			}

			--m_ncDataBlock;

			// 結合したブロックの前から再調査
			if (nBlock >= 1)
				--nBlock;

			continue;
		}

		++nBlock; // 次ブロックを調査
	}


  // actual bit encoding happens here.
	// ビット配列化
	int ncComplete = 0; // 処理済データカウンタ
	uint16_t wBinCode;

	m_ncDataCodeWordBit = 0; // ビット単位処理カウンタ

  for(int n=0;n<MAX_INPUTDATA;n++) m_byDataCodeWord[n]=0;
  
	for (i = 0; i < m_ncDataBlock && m_ncDataCodeWordBit != -1; ++i)
	{
		if (m_byBlockMode[i] == QR_MODE_NUMERAL)
		{
			/////////////////////////////////////////////////////////////////
			// 数字モード

			// インジケータ(0001b)
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, 1, 4); 

			// 文字数セット
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit,
                                         (uint16_t)m_nBlockLength[i],
                                         nIndicatorLenNumeral[nVerGroup]);

			// ビット列保存
			for (j = 0; j < m_nBlockLength[i]; j += 3)
			{
				if (j < m_nBlockLength[i] - 2)
				{
					wBinCode = (uint16_t)(((lpsSource[ncComplete + j]	  - '0') * 100) +
									  ((lpsSource[ncComplete + j + 1] - '0') * 10) +
									   (lpsSource[ncComplete + j + 2] - '0'));

					m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, wBinCode, 10);
				}
				else if (j == m_nBlockLength[i] - 2)
				{
					// 端数２バイト
					wBinCode = (uint16_t)(((lpsSource[ncComplete + j] - '0') * 10) +
									   (lpsSource[ncComplete + j + 1] - '0'));

					m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, wBinCode, 7);
				}
				else if (j == m_nBlockLength[i] - 1)
				{
					// 端数１バイト
					wBinCode = (uint16_t)(lpsSource[ncComplete + j] - '0');

					m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, wBinCode, 4);
				}
			}

			ncComplete += m_nBlockLength[i];
		}

		else if (m_byBlockMode[i] == QR_MODE_ALPHABET)
		{
			/////////////////////////////////////////////////////////////////
			// 英数字モード

			// モードインジケータ(0010b)
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, 2, 4);

			// 文字数セット
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, (uint16_t)m_nBlockLength[i], nIndicatorLenAlphabet[nVerGroup]);

			// ビット列保存
			for (j = 0; j < m_nBlockLength[i]; j += 2)
			{
				if (j < m_nBlockLength[i] - 1)
				{
					wBinCode = (uint16_t)((AlphabetToBinary(lpsSource[ncComplete + j]) * 45) +
									   AlphabetToBinary(lpsSource[ncComplete + j + 1]));

					m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, wBinCode, 11);
				}
				else
				{
					// 端数１バイト
					wBinCode = (uint16_t)AlphabetToBinary(lpsSource[ncComplete + j]);

					m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, wBinCode, 6);
				}
			}

			ncComplete += m_nBlockLength[i];
		}

		else if (m_byBlockMode[i] == QR_MODE_8BIT)
		{
			/////////////////////////////////////////////////////////////////
			// ８ビットバイトモード

			// モードインジケータ(0100b)
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, 4, 4);

			// 文字数セット
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, (uint16_t)m_nBlockLength[i], nIndicatorLen8Bit[nVerGroup]);

			// ビット列保存
			for (j = 0; j < m_nBlockLength[i]; ++j)
			{
				m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, (uint16_t)lpsSource[ncComplete + j], 8);
			}

			ncComplete += m_nBlockLength[i];
		}
		else // m_byBlockMode[i] == QR_MODE_KANJI
		{
			/////////////////////////////////////////////////////////////////
			// 漢字モード

			// モードインジケータ(1000b)
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, 8, 4);

			// 文字数セット
			m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, (uint16_t)(m_nBlockLength[i] / 2), nIndicatorLenKanji[nVerGroup]);

			// 漢字モードでビット列保存
			for (j = 0; j < m_nBlockLength[i] / 2; ++j)
			{
				uint16_t wBinCode = KanjiToBinary((uint16_t)(((uint8_t)lpsSource[ncComplete + (j * 2)] << 8) + (uint8_t)lpsSource[ncComplete + (j * 2) + 1]));

				m_ncDataCodeWordBit = SetBitStream(m_byDataCodeWord,m_ncDataCodeWordBit, wBinCode, 13);
			}

			ncComplete += m_nBlockLength[i];
		}
	}

	return (m_ncDataCodeWordBit != -1);
}




/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::GetRSCodeWord
// 用  途：ＲＳ誤り訂正コードワード取得
// 引  数：データコードワードアドレス、データコードワード長、ＲＳコードワード長
// 備  考：総コードワード分のエリアを確保してから呼び出し

void GetRSCodeWord(uint8_t *lpbyRSWork, int ncDataCodeWord, int ncRSCodeWord)
{
	int i, j;

	for (i = 0; i < ncDataCodeWord ; ++i)
	{
		if (lpbyRSWork[0] != 0)
		{
			uint8_t nExpFirst = byIntToExp[lpbyRSWork[0]]; // 初項係数より乗数算出

			for (j = 0; j < ncRSCodeWord; ++j)
			{
				// 各項乗数に初項乗数を加算（% 255 → α^255 = 1）
				uint8_t nExpElement = (uint8_t)(((int)(byRSExp[ncRSCodeWord][j] + nExpFirst)) % 255);

				// 排他論理和による剰余算出
				lpbyRSWork[j] = (uint8_t)(lpbyRSWork[j + 1] ^ byExpToInt[nExpElement]);
			}

			// 残り桁をシフト
			for (j = ncRSCodeWord; j < ncDataCodeWord + ncRSCodeWord - 1; ++j)
				lpbyRSWork[j] = lpbyRSWork[j + 1];
		}
		else
		{
			// 残り桁をシフト
			for (j = 0; j < ncDataCodeWord + ncRSCodeWord - 1; ++j)
				lpbyRSWork[j] = lpbyRSWork[j + 1];
		}
	}
}


void clear_qrimage(uint8_t *data) {
  for(int n=0;n<MAX_QRCODESIZE;n++) {
    data[n] = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::FormatModule
// 用  途：モジュールへのデータ配置
// 戻り値：一辺のモジュール数

void FormatModule(uint8_t *image,int width,uint8_t *input_data,int input_data_len,int m_nMaskingNo,int version,int level) {
	int i, j;

  clear_qrimage(image);

	// 機能モジュール配置
	SetFunctionModule(image,width,version);

	// データパターン配置
	SetCodeWordPattern(image,width,input_data,input_data_len,version);

	SetMaskingPattern(image,width,m_nMaskingNo,version); // マスキング
  // added masking pattern


	SetFunctionModule(image,width,version);

	SetFormatInfoPattern(image,width,m_nMaskingNo,level); // フォーマット情報パターン配置

}

bool is_within(int start_x,int start_y,int end_x,int end_y,int x,int y) {

  if((x >= start_x) && (x <= end_x) && (y >= start_y) && (y <= end_y)) return true;
  return false;
}

bool is_on_finder_pattern(int width,int x,int y) {
  if(is_within(0      ,0      ,7    ,7    ,x,y)) return true;
  if(is_within(width-7,0      ,width,7    ,x,y)) return true;
  if(is_within(0      ,width-7,7    ,width,x,y)) return true;
  return false;
}

bool is_on_deadarea(int width,int x,int y) {

  if((x >= 0) && (x < 8) && (y == 7)) return true;
  if((y >= 0) && (y < 8) && (x == 7)) return true;

  if((x >= 0) && (x < 8) && (y == width-8)) return true;
  if((y >= (width-8)) && (y <= width) && (x == 7)) return true;

  if((x == (width-8)) && (y >= 0) && (y < 8)) return true;
  if((x >= (width-8)) && (x <= width) && (y == 7)) return true;

	// フォーマット情報記述位置を機能モジュール部として登録

  if((x >= 0) && (x < 9) && (y == 8)) return true;
  if((y >= 0) && (y < 9) && (x == 8)) return true;

  if((x >= width-8) && (x<=width) && (y == 8)) return true;
  if((y >= width-8) && (y<=width) && (x == 8)) return true;

  return false;
}

bool is_on_timing(int width,int x,int y) {
  if((x>=8) && (x<=(width-9)) && (y == 6)) return true;
  if((y>=8) && (y<=(width-9)) && (x == 6)) return true;
  return false;
}

bool is_on_alignment(int version,int width,int x,int y) {

	for (int i = 0; i < QR_VersionInfo[version].ncAlignPoint; ++i)
	{

    if(!is_on_finder_pattern(width,QR_VersionInfo[version].nAlignPoint[i], 6)) {
    if(is_within(QR_VersionInfo[version].nAlignPoint[i]-2,
                 4,
                 QR_VersionInfo[version].nAlignPoint[i]-2+4,
                 8,x,y)) { return true;}
    }
    if(!is_on_finder_pattern(width,6,QR_VersionInfo[version].nAlignPoint[i])) {
    if(is_within(4,
                 QR_VersionInfo[version].nAlignPoint[i]-2,
                 8,
                 QR_VersionInfo[version].nAlignPoint[i]-2+4,
                 x,y)) return true;
    }

		for (int j = 0; j < QR_VersionInfo[version].ncAlignPoint; ++j)
		{

    if(!is_on_finder_pattern(width,QR_VersionInfo[version].nAlignPoint[i],QR_VersionInfo[version].nAlignPoint[i])) {
    if(is_within(QR_VersionInfo[version].nAlignPoint[i]-2,
                 QR_VersionInfo[version].nAlignPoint[j]-2,
                 QR_VersionInfo[version].nAlignPoint[i]-2+4,
                 QR_VersionInfo[version].nAlignPoint[j]-2+4,x,y)) return true; 
    }

		}
	}

  return false;
}

bool is_on_version(int width,int x,int y,int version) {

  if(version <= 6) return false;
  if(is_within(width-11,0,width-11+2,5,x,y)) { return true;}
  if(is_within(0,width-11,5,width-11+2,x,y)) { return true;}

  return false;
}

bool is_on_function_area(int width,int x,int y,int version) {
  if(is_on_finder_pattern(width,x,y)) { return true;}
  if(is_on_deadarea(width,x,y)) { return true;}
  if(is_on_timing(width,x,y)) {return true;}
  if(is_on_alignment(version,width,x,y)) { return true;}
  if(is_on_version(width,x,y,version)){ return true;}
  return false;
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetFunctionModule
// 用  途：機能モジュール配置
// 備  考：フォーマット情報は機能モジュール登録のみ(実データは空白)

// My understanding is that this function places the various formating and alignment data on the image.
// It does not add the coded data to the image.
void SetFunctionModule(uint8_t *image,int width,int version) {
	int i, j;

	// 位置検出パターン
	SetFinderPattern(image,width,0, 0);
	SetFinderPattern(image,width,width - 7, 0);
	SetFinderPattern(image,width,0,width - 7);

	// 位置検出パターンセパレータ
	for (i = 0; i < 8; ++i) {
    qr_setmodule(image,width,i,7,0);
    qr_setmodule(image,width,7,i,0);
//	 	m_byModuleData[i][7] = m_byModuleData[7][i] = '\x20';
    qr_setmodule(image,width,width-8,i,0);
    qr_setmodule(image,width,width-8+i,7,0);
//		m_byModuleData[m_nSymbolSize - 8][i] = m_byModuleData[m_nSymbolSize - 8 + i][7] = '\x20';


    qr_setmodule(image,width,i,width-8,0);
    qr_setmodule(image,width,7,width-8+i,0);
//		m_byModuleData[i][m_nSymbolSize - 8] = m_byModuleData[7][m_nSymbolSize - 8 + i] = '\x20';
	}

	// フォーマット情報記述位置を機能モジュール部として登録
	for (i = 0; i < 9; ++i)
	{
    qr_setmodule(image,width,i,8,0);
    qr_setmodule(image,width,8,i,0);
//		m_byModuleData[i][8] = m_byModuleData[8][i] = '\x20';
	}

	for (i = 0; i < 8; ++i)
	{
    qr_setmodule(image,width,width-8+i,8,0);
    qr_setmodule(image,width,8,width-8+i,0);
//		m_byModuleData[m_nSymbolSize - 8 + i][8] = m_byModuleData[8][m_nSymbolSize - 8 + i] = '\x20';
	}
  
  // Timing Pattern
	// タイミングパターン
	for (i = 8; i <= width- 9; ++i)
	{
    qr_setmodule(image,width,i,6,(i%2));
    qr_setmodule(image,width,6,i,(i%2));
//		m_byModuleData[i][6] = (i % 2) == 0 ? '\x30' : '\x20';
//		m_byModuleData[6][i] = (i % 2) == 0 ? '\x30' : '\x20';
	}

	// バージョン情報パターン
	SetVersionPattern(image,width,version);

	// 位置合わせパターン
	for (i = 0; i < QR_VersionInfo[version].ncAlignPoint; ++i)
	{
		SetAlignmentPattern(image,width,QR_VersionInfo[version].nAlignPoint[i], 6);
		SetAlignmentPattern(image,width,6, QR_VersionInfo[version].nAlignPoint[i]);

		for (j = 0; j < QR_VersionInfo[version].ncAlignPoint; ++j)
		{
			SetAlignmentPattern(image,width,QR_VersionInfo[version].nAlignPoint[i], QR_VersionInfo[version].nAlignPoint[j]);
		}
	}

}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetFinderPattern
// 用  途：位置検出パターン配置
// 引  数：配置左上座標

void SetFinderPattern(uint8_t *image,int width,int x, int y)
{
	static uint8_t byPattern[] = {0x7f,  // 1111111b
							   0x41,  // 1000001b
							   0x5d,  // 1011101b
							   0x5d,  // 1011101b
							   0x5d,  // 1011101b
							   0x41,  // 1000001b
							   0x7f}; // 1111111b
	int i, j;

	for (i = 0; i < 7; ++i)
	{
		for (j = 0; j < 7; ++j)
		{
      qr_setmodule(image,width,x+j,y+i,(byPattern[i] & (1 << (6 - j))));
//			m_byModuleData[x + j][y + i] = (byPattern[i] & (1 << (6 - j))) ? '\x30' : '\x20'; 
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetAlignmentPattern
// 用  途：位置合わせパターン配置
// 引  数：配置中央座標

void SetAlignmentPattern(uint8_t *image,int width, int x, int y)
{
	static uint8_t byPattern[] = {0x1f,  // 11111b
							   0x11,  // 10001b
							   0x15,  // 10101b
							   0x11,  // 10001b
							   0x1f}; // 11111b
	int i, j;

  // There's already alignment data here so return.
  if(qr_getmodule(image,width,x,y) != 0) return;
//	if (m_byModuleData[x][y] & 0x20)
//		return; // 機能モジュールと重複するため除外

	x -= 2; y -= 2; // 左上隅座標に変換

	for (i = 0; i < 5; ++i)
	{
		for (j = 0; j < 5; ++j)
		{
      qr_setmodule(image,width,x+j,y+i,byPattern[i] & (1 << (4 - j)));
			//m_byModuleData[x + j][y + i] = (byPattern[i] & (1 << (4 - j))) ? '\x30' : '\x20'; 
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetVersionPattern
// 用  途：バージョン(型番)情報パターン配置
// 備  考：拡張ＢＣＨ(18,6)符号を誤り訂正として使用

void SetVersionPattern(uint8_t *image,int width,int version)
{
	int i, j;

	if (version <= 6)
		return;

	int nVerData = version << 12;

	// 剰余ビット算出
	for (i = 0; i < 6; ++i)
	{
		if (nVerData & (1 << (17 - i)))
		{
			nVerData ^= (0x1f25 << (5 - i));
		}
	}

	nVerData += version << 12;

	for (i = 0; i < 6; ++i)
	{
		for (j = 0; j < 3; ++j)
		{
      qr_setmodule(image,width,width-11+j,i,(nVerData & (1 << (i * 3 + j))) );
      qr_setmodule(image,width,i,width-11+j,(nVerData & (1 << (i * 3 + j))) );
			//m_byModuleData[m_nSymbolSize - 11 + j][i] = m_byModuleData[i][m_nSymbolSize - 11 + j] =
			//(nVerData & (1 << (i * 3 + j))) ? '\x30' : '\x20';
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetCodeWordPattern
// 用  途：データパターン配置

void SetCodeWordPattern(uint8_t *image,int width,uint8_t *encoded_data,int encoded_data_size,int version)
{
	int x = width;
	int y = width - 1;

	int nCoef_x = 1; // ｘ軸配置向き
	int nCoef_y = 1; // ｙ軸配置向き

	int i, j;
	for (i = 0; i < encoded_data_size; ++i)
	{
		for (j = 0; j < 8; ++j)
		{
			do
			{
				x += nCoef_x;
				nCoef_x *= -1;

				if (nCoef_x < 0)
				{
					y += nCoef_y;

					if (y < 0 || y == width)
					{
						y = (y < 0) ? 0 : width - 1;
						nCoef_y *= -1;

						x -= 2;

						if (x == 6) // タイミングパターン
							--x;
					}
				}
			}
			while (is_on_function_area(width,x,y,version));

//m_byModuleData[x][y] & 0x20); // 機能モジュールを除外
  
      int set=0;
      if(encoded_data[i] & (1 << (7-j))) set=1;else set=0;
 
      qr_setmodule(image,width,x,y,set);

			//m_byModuleData[x][y] = (m_byAllCodeWord[i] & (1 << (7 - j))) ? '\x02' : '\x00';
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetMaskingPattern
// 用  途：マスキングパターン配置
// 引  数：マスキングパターン番号

void SetMaskingPattern(uint8_t *image,int width,int nPatternNo,int version)
{
	int i, j;

  int m_nSymbolSize = width;

	for (i = 0; i < m_nSymbolSize; ++i)
	{
		for (j = 0; j < m_nSymbolSize; ++j)
		{
			if(!is_on_function_area(width,j,i,version)) 
			//if (!(qr_getmodule(image,width,j,i))) // 機能モジュールを除外
			//if (! (m_byModuleData[j][i] & 0x20)) // 機能モジュールを除外
			{
				bool bMask;

				switch (nPatternNo)
				{
				case 0:
					bMask = ((i + j) % 2 == 0);
					break;

				case 1:
					bMask = (i % 2 == 0);
					break;

				case 2:
					bMask = (j % 3 == 0);
					break;

				case 3:
					bMask = ((i + j) % 3 == 0);
					break;

				case 4:
					bMask = (((i / 2) + (j / 3)) % 2 == 0);
					break;

				case 5:
					bMask = (((i * j) % 2) + ((i * j) % 3) == 0);
					break;

				case 6:
					bMask = ((((i * j) % 2) + ((i * j) % 3)) % 2 == 0);
					break;

				default: // case 7:
					bMask = ((((i * j) % 3) + ((i + j) % 2)) % 2 == 0);
					break;
				}

			 // if(is_on_function_area(width,j,i,version)) {
          int d = qr_getmodule(image,width,j,i) ^ bMask;
          qr_setmodule(image,width,j,i,d);
      //  } else {
      //  }
//				m_byModuleData[j][i] = (uint8_t)((m_byModuleData[j][i] & 0xfe) | (((m_byModuleData[j][i] & 0x02) > 1) ^ bMask));
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::SetFormatInfoPattern
// 用  途：フォーマット情報配置
// 引  数：マスキングパターン番号

void SetFormatInfoPattern(uint8_t *image,int width,int nPatternNo,int level)
{
	int nFormatInfo;
	int i;

	switch (level)
	{
	case QR_LEVEL_M:
		nFormatInfo = 0x00; // 00nnnb
		break;

	case QR_LEVEL_L:
		nFormatInfo = 0x08; // 01nnnb
		break;

	case QR_LEVEL_Q:
		nFormatInfo = 0x18; // 11nnnb
		break;

	default: // case QR_LEVEL_H:
		nFormatInfo = 0x10; // 10nnnb
		break;
	}

	nFormatInfo += nPatternNo;

	int nFormatData = nFormatInfo << 10;

	// 剰余ビット算出
	for (i = 0; i < 5; ++i)
	{
		if (nFormatData & (1 << (14 - i)))
		{
			nFormatData ^= (0x0537 << (4 - i)); // 10100110111b
		}
	}

	nFormatData += nFormatInfo << 10;

	// マスキング
	nFormatData ^= 0x5412; // 101010000010010b

	// 左上位置検出パターン周り配置
	for (i = 0; i <= 5; ++i) qr_setmodule(image,width,8,i,(nFormatData & (1 << i)));
//		m_byModuleData[8][i] = (nFormatData & (1 << i)) ? '\x30' : '\x20';

  qr_setmodule(image,width,8,7,(nFormatData & (1 << 6)));
  qr_setmodule(image,width,8,8,(nFormatData & (1 << 7)));
  qr_setmodule(image,width,7,8,(nFormatData & (1 << 8)));
	//m_byModuleData[8][7] = (nFormatData & (1 << 6)) ? '\x30' : '\x20';
	//m_byModuleData[8][8] = (nFormatData & (1 << 7)) ? '\x30' : '\x20';
	//m_byModuleData[7][8] = (nFormatData & (1 << 8)) ? '\x30' : '\x20';

	for (i = 9; i <= 14; ++i) qr_setmodule(image,width,14-i,8,(nFormatData & (1 << i)));
//		m_byModuleData[14 - i][8] = (nFormatData & (1 << i)) ? '\x30' : '\x20';

	// 右上位置検出パターン下配置
	for (i = 0; i <= 7; ++i) qr_setmodule(image,width,width-1-i,8,nFormatData & (1 << i));
//		m_byModuleData[m_nSymbolSize - 1 - i][8] = (nFormatData & (1 << i)) ? '\x30' : '\x20';

	// 左下位置検出パターン右配置
  qr_setmodule(image,width,8,width-8,1);
//	m_byModuleData[8][m_nSymbolSize - 8] = '\x30'; // 固定暗モジュール

	for (i = 8; i <= 14; ++i) qr_setmodule(image,width,8,width-15+i,(nFormatData & (1 << i)));
		//m_byModuleData[8][m_nSymbolSize - 15 + i] = (nFormatData & (1 << i)) ? '\x30' : '\x20';
}

/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::CountPenalty
// 用  途：マスク後ペナルティスコア算出
/*
int CountPenalty(uint8_t *image,int width) {
	int nPenalty = 0;
	int i, j, k;

	// 同色の列の隣接モジュール
	for (i = 0; i < width; ++i)
	{
		for (j = 0; j < width - 4; ++j)
		{
			int nCount = 1;

			for (k = j + 1; k < width; k++)
			{
				if ((qr_getmoduleC(image,width,i,j) == 0) == (qr_getmoduleC(image,width,i,k) == 0))
					++nCount;
				else
					break;
			}

			if (nCount >= 5)
			{
				nPenalty += 3 + (nCount - 5);
			}

			j = k - 1;
		}
	}

	// 同色の行の隣接モジュール
	for (i = 0; i < width; ++i)
	{
		for (j = 0; j < width - 4; ++j)
		{
			int nCount = 1;

			for (k = j + 1; k < width; k++)
			{
				if ((qr_getmoduleC(image,width,j,i) == 0) == (qr_getmoduleC(image,width,k,i) == 0))
					++nCount;
				else
					break;
			}

			if (nCount >= 5)
			{
				nPenalty += 3 + (nCount - 5);
			}

			j = k - 1;
		}
	}

	// 同色のモジュールブロック（２×２）
	for (i = 0; i < width - 1; ++i)
	{
		for (j = 0; j < width - 1; ++j)
		{
			if (((qr_getmodule(image,width,i,j) == 0) == (qr_getmodule(image,width,i + 1,j) == 0)) &&
          ((qr_getmodule(image,width,i,j) == 0) == (qr_getmodule(image,width,i,j + 1) == 0)) &&
				  ((qr_getmodule(image,width,i,j) == 0) == (qr_getmodule(image,width,i + 1,j + 1) == 0)))
			{
				nPenalty += 3;
			}
		}
	}

	// 同一列における 1:1:3:1:1 比率（暗:明:暗:明:暗）のパターン
	for (i = 0; i < width; ++i)
	{
		for (j = 0; j < width - 6; ++j)
		{

      // 明 means bright. 暗 means dark.
			if (((j == 0) || (! (qr_getmodule(image,width,i,j - 1))) && // 明 または シンボル外
											 (  qr_getmodule(image,width,i,j))   && // 暗 - 1
											 (! (qr_getmodule(image,width,i,j + 1)))  && // 明- 1
											 (   qr_getmodule(image,width,i,j + 2))   && // 暗 ┐
											 (   qr_getmodule(image,width,i,j + 3))   && // 暗│3
											 (   qr_getmodule(image,width,i,j + 4))   && // 暗 ┘
											 (! (qr_getmodule(image,width,i,j + 5)))  && // 明 - 1
											 (   qr_getmodule(image,width,i,j + 6))   && // 暗 - 1
				((j == width - 7) || (! (qr_getmoduleC(image,width,i,j + 7))))))   // 明 または シンボル外
			{
				// 前または後に4以上の明パターン
				if (((j < 2 || ! (qr_getmodule(image,width,i,j - 2))) && 
					 (j < 3 || ! (qr_getmodule(image,width,i,j - 3))) &&
					 (j < 4 || ! (qr_getmodule(image,width,i,j - 4)))) ||
					((j >= width - 8  || ! (qr_getmodule(image,width,i,j + 8))) &&
					 (j >= width - 9  || ! (qr_getmodule(image,width,i,j + 9))) &&
					 (j >= width - 10 || ! (qr_getmodule(image,width,i,j + 10)))))
				{
					nPenalty += 40;
				}
			}
		}
	}

	// 同一行における 1:1:3:1:1 比率（暗:明:暗:明:暗）のパターン
	for (i = 0; i < width; ++i)
	{
		for (j = 0; j < width - 6; ++j)
		{
			if (((j == 0) ||				 (! (qr_getmodule(image,width,j - 1,i)))) && // 明 または シンボル外
											 (   qr_getmodule(image,width,j,i))   && // 暗 - 1
											 (! (qr_getmodule(image,width,j + 1,i)))  && // 明 - 1
											 (   qr_getmodule(image,width,j + 2,i))   && // 暗 ┐
											 (   qr_getmodule(image,width,j + 3,i))   && // 暗 │3
											 (   qr_getmodule(image,width,j + 4,i))   && // 暗 ┘
											 (! (qr_getmodule(image,width,j + 5,i)))  && // 明 - 1
											 (   qr_getmodule(image,width,j + 6,i))   && // 暗 - 1
				((j == width - 7) || (! (qr_getmodule(image,width,j + 7,i)))))   // 明 または シンボル外
			{
				// 前または後に4以上の明パターン
				if (((j < 2 || ! (qr_getmodule(image,width,j - 2,i))) && 
					 (j < 3 || ! (qr_getmodule(image,width,j - 3,i))) &&
					 (j < 4 || ! (qr_getmodule(image,width,j - 4,i)))) ||
					((j >= width - 8  || ! (qr_getmodule(image,width,j + 8,i))) &&
					 (j >= width - 9  || ! (qr_getmodule(image,width,j + 9,i))) &&
					 (j >= width - 10 || ! (qr_getmodule(image,width,j + 10,i)))))
				{
					nPenalty += 40;
				}
			}
		}
	}

	// 全体に対する暗モジュールの占める割合
	int nCount = 0;

	for (i = 0; i < width; ++i)
	{
		for (j = 0; j < width; ++j)
		{
			if (! (qr_getmoduleC(image,width,i,j)))
			{
				++nCount;
			}
		}
	}

  if(width == 0) return 0;
	nPenalty += (abs(50 - ((nCount * 100) / (width * width))) / 5) * 10;

	return nPenalty;
}
*/
