// This source file is part of the polarphp.org open source project
//
// Copyright (c) { 2017 - 2018 polarphp software foundation
// Copyright (c) { 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/17.

#include "polarphp/utils/Unicode.h"

namespace polar {
namespace sys {
namespace unicode {

int fold_char_simple(int c) {
   if (c < 0x0041) {
      return c;
   }
   // 26 characters
   if (c <= 0x005a) {
      return c + 32;
   }
   // MICRO SIGN
   if (c == 0x00b5) {
      return 0x03bc;
   }
   if (c < 0x00c0) {
      return c;
   }
   // 23 characters
   if (c <= 0x00d6) {
      return c + 32;
   }
   if (c < 0x00d8) {
      return c;
   }
   // 7 characters
   if (c <= 0x00de) {
      return c + 32;
   }
   if (c < 0x0100) {
      return c;
   }
   // 24 characters
   if (c <= 0x012e) {
      return c | 1;
   }
   if (c < 0x0132) {
      return c;
   }
   // 3 characters
   if (c <= 0x0136) {
      return c | 1;
   }
   if (c < 0x0139) {
      return c;
   }
   // 8 characters
   if (c <= 0x0147 && c % 2 == 1) {
      return c + 1;
   }
   if (c < 0x014a) {
      return c;
   }
   // 23 characters
   if (c <= 0x0176) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER Y WITH DIAERESIS
   if (c == 0x0178) {
      return 0x00ff;
   }
   if (c < 0x0179) {
      return c;
   }
   // 3 characters
   if (c <= 0x017d && c % 2 == 1) {
      return c + 1;
   }
   // LATIN SMALL LETTER LONG S
   if (c == 0x017f) {
      return 0x0073;
   }
   // LATIN CAPITAL LETTER B WITH HOOK
   if (c == 0x0181) {
      return 0x0253;
   }
   if (c < 0x0182) {
      return c;
   }
   // 2 characters
   if (c <= 0x0184) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER OPEN O
   if (c == 0x0186) {
      return 0x0254;
   }
   // LATIN CAPITAL LETTER c WITH HOOK
   if (c == 0x0187) {
      return 0x0188;
   }
   if (c < 0x0189) {
      return c;
   }
   // 2 characters
   if (c <= 0x018a) {
      return c + 205;
   }
   // LATIN CAPITAL LETTER D WITH TOPBAR
   if (c == 0x018b) {
      return 0x018c;
   }
   // LATIN CAPITAL LETTER REVERSED E
   if (c == 0x018e) {
      return 0x01dd;
   }
   // LATIN CAPITAL LETTER SCHWA
   if (c == 0x018f) {
      return 0x0259;
   }
   // LATIN CAPITAL LETTER OPEN E
   if (c == 0x0190) {
      return 0x025b;
   }
   // LATIN CAPITAL LETTER F WITH HOOK
   if (c == 0x0191) {
      return 0x0192;
   }
   // LATIN CAPITAL LETTER G WITH HOOK
   if (c == 0x0193) {
      return 0x0260;
   }
   // LATIN CAPITAL LETTER GAMMA
   if (c == 0x0194) {
      return 0x0263;
   }
   // LATIN CAPITAL LETTER IOTA
   if (c == 0x0196) {
      return 0x0269;
   }
   // LATIN CAPITAL LETTER I WITH STROKE
   if (c == 0x0197) {
      return 0x0268;
   }
   // LATIN CAPITAL LETTER K WITH HOOK
   if (c == 0x0198) {
      return 0x0199;
   }
   // LATIN CAPITAL LETTER TURNED M
   if (c == 0x019c) {
      return 0x026f;
   }
   // LATIN CAPITAL LETTER N WITH LEFT HOOK
   if (c == 0x019d) {
      return 0x0272;
   }
   // LATIN CAPITAL LETTER O WITH MIDDLE TILDE
   if (c == 0x019f) {
      return 0x0275;
   }
   if (c < 0x01a0) {
      return c;
   }
   // 3 characters
   if (c <= 0x01a4) {
      return c | 1;
   }
   // LATIN LETTER YR
   if (c == 0x01a6) {
      return 0x0280;
   }
   // LATIN CAPITAL LETTER TONE TWO
   if (c == 0x01a7) {
      return 0x01a8;
   }
   // LATIN CAPITAL LETTER ESH
   if (c == 0x01a9) {
      return 0x0283;
   }
   // LATIN CAPITAL LETTER T WITH HOOK
   if (c == 0x01ac) {
      return 0x01ad;
   }
   // LATIN CAPITAL LETTER T WITH RETROFLEX HOOK
   if (c == 0x01ae) {
      return 0x0288;
   }
   // LATIN CAPITAL LETTER U WITH HORN
   if (c == 0x01af) {
      return 0x01b0;
   }
   if (c < 0x01b1) {
      return c;
   }
   // 2 characters
   if (c <= 0x01b2) {
      return c + 217;
   }
   if (c < 0x01b3) {
      return c;
   }
   // 2 characters
   if (c <= 0x01b5 && c % 2 == 1) {
      return c + 1;
   }
   // LATIN CAPITAL LETTER EZH
   if (c == 0x01b7) {
      return 0x0292;
   }
   if (c < 0x01b8) {
      return c;
   }
   // 2 characters
   if (c <= 0x01bc && c % 4 == 0) {
      return c + 1;
   }
   // LATIN CAPITAL LETTER DZ WITH CARON
   if (c == 0x01c4) {
      return 0x01c6;
   }
   // LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON
   if (c == 0x01c5) {
      return 0x01c6;
   }
   // LATIN CAPITAL LETTER LJ
   if (c == 0x01c7) {
      return 0x01c9;
   }
   // LATIN CAPITAL LETTER L WITH SMALL LETTER J
   if (c == 0x01c8) {
      return 0x01c9;
   }
   // LATIN CAPITAL LETTER NJ
   if (c == 0x01ca) {
      return 0x01cc;
   }
   if (c < 0x01cb) {
      return c;
   }
   // 9 characters
   if (c <= 0x01db && c % 2 == 1) {
      return c + 1;
   }
   if (c < 0x01de) {
      return c;
   }
   // 9 characters
   if (c <= 0x01ee) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER DZ
   if (c == 0x01f1) {
      return 0x01f3;
   }
   if (c < 0x01f2) {
      return c;
   }
   // 2 characters
   if (c <= 0x01f4) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER HWAIR
   if (c == 0x01f6) {
      return 0x0195;
   }
   // LATIN CAPITAL LETTER WYNN
   if (c == 0x01f7) {
      return 0x01bf;
   }
   if (c < 0x01f8) {
      return c;
   }
   // 20 characters
   if (c <= 0x021e) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER N WITH LONG RIGHT LEG
   if (c == 0x0220) {
      return 0x019e;
   }
   if (c < 0x0222) {
      return c;
   }
   // 9 characters
   if (c <= 0x0232) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER A WITH STROKE
   if (c == 0x023a) {
      return 0x2c65;
   }
   // LATIN CAPITAL LETTER c WITH STROKE
   if (c == 0x023b) {
      return 0x023c;
   }
   // LATIN CAPITAL LETTER L WITH BAR
   if (c == 0x023d) {
      return 0x019a;
   }
   // LATIN CAPITAL LETTER T WITH DIAGONAL STROKE
   if (c == 0x023e) {
      return 0x2c66;
   }
   // LATIN CAPITAL LETTER GLOTTAL STOP
   if (c == 0x0241) {
      return 0x0242;
   }
   // LATIN CAPITAL LETTER B WITH STROKE
   if (c == 0x0243) {
      return 0x0180;
   }
   // LATIN CAPITAL LETTER U BAR
   if (c == 0x0244) {
      return 0x0289;
   }
   // LATIN CAPITAL LETTER TURNED V
   if (c == 0x0245) {
      return 0x028c;
   }
   if (c < 0x0246) {
      return c;
   }
   // 5 characters
   if (c <= 0x024e) {
      return c | 1;
   }
   // COMBINING GREEK YPOGEGRAMMENI
   if (c == 0x0345) {
      return 0x03b9;
   }
   if (c < 0x0370) {
      return c;
   }
   // 2 characters
   if (c <= 0x0372) {
      return c | 1;
   }
   // GREEK CAPITAL LETTER PAMPHYLIAN DIGAMMA
   if (c == 0x0376) {
      return 0x0377;
   }
   // GREEK CAPITAL LETTER YOT
   if (c == 0x037f) {
      return 0x03f3;
   }
   // GREEK CAPITAL LETTER ALPHA WITH TONOS
   if (c == 0x0386) {
      return 0x03ac;
   }
   if (c < 0x0388) {
      return c;
   }
   // 3 characters
   if (c <= 0x038a) {
      return c + 37;
   }
   // GREEK CAPITAL LETTER OMICRON WITH TONOS
   if (c == 0x038c) {
      return 0x03cc;
   }
   if (c < 0x038e) {
      return c;
   }
   // 2 characters
   if (c <= 0x038f) {
      return c + 63;
   }
   if (c < 0x0391) {
      return c;}
   // 17 characters
   if (c <= 0x03a1) {
      return c + 32;
   }
   if (c < 0x03a3) {
      return c;
   }
   // 9 characters
   if (c <= 0x03ab) {
      return c + 32;
   }
   // GREEK SMALL LETTER FINAL SIGMA
   if (c == 0x03c2) {
      return 0x03c3;
   }
   // GREEK CAPITAL KAI SYMBOL
   if (c == 0x03cf) {
      return 0x03d7;
   }
   // GREEK BETA SYMBOL
   if (c == 0x03d0) {
      return 0x03b2;
   }
   // GREEK THETA SYMBOL
   if (c == 0x03d1) {
      return 0x03b8;
   }
   // GREEK PHI SYMBOL
   if (c == 0x03d5) {
      return 0x03c6;
   }
   // GREEK PI SYMBOL
   if (c == 0x03d6) {
      return 0x03c0;
   }
   if (c < 0x03d8) {
      return c;
   }
   // 12 characters
   if (c <= 0x03ee) {
      return c | 1;
   }
   // GREEK KAPPA SYMBOL
   if (c == 0x03f0) {
      return 0x03ba;
   }
   // GREEK RHO SYMBOL
   if (c == 0x03f1) {
      return 0x03c1;
   }
   // GREEK CAPITAL THETA SYMBOL
   if (c == 0x03f4) {
      return 0x03b8;
   }
   // GREEK LUNATE EPSILON SYMBOL
   if (c == 0x03f5) {
      return 0x03b5;
   }
   // GREEK CAPITAL LETTER SHO
   if (c == 0x03f7) {
      return 0x03f8;
   }
   // GREEK CAPITAL LUNATE SIGMA SYMBOL
   if (c == 0x03f9) {
      return 0x03f2;
   }
   // GREEK CAPITAL LETTER SAN
   if (c == 0x03fa) {
      return 0x03fb;
   }
   if (c < 0x03fd) {
      return c;
   }
   // 3 characters
   if (c <= 0x03ff) {
      return c + -130;
   }
   if (c < 0x0400) {
      return c;
   }
   // 16 characters
   if (c <= 0x040f) {
      return c + 80;
   }
   if (c < 0x0410) {
      return c;
   }
   // 32 characters
   if (c <= 0x042f) {
      return c + 32;
   }
   if (c < 0x0460) {
      return c;}
   // 17 characters
   if (c <= 0x0480) {
      return c | 1;
   }
   if (c < 0x048a) {
      return c;
   }
   // 27 characters
   if (c <= 0x04be) {
      return c | 1;
   }
   // CYRILLIC LETTER PALOCHKA
   if (c == 0x04c0) {
      return 0x04cf;
   }
   if (c < 0x04c1) {
      return c;
   }
   // 7 characters
   if (c <= 0x04cd && c % 2 == 1) {
      return c + 1;
   }
   if (c < 0x04d0) {
      return c;
   }
   // 48 characters
   if (c <= 0x052e) {
      return c | 1;
   }
   if (c < 0x0531) {
      return c;
   }
   // 38 characters
   if (c <= 0x0556) {
      return c + 48;
   }
   if (c < 0x10a0) {
      return c;
   }
   // 38 characters
   if (c <= 0x10c5) {
      return c + 7264;
   }
   if (c < 0x10c7) {
      return c;
   }
   // 2 characters
   if (c <= 0x10cd && c % 6 == 5) {
      return c + 7264;
   }
   if (c < 0x13f8) {
      return c;
   }
   // 6 characters
   if (c <= 0x13fd) {
      return c + -8;
   }
   // CYRILLIC SMALL LETTER ROUNDED VE
   if (c == 0x1c80) {
      return 0x0432;
   }
   // CYRILLIC SMALL LETTER LONG-LEGGED DE
   if (c == 0x1c81) {
      return 0x0434;
   }
   // CYRILLIC SMALL LETTER NARROW O
   if (c == 0x1c82) {
      return 0x043e;
   }
   if (c < 0x1c83) {
      return c;
   }
   // 2 characters
   if (c <= 0x1c84) {
      return c + -6210;
   }
   // CYRILLIC SMALL LETTER THREE-LEGGED TE
   if (c == 0x1c85) {
      return 0x0442;
   }
   // CYRILLIC SMALL LETTER TALL HARD SIGN
   if (c == 0x1c86) {
      return 0x044a;
   }
   // CYRILLIC SMALL LETTER TALL YAT
   if (c == 0x1c87) {
      return 0x0463;
   }
   // CYRILLIC SMALL LETTER UNBLENDED UK
   if (c == 0x1c88) {
      return 0xa64b;
   }
   if (c < 0x1e00) {
      return c;}
   // 75 characters
   if (c <= 0x1e94) {
      return c | 1;
   }
   // LATIN SMALL LETTER LONG S WITH DOT ABOVE
   if (c == 0x1e9b) {
      return 0x1e61;
   }
   // LATIN CAPITAL LETTER SHARP S
   if (c == 0x1e9e) {
      return 0x00df;
   }
   if (c < 0x1ea0) {
      return c;}
   // 48 characters
   if (c <= 0x1efe) {
      return c | 1;
   }
   if (c < 0x1f08) {
      return c;
   }
   // 8 characters
   if (c <= 0x1f0f) {
      return c + -8;
   }
   if (c < 0x1f18) {
      return c;
   }
   // 6 characters
   if (c <= 0x1f1d) {
      return c + -8;
   }
   if (c < 0x1f28) {
      return c;
   }
   // 8 characters
   if (c <= 0x1f2f) {
      return c + -8;
   }
   if (c < 0x1f38) {
      return c;
   }
   // 8 characters
   if (c <= 0x1f3f) {
      return c + -8;
   }
   if (c < 0x1f48) {
      return c;
   }
   // 6 characters
   if (c <= 0x1f4d) {
      return c + -8;
   }
   if (c < 0x1f59) {
      return c;
   }
   // 4 characters
   if (c <= 0x1f5f && c % 2 == 1) {
      return c + -8;
   }
   if (c < 0x1f68) {
      return c;
   }
   // 8 characters
   if (c <= 0x1f6f) {
      return c + -8;
   }
   if (c < 0x1f88) {
      return c;
   }
   // 8 characters
   if (c <= 0x1f8f) {
      return c + -8;
   }
   if (c < 0x1f98) {
      return c;
   }
   // 8 characters
   if (c <= 0x1f9f) {
      return c + -8;
   }
   if (c < 0x1fa8) {
      return c;
   }
   // 8 characters
   if (c <= 0x1faf) {
      return c + -8;
   }
   if (c < 0x1fb8) {
      return c;
   }
   // 2 characters
   if (c <= 0x1fb9) {
      return c + -8;
   }
   if (c < 0x1fba) {
      return c;
   }
   // 2 characters
   if (c <= 0x1fbb) {
      return c + -74;
   }
   // GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI
   if (c == 0x1fbc) {
      return 0x1fb3;
   }
   // GREEK PROSGEGRAMMENI
   if (c == 0x1fbe) {
      return 0x03b9;
   }
   if (c < 0x1fc8) {
      return c;
   }
   // 4 characters
   if (c <= 0x1fcb) {
      return c + -86;
   }
   // GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI
   if (c == 0x1fcc) {
      return 0x1fc3;
   }
   if (c < 0x1fd8) {
      return c;
   }
   // 2 characters
   if (c <= 0x1fd9) {
      return c + -8;
   }
   if (c < 0x1fda) {
      return c;
   }
   // 2 characters
   if (c <= 0x1fdb) {
      return c + -100;
   }
   if (c < 0x1fe8) {
      return c;
   }
   // 2 characters
   if (c <= 0x1fe9) {
      return c + -8;
   }
   if (c < 0x1fea) {
      return c;
   }
   // 2 characters
   if (c <= 0x1feb) {
      return c + -112;
   }
   // GREEK CAPITAL LETTER RHO WITH DASIA
   if (c == 0x1fec) {
      return 0x1fe5;
   }
   if (c < 0x1ff8) {
      return c;
   }
   // 2 characters
   if (c <= 0x1ff9) {
      return c + -128;
   }
   if (c < 0x1ffa) {
      return c;
   }
   // 2 characters
   if (c <= 0x1ffb) {
      return c + -126;
   }
   // GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI
   if (c == 0x1ffc) {
      return 0x1ff3;
   }
   // OHM SIGN
   if (c == 0x2126) {
      return 0x03c9;
   }
   // KELVIN SIGN
   if (c == 0x212a) {
      return 0x006b;
   }
   // ANGSTROM SIGN
   if (c == 0x212b) {
      return 0x00e5;
   }
   // TURNED CAPITAL F
   if (c == 0x2132) {
      return 0x214e;
   }
   if (c < 0x2160) {
      return c;
   }
   // 16 characters
   if (c <= 0x216f) {
      return c + 16;
   }
   // ROMAN NUMERAL REVERSED ONE HUNDRED
   if (c == 0x2183) {
      return 0x2184;
   }
   if (c < 0x24b6) {
      return c;
   }
   // 26 characters
   if (c <= 0x24cf) {
      return c + 26;
   }
   if (c < 0x2c00) {
      return c;
   }
   // 47 characters
   if (c <= 0x2c2e) {
      return c + 48;
   }
   // LATIN CAPITAL LETTER L WITH DOUBLE BAR
   if (c == 0x2c60) {
      return 0x2c61;
   }
   // LATIN CAPITAL LETTER L WITH MIDDLE TILDE
   if (c == 0x2c62) {
      return 0x026b;
   }
   // LATIN CAPITAL LETTER P WITH STROKE
   if (c == 0x2c63) {
      return 0x1d7d;
   }
   // LATIN CAPITAL LETTER R WITH TAIL
   if (c == 0x2c64) {
      return 0x027d;
   }
   if (c < 0x2c67) {
      return c;
   }
   // 3 characters
   if (c <= 0x2c6b && c % 2 == 1) {
      return c + 1;
   }
   // LATIN CAPITAL LETTER ALPHA
   if (c == 0x2c6d) {
      return 0x0251;
   }
   // LATIN CAPITAL LETTER M WITH HOOK
   if (c == 0x2c6e) {
      return 0x0271;
   }
   // LATIN CAPITAL LETTER TURNED A
   if (c == 0x2c6f) {
      return 0x0250;
   }
   // LATIN CAPITAL LETTER TURNED ALPHA
   if (c == 0x2c70) {
      return 0x0252;
   }
   if (c < 0x2c72) {
      return c;
   }
   // 2 characters
   if (c <= 0x2c75 && c % 3 == 2) {
      return c + 1;
   }
   if (c < 0x2c7e) {
      return c;
   }
   // 2 characters
   if (c <= 0x2c7f) {
      return c + -10815;
   }
   if (c < 0x2c80) {
      return c;
   }
   // 50 characters
   if (c <= 0x2ce2) {
      return c | 1;
   }
   if (c < 0x2ceb) {
      return c;
   }
   // 2 characters
   if (c <= 0x2ced && c % 2 == 1) {
      return c + 1;
   }
   if (c < 0x2cf2) {
      return c;
   }
   // 2 characters
   if (c <= 0xa640 && c % 31054 == 11506) {
      return c + 1;
   }
   if (c < 0xa642) {
      return c;
   }
   // 22 characters
   if (c <= 0xa66c) {
      return c | 1;
   }
   if (c < 0xa680) {
      return c;
   }
   // 14 characters
   if (c <= 0xa69a) {
      return c | 1;
   }
   if (c < 0xa722) {
      return c;
   }
   // 7 characters
   if (c <= 0xa72e) {
      return c | 1;
   }
   if (c < 0xa732) {
      return c;
   }
   // 31 characters
   if (c <= 0xa76e) {
      return c | 1;
   }
   if (c < 0xa779) {
      return c;
   }
   // 2 characters
   if (c <= 0xa77b && c % 2 == 1) {
      return c + 1;
   }
   // LATIN CAPITAL LETTER INSULAR G
   if (c == 0xa77d) {
      return 0x1d79;
   }
   if (c < 0xa77e) {
      return c;
   }
   // 5 characters
   if (c <= 0xa786) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER SALTILLO
   if (c == 0xa78b) {
      return 0xa78c;
   }
   // LATIN CAPITAL LETTER TURNED H
   if (c == 0xa78d) {
      return 0x0265;
   }
   if (c < 0xa790) {
      return c;
   }
   // 2 characters
   if (c <= 0xa792) {
      return c | 1;
   }
   if (c < 0xa796) {
      return c;
   }
   // 10 characters
   if (c <= 0xa7a8) {
      return c | 1;
   }
   // LATIN CAPITAL LETTER H WITH HOOK
   if (c == 0xa7aa) {
      return 0x0266;
   }
   // LATIN CAPITAL LETTER REVERSED OPEN E
   if (c == 0xa7ab) {
      return 0x025c;
   }
   // LATIN CAPITAL LETTER SCRIPT G
   if (c == 0xa7ac) {
      return 0x0261;
   }
   // LATIN CAPITAL LETTER L WITH BELT
   if (c == 0xa7ad) {
      return 0x026c;
   }
   // LATIN CAPITAL LETTER SMALL CAPITAL I
   if (c == 0xa7ae) {
      return 0x026a;
   }
   // LATIN CAPITAL LETTER TURNED K
   if (c == 0xa7b0) {
      return 0x029e;
   }
   // LATIN CAPITAL LETTER TURNED T
   if (c == 0xa7b1) {
      return 0x0287;
   }
   // LATIN CAPITAL LETTER J WITH CROSSED-TAIL
   if (c == 0xa7b2) {
      return 0x029d;
   }
   // LATIN CAPITAL LETTER CHI
   if (c == 0xa7b3) {
      return 0xab53;
   }
   if (c < 0xa7b4) {
      return c;
   }
   // 2 characters
   if (c <= 0xa7b6) {
      return c | 1;
   }
   if (c < 0xab70) {
      return c;
   }
   // 80 characters
   if (c <= 0xabbf) {
      return c + -38864;}
   if (c < 0xff21) {
      return c;
   }
   // 26 characters
   if (c <= 0xff3a) {
      return c + 32;
   }
   if (c < 0x10400) {
      return c;
   }
   // 40 characters
   if (c <= 0x10427) {
      return c + 40;
   }
   if (c < 0x104b0) {
      return c;
   }
   // 36 characters
   if (c <= 0x104d3) {
      return c + 40;
   }
   if (c < 0x10c80) {
      return c;
   }
   // 51 characters
   if (c <= 0x10cb2) {
      return c + 64;
   }
   if (c < 0x118a0) {
      return c;
   }
   // 32 characters
   if (c <= 0x118bf) {
      return c + 32;
   }
   if (c < 0x1e900) {
      return c;
   }
   // 34 characters
   if (c <= 0x1e921) {
      return c + 34;
   }

   return c;
}

} // unicode
} // sys
} // polar
