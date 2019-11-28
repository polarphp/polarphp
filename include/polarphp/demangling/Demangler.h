//===--- Demangler.h - String to Node-Tree Demangling -----------*- C++ -*-===//
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
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/28.
//
//===----------------------------------------------------------------------===//
// This file is the compiler-private API of the demangler.
// It should only be used within the polarphp compiler or runtime library, but not
// by external tools which use the demangler library (like lldb).
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_DEMANGLING_DEMANGLER_H
#define POLARPHP_DEMANGLING_DEMANGLER_H

#include "polarphp/demangling/Demangle.h"

//#define NODE_FACTORY_DEBUGGING

#ifdef NODE_FACTORY_DEBUGGING
#include <iostream>
#endif

namespace polar::demangling {

using llvm::StringRef;

class CharVector;

/// The allocator for demangling nodes and other demangling-internal stuff.
///
/// It implements a simple bump-pointer allocator.
class NodeFactory
{

   /// Position in the current slab.
   char *m_curPtr = nullptr;

   /// The end of the current slab.
   char *m_end = nullptr;

   struct Slab
   {
      // The previously allocated slab.
      Slab *previous;
      // Tail allocated memory starts here.
   };

   /// The head of the single-linked slab list.
   Slab *m_currentSlab = nullptr;

   /// The size of the previously allocated slab.
   ///
   /// The slab size can only grow, even clear() does not reset the slab size.
   /// This initial size is good enough to fit most de-manglings.
   size_t m_slabSize = 100 * sizeof(Node);

   static char *align(char *ptr, size_t alignment) {
      assert(alignment > 0);
      return reinterpret_cast<char*>(((reinterpret_cast<uintptr_t>(ptr) + alignment - 1)
                                      & ~(reinterpret_cast<uintptr_t>(alignment) - 1)));
   }

   static void freeSlabs(Slab *slab);

   /// If not null, the NodeFactory from which this factory borrowed free memory.
   NodeFactory *m_borrowedFrom = nullptr;

   /// True if some other NodeFactory borrowed free memory from this factory.
   bool isBorrowed = false;

#ifdef NODE_FACTORY_DEBUGGING
   size_t allocatedMemory = 0;
   static int nestingLevel;
   std::string indent() { return std::string(nestingLevel * 2, ' '); }
#endif

public:

   NodeFactory()
   {
#ifdef NODE_FACTORY_DEBUGGING
      std::cerr << indent() << "## New NodeFactory\n";
      nestingLevel++;
#endif
   }

   /// Provide pre-allocated memory, e.g. memory on the stack.
   ///
   /// Only if this memory overflows, the factory begins to malloc.
   void providePreallocatedMemory(char *memory, size_t size)
   {
#ifdef NODE_FACTORY_DEBUGGING
      std::cerr << indent() << "++ provide preallocated memory, size = "
                << size << '\n';
#endif
      assert(!m_curPtr && !m_end && !m_currentSlab);
      m_curPtr = memory;
      m_end = m_curPtr + size;
   }

   /// Borrow free memory from another factory \p borrowFrom.
   ///
   /// While this factory is alive, no allocations can be done in the
   /// \p borrowFrom factory.
   void providePreallocatedMemory(NodeFactory &borrowFrom)
   {
      assert(!m_curPtr && !m_end && !m_currentSlab);
      assert(!borrowFrom.isBorrowed && !m_borrowedFrom);
      borrowFrom.isBorrowed = true;
      m_borrowedFrom = &borrowFrom;
      m_curPtr = borrowFrom.m_curPtr;
      m_end = borrowFrom.m_end;
#ifdef NODE_FACTORY_DEBUGGING
      std::cerr << indent() << "++ borrow memory, size = "
                << (m_end - m_curPtr) << '\n';
#endif
   }

   virtual ~NodeFactory()
   {
      freeSlabs(m_currentSlab);
#ifdef NODE_FACTORY_DEBUGGING
      nestingLevel--;
      std::cerr << indent() << "## Delete NodeFactory: allocated memory = "
                << allocatedMemory  << '\n';
#endif
      if (m_borrowedFrom) {
         m_borrowedFrom->isBorrowed = false;
      }
   }

   virtual void clear();

   /// Allocates an object of type T or an array of objects of type T.
   template<typename T> T *allocate(size_t numObjects = 1)
   {
      assert(!isBorrowed);
      size_t objectSize = numObjects * sizeof(T);
      m_curPtr = align(m_curPtr, alignof(T));
#ifdef NODE_FACTORY_DEBUGGING
      std::cerr << indent() << "alloc " << objectSize << ", m_curPtr = "
                << (void *)m_curPtr << "\n";
      allocatedMemory += objectSize;
#endif

      // Do we have enough space in the current slab?
      if (m_curPtr + objectSize > m_end) {
         // No. We have to malloc a new slab.
         // We double the slab size for each allocated slab.
         m_slabSize = std::max(m_slabSize * 2, objectSize + alignof(T));
         size_t allocSize = sizeof(Slab) + m_slabSize;
         Slab *newSlab = (Slab *)malloc(allocSize);

         // Insert the new slab in the single-linked list of slabs.
         newSlab->previous = m_currentSlab;
         m_currentSlab = newSlab;

         // Initialize the pointers to the new slab.
         m_curPtr = align((char *)(newSlab + 1), alignof(T));
         m_end = (char *)newSlab + allocSize;
         assert(m_curPtr + objectSize <= m_end);
#ifdef NODE_FACTORY_DEBUGGING
         std::cerr << indent() << "** new slab " << newSlab << ", allocsize = "
                   << allocSize << ", m_curPtr = " << (void *)m_curPtr
                   << ", m_end = " << (void *)m_end << "\n";
#endif
      }
      T *allocatedObj = (T *)m_curPtr;
      m_curPtr += objectSize;
      return allocatedObj;
   }

   /// Tries to enlarge the \p m_capacity of an array of \p objects.
   ///
   /// If \p objects is allocated at the end of the current slab and the slab
   /// has enough free space, the \p m_capacity is simply enlarged and no new
   /// allocation needs to be done.
   /// Otherwise a new array of objects is allocated and \p objects is set to the
   /// new memory address.
   /// The \p m_capacity is enlarged at least by \p minGrowth, but can also be
   /// enlarged by a bigger value.
   template<typename T> void reallocate(T *&objects, uint32_t &m_capacity,
                                        size_t minGrowth)
   {
      assert(!isBorrowed);
      size_t oldAllocSize = m_capacity * sizeof(T);
      size_t additionalAlloc = minGrowth * sizeof(T);

#ifdef NODE_FACTORY_DEBUGGING
      std::cerr << indent() << "realloc: m_capacity = " << m_capacity
                << " (size = " << oldAllocSize << "), growth = " << minGrowth
                << " (size = " << additionalAlloc << ")\n";
#endif
      if ((char *)objects + oldAllocSize == m_curPtr
          && m_curPtr + additionalAlloc <= m_end) {
         // The existing array is at the end of the current slab and there is
         // enough space. So we are fine.
         m_curPtr += additionalAlloc;
         m_capacity += minGrowth;
#ifdef NODE_FACTORY_DEBUGGING
         std::cerr << indent() << "** can grow: " << (void *)m_curPtr << '\n';
         allocatedMemory += additionalAlloc;
#endif
         return;
      }
      // We need a new allocation.
      size_t growth = (minGrowth >= 4 ? minGrowth : 4);
      if (growth < m_capacity * 2) {
         growth = m_capacity * 2;
      }
      T *newObjects = allocate<T>(m_capacity + growth);
      memcpy(newObjects, objects, oldAllocSize);
      objects = newObjects;
      m_capacity += growth;
   }

   /// Creates a node of kind \p kind.
   NodePointer createNode(Node::Kind kind);

   /// Creates a node of kind \p kind with an \p index payload.
   NodePointer createNode(Node::Kind kind, Node::IndexType index);

   /// Creates a node of kind \p kind with a \p m_text payload.
   ///
   /// The \p m_text string must be already allocated with the factory and therefore
   /// it is _not_ copied.
   NodePointer createNodeWithAllocatedText(Node::Kind kind, llvm::StringRef m_text);

   /// Creates a node of kind \p kind with a \p m_text payload.
   ///
   /// The \p m_text string is copied.
   NodePointer createNode(Node::Kind kind, llvm::StringRef m_text)
   {
      return createNodeWithAllocatedText(kind, m_text.copy(*this));
   }

   /// Creates a node of kind \p kind with a \p m_text payload.
   ///
   /// The \p m_text string is already allocated with the factory and therefore
   /// it is _not_ copied.
   NodePointer createNode(Node::Kind kind, const CharVector &m_text);

   /// Creates a node of kind \p kind with a \p m_text payload, which must be a C
   /// string literal.
   ///
   /// The \p m_text string is _not_ copied.
   NodePointer createNode(Node::Kind kind, const char *m_text);
};

/// A vector with a storage managed by a NodeFactory.
///
/// This Vector class only provides the minimal functionality needed by the
/// Demangler.
template<typename T>
class Vector
{

protected:
   T *m_elems = nullptr;
   uint32_t m_numElems = 0;
   uint32_t m_capacity = 0;

public:
   using iterator = T *;

   Vector() { }

   /// Construct a vector with an initial m_capacity.
   explicit Vector(NodeFactory &factory, size_t initialCapacity)
   {
      init(factory, initialCapacity);
   }

   /// Clears the content and re-allocates the buffer with an initial m_capacity.
   void init(NodeFactory &factory, size_t initialCapacity)
   {
      m_elems = factory.allocate<T>(initialCapacity);
      m_numElems = 0;
      m_capacity = initialCapacity;
   }

   void free()
   {
      m_capacity = 0;
      m_elems = 0;
   }

   void clear()
   {
      m_numElems = 0;
   }

   iterator begin()
   {
      return m_elems;
   }

   iterator end()
   {
      return m_elems + m_numElems;
   }

   T &operator[](size_t idx)
   {
      assert(idx < m_numElems);
      return m_elems[idx];
   }

   const T &operator[](size_t idx) const
   {
      assert(idx < m_numElems);
      return m_elems[idx];
   }

   size_t size() const
   {
      return m_numElems;
   }

   bool empty() const
   {
      return m_numElems == 0;
   }

   T &back()
   {
      return (*this)[m_numElems - 1];
   }

   void resetSize(size_t toPos)
   {
      assert(toPos <= m_numElems);
      m_numElems = toPos;
   }

   void push_back(const T &newElem, NodeFactory &factory)
   {
      if (m_numElems >= m_capacity) {
         factory.reallocate(m_elems, m_capacity, /*growth*/ 1);
      }
      assert(m_numElems < m_capacity);
      m_elems[m_numElems++] = newElem;
   }

   T pop_back_val()
   {
      if (empty()) {
         return T();
      }
      T val = (*this)[m_numElems - 1];
      m_numElems--;
      return val;
   }
};

/// A vector of chars (a string) with a storage managed by a NodeFactory.
///
/// This CharVector class only provides the minimal functionality needed by the
/// Demangler.
class CharVector : public Vector<char>
{
public:
   // Append another string.
   void append(StringRef rhs, NodeFactory &factory);

   // Append an integer as readable number.
   void append(int number, NodeFactory &factory);

   // Append an unsigned 64 bit integer as readable number.
   void append(unsigned long long number, NodeFactory &factory);

   StringRef str() const
   {
      return StringRef(m_elems, m_numElems);
   }
};

/// Kinds of symbolic reference supported.
enum class SymbolicReferenceKind : uint8_t
{
   /// A symbolic reference to a context descriptor, representing the
   /// (unapplied generic) context.
   Context,
   /// A symbolic reference to an accessor function, which can be executed in
   /// the process to get a pointer to the referenced entity.
   AccessorFunctionReference,
};

using SymbolicReferenceResolverType = NodePointer (SymbolicReferenceKind,
Directness,
int32_t, const void *);

/// The demangler.
///
/// It de-mangles a string and it also owns the returned node-tree. This means
/// The nodes of the tree only live as long as the Demangler itself.
class Demangler : public NodeFactory
{
protected:
   StringRef m_text;
   size_t m_pos = 0;

   /// Mangling style where function type would have
   /// labels attached to it, instead of having them
   /// as part of the name.
   bool m_isOldFunctionTypeMangling = false;

   Vector<NodePointer> m_nodeStack;
   Vector<NodePointer> m_substitutions;

   static const int MaxNumWords = 26;
   StringRef m_words[MaxNumWords];
   int m_numWords = 0;

   std::function<SymbolicReferenceResolverType> m_symbolicReferenceResolver;

   bool nextIf(StringRef str)
   {
      if (!m_text.substr(m_pos).startswith(str)) {
         return false;
      }
      m_pos += str.size();
      return true;
   }

   char peekChar()
   {
      if (m_pos >= m_text.size()) {
         return 0;
      }
      return m_text[m_pos];
   }

   char nextChar()
   {
      if (m_pos >= m_text.size()) {
         return 0;
      }
      return m_text[m_pos++];
   }

   bool nextIf(char c)
   {
      if (peekChar() != c) {
         return false;
      }
      m_pos++;
      return true;
   }

   void pushBack()
   {
      assert(m_pos > 0);
      m_pos--;
   }

   StringRef consumeAll()
   {
      StringRef str = m_text.drop_front(m_pos);
      m_pos = m_text.size();
      return str;
   }

   void pushNode(NodePointer pointer)
   {
      m_nodeStack.push_back(pointer, *this);
   }

   NodePointer popNode()
   {
      return m_nodeStack.pop_back_val();
   }

   NodePointer popNode(Node::Kind kind)
   {
      if (m_nodeStack.empty()) {
         return nullptr;
      }

      Node::Kind ndKind = m_nodeStack.back()->getKind();
      if (ndKind != kind) {
         return nullptr;
      }
      return popNode();
   }

   template <typename Pred>
   NodePointer popNode(Pred pred)
   {
      if (m_nodeStack.empty()) {
         return nullptr;
      }
      Node::Kind ndKind = m_nodeStack.back()->getKind();
      if (!pred(ndKind)) {
         return nullptr;
      }
      return popNode();
   }

   /// This class handles preparing the initial state for a demangle job in a reentrant way, pushing the
   /// existing state back when a demangle job is completed.
   class DemangleInitRAII
   {
      Demangler &m_dem;
      Vector<NodePointer> m_nodeStack;
      Vector<NodePointer> m_substitutions;
      int m_numWords;
      StringRef m_text;
      size_t m_pos;

   public:
      DemangleInitRAII(Demangler &m_dem, StringRef mangledName);
      ~DemangleInitRAII();
   };
   friend DemangleInitRAII;

   void addSubstitution(NodePointer pointer)
   {
      if (pointer) {
         m_substitutions.push_back(pointer, *this);
      }
   }

   NodePointer addChild(NodePointer parent, NodePointer child);
   NodePointer createWithChild(Node::Kind kind, NodePointer child);
   NodePointer createType(NodePointer child);
   NodePointer createWithChildren(Node::Kind kind, NodePointer child1,
                                  NodePointer child2);
   NodePointer createWithChildren(Node::Kind kind, NodePointer child1,
                                  NodePointer child2, NodePointer Child3);
   NodePointer createWithChildren(Node::Kind kind, NodePointer child1,
                                  NodePointer child2, NodePointer Child3,
                                  NodePointer Child4);
   NodePointer createWithPoppedType(Node::Kind kind)
   {
      return createWithChild(kind, popNode(Node::Kind::Type));
   }

   bool parseAndPushNodes();

   NodePointer changeKind(NodePointer node, Node::Kind newKind);

   NodePointer demangleOperator();

   int demangleNatural();
   int demangleIndex();
   NodePointer demangleIndexAsNode();
   NodePointer demangleDependentConformanceIndex();
   NodePointer demangleIdentifier();
   NodePointer demangleOperatorIdentifier();

   std::string demangleBridgedMethodParams();

   NodePointer demangleMultiSubstitutions();
   NodePointer pushMultiSubstitutions(int repeatCount, size_t substIdx);
   NodePointer createSwiftType(Node::Kind typeKind, const char *name);
   NodePointer demangleStandardSubstitution();
   NodePointer createStandardSubstitution(char subst);
   NodePointer demangleLocalIdentifier();

   NodePointer popModule();
   NodePointer popContext();
   NodePointer popTypeAndGetChild();
   NodePointer popTypeAndGetAnyGeneric();
   NodePointer demangleBuiltinType();
   NodePointer demangleAnyGenericType(Node::Kind kind);
   NodePointer demangleExtensionContext();
   NodePointer demanglePlainFunction();
   NodePointer popFunctionType(Node::Kind kind);
   NodePointer popFunctionParams(Node::Kind kind);
   NodePointer popFunctionParamLabels(NodePointer funcType);
   NodePointer popTuple();
   NodePointer popTypeList();
   NodePointer popProtocol();
   NodePointer demangleBoundGenericType();
   NodePointer demangleBoundGenericArgs(NodePointer nominalType,
                                        const Vector<NodePointer> &typeLists,
                                        size_t typeListIdx);
   NodePointer popAnyProtocolConformanceList();
   NodePointer demangleRetroactiveConformance();
   NodePointer demangleInitializer();
   NodePointer demangleImplParamConvention();
   NodePointer demangleImplResultConvention(Node::Kind convKind);
   NodePointer demangleImplFunctionType();
   NodePointer demangleMetatype();
   NodePointer demanglePrivateContextDescriptor();
   NodePointer createArchetypeRef(int depth, int i);
   NodePointer demangleArchetype();
   NodePointer demangleAssociatedTypeSimple(NodePointer genericParamIdx);
   NodePointer demangleAssociatedTypeCompound(NodePointer genericParamIdx);

   NodePointer popAssocTypeName();
   NodePointer popAssocTypePath();
   NodePointer getDependentGenericParamType(int depth, int index);
   NodePointer demangleGenericParamIndex();
   NodePointer popProtocolConformance();
   NodePointer demangleRetroactiveProtocolConformanceRef();
   NodePointer popAnyProtocolConformance();
   NodePointer demangleConcreteProtocolConformance();
   NodePointer popDependentProtocolConformance();
   NodePointer demangleDependentProtocolConformanceRoot();
   NodePointer demangleDependentProtocolConformanceInherited();
   NodePointer popDependentAssociatedConformance();
   NodePointer demangleDependentProtocolConformanceAssociated();
   NodePointer demangleThunkOrSpecialization();
   NodePointer demangleGenericSpecialization(Node::Kind specKind);
   NodePointer demangleFunctionSpecialization();
   NodePointer demangleFuncSpecParam(Node::Kind kind);
   NodePointer addFuncSpecParamNumber(NodePointer param,
                                      FunctionSigSpecializationParamKind kind);

   NodePointer demangleSpecAttributes(Node::Kind specKind);

   NodePointer demangleWitness();
   NodePointer demangleSpecialType();
   NodePointer demangleMetatypeRepresentation();
   NodePointer demangleAccessor(NodePointer childNode);
   NodePointer demangleFunctionEntity();
   NodePointer demangleEntity(Node::Kind kind);
   NodePointer demangleVariable();
   NodePointer demangleSubscript();
   NodePointer demangleProtocolList();
   NodePointer demangleProtocolListType();
   NodePointer demangleGenericSignature(bool hasParamCounts);
   NodePointer demangleGenericRequirement();
   NodePointer demangleGenericType();
   NodePointer demangleValueWitness();

   NodePointer demangleObjCTypeName();
   NodePointer demangleTypeMangling();
   NodePointer demangleSymbolicReference(unsigned char rawKind,
                                         const void *at);

   bool demangleBoundGenerics(Vector<NodePointer> &typeListList,
                              NodePointer &retroactiveConformances);

   void dump();

public:
   Demangler() {}

   void clear() override;

   /// Install a resolver for symbolic references in a mangled string.
   void setSymbolicReferenceResolver(
         std::function<SymbolicReferenceResolverType> resolver)
   {
      m_symbolicReferenceResolver = resolver;
   }

   /// Take the symbolic reference resolver.
   std::function<SymbolicReferenceResolverType> &&
   takeSymbolicReferenceResolver()
   {
      return std::move(m_symbolicReferenceResolver);
   }

   /// Demangle the given symbol and return the parse tree.
   ///
   /// \param mangledName The mangled symbol string, which start with the
   /// mangling prefix $S.
   ///
   /// \returns A parse tree for the demangled string - or a null pointer
   /// on failure.
   /// The lifetime of the returned node tree ends with the lifetime of the
   /// Demangler or with a call of clear().
   NodePointer demangleSymbol(StringRef mangledName);

   /// Demangle the given type and return the parse tree.
   ///
   /// \param mangledName The mangled type string, which does _not_ start with
   /// the mangling prefix $S.
   ///
   /// \returns A parse tree for the demangled string - or a null pointer
   /// on failure.
   /// The lifetime of the returned node tree ends with the lifetime of the
   /// Demangler or with a call of clear().
   NodePointer demangleType(StringRef mangledName);
};

/// A demangler which uses stack space for its initial memory.
///
/// The \p size paramter specifies the size of the stack space.
template <size_t size>
class StackAllocatedDemangler : public Demangler
{
   char m_stackSpace[size];

public:
   StackAllocatedDemangler()
   {
      providePreallocatedMemory(m_stackSpace, size);
   }
};

NodePointer demangle_old_symbol_as_node(StringRef mangledName,
                                        NodeFactory &factory);


} // demangling

#endif // POLARPHP_DEMANGLING_DEMANGLER_H
