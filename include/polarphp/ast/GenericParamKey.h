//===--- GenericParamKey.h - Key for generic parameters ---------*- C++ -*-===//
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
// Created by polarboy on 2019/12/05.

#ifndef POLARPHP_AST_GENERIC_PARAMKEY_H
#define POLARPHP_AST_GENERIC_PARAMKEY_H

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "polarphp/ast/Type.h"

namespace polar {

class GenericTypeParamDecl;
class GenericTypeParamType;


/// A fully-abstracted generic type parameter key, maintaining only the depth
/// and index of the generic parameter.
struct GenericParamKey
{
   unsigned depth : 16;
   unsigned index : 16;

   GenericParamKey(unsigned depth, unsigned index)
      : depth(depth),
        index(index)
   {}

   GenericParamKey(const GenericTypeParamDecl *d);
   GenericParamKey(const GenericTypeParamType *d);

   friend bool operator==(GenericParamKey lhs, GenericParamKey rhs)
   {
      return lhs.depth == rhs.depth && lhs.index == rhs.index;
   }

   friend bool operator!=(GenericParamKey lhs, GenericParamKey rhs)
   {
      return !(lhs == rhs);
   }

   friend bool operator<(GenericParamKey lhs, GenericParamKey rhs)
   {
      return lhs.depth < rhs.depth ||
            (lhs.depth == rhs.depth && lhs.index < rhs.index);
   }

   friend bool operator>(GenericParamKey lhs, GenericParamKey rhs)
   {
      return rhs < lhs;
   }

   friend bool operator<=(GenericParamKey lhs, GenericParamKey rhs)
   {
      return !(rhs < lhs);
   }

   friend bool operator>=(GenericParamKey lhs, GenericParamKey rhs)
   {
      return !(lhs < rhs);
   }

   /// Function object type that can be used to provide an ordering of
   /// generic type parameter keys with themselves, generic type parameter
   /// declarations, and generic type parameter types.
   struct Ordering
   {
      bool operator()(GenericParamKey lhs, GenericParamKey rhs) const
      {
         return lhs < rhs;
      }

      bool operator()(GenericParamKey lhs,
                      const GenericTypeParamDecl *rhs) const
      {
         return (*this)(lhs, GenericParamKey(rhs));
      }

      bool operator()(const GenericTypeParamDecl *lhs,
                      GenericParamKey rhs) const
      {
         return (*this)(GenericParamKey(lhs), rhs);
      }

      bool operator()(GenericParamKey lhs,
                      const GenericTypeParamType *rhs) const
      {
         return (*this)(lhs, GenericParamKey(rhs));
      }

      bool operator()(const GenericTypeParamType *lhs,
                      GenericParamKey rhs) const
      {
         return (*this)(GenericParamKey(lhs), rhs);
      }
   };

   /// Find the index that this key would have into an array of
   /// generic type parameters
   unsigned findIndexIn(TypeArrayView<GenericTypeParamType> genericParams) const;
};

} // polar


namespace llvm {

template<>
struct DenseMapInfo<polar::GenericParamKey>
{
   static inline polar::GenericParamKey getEmptyKey()
   {
      return {0xFFFF, 0xFFFF};
   }

   static inline polar::GenericParamKey getTombstoneKey()
   {
      return {0xFFFE, 0xFFFE};
   }

   static inline unsigned getHashValue(polar::GenericParamKey key)
   {
      return DenseMapInfo<unsigned>::getHashValue(key.depth << 16 | key.index);
   }

   static bool isEqual(polar::GenericParamKey lhs,
                       polar::GenericParamKey rhs)
   {
      return lhs.depth == rhs.depth && lhs.index == rhs.index;
   }
};

} // end namespace llvm

#endif // POLARPHP_AST_GENERIC_PARAMKEY_H
