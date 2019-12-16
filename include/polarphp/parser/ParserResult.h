//===--- ParserResult.h - Parser result Wrapper -----------------*- C++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/15.

#ifndef POLARPHP_PARSER_PARSER_RESULT_H
#define POLARPHP_PARSER_PARSER_RESULT_H

#include "llvm/ADTPointerIntPair.h"

namespace polar::parser {

using polar::PointerIntPair;

class ParserStatus;

/// A wrapper for a parser AST node result (Decl, Stmt, Expr, Pattern,
/// etc.)
///
/// Contains the pointer to the AST node itself (or null) and additional bits
/// that indicate:
/// \li if there was a parse error;
/// \li if there was a code completion token.
///
/// If you want to return an AST node pointer in the Parser, consider using
/// ParserResult instead.
template <typename T>
class ParserResult
{
   enum {
      m_isError = 0x1,
      m_isCodeCompletion = 0x2,
   };

public:
   /// Construct a null result with error bit set.
   ParserResult(std::nullptr_t = nullptr)
   {
      setIsParseError();
   }

   /// Construct a null result with specified error bits set.
   ParserResult(ParserStatus status);

   /// Construct a successful parser result.
   explicit ParserResult(T *result)
      : m_ptrAndBits(result)
   {
      assert(result && "a successful parser result cannot be null");
   }

   /// Convert from a different but compatible parser result.
   template <typename U, typename Enabler = typename std::enable_if<
                std::is_base_of<T, U>::value>::type>
   ParserResult(ParserResult<U> other)
      : m_ptrAndBits(other.m_ptrAndBits.getPointer(),
                     other.m_ptrAndBits.getInt())
   {}

   /// Return true if this result does not have an AST node.
   ///
   /// If returns true, then error bit is set.
   bool isNull() const
   {
      return getPtrOrNull() == nullptr;
   }

   /// Return true if this result has an AST node.
   ///
   /// Note that this does not tell us if there was a parse error or not.
   bool isNonNull() const
   {
      return getPtrOrNull() != nullptr;
   }

   /// Return the AST node if non-null.
   T *get() const
   {
      assert(getPtrOrNull() && "not checked for nullptr");
      return getPtrOrNull();
   }

   /// Return the AST node or a null pointer.
   T *getPtrOrNull() const
   {
      return m_ptrAndBits.getPointer();
   }

   /// Return true if there was a parse error.
   ///
   /// Note that we can still have an AST node which was constructed during
   /// recovery.
   bool isParseError() const
   {
      return m_ptrAndBits.getInt() & m_isError;
   }

   /// Return true if we found a code completion token while parsing this.
   bool hasCodeCompletion() const
   {
      return m_ptrAndBits.getInt() & m_isCodeCompletion;
   }

   void setIsParseError()
   {
      m_ptrAndBits.setInt(m_ptrAndBits.getInt() | m_isError);
   }

   void setHasCodeCompletion()
   {
      m_ptrAndBits.setInt(m_ptrAndBits.getInt() | m_isError | m_isCodeCompletion);
   }

private:
   PointerIntPair<T *, 2> m_ptrAndBits;
   template <typename U> friend class ParserResult;
};

/// Create a successful parser result.
template <typename T>
static inline ParserResult<T> make_parser_result(T *result)
{
   return ParserResult<T>(result);
}

/// Create a result (null or non-null) with error bit set.
template <typename T>
static inline ParserResult<T> makeParserErrorResult(T *result = nullptr)
{
   ParserResult<T> parsedResult;
   if (result) {
      parsedResult = ParserResult<T>(result);
   }
   parsedResult.setIsParseError();
   return parsedResult;
}

/// Create a result (null or non-null) with error and code completion bits set.
template <typename T>
static inline ParserResult<T> makeParserCodeCompletionResult(T *result =
      nullptr)
{
   ParserResult<T> parsedResult;
   if (result) {
      parsedResult = ParserResult<T>(result);
   }
   parsedResult.setHasCodeCompletion();
   return parsedResult;
}

/// Same as \c ParserResult, but just the status bits without the AST
/// node.
///
/// Useful when the AST node is returned by some other means (for example, in
/// a vector out parameter).
///
/// If you want to use 'bool' as a result type in the Parser, consider using
/// ParserStatus instead.
class ParserStatus
{
public:
   /// Construct a successful parser status.
   ParserStatus()
      : m_isError(0),
        m_isCodeCompletion(0)
   {}

   /// Construct a parser status with specified bits.
   template<typename T>
   ParserStatus(ParserResult<T> result)
      : m_isError(0),
        m_isCodeCompletion(0)
   {
      if (result.isParseError())
         setIsParseError();
      if (result.hasCodeCompletion())
         setHasCodeCompletion();
   }

   bool isSuccess() const
   {
      return !isError();
   }

   bool isError() const
   {
      return m_isError;
   }

   /// Return true if we found a code completion token while parsing this.
   bool hasCodeCompletion() const
   {
      return m_isCodeCompletion;
   }

   void setIsParseError()
   {
      m_isError = true;
   }

   void setHasCodeCompletion()
   {
      m_isError = true;
      m_isCodeCompletion = true;
   }

   /// True if we should stop parsing for any reason.
   bool shouldStopParsing() const
   {
      return m_isError || m_isCodeCompletion;
   }

   ParserStatus &operator|=(ParserStatus other)
   {
      m_isError |= other.m_isError;
      m_isCodeCompletion |= other.m_isCodeCompletion;
      return *this;
   }

   friend ParserStatus operator|(ParserStatus lhs, ParserStatus rhs)
   {
      ParserStatus result = lhs;
      result |= rhs;
      return result;
   }
private:
   unsigned m_isError : 1;
   unsigned m_isCodeCompletion : 1;
};

namespace {
/// Create a successful parser status.
inline ParserStatus make_parser_success()
{
   return ParserStatus();
}

/// Create a status with error bit set.
inline ParserStatus make_parser_error()
{
   ParserStatus status;
   status.setIsParseError();
   return status;
}

/// Create a status with error and code completion bits set.
inline ParserStatus make_parser_code_completion_status()
{
   ParserStatus status;
   status.setHasCodeCompletion();
   return status;
}

/// Create a parser result with specified bits.
template <typename T>
inline ParserResult<T> make_parser_result(ParserStatus status,
                                               T *result)
{
   if (status.isSuccess()) {
      return make_parser_result(result);
   }
   if (status.hasCodeCompletion()) {
      return makeParserCodeCompletionResult(result);
   }
   return makeParserErrorResult(result);
}
} // anonymous namespace

template <typename T>
ParserResult<T>::ParserResult(ParserStatus status)
{
   assert(status.isError());
   setIsParseError();
   if (status.hasCodeCompletion()) {
      setHasCodeCompletion();
   }
}

} // polar::parser

#endif // POLARPHP_PARSER_PARSER_RESULT_H
