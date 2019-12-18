//===--- ImmutablePointerSet.h ----------------------------------*- C++ -*-===//
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
///
/// \file
///
/// This file contains an implementation of a bump ptr allocated immutable
/// pointer set.
///
/// The target of this data structure are sets of pointers (with N < 100) that
/// are propagated through many basic blocks. These pointer sets will be merged
/// and copied far more than being created from an array.
///
/// Thus we assume the following constraints:
///
/// 1. Our set operations are purely additive. Given a set, one can only add
/// elements to it. One can not remove elements to it. This means we only
/// support construction of sets from arrays and concatenation of pointer sets.
///
/// 2. Our sets must always be ordered and be able to be iterated over
/// efficiently in that order.
///
/// 3. An O(log(n)) set contains method.
///
/// Beyond these constraints, we would like for our data structure to have the
/// following properties for performance reasons:
///
/// 1. Its memory should be bump ptr allocated. We like fast allocation.
///
/// 2. No destructors need to be called when the bump ptr allocator is being
/// destroyed. We like fast destruction and do not want to have to iterate over
/// potentially many of these sets and invoke destructors.
///
/// Thus our design is to represent our sets as bump ptr allocated arrays whose
/// elements are sorted and uniqued. The actual uniquing of the arrays
/// themselves is performed via folding set node.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_IMMUTABLEPOINTERSET_H
#define POLARPHP_BASIC_IMMUTABLEPOINTERSET_H

#include "polarphp/basic/StlExtras.h"
#include "polarphp/basic/NullablePtr.h"
#include "llvm/Support/Allocator.h"
#include "llvm/ADT/FoldingSet.h"
#include <algorithm>
#include <type_traits>

namespace polar {

template <typename PtrTy>
class ImmutablePointerSetFactory;

/// An immutable set of pointers. It is backed by a tail allocated sorted array
/// ref.
template <typename T>
class ImmutablePointerSet : public llvm::FoldingSetNode
{
   using PtrTy = typename std::add_pointer<T>::type;
   friend class ImmutablePointerSetFactory<T>;

   NullablePtr<ImmutablePointerSetFactory<T>> m_parentFactory;
   ArrayRef<PtrTy> m_data;

   ImmutablePointerSet(ImmutablePointerSetFactory<T> *m_parentFactory,
                       ArrayRef<PtrTy> newData)
      : m_parentFactory(m_parentFactory),
        m_data(newData) {}

public:
   ~ImmutablePointerSet() = default;
   ImmutablePointerSet(const ImmutablePointerSet &) = default;
   ImmutablePointerSet(ImmutablePointerSet &&) = default;

   ImmutablePointerSet &operator=(const ImmutablePointerSet &) = default;
   ImmutablePointerSet &operator=(ImmutablePointerSet &&) = default;

   bool operator==(const ImmutablePointerSet<T> &pset) const
   {
      // If this and P have different sizes, we can not be equivalent.
      if (size() != pset.size()) {
         return false;
      }
      // Ok, we now know that both have the same size. If one is empty, the other
      // must be as well, implying equality.
      if (empty()) {
         return true;
      }
      // Ok, both sets are not empty and the same number of elements. Compare
      // element wise.
      return std::equal(begin(), end(), pset.begin());
   }

   bool operator!=(const ImmutablePointerSet<T> &pset) const
   {
      return !(*this == pset);
   }

   unsigned count(PtrTy ptr) const
   {
      // This returns the first element >= ptr. Since we know that our array is
      // sorted and uniqued, ptr must be that element.
      auto lowerBound = std::lower_bound(begin(), end(), ptr);

      // If ptr is > than everything in the array, then we obviously have 0.
      if (lowerBound == end()) {
         return 0;
      }
      // Then check if ptr is > or ptr is ==. We only have ptr if we have == to.
      return *lowerBound == ptr;
   }

   using iterator = typename ArrayRef<PtrTy>::iterator;
   iterator begin() const
   {
      return m_data.begin();
   }

   iterator end() const
   {
      return m_data.end();
   }

   unsigned size() const
   {
      return m_data.size();
   }

   bool empty() const
   {
      return m_data.empty();
   }

   void profile(llvm::FoldingSetNodeID &id) const
   {
      assert(!m_data.empty() && "Should not profile empty ImmutablePointerSet");
      for (PtrTy ptr : m_data) {
         id.AddPointer(ptr);
      }
   }

   void Profile(llvm::FoldingSetNodeID &id) const
   {
      profile(id);
   }

   ImmutablePointerSet<T> *merge(ImmutablePointerSet<T> *other)
   {
      if (empty()) {
         return other;
      }
      if (other->empty()) {
         return this;
      }
      assert(other->m_parentFactory.get() == m_parentFactory.get());
      return m_parentFactory.get()->merge(this, other);
   }

   bool hasEmptyIntersection(const ImmutablePointerSet<T> *other) const
   {
      // If we are empty or other is empty, then there are automatically no
      // elements that could be in the intersection. Return true.
      if (empty() || other->empty()) {
         return true;
      }

      // Ok, at this point we know that both self and other are non-empty. They
      // can only have a non-empty intersection if they have elements in common.
      // Sadly it seems the STL does not have such a predicate that is
      // non-constructive in the algorithm library, so we implement it ourselves.
      auto lhsIter = begin();
      auto lhsEndIter = end();
      auto rhsIter = other->begin();
      auto rhsEndIter = other->end();

      // Our implementation is to perform a sorted merge like traversal of both
      // lists, always advancing the iterator with a smaller value. If we ever hit
      // an equality in between our iterators, we have a non-empty intersection.
      //
      // Until either of our iterators hits the end of our target arrays...
      while (lhsIter != lhsEndIter && rhsIter != rhsEndIter) {
         // If lhsIter is equivalent to rhsIter, then we have a non-empty intersection...
         // Early exit.
         if (*lhsIter == *rhsIter) {
            return false;
         }
         // Otherwise, if *lhsIter is less than *rhsIter, advance lhsIter.
         if (*lhsIter < *rhsIter) {
            ++lhsIter;
            continue;
         }

         // Otherwise, We know that *rhsIter < *lhsIter. Advance rhsIter.
         ++rhsIter;
      }
      // We did not have any overlapping intersection.
      return true;
   }
};

template <typename T>
class ImmutablePointerSetFactory
{
   using PtrTy = typename std::add_pointer<T>::type;

   using PtrSet = ImmutablePointerSet<T>;
   // This is computed out-of-line so that ImmutablePointerSetFactory is
   // treated as a complete type.
   static const unsigned AllocAlignment;

   llvm::BumpPtrAllocator &m_allocator;
   llvm::FoldingSetVector<PtrSet> m_set;
   static PtrSet sm_emptyPtrSet;

public:
   ImmutablePointerSetFactory(llvm::BumpPtrAllocator &alloc)
      : m_allocator(alloc),
        m_set() {}
   ImmutablePointerSetFactory(const ImmutablePointerSetFactory &) = delete;
   ImmutablePointerSetFactory(ImmutablePointerSetFactory &&) = delete;
   ImmutablePointerSetFactory &
   operator=(const ImmutablePointerSetFactory &) = delete;
   ImmutablePointerSetFactory &operator=(ImmutablePointerSetFactory &&) = delete;

   // We use a sentinel value here so that we can create an empty value
   // statically.
   static PtrSet *getEmptySet()
   {
      return &sm_emptyPtrSet;
   }

   void clear()
   {
      m_set.clear();
   }

   /// Given a sorted and uniqued list \p array, return the ImmutablePointerSet
   /// containing array. Asserts if \p array is not sorted and uniqued.
   PtrSet *get(ArrayRef<PtrTy> array)
   {
      if (array.empty()) {
         return ImmutablePointerSetFactory::getEmptySet();
      }
      // We expect our users to sort/unique the input array. This is because doing
      // it here would either require us to allocate more memory than we need or
      // write into the input array, which we don't want.
      assert(is_sorted_and_uniqued(array));

      llvm::FoldingSetNodeID id;
      for (PtrTy ptr : array) {
         id.AddPointer(ptr);
      }

      void *insertPt;
      if (auto *pset = m_set.FindNodeOrInsertPos(id, insertPt)) {
         return pset;
      }

      size_t numElts = array.size();
      size_t memSize = sizeof(PtrSet) + sizeof(PtrTy) * numElts;

      // Allocate the memory.
      auto *mem =
            reinterpret_cast<PtrSet *>(m_allocator.Allocate(memSize, AllocAlignment));

      // Copy in the pointers into the tail allocated memory. We do not need to do
      // any sorting/uniquing ourselves since we assume that our users perform
      // this task for us.
      MutableArrayRef<PtrTy> dataMem(reinterpret_cast<PtrTy *>(&mem[1]), numElts);
      std::copy(array.begin(), array.end(), dataMem.begin());
      // Allocate the new node and insert it into the m_set.
      auto *newNode = new (mem) PtrSet(this, dataMem);
      m_set.InsertNode(newNode, insertPt);
      return newNode;
   }

   PtrSet *merge(PtrSet *set1, ArrayRef<PtrTy> set2)
   {
      if (set1->empty()) {
         return get(set2);
      }

      if (set2.empty()) {
         return set1;
      }

      // We assume that set2 is sorted and uniqued.
      assert(is_sorted_and_uniqued(set2));

      // If set1 and set2 have the same size, do a quick check to see if they
      // equal. If so, we can bail early and just return set1.
      if (set1->size() == set2.size() &&
          std::equal(set1->begin(), set1->end(), set2.begin())) {
         return set1;
      }
      llvm::FoldingSetNodeID id;

      // We know that both of our pointer sets are sorted, so we can essentially
      // perform a sorted set merging algorithm to create the id. We also count
      // the number of unique elements for allocation purposes.
      unsigned numElts = 0;
      set_union_for_each(*set1, set2, [&id, &numElts](const PtrTy ptr) -> void {
         id.AddPointer(ptr);
         numElts++;
      });

      // If we find our id then continue.
      void *insertPt;
      if (auto *pset = m_set.FindNodeOrInsertPos(id, insertPt)) {
         return pset;
      }

      unsigned memSize = sizeof(PtrSet) + sizeof(PtrTy) * numElts;

      // Allocate the memory.
      auto *mem =
            reinterpret_cast<PtrSet *>(m_allocator.Allocate(memSize, AllocAlignment));

      // Copy in the union of the two pointer sets into the tail allocated
      // memory. Since we know that our sorted arrays are uniqued, we can use
      // set_union to get the uniqued sorted array that we want.
      MutableArrayRef<PtrTy> dataMem(reinterpret_cast<PtrTy *>(&mem[1]), numElts);
      std::set_union(set1->begin(), set1->end(), set2.begin(), set2.end(),
                     dataMem.begin());

      // Allocate the new node, insert it into the m_set, and return it.
      auto *newNode = new (mem) PtrSet(this, dataMem);
      m_set.InsertNode(newNode, insertPt);
      return newNode;
   }

   PtrSet *merge(PtrSet *set1, PtrSet *set2)
   {
      // If either set1 or set2 are the empty PtrSet, just return set2 or set1.
      if (set1->empty()) {
         return set2;
      }

      if (set2->empty()) {
         return set1;
      }
      // We know that all of our PtrSets are uniqued. So if set1 and set2 are the same
      // set, their pointers must also be the same set. In such a case, we return
      // early returning set1 without any loss of generality.
      if (set1 == set2) {
         return set1;
      }
      llvm::FoldingSetNodeID id;
      // We know that both of our pointer sets are sorted, so we can essentially
      // perform a sorted set merging algorithm to create the id. We also count
      // the number of unique elements for allocation purposes.
      unsigned numElts = 0;
      set_union_for_each(*set1, *set2, [&id, &numElts](const PtrTy ptr) -> void {
         id.AddPointer(ptr);
         numElts++;
      });

      // If we find our id then continue.
      void *insertPt;
      if (auto *pset = m_set.FindNodeOrInsertPos(id, insertPt)) {
         return pset;
      }

      unsigned memSize = sizeof(PtrSet) + sizeof(PtrTy) * numElts;

      // Allocate the memory.
      auto *mem =
            reinterpret_cast<PtrSet *>(m_allocator.Allocate(memSize, AllocAlignment));

      // Copy in the union of the two pointer sets into the tail allocated
      // memory. Since we know that our sorted arrays are uniqued, we can use
      // set_union to get the uniqued sorted array that we want.
      MutableArrayRef<PtrTy> dataMem(reinterpret_cast<PtrTy *>(&mem[1]), numElts);
      std::set_union(set1->begin(), set1->end(), set2->begin(), set2->end(),
                     dataMem.begin());

      // Allocate the new node, insert it into the m_set, and return it.
      auto *newNode = new (mem) PtrSet(this, dataMem);
      m_set.InsertNode(newNode, insertPt);
      return newNode;
   }
};

template <typename T>
ImmutablePointerSet<T> ImmutablePointerSetFactory<T>::sm_emptyPtrSet =
      ImmutablePointerSet<T>(nullptr, {});

template <typename T>
#if !defined(_MSC_VER) || defined(__clang__)
constexpr
#endif
const unsigned ImmutablePointerSetFactory<T>::AllocAlignment =
      (alignof(PtrSet) > alignof(PtrTy)) ? alignof(PtrSet) : alignof(PtrTy);

} // polar

#endif // POLARPHP_BASIC_IMMUTABLEPOINTERSET_H

