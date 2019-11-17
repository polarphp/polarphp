//==-- llvm/Support/FileCheck.h ---------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
// Created by polarboy on 2019/04/23.

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include "FileCheckerConfig.h"
#include <boost/regex.hpp>
#include <vector>
#include <map>

namespace polar {
namespace filechecker {

using llvm::StringRef;
using llvm::StringMap;
using llvm::SmallVectorImpl;
using llvm::ArrayRef;
using llvm::Twine;
using llvm::SourceMgr;
using llvm::SMLoc;
using llvm::SMRange;
using llvm::MemoryBuffer;
using llvm::Expected;
using llvm::ErrorInfo;
using llvm::raw_ostream;
using llvm::Error;
using llvm::SMDiagnostic;
using llvm::inconvertibleErrorCode;
using llvm::make_error;

/// Contains info about various FileCheck options.
struct FileCheckRequest
{
   std::vector<std::string> checkPrefixes;
   bool noCanonicalizeWhiteSpace = false;
   std::vector<std::string> implicitCheckNot;
   std::vector<std::string> globalDefines;
   bool allowEmptyInput = false;
   bool matchFullLines = false;
   bool enableVarScope = false;
   bool allowDeprecatedDagOverlap = false;
   bool verbose = false;
   bool verboseVerbose = false;
};

//===----------------------------------------------------------------------===//
// Numeric substitution handling code.
//===----------------------------------------------------------------------===//

/// Base class representing the AST of a given expression.
class FileCheckExpressionAST
{
public:
   virtual ~FileCheckExpressionAST() = default;

   /// Evaluates and \returns the value of the expression represented by this
   /// AST or an error if evaluation fails.
   virtual Expected<uint64_t> eval() const = 0;
};

/// Class representing an unsigned literal in the AST of an expression.
class FileCheckExpressionLiteral : public FileCheckExpressionAST
{
private:
   /// Actual value of the literal.
   uint64_t m_value;

public:
   /// Constructs a literal with the specified value.
   FileCheckExpressionLiteral(uint64_t value)
      : m_value(value)
   {}

   /// \returns the literal's value.
   Expected<uint64_t> eval() const
   {
      return m_value;
   }
};

/// Class to represent an undefined variable error, which quotes that
/// variable's name when printed.
class FileCheckUndefVarError : public ErrorInfo<FileCheckUndefVarError>
{
private:
   StringRef m_varName;

public:
   static char ID;

   FileCheckUndefVarError(StringRef varName)
      : m_varName(varName)
   {}

   StringRef getVarName() const
   {
      return m_varName;
   }

   std::error_code convertToErrorCode() const override
   {
      return inconvertibleErrorCode();
   }

   /// Print name of variable associated with this error.
   void log(raw_ostream &outStream) const override
   {
      outStream << "\"";
      outStream.write_escaped(m_varName) << "\"";
   }
};

/// Class representing a numeric variable and its associated current value.
class FileCheckNumericVariable {
private:
   /// m_name of the numeric variable.
   StringRef m_name;

   /// m_value of numeric variable, if defined, or None otherwise.
   std::optional<uint64_t> m_value;

   /// Line number where this variable is defined, or None if defined before
   /// input is parsed. Used to determine whether a variable is defined on the
   /// same line as a given use.
   std::optional<size_t> m_defLineNumber;

public:
   /// Constructor for a variable \p m_name defined at line \p m_defLineNumber or
   /// defined before input is parsed if m_defLineNumber is None.
   FileCheckNumericVariable(StringRef name,
                            std::optional<size_t> defLineNumber = std::nullopt)
      : m_name(name),
        m_defLineNumber(defLineNumber)
   {}

   /// \returns name of this numeric variable.
   StringRef getName() const
   {
      return m_name;
   }

   /// \returns this variable's value.
   std::optional<uint64_t> getValue() const
   {
      return m_value;
   }

   /// Sets value of this numeric variable, if undefined. Triggers an assertion
   /// failure if the variable is actually defined.
   void setValue(uint64_t m_value);

   /// Clears value of this numeric variable, regardless of whether it is
   /// currently defined or not.
   void clearValue();

   /// \returns the line number where this variable is defined, if any, or None
   /// if defined before input is parsed.
   std::optional<size_t> getDefLineNumber()
   {
      return m_defLineNumber;
   }
};

/// Class representing the use of a numeric variable in the AST of an
/// expression.
class FileCheckNumericVariableUse : public FileCheckExpressionAST
{
private:
   /// m_name of the numeric variable.
   StringRef m_name;

   /// Pointer to the class instance for the variable this use is about.
   FileCheckNumericVariable *m_numericVariable;

public:
   FileCheckNumericVariableUse(StringRef m_name,
                               FileCheckNumericVariable *numericVariable)
      : m_name(m_name),
        m_numericVariable(numericVariable)
   {}

   /// \returns the value of the variable referenced by this instance.
   Expected<uint64_t> eval() const;
};

/// Type of functions evaluating a given binary operation.
using binop_eval_t = uint64_t (*)(uint64_t, uint64_t);

/// Class representing a single binary operation in the AST of an expression.
class FileCheckASTBinop : public FileCheckExpressionAST
{
private:
   /// Left operand.
   std::unique_ptr<FileCheckExpressionAST> m_leftOperand;

   /// Right operand.
   std::unique_ptr<FileCheckExpressionAST> m_rightOperand;

   /// Pointer to function that can evaluate this binary operation.
   binop_eval_t m_evalBinop;

public:
   FileCheckASTBinop(binop_eval_t evalBinop,
                     std::unique_ptr<FileCheckExpressionAST> leftOp,
                     std::unique_ptr<FileCheckExpressionAST> rightOp)
      : m_evalBinop(evalBinop)
   {
      m_leftOperand = std::move(leftOp);
      m_rightOperand = std::move(rightOp);
   }

   /// Evaluates the value of the binary operation represented by this AST,
   /// using m_evalBinop on the result of recursively evaluating the operands.
   /// \returns the expression value or an error if an undefined numeric
   /// variable is used in one of the operands.
   Expected<uint64_t> eval() const;
};

class FileCheckPatternContext;

/// Class representing a substitution to perform in the RegExStr string.
class FileCheckSubstitution
{
protected:
   /// Pointer to a class instance holding, among other things, the table with
   /// the values of live string variables at the start of any given CHECK line.
   /// Used for substituting string variables with the text they were defined
   /// as. Expressions are linked to the numeric variables they use at
   /// parse time and directly access the value of the numeric variable to
   /// evaluate their value.
   FileCheckPatternContext *m_context;

   /// The string that needs to be substituted for something else. For a
   /// string variable this is its name, otherwise this is the whole expression.
   StringRef m_fromStr;

   // Index in RegExStr of where to do the substitution.
   size_t m_insertIdx;

public:
   FileCheckSubstitution(FileCheckPatternContext *context, StringRef varName,
                         size_t insertIdx)
      : m_context(context),
        m_fromStr(varName),
        m_insertIdx(insertIdx)
   {}

   virtual ~FileCheckSubstitution() = default;

   /// \returns the string to be substituted for something else.
   StringRef getFromString() const
   {
      return m_fromStr;
   }

   /// \returns the index where the substitution is to be performed in RegExStr.
   size_t getIndex() const
   {
      return m_insertIdx;
   }

   /// \returns a string containing the result of the substitution represented
   /// by this class instance or an error if substitution failed.
   virtual Expected<std::string> getResult() const = 0;
};

class FileCheckStringSubstitution : public FileCheckSubstitution
{
public:
   FileCheckStringSubstitution(FileCheckPatternContext *context,
                               StringRef varName, size_t insertIdx)
      : FileCheckSubstitution(context, varName, insertIdx) {}

   /// \returns the text that the string variable in this substitution matched
   /// when defined, or an error if the variable is undefined.
   Expected<std::string> getResult() const override;
};

class FileCheckNumericSubstitution : public FileCheckSubstitution
{
private:
   /// Pointer to the class representing the expression whose value is to be
   /// substituted.
   std::unique_ptr<FileCheckExpressionAST> m_expressionAST;

public:
   FileCheckNumericSubstitution(FileCheckPatternContext *context, StringRef expr,
                                std::unique_ptr<FileCheckExpressionAST> exprAST,
                                size_t insertIdx)
      : FileCheckSubstitution(context, expr, insertIdx)
   {
      m_expressionAST = std::move(exprAST);
   }

   /// \returns a string containing the result of evaluating the expression in
   /// this substitution, or an error if evaluation failed.
   Expected<std::string> getResult() const override;
};


//===----------------------------------------------------------------------===//
// Pattern Handling Code.
//===----------------------------------------------------------------------===//
namespace check {
enum FileCheckKind
{
   CheckNone = 0,
   CheckPlain,
   CheckNext,
   CheckSame,
   CheckNot,
   CheckDAG,
   CheckLabel,
   CheckEmpty,

   /// Indicates the pattern only matches the end of file. This is used for
   /// trailing CHECK-NOTs.
   CheckEOF,

   /// Marks when parsing found a -NOT check combined with another CHECK suffix.
   CheckBadNot,

   /// Marks when parsing found a -COUNT directive with invalid count value.
   CheckBadCount
};

class FileCheckType
{
   FileCheckKind m_kind;
   int m_count; ///< optional Count for some checks

public:
   FileCheckType(FileCheckKind kind = CheckNone)
      : m_kind(kind),
        m_count(1)
   {}

   FileCheckType(const FileCheckType &) = default;

   operator FileCheckKind() const
   {
      return m_kind;
   }

   int getCount() const
   {
      return m_count;
   }

   FileCheckType &setCount(int count);

   // \returns a description of \p Prefix.
   std::string getDescription(StringRef prefix) const;
};
} // namespace check

struct FileCheckDiag;

/// Class holding the FileCheckPattern global state, shared by all patterns:
/// tables holding values of variables and whether they are defined or not at
/// any given time in the matching process.
class FileCheckPatternContext
{
   friend class FileCheckPattern;

private:
   /// When matching a given pattern, this holds the value of all the string
   /// variables defined in previous patterns. In a pattern, only the last
   /// definition for a given variable is recorded in this table.
   /// Back-references are used for uses after any the other definition.
   StringMap<std::string> m_globalVariableTable;

   /// Map of all string variables defined so far. Used at parse time to detect
   /// a name conflict between a numeric variable and a string variable when
   /// the former is defined on a later line than the latter.
   StringMap<bool> m_definedVariableTable;

   /// When matching a given pattern, this holds the pointers to the classes
   /// representing the numeric variables defined in previous patterns. When
   /// matching a pattern all definitions for that pattern are recorded in the
   /// NumericVariableDefs table in the FileCheckPattern instance of that
   /// pattern.
   StringMap<FileCheckNumericVariable *> m_globalNumericVariableTable;

   /// Pointer to the class instance representing the @LINE pseudo variable for
   /// easily updating its value.
   FileCheckNumericVariable *m_lineVariable = nullptr;

   /// Vector holding pointers to all parsed numeric variables. Used to
   /// automatically free them once they are guaranteed to no longer be used.
   std::vector<std::unique_ptr<FileCheckNumericVariable>> m_numericVariables;

   /// Vector holding pointers to all substitutions. Used to automatically free
   /// them once they are guaranteed to no longer be used.
   std::vector<std::unique_ptr<FileCheckSubstitution>> m_substitutions;

public:
   /// \returns the value of string variable \p varName or an error if no such
   /// variable has been defined.
   Expected<StringRef> getPatternVarValue(StringRef varName);

   /// Defines string and numeric variables from definitions given on the
   /// command line, passed as a vector of [#]VAR=VAL strings in
   /// \p cmdlineDefines. \returns an error list containing diagnostics against
   /// \p SM for all definition parsing failures, if any, or Success otherwise.
   Error defineCmdlineVariables(std::vector<std::string> &cmdlineDefines,
                                SourceMgr &SM);

   /// Create @LINE pseudo variable. Value is set when pattern are being
   /// matched.
   void createLineVariable();

   /// Undefines local variables (variables whose name does not start with a '$'
   /// sign), i.e. removes them from m_globalVariableTable and from
   /// m_globalNumericVariableTable and also clears the value of numeric
   /// variables.
   void clearLocalVars();

private:
   /// Makes a new numeric variable and registers it for destruction when the
   /// m_context is destroyed.
   template <class... Types>
   FileCheckNumericVariable *makeNumericVariable(Types... args);

   /// Makes a new string substitution and registers it for destruction when the
   /// m_context is destroyed.
   FileCheckSubstitution *makeStringSubstitution(StringRef varName,
                                                 size_t insertIdx);

   /// Makes a new numeric substitution and registers it for destruction when
   /// the m_context is destroyed.
   FileCheckSubstitution *
   makeNumericSubstitution(StringRef expressionStr,
                           std::unique_ptr<FileCheckExpressionAST> expressionAST,
                           size_t insertIdx);
};

/// Class to represent an error holding a diagnostic with location information
/// used when printing it.
class FileCheckErrorDiagnostic : public ErrorInfo<FileCheckErrorDiagnostic>
{
private:
   SMDiagnostic m_diagnostic;

public:
   static char ID;

   FileCheckErrorDiagnostic(SMDiagnostic &&diag)
      : m_diagnostic(diag)
   {}

   std::error_code convertToErrorCode() const override
   {
      return inconvertibleErrorCode();
   }

   /// Print diagnostic associated with this error when printing the error.
   void log(raw_ostream &outStream) const override
   {
      m_diagnostic.print(nullptr, outStream);
   }

   static Error get(const SourceMgr &sourceMgr, SMLoc loc, const Twine &errorMsg)
   {
      return make_error<FileCheckErrorDiagnostic>(
               sourceMgr.GetMessage(loc, SourceMgr::DK_Error, errorMsg));
   }

   static Error get(const SourceMgr &sourceMgr, StringRef buffer, const Twine &errorMsg)
   {
      return get(sourceMgr, SMLoc::getFromPointer(buffer.data()), errorMsg);
   }
};

class FileCheckNotFoundError : public ErrorInfo<FileCheckNotFoundError>
{
public:
   static char ID;

   std::error_code convertToErrorCode() const override
   {
      return inconvertibleErrorCode();
   }

   /// Print diagnostic associated with this error when printing the error.
   void log(raw_ostream &outStream) const override
   {
      outStream << "String not found in input";
   }
};

class FileCheckPattern
{
private:
   SMLoc m_patternLoc;

   /// A fixed string to match as the pattern or empty if this pattern requires
   /// a regex match.
   StringRef m_fixedStr;

   /// A regex string to match as the pattern or empty if this pattern requires
   /// a fixed string to match.
   std::string m_regExStr;

   /// Entries in this vector represent a substitution of a string variable or
   /// an expression in the RegExStr regex at match time. For example, in the
   /// case of a CHECK directive with the pattern "foo[[bar]]baz[[#N+1]]",
   /// RegExStr will contain "foobaz" and we'll get two entries in this vector
   /// that tells us to insert the value of string variable "bar" at offset 3
   /// and the value of expression "N+1" at offset 6.
   std::vector<FileCheckSubstitution *> m_substitutions;

   /// Maps names of string variables defined in a pattern to the number of
   /// their parenthesis group in RegExStr capturing their last definition.
   ///
   /// E.g. for the pattern "foo[[bar:.*]]baz([[bar]][[QUUX]][[bar:.*]])",
   /// RegExStr will be "foo(.*)baz(\1<quux value>(.*))" where <quux value> is
   /// the value captured for QUUX on the earlier line where it was defined, and
   /// VariableDefs will map "bar" to the third parenthesis group which captures
   /// the second definition of "bar".
   ///
   /// Note: uses std::map rather than StringMap to be able to get the key when
   /// iterating over values.
   std::map<StringRef, unsigned> m_variableDefs;

   /// Structure representing the definition of a numeric variable in a pattern.
   /// It holds the pointer to the class representing the numeric variable whose
   /// value is being defined and the number of the parenthesis group in
   /// RegExStr to capture that value.
   struct FileCheckNumericVariableMatch
   {
      /// Pointer to class representing the numeric variable whose value is being
      /// defined.
      FileCheckNumericVariable *definedNumericVariable;

      /// Number of the parenthesis group in RegExStr that captures the value of
      /// this numeric variable definition.
      unsigned captureParenGroup;
   };

   /// Holds the number of the parenthesis group in RegExStr and pointer to the
   /// corresponding FileCheckNumericVariable class instance of all numeric
   /// variable definitions. Used to set the matched value of all those
   /// variables.
   StringMap<FileCheckNumericVariableMatch> numericVariableDefs;

   /// Pointer to a class instance holding the global state shared by all
   /// patterns:
   /// - separate tables with the values of live string and numeric variables
   ///   respectively at the start of any given CHECK line;
   /// - table holding whether a string variable has been defined at any given
   ///   point during the parsing phase.
   FileCheckPatternContext *m_context;

   check::FileCheckType m_checkType;

   /// Line number for this CHECK pattern or None if it is an implicit pattern.
   /// Used to determine whether a variable definition is made on an earlier
   /// line to the one with this CHECK.
   std::optional<size_t> m_lineNumber;

public:
   FileCheckPattern(check::FileCheckType checkType, FileCheckPatternContext *context,
                    std::optional<size_t> line = std::nullopt)
      : m_context(context),
        m_checkType(checkType),
        m_lineNumber(line)
   {}

   /// \returns the location in source code.
   SMLoc getLoc() const
   {
      return m_patternLoc;
   }

   /// \returns the pointer to the global state for all patterns in this
   /// FileCheck instance.
   FileCheckPatternContext *getContext() const
   {
      return m_context;
   }

   /// \returns whether \p C is a valid first character for a variable name.
   static bool isValidVarNameStart(char C);

   /// Parsing information about a variable.
   struct VariableProperties
   {
      StringRef name;
      bool isPseudo;
   };

   /// Parses the string at the start of \p Str for a variable name. \returns
   /// a VariableProperties structure holding the variable name and whether it
   /// is the name of a pseudo variable, or an error holding a diagnostic
   /// against \p SM if parsing fail. If parsing was successful, also strips
   /// \p Str from the variable name.
   static Expected<VariableProperties> parseVariable(StringRef &str,
                                                     const SourceMgr &sourceMgr);

   /// Parses \p Expr for the name of a numeric variable to be defined at line
   /// \p LineNumber or before input is parsed if \p LineNumber is None.
   /// \returns a pointer to the class instance representing that variable,
   /// creating it if needed, or an error holding a diagnostic against \p SM
   /// should defining such a variable be invalid.
   static Expected<FileCheckNumericVariable *> parseNumericVariableDefinition(
         StringRef &Expr, FileCheckPatternContext *Context,
         std::optional<size_t> lineNumber, const SourceMgr &sourceMgr);
   /// Parses \p Expr for a numeric substitution block. Parameter
   /// \p IsLegacyLineExpr indicates whether \p Expr should be a legacy @LINE
   /// expression. \returns a pointer to the class instance representing the AST
   /// of the expression whose value must be substituted, or an error holding a
   /// diagnostic against \p SM if parsing fails. If substitution was
   /// successful, sets \p DefinedNumericVariable to point to the class
   /// representing the numeric variable being defined in this numeric
   /// substitution block, or None if this block does not define any variable.
   Expected<std::unique_ptr<FileCheckExpressionAST>>
   parseNumericSubstitutionBlock(
         StringRef Expr,
         std::optional<FileCheckNumericVariable *> &definedNumericVariable,
         bool isLegacyLineExpr, const SourceMgr &sourceMgr) const;

   /// Parses the pattern in \p PatternStr and initializes this FileCheckPattern
   /// instance accordingly.
   ///
   /// \p Prefix provides which prefix is being matched, \p Req describes the
   /// global options that influence the parsing such as whitespace
   /// canonicalization, \p SM provides the SourceMgr used for error reports.
   /// \returns true in case of an error, false otherwise.
   bool parsePattern(StringRef patternStr, StringRef prefix, SourceMgr &sourceMgr,
                     const FileCheckRequest &req);

   /// Matches the pattern string against the input buffer \p Buffer
   ///
   /// \returns the position that is matched or an error indicating why matching
   /// failed. If there is a match, updates \p MatchLen with the size of the
   /// matched string.
   ///
   /// The GlobalVariableTable StringMap in the FileCheckPatternContext class
   /// instance provides the current values of FileCheck string variables and
   /// is updated if this match defines new values. Likewise, the
   /// GlobalNumericVariableTable StringMap in the same class provides the
   /// current values of FileCheck numeric variables and is updated if this
   /// match defines new numeric values.
   Expected<size_t> match(StringRef buffer, size_t &matchLen,
                          const SourceMgr &sourceMgr) const;
   /// Prints the value of successful substitutions or the name of the undefined
   /// string or numeric variables preventing a successful substitution.
   void printSubstitutions(const SourceMgr &sourceMgr, StringRef buffer,
                           SMRange matchRange = llvm::None) const;
   void printFuzzyMatch(const SourceMgr &sourceMgr, StringRef buffer,
                        std::vector<FileCheckDiag> *diags) const;

   bool hasVariable() const
   {
      return !(m_substitutions.empty() && m_variableDefs.empty());
   }

   check::FileCheckType getCheckType() const
   {
      return m_checkType;
   }

   int getCount() const
   {
      return m_checkType.getCount();
   }

private:
   bool addRegExToRegEx(StringRef regexStr, unsigned &curParen, SourceMgr &sourceMgr);
   void addBackrefToRegEx(unsigned backrefNum);
   /// Computes an arbitrary estimate for the quality of matching this pattern
   /// at the start of \p Buffer; a distance of zero should correspond to a
   /// perfect match.
   unsigned computeMatchDistance(StringRef buffer) const;
   /// Finds the closing sequence of a regex variable usage or definition.
   ///
   /// \p Str has to point in the beginning of the definition (right after the
   /// opening sequence). \p SM holds the SourceMgr used for error repporting.
   ///  \returns the offset of the closing sequence within Str, or npos if it
   /// was not found.
   size_t findRegexVarEnd(StringRef str, SourceMgr &sourceMgr);

   /// Parses \p Name as a (pseudo if \p IsPseudo is true) numeric variable use.
   /// \returns the pointer to the class instance representing that variable if
   /// successful, or an error holding a diagnostic against \p SM otherwise.
   Expected<std::unique_ptr<FileCheckNumericVariableUse>>
   parseNumericVariableUse(StringRef name, bool isPseudo,
                           const SourceMgr &sourceMgr) const;
   enum class AllowedOperand { LineVar, Literal, Any };
   /// Parses \p Expr for use of a numeric operand. Accepts both literal values
   /// and numeric variables, depending on the value of \p AO. \returns the
   /// class representing that operand in the AST of the expression or an error
   /// holding a diagnostic against \p SM otherwise.
   Expected<std::unique_ptr<FileCheckExpressionAST>>
   parseNumericOperand(StringRef &expr, AllowedOperand allowedOperand,
                       const SourceMgr &sourceMgr) const;
   /// Parses \p Expr for a binary operation. The left operand of this binary
   /// operation is given in \p LeftOp and \p IsLegacyLineExpr indicates whether
   /// we are parsing a legacy @LINE expression. \returns the class representing
   /// the binary operation in the AST of the expression, or an error holding a
   /// diagnostic against \p SM otherwise.
   Expected<std::unique_ptr<FileCheckExpressionAST>>
   parseBinop(StringRef &expr, std::unique_ptr<FileCheckExpressionAST> leftOp,
              bool isLegacyLineExpr, const SourceMgr &sourceMgr) const;
};

//===----------------------------------------------------------------------===//
/// Summary of a FileCheck diagnostic.
//===----------------------------------------------------------------------===//

struct FileCheckDiag
{
   /// What is the FileCheck directive for this diagnostic?
   check::FileCheckType checkType;
   /// Where is the FileCheck directive for this diagnostic?
   unsigned checkLine;
   unsigned checkCol;

   /// What type of match result does this diagnostic describe?
   ///
   /// A directive's supplied pattern is said to be either expected or excluded
   /// depending on whether the pattern must have or must not have a match in
   /// order for the directive to succeed.  For example, a CHECK directive's
   /// pattern is expected, and a CHECK-NOT directive's pattern is excluded.
   /// All match result types whose names end with "Excluded" are for excluded
   /// patterns, and all others are for expected patterns.
   ///
   /// There might be more than one match result for a single pattern.  For
   /// example, there might be several discarded matches
   /// (MatchFoundButDiscarded) before either a good match
   /// (MatchFoundAndExpected) or a failure to match (MatchNoneButExpected),
   /// and there might be a fuzzy match (MatchFuzzy) after the latter.
   enum MatchType
   {
      /// Indicates a good match for an expected pattern.
      MatchFoundAndExpected,
      /// Indicates a match for an excluded pattern.
      MatchFoundButExcluded,
      /// Indicates a match for an expected pattern, but the match is on the
      /// wrong line.
      MatchFoundButWrongLine,
      /// Indicates a discarded match for an expected pattern.
      MatchFoundButDiscarded,
      /// Indicates no match for an excluded pattern.
      MatchNoneAndExcluded,
      /// Indicates no match for an expected pattern, but this might follow good
      /// matches when multiple matches are expected for the pattern, or it might
      /// follow discarded matches for the pattern.
      MatchNoneButExpected,
      /// Indicates a fuzzy match that serves as a suggestion for the next
      /// intended match for an expected pattern with too few or no good matches.
      MatchFuzzy,
   } matchType;
   /// The search range if MatchTy is MatchNoneAndExcluded or
   /// MatchNoneButExpected, or the match range otherwise.
   unsigned inputStartLine;
   unsigned inputStartCol;
   unsigned inputEndLine;
   unsigned inputEndCol;

   FileCheckDiag(const SourceMgr &sourceMgr, const check::FileCheckType &checkType,
                 SMLoc checkLoc, MatchType matchType, SMRange inputRange);
};

//===----------------------------------------------------------------------===//
// Check Strings.
//===----------------------------------------------------------------------===//

/// A check that we found in the input file.
struct FileCheckString
{
   /// The pattern to match.
   FileCheckPattern pattern;

   /// Which prefix name this check matched.
   StringRef prefix;

   /// The location in the match file that the check string was specified.
   SMLoc loc;

   /// All of the strings that are disallowed from occurring between this match
   /// string and the previous one (or start of file).
   std::vector<FileCheckPattern> m_dagNotStrings;

   FileCheckString(const FileCheckPattern &pattern, StringRef str, SMLoc loc)
      : pattern(pattern),
        prefix(str),
        loc(loc)
   {}

   /// Matches check string and its "not strings" and/or "dag strings".
   size_t check(const SourceMgr &sourceMgr, StringRef buffer, bool isLabelScanMode,
                size_t &matchLen, FileCheckRequest &req, std::vector<FileCheckDiag> *diags) const;

   /// Verifies that there is a single line in the given \p Buffer. Errors are
   /// reported against \p SM.
   bool checkNext(const SourceMgr &sourceMgr, StringRef buffer) const;
   /// Verifies that there is no newline in the given \p Buffer. Errors are
   /// reported against \p SM.
   bool checkSame(const SourceMgr &sourceMgr, StringRef buffer) const;
   /// Verifies that none of the strings in \p NotStrings are found in the given
   /// \p Buffer. Errors are reported against \p SM and diagnostics recorded in
   /// \p Diags according to the verbosity level set in \p Req.
   bool checkNot(const SourceMgr &sourceMgr, StringRef buffer,
                 const std::vector<const FileCheckPattern *> &notStrings,
                 const FileCheckRequest &req, std::vector<FileCheckDiag> *diags) const;
   /// Matches "dag strings" and their mixed "not strings".
   size_t checkDag(const SourceMgr &sourceMgr, StringRef buffer,
                   std::vector<const FileCheckPattern *> &notStrings,
                   const FileCheckRequest &req, std::vector<FileCheckDiag> *diags) const;
};

/// FileCheck class takes the request and exposes various methods that
/// use information from the request.
class FileCheck
{
private:
   FileCheckRequest m_req;
   FileCheckPatternContext m_patternContext;
public:
   FileCheck(FileCheckRequest req)
      : m_req(req)
   {}

   // Combines the check prefixes into a single regex so that we can efficiently
   // scan for any of the set.
   //
   // The semantics are that the longest-match wins which matches our regex
   // library.
   boost::regex buildCheckPrefixRegex();

   /// Reads the check file from \p Buffer and records the expected strings it
   /// contains in the \p CheckStrings vector. Errors are reported against
   /// \p SM.
   ///
   /// Only expected strings whose prefix is one of those listed in \p PrefixRE
   /// are recorded. \returns true in case of an error, false otherwise.
   bool readCheckFile(SourceMgr &sourceMgr, StringRef buffer, boost::regex &prefixRE,
                      std::vector<FileCheckString> &checkStrings);

   bool validateCheckPrefixes();

   /// Canonicalize whitespaces in the file. Line endings are replaced with
   /// UNIX-style '\n'.
   StringRef canonicalizeFile(MemoryBuffer &memoryBuffer,
                              SmallVectorImpl<char> &outputBuffer);

   /// Checks the input to FileCheck provided in the \p Buffer against the
   /// \p CheckStrings read from the check file and record diagnostics emitted
   /// in \p Diags. Errors are recorded against \p SM.
   ///
   /// \returns false if the input fails to satisfy the checks.
   bool checkInput(SourceMgr &sourceMgr, StringRef buffer,
                   ArrayRef<FileCheckString> checkStrings,
                   std::vector<FileCheckDiag> *diags = nullptr);
};

} // filechecker
} // polar
