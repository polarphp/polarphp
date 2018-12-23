// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/24.

//===----------------------------------------------------------------------===//
//
// This code is taken from public domain
// (http://oauth.googlecode.com/svn/code/c/liboauth/src/Sha1.c and
// http://cvsweb.netbsd.org/bsdweb.cgi/src/common/lib/libc/hash/Sha1/Sha1.c?rev=1.6)
// and modified by wrapping it in a C++ interface for polarVM,
// and removing unnecessary code.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/Sha1.h"
#include "polarphp/utils/Host.h"
#include "polarphp/basic/adt/ArrayRef.h"

namespace polar {
namespace utils {

#include <stdint.h>
#include <string.h>

#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN
#define SHA_BIG_ENDIAN
#endif

namespace {
static uint32_t rol(uint32_t number, int bits)
{
   return (number << bits) | (number >> (32 - bits));
}

uint32_t blk0(uint32_t *buffer, int index)
{
   return buffer[index];
}

uint32_t blk(uint32_t *buffer, int index)
{
   buffer[index & 15] = rol(buffer[(index + 13) & 15] ^ buffer[(index + 8) & 15] ^ buffer[(index + 2) & 15] ^
         buffer[index & 15],
         1);
   return buffer[index & 15];
}

void r0(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d, uint32_t &e,
        int index, uint32_t *buffer)
{
   e += ((b & (c ^ d)) ^ d) + blk0(buffer, index) + 0x5a827999 + rol(a, 5);
   b = rol(b, 30);
}

void r1(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d, uint32_t &e,
        int index, uint32_t *buffer)
{
   e += ((b & (c ^ d)) ^ d) + blk(buffer, index) + 0x5A827999 + rol(a, 5);
   b = rol(b, 30);
}

void r2(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d, uint32_t &e,
        int index, uint32_t *buffer)
{
   e += (b ^ c ^ d) + blk(buffer, index) + 0x6ED9EBA1 + rol(a, 5);
   b = rol(b, 30);
}

void r3(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d, uint32_t &e,
        int index, uint32_t *buffer)
{
   e += (((b | c) & d) | (b & c)) + blk(buffer, index) + 0x8F1BBCDC + rol(a, 5);
   b = rol(b, 30);
}

void r4(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d, uint32_t &e,
        int index, uint32_t *buffer)
{
   e += (b ^ c ^ d) + blk(buffer, index) + 0xCA62C1D6 + rol(a, 5);
   b = rol(b, 30);
}
} // anonymous namespace

/* code */
#define Sha1_K0 0x5a827999
#define Sha1_K20 0x6ed9eba1
#define Sha1_K40 0x8f1bbcdc
#define Sha1_K60 0xca62c1d6

#define SEED_0 0x67452301
#define SEED_1 0xefcdab89
#define SEED_2 0x98badcfe
#define SEED_3 0x10325476
#define SEED_4 0xc3d2e1f0

void Sha1::init()
{
   m_internalState.m_state[0] = SEED_0;
   m_internalState.m_state[1] = SEED_1;
   m_internalState.m_state[2] = SEED_2;
   m_internalState.m_state[3] = SEED_3;
   m_internalState.m_state[4] = SEED_4;
   m_internalState.m_byteCount = 0;
   m_internalState.m_bufferOffset = 0;
}

void Sha1::hashBlock()
{
   uint32_t A = m_internalState.m_state[0];
   uint32_t B = m_internalState.m_state[1];
   uint32_t C = m_internalState.m_state[2];
   uint32_t D = m_internalState.m_state[3];
   uint32_t E = m_internalState.m_state[4];

   // 4 rounds of 20 operations each. Loop unrolled.
   r0(A, B, C, D, E, 0, m_internalState.m_buffer.L);
   r0(E, A, B, C, D, 1, m_internalState.m_buffer.L);
   r0(D, E, A, B, C, 2, m_internalState.m_buffer.L);
   r0(C, D, E, A, B, 3, m_internalState.m_buffer.L);
   r0(B, C, D, E, A, 4, m_internalState.m_buffer.L);
   r0(A, B, C, D, E, 5, m_internalState.m_buffer.L);
   r0(E, A, B, C, D, 6, m_internalState.m_buffer.L);
   r0(D, E, A, B, C, 7, m_internalState.m_buffer.L);
   r0(C, D, E, A, B, 8, m_internalState.m_buffer.L);
   r0(B, C, D, E, A, 9, m_internalState.m_buffer.L);
   r0(A, B, C, D, E, 10, m_internalState.m_buffer.L);
   r0(E, A, B, C, D, 11, m_internalState.m_buffer.L);
   r0(D, E, A, B, C, 12, m_internalState.m_buffer.L);
   r0(C, D, E, A, B, 13, m_internalState.m_buffer.L);
   r0(B, C, D, E, A, 14, m_internalState.m_buffer.L);
   r0(A, B, C, D, E, 15, m_internalState.m_buffer.L);
   r1(E, A, B, C, D, 16, m_internalState.m_buffer.L);
   r1(D, E, A, B, C, 17, m_internalState.m_buffer.L);
   r1(C, D, E, A, B, 18, m_internalState.m_buffer.L);
   r1(B, C, D, E, A, 19, m_internalState.m_buffer.L);

   r2(A, B, C, D, E, 20, m_internalState.m_buffer.L);
   r2(E, A, B, C, D, 21, m_internalState.m_buffer.L);
   r2(D, E, A, B, C, 22, m_internalState.m_buffer.L);
   r2(C, D, E, A, B, 23, m_internalState.m_buffer.L);
   r2(B, C, D, E, A, 24, m_internalState.m_buffer.L);
   r2(A, B, C, D, E, 25, m_internalState.m_buffer.L);
   r2(E, A, B, C, D, 26, m_internalState.m_buffer.L);
   r2(D, E, A, B, C, 27, m_internalState.m_buffer.L);
   r2(C, D, E, A, B, 28, m_internalState.m_buffer.L);
   r2(B, C, D, E, A, 29, m_internalState.m_buffer.L);
   r2(A, B, C, D, E, 30, m_internalState.m_buffer.L);
   r2(E, A, B, C, D, 31, m_internalState.m_buffer.L);
   r2(D, E, A, B, C, 32, m_internalState.m_buffer.L);
   r2(C, D, E, A, B, 33, m_internalState.m_buffer.L);
   r2(B, C, D, E, A, 34, m_internalState.m_buffer.L);
   r2(A, B, C, D, E, 35, m_internalState.m_buffer.L);
   r2(E, A, B, C, D, 36, m_internalState.m_buffer.L);
   r2(D, E, A, B, C, 37, m_internalState.m_buffer.L);
   r2(C, D, E, A, B, 38, m_internalState.m_buffer.L);
   r2(B, C, D, E, A, 39, m_internalState.m_buffer.L);

   r3(A, B, C, D, E, 40, m_internalState.m_buffer.L);
   r3(E, A, B, C, D, 41, m_internalState.m_buffer.L);
   r3(D, E, A, B, C, 42, m_internalState.m_buffer.L);
   r3(C, D, E, A, B, 43, m_internalState.m_buffer.L);
   r3(B, C, D, E, A, 44, m_internalState.m_buffer.L);
   r3(A, B, C, D, E, 45, m_internalState.m_buffer.L);
   r3(E, A, B, C, D, 46, m_internalState.m_buffer.L);
   r3(D, E, A, B, C, 47, m_internalState.m_buffer.L);
   r3(C, D, E, A, B, 48, m_internalState.m_buffer.L);
   r3(B, C, D, E, A, 49, m_internalState.m_buffer.L);
   r3(A, B, C, D, E, 50, m_internalState.m_buffer.L);
   r3(E, A, B, C, D, 51, m_internalState.m_buffer.L);
   r3(D, E, A, B, C, 52, m_internalState.m_buffer.L);
   r3(C, D, E, A, B, 53, m_internalState.m_buffer.L);
   r3(B, C, D, E, A, 54, m_internalState.m_buffer.L);
   r3(A, B, C, D, E, 55, m_internalState.m_buffer.L);
   r3(E, A, B, C, D, 56, m_internalState.m_buffer.L);
   r3(D, E, A, B, C, 57, m_internalState.m_buffer.L);
   r3(C, D, E, A, B, 58, m_internalState.m_buffer.L);
   r3(B, C, D, E, A, 59, m_internalState.m_buffer.L);

   r4(A, B, C, D, E, 60, m_internalState.m_buffer.L);
   r4(E, A, B, C, D, 61, m_internalState.m_buffer.L);
   r4(D, E, A, B, C, 62, m_internalState.m_buffer.L);
   r4(C, D, E, A, B, 63, m_internalState.m_buffer.L);
   r4(B, C, D, E, A, 64, m_internalState.m_buffer.L);
   r4(A, B, C, D, E, 65, m_internalState.m_buffer.L);
   r4(E, A, B, C, D, 66, m_internalState.m_buffer.L);
   r4(D, E, A, B, C, 67, m_internalState.m_buffer.L);
   r4(C, D, E, A, B, 68, m_internalState.m_buffer.L);
   r4(B, C, D, E, A, 69, m_internalState.m_buffer.L);
   r4(A, B, C, D, E, 70, m_internalState.m_buffer.L);
   r4(E, A, B, C, D, 71, m_internalState.m_buffer.L);
   r4(D, E, A, B, C, 72, m_internalState.m_buffer.L);
   r4(C, D, E, A, B, 73, m_internalState.m_buffer.L);
   r4(B, C, D, E, A, 74, m_internalState.m_buffer.L);
   r4(A, B, C, D, E, 75, m_internalState.m_buffer.L);
   r4(E, A, B, C, D, 76, m_internalState.m_buffer.L);
   r4(D, E, A, B, C, 77, m_internalState.m_buffer.L);
   r4(C, D, E, A, B, 78, m_internalState.m_buffer.L);
   r4(B, C, D, E, A, 79, m_internalState.m_buffer.L);

   m_internalState.m_state[0] += A;
   m_internalState.m_state[1] += B;
   m_internalState.m_state[2] += C;
   m_internalState.m_state[3] += D;
   m_internalState.m_state[4] += E;
}

void Sha1::addUncounted(uint8_t data)
{
#ifdef SHA_BIG_ENDIAN
   m_internalState.m_buffer.C[m_internalState.m_bufferOffset] = data;
#else
   m_internalState.m_buffer.C[m_internalState.m_bufferOffset ^ 3] = data;
#endif

   m_internalState.m_bufferOffset++;
   if (m_internalState.m_bufferOffset == BLOCK_LENGTH) {
      hashBlock();
      m_internalState.m_bufferOffset = 0;
   }
}

void Sha1::writebyte(uint8_t data)
{
   ++m_internalState.m_byteCount;
   addUncounted(data);
}

void Sha1::update(ArrayRef<uint8_t> data)
{
   for (auto &c : data) {
      writebyte(c);
   }
}

void Sha1::pad()
{
   // Implement SHA-1 padding (fips180-2 5.1.1)

   // Pad with 0x80 followed by 0x00 until the end of the block
   addUncounted(0x80);
   while (m_internalState.m_bufferOffset != 56) {
      addUncounted(0x00);
   }
   // Append length in the last 8 bytes
   addUncounted(0); // We're only using 32 bit lengths
   addUncounted(0); // But SHA-1 supports 64 bit lengths
   addUncounted(0); // So zero pad the top bits
   addUncounted(m_internalState.m_byteCount >> 29); // Shifting to multiply by 8
   addUncounted(m_internalState.m_byteCount >>
                21); // as SHA-1 supports bitstreams as well as
   addUncounted(m_internalState.m_byteCount >> 13); // byte.
   addUncounted(m_internalState.m_byteCount >> 5);
   addUncounted(m_internalState.m_byteCount << 3);
}

StringRef Sha1::final()
{
   // Pad to complete the last block
   pad();

#ifdef SHA_BIG_ENDIAN
   // Just copy the current state
   for (int i = 0; i < 5; i++) {
      m_hashResult[i] = m_internalState.m_state[i];
   }
#else
   // Swap byte order back
   for (int i = 0; i < 5; i++) {
      m_hashResult[i] = (((m_internalState.m_state[i]) << 24) & 0xff000000) |
            (((m_internalState.m_state[i]) << 8) & 0x00ff0000) |
            (((m_internalState.m_state[i]) >> 8) & 0x0000ff00) |
            (((m_internalState.m_state[i]) >> 24) & 0x000000ff);
   }
#endif

   // Return pointer to hash (20 characters)
   return StringRef((char *)m_hashResult, HASH_LENGTH);
}

StringRef Sha1::result()
{
   auto StateToRestore = m_internalState;
   auto hash = final();
   // Restore the state
   m_internalState = StateToRestore;
   // Return pointer to hash (20 characters)
   return hash;
}

std::array<uint8_t, 20> Sha1::hash(ArrayRef<uint8_t> data)
{
   Sha1 hash;
   hash.update(data);
   StringRef str = hash.final();
   std::array<uint8_t, 20> array;
   memcpy(array.data(), str.getData(), str.getSize());
   return array;
}

} // utils
} // polar
