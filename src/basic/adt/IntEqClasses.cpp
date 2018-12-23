// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018  polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of  polarphp project authors
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

#include "polarphp/basic/adt/IntEqClasses.h"

namespace polar {
namespace basic {

void IntEqClasses::grow(unsigned size)
{
   assert(m_numClasses == 0 && "grow() called after compress().");
   m_eqClasses.reserve(size);
   while (m_eqClasses.getSize() < size) {
      m_eqClasses.push_back(m_eqClasses.getSize());
   }
}

unsigned IntEqClasses::join(unsigned lhs, unsigned rhs)
{
   assert(m_numClasses == 0 && "join() called after compress().");
   unsigned eca = m_eqClasses[lhs];
   unsigned ecb = m_eqClasses[rhs];
   // Update pointers while searching for the leaders, compressing the paths
   // incrementally. The larger leader will eventually be updated, joining the
   // classes.
   while (eca != ecb) {
      if (eca < ecb) {
         m_eqClasses[rhs] = eca;
         rhs = ecb;
         ecb = m_eqClasses[rhs];
      } else {
         m_eqClasses[lhs] = ecb;
         lhs = eca;
         eca = m_eqClasses[lhs];
      }
   }
   return eca;
}

unsigned IntEqClasses::findLeader(unsigned index) const
{
   assert(m_numClasses == 0 && "findLeader() called after compress().");
   while (index != m_eqClasses[index]) {
      index = m_eqClasses[index];
   }
   return index;
}

void IntEqClasses::compress()
{
   if (m_numClasses) {
      return;
   }
   for (unsigned i = 0, e = m_eqClasses.getSize(); i != e; ++i) {
      m_eqClasses[i] = (m_eqClasses[i] == i) ? m_numClasses++ : m_eqClasses[m_eqClasses[i]];
   }
}

void IntEqClasses::uncompress()
{
   if (!m_numClasses) {
      return;
   }
   SmallVector<unsigned, 8> leader;
   for (unsigned i = 0, e = m_eqClasses.getSize(); i != e; ++i) {
      if (m_eqClasses[i] < leader.getSize()) {
         m_eqClasses[i] = leader[m_eqClasses[i]];
      } else {
         leader.push_back(m_eqClasses[i] = i);
      }
   }
   m_numClasses = 0;
}

} // basic
} // polar
