// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/18.

//===----------------------------------------------------------------------===//
//
// This file defines a Levenshtein distance function that works for any two
// sequences, with each element of each sequence being analogous to a character
// in a string.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ADT_EDIT_DISTANCE_H
#define POLARPHP_BASIC_ADT_EDIT_DISTANCE_H

#include "polarphp/basic/adt/ArrayRef.h"
#include <algorithm>
#include <memory>

namespace polar {
namespace basic {

/// Determine the edit distance between two sequences.
///
/// \param fromArray the first sequence to compare.
///
/// \param toArray the second sequence to compare.
///
/// \param allowReplacements whether to allow element replacements (change one
/// element into another) as a single operation, rather than as two operations
/// (an insertion and a removal).
///
/// \param maxEditDistance If non-zero, the maximum edit distance that this
/// routine is allowed to compute. If the edit distance will exceed that
/// maximum, returns \c maxEditDistance+1.
///
/// \returns the minimum number of element insertions, removals, or (if
/// \p allowReplacements is \c true) replacements needed to transform one of
/// the given sequences into the other. If zero, the sequences are identical.
template<typename T>
unsigned compute_edit_distance(ArrayRef<T> fromArray, ArrayRef<T> toArray,
                               bool allowReplacements = true,
                               unsigned maxEditDistance = 0)
{
   // The algorithm implemented below is the "classic"
   // dynamic-programming algorithm for computing the Levenshtein
   // distance, which is described here:
   //
   //   http://en.wikipedia.org/wiki/Levenshtein_distance
   //
   // Although the algorithm is typically described using an m x n
   // array, only one row plus one element are used at a time, so this
   // implementation just keeps one vector for the row.  To update one entry,
   // only the entries to the left, top, and top-left are needed.  The left
   // entry is in Row[x-1], the top entry is what's in Row[x] from the last
   // iteration, and the top-left entry is stored in Previous.
   typename ArrayRef<T>::size_type m = fromArray.getSize();
   typename ArrayRef<T>::size_type n = toArray.getSize();

   const unsigned smallBufferSize = 64;
   unsigned smallBuffer[smallBufferSize];
   std::unique_ptr<unsigned[]> allocated;
   unsigned *row = smallBuffer;
   if (n + 1 > smallBufferSize) {
      row = new unsigned[n + 1];
      allocated.reset(row);
   }
   for (unsigned i = 1; i <= n; ++i) {
      row[i] = i;
   }
   for (typename ArrayRef<T>::size_type y = 1; y <= m; ++y) {
      row[0] = y;
      unsigned bestThisRow = row[0];
      unsigned previous = y - 1;
      for (typename ArrayRef<T>::size_type x = 1; x <= n; ++x) {
         int oldRow = row[x];
         if (allowReplacements) {
            row[x] = std::min(
                     previous + (fromArray[y-1] == toArray[x-1] ? 0u : 1u),
                  std::min(row[x-1], row[x])+1);
         } else {
            if (fromArray[y-1] == toArray[x-1]) {
               row[x] = previous;
            } else {
               row[x] = std::min(row[x-1], row[x]) + 1;
            }
         }
         previous = oldRow;
         bestThisRow = std::min(bestThisRow, row[x]);
      }

      if (maxEditDistance && bestThisRow > maxEditDistance) {
         return maxEditDistance + 1;
      }
   }
   unsigned result = row[n];
   return result;
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_EDIT_DISTANCE_H
