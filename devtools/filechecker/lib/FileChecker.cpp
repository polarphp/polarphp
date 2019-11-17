//===- FileCheck.cpp - Check that File's Contents match what is expected --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
// Created by polarboy on 2019/04/23.

#include "FileChecker.h"

#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FormatVariadic.h"
#include "polarphp/utils/StringUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/SourceMgr.h"

#include <cstdint>
#include <list>
#include <map>
#include <tuple>
#include <utility>

namespace polar::filechecker {

using llvm::SmallString;
using llvm::Twine;
using llvm::StringSet;
using llvm::StringRef;
using llvm::SmallVector;
using llvm::StringMapEntry;
using polar::utils::regex_escape;
using llvm::raw_svector_ostream;
using llvm::consumeError;
using llvm::logAllUnhandledErrors;
using llvm::handleAllErrors;
using llvm::joinErrors;
using llvm::formatv;
using llvm::make_error;
using llvm::Expected;
using llvm::Error;
using check::FileCheckType;
using check::FileCheckKind;
using llvm::SourceMgr;

void FileCheckNumericVariable::setValue(uint64_t newValue)
{
   assert(!m_value && "Overwriting numeric variable's value is not allowed");
   m_value = newValue;
}

void FileCheckNumericVariable::clearValue()
{
   if (!m_value) {
      return;
   }
   m_value = std::nullopt;
}

Expected<uint64_t> FileCheckNumericVariableUse::eval() const
{
   std::optional<uint64_t> m_value = m_numericVariable->getValue();
   if (m_value) {
      return *m_value;
   }
   return make_error<FileCheckUndefVarError>(m_name);
}

Expected<uint64_t> FileCheckASTBinop::eval() const
{
   Expected<uint64_t> leftOp = m_leftOperand->eval();
   Expected<uint64_t> rightOp = m_rightOperand->eval();

   // Bubble up any error (e.g. undefined variables) in the recursive
   // evaluation.
   if (!leftOp || !rightOp) {
      Error error = Error::success();
      if (!leftOp) {
         error = joinErrors(std::move(error), leftOp.takeError());
      }
      if (!rightOp) {
         error = joinErrors(std::move(error), rightOp.takeError());
      }
      return std::move(error);
   }

   return m_evalBinop(*leftOp, *rightOp);
}

Expected<std::string> FileCheckNumericSubstitution::getResult() const
{
   Expected<uint64_t> evaluatedValue = m_expressionAST->eval();
   if (!evaluatedValue) {
      return evaluatedValue.takeError();
   }
   return llvm::utostr(*evaluatedValue);
}

Expected<std::string> FileCheckStringSubstitution::getResult() const
{
   // Look up the value and escape it so that we can put it into the regex.
   Expected<StringRef> varVal = m_context->getPatternVarValue(m_fromStr);
   if (!varVal) {
      return varVal.takeError();
   }
   return regex_escape(*varVal);
}

bool FileCheckPattern::isValidVarNameStart(char C)
{
   return C == '_' || isalpha(C);
}

Expected<FileCheckPattern::VariableProperties>
FileCheckPattern::parseVariable(StringRef &str, const SourceMgr &sourceMgr) {
   if (str.empty()) {
      return FileCheckErrorDiagnostic::get(sourceMgr, str, "empty variable name");
   }
   bool parsedOneChar = false;
   unsigned index = 0;
   bool isPseudo = str[0] == '@';
   // Global vars Start with '$'.
   if (str[0] == '$' || isPseudo) {
      ++index;
   }
   for (unsigned end = str.size(); index != end; ++index) {
      if (!parsedOneChar && !isValidVarNameStart(str[index])) {
         return FileCheckErrorDiagnostic::get(sourceMgr, str, "invalid variable name");
      }
      // Variable names are composed of alphanumeric characters and underscores.
      if (str[index] != '_' && !isalnum(str[index])) {
         break;
      }
      parsedOneChar = true;
   }
   StringRef name = str.take_front(index);
   str = str.substr(index);
   return VariableProperties {name, isPseudo};
}

// StringRef holding all characters considered as horizontal whitespaces by
// FileCheck input canonicalization.
StringRef g_spaceChars = " \t";

// Parsing helper function that strips the first character in S and returns it.
static char pop_front(StringRef &str)
{
   char C = str.front();
   str = str.drop_front();
   return C;
}

char FileCheckUndefVarError::ID = 0;
char FileCheckErrorDiagnostic::ID = 0;
char FileCheckNotFoundError::ID = 0;

Expected<FileCheckNumericVariable *>
FileCheckPattern::parseNumericVariableDefinition(
      StringRef &expr, FileCheckPatternContext *context,
      std::optional<size_t> lineNumber, const SourceMgr &sourceMgr)
{
   Expected<VariableProperties> parseVarResult = parseVariable(expr, sourceMgr);
   if (!parseVarResult) {
      return parseVarResult.takeError();
   }
   StringRef name = parseVarResult->name;
   if (parseVarResult->isPseudo) {
      return FileCheckErrorDiagnostic::get(
               sourceMgr, name, "definition of pseudo numeric variable unsupported");
   }
   // Detect collisions between string and numeric variables when the latter
   // is created later than the former.
   if (context->m_definedVariableTable.find(name) !=
       context->m_definedVariableTable.end()) {
      return FileCheckErrorDiagnostic::get(
               sourceMgr, name, "string variable with name '" + name + "' already exists");
   }
   expr = expr.ltrim(g_spaceChars);
   if (!expr.empty())
      return FileCheckErrorDiagnostic::get(
               sourceMgr, expr, "unexpected characters after numeric variable name");

   FileCheckNumericVariable *definedNumericVariable;
   auto varTableIter = context->m_globalNumericVariableTable.find(name);
   if (varTableIter != context->m_globalNumericVariableTable.end()) {
      definedNumericVariable = varTableIter->second;
   } else {
      definedNumericVariable = context->makeNumericVariable(name, lineNumber);
   }
   return definedNumericVariable;
}

Expected<std::unique_ptr<FileCheckNumericVariableUse>>
FileCheckPattern::parseNumericVariableUse(StringRef name, bool isPseudo,
                                          const SourceMgr &sourceMgr) const
{
   if (isPseudo && !name.equals("@LINE")) {
      return FileCheckErrorDiagnostic::get(
               sourceMgr, name, "invalid pseudo numeric variable '" + name + "'");
   }
   // Numeric variable definitions and uses are parsed in the order in which
   // they appear in the CHECK patterns. For each definition, the pointer to the
   // class instance of the corresponding numeric variable definition is stored
   // in m_globalNumericVariableTable in parsePattern. Therefore, if the pointer
   // we get below is null, it means no such variable was defined before. When
   // that happens, we create a dummy variable so that parsing can continue. All
   // uses of undefined variables, whether string or numeric, are then diagnosed
   // in printSubstitutions() after failing to match.
   auto varTableIter = m_context->m_globalNumericVariableTable.find(name);
   FileCheckNumericVariable *m_numericVariable;
   if (varTableIter != m_context->m_globalNumericVariableTable.end()) {
      m_numericVariable = varTableIter->second;
   } else {
      m_numericVariable = m_context->makeNumericVariable(name);
      m_context->m_globalNumericVariableTable[name] = m_numericVariable;
   }

   std::optional<size_t> defLineNumber = m_numericVariable->getDefLineNumber();
   if (defLineNumber && m_lineNumber && *defLineNumber == *m_lineNumber) {
      return FileCheckErrorDiagnostic::get(
               sourceMgr, name,
               "numeric variable '" + name + "' defined on the same line as used");
   }
   return std::make_unique<FileCheckNumericVariableUse>(name, m_numericVariable);
}

Expected<std::unique_ptr<FileCheckExpressionAST>>
FileCheckPattern::parseNumericOperand(StringRef &expr, AllowedOperand allowedOperand,
                                      const SourceMgr &sourceMgr) const
{
   if (allowedOperand == AllowedOperand::LineVar || allowedOperand == AllowedOperand::Any) {
      // Try to parse as a numeric variable use.
      Expected<FileCheckPattern::VariableProperties> parseVarResult =
            parseVariable(expr, sourceMgr);
      if (parseVarResult) {
         return parseNumericVariableUse(parseVarResult->name,
                                        parseVarResult->isPseudo, sourceMgr);
      }
      if (allowedOperand == AllowedOperand::LineVar) {
         return parseVarResult.takeError();
      }
      // Ignore the error and retry parsing as a literal.
      consumeError(parseVarResult.takeError());
   }

   // Otherwise, parse it as a literal.
   uint64_t LiteralValue;
   if (!expr.consumeInteger(/*Radix=*/10, LiteralValue)) {
      return std::make_unique<FileCheckExpressionLiteral>(LiteralValue);
   }
   return FileCheckErrorDiagnostic::get(sourceMgr, expr,
                                        "invalid operand format '" + expr + "'");
}

static uint64_t add(uint64_t leftOp, uint64_t rightOp)
{
   return leftOp + rightOp;
}

static uint64_t sub(uint64_t leftOp, uint64_t rightOp)
{
   return leftOp - rightOp;
}

Expected<std::unique_ptr<FileCheckExpressionAST>>
FileCheckPattern::parseBinop(StringRef &expr,
                             std::unique_ptr<FileCheckExpressionAST> leftOp,
                             bool isLegacyLineExpr, const SourceMgr &sourceMgr) const
{
   expr = expr.ltrim(g_spaceChars);
   if (expr.empty()) {
      return std::move(leftOp);
   }
   // Check if this is a supported operation and select a function to perform
   // it.
   SMLoc opLoc = SMLoc::getFromPointer(expr.data());
   char optor = pop_front(expr);
   binop_eval_t evalBinop;
   switch (optor) {
   case '+':
      evalBinop = add;
      break;
   case '-':
      evalBinop = sub;
      break;
   default:
      return FileCheckErrorDiagnostic::get(
               sourceMgr, opLoc, Twine("unsupported operation '") + Twine(optor) + "'");
   }

   // Parse right operand.
   expr = expr.ltrim(g_spaceChars);
   if (expr.empty()) {
      return FileCheckErrorDiagnostic::get(sourceMgr, expr,
                                           "missing operand in expression");
   }

   // The second operand in a legacy @LINE expression is always a literal.
   AllowedOperand allowedOperand =
         isLegacyLineExpr ? AllowedOperand::Literal : AllowedOperand::Any;
   Expected<std::unique_ptr<FileCheckExpressionAST>> rightOpResult =
         parseNumericOperand(expr, allowedOperand, sourceMgr);
   if (!rightOpResult) {
      return rightOpResult;
   }
   expr = expr.ltrim(g_spaceChars);
   return std::make_unique<FileCheckASTBinop>(evalBinop, std::move(leftOp),
                                              std::move(*rightOpResult));
}

Expected<std::unique_ptr<FileCheckExpressionAST>>
FileCheckPattern::parseNumericSubstitutionBlock(
      StringRef expr,
      std::optional<FileCheckNumericVariable *> &definedNumericVariable,
      bool isLegacyLineExpr, const SourceMgr &sourceMgr) const
{
   // Parse the numeric variable definition.
   definedNumericVariable = std::nullopt;
   size_t defEnd = expr.find(':');
   if (defEnd != StringRef::npos) {
      StringRef defExpr = expr.substr(0, defEnd);
      StringRef useExpr = expr.substr(defEnd + 1);

      useExpr = useExpr.ltrim(g_spaceChars);
      if (!useExpr.empty())
         return FileCheckErrorDiagnostic::get(
                  sourceMgr, useExpr,
                  "unexpected string after variable definition: '" + useExpr + "'");

      defExpr = defExpr.ltrim(g_spaceChars);
      Expected<FileCheckNumericVariable *> parseResult =
            parseNumericVariableDefinition(defExpr, m_context, m_lineNumber, sourceMgr);
      if (!parseResult) {
         return parseResult.takeError();
      }
      definedNumericVariable = *parseResult;
      return nullptr;
   }

   // Parse the expression itself.
   expr = expr.ltrim(g_spaceChars);
   // The first operand in a legacy @LINE expression is always the @LINE pseudo
   // variable.
   AllowedOperand allowedOperand =
         isLegacyLineExpr ? AllowedOperand::LineVar : AllowedOperand::Any;
   Expected<std::unique_ptr<FileCheckExpressionAST>> parseResult =
         parseNumericOperand(expr, allowedOperand, sourceMgr);
   while (parseResult && !expr.empty()) {
      parseResult =
            parseBinop(expr, std::move(*parseResult), isLegacyLineExpr, sourceMgr);
      // Legacy @LINE expressions only allow 2 operands.
      if (parseResult && isLegacyLineExpr && !expr.empty())
         return FileCheckErrorDiagnostic::get(
                  sourceMgr, expr,
                  "unexpected characters at end of expression '" + expr + "'");
   }
   if (!parseResult) {
      return parseResult;
   }
   return std::move(*parseResult);
}

/// Parses the given string into the Pattern.
///
/// \p prefix provides which prefix is being matched, \p sourceMgr provides the
/// SourceMgr used for error reports, and \p lineNumber is the line number in
/// the input file from which the pattern string was read. Returns true in
/// case of an error, false otherwise.
bool FileCheckPattern::parsePattern(StringRef patternStr, StringRef prefix,
                                    SourceMgr &sourceMgr,
                                    const FileCheckRequest &req)
{
   bool matchFullLinesHere = req.matchFullLines && m_checkType != check::CheckNot;

   m_patternLoc = SMLoc::getFromPointer(patternStr.data());

   if (!(req.noCanonicalizeWhiteSpace && req.matchFullLines)) {
      // Ignore trailing whitespace.
      while (!patternStr.empty() &&
             (patternStr.back() == ' ' || patternStr.back() == '\t'))
         patternStr = patternStr.substr(0, patternStr.size() - 1);
   }
   // Check that there is something on the line.
   if (patternStr.empty() && m_checkType != check::CheckEmpty) {
      sourceMgr.PrintMessage(m_patternLoc, SourceMgr::DK_Error,
                             "found empty check string with prefix '" + prefix + ":'");
      return true;
   }

   if (!patternStr.empty() && m_checkType == check::CheckEmpty) {
      sourceMgr.PrintMessage(
               m_patternLoc, SourceMgr::DK_Error,
               "found non-empty check string for empty check with prefix '" + prefix +
               ":'");
      return true;
   }

   if (m_checkType == check::CheckEmpty) {
      m_regExStr = "(\n$)";
      return false;
   }

   // Check to see if this is a fixed string, or if it has regex pieces.
   if (!matchFullLinesHere &&
       (patternStr.size() < 2 || (patternStr.find("{{") == StringRef::npos &&
                                  patternStr.find("[[") == StringRef::npos))) {
      m_fixedStr = patternStr;
      return false;
   }

   if (matchFullLinesHere) {
      m_regExStr += '^';
      if (!req.noCanonicalizeWhiteSpace) {
         m_regExStr += " *";
      }
   }

   // Paren value #0 is for the fully matched string.  Any new parenthesized
   // values add from there.
   unsigned curParen = 1;

   // Otherwise, there is at least one regex piece.  Build up the regex pattern
   // by escaping scary characters in fixed strings, building up one big regex.
   while (!patternStr.empty()) {
      // RegEx matches.
      if (patternStr.startswith("{{")) {
         // This is the Start of a regex match.  Scan for the }}.
         size_t end = patternStr.find("}}");
         if (end == StringRef::npos) {
            sourceMgr.PrintMessage(SMLoc::getFromPointer(patternStr.data()),
                                   SourceMgr::DK_Error,
                                   "found Start of regex string with no end '}}'");
            return true;
         }

         // Enclose {{}} patterns in parens just like [[]] even though we're not
         // capturing the result for any purpose.  This is required in case the
         // expression contains an alternation like: CHECK:  abc{{x|z}}def.  We
         // want this to turn into: "abc(x|z)def" not "abcx|zdef".
         m_regExStr += '(';
         ++curParen;
         if (addRegExToRegEx(patternStr.substr(2, end - 2), curParen, sourceMgr)) {
            return true;
         }
         m_regExStr += ')';

         patternStr = patternStr.substr(end + 2);
         continue;
      }

      // String and numeric substitution blocks. String substitution blocks come
      // in two forms: [[foo:.*]] and [[foo]]. The former matches .* (or some
      // other regex) and assigns it to the string variable 'foo'. The latter
      // substitutes foo's value. Numeric substitution blocks work the same way
      // as string ones, but Start with a '#' sign after the double brackets.
      // Both string and numeric variable names must satisfy the regular
      // expression "[a-zA-Z_][0-9a-zA-Z_]*" to be valid, as this helps catch
      // some common errors.
      if (patternStr.startswith("[[")) {
         StringRef unparsedPatternStr = patternStr.substr(2);
         // Find the closing bracket pair ending the match.  end is going to be an
         // offset relative to the beginning of the match string.
         size_t end = findRegexVarEnd(unparsedPatternStr, sourceMgr);
         StringRef matchStr = unparsedPatternStr.substr(0, end);
         bool isNumBlock = matchStr.consume_front("#");

         if (end == StringRef::npos) {
            sourceMgr.PrintMessage(SMLoc::getFromPointer(patternStr.data()),
                                   SourceMgr::DK_Error,
                                   "Invalid substitution block, no ]] found");
            return true;
         }
         // Strip the substitution block we are parsing. end points to the Start
         // of the "]]" closing the expression so account for it in computing the
         // index of the first unparsed character.
         patternStr = unparsedPatternStr.substr(end + 2);

         bool isDefinition = false;
         // Whether the substitution block is a legacy use of @LINE with string
         // substitution block syntax.
         bool isLegacyLineExpr = false;
         StringRef defName;
         StringRef substStr;
         StringRef matchRegexp;
         size_t substInsertIdx = m_regExStr.size();

         // Parse string variable or legacy @LINE expression.
         if (!isNumBlock) {
            size_t varEndIdx = matchStr.find(":");
            size_t spacePos = matchStr.substr(0, varEndIdx).find_first_of(" \t");
            if (spacePos != StringRef::npos) {
               sourceMgr.PrintMessage(SMLoc::getFromPointer(matchStr.data() + spacePos),
                                      SourceMgr::DK_Error, "unexpected whitespace");
               return true;
            }

            // Get the name (e.g. "foo") and verify it is well formed.
            StringRef origMatchStr = matchStr;
            Expected<FileCheckPattern::VariableProperties> parseVarResult =
                  parseVariable(matchStr, sourceMgr);
            if (!parseVarResult) {
               logAllUnhandledErrors(parseVarResult.takeError(), llvm::errs());
               return true;
            }
            StringRef name = parseVarResult->name;
            bool isPseudo = parseVarResult->isPseudo;

            isDefinition = (varEndIdx != StringRef::npos);
            if (isDefinition) {
               if ((isPseudo || !matchStr.consume_front(":"))) {
                  sourceMgr.PrintMessage(SMLoc::getFromPointer(name.data()),
                                         SourceMgr::DK_Error,
                                         "invalid name in string variable definition");
                  return true;
               }

               // Detect collisions between string and numeric variables when the
               // former is created later than the latter.
               if (m_context->m_globalNumericVariableTable.find(name) !=
                   m_context->m_globalNumericVariableTable.end()) {
                  sourceMgr.PrintMessage(
                           SMLoc::getFromPointer(name.data()), SourceMgr::DK_Error,
                           "numeric variable with name '" + name + "' already exists");
                  return true;
               }
               defName = name;
               matchRegexp = matchStr;
            } else {
               if (isPseudo) {
                  matchStr = origMatchStr;
                  isLegacyLineExpr = isNumBlock = true;
               } else
                  substStr = name;
            }
         }

         // Parse numeric substitution block.
         std::unique_ptr<FileCheckExpressionAST> expressionAST;
         std::optional<FileCheckNumericVariable *> definedNumericVariable;
         if (isNumBlock) {
            Expected<std::unique_ptr<FileCheckExpressionAST>> parseResult =
                  parseNumericSubstitutionBlock(matchStr, definedNumericVariable,
                                                isLegacyLineExpr, sourceMgr);
            if (!parseResult) {
               logAllUnhandledErrors(parseResult.takeError(), llvm::errs());
               return true;
            }
            expressionAST = std::move(*parseResult);
            if (definedNumericVariable) {
               isDefinition = true;
               defName = (*definedNumericVariable)->getName();
               matchRegexp = StringRef("[0-9]+");
            } else {
               substStr = matchStr;
            }
         }

         // Handle substitutions: [[foo]] and [[#<foo expr>]].
         if (!isDefinition) {
            // Handle substitution of string variables that were defined earlier on
            // the same line by emitting a backreference. Expressions do not
            // support substituting a numeric variable defined on the same line.
            if (!isNumBlock && m_variableDefs.find(substStr) != m_variableDefs.end()) {
               unsigned captureParenGroup = m_variableDefs[substStr];
               if (captureParenGroup < 1 || captureParenGroup > 9) {
                  sourceMgr.PrintMessage(SMLoc::getFromPointer(substStr.data()),
                                         SourceMgr::DK_Error,
                                         "Can't back-reference more than 9 variables");
                  return true;
               }
               addBackrefToRegEx(captureParenGroup);
            } else {
               // Handle substitution of string variables ([[<var>]]) defined in
               // previous CHECK patterns, and substitution of expressions.
               FileCheckSubstitution *substitution =
                     isNumBlock
                     ? m_context->makeNumericSubstitution(
                          substStr, std::move(expressionAST), substInsertIdx)
                     : m_context->makeStringSubstitution(substStr, substInsertIdx);
               m_substitutions.push_back(substitution);
            }
            continue;
         }

         // Handle variable definitions: [[<def>:(...)]] and
         // [[#(...)<def>:(...)]].
         if (isNumBlock) {
            FileCheckNumericVariableMatch numericVariableDefinition = {
               *definedNumericVariable, curParen};
            numericVariableDefs[defName] = numericVariableDefinition;
            // This store is done here rather than in match() to allow
            // parseNumericVariableUse() to get the pointer to the class instance
            // of the right variable definition corresponding to a given numeric
            // variable use.
            m_context->m_globalNumericVariableTable[defName] = *definedNumericVariable;
         } else {
            m_variableDefs[defName] = curParen;
            // Mark the string variable as defined to detect collisions between
            // string and numeric variables in parseNumericVariableUse() and
            // DefineCmdlineVariables() when the latter is created later than the
            // former. We cannot reuse globalVariableTable for this by populating
            // it with an empty string since we would then lose the ability to
            // detect the use of an undefined variable in match().
            m_context->m_definedVariableTable[defName] = true;
         }
         m_regExStr += '(';
         ++curParen;
         if (addRegExToRegEx(matchRegexp, curParen, sourceMgr)) {
            return true;
         }
         m_regExStr += ')';
      }

      // Handle fixed string matches.
      // Find the end, which is the Start of the next regex.
      size_t fixedMatchEnd = patternStr.find("{{");
      fixedMatchEnd = std::min(fixedMatchEnd, patternStr.find("[["));
      m_regExStr += regex_escape(patternStr.substr(0, fixedMatchEnd));
      patternStr = patternStr.substr(fixedMatchEnd);
   }

   if (matchFullLinesHere) {
      if (!req.noCanonicalizeWhiteSpace) {
         m_regExStr += " *";
      }
      m_regExStr += '$';
   }
   return false;
}

bool FileCheckPattern::addRegExToRegEx(StringRef regexStr, unsigned &curParen, SourceMgr &sourceMgr)
{
   try {
      std::string stdRegexStr = regexStr.str();
      boost::regex regex(stdRegexStr);
      m_regExStr += stdRegexStr;
      curParen += regex.mark_count();
      return false;
   } catch (boost::regex_error &e) {
      sourceMgr.PrintMessage(SMLoc::getFromPointer(regexStr.data()), SourceMgr::DK_Error,
                             "invalid regex: " + StringRef(e.what()));
      return true;
   }
}

void FileCheckPattern::addBackrefToRegEx(unsigned backrefNum)
{
   assert(backrefNum >= 1 && backrefNum <= 9 && "Invalid backref number");
   std::string backref = std::string("\\") + std::string(1, '0' + backrefNum);
   m_regExStr += backref;
}

Expected<size_t> FileCheckPattern::match(StringRef buffer, size_t &matchLen,
                                         const SourceMgr &sourceMgr) const
{
   // If this is the EOF pattern, match it immediately.
   if (m_checkType == check::CheckEOF) {
      matchLen = 0;
      return buffer.size();
   }

   // If this is a fixed string pattern, just match it now.
   if (!m_fixedStr.empty()) {
      matchLen = m_fixedStr.size();
      size_t pos = buffer.find(m_fixedStr);
      if (pos == StringRef::npos) {
         return make_error<FileCheckNotFoundError>();
      }
      return pos;
   }

   // Regex match.

   // If there are substitutions, we need to create a temporary string with the
   // actual value.
   StringRef regExToMatch = m_regExStr;
   std::string tmpStr;
   if (!m_substitutions.empty()) {
      tmpStr = m_regExStr;
      if (m_lineNumber) {
         m_context->m_lineVariable->setValue(*m_lineNumber);
      }
      size_t insertOffset = 0;
      // Substitute all string variables and expressions whose values are only
      // now known. Use of string variables defined on the same line are handled
      // by back-references.
      for (const auto &substitution : m_substitutions) {
         // Substitute and check for failure (e.g. use of undefined variable).
         Expected<std::string> value = substitution->getResult();
         if (!value) {
            m_context->m_lineVariable->clearValue();
            return value.takeError();
         }
         // Plop it into the regex at the adjusted offset.
         tmpStr.insert(tmpStr.begin() + substitution->getIndex() + insertOffset,
                       value->begin(), value->end());
         insertOffset += value->size();
      }

      // Match the newly constructed regex.
      regExToMatch = tmpStr;
      m_context->m_lineVariable->clearValue();
   }

   boost::cmatch matchInfo;
   if (!boost::regex_search(buffer.begin(), buffer.end(), matchInfo, boost::regex(regExToMatch.str()), boost::match_not_dot_newline)) {
      return make_error<FileCheckNotFoundError>();
   }

   // Successful regex match.
   assert(!matchInfo.empty() && "Didn't get any match");
   StringRef fullMatch(buffer.data() + matchInfo.position(), matchInfo[0].length());


   // If this defines any string variables, remember their values.
   for (const auto &variableDef : m_variableDefs) {
      assert(variableDef.second < matchInfo.size() && "Internal paren error");
      m_context->m_globalVariableTable[variableDef.first] = matchInfo[variableDef.second];
   }

   // If this defines any numeric variables, remember their values.
   for (const auto &numericVariableDef : numericVariableDefs) {
      const FileCheckNumericVariableMatch &numericVariableMatch =
            numericVariableDef.getValue();
      unsigned captureParenGroup = numericVariableMatch.captureParenGroup;
      assert(captureParenGroup < matchInfo.size() && "Internal paren error");
      FileCheckNumericVariable *definedNumericVariable =
            numericVariableMatch.definedNumericVariable;

      StringRef matchedValue = StringRef(buffer.data() + matchInfo.position(captureParenGroup), matchInfo[captureParenGroup].length());
      uint64_t val;
      if (matchedValue.getAsInteger(10, val)) {
         return FileCheckErrorDiagnostic::get(sourceMgr, matchedValue,
                                              "Unable to represent numeric value");
      }
      definedNumericVariable->setValue(val);
   }

   // Like CHECK-NEXT, CHECK-EMPTY's match range is considered to Start after
   // the required preceding newline, which is consumed by the pattern in the
   // case of CHECK-EMPTY but not CHECK-NEXT.
   size_t matchStartSkip = m_checkType == check::CheckEmpty;
   matchLen = fullMatch.size() - matchStartSkip;
   return fullMatch.data() - buffer.data() + matchStartSkip;
}

unsigned
FileCheckPattern::computeMatchDistance(StringRef buffer) const
{
   // Just compute the number of matching characters. For regular expressions, we
   // just compare against the regex itself and hope for the best.
   //
   // FIXME: One easy improvement here is have the regex lib generate a single
   // example regular expression which matches, and use that as the example
   // string.
   StringRef exampleString(m_fixedStr);
   if (exampleString.empty()) {
      exampleString = m_regExStr;
   }

   // Only compare up to the first line in the buffer, or the string size.
   StringRef bufferPrefix = buffer.substr(0, exampleString.size());
   bufferPrefix = bufferPrefix.split('\n').first;
   return bufferPrefix.edit_distance(exampleString);
}


void FileCheckPattern::printSubstitutions(const SourceMgr &sourceMgr, StringRef buffer,
                                          SMRange matchRange) const
{
   // Print what we know about substitutions.
   if (!m_substitutions.empty()) {
      for (const auto &substitution : m_substitutions) {
         SmallString<256> msg;
         raw_svector_ostream outStream(msg);
         Expected<std::string> matchedValue = substitution->getResult();

         // substitution failed or is not known at match time, print the undefined
         // variables it uses.
         if (!matchedValue) {
            bool undefSeen = false;
            handleAllErrors(matchedValue.takeError(),
                              [](const FileCheckNotFoundError &) {},
            // Handled in PrintNoMatch().
            [](const FileCheckErrorDiagnostic &) {},
            [&](const FileCheckUndefVarError &E) {
               if (!undefSeen) {
                  outStream << "uses undefined variable(s):";
                  undefSeen = true;
               }
               outStream << " ";
               E.log(outStream);
            });
         } else {
            // substitution succeeded. Print substituted value.
            outStream << "with \"";
            outStream.write_escaped(substitution->getFromString()) << "\" equal to \"";
            outStream.write_escaped(*matchedValue) << "\"";
         }

         if (matchRange.isValid())
            sourceMgr.PrintMessage(matchRange.Start, SourceMgr::DK_Note, outStream.str(),
            {matchRange});
         else
            sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.data()),
                                   SourceMgr::DK_Note, outStream.str());
      }
   }
}

static SMRange process_match_result(FileCheckDiag::MatchType matchType,
                                    const SourceMgr &sourceMgr, SMLoc loc,
                                    FileCheckType checkType,
                                    StringRef buffer, size_t pos, size_t len,
                                    std::vector<FileCheckDiag> *diags,
                                    bool adjustPrevDiag = false)
{
   SMLoc Start = SMLoc::getFromPointer(buffer.data() + pos);
   SMLoc end = SMLoc::getFromPointer(buffer.data() + pos + len);
   SMRange range(Start, end);
   if (diags) {
      if (adjustPrevDiag) {
         diags->rbegin()->matchType = matchType;
      } else {
         diags->emplace_back(sourceMgr, checkType, loc, matchType, range);
      }
   }
   return range;
}

void FileCheckPattern::printFuzzyMatch(
      const SourceMgr &sourceMgr, StringRef buffer,
      std::vector<FileCheckDiag> *diags) const
{
   // Attempt to find the closest/best fuzzy match.  Usually an error happens
   // because some string in the output didn't exactly match. In these cases, we
   // would like to show the user a best guess at what "should have" matched, to
   // save them having to actually check the input manually.
   size_t numLinesForward = 0;
   size_t best = StringRef::npos;
   double bestQuality = 0;

   // Use an arbitrary 4k limit on how far we will search.
   for (size_t i = 0, e = std::min(size_t(4096), buffer.size()); i != e; ++i) {
      if (buffer[i] == '\n') {
         ++numLinesForward;
      }
      // Patterns have leading whitespace stripped, so skip whitespace when
      // looking for something which looks like a pattern.
      if (buffer[i] == ' ' || buffer[i] == '\t')
         continue;

      // Compute the "quality" of this match as an arbitrary combination of the
      // match distance and the number of lines skipped to get to this match.
      unsigned distance = computeMatchDistance(buffer.substr(i));
      double quality = distance + (numLinesForward / 100.);

      if (quality < bestQuality || best == StringRef::npos) {
         best = i;
         bestQuality = quality;
      }
   }

   // Print the "possible intended match here" line if we found something
   // reasonable and not equal to what we showed in the "scanning from here"
   // line.
   if (best && best != StringRef::npos && bestQuality < 50) {
      SMRange matchRange =
            process_match_result(FileCheckDiag::MatchFuzzy, sourceMgr, getLoc(),
                                 getCheckType(), buffer, best, 0, diags);
      sourceMgr.PrintMessage(matchRange.Start, SourceMgr::DK_Note,
                             "possible intended match here");

      // FIXME: If we wanted to be really friendly we would show why the match
      // failed, as it can be hard to spot simple one character differences.
   }
}

Expected<StringRef>
FileCheckPatternContext::getPatternVarValue(StringRef varName)
{
   auto varIter = m_globalVariableTable.find(varName);
   if (varIter == m_globalVariableTable.end()) {
      return make_error<FileCheckUndefVarError>(varName);
   }
   return varIter->second;
}

template <typename... Types>
FileCheckNumericVariable *
FileCheckPatternContext::makeNumericVariable(Types... args)
{
   m_numericVariables.push_back(
            std::make_unique<FileCheckNumericVariable>(args...));
   return m_numericVariables.back().get();
}

FileCheckSubstitution *
FileCheckPatternContext::makeStringSubstitution(StringRef varName,
                                                size_t insertIdx)
{
   m_substitutions.push_back(
            std::make_unique<FileCheckStringSubstitution>(this, varName, insertIdx));
   return m_substitutions.back().get();
}


FileCheckSubstitution *FileCheckPatternContext::makeNumericSubstitution(
      StringRef expressionStr,
      std::unique_ptr<FileCheckExpressionAST> expressionAST, size_t insertIdx)
{
   m_substitutions.push_back(std::make_unique<FileCheckNumericSubstitution>(
                                this, expressionStr, std::move(expressionAST), insertIdx));
   return m_substitutions.back().get();
}

/// Finds the closing sequence of a regex variable usage or definition.
///
/// \p str has to point in the beginning of the definition (right after the
/// opening sequence). Returns the offset of the closing sequence within str,
/// or npos if it was not found.
size_t FileCheckPattern::findRegexVarEnd(StringRef str, SourceMgr &sourceMgr)
{
   // offset keeps track of the current offset within the input str
   size_t offset = 0;
   // [...] Nesting depth
   size_t bracketDepth = 0;

   while (!str.empty()) {
      if (str.startswith("]]") && bracketDepth == 0)
         return offset;
      if (str[0] == '\\') {
         // Backslash escapes the next char within regexes, so skip them both.
         str = str.substr(2);
         offset += 2;
      } else {
         switch (str[0]) {
         default:
            break;
         case '[':
            bracketDepth++;
            break;
         case ']':
            if (bracketDepth == 0) {
               sourceMgr.PrintMessage(SMLoc::getFromPointer(str.data()),
                                      SourceMgr::DK_Error,
                                      "missing closing \"]\" for regex variable");
               exit(1);
            }
            bracketDepth--;
            break;
         }
         str = str.substr(1);
         offset++;
      }
   }
   return StringRef::npos;
}

StringRef FileCheck::canonicalizeFile(MemoryBuffer &memoryBuffer,
                                      SmallVectorImpl<char> &outputBuffer)
{
   outputBuffer.reserve(memoryBuffer.getBufferSize());
   for (const char *ptr = memoryBuffer.getBufferStart(), *end = memoryBuffer.getBufferEnd();
        ptr != end; ++ptr) {
      // Eliminate trailing dosish \r.
      if (ptr <= end - 2 && ptr[0] == '\r' && ptr[1] == '\n') {
         continue;
      }

      // If current char is not a horizontal whitespace or if horizontal
      // whitespace canonicalization is disabled, dump it to output as is.
      if (m_req.noCanonicalizeWhiteSpace || (*ptr != ' ' && *ptr != '\t')) {
         outputBuffer.push_back(*ptr);
         continue;
      }

      // Otherwise, add one space and advance over neighboring space.
      outputBuffer.push_back(' ');
      while (ptr + 1 != end && (ptr[1] == ' ' || ptr[1] == '\t')) {
         ++ptr;
      }
   }

   // Add a null byte and then return all but that byte.
   outputBuffer.push_back('\0');
   return StringRef(outputBuffer.data(), outputBuffer.size() - 1);
}

FileCheckDiag::FileCheckDiag(const SourceMgr &sourceMgr,
                             const FileCheckType &checkType,
                             SMLoc checkLoc, MatchType matchType,
                             SMRange inputRange)
   : checkType(checkType),
     matchType(matchType)
{
   auto Start = sourceMgr.getLineAndColumn(inputRange.Start);
   auto end = sourceMgr.getLineAndColumn(inputRange.End);
   inputStartLine = Start.first;
   inputStartCol = Start.second;
   inputEndLine = end.first;
   inputEndCol = end.second;
   Start = sourceMgr.getLineAndColumn(checkLoc);
   checkLine = Start.first;
   checkCol = Start.second;
}

static bool is_part_of_word(char c)
{
   return (isalnum(c) || c == '-' || c == '_');
}

FileCheckType &FileCheckType::setCount(int count)
{
   assert(count > 0 && "zero and negative counts are not supported");
   assert((count == 1 || m_kind == CheckPlain) &&
          "count supported only for plain CHECK directives");
   m_count = count;
   return *this;
}

std::string FileCheckType::getDescription(StringRef prefix) const
{
   switch (m_kind) {
   case FileCheckKind::CheckNone:
      return "invalid";
   case FileCheckKind::CheckPlain:
      if (m_count > 1) {
         return prefix.str() + "-COUNT";
      }
      return prefix;
   case FileCheckKind::CheckNext:
      return prefix.str() + "-NEXT";
   case FileCheckKind::CheckSame:
      return prefix.str() + "-SAME";
   case FileCheckKind::CheckNot:
      return prefix.str() + "-NOT";
   case FileCheckKind::CheckDAG:
      return prefix.str() + "-DAG";
   case FileCheckKind::CheckLabel:
      return prefix.str() + "-LABEL";
   case FileCheckKind::CheckEmpty:
      return prefix.str() + "-EMPTY";
   case FileCheckKind::CheckEOF:
      return "implicit EOF";
   case FileCheckKind::CheckBadNot:
      return "bad NOT";
   case FileCheckKind::CheckBadCount:
      return "bad COUNT";
   }
   llvm_unreachable("unknown FileCheckType");
}

static std::pair<check::FileCheckType, StringRef>
find_check_type(StringRef buffer, StringRef prefix)
{
   if (buffer.size() <= prefix.size()) {
      return {check::FileCheckKind::CheckNone, StringRef()};
   }
   char nextChar = buffer[prefix.size()];

   StringRef rest = buffer.drop_front(prefix.size() + 1);
   // Verify that the : is present after the prefix.
   if (nextChar == ':') {
      return {check::FileCheckKind::CheckPlain, rest};
   }
   if (nextChar != '-') {
      return {check::FileCheckKind::CheckNone, StringRef()};
   }

   if (rest.consume_front("COUNT-")) {
      int64_t count;
      if (rest.consumeInteger(10, count)) {
         // Error happened in parsing integer.
         return {FileCheckKind::CheckBadCount, rest};
      }
      if (count <= 0 || count > INT32_MAX) {
         return {FileCheckKind::CheckBadCount, rest};
      }

      if (!rest.consume_front(":")) {
         return {FileCheckKind::CheckBadCount, rest};
      }
      return {FileCheckType(FileCheckKind::CheckPlain).setCount(count), rest};
   }

   if (rest.consume_front("NEXT:"))
      return {FileCheckKind::CheckNext, rest};

   if (rest.consume_front("SAME:"))
      return {FileCheckKind::CheckSame, rest};

   if (rest.consume_front("NOT:"))
      return {FileCheckKind::CheckNot, rest};

   if (rest.consume_front("DAG:"))
      return {FileCheckKind::CheckDAG, rest};

   if (rest.consume_front("LABEL:"))
      return {FileCheckKind::CheckLabel, rest};

   if (rest.consume_front("EMPTY:"))
      return {FileCheckKind::CheckEmpty, rest};

   // You can't combine -NOT with another suffix.
   if (rest.startswith("DAG-NOT:") || rest.startswith("NOT-DAG:") ||
       rest.startswith("NEXT-NOT:") || rest.startswith("NOT-NEXT:") ||
       rest.startswith("SAME-NOT:") || rest.startswith("NOT-SAME:") ||
       rest.startswith("EMPTY-NOT:") || rest.startswith("NOT-EMPTY:")) {
      return {FileCheckKind::CheckBadNot, rest};
   }

   return {FileCheckKind::CheckNone, rest};
}

// From the given position, find the next character after the word.
static size_t skip_word(StringRef str, size_t loc)
{
   while (loc < str.size() && is_part_of_word(str[loc])) {
      ++loc;
   }
   return loc;
}

/// Searches the buffer for the first prefix in the prefix regular expression.
///
/// This searches the buffer using the provided regular expression, however it
/// enforces constraints beyond that:
/// 1) The found prefix must not be a suffix of something that looks like
///    a valid prefix.
/// 2) The found prefix must be followed by a valid check type suffix using \c
///    find_check_type above.
///
/// \returns a pair of StringRefs into the buffer, which combines:
///   - the first match of the regular expression to satisfy these two is
///   returned,
///     otherwise an empty StringRef is returned to indicate failure.
///   - buffer rewound to the location right after parsed suffix, for parsing
///     to continue from
///
/// If this routine returns a valid prefix, it will also shrink \p buffer to
/// Start at the beginning of the returned prefix, increment \p lineNumber for
/// each new line consumed from \p buffer, and set \p checkType to the type of
/// check found by examining the suffix.
///
/// If no valid prefix is found, the state of buffer, lineNumber, and checkType
/// is unspecified.

static std::pair<StringRef, StringRef>
find_first_matching_prefix(boost::regex &prefixRegex, StringRef &buffer,
                           unsigned &lineNumber,
                           FileCheckType &checkType)
{
   while (!buffer.empty()) {
      boost::cmatch matches;
      // Find the first (longest) match using the RE.
      if (!boost::regex_search(buffer.begin(), buffer.end(), matches, prefixRegex,
                               boost::match_posix)) {
         // No match at all, bail.
         return {StringRef(), StringRef()};
      }
      StringRef prefix(buffer.data() + matches.position(), matches[0].length());
      assert(prefix.data() >= buffer.data() &&
             prefix.data() < buffer.data() + buffer.size() &&
             "prefix doesn't Start inside of buffer!");

      size_t loc = matches.position();
      StringRef skipped = buffer.substr(0, loc);
      buffer = buffer.drop_front(loc);
      lineNumber += skipped.count('\n');

      // Check that the matched prefix isn't a suffix of some other check-like
      // word.
      // FIXME: This is a very ad-hoc check. it would be better handled in some
      // other way. Among other things it seems hard to distinguish between
      // intentional and unintentional uses of this feature.
      if (skipped.empty() || !is_part_of_word(skipped.back())) {
         // Now extract the type.
         StringRef afterSuffix;
         std::tie(checkType, afterSuffix) = find_check_type(buffer, prefix);

         // If we've found a valid check type for this prefix, we're done.
         if (checkType != FileCheckKind::CheckNone) {
            return {prefix, afterSuffix};
         }
      }

      // If we didn't successfully find a prefix, we need to skip this invalid
      // prefix and continue scanning. We directly skip the prefix that was
      // matched and any additional parts of that check-like word.
      buffer = buffer.drop_front(skip_word(buffer, prefix.size()));
   }

   // We ran out of buffer while skipping partial matches so give up.
   return {StringRef(), StringRef()};
}

void FileCheckPatternContext::createLineVariable()
{
   assert(!m_lineVariable && "@LINE pseudo numeric variable already created");
   StringRef LineName = "@LINE";
   m_lineVariable = makeNumericVariable(LineName);
   m_globalNumericVariableTable[LineName] = m_lineVariable;
}

/// Read the check file, which specifies the sequence of expected strings.
///
/// The strings are added to the checkStrings vector. Returns true in case of
/// an error, false otherwise.
bool FileCheck::readCheckFile(SourceMgr &sourceMgr, StringRef buffer,
                              boost::regex &prefixRE,
                              std::vector<FileCheckString> &checkStrings)
{
   Error defineError =
         m_patternContext.defineCmdlineVariables(m_req.globalDefines, sourceMgr);
   if (defineError) {
      logAllUnhandledErrors(std::move(defineError), llvm::errs());
      return true;
   }

   m_patternContext.createLineVariable();

   std::vector<FileCheckPattern> implicitNegativeChecks;
   for (const auto &patternString : m_req.implicitCheckNot) {
      // Create a buffer with fake command line content in order to display the
      // command line option responsible for the specific implicit CHECK-NOT.
      std::string prefix = "--implicit-check-not='";
      std::string Suffix = "'";
      std::unique_ptr<MemoryBuffer> cmdLine = MemoryBuffer::getMemBufferCopy(
               prefix + patternString + Suffix, "command line");

      StringRef patternInBuffer =
            cmdLine->getBuffer().substr(prefix.size(), patternString.size());
      sourceMgr.AddNewSourceBuffer(std::move(cmdLine), SMLoc());

      implicitNegativeChecks.push_back(
               FileCheckPattern(check::CheckNot, &m_patternContext));
      implicitNegativeChecks.back().parsePattern(patternInBuffer,
                                                 "IMPLICIT-CHECK", sourceMgr, m_req);
   }

   std::vector<FileCheckPattern> dagNotMatches = implicitNegativeChecks;

   // lineNumber keeps track of the line on which CheckPrefix instances are
   // found.
   unsigned lineNumber = 1;

   while (1) {
      FileCheckType checkType;

      // See if a prefix occurs in the memory buffer.
      StringRef usedPrefix;
      StringRef afterSuffix;
      std::tie(usedPrefix, afterSuffix) =
            find_first_matching_prefix(prefixRE, buffer, lineNumber, checkType);
      if (usedPrefix.empty())
         break;
      assert(usedPrefix.data() == buffer.data() &&
             "Failed to move buffer's Start forward, or pointed prefix outside "
             "of the buffer!");
      assert(afterSuffix.data() >= buffer.data() &&
             afterSuffix.data() < buffer.data() + buffer.size() &&
             "Parsing after suffix doesn't Start inside of buffer!");

      // Location to use for error messages.
      const char *usedPrefixStart = usedPrefix.data();

      // Skip the buffer to the end of parsed suffix (or just prefix, if no good
      // suffix was processed).
      buffer = afterSuffix.empty() ? buffer.drop_front(usedPrefix.size())
                                   : afterSuffix;

      // Complain about useful-looking but unsupported suffixes.
      if (checkType == check::CheckBadNot) {
         sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.data()), SourceMgr::DK_Error,
                                "unsupported -NOT combo on prefix '" + usedPrefix + "'");
         return true;
      }

      // Complain about invalid count specification.
      if (checkType == check::CheckBadCount) {
         sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.data()), SourceMgr::DK_Error,
                                "invalid count in -COUNT specification on prefix '" +
                                usedPrefix + "'");
         return true;
      }

      // Okay, we found the prefix, yay. Remember the rest of the line, but ignore
      // leading whitespace.
      if (!(m_req.noCanonicalizeWhiteSpace && m_req.matchFullLines)) {
         buffer = buffer.substr(buffer.find_first_not_of(" \t"));
      }
      // Scan ahead to the end of line.
      size_t eol = buffer.find_first_of("\n\r");

      // Remember the location of the Start of the pattern, for diagnostics.
      SMLoc patternLoc = SMLoc::getFromPointer(buffer.data());

      // Parse the pattern.
      FileCheckPattern P(checkType, &m_patternContext, lineNumber);
      if (P.parsePattern(buffer.substr(0, eol), usedPrefix, sourceMgr, m_req))
         return true;

      // Verify that CHECK-LABEL lines do not define or use variables
      if ((checkType == check::CheckLabel) && P.hasVariable()) {
         sourceMgr.PrintMessage(
                  SMLoc::getFromPointer(usedPrefixStart), SourceMgr::DK_Error,
                  "found '" + usedPrefix + "-LABEL:'"
                                           " with variable definition or use");
         return true;
      }

      buffer = buffer.substr(eol);

      // Verify that CHECK-NEXT/SAME/EMPTY lines have at least one CHECK line before them.
      if ((checkType == check::CheckNext || checkType == check::CheckSame ||
           checkType == check::CheckEmpty) &&
          checkStrings.empty()) {
         StringRef Type = checkType == check::CheckNext
               ? "NEXT"
               : checkType == check::CheckEmpty ? "EMPTY" : "SAME";
         sourceMgr.PrintMessage(SMLoc::getFromPointer(usedPrefixStart),
                                SourceMgr::DK_Error,
                                "found '" + usedPrefix + "-" + Type +
                                "' without previous '" + usedPrefix + ": line");
         return true;
      }

      // Handle CHECK-DAG/-NOT.
      if (checkType == check::CheckDAG || checkType == check::CheckNot) {
         dagNotMatches.push_back(P);
         continue;
      }

      // Okay, add the string we captured to the output vector and move on.
      checkStrings.emplace_back(P, usedPrefix, patternLoc);
      std::swap(dagNotMatches, checkStrings.back().m_dagNotStrings);
      dagNotMatches = implicitNegativeChecks;
   }

   // Add an EOF pattern for any trailing CHECK-DAG/-NOTs, and use the first
   // prefix as a filler for the error message.
   if (!dagNotMatches.empty()) {
      checkStrings.emplace_back(
               FileCheckPattern(check::CheckEOF, &m_patternContext, lineNumber + 1),
               *m_req.checkPrefixes.begin(), SMLoc::getFromPointer(buffer.data()));
      std::swap(dagNotMatches, checkStrings.back().m_dagNotStrings);
   }

   if (checkStrings.empty()) {
      llvm::errs() << "error: no check strings found with prefix"
                     << (m_req.checkPrefixes.size() > 1 ? "es " : " ");
      auto I = m_req.checkPrefixes.begin();
      auto E = m_req.checkPrefixes.end();
      if (I != E) {
         llvm::errs() << "\'" << *I << ":'";
         ++I;
      }
      for (; I != E; ++I) {
         llvm::errs() << ", \'" << *I << ":'";
      }


      llvm::errs() << '\n';
      return true;
   }

   return false;
}

static void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                        StringRef prefix, SMLoc loc, const FileCheckPattern &pattern,
                        int matchedCount, StringRef buffer, size_t matchPos,
                        size_t matchLen, const FileCheckRequest &req,
                        std::vector<FileCheckDiag> *diags)
{
   bool printDiag = true;
   if (expectedMatch) {
      if (!req.verbose) {
         return;
      }
      if (!req.verboseVerbose && pattern.getCheckType() == check::CheckEOF) {
         return;
      }
      // Due to their verbosity, we don't print verbose diagnostics here if we're
      // gathering them for a different rendering, but we always print other
      // diagnostics.
      printDiag = !diags;
   }
   SMRange MatchRange = process_match_result(
            expectedMatch ? FileCheckDiag::MatchFoundAndExpected
                          : FileCheckDiag::MatchFoundButExcluded,
            sourceMgr, loc, pattern.getCheckType(), buffer, matchPos, matchLen, diags);
   if (!printDiag) {
      return;
   }

   std::string message = formatv("{0}: {1} string found in input",
                                 pattern.getCheckType().getDescription(prefix),
                                 (expectedMatch ? "expected" : "excluded"))
         .str();
   if (pattern.getCount() > 1)
      message += formatv(" ({0} out of {1})", matchedCount, pattern.getCount()).str();

   sourceMgr.PrintMessage(
            loc, expectedMatch ? SourceMgr::DK_Remark : SourceMgr::DK_Error, message);
   sourceMgr.PrintMessage(MatchRange.Start, SourceMgr::DK_Note, "found here",
   {MatchRange});
   pattern.printSubstitutions(sourceMgr, buffer, MatchRange);
}

static void print_match(bool expectedMatch, const SourceMgr &sourceMgr,
                        const FileCheckString &checkStr, int matchedCount,
                        StringRef buffer, size_t matchPos, size_t matchLen,
                        FileCheckRequest &req, std::vector<FileCheckDiag> *diags)
{
   print_match(expectedMatch, sourceMgr, checkStr.prefix, checkStr.loc, checkStr.pattern,
               matchedCount, buffer, matchPos, matchLen, req,
               diags);
}

static void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                           StringRef prefix, SMLoc loc,
                           const FileCheckPattern &pattern, int matchedCount,
                           StringRef buffer, bool verboseVerbose,
                           std::vector<FileCheckDiag> *diags, Error matchErrors)
{
   assert(matchErrors && "Called on successful match");
   bool printDiag = true;
   if (!expectedMatch) {
      if (!verboseVerbose) {
         consumeError(std::move(matchErrors));
         return;
      }
      // Due to their verbosity, we don't print verbose diagnostics here if we're
      // gathering them for a different rendering, but we always print other
      // diagnostics.
      printDiag = !diags;
   }

   // If the current position is at the end of a line, advance to the Start of
   // the next line.
   buffer = buffer.substr(buffer.find_first_not_of(" \t\n\r"));
   SMRange SearchRange = process_match_result(
            expectedMatch ? FileCheckDiag::MatchNoneButExpected
                          : FileCheckDiag::MatchNoneAndExcluded,
            sourceMgr, loc, pattern.getCheckType(), buffer, 0, buffer.size(), diags);
   if (!printDiag) {
      consumeError(std::move(matchErrors));
      return;
   }

   matchErrors =
         handleErrors(std::move(matchErrors),
                      [](const FileCheckErrorDiagnostic &E) { E.log(llvm::errs()); });

   // No problem matching the string per se.
   if (!matchErrors) {
      return;
   }

   consumeError(std::move(matchErrors));

   // Print "not found" diagnostic.
   std::string message = formatv("{0}: {1} string not found in input",
                                 pattern.getCheckType().getDescription(prefix),
                                 (expectedMatch ? "expected" : "excluded"))
         .str();
   if (pattern.getCount() > 1) {
      message += formatv(" ({0} out of {1})", matchedCount, pattern.getCount()).str();
   }
   sourceMgr.PrintMessage(
            loc, expectedMatch ? SourceMgr::DK_Error : SourceMgr::DK_Remark, message);

   // Print the "scanning from here" line.
   sourceMgr.PrintMessage(SearchRange.Start, SourceMgr::DK_Note, "scanning from here");
   // Allow the pattern to print additional information if desired.
   pattern.printSubstitutions(sourceMgr, buffer);
   if (expectedMatch) {
      pattern.printFuzzyMatch(sourceMgr, buffer, diags);
   }

}

static void print_no_match(bool expectedMatch, const SourceMgr &sourceMgr,
                           const FileCheckString &checkStr, int matchedCount,
                           StringRef buffer, bool verboseVerbose, std::vector<FileCheckDiag> *diags,
                           Error matchErrors)
{
   print_no_match(expectedMatch, sourceMgr, checkStr.prefix, checkStr.loc, checkStr.pattern,
                  matchedCount, buffer, verboseVerbose, diags,
                  std::move(matchErrors));
}

// Counts the number of newlines in the specified range.
static unsigned count_num_newlines_between(StringRef range,
                                           const char *&firstNewLine)
{
   unsigned numNewLines = 0;
   while (1) {
      // Scan for newline.
      range = range.substr(range.find_first_of("\n\r"));
      if (range.empty()) {
         return numNewLines;
      }

      ++numNewLines;

      // Handle \n\r and \r\n as a single newline.
      if (range.size() > 1 && (range[1] == '\n' || range[1] == '\r') &&
          (range[0] != range[1])) {
         range = range.substr(1);
      }
      range = range.substr(1);
      if (numNewLines == 1) {
         firstNewLine = range.begin();
      }
   }
}

/// Match check string and its "not strings" and/or "dag strings".
size_t FileCheckString::check(const SourceMgr &sourceMgr, StringRef buffer,
                              bool isLabelScanMode, size_t &matchLen,
                              FileCheckRequest &req, std::vector<FileCheckDiag> *diags) const
{
   size_t lastPos = 0;
   std::vector<const FileCheckPattern *> notStrings;

   // isLabelScanMode is true when we are scanning forward to find CHECK-LABEL
   // bounds; we have not processed variable definitions within the bounded block
   // yet so cannot handle any final CHECK-DAG yet; this is handled when going
   // over the block again (including the last CHECK-LABEL) in normal mode.
   if (!isLabelScanMode) {
      // Match "dag strings" (with mixed "not strings" if any).
      lastPos = checkDag(sourceMgr, buffer, notStrings, req, diags);
      if (lastPos == StringRef::npos) {
         return StringRef::npos;
      }
   }

   // Match itself from the last position after matching CHECK-DAG.
   size_t lastMatchEnd = lastPos;
   size_t firstMatchPos = 0;
   // Go match the pattern Count times. Majority of patterns only match with
   // count 1 though.
   assert(pattern.getCount() != 0 && "pattern count can not be zero");
   for (int i = 1; i <= pattern.getCount(); i++) {
      StringRef matchBuffer = buffer.substr(lastMatchEnd);
      size_t CurrentMatchLen;
      // get a match at current Start point
      Expected<size_t> matchResult = pattern.match(matchBuffer, CurrentMatchLen, sourceMgr);

      // report
      if (!matchResult) {
         print_no_match(true, sourceMgr, *this, i, matchBuffer, req.verboseVerbose, diags,
                        matchResult.takeError());
         return StringRef::npos;
      }
      size_t matchPos = *matchResult;
      print_match(true, sourceMgr, *this, i, matchBuffer, matchPos, CurrentMatchLen, req,
                  diags);
      if (i == 1) {
         firstMatchPos = lastPos + matchPos;
      }
      // move Start point after the match
      lastMatchEnd += matchPos + CurrentMatchLen;
   }
   // Full match len counts from first match pos.
   matchLen = lastMatchEnd - firstMatchPos;

   // Similar to the above, in "label-scan mode" we can't yet handle CHECK-NEXT
   // or CHECK-NOT
   if (!isLabelScanMode) {
      size_t matchPos = firstMatchPos - lastPos;
      StringRef matchBuffer = buffer.substr(lastPos);
      StringRef skippedRegion = buffer.substr(lastPos, matchPos);

      // If this check is a "CHECK-NEXT", verify that the previous match was on
      // the previous line (i.e. that there is one newline between them).
      if (checkNext(sourceMgr, skippedRegion)) {
         process_match_result(FileCheckDiag::MatchFoundButWrongLine, sourceMgr, loc,
                              pattern.getCheckType(), matchBuffer, matchPos, matchLen,
                              diags, req.verbose);
         return StringRef::npos;
      }

      // If this check is a "CHECK-SAME", verify that the previous match was on
      // the same line (i.e. that there is no newline between them).
      if (checkSame(sourceMgr, skippedRegion)) {
         process_match_result(FileCheckDiag::MatchFoundButWrongLine, sourceMgr, loc,
                              pattern.getCheckType(), matchBuffer, matchPos, matchLen,
                              diags, req.verbose);
         return StringRef::npos;
      }

      // If this match had "not strings", verify that they don't exist in the
      // skipped region.
      if (checkNot(sourceMgr, skippedRegion, notStrings, req, diags)) {
         return StringRef::npos;
      }
   }

   return firstMatchPos;
}

bool FileCheckString::checkNext(const SourceMgr &sourceMgr, StringRef buffer) const
{
   if (pattern.getCheckType() != check::CheckNext &&
       pattern.getCheckType() != check::CheckEmpty)
      return false;

   Twine checkName =
         prefix +
         Twine(pattern.getCheckType() == check::CheckEmpty ? "-EMPTY" : "-NEXT");

   // Count the number of newlines between the previous match and this one.
   const char *firstNewLine = nullptr;
   unsigned numNewLines = count_num_newlines_between(buffer, firstNewLine);

   if (numNewLines == 0) {
      sourceMgr.PrintMessage(loc, SourceMgr::DK_Error,
                             checkName + ": is on the same line as previous match");
      sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.end()), SourceMgr::DK_Note,
                             "'next' match was here");
      sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.data()), SourceMgr::DK_Note,
                             "previous match ended here");
      return true;
   }

   if (numNewLines != 1) {
      sourceMgr.PrintMessage(loc, SourceMgr::DK_Error,
                             checkName +
                             ": is not on the line after the previous match");
      sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.end()), SourceMgr::DK_Note,
                             "'next' match was here");
      sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.data()), SourceMgr::DK_Note,
                             "previous match ended here");
      sourceMgr.PrintMessage(SMLoc::getFromPointer(firstNewLine), SourceMgr::DK_Note,
                             "non-matching line after previous match is here");
      return true;
   }
   return false;
}

bool FileCheckString::checkSame(const SourceMgr &sourceMgr, StringRef buffer) const
{
   if (pattern.getCheckType() != check::CheckSame) {
      return false;
   }

   // Count the number of newlines between the previous match and this one.
   const char *FirstNewLine = nullptr;
   unsigned numNewLines = count_num_newlines_between(buffer, FirstNewLine);

   if (numNewLines != 0) {
      sourceMgr.PrintMessage(loc, SourceMgr::DK_Error,
                             prefix +
                             "-SAME: is not on the same line as the previous match");
      sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.end()), SourceMgr::DK_Note,
                             "'next' match was here");
      sourceMgr.PrintMessage(SMLoc::getFromPointer(buffer.data()), SourceMgr::DK_Note,
                             "previous match ended here");
      return true;
   }
   return false;
}

/// Verify there's no "not strings" in the given buffer.
bool FileCheckString::checkNot(
      const SourceMgr &sourceMgr, StringRef buffer,
      const std::vector<const FileCheckPattern *> &notStrings,
      const FileCheckRequest &req, std::vector<FileCheckDiag> *diags) const
{
   for (const FileCheckPattern *pattern : notStrings) {
      assert((pattern->getCheckType() == check::CheckNot) && "Expect CHECK-NOT!");
      size_t matchLen = 0;
      Expected<size_t> matchResult = pattern->match(buffer, matchLen, sourceMgr);

      if (!matchResult) {
         print_no_match(false, sourceMgr, prefix, pattern->getLoc(), *pattern, 1, buffer,
                        req.verboseVerbose, diags, matchResult.takeError());
         continue;
      }
      size_t pos = *matchResult;

      print_match(false, sourceMgr, prefix, pattern->getLoc(), *pattern, 1, buffer, pos, matchLen,
                  req, diags);
      return true;
   }
   return false;
}

/// Match "dag strings" and their mixed "not strings".
size_t
FileCheckString::checkDag(const SourceMgr &sourceMgr, StringRef buffer,
                          std::vector<const FileCheckPattern *> &notStrings,
                          const FileCheckRequest &req, std::vector<FileCheckDiag> *diags) const
{
   if (m_dagNotStrings.empty()) {
      return 0;
   }
   // The Start of the search range.
   size_t startPos = 0;

   struct MatchRange {
      size_t pos;
      size_t End;
   };
   // A sorted list of ranges for non-overlapping CHECK-DAG matches.  Match
   // ranges are erased from this list once they are no longer in the search
   // range.
   std::list<MatchRange> matchRanges;

   // We need pattenIter and pattenIterEnd later for detecting the end of a CHECK-DAG
   // group, so we don't use a range-based for loop here.
   for (auto pattenIter = m_dagNotStrings.begin(), pattenIterEnd = m_dagNotStrings.end();
        pattenIter != pattenIterEnd; ++pattenIter) {
      const FileCheckPattern &patten = *pattenIter;
      assert((patten.getCheckType() == check::CheckDAG ||
              patten.getCheckType() == check::CheckNot) &&
             "Invalid CHECK-DAG or CHECK-NOT!");

      if (patten.getCheckType() == check::CheckNot) {
         notStrings.push_back(&patten);
         continue;
      }

      assert((patten.getCheckType() == check::CheckDAG) && "Expect CHECK-DAG!");

      // CHECK-DAG always matches from the Start.
      size_t matchLen = 0, matchPos = startPos;

      // Search for a match that doesn't overlap a previous match in this
      // CHECK-DAG group.
      for (auto miter = matchRanges.begin(), mEndIter = matchRanges.end(); true; ++miter) {
         StringRef MatchBuffer = buffer.substr(matchPos);
         Expected<size_t> matchResult = patten.match(MatchBuffer, matchLen, sourceMgr);
         // With a group of CHECK-DAGs, a single mismatching means the match on
         // that group of CHECK-DAGs fails immediately.
         if (!matchResult) {
            print_no_match(true, sourceMgr, prefix, patten.getLoc(), patten, 1, MatchBuffer,
                           req.verboseVerbose, diags, matchResult.takeError());
            return StringRef::npos;
         }
         size_t matchPosBuf = *matchResult;
         // Re-calc it as the offset relative to the Start of the original string.
         matchPos += matchPosBuf;
         if (req.verboseVerbose)
            print_match(true, sourceMgr, prefix, patten.getLoc(), patten, 1, buffer, matchPos,
                        matchLen, req, diags);
         MatchRange M{matchPos, matchPos + matchLen};
         if (req.allowDeprecatedDagOverlap) {
            // We don't need to track all matches in this mode, so we just maintain
            // one match range that encompasses the current CHECK-DAG group's
            // matches.
            if (matchRanges.empty())
               matchRanges.insert(matchRanges.end(), M);
            else {
               auto block = matchRanges.begin();
               block->pos = std::min(block->pos, M.pos);
               block->End = std::max(block->End, M.End);
            }
            break;
         }
         // Iterate previous matches until overlapping match or insertion point.
         bool overlap = false;
         for (; miter != mEndIter; ++miter) {
            if (M.pos < miter->End) {
               // !overlap => New match has no overlap and is before this old match.
               // overlap => New match overlaps this old match.
               overlap = miter->pos < M.End;
               break;
            }
         }
         if (!overlap) {
            // Insert non-overlapping match into list.
            matchRanges.insert(miter, M);
            break;
         }
         if (req.verboseVerbose) {
            // Due to their verbosity, we don't print verbose diagnostics here if
            // we're gathering them for a different rendering, but we always print
            // other diagnostics.
            if (!diags) {
               SMLoc oldStart = SMLoc::getFromPointer(buffer.data() + miter->pos);
               SMLoc oldEnd = SMLoc::getFromPointer(buffer.data() + miter->End);
               SMRange oldRange(oldStart, oldEnd);
               sourceMgr.PrintMessage(oldStart, SourceMgr::DK_Note,
                                      "match discarded, overlaps earlier DAG match here",
               {oldRange});
            } else
               diags->rbegin()->matchType = FileCheckDiag::MatchFoundButDiscarded;
         }
         matchPos = miter->End;
      }
      if (!req.verboseVerbose)
         print_match(true, sourceMgr, prefix, patten.getLoc(), patten, 1, buffer, matchPos,
                     matchLen, req, diags);

      // Handle the end of a CHECK-DAG group.
      if (std::next(pattenIter) == pattenIterEnd ||
          std::next(pattenIter)->getCheckType() == check::CheckNot) {
         if (!notStrings.empty()) {
            // If there are CHECK-NOTs between two CHECK-DAGs or from CHECK to
            // CHECK-DAG, verify that there are no 'not' strings occurred in that
            // region.
            StringRef skippedRegion =
                  buffer.slice(startPos, matchRanges.begin()->pos);
            if (checkNot(sourceMgr, skippedRegion, notStrings, req, diags))
               return StringRef::npos;
            // Clear "not strings".
            notStrings.clear();
         }
         // All subsequent CHECK-DAGs and CHECK-NOTs should be matched from the
         // end of this CHECK-DAG group's match range.
         startPos = matchRanges.rbegin()->End;
         // Don't waste time checking for (impossible) overlaps before that.
         matchRanges.clear();
      }
   }

   return startPos;
}

// A check prefix must contain only alphanumeric, hyphens and underscores.
static bool validate_check_prefix(StringRef checkPrefix)
{
   return boost::regex_match(checkPrefix.str(), boost::regex("^[a-zA-Z0-9_-]*$"));
}

bool FileCheck::validateCheckPrefixes()
{
   StringSet<> prefixSet;

   for (StringRef prefix : m_req.checkPrefixes) {
      // Reject empty prefixes.
      if (prefix == "") {
         return false;
      }
      if (!prefixSet.insert(prefix).second) {
         return false;
      }
      if (!validate_check_prefix(prefix)) {
         return false;
      }
   }
   return true;
}

boost::regex FileCheck::buildCheckPrefixRegex()
{
   // I don't think there's a way to specify an initial value for cl::list,
   // so if nothing was specified, add the default
   if (m_req.checkPrefixes.empty()) {
      m_req.checkPrefixes.push_back("CHECK");
   }

   // We already validated the contents of CheckPrefixes so just concatenate
   // them as alternatives.
   SmallString<32> prefixRegexStr;
   for (StringRef prefix : m_req.checkPrefixes) {
      if (prefix != m_req.checkPrefixes.front()) {
         prefixRegexStr.push_back('|');
      }
      prefixRegexStr.append(prefix);
   }
   return boost::regex(prefixRegexStr.str().str());
}

Error FileCheckPatternContext::defineCmdlineVariables(
      std::vector<std::string> &cmdlineDefines, SourceMgr &sourceMgr)
{
   assert(m_globalVariableTable.empty() && m_globalVariableTable.empty() &&
          "Overriding defined variable with command-line variable definitions");
   if (cmdlineDefines.empty()) {
      return Error::success();
   }
   // Create a string representing the vector of command-line definitions. Each
   // definition is on its own line and prefixed with a definition number to
   // clarify which definition a given diagnostic corresponds to.
   unsigned I = 0;
   Error errors = Error::success();
   std::string cmdlineDefsDiag;
   StringRef prefix1 = "Global define #";
   StringRef prefix2 = ": ";
   for (StringRef cmdlineDef : cmdlineDefines) {
      cmdlineDefsDiag +=
            (prefix1 + Twine(++I) + prefix2 + cmdlineDef + "\n").str();
   }


   // Create a buffer with fake command line content in order to display
   // parsing diagnostic with location information and point to the
   // global definition with invalid syntax.
   std::unique_ptr<MemoryBuffer> cmdLineDefsDiagBuffer =
         MemoryBuffer::getMemBufferCopy(cmdlineDefsDiag, "Global defines");
   StringRef cmdlineDefsDiagRef = cmdLineDefsDiagBuffer->getBuffer();
   sourceMgr.AddNewSourceBuffer(std::move(cmdLineDefsDiagBuffer), SMLoc());

   SmallVector<StringRef, 4> cmdlineDefsDiagVec;
   cmdlineDefsDiagRef.split(cmdlineDefsDiagVec, '\n', -1 /*MaxSplit*/,
                            false /*KeepEmpty*/);
   for (StringRef cmdlineDefDiag : cmdlineDefsDiagVec) {
      unsigned defStart = cmdlineDefDiag.find(prefix2) + prefix2.size();
      StringRef cmdlineDef = cmdlineDefDiag.substr(defStart);
      size_t eqIdx = cmdlineDef.find('=');
      if (eqIdx == StringRef::npos) {
         errors = joinErrors(
                  std::move(errors),
                  FileCheckErrorDiagnostic::get(
                     sourceMgr, cmdlineDef, "missing equal sign in global definition"));
         continue;
      }

      // Numeric variable definition.
      if (cmdlineDef[0] == '#') {
         StringRef cmdlineName = cmdlineDef.substr(1, eqIdx - 1);
         Expected<FileCheckNumericVariable *> ParseResult =
               FileCheckPattern::parseNumericVariableDefinition(cmdlineName, this,
                                                                std::nullopt, sourceMgr);
         if (!ParseResult) {
            errors = joinErrors(std::move(errors), ParseResult.takeError());
            continue;
         }

         StringRef CmdlineVal = cmdlineDef.substr(eqIdx + 1);
         uint64_t val;
         if (CmdlineVal.getAsInteger(10, val)) {
            errors = joinErrors(std::move(errors),
                                FileCheckErrorDiagnostic::get(
                                   sourceMgr, CmdlineVal,
                                   "invalid value in numeric variable definition '" +
                                   CmdlineVal + "'"));
            continue;
         }
         FileCheckNumericVariable *definedNumericVariable = *ParseResult;
         definedNumericVariable->setValue(val);

         // Record this variable definition.
         m_globalNumericVariableTable[definedNumericVariable->getName()] =
               definedNumericVariable;
      } else {
         // String variable definition.
         std::pair<StringRef, StringRef> cmdlineNameVal = cmdlineDef.split('=');
         StringRef cmdlineName = cmdlineNameVal.first;
         StringRef origCmdlineName = cmdlineName;
         Expected<FileCheckPattern::VariableProperties> parseVarResult =
               FileCheckPattern::parseVariable(cmdlineName, sourceMgr);
         if (!parseVarResult) {
            errors = joinErrors(std::move(errors), parseVarResult.takeError());
            continue;
         }
         // Check that cmdlineName does not denote a pseudo variable is only
         // composed of the parsed numeric variable. This catches cases like
         // "FOO+2" in a "FOO+2=10" definition.
         if (parseVarResult->isPseudo || !cmdlineName.empty()) {
            errors = joinErrors(std::move(errors),
                                FileCheckErrorDiagnostic::get(
                                   sourceMgr, origCmdlineName,
                                   "invalid name in string variable definition '" +
                                   origCmdlineName + "'"));
            continue;
         }
         StringRef name = parseVarResult->name;

         // Detect collisions between string and numeric variables when the former
         // is created later than the latter.
         if (m_globalNumericVariableTable.find(name) !=
             m_globalNumericVariableTable.end()) {
            errors = joinErrors(std::move(errors), FileCheckErrorDiagnostic::get(
                                   sourceMgr, name,
                                   "numeric variable with name '" +
                                   name + "' already exists"));
            continue;
         }
         m_globalVariableTable.insert(cmdlineNameVal);
         // Mark the string variable as defined to detect collisions between
         // string and numeric variables in DefineCmdlineVariables when the latter
         // is created later than the former. We cannot reuse globalVariableTable
         // for this by populating it with an empty string since we would then
         // lose the ability to detect the use of an undefined variable in
         // match().
         m_definedVariableTable[name] = true;
      }
   }
   return errors;
}

void FileCheckPatternContext::clearLocalVars()
{
   SmallVector<StringRef, 16> localPatternVars;
   SmallVector<StringRef, 16> localNumericVars;
   for (const StringMapEntry<std::string> &var : m_globalVariableTable) {
      if (var.first()[0] != '$') {
         localPatternVars.push_back(var.first());
      }
   }
   // Numeric substitution reads the value of a variable directly, not via
   // m_globalNumericVariableTable. Therefore, we clear local variables by
   // clearing their value which will lead to a numeric substitution failure. We
   // also mark the variable for removal from m_globalNumericVariableTable since
   // this is what defineCmdlineVariables checks to decide that no global
   // variable has been defined.
   for (const auto &var : m_globalNumericVariableTable) {
      if (var.first()[0] != '$') {
         var.getValue()->clearValue();
         localNumericVars.push_back(var.first());
      }
   }
   for (const auto &var : localPatternVars) {
      m_globalVariableTable.erase(var);
   }
   for (const auto &var : localNumericVars) {
      m_globalNumericVariableTable.erase(var);
   }
}

/// Check the input to FileCheck provided in the \p buffer against the \p
/// checkStrings read from the check file.
///
/// Returns false if the input fails to satisfy the checks.
bool FileCheck::checkInput(SourceMgr &sourceMgr, StringRef buffer,
                           ArrayRef<FileCheckString> checkStrings,
                           std::vector<FileCheckDiag> *diags)
{
   bool checksFailed = false;
   unsigned i = 0;
   unsigned j = 0;
   unsigned e = checkStrings.size();
   while (true) {
      StringRef checkRegion;
      if (j == e) {
         checkRegion = buffer;
      } else {
         const FileCheckString &checkLabelStr = checkStrings[j];
         if (checkLabelStr.pattern.getCheckType() != check::CheckLabel) {
            ++j;
            continue;
         }
         // Scan to next CHECK-LABEL match, ignoring CHECK-NOT and CHECK-DAG
         size_t matchLabelLen = 0;
         size_t matchLabelPos =
               checkLabelStr.check(sourceMgr, buffer, true, matchLabelLen, m_req, diags);
         if (matchLabelPos == StringRef::npos) {
            // Immediately bail if CHECK-LABEL fails, nothing else we can do.
            return false;
         }
         checkRegion = buffer.substr(0, matchLabelPos + matchLabelLen);
         buffer = buffer.substr(matchLabelPos + matchLabelLen);
         ++j;
      }

      // Do not clear the first region as it's the one before the first
      // CHECK-LABEL and it would clear variables defined on the command-line
      // before they get used.
      if (i != 0 && m_req.enableVarScope) {
         m_patternContext.clearLocalVars();
      }
      for (; i != j; ++i) {
         const FileCheckString &checkStr = checkStrings[i];
         // Check each string within the scanned region, including a second check
         // of any final CHECK-LABEL (to verify CHECK-NOT and CHECK-DAG)
         size_t matchLen = 0;
         size_t matchPos =
               checkStr.check(sourceMgr, checkRegion, false, matchLen, m_req, diags);
         if (matchPos == StringRef::npos) {
            checksFailed = true;
            i = j;
            break;
         }
         checkRegion = checkRegion.substr(matchPos + matchLen);
      }
      if (j == e) {
         break;
      }
   }
   // Success if no checks failed.
   return !checksFailed;
}

} // polar::filechecker
