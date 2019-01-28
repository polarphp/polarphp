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
// This code is taken from public domain
// (http://oauth.googlecode.com/svn/code/c/liboauth/src/Sha1.c)
// and modified by wrapping it in a C++ interface for LLVM,
// and removing unnecessary code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_SHA1_H
#define POLARPHP_UTILS_SHA1_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StringRef.h"
#include <array>
#include <cstdint>

namespace polar {

// forward declare class with namespace
namespace basic {
template <typename T>
class ArrayRef;
} //basic

namespace utils {

using polar::basic::ArrayRef;
using polar::basic::StringRef;

/// A class that wrap the Sha1 algorithm.
class Sha1
{
public:
   Sha1()
   {
      init();
   }

   /// Reinitialize the internal state
   void init();

   /// Digest more data.
   void update(ArrayRef<uint8_t> data);

   /// Digest more data.
   void update(StringRef str)
   {
      update(ArrayRef<uint8_t>((uint8_t *)const_cast<char *>(str.getData()),
                               str.getSize()));
   }

   /// Return a reference to the current raw 160-bits Sha1 for the digested data
   /// since the last call to init(). This call will add data to the internal
   /// state and as such is not suited for getting an intermediate result
   /// (see result()).
   StringRef final();

   /// Return a reference to the current raw 160-bits Sha1 for the digested data
   /// since the last call to init(). This is suitable for getting the Sha1 at
   /// any time without invalidating the internal state so that more calls can be
   /// made into update.
   StringRef result();

   /// Returns a raw 160-bit Sha1 hash for the given data.
   static std::array<uint8_t, 20> hash(ArrayRef<uint8_t> data);

private:
   /// Define some constants.
   /// "static constexpr" would be cleaner but MSVC does not support it yet.
   enum { BLOCK_LENGTH = 64 };
   enum { HASH_LENGTH = 20 };

   // Internal State
   struct {
      union {
         uint8_t C[BLOCK_LENGTH];
         uint32_t L[BLOCK_LENGTH / 4];
      } m_buffer;
      uint32_t m_state[HASH_LENGTH / 4];
      uint32_t m_byteCount;
      uint8_t m_bufferOffset;
   } m_internalState;

   // Internal copy of the hash, populated and accessed on calls to result()
   uint32_t m_hashResult[HASH_LENGTH / 4];

   // Helper
   void writebyte(uint8_t data);
   void hashBlock();
   void addUncounted(uint8_t data);
   void pad();
};

} // basic
} // utils

#endif // POLARPHP_UTILS_SHA1_H
