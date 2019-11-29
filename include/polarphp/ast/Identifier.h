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
   explicit Identifier()
      : m_pointer(nullptr) {}

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
      static const char opChars[] = "/=-+*%<>!&|^~.?";
      if (c < 0x80) {
         return memchr(opChars, c, sizeof(opChars) - 1) != nullptr;
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

   const void *getAsOpaquePointer() const
   {
      return reinterpret_cast<const void *>(m_pointer);
   }

   static Identifier getFromOpaquePointer(void *pointer)
   {
      return Identifier(reinterpret_cast<const char*>(pointer));
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

} // end namespace swift

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
struct PointerLikeTypeTraits<polar::ast::Identifier> {
public:
   static inline void *getAsVoidPointer(polar::ast::Identifier identifier)
   {
      return const_cast<void *>(identifier.getAsOpaquePointer());
   }

   static inline polar::ast::Identifier getFromVoidPointer(void *pointer)
   {
      return polar::ast::Identifier::getFromOpaquePointer(pointer);
   }
   enum { NumLowBitsAvailable = 2 };
};

} // polar::ast

#endif // POLARPHP_AST_IDENTIFIER_H
