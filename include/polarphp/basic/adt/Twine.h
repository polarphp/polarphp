// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/27.

#ifndef POLARPHP_BASIC_ADT_TWINE_H
#define POLARPHP_BASIC_ADT_TWINE_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/ErrorHandling.h"
#include <cassert>
#include <cstdint>
#include <string>

namespace polar {

// forward declare class with namespace
namespace utils {
class FormatvObjectBase;
class RawOutStream;
} // utils

namespace basic {

using polar::utils::FormatvObjectBase;
using polar::utils::RawOutStream;

/// Twine - A lightweight data structure for efficiently representing the
/// concatenation of temporary values as strings.
///
/// A Twine is a kind of rope, it represents a concatenated string using a
/// binary-tree, where the string is the preorder of the nodes. Since the
/// Twine can be efficiently rendered into a buffer when its result is used,
/// it avoids the cost of generating temporary values for intermediate string
/// results -- particularly in cases when the Twine result is never
/// required. By explicitly tracking the type of leaf nodes, we can also avoid
/// the creation of temporary strings for conversions operations (such as
/// appending an integer to a string).
///
/// A Twine is not intended for use directly and should not be stored, its
/// implementation relies on the ability to store pointers to temporary stack
/// objects which may be deallocated at the end of a statement. Twines should
/// only be used accepted as const references in arguments, when an API wishes
/// to accept possibly-concatenated strings.
///
/// Twines support a special 'null' value, which always concatenates to form
/// itself, and renders as an empty string. This can be returned from APIs to
/// effectively nullify any concatenations performed on the result.
///
/// \b Implementation
///
/// Given the nature of a Twine, it is not possible for the Twine's
/// concatenation method to construct interior nodes; the result must be
/// represented inside the returned value. For this reason a Twine object
/// actually holds two values, the left- and right-hand sides of a
/// concatenation. We also have nullary Twine objects, which are effectively
/// sentinel values that represent empty strings.
///
/// Thus, a Twine can effectively have zero, one, or two children. The \see
/// isNullary(), \see isUnary(), and \see isBinary() predicates exist for
/// testing the number of children.
///
/// We maintain a number of invariants on Twine objects (FIXME: Why):
///  - Nullary twines are always represented with their Kind on the left-hand
///    side, and the Empty kind on the right-hand side.
///  - Unary twines are always represented with the value on the left-hand
///    side, and the Empty kind on the right-hand side.
///  - If a Twine has another Twine as a child, that child should always be
///    binary (otherwise it could have been folded into the parent).
///
/// These invariants are check by \see isValid().
///
/// \b Efficiency Considerations
///
/// The Twine is designed to yield efficient and small code for common
/// situations. For this reason, the concat() method is inlined so that
/// concatenations of leaf nodes can be optimized into stores directly into a
/// single stack allocated object.
///
/// In practice, not all compilers can be trusted to optimize concat() fully,
/// so we provide two additional methods (and accompanying operator+
/// overloads) to guarantee that particularly important cases (cstring plus
/// StringRef) codegen as desired.
class Twine
{
   /// NodeKind - Represent the type of an argument.
   enum class NodeKind : unsigned char
   {
      /// An empty string; the result of concatenating anything with it is also
      /// empty.
      NullKind,

      /// The empty string.
      EmptyKind,

      /// A pointer to a Twine instance.
      TwineKind,

      /// A pointer to a C string instance.
      CStringKind,

      /// A pointer to an std::string instance.
      StdStringKind,

      /// A pointer to a StringRef instance.
      StringRefKind,

      /// A pointer to a SmallString instance.
      SmallStringKind,

      /// A pointer to a FormatvObjectBase instance.
      FormatvObjectKind,

      /// A char value, to render as a character.
      CharKind,

      /// An unsigned int value, to render as an unsigned decimal integer.
      DecUIKind,

      /// An int value, to render as a signed decimal integer.
      DecIKind,

      /// A pointer to an unsigned long value, to render as an unsigned decimal
      /// integer.
      DecULKind,

      /// A pointer to a long value, to render as a signed decimal integer.
      DecLKind,

      /// A pointer to an unsigned long long value, to render as an unsigned
      /// decimal integer.
      DecULLKind,

      /// A pointer to a long long value, to render as a signed decimal integer.
      DecLLKind,

      /// A pointer to a uint64_t value, to render as an unsigned hexadecimal
      /// integer.
      UHexKind
   };

   union Child
   {
      const Twine *m_twine;
      const char *m_CString;
      const std::string *m_stdString;
      const StringRef *m_stringRef;
      const SmallVectorImpl<char> *m_smallString;
      const FormatvObjectBase *m_formatvObject;
      char m_character;
      unsigned int m_decUInt;
      int m_decInt;
      const unsigned long *m_decULong;
      const long *m_decLong;
      const unsigned long long *m_decULongLong;
      const long long *m_decLongLong;
      const uint64_t *m_uHex;
   };

   /// LHS - The prefix in the concatenation, which may be uninitialized for
   /// Null or Empty kinds.
   Child m_lhs;

   /// RHS - The suffix in the concatenation, which may be uninitialized for
   /// Null or Empty kinds.
   Child m_rhs;

   /// LHSKind - The NodeKind of the left hand side, \see getLhsKind().
   NodeKind m_lhsKind = NodeKind::EmptyKind;

   /// RHSKind - The NodeKind of the right hand side, \see getRhsKind().
   NodeKind m_rhsKind = NodeKind::EmptyKind;

   /// Construct a nullary twine; the kind must be NullKind or EmptyKind.
   explicit Twine(NodeKind kind) : m_lhsKind(kind)
   {
      assert(isNullary() && "Invalid kind!");
   }

   /// Construct a binary twine.
   explicit Twine(const Twine &lhs, const Twine &rhs)
      : m_lhsKind(NodeKind::TwineKind), m_rhsKind(NodeKind::TwineKind)
   {
      m_lhs.m_twine = &lhs;
      m_rhs.m_twine = &rhs;
      assert(isValid() && "Invalid twine!");
   }

   /// Construct a twine from explicit values.
   explicit Twine(Child lhs, NodeKind lhsKind, Child rhs, NodeKind rhsKind)
      : m_lhs(lhs), m_rhs(rhs), m_lhsKind(lhsKind), m_rhsKind(rhsKind)
   {
      assert(isValid() && "Invalid twine!");
   }

   /// Check for the null twine.
   bool isNull() const
   {
      return getLhsKind() == NodeKind::NullKind;
   }

   /// Check for the empty twine.
   bool isEmpty() const
   {
      return getLhsKind() == NodeKind::EmptyKind;
   }

   /// Check if this is a nullary twine (null or empty).
   bool isNullary() const
   {
      return isNull() || isEmpty();
   }

   /// Check if this is a unary twine.
   bool isUnary() const
   {
      return getRhsKind() == NodeKind::EmptyKind && !isNullary();
   }

   /// Check if this is a binary twine.
   bool isBinary() const
   {
      return getLhsKind() != NodeKind::NullKind && getRhsKind() != NodeKind::EmptyKind;
   }

   /// Check if this is a valid twine (satisfying the invariants on
   /// order and number of arguments).
   bool isValid() const
   {
      // Nullary twines always have Empty on the rhs.
      if (isNullary() && getRhsKind() != NodeKind::EmptyKind) {
         return false;
      }

      // Null should never appear on the RHS.
      if (getRhsKind() == NodeKind::NullKind) {
         return false;
      }

      // The RHS cannot be non-empty if the LHS is empty.
      if (getRhsKind() != NodeKind::EmptyKind && getLhsKind() == NodeKind::EmptyKind) {
         return false;
      }
      // A twine child should always be binary.
      if (getLhsKind() == NodeKind::TwineKind &&
          !m_lhs.m_twine->isBinary()) {
         return false;
      }

      if (getRhsKind() == NodeKind::TwineKind &&
          !m_rhs.m_twine->isBinary()) {
         return false;
      }
      return true;
   }

   /// Get the NodeKind of the left-hand side.
   NodeKind getLhsKind() const
   {
      return m_lhsKind;
   }

   /// Get the NodeKind of the right-hand side.
   NodeKind getRhsKind() const
   {
      return m_rhsKind;
   }

   /// Print one child from a twine.
   void printOneChild(RawOutStream &outStream, Child ptr, NodeKind kind) const;

   /// Print the representation of one child from a twine.
   void printOneChildRepr(RawOutStream &outStream, Child ptr,
                          NodeKind kind) const;

public:
   /// @name Constructors
   /// @{

   /// Construct from an empty string.
   /*implicit*/ Twine()
   {
      assert(isValid() && "Invalid twine!");
   }

   Twine(const Twine &) = default;

   /// Construct from a C string.
   ///
   /// We take care here to optimize "" into the empty twine -- this will be
   /// optimized out for string constants. This allows Twine arguments have
   /// default "" values, without introducing unnecessary string constants.
   /*implicit*/ Twine(const char *str)
   {
      if (str[0] != '\0') {
         m_lhs.m_CString = str;
         m_lhsKind = NodeKind::CStringKind;
      } else {
         m_lhsKind = NodeKind::EmptyKind;
      }
      assert(isValid() && "Invalid twine!");
   }

   /// Construct from an std::string.
   /*implicit*/ Twine(const std::string &str) : m_lhsKind(NodeKind::StdStringKind)
   {
      m_lhs.m_stdString = &str;
      assert(isValid() && "Invalid twine!");
   }

   /// Construct from a StringRef.
   /*implicit*/ Twine(const StringRef &str) : m_lhsKind(NodeKind::StringRefKind)
   {
      m_lhs.m_stringRef = &str;
      assert(isValid() && "Invalid twine!");
   }

   /// Construct from a SmallString.
   /*implicit*/ Twine(const SmallVectorImpl<char> &str)
      : m_lhsKind(NodeKind::SmallStringKind)
   {
      m_lhs.m_smallString = &str;
      assert(isValid() && "Invalid twine!");
   }

   /// Construct from a FormatvObjectBase.
   /*implicit*/ Twine(const FormatvObjectBase &fmt)
      : m_lhsKind(NodeKind::FormatvObjectKind)
   {
      m_lhs.m_formatvObject = &fmt;
      assert(isValid() && "Invalid twine!");
   }

   /// Construct from a char.
   explicit Twine(char value) : m_lhsKind(NodeKind::CharKind)
   {
      m_lhs.m_character = value;
   }

   /// Construct from a signed char.
   explicit Twine(signed char value) : m_lhsKind(NodeKind::CharKind)
   {
      m_lhs.m_character = static_cast<char>(value);
   }

   /// Construct from an unsigned char.
   explicit Twine(unsigned char value) : m_lhsKind(NodeKind::CharKind)
   {
      m_lhs.m_character = static_cast<char>(value);
   }

   /// Construct a twine to print \p Val as an unsigned decimal integer.
   explicit Twine(unsigned value) : m_lhsKind(NodeKind::DecUIKind)
   {
      m_lhs.m_decUInt = value;
   }

   /// Construct a twine to print \p Val as a signed decimal integer.
   explicit Twine(int value) : m_lhsKind(NodeKind::DecIKind)
   {
      m_lhs.m_decInt = value;
   }

   /// Construct a twine to print \p Val as an unsigned decimal integer.
   explicit Twine(const unsigned long &value) : m_lhsKind(NodeKind::DecULKind)
   {
      m_lhs.m_decULong = &value;
   }

   /// Construct a twine to print \p Val as a signed decimal integer.
   explicit Twine(const long &value) : m_lhsKind(NodeKind::DecLKind)
   {
      m_lhs.m_decLong = &value;
   }

   /// Construct a twine to print \p Val as an unsigned decimal integer.
   explicit Twine(const unsigned long long &value) : m_lhsKind(NodeKind::DecULLKind)
   {
      m_lhs.m_decULongLong = &value;
   }

   /// Construct a twine to print \p Val as a signed decimal integer.
   explicit Twine(const long long &value) : m_lhsKind(NodeKind::DecLLKind)
   {
      m_lhs.m_decLongLong = &value;
   }

   // FIXME: Unfortunately, to make sure this is as efficient as possible we
   // need extra binary constructors from particular types. We can't rely on
   // the compiler to be smart enough to fold operator+()/concat() down to the
   // right thing. Yet.

   /// Construct as the concatenation of a C string and a StringRef.
   /*implicit*/ Twine(const char *lhs, const StringRef &rhs)
      : m_lhsKind(NodeKind::CStringKind), m_rhsKind(NodeKind::StringRefKind)
   {
      m_lhs.m_CString = lhs;
      m_rhs.m_stringRef = &rhs;
      assert(isValid() && "Invalid twine!");
   }

   /// Construct as the concatenation of a StringRef and a C string.
   /*implicit*/ Twine(const StringRef &lhs, const char *rhs)
      : m_lhsKind(NodeKind::StringRefKind), m_rhsKind(NodeKind::CStringKind)
   {
      m_lhs.m_stringRef = &lhs;
      m_rhs.m_CString = rhs;
      assert(isValid() && "Invalid twine!");
   }

   /// Since the intended use of twines is as temporary objects, assignments
   /// when concatenating might cause undefined behavior or stack corruptions
   Twine &operator=(const Twine &) = delete;

   /// Create a 'null' string, which is an empty string that always
   /// concatenates to form another empty string.
   static Twine createNull()
   {
      return Twine(NodeKind::NullKind);
   }

   /// @}
   /// @name Numeric Conversions
   /// @{

   // Construct a twine to print \p Val as an unsigned hexadecimal integer.
   static Twine utohexstr(const uint64_t &value)
   {
      Child lhs, rhs;
      lhs.m_uHex = &value;
      rhs.m_twine = nullptr;
      return Twine(lhs, NodeKind::UHexKind, rhs, NodeKind::EmptyKind);
   }

   /// @}
   /// @name Predicate Operations
   /// @{

   /// Check if this twine is trivially empty; a false return value does not
   /// necessarily mean the twine is empty.
   bool isTriviallyEmpty() const
   {
      return isNullary();
   }

   /// Return true if this twine can be dynamically accessed as a single
   /// StringRef value with getSingleStringRef().
   bool isSingleStringRef() const
   {
      if (getRhsKind() != NodeKind::EmptyKind) {
         return false;
      }

      switch (getLhsKind())
      {
      case NodeKind::EmptyKind:
      case NodeKind::CStringKind:
      case NodeKind::StdStringKind:
      case NodeKind::StringRefKind:
      case NodeKind::SmallStringKind:
         return true;
      default:
         return false;
      }
   }

   /// @}
   /// @name String Operations
   /// @{

   Twine concat(const Twine &suffix) const;

   /// @}
   /// @name Output & Conversion.
   /// @{

   /// Return the twine contents as a std::string.
   std::string getStr() const;

   /// Append the concatenated string into the given SmallString or SmallVector.
   void toVector(SmallVectorImpl<char> &out) const;

   /// This returns the twine as a single StringRef.  This method is only valid
   /// if isSingleStringRef() is true.
   StringRef getSingleStringRef() const
   {
      assert(isSingleStringRef() &&"This cannot be had as a single stringref!");
      switch (getLhsKind()) {
      default: polar_unreachable("Out of sync with isSingleStringRef");
      case NodeKind::EmptyKind:      return StringRef();
      case NodeKind::CStringKind:    return StringRef(m_lhs.m_CString);
      case NodeKind::StdStringKind:  return StringRef(*m_lhs.m_stdString);
      case NodeKind::StringRefKind:  return *m_lhs.m_stringRef;
      case NodeKind::SmallStringKind:
         return StringRef(m_lhs.m_smallString->getData(), m_lhs.m_smallString->getSize());
      }
   }

   /// This returns the twine as a single StringRef if it can be
   /// represented as such. Otherwise the twine is written into the given
   /// SmallVector and a StringRef to the SmallVector's data is returned.
   StringRef toStringRef(SmallVectorImpl<char> &out) const
   {
      if (isSingleStringRef()) {
         return getSingleStringRef();
      }
      toVector(out);
      return StringRef(out.getData(), out.getSize());
   }

   /// This returns the twine as a single null terminated StringRef if it
   /// can be represented as such. Otherwise the twine is written into the
   /// given SmallVector and a StringRef to the SmallVector's data is returned.
   ///
   /// The returned StringRef's size does not include the null terminator.
   StringRef toNullTerminatedStringRef(SmallVectorImpl<char> &out) const;

   /// Write the concatenated string represented by this twine to the
   /// stream \p OS.
   void print(RawOutStream &outStream) const;

   /// Dump the concatenated string represented by this twine to stderr.
   void dump() const;

   /// Write the representation of this twine to the stream \p OS.
   void printRepr(RawOutStream &outStream) const;

   /// Dump the representation of this twine to stderr.
   void dumpRepr() const;

   /// @}
};

/// @name Twine Inline Implementations
/// @{

inline Twine Twine::concat(const Twine &suffix) const
{
   // Concatenation with null is null.
   if (isNull() || suffix.isNull()) {
      return Twine(NodeKind::NullKind);
   }
   // Concatenation with empty yields the other side.
   if (isEmpty()){
      return suffix;
   }
   if (suffix.isEmpty()) {
      return *this;
   }
   // Otherwise we need to create a new node, taking care to fold in unary
   // twines.
   Child newLHS, newRHS;
   newLHS.m_twine = this;
   newRHS.m_twine = &suffix;
   NodeKind newLHSKind = NodeKind::TwineKind;
   NodeKind newRHSKind = NodeKind::TwineKind;
   if (isUnary()) {
      newLHS = m_lhs;
      newLHSKind = getLhsKind();
   }
   if (suffix.isUnary()) {
      newRHS = suffix.m_lhs;
      newRHSKind = suffix.getLhsKind();
   }

   return Twine(newLHS, newLHSKind, newRHS, newRHSKind);
}

inline Twine operator+(const Twine &lhs, const Twine &rhs)
{
   return lhs.concat(rhs);
}

/// Additional overload to guarantee simplified codegen; this is equivalent to
/// concat().

inline Twine operator+(const char *lhs, const StringRef &rhs)
{
   return Twine(lhs, rhs);
}

/// Additional overload to guarantee simplified codegen; this is equivalent to
/// concat().

inline Twine operator+(const StringRef &lhs, const char *rhs)
{
   return Twine(lhs, rhs);
}

inline RawOutStream &operator<<(RawOutStream &outStream, const Twine &rhs)
{
   rhs.print(outStream);
   return outStream;
}

/// @}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_TWINE_H
