// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/14.

//===----------------------------------------------------------------------===//
//
// This file defines a hash set that can be used to remove duplication of nodes
// in a graph.  This code was originally created by Chris Lattner for use with
// SelectionDAGCSEMap, but was isolated to provide use across the llvm code set.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ADT_FOLDING_SET_H
#define POLARPHP_BASIC_ADT_FOLDING_SET_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/utils/Allocator.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace polar {
namespace basic {

using polar::utils::BumpPtrAllocator;

/// This folding set used for two purposes:
///   1. Given information about a node we want to create, look up the unique
///      instance of the node in the set.  If the node already exists, return
///      it, otherwise return the bucket it should be inserted into.
///   2. Given a node that has already been created, remove it from the set.
///
/// This class is implemented as a single-link chained hash table, where the
/// "buckets" are actually the nodes themselves (the next pointer is in the
/// node).  The last node points back to the bucket to simplify node removal.
///
/// Any node that is to be included in the folding set must be a subclass of
/// FoldingSetNode.  The node class must also define a profile method used to
/// establish the unique m_bits of data for the node.  The profile method is
/// passed a FoldingSetNodeId object which is used to gather the m_bits.  Just
/// call one of the Add* functions defined in the FoldingSetBase::NodeID class.
/// NOTE: That the folding set does not own the nodes and it is the
/// responsibility of the user to dispose of the nodes.
///
/// Eg.
///    class MyNode : public FoldingSetNode {
///    private:
///      std::string Name;
///      unsigned Value;
///    public:
///      MyNode(const char *N, unsigned V) : Name(N), Value(V) {}
///       ...
///      void profile(FoldingSetNodeId &ID) const {
///        ID.AddString(Name);
///        ID.addPointer(Value);
///      }
///      ...
///    };
///
/// To define the folding set itself use the FoldingSet template;
///
/// Eg.
///    FoldingSet<MyNode> MyFoldingSet;
///
/// Four public methods are available to manipulate the folding set;
///
/// 1) If you have an existing node that you want add to the set but unsure
/// that the node might already exist then call;
///
///    MyNode *M = MyFoldingSet.getOrInsertNode(N);
///
/// If The result is equal to the input then the node has been inserted.
/// Otherwise, the result is the node existing in the folding set, and the
/// input can be discarded (use the result instead.)
///
/// 2) If you are ready to construct a node but want to check if it already
/// exists, then call findNodeOrInsertPos with a FoldingSetNodeId of the m_bits to
/// check;
///
///   FoldingSetNodeId ID;
///   ID.AddString(Name);
///   ID.addPointer(Value);
///   void *InsertPoint;
///
///    MyNode *M = MyFoldingSet.findNodeOrInsertPos(ID, InsertPoint);
///
/// If found then M with be non-NULL, else InsertPoint will point to where it
/// should be inserted using insertNode.
///
/// 3) If you get a NULL result from findNodeOrInsertPos then you can as a new
/// node with findNodeOrInsertPos;
///
///    insertNode(N, InsertPoint);
///
/// 4) Finally, if you want to remove a node from the folding set call;
///
///    bool WasRemoved = removeNode(N);
///
/// The result indicates whether the node existed in the folding set.

class FoldingSetNodeId;
class StringRef;

//===----------------------------------------------------------------------===//
/// FoldingSetBase - Implements the folding set functionality.  The main
/// structure is an array of buckets.  Each bucket is indexed by the hash of
/// the nodes it contains.  The bucket itself points to the nodes contained
/// in the bucket via a singly linked list.  The last node in the list points
/// back to the bucket to facilitate node removal.
///
class FoldingSetBase
{
   virtual void anchor(); // Out of line virtual method.

protected:
   /// Buckets - Array of bucket chains.
   void **m_buckets;

   /// NumBuckets - Length of the Buckets array.  Always a power of 2.
   unsigned m_numBuckets;

   /// NumNodes - Number of nodes in the folding set. Growth occurs when NumNodes
   /// is greater than twice the number of buckets.
   unsigned m_numNodes;

   explicit FoldingSetBase(unsigned log2InitSize = 6);
   FoldingSetBase(FoldingSetBase &&arg);
   FoldingSetBase &operator=(FoldingSetBase &&other);
   ~FoldingSetBase();

public:
   //===--------------------------------------------------------------------===//
   /// Node - This class is used to maintain the singly linked bucket list in
   /// a folding set.
   class Node
   {
   private:
      // m_nextInFoldingSetBucket - next link in the bucket list.
      void *m_nextInFoldingSetBucket = nullptr;

   public:
      Node() = default;

      // Accessors
      void *getNextInBucket() const
      {
         return m_nextInFoldingSetBucket;
      }
      void setNextInBucket(void *size)
      {
         m_nextInFoldingSetBucket = size;
      }
   };

   /// clear - Remove all nodes from the folding set.
   void clear();

   /// size - Returns the number of nodes in the folding set.
   unsigned size() const
   {
      return m_numNodes;
   }

   unsigned getSize() const
   {
      return size();
   }

   /// empty - Returns true if there are no nodes in the folding set.
   bool empty() const
   {
      return m_numNodes == 0;
   }

   /// reserve - Increase the number of buckets such that adding the
   /// eltCount-th node won't cause a rebucket operation. reserve is permitted
   /// to allocate more space than requested by eltCount.
   void reserve(unsigned eltCount);

   /// capacity - Returns the number of nodes permitted in the folding set
   /// before a rebucket operation is performed.
   unsigned getCapacity()
   {
      // We allow a load factor of up to 2.0,
      // so that means our capacity is NumBuckets * 2
      return m_numBuckets * 2;
   }

private:
   /// growHashTable - Double the size of the hash table and rehash everything.
   void growHashTable();

   /// growBucketCount - resize the hash table and rehash everything.
   /// NewBucketCount must be a power of two, and must be greater than the old
   /// bucket count.
   void growBucketCount(unsigned newBucketCount);

protected:
   /// getNodeprofile - Instantiations of the FoldingSet template implement
   /// this function to gather data m_bits for the given node.
   virtual void getNodeprofile(Node *node, FoldingSetNodeId &id) const = 0;

   /// nodeEquals - Instantiations of the FoldingSet template implement
   /// this function to compare the given node with the given ID.
   virtual bool nodeEquals(Node *node, const FoldingSetNodeId &id, unsigned idHash,
                           FoldingSetNodeId &tempId) const=0;

   /// computeNodeHash - Instantiations of the FoldingSet template implement
   /// this function to compute a hash value for the given node.
   virtual unsigned computeNodeHash(Node *node, FoldingSetNodeId &tempId) const = 0;

   // The below methods are protected to encourage subclasses to provide a more
   // type-safe API.

   /// removeNode - Remove a node from the folding set, returning true if one
   /// was removed or false if the node was not in the folding set.
   bool removeNode(Node *node);

   /// getOrInsertNode - If there is an existing simple Node exactly
   /// equal to the specified node, return it.  Otherwise, insert 'N' and return
   /// it instead.
   Node *getOrInsertNode(Node *node);

   /// findNodeOrInsertPos - Look up the node specified by ID.  If it exists,
   /// return it.  If not, return the insertion token that will make insertion
   /// faster.
   Node *findNodeOrInsertPos(const FoldingSetNodeId &id, void *&insertPos);

   /// insertNode - Insert the specified node into the folding set, knowing that
   /// it is not already in the folding set.  InsertPos must be obtained from
   /// findNodeOrInsertPos.
   void insertNode(Node *node, void *insertPos);
};

//===----------------------------------------------------------------------===//

/// DefaultFoldingSetTrait - This class provides default implementations
/// for FoldingSetTrait implementations.
template<typename T>
struct DefaultFoldingSetTrait
{
   static void profile(const T &x, FoldingSetNodeId &id)
   {
      x.profile(id);
   }

   static void profile(T &x, FoldingSetNodeId &id)
   {
      x.profile(id);
   }

   // equals - Test if the profile for X would match ID, using tempId
   // to compute a temporary ID if necessary. The default implementation
   // just calls profile and does a regular comparison. Implementations
   // can override this to provide more efficient implementations.
   static inline bool equals(T &x, const FoldingSetNodeId &id, unsigned idHash,
                             FoldingSetNodeId &tempId);

   // computeHash - Compute a hash value for X, using tempId to
   // compute a temporary ID if necessary. The default implementation
   // just calls profile and does a regular hash computation.
   // Implementations can override this to provide more efficient
   // implementations.
   static inline unsigned computeHash(T &x, FoldingSetNodeId &tempId);
};

/// FoldingSetTrait - This trait class is used to define behavior of how
/// to "profile" (in the FoldingSet parlance) an object of a given type.
/// The default behavior is to invoke a 'profile' method on an object, but
/// through template specialization the behavior can be tailored for specific
/// types.  Combined with the FoldingSetNodeWrapper class, one can add objects
/// to FoldingSets that were not originally designed to have that behavior.
template<typename T>
struct FoldingSetTrait : public DefaultFoldingSetTrait<T>
{};

/// DefaultContextualFoldingSetTrait - Like DefaultFoldingSetTrait, but
/// for ContextualFoldingSets.
template<typename T, typename Ctx>
struct DefaultContextualFoldingSetTrait {
   static void profile(T &x, FoldingSetNodeId &id, Ctx context)
   {
      x.profile(id, context);
   }

   static inline bool equals(T &x, const FoldingSetNodeId &id, unsigned idHash,
                             FoldingSetNodeId &tempId, Ctx context);
   static inline unsigned computeHash(T &x, FoldingSetNodeId &tempId,
                                      Ctx context);
};

/// ContextualFoldingSetTrait - Like FoldingSetTrait, but for
/// ContextualFoldingSets.
template<typename T, typename Ctx>
struct ContextualFoldingSetTrait
      : public DefaultContextualFoldingSetTrait<T, Ctx>
{};

//===--------------------------------------------------------------------===//
/// FoldingSetNodeIdRef - This class describes a reference to an interned
/// FoldingSetNodeId, which can be a useful to store node id data rather
/// than using plain FoldingSetNodeIds, since the 32-element SmallVector
/// is often much larger than necessary, and the possibility of heap
/// allocation means it requires a non-trivial destructor call.
class FoldingSetNodeIdRef
{
   const unsigned *m_data = nullptr;
   size_t m_size = 0;

public:
   FoldingSetNodeIdRef() = default;
   FoldingSetNodeIdRef(const unsigned *data, size_t size)
      : m_data(data), m_size(size)
   {}

   /// computeHash - Compute a strong hash value for this FoldingSetNodeIdRef,
   /// used to lookup the node in the FoldingSetBase.
   unsigned computeHash() const;

   bool operator==(FoldingSetNodeIdRef) const;

   bool operator!=(FoldingSetNodeIdRef other) const
   {
      return !(*this == other);
   }

   /// Used to compare the "ordering" of two nodes as defined by the
   /// profiled m_bits and their ordering defined by memcmp().
   bool operator<(FoldingSetNodeIdRef) const;

   const unsigned *getData() const
   {
      return m_data;
   }
   size_t getSize() const
   {
      return m_size;
   }
};

//===--------------------------------------------------------------------===//
/// FoldingSetNodeId - This class is used to gather all the unique data m_bits of
/// a node.  When all the m_bits are gathered this class is used to produce a
/// hash value for the node.
class FoldingSetNodeId
{
   /// m_bits - Vector of all the data m_bits that make the node unique.
   /// Use a SmallVector to avoid a heap allocation in the common case.
   SmallVector<unsigned, 32> m_bits;

public:
   FoldingSetNodeId() = default;

   FoldingSetNodeId(FoldingSetNodeIdRef ref) : m_bits(ref.getData(), ref.getData() + ref.getSize())
   {}

   /// Add* - Add various data types to Bit data.
   void addPointer(const void *ptr);
   void addInteger(signed value);
   void addInteger(unsigned value);
   void addInteger(long value);
   void addInteger(unsigned long value);
   void addInteger(long long value);
   void addInteger(unsigned long long value);
   void addBoolean(bool value)
   {
      addInteger(value ? 1U : 0U);
   }
   void addString(StringRef string);
   void addNodeId(const FoldingSetNodeId &id);

   template <typename T>
   inline void add(const T &x)
   {
      FoldingSetTrait<T>::profile(x, *this);
   }

   /// clear - Clear the accumulated profile, allowing this FoldingSetNodeId
   /// object to be used to compute a new profile.
   inline void clear()
   {
      m_bits.clear();
   }

   /// computeHash - Compute a strong hash value for this FoldingSetNodeId, used
   /// to lookup the node in the FoldingSetBase.
   unsigned computeHash() const;

   /// operator== - Used to compare two nodes to each other.
   bool operator==(const FoldingSetNodeId &other) const;
   bool operator==(const FoldingSetNodeIdRef other) const;

   bool operator!=(const FoldingSetNodeId &other) const
   {
      return !(*this == other);
   }

   bool operator!=(const FoldingSetNodeIdRef other) const
   {
      return !(*this ==other);
   }

   /// Used to compare the "ordering" of two nodes as defined by the
   /// profiled m_bits and their ordering defined by memcmp().
   bool operator<(const FoldingSetNodeId &other) const;
   bool operator<(const FoldingSetNodeIdRef other) const;

   /// intern - Copy this node's data to a memory region allocated from the
   /// given allocator and return a FoldingSetNodeIdRef describing the
   /// interned data.
   FoldingSetNodeIdRef intern(BumpPtrAllocator &allocator) const;
};

// Convenience type to hide the implementation of the folding set.
using FoldingSetNode = FoldingSetBase::Node;
template<typename T> class FoldingSetIterator;
template<typename T> class FoldingSetBucketIterator;

// Definitions of FoldingSetTrait and ContextualFoldingSetTrait functions, which
// require the definition of FoldingSetNodeId.
template<typename T>
inline bool DefaultFoldingSetTrait<T>::equals(T &x, const FoldingSetNodeId &id,
                                              unsigned /*IDHash*/,
                                              FoldingSetNodeId &tempId)
{
   FoldingSetTrait<T>::profile(x, tempId);
   return tempId == id;
}

template<typename T>
inline unsigned DefaultFoldingSetTrait<T>::computeHash(T &x, FoldingSetNodeId &tempId)
{
   FoldingSetTrait<T>::profile(x, tempId);
   return tempId.computeHash();
}

template<typename T, typename Ctx>
inline bool DefaultContextualFoldingSetTrait<T, Ctx>::equals(T &X,
                                                             const FoldingSetNodeId &ID,
                                                             unsigned /*IDHash*/,
                                                             FoldingSetNodeId &tempId,
                                                             Ctx context)
{
   ContextualFoldingSetTrait<T, Ctx>::profile(X, tempId, context);
   return tempId == ID;
}

template<typename T, typename Ctx>
inline unsigned DefaultContextualFoldingSetTrait<T, Ctx>::computeHash(T &x,
                                                                      FoldingSetNodeId &tempId,
                                                                      Ctx context)
{
   ContextualFoldingSetTrait<T, Ctx>::profile(x, tempId, context);
   return tempId.computeHash();
}

//===----------------------------------------------------------------------===//
/// FoldingSetImpl - An implementation detail that lets us share code between
/// FoldingSet and ContextualFoldingSet.
template <typename T>
class FoldingSetImpl : public FoldingSetBase
{
protected:
   explicit FoldingSetImpl(unsigned log2InitSize)
      : FoldingSetBase(log2InitSize) {}

   FoldingSetImpl(FoldingSetImpl &&arg) = default;
   FoldingSetImpl &operator=(FoldingSetImpl &&other) = default;
   ~FoldingSetImpl() = default;

public:
   using iterator = FoldingSetIterator<T>;

   iterator begin()
   {
      return iterator(m_buckets);
   }

   iterator end()
   {
      return iterator(m_buckets+m_numBuckets);
   }

   using const_iterator = FoldingSetIterator<const T>;

   const_iterator begin() const
   {
      return const_iterator(m_buckets);
   }

   const_iterator end() const
   {
      return const_iterator(m_buckets+m_numBuckets);
   }

   using bucket_iterator = FoldingSetBucketIterator<T>;

   bucket_iterator bucketBegin(unsigned hash)
   {
      return bucket_iterator(m_buckets + (hash & (m_numBuckets-1)));
   }

   bucket_iterator bucket_end(unsigned hash)
   {
      return bucket_iterator(m_buckets + (hash & (m_numBuckets-1)), true);
   }

   /// removeNode - Remove a node from the folding set, returning true if one
   /// was removed or false if the node was not in the folding set.
   bool removeNode(T *node)
   {
      return FoldingSetBase::removeNode(node);
   }

   /// getOrInsertNode - If there is an existing simple Node exactly
   /// equal to the specified node, return it.  Otherwise, insert 'N' and
   /// return it instead.
   T *getOrInsertNode(T *node)
   {
      return static_cast<T *>(FoldingSetBase::getOrInsertNode(node));
   }

   /// findNodeOrInsertPos - Look up the node specified by ID.  If it exists,
   /// return it.  If not, return the insertion token that will make insertion
   /// faster.
   T *findNodeOrInsertPos(const FoldingSetNodeId &id, void *&insertPos)
   {
      return static_cast<T *>(FoldingSetBase::findNodeOrInsertPos(id, insertPos));
   }

   /// insertNode - Insert the specified node into the folding set, knowing that
   /// it is not already in the folding set.  InsertPos must be obtained from
   /// findNodeOrInsertPos.
   void insertNode(T *node, void *insertPos)
   {
      FoldingSetBase::insertNode(node, insertPos);
   }

   /// insertNode - Insert the specified node into the folding set, knowing that
   /// it is not already in the folding set.
   void insertNode(T *node)
   {
      T *inserted = getOrInsertNode(node);
      (void)inserted;
      assert(inserted == node && "Node already inserted!");
   }
};

//===----------------------------------------------------------------------===//
/// FoldingSet - This template class is used to instantiate a specialized
/// implementation of the folding set to the node class T.  T must be a
/// subclass of FoldingSetNode and implement a profile function.
///
/// Note that this set type is movable and move-assignable. However, its
/// moved-from state is not a valid state for anything other than
/// move-assigning and destroying. This is primarily to enable movable APIs
/// that incorporate these objects.
template <typename T>
class FoldingSet final : public FoldingSetImpl<T>
{
   using Super = FoldingSetImpl<T>;
   using Node = typename Super::Node;

   /// getNodeprofile - Each instantiatation of the FoldingSet needs to provide a
   /// way to convert nodes into a unique specifier.
   void getNodeprofile(Node *node, FoldingSetNodeId &id) const override
   {
      T *tempNode = static_cast<T *>(node);
      FoldingSetTrait<T>::profile(*tempNode, id);
   }

   /// nodeEquals - Instantiations may optionally provide a way to compare a
   /// node with a specified ID.
   bool nodeEquals(Node *node, const FoldingSetNodeId &id, unsigned idHash,
                   FoldingSetNodeId &tempId) const override
   {
      T *tempNode = static_cast<T *>(node);
      return FoldingSetTrait<T>::equals(*tempNode, id, idHash, tempId);
   }

   /// computeNodeHash - Instantiations may optionally provide a way to compute a
   /// hash value directly from a node.
   unsigned computeNodeHash(Node *node, FoldingSetNodeId &tempId) const override
   {
      T *tempNode = static_cast<T *>(node);
      return FoldingSetTrait<T>::computeHash(*tempNode, tempId);
   }

public:
   explicit FoldingSet(unsigned log2InitSize = 6) : Super(log2InitSize)
   {}
   FoldingSet(FoldingSet &&arg) = default;
   FoldingSet &operator=(FoldingSet &&other) = default;
};

//===----------------------------------------------------------------------===//
/// ContextualFoldingSet - This template class is a further refinement
/// of FoldingSet which provides a context argument when calling
/// profile on its nodes.  Currently, that argument is fixed at
/// initialization time.
///
/// T must be a subclass of FoldingSetNode and implement a profile
/// function with signature
///   void profile(FoldingSetNodeId &, Ctx);
template <typename T, class Ctx>
class ContextualFoldingSet final : public FoldingSetImpl<T>
{
   // Unfortunately, this can't derive from FoldingSet<T> because the
   // construction of the vtable for FoldingSet<T> requires
   // FoldingSet<T>::getNodeprofile to be instantiated, which in turn
   // requires a single-argument T::profile().

   using Super = FoldingSetImpl<T>;
   using Node = typename Super::Node;

   Ctx m_context;

   /// getNodeprofile - Each instantiatation of the FoldingSet needs to provide a
   /// way to convert nodes into a unique specifier.
   void getNodeprofile(Node *node, FoldingSetNodeId &id) const override
   {
      T *tempNode = static_cast<T *>(node);
      ContextualFoldingSetTrait<T, Ctx>::profile(*tempNode, id, m_context);
   }

   bool nodeEquals(Node *node, const FoldingSetNodeId &id, unsigned idHash,
                   FoldingSetNodeId &tempId) const override
   {
      T *tempNode = static_cast<T *>(node);
      return ContextualFoldingSetTrait<T, Ctx>::equals(*tempNode, id, idHash, tempId,
                                                       m_context);
   }

   unsigned computeNodeHash(Node *node, FoldingSetNodeId &tempId) const override
   {
      T *tempNode = static_cast<T *>(node);
      return ContextualFoldingSetTrait<T, Ctx>::computeHash(*tempNode, tempId, m_context);
   }

public:
   explicit ContextualFoldingSet(Ctx context, unsigned log2InitSize = 6)
      : Super(log2InitSize), m_context(context)
   {}

   Ctx getContext() const
   {
      return m_context;
   }
};

//===----------------------------------------------------------------------===//
/// FoldingSetVector - This template class combines a FoldingSet and a vector
/// to provide the interface of FoldingSet but with deterministic iteration
/// order based on the insertion order. T must be a subclass of FoldingSetNode
/// and implement a profile function.
template <typename T, class VectorType = SmallVector<T*, 8>>
class FoldingSetVector
{
   FoldingSet<T> m_set;
   VectorType m_vector;

public:
   explicit FoldingSetVector(unsigned log2InitSize = 6) : m_set(log2InitSize)
   {}

   using iterator = PointeeIterator<typename VectorType::iterator>;

   iterator begin()
   {
      return m_vector.begin();
   }

   iterator end()
   {
      return m_vector.end();
   }

   using const_iterator = PointeeIterator<typename VectorType::const_iterator>;

   const_iterator begin() const { return m_vector.begin(); }
   const_iterator end()   const { return m_vector.end(); }

   /// clear - Remove all nodes from the folding set.
   void clear()
   {
      m_set.clear();
      m_vector.clear();
   }

   /// findNodeOrInsertPos - Look up the node specified by ID.  If it exists,
   /// return it.  If not, return the insertion token that will make insertion
   /// faster.
   T *findNodeOrInsertPos(const FoldingSetNodeId &id, void *&insertPos)
   {
      return m_set.findNodeOrInsertPos(id, insertPos);
   }

   /// getOrInsertNode - If there is an existing simple Node exactly
   /// equal to the specified node, return it.  Otherwise, insert 'N' and
   /// return it instead.
   T *getOrInsertNode(T *node)
   {
      T *result = m_set.getOrInsertNode(node);
      if (result == node) {
         m_vector.push_back(node);
      }
      return result;
   }

   /// insertNode - Insert the specified node into the folding set, knowing that
   /// it is not already in the folding set.  InsertPos must be obtained from
   /// findNodeOrInsertPos.
   void insertNode(T *node, void *insertPos)
   {
      m_set.insertNode(node, insertPos);
      m_vector.push_back(node);
   }

   /// insertNode - Insert the specified node into the folding set, knowing that
   /// it is not already in the folding set.
   void insertNode(T *node)
   {
      m_set.insertNode(node);
      m_vector.pushBack(node);
   }

   /// size - Returns the number of nodes in the folding set.
   unsigned size() const
   {
      return m_set.size();
   }

   unsigned getSize() const
   {
      return size();
   }

   /// empty - Returns true if there are no nodes in the folding set.
   bool empty() const
   {
      return m_set.empty();
   }
};

//===----------------------------------------------------------------------===//
/// FoldingSetIteratorImpl - This is the common iterator support shared by all
/// folding sets, which knows how to walk the folding set hash table.
class FoldingSetIteratorImpl
{
protected:
   FoldingSetNode *m_nodePtr;

   FoldingSetIteratorImpl(void **bucket);

   void advance();

public:
   bool operator==(const FoldingSetIteratorImpl &other) const
   {
      return m_nodePtr == other.m_nodePtr;
   }
   bool operator!=(const FoldingSetIteratorImpl &other) const
   {
      return m_nodePtr != other.m_nodePtr;
   }
};

template <typename T>
class FoldingSetIterator : public FoldingSetIteratorImpl
{
public:
   explicit FoldingSetIterator(void **bucket)
      : FoldingSetIteratorImpl(bucket)
   {}

   T &operator*() const
   {
      return *static_cast<T*>(m_nodePtr);
   }

   T *operator->() const
   {
      return static_cast<T*>(m_nodePtr);
   }

   inline FoldingSetIterator &operator++()
   {          // Preincrement
      advance();
      return *this;
   }

   FoldingSetIterator operator++(int)
   {        // Postincrement
      FoldingSetIterator tmp = *this;
      ++*this;
      return tmp;
   }
};

//===----------------------------------------------------------------------===//
/// FoldingSetBucketIteratorImpl - This is the common bucket iterator support
/// shared by all folding sets, which knows how to walk a particular bucket
/// of a folding set hash table.
class FoldingSetBucketIteratorImpl
{
protected:
   void *m_ptr;

   explicit FoldingSetBucketIteratorImpl(void **bucket);

   FoldingSetBucketIteratorImpl(void **bucket, bool) : m_ptr(bucket)
   {}

   void advance()
   {
      void *probe = static_cast<FoldingSetNode*>(m_ptr)->getNextInBucket();
      uintptr_t x = reinterpret_cast<uintptr_t>(probe) & ~0x1;
      m_ptr = reinterpret_cast<void*>(x);
   }

public:
   bool operator==(const FoldingSetBucketIteratorImpl &other) const
   {
      return m_ptr == other.m_ptr;
   }

   bool operator!=(const FoldingSetBucketIteratorImpl &other) const {
      return m_ptr != other.m_ptr;
   }
};

template <typename T>
class FoldingSetBucketIterator : public FoldingSetBucketIteratorImpl
{
public:
   explicit FoldingSetBucketIterator(void **bucket) :
      FoldingSetBucketIteratorImpl(bucket) {}

   FoldingSetBucketIterator(void **bucket, bool) :
      FoldingSetBucketIteratorImpl(bucket, true) {}

   T &operator*() const
   {
      return *static_cast<T*>(m_ptr);
   }

   T *operator->() const
   {
      return static_cast<T*>(m_ptr);
   }

   inline FoldingSetBucketIterator &operator++()
   { // Preincrement
      advance();
      return *this;
   }

   FoldingSetBucketIterator operator++(int)
   {      // Postincrement
      FoldingSetBucketIterator tmp = *this;
      ++*this;
      return tmp;
   }
};

//===----------------------------------------------------------------------===//
/// FoldingSetNodeWrapper - This template class is used to "wrap" arbitrary
/// types in an enclosing object so that they can be inserted into FoldingSets.
template <typename T>
class FoldingSetNodeWrapper : public FoldingSetNode
{
   T m_data;

public:
   template <typename... Ts>
   explicit FoldingSetNodeWrapper(Ts &&... args)
      : m_data(std::forward<Ts>(args)...) {}

   void profile(FoldingSetNodeId &id)
   {
      FoldingSetTrait<T>::profile(m_data, id);
   }

   T &getValue()
   {
      return m_data;
   }

   const T &getValue() const
   {
      return m_data;
   }

   operator T&()
   {
      return m_data;
   }

   operator const T&() const
   {
      return m_data;
   }
};

//===----------------------------------------------------------------------===//
/// FastFoldingSetNode - This is a subclass of FoldingSetNode which stores
/// a FoldingSetNodeId value rather than requiring the node to recompute it
/// each time it is needed. This trades space for speed (which can be
/// significant if the ID is long), and it also permits nodes to drop
/// information that would otherwise only be required for recomputing an ID.
class FastFoldingSetNode : public FoldingSetNode
{
   FoldingSetNodeId m_fastId;

protected:
   explicit FastFoldingSetNode(const FoldingSetNodeId &id)
      : m_fastId(id)
   {}

public:
   void profile(FoldingSetNodeId &id) const
   {
      id.addNodeId(m_fastId);
   }
};

//===----------------------------------------------------------------------===//
// Partial specializations of FoldingSetTrait.

template<typename T> struct FoldingSetTrait<T*>
{
   static inline void profile(T *value, FoldingSetNodeId &id)
   {
      id.addPointer(value);
   }
};
template <typename T1, typename T2>
struct FoldingSetTrait<std::pair<T1, T2>>
{
   static inline void profile(const std::pair<T1, T2> &value,
                              FoldingSetNodeId &id)
   {
      id.add(value.first);
      id.add(value.second);
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_FOLDING_SET_H
