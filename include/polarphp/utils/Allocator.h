// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/31.

//===----------------------------------------------------------------------===//
/// \file
///
/// This file defines the MallocAllocator and BumpPtrAllocator interfaces. Both
/// of these conform to an LLVM "Allocator" concept which consists of an
/// Allocate method accepting a size and alignment, and a Deallocate accepting
/// a pointer and size. Further, the LLVM "Allocator" concept has overloads of
/// Allocate and Deallocate for setting size and alignment based on the final
/// type. These overloads are typically provided by a base class template \c
/// AllocatorBase.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_ALLOCATOR_H
#define POLARPHP_UTILS_ALLOCATOR_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/MemoryAlloc.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <type_traits>
#include <utility>
#include <optional>

namespace polar {
namespace utils {

using polar::basic::SmallVector;
using polar::basic::SmallVectorImpl;

/// CRTP base class providing obvious overloads for the core \c
/// Allocate() methods of LLVM-style allocators.
///
/// This base class both documents the full public interface exposed by all
/// LLVM-style allocators, and redirects all of the overloads to a single core
/// set of methods which the derived class must define.
template <typename DerivedType>
class AllocatorBase
{
public:
   /// Allocate \a Size bytes of \a Alignment aligned memory. This method
   /// must be implemented by \c DerivedType.
   void *allocate(size_t size, size_t alignment)
   {
#ifdef __clang__
      static_assert(static_cast<void *(AllocatorBase::*)(size_t, size_t)>(
                       &AllocatorBase::allocate) !=
            static_cast<void *(DerivedType::*)(size_t, size_t)>(
               &DerivedType::allocate),
                    "Class derives from AllocatorBase without implementing the "
                    "core Allocate(size_t, size_t) overload!");
#endif
      return static_cast<DerivedType *>(this)->allocate(size, alignment);
   }

   /// Deallocate \a ptr to \a Size bytes of memory allocated by this
   /// allocator.
   void deallocate(const void *ptr, size_t size)
   {
#ifdef __clang__
      static_assert(static_cast<void (AllocatorBase::*)(const void *, size_t)>(
                       &AllocatorBase::deallocate) !=
            static_cast<void (DerivedType::*)(const void *, size_t)>(
               &DerivedType::Deallocate),
                    "Class derives from AllocatorBase without implementing the "
                    "core Deallocate(void *) overload!");
#endif
      return static_cast<DerivedType *>(this)->deallocate(ptr, size);
   }

   // The rest of these methods are helpers that redirect to one of the above
   // core methods.

   /// Allocate space for a sequence of objects without constructing them.
   template <typename T> T *
   allocate(size_t num = 1)
   {
      return static_cast<T *>(allocate(num * sizeof(T), alignof(T)));
   }

   /// Deallocate space for a sequence of objects without constructing them.
   template <typename T>
   typename std::enable_if<
   !std::is_same<typename std::remove_cv<T>::type, void>::value, void>::type
   deallocate(T *ptr, size_t num = 1)
   {
      deallocate(static_cast<const void *>(ptr), num * sizeof(T));
   }
};

class MallocAllocator : public AllocatorBase<MallocAllocator>
{
public:
   void reset() {}

   POLAR_ATTRIBUTE_RETURNS_NONNULL
   void *allocate(size_t size, size_t /*Alignment*/)
   {
      return safe_malloc(size);
   }

   // Pull in base class overloads.
   using AllocatorBase<MallocAllocator>::allocate;

   void deallocate(const void *ptr, size_t /*Size*/)
   {
      free(const_cast<void *>(ptr));
   }

   // Pull in base class overloads.
   using AllocatorBase<MallocAllocator>::deallocate;

   void printStats() const
   {}
};

namespace internal {

// We call out to an external function to actually print the message as the
// printing code uses Allocator.h in its implementation.
void print_bump_ptr_allocator_stats(unsigned numSlabs, size_t bytesAllocated,
                                    size_t totalMemory);

} // end namespace internal

/// Allocate memory in an ever growing pool, as if by bump-pointer.
///
/// This isn't strictly a bump-pointer allocator as it uses backing slabs of
/// memory rather than relying on a boundless contiguous heap. However, it has
/// bump-pointer semantics in that it is a monotonically growing pool of memory
/// where every allocation is found by merely allocating the next N bytes in
/// the slab, or the next N bytes in the next slab.
///
/// Note that this also has a threshold for forcing allocations above a certain
/// size into their own slab.
///
/// The BumpPtrAllocatorImpl template defaults to using a MallocAllocator
/// object, which wraps malloc, to allocate memory, but it can be changed to
/// use a custom allocator.
template <typename AllocatorType = MallocAllocator, size_t SlabSize = 4096,
          size_t SizeThreshold = SlabSize>
class BumpPtrAllocatorImpl
      : public AllocatorBase<
      BumpPtrAllocatorImpl<AllocatorType, SlabSize, SizeThreshold>> {
public:
   static_assert(SizeThreshold <= SlabSize,
                 "The SizeThreshold must be at most the SlabSize to ensure "
                 "that objects larger than a slab go into their own memory "
                 "allocation.");

   BumpPtrAllocatorImpl() = default;

   template <typename T>
   BumpPtrAllocatorImpl(T &&allocator)
      : m_allocator(std::forward<T &&>(allocator))
   {}

   // Manually implement a move constructor as we must clear the old allocator's
   // slabs as a matter of correctness.
   BumpPtrAllocatorImpl(BumpPtrAllocatorImpl &&old)
      : m_curPtr(old.m_curPtr), m_end(old.m_end), m_slabs(std::move(old.m_slabs)),
        m_customSizedSlabs(std::move(old.m_customSizedSlabs)),
        m_bytesAllocated(old.m_bytesAllocated), m_redZoneSize(old.m_redZoneSize),
        m_allocator(std::move(old.m_allocator))
   {
      old.m_curPtr = old.m_end = nullptr;
      old.m_bytesAllocated = 0;
      old.m_slabs.clear();
      old.m_customSizedSlabs.clear();
   }

   ~BumpPtrAllocatorImpl()
   {
      deallocateSlabs(m_slabs.begin(), m_slabs.end());
      deallocateCustomSizedSlabs();
   }

   BumpPtrAllocatorImpl &operator=(BumpPtrAllocatorImpl &&rhs)
   {
      deallocateSlabs(m_slabs.begin(), m_slabs.end());
      deallocateCustomSizedSlabs();

      m_curPtr = rhs.m_curPtr;
      m_end = rhs.m_end;
      m_bytesAllocated = rhs.m_bytesAllocated;
      m_redZoneSize = rhs.m_redZoneSize;
      m_slabs = std::move(rhs.m_slabs);
      m_customSizedSlabs = std::move(rhs.m_customSizedSlabs);
      m_allocator = std::move(rhs.m_allocator);

      rhs.m_curPtr = rhs.m_end = nullptr;
      rhs.m_bytesAllocated = 0;
      rhs.m_slabs.clear();
      rhs.m_customSizedSlabs.clear();
      return *this;
   }

   /// Deallocate all but the current slab and reset the current pointer
   /// to the beginning of it, freeing all memory allocated so far.
   void reset()
   {
      // Deallocate all but the first slab, and deallocate all custom-sized slabs.
      deallocateCustomSizedSlabs();
      m_customSizedSlabs.clear();

      if (m_slabs.empty()) {
         return;
      }
      // Reset the state.
      m_bytesAllocated = 0;
      m_curPtr = (char *)m_slabs.getFront();
      m_end = m_curPtr + SlabSize;

      __asan_poison_memory_region(*m_slabs.begin(), computeSlabSize(0));
      deallocateSlabs(std::next(m_slabs.begin()), m_slabs.end());
      m_slabs.erase(std::next(m_slabs.begin()), m_slabs.end());
   }

   /// Allocate space at the specified alignment.
   POLAR_ATTRIBUTE_RETURNS_NONNULL POLAR_ATTRIBUTE_RETURNS_NOALIAS void *
   allocate(size_t size, size_t alignment)
   {
      assert(alignment > 0 && "0-byte alignnment is not allowed. Use 1 instead.");

      // Keep track of how many bytes we've allocated.
      m_bytesAllocated += size;

      size_t adjustment = alignment_adjustment(m_curPtr, alignment);
      assert(adjustment + size >= size && "Adjustment + Size must not overflow");

      size_t sizeToAllocate = size;
#if POLAR_ADDRESS_SANITIZER_BUILD
      // Add trailing bytes as a "red zone" under ASan.
      sizeToAllocate += m_redZoneSize;
#endif

      // Check if we have enough space.
      if (adjustment + sizeToAllocate <= size_t(m_end - m_curPtr)) {
         char *alignedPtr = m_curPtr + adjustment;
         m_curPtr = alignedPtr + sizeToAllocate;
         // Update the allocation point of this memory block in MemorySanitizer.
         // Without this, MemorySanitizer messages for values originated from here
         // will point to the allocation of the entire slab.
         __msan_allocated_memory(alignedPtr, size);
         // Similarly, tell ASan about this space.
         __asan_unpoison_memory_region(alignedPtr, size);
         return alignedPtr;
      }

      // If Size is really big, allocate a separate slab for it.
      size_t paddedSize = sizeToAllocate + alignment - 1;
      if (paddedSize > SizeThreshold) {
         void *newSlab = m_allocator.allocate(paddedSize, 0);
         // We own the new slab and don't want anyone reading anyting other than
         // pieces returned from this method.  So poison the whole slab.
         __asan_poison_memory_region(newSlab, paddedSize);
         m_customSizedSlabs.push_back(std::make_pair(newSlab, paddedSize));

         uintptr_t alignedAddr = align_addr(newSlab, alignment);
         assert(alignedAddr + size <= (uintptr_t)newSlab + paddedSize);
         char *alignedPtr = (char*)alignedAddr;
         __msan_allocated_memory(alignedPtr, size);
         __asan_unpoison_memory_region(alignedPtr, size);
         return alignedPtr;
      }

      // Otherwise, start a new slab and try again.
      startNewSlab();
      uintptr_t alignedAddr = align_addr(m_curPtr, alignment);
      assert(alignedAddr + sizeToAllocate <= (uintptr_t)m_end &&
             "Unable to allocate memory!");
      char *alignedPtr = (char*)alignedAddr;
      m_curPtr = alignedPtr + sizeToAllocate;
      __msan_allocated_memory(alignedPtr, size);
      __asan_unpoison_memory_region(alignedPtr, size);
      return alignedPtr;
   }

   // Pull in base class overloads.
   using AllocatorBase<BumpPtrAllocatorImpl>::allocate;

   // Bump pointer allocators are expected to never free their storage; and
   // clients expect pointers to remain valid for non-dereferencing uses even
   // after deallocation.
   void deallocate(const void *ptr, size_t size)
   {
      __asan_poison_memory_region(ptr, size);
   }

   // Pull in base class overloads.
   using AllocatorBase<BumpPtrAllocatorImpl>::deallocate;

   size_t getNumSlabs() const
   {
      return m_slabs.getSize() + m_customSizedSlabs.getSize();
   }

   /// \return An index uniquely and reproducibly identifying
   /// an input pointer \p ptr in the given allocator.
   /// The returned value is negative iff the object is inside a custom-size
   /// slab.
   /// Returns an empty optional if the pointer is not found in the allocator.
   std::optional<int64_t> identifyObject(const void *ptr)
   {
      const char *P = static_cast<const char *>(ptr);
      int64_t inSlabIdx = 0;
      for (size_t idx = 0, E = m_slabs.size(); idx < E; idx++) {
         const char *S = static_cast<const char *>(m_slabs[idx]);
         if (P >= S && P < S + computeSlabSize(idx)) {
            return inSlabIdx + static_cast<int64_t>(P - S);
         }
         inSlabIdx += static_cast<int64_t>(computeSlabSize(idx));
      }

      // Use negative index to denote custom sized slabs.
      int64_t inCustomSizedSlabIdx = -1;
      for (size_t idx = 0, E = m_customSizedSlabs.size(); idx < E; idx++) {
         const char *S = static_cast<const char *>(m_customSizedSlabs[idx].first);
         size_t Size = m_customSizedSlabs[idx].second;
         if (P >= S && P < S + Size) {
            return inCustomSizedSlabIdx - static_cast<int64_t>(P - S);
         }
         inCustomSizedSlabIdx -= static_cast<int64_t>(Size);
      }
      return std::nullopt;
   }

   /// A wrapper around identifyObject that additionally asserts that
   /// the object is indeed within the allocator.
   /// \return An index uniquely and reproducibly identifying
   /// an input pointer \p ptr in the given allocator.
   int64_t identifyKnownObject(const void *ptr)
   {
      std::optional<int64_t> out = identifyObject(ptr);
      assert(out && "Wrong allocator used");
      return *out;
   }

   /// A wrapper around identifyKnownObject. Accepts type information
   /// about the object and produces a smaller identifier by relying on
   /// the alignment information. Note that sub-classes may have different
   /// alignment, so the most base class should be passed as template parameter
   /// in order to obtain correct results. For that reason automatic template
   /// parameter deduction is disabled.
   /// \return An index uniquely and reproducibly identifying
   /// an input pointer \p ptr in the given allocator. This identifier is
   /// different from the ones produced by identifyObject and
   /// identifyAlignedObject.
   template <typename T>
   int64_t identifyKnownAlignedObject(const void *ptr)
   {
      int64_t out = identifyKnownObject(ptr);
      assert(out % alignof(T) == 0 && "Wrong alignment information");
      return out / alignof(T);
   }

   size_t getTotalMemory() const
   {
      size_t totalMemory = 0;
      for (auto iter = m_slabs.begin(), end = m_slabs.end(); iter != end; ++iter) {
         totalMemory += computeSlabSize(std::distance(m_slabs.begin(), iter));
      }
      for (auto &ptrAndSize : m_customSizedSlabs) {
         totalMemory += ptrAndSize.second;
      }
      return totalMemory;
   }

   size_t getBytesAllocated() const
   {
      return m_bytesAllocated;
   }

   void setRedZoneSize(size_t newSize)
   {
      m_redZoneSize = newSize;
   }

   void printStats() const
   {
      internal::print_bump_ptr_allocator_stats(m_slabs.getSize(), m_bytesAllocated,
                                               getTotalMemory());
   }

private:
   /// The current pointer into the current slab.
   ///
   /// This points to the next free byte in the slab.
   char *m_curPtr = nullptr;

   /// The end of the current slab.
   char *m_end = nullptr;

   /// The slabs allocated so far.
   SmallVector<void *, 4> m_slabs;

   /// Custom-sized slabs allocated for too-large allocation requests.
   SmallVector<std::pair<void *, size_t>, 0> m_customSizedSlabs;

   /// How many bytes we've allocated.
   ///
   /// Used so that we can compute how much space was wasted.
   size_t m_bytesAllocated = 0;

   /// The number of bytes to put between allocations when running under
   /// a sanitizer.
   size_t m_redZoneSize = 1;

   /// The allocator instance we use to get slabs of memory.
   AllocatorType m_allocator;

   static size_t computeSlabSize(unsigned slabIdx)
   {
      // Scale the actual allocated slab size based on the number of slabs
      // allocated. Every 128 slabs allocated, we double the allocated size to
      // reduce allocation frequency, but saturate at multiplying the slab size by
      // 2^30.
      return SlabSize * ((size_t)1 << std::min<size_t>(30, slabIdx / 128));
   }

   /// Allocate a new slab and move the bump pointers over into the new
   /// slab, modifying CurPtr and End.
   void startNewSlab()
   {
      size_t allocatedSlabSize = computeSlabSize(m_slabs.getSize());

      void *newSlab = m_allocator.allocate(allocatedSlabSize, 0);
      // We own the new slab and don't want anyone reading anything other than
      // pieces returned from this method.  So poison the whole slab.
      __asan_poison_memory_region(newSlab, allocatedSlabSize);

      m_slabs.push_back(newSlab);
      m_curPtr = (char *)(newSlab);
      m_end = ((char *)newSlab) + allocatedSlabSize;
   }

   /// Deallocate a sequence of slabs.
   void deallocateSlabs(SmallVectorImpl<void *>::iterator iter,
                        SmallVectorImpl<void *>::iterator end)
   {
      for (; iter != end; ++iter) {
         size_t allocatedSlabSize =
               computeSlabSize(std::distance(m_slabs.begin(), iter));
         m_allocator.deallocate(*iter, allocatedSlabSize);
      }
   }

   /// Deallocate all memory for custom sized slabs.
   void deallocateCustomSizedSlabs()
   {
      for (auto &ptrAndSize : m_customSizedSlabs) {
         void *ptr = ptrAndSize.first;
         size_t size = ptrAndSize.second;
         m_allocator.deallocate(ptr, size);
      }
   }

   template <typename T> friend class SpecificBumpPtrAllocator;
};

/// The standard BumpPtrAllocator which just uses the default template
/// parameters.
typedef BumpPtrAllocatorImpl<> BumpPtrAllocator;

/// A BumpPtrAllocator that allows only elements of a specific type to be
/// allocated.
///
/// This allows calling the destructor in DestroyAll() and when the allocator is
/// destroyed.
template <typename T> class SpecificBumpPtrAllocator
{
   BumpPtrAllocator m_allocator;

public:
   SpecificBumpPtrAllocator()
   {
      // Because SpecificBumpPtrAllocator walks the memory to call destructors,
      // it can't have red zones between allocations.
      m_allocator.setRedZoneSize(0);
   }

   SpecificBumpPtrAllocator(SpecificBumpPtrAllocator &&old)
      : m_allocator(std::move(old.m_allocator))
   {}

   ~SpecificBumpPtrAllocator()
   {
      destroyAll();
   }

   SpecificBumpPtrAllocator &operator=(SpecificBumpPtrAllocator &&rhs)
   {
      m_allocator = std::move(rhs.m_allocator);
      return *this;
   }

   /// Call the destructor of each allocated object and deallocate all but the
   /// current slab and reset the current pointer to the beginning of it, freeing
   /// all memory allocated so far.
   void destroyAll()
   {
      auto destroyElements = [](char *begin, char *end) {
         assert(begin == (char *)align_addr(begin, alignof(T)));
         for (char *ptr = begin; ptr + sizeof(T) <= end; ptr += sizeof(T)) {
            reinterpret_cast<T *>(ptr)->~T();
         }
      };

      for (auto iter = m_allocator.m_slabs.begin(), endMark = m_allocator.m_slabs.end(); iter != endMark;
           ++iter) {
         size_t allocatedSlabSize = BumpPtrAllocator::computeSlabSize(
                  std::distance(m_allocator.m_slabs.begin(), iter));
         char *begin = (char *)align_addr(*iter, alignof(T));
         char *end = *iter == m_allocator.m_slabs.getBack() ? m_allocator.m_curPtr
                                                            : (char *)*iter + allocatedSlabSize;

         destroyElements(begin, end);
      }

      for (auto &ptrAndSize : m_allocator.m_customSizedSlabs) {
         void *ptr = ptrAndSize.first;
         size_t size = ptrAndSize.second;
         destroyElements((char *)align_addr(ptr, alignof(T)), (char *)ptr + size);
      }

      m_allocator.reset();
   }

   /// Allocate space for an array of objects without constructing them.
   T *allocate(size_t num = 1)
   {
      return m_allocator.allocate<T>(num);
   }
};

} // utils
} // polar

template <typename AllocatorType, size_t SlabSize, size_t SizeThreshold>
void *operator new(size_t size,
                   polar::utils::BumpPtrAllocatorImpl<AllocatorType, SlabSize,
                   SizeThreshold> &allocator)
{
   struct S
   {
      char m_char;
      union {
         double m_double;
         long double m_longDouble;
         long long m_long;
         void *m_ptr;
      } x;
   };
   return allocator.allocate(
            size, std::min((size_t)polar::utils::next_power_of_two(size), offsetof(S, x)));
}

template <typename AllocatorType, size_t SlabSize, size_t SizeThreshold>
void operator delete(
      void *, polar::utils::BumpPtrAllocatorImpl<AllocatorType, SlabSize, SizeThreshold> &)
{}

#endif // POLARPHP_UTILS_ALLOCATOR_H
