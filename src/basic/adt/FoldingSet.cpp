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
// This file implements a hash set that can be used to remove duplication of
// nodes in a graph.
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/MathExtras.h"
#include <cassert>
#include <cstring>

namespace polar {
namespace basic {

using polar::utils::is_power_of_two32;
using polar::utils::power_of_two_floor;
using polar::basic::StringRef;

//===----------------------------------------------------------------------===//
// FoldingSetNodeIdRef Implementation

/// computeHash - Compute a strong hash value for this FoldingSetNodeIdRef,
/// used to lookup the node in the FoldingSetBase.
unsigned FoldingSetNodeIdRef::computeHash() const
{
   return static_cast<unsigned>(hash_combine_range(m_data, m_data + m_size));
}

bool FoldingSetNodeIdRef::operator==(FoldingSetNodeIdRef other) const
{
   if (m_size != other.m_size) {
      return false;
   }
   return memcmp(m_data, other.m_data, m_size * sizeof(*m_data)) == 0;
}

/// Used to compare the "ordering" of two nodes as defined by the
/// profiled bits and their ordering defined by memcmp().
bool FoldingSetNodeIdRef::operator<(FoldingSetNodeIdRef other) const
{
   if (m_size != other.m_size) {
      return m_size < other.m_size;
   }
   return memcmp(m_data, other.m_data, m_size * sizeof(*m_data)) < 0;
}

//===----------------------------------------------------------------------===//
// FoldingSetNodeId Implementation

/// Add* - Add various data types to Bit data.
///
void FoldingSetNodeId::addPointer(const void *ptr)
{
   // Note: this adds pointers to the hash using sizes and endianness that
   // depend on the host. It doesn't matter, however, because hashing on
   // pointer values is inherently unstable. Nothing should depend on the
   // ordering of nodes in the folding set.
   static_assert(sizeof(uintptr_t) <= sizeof(unsigned long long),
                 "unexpected pointer size");
   addInteger(reinterpret_cast<uintptr_t>(ptr));
}

void FoldingSetNodeId::addInteger(signed value)
{
   m_bits.push_back(value);
}

void FoldingSetNodeId::addInteger(unsigned value)
{
   m_bits.push_back(value);
}

void FoldingSetNodeId::addInteger(long value)
{
   addInteger((unsigned long)value);
}
void FoldingSetNodeId::addInteger(unsigned long value)
{
   if (sizeof(long) == sizeof(int)) {
      addInteger(unsigned(value));
   } else if (sizeof(long) == sizeof(long long)) {
      addInteger((unsigned long long)value);
   } else {
      polar_unreachable("unexpected sizeof(long)");
   }
}
void FoldingSetNodeId::addInteger(long long value)
{
   addInteger((unsigned long long)value);
}

void FoldingSetNodeId::addInteger(unsigned long long value)
{
   addInteger(unsigned(value));
   addInteger(unsigned(value >> 32));
}

void FoldingSetNodeId::addString(StringRef string)
{
   unsigned size =  string.getSize();
   m_bits.push_back(size);
   if (!size) {
      return;
   }

   unsigned units = size / 4;
   unsigned pos = 0;
   const unsigned *base = (const unsigned*) string.getData();

   // If the string is aligned do a bulk transfer.
   if (!((intptr_t)base & 3)) {
      m_bits.append(base, base + units);
      pos = (units + 1) * 4;
   } else {
      // Otherwise do it the hard way.
      // To be compatible with above bulk transfer, we need to take endianness
      // into account.
      static_assert(polar::sys::sg_isBigEndianHost || polar::sys::sg_isLittleEndianHost,
                    "Unexpected host endianness");
      if (polar::sys::sg_isBigEndianHost) {
         for (pos += 4; pos <= size; pos += 4) {
            unsigned v = ((unsigned char)string[pos - 4] << 24) |
                  ((unsigned char)string[pos - 3] << 16) |
                  ((unsigned char)string[pos - 2] << 8) |
                  (unsigned char)string[pos - 1];
            m_bits.push_back(v);
         }
      } else {  // Little-endian host
         for (pos += 4; pos <= size; pos += 4) {
            unsigned v = ((unsigned char)string[pos - 1] << 24) |
                  ((unsigned char)string[pos - 2] << 16) |
                  ((unsigned char)string[pos - 3] << 8) |
                  (unsigned char)string[pos - 4];
            m_bits.push_back(v);
         }
      }
   }

   // With the leftover bits.
   unsigned v = 0;
   // pos will have overshot size by 4 - #bytes left over.
   // No need to take endianness into account here - this is always executed.
   switch (pos - size) {
   case 1: v = (v << 8) | (unsigned char)string[size - 3]; POLAR_FALLTHROUGH;
   case 2: v = (v << 8) | (unsigned char)string[size - 2]; POLAR_FALLTHROUGH;
   case 3: v = (v << 8) | (unsigned char)string[size - 1]; break;
   default: return; // Nothing left.
   }
   m_bits.push_back(v);

}

// addNodeId - Adds the Bit data of another ID to *this.
void FoldingSetNodeId::addNodeId(const FoldingSetNodeId &id)
{
   m_bits.append(id.m_bits.begin(), id.m_bits.end());
}

/// computeHash - Compute a strong hash value for this FoldingSetNodeId, used to
/// lookup the node in the FoldingSetBase.
unsigned FoldingSetNodeId::computeHash() const
{
   return FoldingSetNodeIdRef(m_bits.getData(), m_bits.getSize()).computeHash();
}

/// operator== - Used to compare two nodes to each other.
///
bool FoldingSetNodeId::operator==(const FoldingSetNodeId &other) const
{
   return *this == FoldingSetNodeIdRef(other.m_bits.getData(), other.m_bits.getSize());
}

/// operator== - Used to compare two nodes to each other.
///
bool FoldingSetNodeId::operator==(FoldingSetNodeIdRef other) const
{
   return FoldingSetNodeIdRef(m_bits.getData(), m_bits.getSize()) == other;
}

/// Used to compare the "ordering" of two nodes as defined by the
/// profiled bits and their ordering defined by memcmp().
bool FoldingSetNodeId::operator<(const FoldingSetNodeId &other) const
{
   return *this < FoldingSetNodeIdRef(other.m_bits.getData(), other.m_bits.getSize());
}

bool FoldingSetNodeId::operator<(FoldingSetNodeIdRef other) const
{
   return FoldingSetNodeIdRef(m_bits.getData(), m_bits.getSize()) < other;
}

/// Intern - Copy this node's data to a memory region allocated from the
/// given allocator and return a FoldingSetNodeIdRef describing the
/// interned data.
FoldingSetNodeIdRef
FoldingSetNodeId::intern(BumpPtrAllocator &allocator) const
{
   unsigned *newStorage = allocator.allocate<unsigned>(m_bits.getSize());
   std::uninitialized_copy(m_bits.begin(), m_bits.end(), newStorage);
   return FoldingSetNodeIdRef(newStorage, m_bits.getSize());
}

namespace {

//===----------------------------------------------------------------------===//
/// Helper functions for FoldingSetBase.

/// get_next_ptr - In order to save space, each bucket is a
/// singly-linked-list. In order to make deletion more efficient, we make
/// the list circular, so we can delete a node without computing its hash.
/// The problem with this is that the start of the hash buckets are not
/// Nodes.  If nextInBucketPtr is a bucket pointer, this method returns null:
/// use get_bucket_ptr when this happens.
FoldingSetBase::Node *get_next_ptr(void *nextInBucketPtr)
{
   // The low bit is set if this is the pointer back to the bucket.
   if (reinterpret_cast<intptr_t>(nextInBucketPtr) & 1) {
      return nullptr;
   }
   return static_cast<FoldingSetBase::Node*>(nextInBucketPtr);
}


/// testing.
void **get_bucket_ptr(void *nextInBucketPtr)
{
   intptr_t ptr = reinterpret_cast<intptr_t>(nextInBucketPtr);
   assert((ptr & 1) && "Not a bucket pointer");
   return reinterpret_cast<void**>(ptr & ~intptr_t(1));
}

/// get_bucket_for - Hash the specified node ID and return the hash bucket for
/// the specified ID.
void **get_bucket_for(unsigned hashValue, void **buckets, unsigned numBuckets)
{
   // NumBuckets is always a power of 2.
   unsigned bucketNum = hashValue & (numBuckets-1);
   return buckets + bucketNum;
}

/// allocate_buckets - Allocated initialized bucket memory.
void **allocate_buckets(unsigned numBuckets)
{
   void **buckets = static_cast<void**>(polar::utils::safe_calloc(numBuckets + 1,
                                                    sizeof(void*)));
   // Set the very last bucket to be a non-null "pointer".
   buckets[numBuckets] = reinterpret_cast<void*>(-1);
   return buckets;
}

} // anonymous namespace

//===----------------------------------------------------------------------===//
// FoldingSetBase Implementation

void FoldingSetBase::anchor()
{}

FoldingSetBase::FoldingSetBase(unsigned log2InitSize)
{
   assert(5 < log2InitSize && log2InitSize < 32 &&
          "Initial hash table size out of range");
   m_numBuckets = 1 << log2InitSize;
   m_buckets = allocate_buckets(m_numBuckets);
   m_numNodes = 0;
}

FoldingSetBase::FoldingSetBase(FoldingSetBase &&other)
   : m_buckets(other.m_buckets),
     m_numBuckets(other.m_numBuckets),
     m_numNodes(other.m_numNodes)
{
   other.m_buckets = nullptr;
   other.m_numBuckets = 0;
   other.m_numNodes = 0;
}

FoldingSetBase &FoldingSetBase::operator=(FoldingSetBase &&other)
{
   free(m_buckets); // This may be null if the set is in a moved-from state.
   m_buckets = other.m_buckets;
   m_numBuckets = other.m_numBuckets;
   m_numNodes = other.m_numNodes;
   other.m_buckets = nullptr;
   other.m_numBuckets = 0;
   other.m_numNodes = 0;
   return *this;
}

FoldingSetBase::~FoldingSetBase()
{
   free(m_buckets);
}

void FoldingSetBase::clear()
{
   // Set all but the last bucket to null pointers.
   memset(m_buckets, 0, m_numBuckets * sizeof(void*));
   // Set the very last bucket to be a non-null "pointer".
   m_buckets[m_numBuckets] = reinterpret_cast<void*>(-1);
   // Reset the node count to zero.
   m_numNodes = 0;
}

void FoldingSetBase::growBucketCount(unsigned newBucketCount)
{
   assert((newBucketCount > m_numBuckets) && "Can't shrink a folding set with growBucketCount");
   assert(is_power_of_two32(newBucketCount) && "Bad bucket count!");
   void **oldBuckets = m_buckets;
   unsigned oldNumBuckets = m_numBuckets;

   // Clear out new buckets.
   m_buckets = allocate_buckets(newBucketCount);
   // Set NumBuckets only if allocation of new buckets was successful.
   m_numBuckets = newBucketCount;
   m_numNodes = 0;

   // Walk the old buckets, rehashing nodes into their new place.
   FoldingSetNodeId tempId;
   for (unsigned i = 0; i != oldNumBuckets; ++i) {
      void *probe = oldBuckets[i];
      if (!probe) continue;
      while (Node *nodeInBucket = get_next_ptr(probe)) {
         // Figure out the next link, remove NodeInBucket from the old link.
         probe = nodeInBucket->getNextInBucket();
         nodeInBucket->setNextInBucket(nullptr);

         // Insert the node into the new bucket, after recomputing the hash.
         insertNode(nodeInBucket,
                    get_bucket_for(computeNodeHash(nodeInBucket, tempId),
                                   m_buckets, m_numBuckets));
         tempId.clear();
      }
   }

   free(oldBuckets);
}

/// GrowHashTable - Double the size of the hash table and rehash everything.
///
void FoldingSetBase::growHashTable()
{
   growBucketCount(m_numBuckets * 2);
}

void FoldingSetBase::reserve(unsigned eltCount)
{
   // This will give us somewhere between eltCount / 2 and
   // eltCount buckets.  This puts us in the load factor
   // range of 1.0 - 2.0.
   if(eltCount < getCapacity()) {
      return;
   }
   growBucketCount(power_of_two_floor(eltCount));
}

/// findNodeOrInsertPos - Look up the node specified by ID.  If it exists,
/// return it.  If not, return the insertion token that will make insertion
/// faster.
FoldingSetBase::Node *
FoldingSetBase::findNodeOrInsertPos(const FoldingSetNodeId &id,
                                    void *&insertpos)
{
   unsigned idHash = id.computeHash();
   void **bucket = get_bucket_for(idHash, m_buckets, m_numBuckets);
   void *probe = *bucket;
   insertpos = nullptr;
   FoldingSetNodeId tempId;
   while (Node *nodeInBucket = get_next_ptr(probe)) {
      if (nodeEquals(nodeInBucket, id, idHash, tempId)) {
         return nodeInBucket;
      }
      tempId.clear();
      probe = nodeInBucket->getNextInBucket();
   }
   // Didn't find the node, return null with the bucket as the Insertpos.
   insertpos = bucket;
   return nullptr;
}

/// insertNode - Insert the specified node into the folding set, knowing that it
/// is not already in the map.  Insertpos must be obtained from
/// findNodeOrInsertPos.
void FoldingSetBase::insertNode(Node *node, void *insertpos)
{
   assert(!node->getNextInBucket());
   // Do we need to grow the hashtable?
   if (m_numNodes + 1 > getCapacity()) {
      growHashTable();
      FoldingSetNodeId tempId;
      insertpos = get_bucket_for(computeNodeHash(node, tempId), m_buckets, m_numBuckets);
   }
   ++m_numNodes;
   /// The insert position is actually a bucket pointer.
   void **bucket = static_cast<void**>(insertpos);
   void *next = *bucket;
   // If this is the first insertion into this bucket, its next pointer will be
   // null.  Pretend as if it pointed to itself, setting the low bit to indicate
   // that it is a pointer to the bucket.
   if (!next) {
      next = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(bucket)|1);
   }
   // Set the node's next pointer, and make the bucket point to the node.
   node->setNextInBucket(next);
   *bucket = node;
}

/// removeNode - Remove a node from the folding set, returning true if one was
/// removed or false if the node was not in the folding set.
bool FoldingSetBase::removeNode(Node *node) {
   // Because each bucket is a circular list, we don't need to compute N's hash
   // to remove it.
   void *ptr = node->getNextInBucket();
   if (!ptr) {
      return false;  // Not in folding set.
   }
   --m_numNodes;
   node->setNextInBucket(nullptr);
   // Remember what N originally pointed to, either a bucket or another node.
   void *nodeNextPtr = ptr;
   // Chase around the list until we find the node (or bucket) which points to N.
   while (true) {
      if (Node *nodeInBucket = get_next_ptr(ptr)) {
         // Advance pointer.
         ptr = nodeInBucket->getNextInBucket();

         // We found a node that points to N, change it to point to N's next node,
         // removing N from the list.
         if (ptr == node) {
            nodeInBucket->setNextInBucket(nodeNextPtr);
            return true;
         }
      } else {
         void **bucket = get_bucket_ptr(ptr);
         ptr = *bucket;
         // If we found that the bucket points to N, update the bucket to point to
         // whatever is next.
         if (ptr == node) {
            *bucket = nodeNextPtr;
            return true;
         }
      }
   }
}

/// getOrInsertNode - If there is an existing simple Node exactly
/// equal to the specified node, return it.  Otherwise, insert 'N' and it
/// instead.
FoldingSetBase::Node *FoldingSetBase::getOrInsertNode(FoldingSetBase::Node *node)
{
   FoldingSetNodeId id;
   getNodeprofile(node, id);
   void *ip;
   if (Node *end = findNodeOrInsertPos(id, ip)) {
      return end;
   }
   insertNode(node, ip);
   return node;
}

//===----------------------------------------------------------------------===//
// FoldingSetIteratorImpl Implementation

FoldingSetIteratorImpl::FoldingSetIteratorImpl(void **bucket)
{
   // Skip to the first non-null non-self-cycle bucket.
   while (*bucket != reinterpret_cast<void*>(-1) &&
          (!*bucket || !get_next_ptr(*bucket))) {
      ++bucket;
   }
   m_nodePtr = static_cast<FoldingSetNode*>(*++bucket);
}

void FoldingSetIteratorImpl::advance()
{
   // If there is another link within this bucket, go to it.
   void *probe = m_nodePtr->getNextInBucket();
   if (FoldingSetNode *nextNodeInBucket = get_next_ptr(probe)) {
      m_nodePtr = nextNodeInBucket;
   } else {
      // Otherwise, this is the last link in this bucket.
      void **bucket = get_bucket_ptr(probe);
      // Skip to the next non-null non-self-cycle bucket.
      do {
         ++bucket;
      } while (*bucket != reinterpret_cast<void*>(-1) &&
               (!*bucket || !get_next_ptr(*bucket)));
      m_nodePtr = static_cast<FoldingSetNode*>(*bucket);
   }
}

//===----------------------------------------------------------------------===//
// FoldingSetBucketIteratorImpl Implementation

FoldingSetBucketIteratorImpl::FoldingSetBucketIteratorImpl(void **bucket)
{
   m_ptr = (!*bucket || !get_next_ptr(*bucket)) ? (void*) bucket : *bucket;
}

} // basic
} // polar
