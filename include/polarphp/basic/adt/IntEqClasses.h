// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/06/24.

//===----------------------------------------------------------------------===//
//
// Equivalence classes for small integers. This is a mapping of the integers
// 0 .. N-1 into M equivalence classes numbered 0 .. M-1.
//
// Initially each integer has its own equivalence class. Classes are joined by
// passing a representative member of each class to join().
//
// Once the classes are built, compress() will number them 0 .. M-1 and prevent
// further changes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ADT_INT_EQ_CLASSES_H
#define POLARPHP_BASIC_ADT_INT_EQ_CLASSES_H

#include "polarphp/basic/adt/SmallVector.h"

namespace polar {
namespace basic {

class IntEqClasses
{
   /// EC - When uncompressed, map each integer to a smaller member of its
   /// equivalence class. The class leader is the smallest member and maps to
   /// itself.
   ///
   /// When compressed, EC[i] is the equivalence class of i.
   SmallVector<unsigned, 8> m_eqClasses;

   /// NumClasses - The number of equivalence classes when compressed, or 0 when
   /// uncompressed.
   unsigned m_numClasses;

public:
   /// IntEqClasses - Create an equivalence class mapping for 0 .. N-1.
   IntEqClasses(unsigned size = 0) : m_numClasses(0)
   {
      grow(size);
   }

   /// grow - Increase capacity to hold 0 .. N-1, putting new integers in unique
   /// equivalence classes.
   /// This requires an uncompressed map.
   void grow(unsigned size);

   /// clear - Clear all classes so that grow() will assign a unique class to
   /// every integer.
   void clear()
   {
      m_eqClasses.clear();
      m_numClasses = 0;
   }

   /// Join the equivalence classes of a and b. After joining classes,
   /// findLeader(a) == findLeader(b). This requires an uncompressed map.
   /// Returns the new leader.
   unsigned join(unsigned lhs, unsigned rhs);

   /// findLeader - Compute the leader of a's equivalence class. This is the
   /// smallest member of the class.
   /// This requires an uncompressed map.
   unsigned findLeader(unsigned index) const;

   /// compress - Compress equivalence classes by numbering them 0 .. M.
   /// This makes the equivalence class map immutable.
   void compress();

   /// getNumClasses - Return the number of equivalence classes after compress()
   /// was called.
   unsigned getNumClasses() const
   {
      return m_numClasses;
   }

   /// operator[] - Return a's equivalence class number, 0 .. getNumClasses()-1.
   /// This requires a compressed map.
   unsigned operator[](unsigned index) const
   {
      assert(m_numClasses && "operator[] called before compress()");
      return m_eqClasses[index];
   }

   /// uncompress - Change back to the uncompressed representation that allows
   /// editing.
   void uncompress();
};

} // basic
} // polar


#endif //  POLARPHP_BASIC_ADT_INT_EQ_CLASSES_H
