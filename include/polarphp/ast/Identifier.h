//===--- Identifier.cpp - Uniqued Identifier ------------------------------===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Identifier interface.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_IDENTIFIER_H
#define POLARPHP_AST_IDENTIFIER_H

#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/utils/EditorPlaceholder.h"
#include "polarphp/utils/TrailingObjects.h"

/// forward declare class with namespace
namespace polar::utils {
class RawOutStream;
}

namespace polar::ast {

using polar::utils::RawOutStream;
using polar::basic::DenseMapInfo;
using polar::basic::StringRef;

/// forward declare class
class AstContext;
class ParameterList;
class DeclName;

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
   explicit Identifier()
      : m_pointer(nullptr)
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
      if ((unsigned char)m_pointer[0] < 0x80) {
         return isOperatorStartCodePoint((unsigned char)m_pointer[0]);
      }

      // Handle the high unicode case out of line.
      return isOperatorSlow();
   }

   /// isOperatorStartCodePoint - Return true if the specified code point is a
   /// valid start of an operator.
   static bool isOperatorStartCodePoint(uint32_t codePoint)
   {
      // ASCII operator chars.
      static const char opChars[] = "/=-+*%<>!&|^~.?";
      if (codePoint < 0x80) {
         return memchr(opChars, codePoint, sizeof(opChars) - 1) != 0;
      }
      // Unicode math, symbol, arrow, dingbat, and line/box drawing chars.
      return (codePoint >= 0x00A1 && codePoint <= 0x00A7)
            || codePoint == 0x00A9 || codePoint == 0x00AB || codePoint == 0x00AC || codePoint == 0x00AE
            || codePoint == 0x00B0 || codePoint == 0x00B1 || codePoint == 0x00B6 || codePoint == 0x00BB
            || codePoint == 0x00BF || codePoint == 0x00D7 || codePoint == 0x00F7
            || codePoint == 0x2016 || codePoint == 0x2017 || (codePoint >= 0x2020 && codePoint <= 0x2027)
            || (codePoint >= 0x2030 && codePoint <= 0x203E) || (codePoint >= 0x2041 && codePoint <= 0x2053)
            || (codePoint >= 0x2055 && codePoint <= 0x205E) || (codePoint >= 0x2190 && codePoint <= 0x23FF)
            || (codePoint >= 0x2500 && codePoint <= 0x2775) || (codePoint >= 0x2794 && codePoint <= 0x2BFF)
            || (codePoint >= 0x2E00 && codePoint <= 0x2E7F) || (codePoint >= 0x3001 && codePoint <= 0x3003)
            || (codePoint >= 0x3008 && codePoint <= 0x3030);
   }

   /// isOperatorContinuationCodePoint - Return true if the specified code point
   /// is a valid operator code point.
   static bool isOperatorContinuationCodePoint(uint32_t codePoint)
   {
      if (isOperatorStartCodePoint(codePoint)) {
         return true;
      }

      // Unicode combining characters and variation selectors.
      return (codePoint >= 0x0300 && codePoint <= 0x036F)
            || (codePoint >= 0x1DC0 && codePoint <= 0x1DFF)
            || (codePoint >= 0x20D0 && codePoint <= 0x20FF)
            || (codePoint >= 0xFE00 && codePoint <= 0xFE0F)
            || (codePoint >= 0xFE20 && codePoint <= 0xFE2F)
            || (codePoint >= 0xE0100 && codePoint <= 0xE01EF);
   }

   static bool isEditorPlaceholder(StringRef name)
   {
      return polar::utils::is_editor_placeholder(name);
   }

   bool isEditorPlaceholder() const
   {
      return !empty() && isEditorPlaceholder(str());
   }

   const void *getAsOpaquePointer() const
   {
      return static_cast<const void *>(m_pointer);
   }

   static Identifier getFromOpaquePointer(void *ptr)
   {
      return Identifier((const char*)ptr);
   }

   /// Compare two identifiers, producing -1 if \c *this comes before \c other,
   /// 1 if \c *this comes after \c other, and 0 if they are equal.
   ///
   /// Null identifiers come after all other identifiers.
   int compare(Identifier other) const;

   bool operator==(Identifier other) const
   {
      return m_pointer == other.m_pointer;
   }

   bool operator!=(Identifier other) const
   {
      return !(*this == other);
   }

   bool operator<(Identifier other) const
   {
      return m_pointer < other.m_pointer;
   }

   static Identifier getEmptyKey()
   {
      return Identifier((const char*)
                        DenseMapInfo<const void*>::getEmptyKey());
   }

   static Identifier getTombstoneKey()
   {
      return Identifier((const char*)
                        DenseMapInfo<const void*>::getTombstoneKey());
   }

private:
   bool isOperatorSlow() const;
};

} // polar::ast

namespace polar {

using polar::utils::RawOutStream;
using polar::ast::Identifier;
using polar::ast::DeclName;

namespace basic {

RawOutStream &operator<<(RawOutStream &outStream, Identifier identifier);
RawOutStream &operator<<(RawOutStream &outStream, DeclName declName);

// Identifiers hash just like pointers.
template<>
struct DenseMapInfo<Identifier>
{
   static Identifier getEmptyKey()
   {
      return Identifier::getEmptyKey();
   }
   static Identifier getTombstoneKey()
   {
      return Identifier::getTombstoneKey();
   }

   static unsigned getHashValue(Identifier value)
   {
      return DenseMapInfo<const void*>::getHashValue(value.get());
   }

   static bool isEqual(Identifier lhs, Identifier rhs)
   {
      return lhs == rhs;
   }
};
} // basic

namespace utils {
// An Identifier is "pointer like".
template<typename T>
struct PointerLikeTypeTraits;

template<>
struct PointerLikeTypeTraits<Identifier>
{
public:
   static inline void *getAsVoidPointer(Identifier identifier)
   {
      return const_cast<void *>(identifier.getAsOpaquePointer());
   }

   static inline Identifier getFromVoidPointer(void *ptr)
   {
      return Identifier::getFromOpaquePointer(ptr);
   }

   enum { NumLowBitsAvailable = 2 };
};
} // utils
} // polar

namespace polar::ast {

/// Wrapper that may either be an Identifier or a special name
/// (e.g. for subscripts)
class DeclBaseName
{
public:
   enum class Kind: uint8_t {
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
   static void *m_subscriptIdentifierData;
   /// As above, for special constructor DeclNames.
   static void *m_constructorIdentifierData;
   /// As above, for special destructor DeclNames.
   static void *m_destructorIdentifierData;

   Identifier m_identifier;

public:
   DeclBaseName()
      : DeclBaseName(Identifier())
   {}

   DeclBaseName(Identifier identifier)
      : m_identifier(identifier)
   {}

   static DeclBaseName createSubscript()
   {
      return DeclBaseName(Identifier((const char *)m_subscriptIdentifierData));
   }

   static DeclBaseName createConstructor()
   {
      return DeclBaseName(Identifier((const char *)m_constructorIdentifierData));
   }

   static DeclBaseName createDestructor()
   {
      return DeclBaseName(Identifier((const char *)m_destructorIdentifierData));
   }

   Kind getKind() const
   {
      if (m_identifier.get() == m_subscriptIdentifierData) {
         return Kind::Subscript;
      } else if (m_identifier.get() == m_constructorIdentifierData) {
         return Kind::Constructor;
      } else if (m_identifier.get() == m_destructorIdentifierData) {
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
      return m_identifier;
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
      polar_unreachable("unhandled kind");
   }

   int compare(DeclBaseName other) const
   {
      return userFacingName().compare(other.userFacingName());
   }

   bool operator==(StringRef str) const
   {
      return !isSpecial() && getIdentifier().is(str);
   }
   bool operator!=(StringRef str) const
   {
      return !(*this == str); }

   bool operator==(DeclBaseName other) const
   {
      return m_identifier == other.m_identifier;
   }

   bool operator!=(DeclBaseName other) const
   {
      return !(*this == other);
   }

   bool operator<(DeclBaseName other) const
   {
      return m_identifier.get() < other.m_identifier.get();
   }

   const void *getAsOpaquePointer() const
   {
      return m_identifier.get();
   }

   static DeclBaseName getFromOpaquePointer(void *ptr)
   {
      return Identifier::getFromOpaquePointer(ptr);
   }
};

} // polar::ast

namespace polar {

using polar::ast::DeclBaseName;

namespace basic {
RawOutStream &operator<<(RawOutStream &outStream, DeclBaseName declBaseName);

// DeclBaseNames hash just like pointers.
template<>
struct DenseMapInfo<DeclBaseName>
{
   static DeclBaseName getEmptyKey()
   {
      return Identifier::getEmptyKey();
   }

   static DeclBaseName getTombstoneKey()
   {
      return Identifier::getTombstoneKey();
   }

   static unsigned getHashValue(DeclBaseName value)
   {
      return DenseMapInfo<const void *>::getHashValue(value.getAsOpaquePointer());
   }

   static bool isEqual(DeclBaseName lhs, DeclBaseName rhs)
   {
      return lhs == rhs;
   }
};
} // basic

namespace utils {
// A DeclBaseName is "pointer like".
template <typename T>
struct PointerLikeTypeTraits;

template <>
struct PointerLikeTypeTraits<DeclBaseName>
{
public:
   static inline void *getAsVoidPointer(DeclBaseName declBaseName)
   {
      return const_cast<void *>(declBaseName.getAsOpaquePointer());
   }

   static inline DeclBaseName getFromVoidPointer(void *P)
   {
      return DeclBaseName::getFromOpaquePointer(P);
   }

   enum {
      NumLowBitsAvailable = PointerLikeTypeTraits<Identifier>::NumLowBitsAvailable
   };
};
} // utils

} // polar

namespace polar::ast {

#define ALIGNAS_IDENTIFIFER alignas(Identifier)

using polar::basic::FoldingSetNode;
using polar::basic::PointerUnion;
using polar::basic::ArrayRef;
using polar::basic::MutableArrayRef;
using polar::basic::SmallVectorImpl;
using polar::basic::FoldingSetNodeId;
using polar::utils::PointerIntPair;
using polar::utils::TrailingObjects;

/// A declaration name, which may comprise one or more identifier pieces.
class DeclName
{
   friend class AstContext;

   /// Represents a compound declaration name.
   struct ALIGNAS_IDENTIFIFER CompoundDeclName final
         : FoldingSetNode,
         private TrailingObjects<CompoundDeclName, Identifier>
   {
      friend class TrailingObjects;
      friend class DeclName;

      DeclBaseName m_baseName;
      size_t m_numArgs;

      explicit CompoundDeclName(DeclBaseName baseName, size_t numArgs)
         : m_baseName(baseName),
           m_numArgs(numArgs)
      {
         assert(m_numArgs > 0 && "Should use IdentifierAndCompound");
      }

      ArrayRef<Identifier> getArgumentNames() const
      {
         return {getTrailingObjects<Identifier>(), m_numArgs};
      }

      MutableArrayRef<Identifier> getArgumentNames()
      {
         return {getTrailingObjects<Identifier>(), m_numArgs};
      }

      /// Uniquing for the AstContext.
      static void getProfile(FoldingSetNodeId &id, DeclBaseName baseName,
                             ArrayRef<Identifier> argumentNames);

      void getProfile(FoldingSetNodeId &id)
      {
         getProfile(id, m_baseName, getArgumentNames());
      }
   };

   // A single stored identifier, along with a bit stating whether it is the
   // base name for a zero-argument compound name.
   using BaseNameAndCompound = PointerIntPair<DeclBaseName, 1, bool>;
   using SimpleOrCompoundType = PointerUnion<BaseNameAndCompound, CompoundDeclName *>;

   // Either a single identifier piece stored inline (with a bit to say whether
   // it is simple or compound), or a reference to a compound declaration name.
   PointerUnion<BaseNameAndCompound, CompoundDeclName *> m_simpleOrCompound;

   DeclName(void *Opaque)
      : m_simpleOrCompound(SimpleOrCompoundType::getFromOpaqueValue(Opaque))
   {}

   void initialize(AstContext &context, DeclBaseName baseName,
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
   DeclName(AstContext &context, DeclBaseName baseName,
            ArrayRef<Identifier> argumentNames)
   {
      initialize(context, baseName, argumentNames);
   }

   /// Build a compound value name given a base name and a set of argument names
   /// extracted from a parameter list.
   DeclName(AstContext &context, DeclBaseName baseName, ParameterList *paramList);

   /// Retrieve the 'base' name, i.e., the name that follows the introducer,
   /// such as the 'foo' in 'func foo(x:Int, y:Int)' or the 'bar' in
   /// 'var bar: Int'.
   DeclBaseName getBaseName() const
   {
      if (auto compound = m_simpleOrCompound.dynamicCast<CompoundDeclName*>())
         return compound->m_baseName;

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
      if (auto compound = m_simpleOrCompound.dynamicCast<CompoundDeclName*>()) {
         return compound->getArgumentNames();
      }
      return { };
   }

   bool isSpecial() const
   {
      return getBaseName().isSpecial();
   }

   explicit operator bool() const
   {
      if (m_simpleOrCompound.dynamicCast<CompoundDeclName*>()) {
         return true;
      }
      return !m_simpleOrCompound.get<BaseNameAndCompound>().getPointer().empty();
   }

   /// True if this is a simple one-component name.
   bool isSimpleName() const
   {
      if (m_simpleOrCompound.dynamicCast<CompoundDeclName*>()) {
         return false;
      }
      return !m_simpleOrCompound.get<BaseNameAndCompound>().getInt();
   }

   /// True if this is a compound name.
   bool isCompoundName() const
   {
      if (m_simpleOrCompound.dynamicCast<CompoundDeclName*>()) {
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
   /// Null declaration names come after all other declaration names.
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
   StringRef getString(SmallVectorImpl<char> &scratch,
                       bool skipEmptyArgumentNames = false) const;

   /// Print the representation of this declaration name to the given
   /// stream.
   ///
   /// \param skipEmptyArgumentNames When true, don't print the argument labels
   /// if they are all empty.
   RawOutStream &print(RawOutStream &os,
                       bool skipEmptyArgumentNames = false) const;

   /// Print a "pretty" representation of this declaration name to the given
   /// stream.
   ///
   /// This is the name used for diagnostics; it is not necessarily the
   /// fully-specified name that would be written in the source.
   RawOutStream &printPretty(RawOutStream &os) const;

   /// Dump this name to standard error.
   POLAR_ATTRIBUTE_DEPRECATED(void dump() const,
                              "only for use within the debugger");
};

#undef ALIGNAS_IDENTIFIFER

} // polar::ast

namespace polar {

using polar::ast::DeclName;
using polar::ast::Identifier;

namespace utils {
// A DeclName is "pointer like".
template<typename T>
struct PointerLikeTypeTraits;

template<>
struct PointerLikeTypeTraits<DeclName>
{
public:
   static inline void *getAsVoidPointer(DeclName name)
   {
      return name.getOpaqueValue();
   }

   static inline DeclName getFromVoidPointer(void *ptr)
   {
      return DeclName::getFromOpaqueValue(ptr);
   }
   enum { NumLowBitsAvailable = 0 };
};
} // utils

namespace basic {
// DeclNames hash just like pointers.
template<>
struct DenseMapInfo<DeclName>
{
   static DeclName getEmptyKey()
   {
      return Identifier::getEmptyKey();
   }

   static DeclName getTombstoneKey()
   {
      return Identifier::getTombstoneKey();
   }

   static unsigned getHashValue(DeclName value)
   {
      return DenseMapInfo<void*>::getHashValue(value.getOpaqueValue());
   }

   static bool isEqual(DeclName lhs, DeclName rhs)
   {
      return lhs.getOpaqueValue() == rhs.getOpaqueValue();
   }
};
} // basic
} // polar

#endif // POLARPHP_AST_IDENTIFIER_H
