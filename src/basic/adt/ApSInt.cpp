// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/16.

//===----------------------------------------------------------------------===//
//
// This file implements the ApSInt class, which is a simple class that
// represents an arbitrary sized integer that knows its signedness.
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/adt/ApSInt.h"
#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace basic {

ApSInt::ApSInt(StringRef str)
{
   assert(!str.empty() && "Invalid string length");

   // (Over-)estimate the required number of bits.
   unsigned numBits = ((str.getSize() * 64) / 19) + 2;
   ApInt temp(numBits, str, /*Radix=*/10);
   if (str[0] == '-') {
      unsigned minBits = temp.getMinSignedBits();
      if (minBits > 0 && minBits < numBits) {
         temp = temp.trunc(minBits);
      }
      *this = ApSInt(temp, /*IsUnsigned=*/false);
      return;
   }
   unsigned activeBits = temp.getActiveBits();
   if (activeBits > 0 && activeBits < numBits) {
      temp = temp.trunc(activeBits);
   }
   *this = ApSInt(temp, /*IsUnsigned=*/true);
}

void ApSInt::profile(FoldingSetNodeId &id) const
{
   id.addInteger((unsigned) (m_isUnsigned ? 1 : 0));
   ApInt::profile(id);
}

} // basic
} // polar
