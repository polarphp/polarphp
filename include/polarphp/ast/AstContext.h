//===--- AstContext.h - AST Context Object ----------------------*- C++ -*-===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines the AstContext interface.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ASTCONTEXT_H
#define POLARPHP_AST_ASTCONTEXT_H

#include "polarphp/global/DataTypes.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/SearchPathOptions.h"
#include "polarphp/basic/LangOptions.h"
#include "polarphp/basic/Malloc.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/MapVector.h"
#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/basic/adt/SetVector.h"
#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/TinyPtrVector.h"
#include "polarphp/utils/Allocator.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace polar::ast {

class AstContext;
enum class Associativity : unsigned char;
class BoundGenericType;
class ConstructorDecl;
class Decl;
class DeclContext;
class DefaultArgumentInitializer;
class FuncDecl;
class GenericContext;
class InFlightDiagnostic;
class IterableDeclContext;
class SourceFile;
class SourceLoc;
class Type;
class TypeVariableType;
class TupleType;
class FunctionType;
class GenericSignatureBuilder;
class ArchetypeType;
class Identifier;
class InheritedNameSet;
class NominalTypeDecl;
enum PointerTypeKind : unsigned;
class PrecedenceGroupDecl;
class TupleTypeElt;
class EnumElementDecl;
class ProtocolDecl;
class SubstitutableType;
class SourceManager;
class ValueDecl;
class DiagnosticEngine;
struct RawComment;
class DocComment;
class TypeAliasDecl;
class VarDecl;
class UnifiedStatsReporter;
enum class KnownProtocolKind : uint8_t;

using polar::basic::SearchPathOptions;
using polar::basic::LangOptions;
using polar::basic::SetVector;
using polar::basic::StringMap;
using polar::utils::BumpPtrAllocator;

/// The arena in which a particular AstContext allocation will go.
enum class AllocationArena
{
   /// The permanent arena, which is tied to the lifetime of
   /// the AstContext.
   ///
   /// All global declarations and types need to be allocated into this arena.
   /// At present, everything that is not a type involving a type variable is
   /// allocated in this arena.
   Permanent,
   /// The constraint solver's temporary arena, which is tied to the
   /// lifetime of a particular instance of the constraint solver.
   ///
   /// Any type involving a type variable is allocated in this arena.
   ConstraintSolver
};

class AstContext final
{
   AstContext(const AstContext&) = delete;
   void operator=(const AstContext&) = delete;
   AstContext(LangOptions &langOpts, SearchPathOptions &searchPathOpts,
              SourceManager &sourceMgr, DiagnosticEngine &diags);

public:
   // Members that should only be used by AstContext.cpp.
   struct Implementation;
   Implementation &getImpl() const;

   void operator delete(void *data) throw();

   static AstContext *get(LangOptions &langOpts,
                          SearchPathOptions &searchPathOpts,
                          SourceManager &sourceMgr,
                          DiagnosticEngine &diags);
   ~AstContext();

   /// Optional table of counters to report, nullptr when not collecting.
   ///
   /// This must be initialized early so that allocate() doesn't try to access
   /// it before being set to null.
   UnifiedStatsReporter *stats = nullptr;

   /// The language options used for translation.
   LangOptions &langOpts;

   /// The search path options used by this AST context.
   SearchPathOptions &searchPathOpts;

   /// The source manager object.
   SourceManager &sourceMgr;

   /// Diags - The diagnostics engine.
   DiagnosticEngine &diags;

   /// The list of external definitions imported by this context.
   SetVector<Decl *> externalDefinitions;

   /// FIXME: HACK HACK HACK
   /// This state should be tracked somewhere else.
   unsigned lastCheckedExternalDefinition = 0;

   /// Cache for names of canonical GenericTypeParamTypes.
   mutable DenseMap<unsigned, Identifier>
   canonicalGenericTypeParamTypeNames;

   /// Cache of remapped types (useful for diagnostics).
   StringMap<Type> remappedTypes;

private:
   /// The current generation number, which reflects the number of
   /// times that external modules have been loaded.
   ///
   /// Various places in the AST, such as the set of extensions associated with
   /// a nominal type, keep track of the generation number they saw and will
   /// automatically update when they are out of date.
   unsigned m_currentGeneration = 0;

   /// Retrieve the allocator for the given arena.
   BumpPtrAllocator &
   getAllocator(AllocationArena arena = AllocationArena::Permanent) const;

public:
   /// allocate - allocate memory from the AstContext bump pointer.
   void *allocate(unsigned long bytes, unsigned alignment,
                  AllocationArena arena = AllocationArena::Permanent) const
   {
      if (bytes == 0) {
         return nullptr;
      }

      if (langOpts.useMalloc) {
         return polar::basic::aligned_alloc(bytes, alignment);
      }
      if (arena == AllocationArena::Permanent && stats) {
         //stats->getFrontendCounters().NumASTBytesAllocated += bytes;
      }

      return getAllocator(arena).allocate(bytes, alignment);
   }

   template <typename T>
   T *allocate(AllocationArena arena = AllocationArena::Permanent) const
   {
      T *res = (T *) allocate(sizeof(T), alignof(T), arena);
      new (res) T();
      return res;
   }

   template <typename T>
   MutableArrayRef<T> allocateUninitialized(unsigned numElts,
                                            AllocationArena Arena = AllocationArena::Permanent) const
   {
      T *data = (T *) allocate(sizeof(T) * numElts, alignof(T), Arena);
      return { data, numElts };
   }

   template <typename T>
   MutableArrayRef<T> allocate(unsigned numElts,
                               AllocationArena arena = AllocationArena::Permanent) const
   {
      T *res = (T *) allocate(sizeof(T) * numElts, alignof(T), arena);
      for (unsigned i = 0; i != numElts; ++i) {
         new (res+i) T();
      }
      return {res, numElts};
   }

   /// allocate a copy of the specified object.
   template <typename T>
   typename std::remove_reference<T>::type *allocateObjectCopy(T &&t,
                                                               AllocationArena arena = AllocationArena::Permanent) const
   {
      // This function cannot be named allocateCopy because it would always win
      // overload resolution over the allocateCopy(ArrayRef<T>).
      using TNoRef = typename std::remove_reference<T>::type;
      TNoRef *res = (TNoRef *) allocate(sizeof(TNoRef), alignof(TNoRef), arena);
      new (res) TNoRef(std::forward<T>(t));
      return res;
   }

   template <typename T, typename It>
   T *allocateCopy(It start, It end,
                   AllocationArena arena = AllocationArena::Permanent) const
   {
      T *res = (T*)allocate(sizeof(T)*(end-start), alignof(T), arena);
      for (unsigned i = 0; start != end; ++start, ++i) {
         new (res+i) T(*start);
      }
      return res;
   }

   template<typename T, size_t N>
   MutableArrayRef<T> allocateCopy(T (&array)[N],
                                   AllocationArena arena = AllocationArena::Permanent) const
   {
      return MutableArrayRef<T>(allocateCopy<T>(array, array+N, arena), N);
   }

   template<typename T>
   MutableArrayRef<T> allocateCopy(ArrayRef<T> array,
                                   AllocationArena arena = AllocationArena::Permanent) const
   {
      return MutableArrayRef<T>(allocateCopy<T>(array.begin(),array.end(), arena),
                                array.size());
   }


   template<typename T>
   ArrayRef<T> allocateCopy(const SmallVectorImpl<T> &vec,
                            AllocationArena arena = AllocationArena::Permanent) const
   {
      return allocateCopy(ArrayRef<T>(vec), arena);
   }

   template<typename T>
   MutableArrayRef<T>
   allocateCopy(SmallVectorImpl<T> &vec,
                AllocationArena arena = AllocationArena::Permanent) const
   {
      return allocateCopy(MutableArrayRef<T>(vec), arena);
   }

   StringRef allocateCopy(StringRef Str,
                          AllocationArena arena = AllocationArena::Permanent) const
   {
      ArrayRef<char> Result =
            allocateCopy(polar::basic::make_array_ref(Str.data(), Str.size()), arena);
      return StringRef(Result.data(), Result.size());
   }

   template<typename T, typename Vector, typename Set>
   MutableArrayRef<T>
   allocateCopy(SetVector<T, Vector, Set> setVector,
                AllocationArena arena = AllocationArena::Permanent) const
   {
      return MutableArrayRef<T>(allocateCopy<T>(setVector.begin(),
                                                setVector.end(),
                                                arena),
                                setVector.size());
   }

};

} // polar::ast

#endif // POLARPHP_AST_ASTCONTEXT_H
