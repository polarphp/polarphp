// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/28.

#ifndef POLARPHP_SYNTAX_FACTORY_TOKEN_SYNTAX_NODE_FACTORY_H
#define POLARPHP_SYNTAX_FACTORY_TOKEN_SYNTAX_NODE_FACTORY_H

#include "polarphp/syntax/AbstractFactory.h"

namespace polar::syntax {

class TokenSyntaxNodeFactory final : public AbstractFactory
{
public:
   // normal keyword
   static TokenSyntax makeLineKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeFileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDirKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeClassConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeTraitConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeMethodConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeFuncConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeNamespaceConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);

   // decl keyword
   static TokenSyntax makeNamespaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeClassKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeTraitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeInterfaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeExtendsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeImplementsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeFunctionKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeVarKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeUseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeInsteadofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeAsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeGlobalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeAbstractKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeFinalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePrivateKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeProtectedKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePublicKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeListKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeArrayKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCallableKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeThreadLocalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeModuleKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePackageKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeAsyncKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeExportKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);

   // stmt keyword
   static TokenSyntax makeDeferKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeElseIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeElseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeEchoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeWhileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeForKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeForeachKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeSwitchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCaseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDefaultKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeBreakKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeContinueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeFallthroughKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeGotoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeReturnKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeTryKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCatchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeFinallyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeThrowKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);

   // expr keyword
   static TokenSyntax makeUnsetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIssetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeEmptyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeHaltCompilerKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeEvalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIncludeKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIncludeOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeRequireKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeRequireOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeLogicOrKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeLogicXorKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeLogicAndKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePrintKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeYieldKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeYieldFromKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeInstanceofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIntCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDoubleCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeStringCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeArrayCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeObjectCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeBoolCatsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeUnsetCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeNewKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCloneKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeExitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDeclareKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeClassRefStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeClassRefSelfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeClassRefParentKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeObjRefKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeTrueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeFalseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeNullKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeAwaitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);

   // punctuator keyword
   static TokenSyntax makePlusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeMinusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeMulSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDivSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeModSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeStrConcatToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePlusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeMinusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeMulEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDivEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeStrConcatEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeModEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeAndEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeXorEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeShiftLeftEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeShiftRightEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCoalesceEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeBooleanOrToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeBooleanAndToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsNotEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsNotIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsSmallerToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsSmallerOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsGreaterToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIsGreaterOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeSpaceshipToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeShiftLeftToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeShiftRightToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIncToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDecToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeNamespaceSeparatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeEllipsisToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCoalesceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePowEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeObjectOperatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDoubleArrowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDollarOpenCurlyBracesToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCurlyOpenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePaamayimNekudotayimTokenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeLeftParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeRightParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeLeftBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeRightBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeLeftSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeRightSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeLeftAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeRightAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCommaToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeColonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeSemicolonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeBacktickToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeSingleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDoubleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeVerticalBarToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeCaretToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeExclamationMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeTildeToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDollarToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeQuestionMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeErrorSuppressSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeAmpersandToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);

   // misc token
   static TokenSyntax makeLNumber(std::int64_t value, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDNumber(double value, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeIdentifierString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeVariable(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeEncapsedAndWhitespace(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeConstantEncapsedString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeStringVarName(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeNumString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeWhiteSpace(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePrefixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makePostfixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeBinaryOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeDocComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeStartHereDoc(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeEndHereDoc(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeError(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeUnknown(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_TOKEN_SYNTAX_NODE_FACTORY_H
