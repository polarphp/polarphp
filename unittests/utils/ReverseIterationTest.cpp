// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/ReverseIteration.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/DenseMapInfo.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::utils;
using namespace polar::basic;

namespace {

TEST(ReverseIterationTest, testDenseMapTest1)
{
   static_assert(polar::utils::internal::IsPointerLike<int *>::value,
                 "int * is pointer-like");
   static_assert(polar::utils::internal::IsPointerLike<uintptr_t>::value,
                 "uintptr_t is pointer-like");
   static_assert(!polar::utils::internal::IsPointerLike<int>::value,
                 "int is not pointer-like");
   static_assert(polar::utils::internal::IsPointerLike<void *>::value,
                 "void * is pointer-like");
   struct IncompleteType;
   static_assert(polar::utils::internal::IsPointerLike<IncompleteType *>::value,
                 "incomplete * is pointer-like");

   // For a DenseMap with non-pointer-like keys, forward iteration equals
   // reverse iteration.
   DenseMap<int, int> map;
   int keys[] = { 1, 2, 3, 4 };

   // Insert keys into the DenseMap.
   for (auto key: keys) {
      map[key] = 0;
   }
   // Note: This is the observed order of keys in the DenseMap.
   // If there is any change in the behavior of the DenseMap, this order
   // would need to be adjusted accordingly.
   int iterKeys[] = { 2, 4, 1, 3 };

   // Check that the DenseMap is iterated in the expected order.
   for (const auto &Tuple : zip(map, iterKeys)) {
      ASSERT_EQ(std::get<0>(Tuple).first, std::get<1>(Tuple));
   }
   // Check operator++ (post-increment).
   int i = 0;
   for (auto iter = map.begin(), end = map.end(); iter != end; iter++, ++i) {
      ASSERT_EQ(iter->first, iterKeys[i]);
   }
}
} // anonymous namespace

// Define a pointer-like int.
struct PtrLikeInt { int value; };

namespace polar {
namespace basic {

template<>
struct DenseMapInfo<PtrLikeInt *>
{
   static PtrLikeInt *getEmptyKey()
   {
      static PtrLikeInt EmptyKey;
      return &EmptyKey;
   }

   static PtrLikeInt *getTombstoneKey()
   {
      static PtrLikeInt tombstoneKey;
      return &tombstoneKey;
   }

   static int getHashValue(const PtrLikeInt *P)
   {
      return P->value;
   }

   static bool isEqual(const PtrLikeInt *lhs, const PtrLikeInt *rhs)
   {
      return lhs == rhs;
   }
};
} // basic
} // end namespace polar

namespace {

TEST(ReverseIterationTest, testDenseMapTest2)
{
   static_assert(polar::utils::internal::IsPointerLike<PtrLikeInt *>::value,
                 "PtrLikeInt * is pointer-like");

   PtrLikeInt a = {4}, b = {8}, c = {12}, d = {16};
   PtrLikeInt *keys[] = { &a, &b, &c, &d };

   // Insert keys into the DenseMap.
   DenseMap<PtrLikeInt *, int> map;
   for (auto *key : keys) {
      map[key] = key->value;
   }

   // Note: If there is any change in the behavior of the DenseMap,
   // the observed order of keys would need to be adjusted accordingly.
   if (should_reverse_iterate<PtrLikeInt *>()) {
      std::reverse(&keys[0], &keys[4]);
   }

   // Check that the DenseMap is iterated in the expected order.
   for (const auto &Tuple : zip(map, keys)) {
      ASSERT_EQ(std::get<0>(Tuple).second, std::get<1>(Tuple)->value);
   }

   // Check operator++ (post-increment).
   int i = 0;
   for (auto iter = map.begin(), end = map.end(); iter != end; iter++, ++i) {
      ASSERT_EQ(iter->second, keys[i]->value);
   }

}

} // anonymous namespace
