//===--- ValueEnumerator.h --- Enumerates values ----------------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.

#ifndef POLARPHP_BASIC_VALUE_ENUMERATOR_H
#define POLARPHP_BASIC_VALUE_ENUMERATOR_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"

namespace polar {

/// / This class maps values to unique indices.
template<class ValueTy, class IndexTy = size_t>
class ValueEnumerator
{
   /// A running m_counter to enumerate values.
   IndexTy m_counter = 0;

   /// Maps values to unique integers.
   llvm::DenseMap<ValueTy, IndexTy> m_valueToIndex;

public:
   /// Return the index of value \p v.
   IndexTy getIndex(const ValueTy &v)
   {
      // Return the index of this Key, if we've assigned one already.
      auto iter = m_valueToIndex.find(v);
      if (iter != m_valueToIndex.end()) {
         return iter->second;
      }
      // Generate a new m_counter for the key.
      m_valueToIndex[v] = ++m_counter;
      return m_counter;
   }

   ValueEnumerator() = default;

   /// Forget about key \p v.
   void invalidateValue(const ValueTy &v)
   {
      m_valueToIndex.erase(v);
   }

   /// Clear the enumeration state of the
   void clear()
   {
      m_valueToIndex.clear();
      m_counter = 0;
   }
};

} // polar

#endif // POLARPHP_BASIC_VALUE_ENUMERATOR_H
