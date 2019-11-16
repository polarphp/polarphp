//===--- UUID.h - UUID generation -------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This is an interface over the standard OSF uuid library that gives UUIDs
// sane m_value semantics and operators.
//
//===----------------------------------------------------------------------===//

// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/13.

#ifndef POLARPHP_BASIC_UUID_H
#define POLARPHP_BASIC_UUID_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <array>

namespace polar::basic {

using llvm::raw_ostream;
using llvm::DenseMap;
using llvm::DenseMapInfo;
using llvm::SmallString;
using llvm::StringRef;
using llvm::SmallVectorImpl;

class UUID
{
public:
   enum {
      /// The number of bytes in a UUID's binary representation.
      Size = 16,

      /// The number of characters in a UUID's string representation.
      StringSize = 36,

      /// The number of bytes necessary to store a null-terminated UUID's string
      /// representation.
      StringBufferSize = StringSize + 1,
   };

   unsigned char m_value[Size];

private:
   enum FromRandom_t { FromRandom };
   enum FromTime_t { FromTime };

   UUID(FromRandom_t);

   UUID(FromTime_t);

public:
   /// Default constructor.
   UUID();

   UUID(std::array<unsigned char, Size> bytes)
   {
      memcpy(m_value, &bytes, Size);
   }

   /// Create a new random UUID from entropy (/dev/random).
   static UUID fromRandom()
   {
      return UUID(FromRandom);
   }

   /// Create a new pseudorandom UUID using the time, MAC address, and pid.
   static UUID fromTime()
   {
      return UUID(FromTime);
   }

   /// Parse a UUID from a C string.
   static std::optional<UUID> fromString(const char *s);

   /// Convert a UUID to its string representation.
   void toString(SmallVectorImpl<char> &out) const;

   int compare(UUID y) const;

#define COMPARE_UUID(op) \
   bool operator op(UUID y) { return compare(y) op 0; }

   COMPARE_UUID(==)
   COMPARE_UUID(!=)
   COMPARE_UUID(<)
   COMPARE_UUID(<=)
   COMPARE_UUID(>)
   COMPARE_UUID(>=)
#undef COMPARE_UUID
};

raw_ostream &operator<<(raw_ostream &outStream, UUID uuid);

} // polar

namespace llvm {
using polar::basic::UUID;
template<>
struct DenseMapInfo<UUID>
{
   static inline UUID getEmptyKey()
   {
      return {{{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
               0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}}};
   }
   static inline UUID getTombstoneKey()
   {
      return {{{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
               0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE}}};
   }

   static unsigned getHashm_value(UUID uuid)
   {
      union {
         UUID uu;
         unsigned words[4];
      } reinterpret = {uuid};
      return reinterpret.words[0] ^ reinterpret.words[1]
            ^ reinterpret.words[2] ^ reinterpret.words[3];
   }

   static bool isEqual(UUID a, UUID b)
   {
      return a == b;
   }
};
}

#endif // POLARPHP_BASIC_UUID_H
