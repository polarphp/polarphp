//===--- Identifier.h - Uniqued Identifier ----------------------*- c++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/11/28.
//===----------------------------------------------------------------------===//
//
// This file defines the Identifier interface.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_IDENTIFIER_H
#define POLARPHP_AST_IDENTIFIER_H

#include "polarphp/basic/EditorPlaceholder.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/TrailingObjects.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/IR/BasicBlock.h"

namespace llvm {
class raw_ostream;
}

namespace polar::ast {
class AstContext;
class ParameterList;

/// DeclRefKind - The kind of reference to an identifier.
enum class DeclRefKind
{
   /// An ordinary reference to an identifier, e.g. 'foo'.
   Ordinary,

   /// A reference to an identifier as a binary operator, e.g. '+' in 'a+b'.
   BinaryOperator,

   /// A reference to an identifier as a postfix unary operator, e.g. '++' in
   /// 'a++'.
   PostfixOperator,

   /// A reference to an identifier as a prefix unary operator, e.g. '--' in
   /// '--a'.
   PrefixOperator
};

/// Identifier - This is an instance of a uniqued identifier created by
/// AstContext.  It just wraps a nul-terminated "const char*".
class Identifier
{
   friend class AstContext;
   friend class DeclBaseName;
   const char *m_pointer;
   /// Constructor, only accessible by AstContext, which handles the uniquing.
   explicit Identifier(const char *ptr)
      : m_pointer(ptr)
   {}
public:
   explicit Identifier() : m_pointer(nullptr)
   {}

   const char *get() const
   {
      return m_pointer;
   }

   StringRef str() const
   {
      return m_pointer;
   }

   unsigned getLength() const
   {
      assert(m_pointer != nullptr && "Tried getting length of empty identifier");
      return ::strlen(m_pointer);
   }

   bool empty() const
   {
      return m_pointer == nullptr;
   }

   bool is(StringRef string) const
   {
      return str().equals(string);
   }

   /// isOperator - Return true if this identifier is an operator, false if it is
   /// a normal identifier.
   /// FIXME: We should maybe cache this.
   bool isOperator() const
   {
      if (empty()) {
         return false;
      }
      if (isEditorPlaceholder()) {
         return false;
      }
      if (static_cast<unsigned char>(m_pointer[0]) < 0x80) {
         return isOperatorStartCodePoint(static_cast<unsigned char>(m_pointer[0]));
      }
      // Handle the high unicode case out of line.
      return isOperatorSlow();
   }

   /// isOperatorStartCodePoint - Return true if the specified code point is a
   /// valid start of an operator.
   static bool isOperatorStartCodePoint(uint32_t c)
   {
      // ASCII operator chars.
      static const char OpChars[] = "/=-+*%<>!&|^~.?";
      if (c < 0x80) {
         return memchr(OpChars, c, sizeof(OpChars) - 1) != nullptr;
      }
      // Unicode math, symbol, arrow, dingbat, and line/box drawing chars.
      return (c >= 0x00A1 && c <= 0x00A7)
            || c == 0x00A9 || c == 0x00AB || c == 0x00AC || c == 0x00AE
            || c == 0x00B0 || c == 0x00B1 || c == 0x00B6 || c == 0x00BB
            || c == 0x00BF || c == 0x00D7 || c == 0x00F7
            || c == 0x2016 || c == 0x2017 || (c >= 0x2020 && c <= 0x2027)
            || (c >= 0x2030 && c <= 0x203E) || (c >= 0x2041 && c <= 0x2053)
            || (c >= 0x2055 && c <= 0x205E) || (c >= 0x2190 && c <= 0x23FF)
            || (c >= 0x2500 && c <= 0x2775) || (c >= 0x2794 && c <= 0x2BFF)
            || (c >= 0x2E00 && c <= 0x2E7F) || (c >= 0x3001 && c <= 0x3003)
            || (c >= 0x3008 && c <= 0x3030);
   }

   /// isOperatorContinuationCodePoint - Return true if the specified code point
   /// is a valid operator code point.
   static bool isOperatorContinuationCodePoint(uint32_t c)
   {
      if (isOperatorStartCodePoint(c)) {
         return true;
      }
      // Unicode combining characters and variation selectors.
      return (c >= 0x0300 && c <= 0x036F)
            || (c >= 0x1DC0 && c <= 0x1DFF)
            || (c >= 0x20D0 && c <= 0x20FF)
            || (c >= 0xFE00 && c <= 0xFE0F)
            || (c >= 0xFE20 && c <= 0xFE2F)
            || (c >= 0xE0100 && c <= 0xE01EF);
   }

   static bool isEditorPlaceholder(StringRef name)
   {
      return polar::basic::is_editor_placeholder(name);
   }

   bool isEditorPlaceholder() const
   {
      return !empty() && isEditorPlaceholder(str());
   }

   const void *getAsOpaquem_pointer() const
   {
      return static_cast<const void *>(m_pointer);
   }

   static Identifier getFromOpaquePointer(void *pointer)
   {
      return Identifier(reinterpret_cast<const char*>(pointer));
   }

   /// Compare two identifiers, producing -1 if \c *this comes before \c rhs,
   /// 1 if \c *this comes after \c rhs, and 0 if they are equal.
   ///
   /// Null identifiers come after all rhs identifiers.
   int compare(Identifier other) const;

   bool operator==(Identifier other) const
   {
      return m_pointer == other.m_pointer;
   }

   bool operator!=(Identifier other) const
   {
      return !(*this==other);
   }

   bool operator<(Identifier other) const
   {
      return m_pointer < other.m_pointer;
   }

   static Identifier getEmptyKey()
   {
      return Identifier(reinterpret_cast<const char*>(
                           llvm::DenseMapInfo<const void*>::getEmptyKey()));
   }

   static Identifier getTombstoneKey()
   {
      return Identifier(reinterpret_cast<const char*>(
                           llvm::DenseMapInfo<const void*>::getTombstoneKey()));
   }

private:
   bool isOperatorSlow() const;
};

class DeclName;

} // end namespace polar::ast

namespace llvm {
raw_ostream &operator<<(raw_ostream &ostream, polar::ast::Identifier identifier);
raw_ostream &operator<<(raw_ostream &ostream, polar::ast::DeclName declName);

// Identifiers hash just like pointers.
template<>
struct DenseMapInfo<polar::ast::Identifier>
{
   static polar::ast::Identifier getEmptyKey()
   {
      return polar::ast::Identifier::getEmptyKey();
   }

   static polar::ast::Identifier getTombstoneKey()
   {
      return polar::ast::Identifier::getTombstoneKey();
   }

   static unsigned getHashValue(polar::ast::Identifier value)
   {
      return DenseMapInfo<const void*>::getHashValue(value.get());
   }

   static bool isEqual(polar::ast::Identifier lhs, polar::ast::Identifier rhs)
   {
      return lhs == rhs;
   }
};

// An Identifier is "pointer like".
template<typename T> struct PointerLikeTypeTraits;
template<>
struct PointerLikeTypeTraits<polar::ast::Identifier>
{
public:
   static inline void *getAsVoidm_pointer(polar::ast::Identifier identifier)
   {
      return const_cast<void *>(identifier.getAsOpaquem_pointer());
   }

   static inline polar::ast::Identifier getFromVoidPointer(void *pointer)
   {
      return polar::ast::Identifier::getFromOpaquePointer(pointer);
   }

   enum { NumLowBitsAvailable = 2 };
};

} // end namespace llvm

namespace polar::ast {

/// Wrapper that may either be an Identifier or a special name
/// (e.g. for subscripts)
class DeclBaseName
{
public:
   enum class Kind: uint8_t
   {
      Normal,
      Subscript,
      Constructor,
      Destructor
   };

private:
   /// In a special DeclName represenenting a subscript, this opaque pointer
   /// is used as the data of the base name identifier.
   /// This is an implementation detail that should never leak outside of
   /// DeclName.
   static void *sm_subscriptIdentifierData;
   /// As above, for special constructor DeclNames.
   static void *sm_constructorIdentifierData;
   /// As above, for special destructor DeclNames.
   static void *sm_destructorIdentifierData;

   Identifier m_ident;

public:
   DeclBaseName() : DeclBaseName(Identifier())
   {}

   DeclBaseName(Identifier identifier)
      : m_ident(identifier) {}

   static DeclBaseName createSubscript()
   {
      return DeclBaseName(Identifier(reinterpret_cast<const char *>(sm_subscriptIdentifierData)));
   }

   static DeclBaseName createConstructor()
   {
      return DeclBaseName(Identifier(reinterpret_cast<const char *>(sm_constructorIdentifierData)));
   }

   static DeclBaseName createDestructor()
   {
      return DeclBaseName(Identifier(reinterpret_cast<const char *>(sm_destructorIdentifierData)));
   }

   Kind getKind() const
   {
      if (m_ident.get() == sm_subscriptIdentifierData) {
         return Kind::Subscript;
      } else if (m_ident.get() == sm_constructorIdentifierData) {
         return Kind::Constructor;
      } else if (m_ident.get() == sm_destructorIdentifierData) {
         return Kind::Destructor;
      } else {
         return Kind::Normal;
      }
   }

   bool isSpecial() const
   {
      return getKind() != Kind::Normal;
   }

   /// Return the identifier backing the name. Assumes that the name is not
   /// special.
   Identifier getIdentifier() const
   {
      assert(!isSpecial() && "Cannot retrieve identifier from special names");
      return m_ident;
   }

   bool empty() const
   {
      return !isSpecial() && getIdentifier().empty();
   }

   bool isOperator() const
   {
      return !isSpecial() && getIdentifier().isOperator();
   }

   bool isEditorPlaceholder() const
   {
      return !isSpecial() && getIdentifier().isEditorPlaceholder();
   }

   /// A representation of the name to be displayed to users. May be ambiguous
   /// between identifiers and special names.
   StringRef userFacingName() const
   {
      if (empty()) {
         return "_";
      }
      switch (getKind()) {
      case Kind::Normal:
         return getIdentifier().str();
      case Kind::Subscript:
         return "subscript";
      case Kind::Constructor:
         return "init";
      case Kind::Destructor:
         return "deinit";
      }
      llvm_unreachable("unhandled kind");
   }

   int compare(DeclBaseName other) const
   {
      return userFacingName().compare(other.userFacingName());
   }

   bool operator==(StringRef other) const
   {
      return !isSpecial() && getIdentifier().is(other);
   }

   bool operator!=(StringRef other) const
   {
      return !(*this == other);
   }

   bool operator==(DeclBaseName other) const
   {
      return m_ident == other.m_ident;
   }

   bool operator!=(DeclBaseName other) const
   {
      return !(*this == other);
   }

   bool operator<(DeclBaseName other) const
   {
      return m_ident.get() < other.m_ident.get();
   }

   const void *getAsOpaquePointer() const
   {
      return m_ident.get();
   }

   static DeclBaseName getFromOpaquePointer(void *pointer)
   {
      return Identifier::getFromOpaquePointer(pointer);
   }
};

} // end namespace polar::ast

namespace llvm {

raw_ostream &operator<<(raw_ostream &ostream, polar::ast::DeclBaseName decl);

// DeclBaseNames hash just like pointers.
template<>
struct DenseMapInfo<polar::ast::DeclBaseName>
{
   static polar::ast::DeclBaseName getEmptyKey()
   {
      return polar::ast::Identifier::getEmptyKey();
   }

   static polar::ast::DeclBaseName getTombstoneKey()
   {
      return polar::ast::Identifier::getTombstoneKey();
   }

   static unsigned getHashValue(polar::ast::DeclBaseName value)
   {
      return DenseMapInfo<const void *>::getHashValue(value.getAsOpaquePointer());
   }

   static bool isEqual(polar::ast::DeclBaseName lhs, polar::ast::DeclBaseName rhs)
   {
      return lhs == rhs;
   }
};

// A DeclBaseName is "pointer like".
template <typename T>
struct PointerLikeTypeTraits;

template <>
struct PointerLikeTypeTraits<polar::ast::DeclBaseName>
{
public:
   static inline void *getAsVoidPointer(polar::ast::DeclBaseName decl)
   {
      return const_cast<void *>(decl.getAsOpaquePointer());
   }

   static inline polar::ast::DeclBaseName getFromVoidPointer(void *pointer)
   {
      return polar::ast::DeclBaseName::getFromOpaquePointer(pointer);
   }
   enum { NumLowBitsAvailable = PointerLikeTypeTraits<polar::ast::Identifier>::NumLowBitsAvailable };
};

} // end namespace llvm

namespace polar::ast {

/// A declaration name, which may comprise one or more identifier pieces.
class DeclName
{
   friend class AstContext;

   /// Represents a compound declaration name.
   struct /*alignas(Identifier)*/ CompoundDeclName final : llvm::FoldingSetNode,
         private llvm::TrailingObjects<CompoundDeclName, Identifier>
   {
      friend TrailingObjects;
      friend class DeclName;

      DeclBaseName baseName;
      size_t numArgs;

      explicit CompoundDeclName(DeclBaseName baseName, size_t numArgs)
         : baseName(baseName), numArgs(numArgs)
      {
         assert(numArgs > 0 && "Should use IdentifierAndCompound");
      }

      ArrayRef<Identifier> getArgumentNames() const
      {
         return {getTrailingObjects<Identifier>(), numArgs};
      }

      MutableArrayRef<Identifier> getArgumentNames()
      {
         return {getTrailingObjects<Identifier>(), numArgs};
      }

      /// Uniquing for the AstContext.
      static void profile(llvm::FoldingSetNodeID &id, DeclBaseName baseName,
                          ArrayRef<Identifier> argumentNames);

      void profile(llvm::FoldingSetNodeID &id)
      {
         profile(id, baseName, getArgumentNames());
      }
   };

   // A single stored identifier, along with a bit stating whether it is the
   // base name for a zero-argument compound name.
   typedef llvm::PointerIntPair<DeclBaseName, 1, bool> BaseNameAndCompound;

   // Either a single identifier piece stored inline (with a bit to say whether
   // it is simple or compound), or a reference to a compound declaration name.
   llvm::PointerUnion<BaseNameAndCompound, CompoundDeclName *> m_simpleOrCompound;

   DeclName(void *opaque)
      : m_simpleOrCompound(decltype(m_simpleOrCompound)::getFromOpaqueValue(opaque))
   {}

   void initialize(AstContext &c, DeclBaseName baseName,
                   ArrayRef<Identifier> argumentNames);

public:
   /// Build a null name.
   DeclName()
      : m_simpleOrCompound(BaseNameAndCompound())
   {}

   /// Build a simple value name with one component.
   /*implicit*/ DeclName(DeclBaseName simpleName)
      : m_simpleOrCompound(BaseNameAndCompound(simpleName, false))
   {}

   /*implicit*/ DeclName(Identifier simpleName)
      : DeclName(DeclBaseName(simpleName))
   {}

   /// Build a compound value name given a base name and a set of argument names.
   DeclName(AstContext &c, DeclBaseName baseName,
            ArrayRef<Identifier> argumentNames)
   {
      initialize(c, baseName, argumentNames);
   }

   /// Build a compound value name given a base name and a set of argument names
   /// extracted from a parameter list.
   DeclName(AstContext &c, DeclBaseName baseName, ParameterList *paramList);

   /// Retrieve the 'base' name, i.e., the name that follows the introducer,
   /// such as the 'foo' in 'func foo(x:Int, y:Int)' or the 'bar' in
   /// 'var bar: Int'.
   DeclBaseName getBaseName() const
   {
      if (auto compound = m_simpleOrCompound.dyn_cast<CompoundDeclName*>()) {
         return compound->baseName;
      }
      return m_simpleOrCompound.get<BaseNameAndCompound>().getPointer();
   }

   /// Assert that the base name is not special and return its identifier.
   Identifier getBaseIdentifier() const
   {
      auto baseName = getBaseName();
      assert(!baseName.isSpecial() &&
             "Can't retrieve the identifier of a special base name");
      return baseName.getIdentifier();
   }

   /// Retrieve the names of the arguments, if there are any.
   ArrayRef<Identifier> getArgumentNames() const
   {
      if (auto compound = m_simpleOrCompound.dyn_cast<CompoundDeclName*>()) {
         return compound->getArgumentNames();
      }
      return {};
   }

   bool isSpecial() const
   {
      return getBaseName().isSpecial();
   }

   explicit operator bool() const
   {
      if (m_simpleOrCompound.dyn_cast<CompoundDeclName*>())
         return true;
      return !m_simpleOrCompound.get<BaseNameAndCompound>().getPointer().empty();
   }

   /// True if this is a simple one-component name.
   bool isSimpleName() const
   {
      if (m_simpleOrCompound.dyn_cast<CompoundDeclName*>()) {
         return false;
      }
      return !m_simpleOrCompound.get<BaseNameAndCompound>().getInt();
   }

   /// True if this is a compound name.
   bool isCompoundName() const
   {
      if (m_simpleOrCompound.dyn_cast<CompoundDeclName*>()) {
         return true;
      }
      return m_simpleOrCompound.get<BaseNameAndCompound>().getInt();
   }

   /// True if this name is a simple one-component name identical to the
   /// given identifier.
   bool isSimpleName(DeclBaseName name) const
   {
      return isSimpleName() && getBaseName() == name;
   }

   /// True if this name is a simple one-component name equal to the
   /// given string.
   bool isSimpleName(StringRef name) const
   {
      return isSimpleName() && getBaseName() == name;
   }

   /// True if this name is a compound name equal to the given base name and
   /// argument names.
   bool isCompoundName(DeclBaseName base, ArrayRef<StringRef> args) const;

   /// True if this name is a compound name equal to the given normal
   /// base name and argument names.
   bool isCompoundName(StringRef base, ArrayRef<StringRef> args) const;

   /// True if this name is an operator.
   bool isOperator() const
   {
      return getBaseName().isOperator();
   }

   /// True if this name should be found by a decl ref or member ref under the
   /// name specified by 'refName'.
   ///
   /// We currently match compound names either when their first component
   /// matches a simple name lookup or when the full compound name matches.
   bool matchesRef(DeclName refName) const
   {
      // Identical names always match.
      if (m_simpleOrCompound == refName.m_simpleOrCompound) {
         return true;
      }
      // If the reference is a simple name, try simple name matching.
      if (refName.isSimpleName()) {
         return refName.getBaseName() == getBaseName();
      }
      // The names don't match.
      return false;
   }

   /// Add a DeclName to a lookup table so that it can be found by its simple
   /// name or its compound name.
   template<typename LookupTable, typename Element>
   void addToLookupTable(LookupTable &table, const Element &elt)
   {
      table[*this].push_back(elt);
      if (!isSimpleName()) {
         table[getBaseName()].push_back(elt);
      }
   }

   /// Compare two declaration names, producing -1 if \c *this comes before
   /// \c other,  1 if \c *this comes after \c other, and 0 if they are equal.
   ///
   /// Null declaration names come after all rhs declaration names.
   int compare(DeclName other) const;

   friend bool operator==(DeclName lhs, DeclName rhs)
   {
      return lhs.getOpaqueValue() == rhs.getOpaqueValue();
   }

   friend bool operator!=(DeclName lhs, DeclName rhs)
   {
      return !(lhs == rhs);
   }

   friend bool operator<(DeclName lhs, DeclName rhs)
   {
      return lhs.compare(rhs) < 0;
   }

   friend bool operator<=(DeclName lhs, DeclName rhs)
   {
      return lhs.compare(rhs) <= 0;
   }

   friend bool operator>(DeclName lhs, DeclName rhs)
   {
      return lhs.compare(rhs) > 0;
   }

   friend bool operator>=(DeclName lhs, DeclName rhs)
   {
      return lhs.compare(rhs) >= 0;
   }

   void *getOpaqueValue() const
   {
      return m_simpleOrCompound.getOpaqueValue();
   }

   static DeclName getFromOpaqueValue(void *p)
   {
      return DeclName(p);
   }

   /// Get a string representation of the name,
   ///
   /// \param scratch Scratch space to use.
   StringRef getString(llvm::SmallVectorImpl<char> &scratch,
                       bool skipEmptyArgumentNames = false) const;

   /// Print the representation of this declaration name to the given
   /// stream.
   ///
   /// \param skipEmptyArgumentNames When true, don't print the argument labels
   /// if they are all empty.
   llvm::raw_ostream &print(llvm::raw_ostream &os,
                            bool skipEmptyArgumentNames = false) const;

   /// Print a "pretty" representation of this declaration name to the given
   /// stream.
   ///
   /// This is the name used for diagnostics; it is not necessarily the
   /// fully-specified name that would be written in the source.
   llvm::raw_ostream &printPretty(llvm::raw_ostream &os) const;

   /// Dump this name to standard error.
   LLVM_ATTRIBUTE_DEPRECATED(void dump() const,
                             "only for use within the debugger");
};

} // end namespace polas::ast

namespace llvm {
// A DeclName is "pointer like".
template<typename T>
struct PointerLikeTypeTraits;
template<>
struct PointerLikeTypeTraits<polar::ast::DeclName>
{
public:
   static inline void *getAsVoidm_pointer(polar::ast::DeclName name)
   {
      return name.getOpaqueValue();
   }

   static inline polar::ast::DeclName getFromVoidPointer(void *ptr)
   {
      return polar::ast::DeclName::getFromOpaqueValue(ptr);
   }

   enum { NumLowBitsAvailable = 0 };
};

// DeclNames hash just like pointers.
template<>
struct DenseMapInfo<polar::ast::DeclName>
{
   static polar::ast::DeclName getEmptyKey()
   {
      return polar::ast::Identifier::getEmptyKey();
   }

   static polar::ast::DeclName getTombstoneKey()
   {
      return polar::ast::Identifier::getTombstoneKey();
   }

   static unsigned getHashValue(polar::ast::DeclName value)
   {
      return DenseMapInfo<void*>::getHashValue(value.getOpaqueValue());
   }

   static bool isEqual(polar::ast::DeclName lhs, polar::ast::DeclName rhs)
   {
      return lhs.getOpaqueValue() == rhs.getOpaqueValue();
   }
};

} // end namespace llvm

#endif // POLARPHP_AST_IDENTIFIER_H
