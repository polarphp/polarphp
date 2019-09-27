//===- llvm/unittest/Support/FileCheckTest.cpp - FileCheck tests --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FileChecker.h"
#include "gtest/gtest.h"
#include <unordered_set>

using namespace polar::basic;
using namespace polar::filechecker;
using namespace polar::utils;

namespace {

class FileCheckTest : public ::testing::Test {};

TEST_F(FileCheckTest, testLiteral)
{
   // Eval returns the literal's value.
   FileCheckExpressionLiteral ten(10);
   Expected<uint64_t> value = ten.eval();
   EXPECT_TRUE(bool(value));
   EXPECT_EQ(10U, *value);

   // max value can be correctly represented.
   FileCheckExpressionLiteral max(std::numeric_limits<uint64_t>::max());
   value = max.eval();
   EXPECT_TRUE(bool(value));
   EXPECT_EQ(std::numeric_limits<uint64_t>::max(), *value);
}

static std::string toString(const std::unordered_set<std::string> &Set)
{
   bool First = true;
   std::string Str;
   for (StringRef S : Set) {
      Str += Twine(First ? "{" + S : ", " + S).getStr();
      First = false;
   }
   Str += '}';
   return Str;
}

static void
expectUndefErrors(std::unordered_set<std::string> ExpectedUndefVarNames,
                  Error Err) {
   handle_all_errors(std::move(Err), [&](const FileCheckUndefVarError &E) {
      ExpectedUndefVarNames.erase(E.getVarName());
   });
   EXPECT_TRUE(ExpectedUndefVarNames.empty()) << toString(ExpectedUndefVarNames);
}

static void expectUndefError(const Twine &ExpectedUndefVarName, Error Err) {
   expectUndefErrors({ExpectedUndefVarName.getStr()}, std::move(Err));
}

TEST_F(FileCheckTest, testNumericVariable) {
   // Undefined variable: getValue and eval fail, error returned by eval holds
   // the name of the undefined variable and setValue does not trigger assert.
   FileCheckNumericVariable fooVar = FileCheckNumericVariable("FOO", 1);
   EXPECT_EQ("FOO", fooVar.getName());
   FileCheckNumericVariableUse fooVarUse =
         FileCheckNumericVariableUse("FOO", &fooVar);
   EXPECT_FALSE(fooVar.getValue());
   Expected<uint64_t> evalResult = fooVarUse.eval();
   EXPECT_FALSE(evalResult);
   expectUndefError("FOO", evalResult.takeError());
   fooVar.setValue(42);

   // Defined variable: getValue and eval return value set.
   std::optional<uint64_t> value = fooVar.getValue();
   EXPECT_TRUE(bool(value));
   EXPECT_EQ(42U, *value);
   evalResult = fooVarUse.eval();
   EXPECT_TRUE(bool(evalResult));
   EXPECT_EQ(42U, *evalResult);

   // Clearing variable: getValue and eval fail. Error returned by eval holds
   // the name of the cleared variable.
   fooVar.clearValue();
   value = fooVar.getValue();
   EXPECT_FALSE(value);
   evalResult = fooVarUse.eval();
   EXPECT_FALSE(evalResult);
   expectUndefError("FOO", evalResult.takeError());
}

uint64_t doAdd(uint64_t OpL, uint64_t OpR) { return OpL + OpR; }

TEST_F(FileCheckTest, testBinop) {
   FileCheckNumericVariable fooVar = FileCheckNumericVariable("FOO");
   fooVar.setValue(42);
   std::unique_ptr<FileCheckNumericVariableUse> fooVarUse =
         std::make_unique<FileCheckNumericVariableUse>("FOO", &fooVar);
   FileCheckNumericVariable barVar = FileCheckNumericVariable("BAR");
   barVar.setValue(18);
   std::unique_ptr<FileCheckNumericVariableUse> BarVarUse =
         std::make_unique<FileCheckNumericVariableUse>("BAR", &barVar);
   FileCheckASTBinop Binop =
         FileCheckASTBinop(doAdd, std::move(fooVarUse), std::move(BarVarUse));

   // Defined variable: eval returns right value.
   Expected<uint64_t> value = Binop.eval();
   EXPECT_TRUE(bool(value));
   EXPECT_EQ(60U, *value);

   // 1 undefined variable: eval fails, error contains name of undefined
   // variable.
   fooVar.clearValue();
   value = Binop.eval();
   EXPECT_FALSE(value);
   expectUndefError("FOO", value.takeError());

   // 2 undefined variables: eval fails, error contains names of all undefined
   // variables.
   barVar.clearValue();
   value = Binop.eval();
   EXPECT_FALSE(value);
   expectUndefErrors({"FOO", "BAR"}, value.takeError());
}

TEST_F(FileCheckTest, testValidVarNameStart) {
   EXPECT_TRUE(FileCheckPattern::isValidVarNameStart('a'));
   EXPECT_TRUE(FileCheckPattern::isValidVarNameStart('G'));
   EXPECT_TRUE(FileCheckPattern::isValidVarNameStart('_'));
   EXPECT_FALSE(FileCheckPattern::isValidVarNameStart('2'));
   EXPECT_FALSE(FileCheckPattern::isValidVarNameStart('$'));
   EXPECT_FALSE(FileCheckPattern::isValidVarNameStart('@'));
   EXPECT_FALSE(FileCheckPattern::isValidVarNameStart('+'));
   EXPECT_FALSE(FileCheckPattern::isValidVarNameStart('-'));
   EXPECT_FALSE(FileCheckPattern::isValidVarNameStart(':'));
}

static StringRef bufferize(SourceMgr &SM, StringRef Str) {
   std::unique_ptr<MemoryBuffer> Buffer =
         MemoryBuffer::getMemBufferCopy(Str, "TestBuffer");
   StringRef StrBufferRef = Buffer->getBuffer();
   SM.addNewSourceBuffer(std::move(Buffer), SMLocation());
   return StrBufferRef;
}

TEST_F(FileCheckTest, testParseVar) {
   SourceMgr SM;
   StringRef OrigVarName = bufferize(SM, "GoodVar42");
   StringRef varName = OrigVarName;
   Expected<FileCheckPattern::VariableProperties> parsedVarResult =
         FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(parsedVarResult->name, OrigVarName);
   EXPECT_TRUE(varName.empty());
   EXPECT_FALSE(parsedVarResult->isPseudo);

   varName = OrigVarName = bufferize(SM, "$GoodGlobalVar");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(parsedVarResult->name, OrigVarName);
   EXPECT_TRUE(varName.empty());
   EXPECT_FALSE(parsedVarResult->isPseudo);

   varName = OrigVarName = bufferize(SM, "@GoodPseudoVar");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(parsedVarResult->name, OrigVarName);
   EXPECT_TRUE(varName.empty());
   EXPECT_TRUE(parsedVarResult->isPseudo);

   varName = bufferize(SM, "42BadVar");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(error_to_bool(parsedVarResult.takeError()));

   varName = bufferize(SM, "$@");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(error_to_bool(parsedVarResult.takeError()));

   varName = OrigVarName = bufferize(SM, "B@dVar");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(varName, OrigVarName.substr(1));
   EXPECT_EQ(parsedVarResult->name, "B");
   EXPECT_FALSE(parsedVarResult->isPseudo);

   varName = OrigVarName = bufferize(SM, "B$dVar");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(varName, OrigVarName.substr(1));
   EXPECT_EQ(parsedVarResult->name, "B");
   EXPECT_FALSE(parsedVarResult->isPseudo);

   varName = bufferize(SM, "BadVar+");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(varName, "+");
   EXPECT_EQ(parsedVarResult->name, "BadVar");
   EXPECT_FALSE(parsedVarResult->isPseudo);

   varName = bufferize(SM, "BadVar-");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(varName, "-");
   EXPECT_EQ(parsedVarResult->name, "BadVar");
   EXPECT_FALSE(parsedVarResult->isPseudo);

   varName = bufferize(SM, "BadVar:");
   parsedVarResult = FileCheckPattern::parseVariable(varName, SM);
   EXPECT_TRUE(bool(parsedVarResult));
   EXPECT_EQ(varName, ":");
   EXPECT_EQ(parsedVarResult->name, "BadVar");
   EXPECT_FALSE(parsedVarResult->isPseudo);
}

class PatternTester {
private:
   size_t LineNumber = 1;
   SourceMgr SM;
   FileCheckRequest Req;
   FileCheckPatternContext Context;
   FileCheckPattern P =
         FileCheckPattern(check::CheckPlain, &Context, LineNumber++);

public:
   PatternTester() {
      std::vector<std::string> globalDefines;
      globalDefines.emplace_back(std::string("#FOO=42"));
      globalDefines.emplace_back(std::string("BAR=BAZ"));
      EXPECT_FALSE(
               error_to_bool(Context.defineCmdlineVariables(globalDefines, SM)));
      Context.createLineVariable();
      // Call parsePattern to have @LINE defined.
      P.parsePattern("N/A", "CHECK", SM, Req);
      // parsePattern does not expect to be called twice for the same line and
      // will set FixedStr and RegExStr incorrectly if it is. Therefore prepare
      // a pattern for a different line.
      initNextPattern();
   }

   void initNextPattern() {
      P = FileCheckPattern(check::CheckPlain, &Context, LineNumber++);
   }

   bool parseNumVarDefExpect(StringRef Expr) {
      StringRef ExprBufferRef = bufferize(SM, Expr);
      return error_to_bool(FileCheckPattern::parseNumericVariableDefinition(
                            ExprBufferRef, &Context, LineNumber, SM)
                         .takeError());
   }

   bool parseSubstExpect(StringRef Expr) {
      StringRef ExprBufferRef = bufferize(SM, Expr);
      std::optional<FileCheckNumericVariable *> DefinedNumericVariable;
      return error_to_bool(P.parseNumericSubstitutionBlock(
                            ExprBufferRef, DefinedNumericVariable, false, SM)
                         .takeError());
   }

   bool parsePatternExpect(StringRef Pattern) {
      StringRef PatBufferRef = bufferize(SM, Pattern);
      return P.parsePattern(PatBufferRef, "CHECK", SM, Req);
   }

   bool matchExpect(StringRef Buffer) {
      StringRef BufferRef = bufferize(SM, Buffer);
      size_t MatchLen;
      return error_to_bool(P.match(BufferRef, MatchLen, SM).takeError());
   }
};

TEST_F(FileCheckTest, testParseNumericVariableDefinition) {
   PatternTester tester;

   // Invalid definition of pseudo.
   EXPECT_TRUE(tester.parseNumVarDefExpect("@LINE"));

   // Conflict with pattern variable.
   EXPECT_TRUE(tester.parseNumVarDefExpect("BAR"));

   // Defined variable.
   EXPECT_FALSE(tester.parseNumVarDefExpect("FOO"));
}

TEST_F(FileCheckTest, testParseExpr) {
   PatternTester tester;

   // Variable definition.

   // Definition of invalid variable.
   EXPECT_TRUE(tester.parseSubstExpect("10VAR:"));
   EXPECT_TRUE(tester.parseSubstExpect("@FOO:"));
   EXPECT_TRUE(tester.parseSubstExpect("@LINE:"));

   // Garbage after name of variable being defined.
   EXPECT_TRUE(tester.parseSubstExpect("VAR GARBAGE:"));

   // Variable defined to numeric expression.
   EXPECT_TRUE(tester.parseSubstExpect("VAR1: FOO"));

   // Acceptable variable definition.
   EXPECT_FALSE(tester.parseSubstExpect("VAR1:"));
   EXPECT_FALSE(tester.parseSubstExpect("  VAR2:"));
   EXPECT_FALSE(tester.parseSubstExpect("VAR3  :"));
   EXPECT_FALSE(tester.parseSubstExpect("VAR3:  "));

   // Numeric expression.

   // Unacceptable variable.
   EXPECT_TRUE(tester.parseSubstExpect("10VAR"));
   EXPECT_TRUE(tester.parseSubstExpect("@FOO"));

   // Only valid variable.
   EXPECT_FALSE(tester.parseSubstExpect("@LINE"));
   EXPECT_FALSE(tester.parseSubstExpect("FOO"));
   EXPECT_FALSE(tester.parseSubstExpect("UNDEF"));

   // Use variable defined on same line.
   EXPECT_FALSE(tester.parsePatternExpect("[[#LINE1VAR:]]"));
   EXPECT_TRUE(tester.parseSubstExpect("LINE1VAR"));

   // Unsupported operator.
   EXPECT_TRUE(tester.parseSubstExpect("@LINE/2"));

   // Missing offset operand.
   EXPECT_TRUE(tester.parseSubstExpect("@LINE+"));

   // Valid expression.
   EXPECT_FALSE(tester.parseSubstExpect("@LINE+5"));
   EXPECT_FALSE(tester.parseSubstExpect("FOO+4"));
   tester.initNextPattern();
   EXPECT_FALSE(tester.parsePatternExpect("[[#FOO+FOO]]"));
   EXPECT_FALSE(tester.parsePatternExpect("[[#FOO+3-FOO]]"));
}

TEST_F(FileCheckTest, testParsePattern) {
   PatternTester tester;

   // Space in pattern variable expression.
   EXPECT_TRUE(tester.parsePatternExpect("[[ BAR]]"));

   // Invalid variable name.
   EXPECT_TRUE(tester.parsePatternExpect("[[42INVALID]]"));

   // Invalid pattern variable definition.
   EXPECT_TRUE(tester.parsePatternExpect("[[@PAT:]]"));
   EXPECT_TRUE(tester.parsePatternExpect("[[PAT+2:]]"));

   // Collision with numeric variable.
   EXPECT_TRUE(tester.parsePatternExpect("[[FOO:]]"));

   // Valid use of pattern variable.
   EXPECT_FALSE(tester.parsePatternExpect("[[BAR]]"));

   // Valid pattern variable definition.
   EXPECT_FALSE(tester.parsePatternExpect("[[PAT:[0-9]+]]"));

   // Invalid numeric expressions.
   EXPECT_TRUE(tester.parsePatternExpect("[[#42INVALID]]"));
   EXPECT_TRUE(tester.parsePatternExpect("[[#@FOO]]"));
   EXPECT_TRUE(tester.parsePatternExpect("[[#@LINE/2]]"));
   EXPECT_TRUE(tester.parsePatternExpect("[[#YUP:@LINE]]"));

   // Valid numeric expressions and numeric variable definition.
   EXPECT_FALSE(tester.parsePatternExpect("[[#FOO]]"));
   EXPECT_FALSE(tester.parsePatternExpect("[[#@LINE+2]]"));
   EXPECT_FALSE(tester.parsePatternExpect("[[#NUMVAR:]]"));
}

TEST_F(FileCheckTest, testMatch) {
   PatternTester tester;

   // Check matching a definition only matches a number.
   tester.parsePatternExpect("[[#NUMVAR:]]");
   EXPECT_TRUE(tester.matchExpect("FAIL"));
   EXPECT_FALSE(tester.matchExpect("18"));

   // Check matching the variable defined matches the correct number only
   tester.initNextPattern();
   tester.parsePatternExpect("[[#NUMVAR]] [[#NUMVAR+2]]");
   EXPECT_TRUE(tester.matchExpect("19 21"));
   EXPECT_TRUE(tester.matchExpect("18 21"));
   EXPECT_FALSE(tester.matchExpect("18 20"));

   // Check matching a numeric expression using @LINE after match failure uses
   // the correct value for @LINE.
   tester.initNextPattern();
   EXPECT_FALSE(tester.parsePatternExpect("[[#@LINE]]"));
   // Ok, @LINE is 4 now.
   EXPECT_FALSE(tester.matchExpect("4"));
   tester.initNextPattern();
   // @LINE is now 5, match with substitution failure.
   EXPECT_FALSE(tester.parsePatternExpect("[[#UNKNOWN]]"));
   EXPECT_TRUE(tester.matchExpect("FOO"));
   tester.initNextPattern();
   // Check that @LINE is 6 as expected.
   EXPECT_FALSE(tester.parsePatternExpect("[[#@LINE]]"));
   EXPECT_FALSE(tester.matchExpect("6"));
}

TEST_F(FileCheckTest, testSubstitution) {
   SourceMgr SM;
   FileCheckPatternContext Context;
   std::vector<std::string> globalDefines;
   globalDefines.emplace_back(std::string("FOO=BAR"));
   EXPECT_FALSE(error_to_bool(Context.defineCmdlineVariables(globalDefines, SM)));

   // Substitution of an undefined string variable fails and error holds that
   // variable's name.
   FileCheckStringSubstitution StringSubstitution =
         FileCheckStringSubstitution(&Context, "VAR404", 42);
   Expected<std::string> SubstValue = StringSubstitution.getResult();
   EXPECT_FALSE(bool(SubstValue));
   expectUndefError("VAR404", SubstValue.takeError());

   // Substitutions of defined pseudo and non-pseudo numeric variables return
   // the right value.
   FileCheckNumericVariable LineVar = FileCheckNumericVariable("@LINE");
   LineVar.setValue(42);
   FileCheckNumericVariable nvar = FileCheckNumericVariable("N");
   nvar.setValue(10);
   auto LineVarUse =
         std::make_unique<FileCheckNumericVariableUse>("@LINE", &LineVar);
   auto NVarUse = std::make_unique<FileCheckNumericVariableUse>("N", &nvar);
   FileCheckNumericSubstitution SubstitutionLine = FileCheckNumericSubstitution(
            &Context, "@LINE", std::move(LineVarUse), 12);
   FileCheckNumericSubstitution SubstitutionN =
         FileCheckNumericSubstitution(&Context, "N", std::move(NVarUse), 30);
   SubstValue = SubstitutionLine.getResult();
   EXPECT_TRUE(bool(SubstValue));
   EXPECT_EQ("42", *SubstValue);
   SubstValue = SubstitutionN.getResult();
   EXPECT_TRUE(bool(SubstValue));
   EXPECT_EQ("10", *SubstValue);

   // Substitution of an undefined numeric variable fails, error holds name of
   // undefined variable.
   LineVar.clearValue();
   SubstValue = SubstitutionLine.getResult();
   EXPECT_FALSE(bool(SubstValue));
   expectUndefError("@LINE", SubstValue.takeError());
   nvar.clearValue();
   SubstValue = SubstitutionN.getResult();
   EXPECT_FALSE(bool(SubstValue));
   expectUndefError("N", SubstValue.takeError());

   // Substitution of a defined string variable returns the right value.
   FileCheckPattern P = FileCheckPattern(check::CheckPlain, &Context, 1);
   StringSubstitution = FileCheckStringSubstitution(&Context, "FOO", 42);
   SubstValue = StringSubstitution.getResult();
   EXPECT_TRUE(bool(SubstValue));
   EXPECT_EQ("BAR", *SubstValue);
}

TEST_F(FileCheckTest, testFileCheckContext) {
   FileCheckPatternContext cxt = FileCheckPatternContext();
   std::vector<std::string> globalDefines;
   SourceMgr SM;

   // Missing equal sign.
   globalDefines.emplace_back(std::string("LocalVar"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));
   globalDefines.clear();
   globalDefines.emplace_back(std::string("#LocalNumVar"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));

   // Empty variable name.
   globalDefines.clear();
   globalDefines.emplace_back(std::string("=18"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));
   globalDefines.clear();
   globalDefines.emplace_back(std::string("#=18"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));

   // Invalid variable name.
   globalDefines.clear();
   globalDefines.emplace_back(std::string("18LocalVar=18"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));
   globalDefines.clear();
   globalDefines.emplace_back(std::string("#18LocalNumVar=18"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));

   // name conflict between pattern and numeric variable.
   globalDefines.clear();
   globalDefines.emplace_back(std::string("LocalVar=18"));
   globalDefines.emplace_back(std::string("#LocalVar=36"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));
   cxt = FileCheckPatternContext();
   globalDefines.clear();
   globalDefines.emplace_back(std::string("#LocalNumVar=18"));
   globalDefines.emplace_back(std::string("LocalNumVar=36"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));
   cxt = FileCheckPatternContext();

   // Invalid numeric value for numeric variable.
   globalDefines.clear();
   globalDefines.emplace_back(std::string("#LocalNumVar=x"));
   EXPECT_TRUE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));

   // Define local variables from command-line.
   globalDefines.clear();
   globalDefines.emplace_back(std::string("LocalVar=FOO"));
   globalDefines.emplace_back(std::string("emptyVar="));
   globalDefines.emplace_back(std::string("#LocalNumVar=18"));
   EXPECT_FALSE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));

   // Check defined variables are present and undefined is absent.
   StringRef LocalVarStr = "LocalVar";
   StringRef LocalNumVarRef = bufferize(SM, "LocalNumVar");
   StringRef EmptyVarStr = "emptyVar";
   StringRef UnknownVarStr = "UnknownVar";
   Expected<StringRef> LocalVar = cxt.getPatternVarValue(LocalVarStr);
   FileCheckPattern P = FileCheckPattern(check::CheckPlain, &cxt, 1);
   std::optional<FileCheckNumericVariable *> DefinedNumericVariable;
   Expected<std::unique_ptr<FileCheckExpressionAST>> expressionAST =
         P.parseNumericSubstitutionBlock(LocalNumVarRef, DefinedNumericVariable,
                                         /*IsLegacyLineExpr=*/false, SM);
   EXPECT_TRUE(bool(LocalVar));
   EXPECT_EQ(*LocalVar, "FOO");
   Expected<StringRef> emptyVar = cxt.getPatternVarValue(EmptyVarStr);
   Expected<StringRef> UnknownVar = cxt.getPatternVarValue(UnknownVarStr);
   EXPECT_TRUE(bool(expressionAST));
   Expected<uint64_t> expressionVal = (*expressionAST)->eval();
   EXPECT_TRUE(bool(expressionVal));
   EXPECT_EQ(*expressionVal, 18U);
   EXPECT_TRUE(bool(emptyVar));
   EXPECT_EQ(*emptyVar, "");
   EXPECT_TRUE(error_to_bool(UnknownVar.takeError()));

   // Clear local variables and check they become absent.
   cxt.clearLocalVars();
   LocalVar = cxt.getPatternVarValue(LocalVarStr);
   EXPECT_TRUE(error_to_bool(LocalVar.takeError()));
   // Check a numeric expression's evaluation fails if called after clearing of
   // local variables, if it was created before. This is important because local
   // variable clearing due to --enable-var-scope happens after numeric
   // expressions are linked to the numeric variables they use.
   EXPECT_TRUE(error_to_bool((*expressionAST)->eval().takeError()));
   P = FileCheckPattern(check::CheckPlain, &cxt, 2);
   expressionAST = P.parseNumericSubstitutionBlock(
            LocalNumVarRef, DefinedNumericVariable, /*IsLegacyLineExpr=*/false, SM);
   EXPECT_TRUE(bool(expressionAST));
   expressionVal = (*expressionAST)->eval();
   EXPECT_TRUE(error_to_bool(expressionVal.takeError()));
   emptyVar = cxt.getPatternVarValue(EmptyVarStr);
   EXPECT_TRUE(error_to_bool(emptyVar.takeError()));
   // Clear again because parseNumericSubstitutionBlock would have created a
   // dummy variable and stored it in GlobalNumericVariableTable.
   cxt.clearLocalVars();

   // Redefine global variables and check variables are defined again.
   globalDefines.emplace_back(std::string("$GlobalVar=BAR"));
   globalDefines.emplace_back(std::string("#$GlobalNumVar=36"));
   EXPECT_FALSE(error_to_bool(cxt.defineCmdlineVariables(globalDefines, SM)));
   StringRef GlobalVarStr = "$GlobalVar";
   StringRef GlobalNumVarRef = bufferize(SM, "$GlobalNumVar");
   Expected<StringRef> GlobalVar = cxt.getPatternVarValue(GlobalVarStr);
   EXPECT_TRUE(bool(GlobalVar));
   EXPECT_EQ(*GlobalVar, "BAR");
   P = FileCheckPattern(check::CheckPlain, &cxt, 3);
   expressionAST = P.parseNumericSubstitutionBlock(
            GlobalNumVarRef, DefinedNumericVariable, /*IsLegacyLineExpr=*/false, SM);
   EXPECT_TRUE(bool(expressionAST));
   expressionVal = (*expressionAST)->eval();
   EXPECT_TRUE(bool(expressionVal));
   EXPECT_EQ(*expressionVal, 36U);

   // Clear local variables and check global variables remain defined.
   cxt.clearLocalVars();
   EXPECT_FALSE(error_to_bool(cxt.getPatternVarValue(GlobalVarStr).takeError()));
   P = FileCheckPattern(check::CheckPlain, &cxt, 4);
   expressionAST = P.parseNumericSubstitutionBlock(
            GlobalNumVarRef, DefinedNumericVariable, /*IsLegacyLineExpr=*/false, SM);
   EXPECT_TRUE(bool(expressionAST));
   expressionVal = (*expressionAST)->eval();
   EXPECT_TRUE(bool(expressionVal));
   EXPECT_EQ(*expressionVal, 36U);
}
} // namespace
