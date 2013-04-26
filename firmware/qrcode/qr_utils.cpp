#ifndef QR_UTILS_H
#define QR_UTILS_H
#include <stdint.h>

// 文字数インジケータビット長(バージョングループ別, {S, M, L})
static int nIndicatorLenNumeral[]  = {10, 12, 14};
static int nIndicatorLenAlphabet[] = { 9, 11, 13};
static int nIndicatorLen8Bit[]	   = { 8, 16, 16};
static int nIndicatorLenKanji[]	   = { 8, 10, 12};

#define QR_MODE_NUMERAL		0
#define QR_MODE_ALPHABET	1
#define QR_MODE_8BIT		2
#define QR_MODE_KANJI		3

/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::IsKanjiData
// 用  途：漢字モード該当チェック
// 引  数：調査文字（16ビット文字）
// 戻り値：該当時=true
// 備  考：EBBFh 以降の S-JIS は対象外

bool IsKanjiData(unsigned char c1, unsigned char c2)
{
	if (((c1 >= 0x81 && c1 <= 0x9f) || (c1 >= 0xe0 && c1 <= 0xeb)) && (c2 >= 0x40))
	{
		if ((c1 == 0x9f && c2 > 0xfc) || (c1 == 0xeb && c2 > 0xbf))
			return false;

		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::IsNumeralData
// 用  途：数字モード該当チェック
// 引  数：調査文字
// 戻り値：該当時=true

bool IsNumeralData(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return true;

	return false;
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::IsAlphabetData
// 用  途：英数字モード該当チェック
// 引  数：調査文字
// 戻り値：該当時=true

bool IsAlphabetData(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return true;

	if (c >= 'A' && c <= 'Z')
		return true;

	if (c == ' ' || c == '$' || c == '%' || c == '*' || c == '+' || c == '-' || c == '.' || c == '/' || c == ':')
		return true;

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::GetBitLength
// 用  途：ビット長取得
// 引  数：データモード種別、データ長、バージョン(型番)グループ
// 戻り値：データビット長
// 備  考：漢字モードでのデータ長引数は文字数ではなくバイト数

int GetBitLength(uint8_t nMode, int ncData, int nVerGroup)
{
	int ncBits = 0;

	switch (nMode)
	{
	case QR_MODE_NUMERAL:
		ncBits = 4 + nIndicatorLenNumeral[nVerGroup] + (10 * (ncData / 3));
		switch (ncData % 3)
		{
		case 1:
			ncBits += 4;
			break;
		case 2:
			ncBits += 7;
			break;
		default: // case 0:
			break;
		}

		break;

	case QR_MODE_ALPHABET:
		ncBits = 4 + nIndicatorLenAlphabet[nVerGroup] + (11 * (ncData / 2)) + (6 * (ncData % 2));
		break;

	case QR_MODE_8BIT:
		ncBits = 4 + nIndicatorLen8Bit[nVerGroup] + (8 * ncData);
		break;

	default: // case QR_MODE_KANJI:
		ncBits = 4 + nIndicatorLenKanji[nVerGroup] + (13 * (ncData / 2));
		break;
	}

	return ncBits;
}


// CQR_Encode::AlphabetToBinary
// 用  途：英数字モード文字のバイナリ化
// 引  数：対象文字
// 戻り値：バイナリ値

uint8_t AlphabetToBinary(unsigned char c)
{
	if (c >= '0' && c <= '9') return (unsigned char)(c - '0');

	if (c >= 'A' && c <= 'Z') return (unsigned char)(c - 'A' + 10);

	if (c == ' ') return 36;

	if (c == '$') return 37;

	if (c == '%') return 38;

	if (c == '*') return 39;

	if (c == '+') return 40;

	if (c == '-') return 41;

	if (c == '.') return 42;

	if (c == '/') return 43;

	return 44; // c == ':'
}


/////////////////////////////////////////////////////////////////////////////
// CQR_Encode::KanjiToBinary
// 用  途：漢字モード文字のバイナリ化
// 引  数：対象文字
// 戻り値：バイナリ値

uint16_t KanjiToBinary(uint16_t wc)
{
	if (wc >= 0x8140 && wc <= 0x9ffc)
		wc -= 0x8140;
	else // (wc >= 0xe040 && wc <= 0xebbf)
		wc -= 0xc140;

	return (uint16_t)(((wc >> 8) * 0xc0) + (wc & 0x00ff));
}

#endif
