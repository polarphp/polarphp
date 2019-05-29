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
   static TokenSyntax makeLineKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeFileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDirKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeClassConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeTraitConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeMethodConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeFuncConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeNamespaceConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);

   // decl keyword
   static TokenSyntax makeNamespaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeClassKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeTraitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeInterfaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeExtendsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeImplementsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeFunctionKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeVarKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeUseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeInsteadofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeAsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeGlobalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeAbstractKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeFinalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePrivateKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeProtectedKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePublicKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeListKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeArrayKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCallableKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeThreadLocalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeModuleKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePackageKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeAsyncKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeExportKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);

   // stmt keyword
   static TokenSyntax makeDeferKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeElseIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeElseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEchoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeWhileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeForKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeForeachKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeSwitchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCaseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDefaultKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeBreakKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeContinueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeFallthroughKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeGotoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeReturnKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeTryKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCatchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeFinallyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeThrowKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);

   // expr keyword
   static TokenSyntax makeUnsetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIssetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEmptyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeHaltCompilerKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEvalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIncludeKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIncludeOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeRequireKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeRequireOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeLogicOrKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeLogicXorKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeLogicAndKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePrintKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeYieldKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeYieldFromKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeInstanceofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIntCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDoubleCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeStringCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeArrayCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeObjectCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeBoolCatsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeUnsetCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeNewKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCloneKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeExitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDeclareKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEndDeclareKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeClassRefStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeClassRefSelfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeClassRefParentKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeObjRefKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeTrueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeFalseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeNullKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeAwaitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);

   // punctuator keyword
   static TokenSyntax makePlusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeMinusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeMulSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDivSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeModSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeStrConcatToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePlusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeMinusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeMulEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDivEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeStrConcatEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeModEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeAndEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeXorEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeShiftLeftEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeShiftRightEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCoalesceEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeBooleanOrToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeBooleanAndToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsNotEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsNotIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsSmallerToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsSmallerOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsGreaterToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIsGreaterOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeSpaceshipToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeShiftLeftToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeShiftRightToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeIncToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDecToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeNamespaceSeparatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEllipsisToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCoalesceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePowEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeObjectOperatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDoubleArrowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDollarOpenCurlyBracesToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCurlyOpenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePaamayimNekudotayimTokenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeLeftParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeRightParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeLeftBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeRightBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeLeftSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeRightSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeLeftAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeRightAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCommaToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeColonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeSemiColonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeBacktickToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeSingleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDoubleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeVerticalBarToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCaretToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeExclamationMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeTildeToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePrefixQuestionMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeInfixQuestionMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeErrorSuppressSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePrefixAmpersandToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);

   // misc token
   static TokenSyntax makeLNumber(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDNumber(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeVariable(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeInlineHtml(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEncapsedAndWhitespace(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeConstantEncapsedString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeStringVarName(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeNumString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeWhiteSpace(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePrefixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makePostfixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeBinaryOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeDocComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeOpenTag(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeOpenTagWithEcho(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeCloseTag(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeStartHereDoc(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeEndHereDoc(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeError(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
   static TokenSyntax makeUnknown(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> &arena);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_TOKEN_SYNTAX_NODE_FACTORY_H
