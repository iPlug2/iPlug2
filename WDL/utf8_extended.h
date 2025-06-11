#ifndef _WDL_UTF8_EXTENDED_H_
#define _WDL_UTF8_EXTENDED_H_

// these are latin-1 supplemental (first utf-8 byte must be 0xc3), pass second byte&~0x20, second byte
#define WDL_IS_UTF8_BYTE2_LATIN1S_A(cc,ccf) ((cc) >= 0x80 && (cc) <= 0x85)
#define WDL_IS_UTF8_BYTE2_LATIN1S_C(cc,ccf) ((cc) == 0x87)
#define WDL_IS_UTF8_BYTE2_LATIN1S_E(cc,ccf) ((cc) >= 0x88 && (cc) <= 0x8b)
#define WDL_IS_UTF8_BYTE2_LATIN1S_I(cc,ccf) ((cc) >= 0x8c && (cc) <= 0x8f)
#define WDL_IS_UTF8_BYTE2_LATIN1S_N(cc,ccf) ((cc) == 0x91)
#define WDL_IS_UTF8_BYTE2_LATIN1S_O(cc,ccf) ((cc) >= 0x92 && (cc) <= 0x96)
#define WDL_IS_UTF8_BYTE2_LATIN1S_U(cc,ccf) ((cc) >= 0x99 && (cc) <= 0x9c)
#define WDL_IS_UTF8_BYTE2_LATIN1S_Y(cc,ccf) ((cc) == 0x9d || (ccf) == 0x9f)

// latin extended A
#define WDL_IS_UTF8_EXT1A_A(b1, b2) ((b1)==0xc4 && (b2) >= 0x80 && (b2) <= 0x85)
#define WDL_IS_UTF8_EXT1A_C(b1, b2) ((b1)==0xc4 && (b2) >= 0x86 && (b2) <= 0x8D)
#define WDL_IS_UTF8_EXT1A_D(b1, b2) ((b1)==0xc4 && (b2) >= 0x8E && (b2) <= 0x91)
#define WDL_IS_UTF8_EXT1A_E(b1, b2) ((b1)==0xc4 && (b2) >= 0x92 && (b2) <= 0x9B)
#define WDL_IS_UTF8_EXT1A_G(b1, b2) ((b1)==0xc4 && (b2) >= 0x9C && (b2) <= 0xa3)
#define WDL_IS_UTF8_EXT1A_H(b1, b2) ((b1)==0xc4 && (b2) >= 0xa4 && (b2) <= 0xa7)
#define WDL_IS_UTF8_EXT1A_I(b1, b2) ((b1)==0xc4 && (b2) >= 0xa8 && (b2) <= 0xb1)
#define WDL_IS_UTF8_EXT1A_J(b1, b2) ((b1)==0xc4 && (b2) >= 0xb4 && (b2) <= 0xb5)
#define WDL_IS_UTF8_EXT1A_K(b1, b2) ((b1)==0xc4 && (b2) >= 0xb6 && (b2) <= 0xb8)
#define WDL_IS_UTF8_EXT1A_L(b1, b2) ((b1)==0xc4 ? ((b2) >= 0xb9 && (b2) <= 0xbf) : \
                                ((b1)==0xc5 && (b2) >= 0x80 && (b2) <= 0x82))
#define WDL_IS_UTF8_EXT1A_N(b1, b2) ((b1)==0xc5 && (b2) >= 0x83 && (b2) <= 0x89)
#define WDL_IS_UTF8_EXT1A_O(b1, b2) ((b1)==0xc5 && (b2) >= 0x8c && (b2) <= 0x91)
#define WDL_IS_UTF8_EXT1A_R(b1, b2) ((b1)==0xc5 && (b2) >= 0x94 && (b2) <= 0x99)
#define WDL_IS_UTF8_EXT1A_S(b1, b2) ((b1)==0xc5 && (b2) >= 0x9a && (b2) <= 0xa1)
#define WDL_IS_UTF8_EXT1A_T(b1, b2) ((b1)==0xc5 && (b2) >= 0xa2 && (b2) <= 0xa7)
#define WDL_IS_UTF8_EXT1A_U(b1, b2) ((b1)==0xc5 && (b2) >= 0xa8 && (b2) <= 0xb3)
#define WDL_IS_UTF8_EXT1A_W(b1, b2) ((b1)==0xc5 && (b2) >= 0xb4 && (b2) <= 0xb5)
#define WDL_IS_UTF8_EXT1A_Y(b1, b2) ((b1)==0xc5 && (b2) >= 0xb6 && (b2) <= 0xb8)
#define WDL_IS_UTF8_EXT1A_Z(b1, b2) ((b1)==0xc5 && (b2) >= 0xb9 && (b2) <= 0xbe)

// U+300..U+36F are combining accents and get filtered/ignored
#define WDL_IS_UTF8_SKIPPABLE(ca, nextc) \
       (((((ca)&~1) == 0xCC) && ((nextc) >= 0x80 && (nextc) < ((ca) == 0xCD ? 0xAF : 0xc0))) ? 2 : 0)

#endif
