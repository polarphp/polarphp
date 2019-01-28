// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/26.

//===----------------------------------------------------------------------===//
//
// This file implements an efficient scoped hash table, which is useful for
// things like dominator-based optimizations.  This allows clients to do things
// like this:
//
//  ScopedHashTable<int, int> HT;
//  {
//    ScopedHashTableScope<int, int> Scope1(HT);
//    HT.insert(0, 0);
//    HT.insert(1, 1);
//    {
//      ScopedHashTableScope<int, int> Scope2(HT);
//      HT.insert(0, 42);
//    }
//  }
//
// Looking up the value for "0" in the Scope2 block will return 42.  Looking
// up the value for 0 before 42 is inserted or after Scope2 is popped will
// return 0.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_SET_OPERATIONS_H
#define POLARPHP_BASIC_SET_OPERATIONS_H

namespace polar {
namespace basic {

/// set_union(A, B) - Compute A := A u B, return whether A changed.
///
template <class S1Ty, class S2Ty>
bool set_union(S1Ty &lhs, const S2Ty &rhs)
{
   bool changed = false;
   for (typename S2Ty::const_iterator iter = rhs.begin(), end = rhs.end();
        iter != end; ++iter) {
      if (lhs.insert(*iter).second) {
         changed = true;
      }
   }
   return changed;
}

/// set_intersect(A, B) - Compute A := A ^ B
/// Identical to set_intersection, except that it works on set<>'s and
/// is nicer to use.  Functionally, this iterates through S1, removing
/// elements that are not contained in S2.
///
template <class S1Ty, class S2Ty>
void set_intersect(S1Ty &lhs, const S2Ty &rhs)
{
   for (typename S1Ty::iterator iter = lhs.begin(); iter != lhs.end();) {
      const auto &elem = *iter;
      ++iter;
      if (!rhs.count(elem)) {
         lhs.erase(elem);   // Erase element if not in S2
      }
   }
}

/// set_difference(A, B) - Return A - B
///
template <class S1Ty, class S2Ty>
S1Ty set_difference(const S1Ty &lhs, const S2Ty &rhs)
{
   S1Ty result;
   for (typename S1Ty::const_iterator iter = lhs.begin(), end = lhs.end();
        iter != end; ++iter) {
      if (!rhs.count(*iter)) {       // if the element is not in set2
         result.insert(*iter);
      }
   }
   return result;
}

/// set_subtract(A, B) - Compute A := A - B
///
template <class S1Ty, class S2Ty>
void set_subtract(S1Ty &lhs, const S2Ty &rhs)
{
   for (typename S2Ty::const_iterator iter = rhs.begin(), end = rhs.end();
        iter != end; ++iter) {
      lhs.erase(*iter);
   }
}

} // basic
} // polar

#endif // POLARPHP_BASIC_SET_OPERATIONS_H
