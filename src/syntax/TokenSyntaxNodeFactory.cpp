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

#include "polarphp/syntax/factory/TokenSyntaxNodeFactory.h"

#define make_token_by_kind(token) \
   makeToken(TokenKindType::token, OwnedString::makeUnowned(get_token_text(TokenKindType::token)),\
   leadingTrivia, trailingTrivia, SourcePresence::Present, arena)

namespace polar::syntax {

TokenSyntax TokenSyntaxNodeFactory::makeLineKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LINE);
}

TokenSyntax TokenSyntaxNodeFactory::makeFileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FILE);
}

TokenSyntax TokenSyntaxNodeFactory::makeDirKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DIR);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CLASS_CONST);
}

TokenSyntax TokenSyntaxNodeFactory::makeTraitConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_TRAIT_CONST);
}

TokenSyntax TokenSyntaxNodeFactory::makeMethodConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_METHOD_CONST);
}

TokenSyntax TokenSyntaxNodeFactory::makeFuncConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FUNC_CONST);
}

TokenSyntax TokenSyntaxNodeFactory::makeNamespaceConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_NS_CONST);
}

// decl keyword
TokenSyntax TokenSyntaxNodeFactory::makeNamespaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_NAMESPACE);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CLASS);
}

TokenSyntax TokenSyntaxNodeFactory::makeTraitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_TRAIT);
}

TokenSyntax TokenSyntaxNodeFactory::makeInterfaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_INTERFACE);
}

TokenSyntax TokenSyntaxNodeFactory::makeExtendsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_EXTENDS);
}

TokenSyntax TokenSyntaxNodeFactory::makeImplementsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IMPLEMENTS);
}

TokenSyntax TokenSyntaxNodeFactory::makeFunctionKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FUNCTION);
}

TokenSyntax
TokenSyntaxNodeFactory::makeFnKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia, RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FN);
}

TokenSyntax TokenSyntaxNodeFactory::makeConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CONST);
}

TokenSyntax TokenSyntaxNodeFactory::makeVarKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_VAR);
}

TokenSyntax TokenSyntaxNodeFactory::makeUseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_USE);
}

TokenSyntax TokenSyntaxNodeFactory::makeInsteadofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_INSTEADOF);
}

TokenSyntax TokenSyntaxNodeFactory::makeAsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_AS);
}

TokenSyntax TokenSyntaxNodeFactory::makeGlobalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_GLOBAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_STATIC);
}

TokenSyntax TokenSyntaxNodeFactory::makeAbstractKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ABSTRACT);
}

TokenSyntax TokenSyntaxNodeFactory::makeFinalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FINAL);
}

TokenSyntax TokenSyntaxNodeFactory::makePrivateKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PRIVATE);
}

TokenSyntax TokenSyntaxNodeFactory::makeProtectedKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PROTECTED);
}

TokenSyntax TokenSyntaxNodeFactory::makePublicKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PUBLIC);
}

TokenSyntax TokenSyntaxNodeFactory::makeListKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LIST);
}

TokenSyntax TokenSyntaxNodeFactory::makeArrayKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ARRAY);
}

TokenSyntax TokenSyntaxNodeFactory::makeCallableKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CALLABLE);
}

TokenSyntax TokenSyntaxNodeFactory::makeThreadLocalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_THREAD_LOCAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeModuleKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_MODULE);
}

TokenSyntax TokenSyntaxNodeFactory::makePackageKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PACKAGE);
}

TokenSyntax TokenSyntaxNodeFactory::makeAsyncKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ASYNC);
}

TokenSyntax TokenSyntaxNodeFactory::makeExportKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_EXPORT);
}

// stmt keyword
TokenSyntax TokenSyntaxNodeFactory::makeDeferKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DEFER);
}

TokenSyntax TokenSyntaxNodeFactory::makeIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IF);
}

TokenSyntax TokenSyntaxNodeFactory::makeElseIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ELSEIF);
}

TokenSyntax TokenSyntaxNodeFactory::makeElseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ELSE);
}

TokenSyntax TokenSyntaxNodeFactory::makeEchoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ECHO);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DO);
}

TokenSyntax TokenSyntaxNodeFactory::makeWhileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_WHILE);
}

TokenSyntax TokenSyntaxNodeFactory::makeForKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FOR);
}

TokenSyntax TokenSyntaxNodeFactory::makeForeachKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FOREACH);
}

TokenSyntax TokenSyntaxNodeFactory::makeSwitchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SWITCH);
}

TokenSyntax TokenSyntaxNodeFactory::makeCaseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CASE);
}

TokenSyntax TokenSyntaxNodeFactory::makeDefaultKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DEFAULT);
}

TokenSyntax TokenSyntaxNodeFactory::makeBreakKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_BREAK);
}

TokenSyntax TokenSyntaxNodeFactory::makeContinueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CONTINUE);
}

TokenSyntax TokenSyntaxNodeFactory::makeFallthroughKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FALLTHROUGH);
}

TokenSyntax TokenSyntaxNodeFactory::makeGotoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_GOTO);
}

TokenSyntax TokenSyntaxNodeFactory::makeReturnKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_RETURN);
}

TokenSyntax TokenSyntaxNodeFactory::makeTryKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_TRY);
}

TokenSyntax TokenSyntaxNodeFactory::makeCatchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CATCH);
}

TokenSyntax TokenSyntaxNodeFactory::makeFinallyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FINALLY);
}

TokenSyntax TokenSyntaxNodeFactory::makeThrowKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_THROW);
}

// expr keyword
TokenSyntax TokenSyntaxNodeFactory::makeUnsetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_UNSET);
}

TokenSyntax TokenSyntaxNodeFactory::makeIssetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ISSET);
}

TokenSyntax TokenSyntaxNodeFactory::makeEmptyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_EMPTY);
}

TokenSyntax TokenSyntaxNodeFactory::makeHaltCompilerKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_HALT_COMPILER);
}

TokenSyntax TokenSyntaxNodeFactory::makeEvalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_EVAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeIncludeKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_INCLUDE);
}

TokenSyntax TokenSyntaxNodeFactory::makeIncludeOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_INCLUDE_ONCE);
}

TokenSyntax TokenSyntaxNodeFactory::makeRequireKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_REQUIRE);
}

TokenSyntax TokenSyntaxNodeFactory::makeRequireOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_REQUIRE_ONCE);
}

TokenSyntax TokenSyntaxNodeFactory::makeLogicOrKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LOGICAL_OR);
}

TokenSyntax TokenSyntaxNodeFactory::makeLogicXorKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LOGICAL_XOR);
}

TokenSyntax TokenSyntaxNodeFactory::makeLogicAndKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LOGICAL_AND);
}

TokenSyntax TokenSyntaxNodeFactory::makePrintKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PRINT);
}

TokenSyntax TokenSyntaxNodeFactory::makeYieldKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_YIELD);
}

TokenSyntax TokenSyntaxNodeFactory::makeYieldFromKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_YIELD_FROM);
}

TokenSyntax TokenSyntaxNodeFactory::makeInstanceofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_INSTANCEOF);
}

TokenSyntax TokenSyntaxNodeFactory::makeIntCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_INT_CAST);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoubleCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DOUBLE_CAST);
}

TokenSyntax TokenSyntaxNodeFactory::makeStringCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_STRING_CAST);
}

TokenSyntax TokenSyntaxNodeFactory::makeArrayCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ARRAY_CAST);
}

TokenSyntax TokenSyntaxNodeFactory::makeObjectCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_OBJECT_CAST);
}

TokenSyntax TokenSyntaxNodeFactory::makeBoolCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_BOOL_CAST);
}

TokenSyntax TokenSyntaxNodeFactory::makeUnsetCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_UNSET_CAST);
}

TokenSyntax TokenSyntaxNodeFactory::makeNewKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_NEW);
}

TokenSyntax TokenSyntaxNodeFactory::makeCloneKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CLONE);
}

TokenSyntax TokenSyntaxNodeFactory::makeExitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_EXIT);
}

TokenSyntax TokenSyntaxNodeFactory::makeDeclareKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DECLARE);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassRefStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CLASS_REF_STATIC);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassRefSelfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CLASS_REF_SELF);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassRefParentKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CLASS_REF_PARENT);
}

TokenSyntax TokenSyntaxNodeFactory::makeObjRefKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_OBJ_REF);
}

TokenSyntax TokenSyntaxNodeFactory::makeTrueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_TRUE);
}

TokenSyntax TokenSyntaxNodeFactory::makeFalseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_FALSE);
}

TokenSyntax TokenSyntaxNodeFactory::makeNullKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_NULL);
}

TokenSyntax TokenSyntaxNodeFactory::makeAwaitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_AWAIT);
}

// punctuator keyword
TokenSyntax TokenSyntaxNodeFactory::makePlusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PLUS_SIGN);
}

TokenSyntax TokenSyntaxNodeFactory::makeMinusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_MINUS_SIGN);
}

TokenSyntax TokenSyntaxNodeFactory::makeMulSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_MUL_SIGN);
}

TokenSyntax TokenSyntaxNodeFactory::makeDivSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DIV_SIGN);
}

TokenSyntax TokenSyntaxNodeFactory::makeModSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_MOD_SIGN);
}

TokenSyntax TokenSyntaxNodeFactory::makeEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeStrConcatToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_STR_CONCAT);
}

TokenSyntax TokenSyntaxNodeFactory::makePlusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PLUS_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeMinusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_MINUS_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeMulEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_MUL_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeDivEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DIV_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeStrConcatEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_STR_CONCAT_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeModEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_MOD_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeAndEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_AND_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_OR_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeXorEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_XOR_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftLeftEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SL_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftRightEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                             RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SR_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeCoalesceEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_COALESCE_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeBooleanOrToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_BOOLEAN_OR);
}

TokenSyntax TokenSyntaxNodeFactory::makeBooleanAndToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_BOOLEAN_AND);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsNotEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_NOT_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_IDENTICAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsNotIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_NOT_IDENTICAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsSmallerToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_SMALLER);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsSmallerOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_SMALLER_OR_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsGreaterToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_GREATER);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsGreaterOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_IS_GREATER_OR_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeSpaceshipToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SPACESHIP);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftLeftToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SL);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftRightToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SR);
}

TokenSyntax TokenSyntaxNodeFactory::makeIncToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_INC);
}

TokenSyntax TokenSyntaxNodeFactory::makeDecToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DEC);
}

TokenSyntax TokenSyntaxNodeFactory::makeNamespaceSeparatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_NS_SEPARATOR);
}

TokenSyntax TokenSyntaxNodeFactory::makeEllipsisToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ELLIPSIS);
}

TokenSyntax TokenSyntaxNodeFactory::makeCoalesceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_COALESCE);
}

TokenSyntax TokenSyntaxNodeFactory::makePowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_POW);
}

TokenSyntax TokenSyntaxNodeFactory::makePowEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_POW_EQUAL);
}

TokenSyntax TokenSyntaxNodeFactory::makeObjectOperatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_OBJECT_OPERATOR);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoubleArrowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DOUBLE_ARROW);
}

TokenSyntax TokenSyntaxNodeFactory::makeDollarOpenCurlyBracesToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DOLLAR_OPEN_CURLY_BRACES);
}

TokenSyntax TokenSyntaxNodeFactory::makeCurlyOpenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CURLY_OPEN);
}

TokenSyntax TokenSyntaxNodeFactory::makePaamayimNekudotayimToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_PAAMAYIM_NEKUDOTAYIM);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LEFT_PAREN);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_RIGHT_PAREN);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LEFT_BRACE);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_RIGHT_BRACE);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                               RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LEFT_SQUARE_BRACKET);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_RIGHT_SQUARE_BRACKET);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_LEFT_ANGLE);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_RIGHT_ANGLE);
}

TokenSyntax TokenSyntaxNodeFactory::makeCommaToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_COMMA);
}

TokenSyntax TokenSyntaxNodeFactory::makeColonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_COLON);
}

TokenSyntax TokenSyntaxNodeFactory::makeSemicolonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SEMICOLON);
}

TokenSyntax TokenSyntaxNodeFactory::makeBacktickToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_BACKTICK);
}

TokenSyntax TokenSyntaxNodeFactory::makeSingleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_SINGLE_QUOTE);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoubleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DOUBLE_QUOTE);
}

TokenSyntax TokenSyntaxNodeFactory::makeVerticalBarToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_VBAR);
}

TokenSyntax TokenSyntaxNodeFactory::makeCaretToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CARET);
}

TokenSyntax TokenSyntaxNodeFactory::makeExclamationMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                             RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_EXCLAMATION_MARK);
}

TokenSyntax TokenSyntaxNodeFactory::makeTildeToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_TILDE);
}

TokenSyntax TokenSyntaxNodeFactory::makeDollarToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_DOLLAR_SIGN);
}

TokenSyntax TokenSyntaxNodeFactory::makeQuestionMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_QUESTION_MARK);
}

TokenSyntax TokenSyntaxNodeFactory::makeErrorSuppressSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                               RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_ERROR_SUPPRESS_SIGN);
}

TokenSyntax TokenSyntaxNodeFactory::makeAmpersandToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_AMPERSAND);
}

// misc token
TokenSyntax TokenSyntaxNodeFactory::makeLNumber(std::int64_t value, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> arena)
{
   return make<TokenSyntax>(RawSyntax::make(TokenKindType::T_LNUMBER, OwnedString::makeRefCounted(std::to_string(value)),
                                            value, leadingTrivia.pieces, trailingTrivia.pieces, SourcePresence::Present, arena));
}

TokenSyntax TokenSyntaxNodeFactory::makeDNumber(double value, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> arena)
{
   return make<TokenSyntax>(RawSyntax::make(TokenKindType::T_DNUMBER, OwnedString::makeRefCounted(std::to_string(value)), value,
                                            leadingTrivia.pieces, trailingTrivia.pieces, SourcePresence::Present, arena));
}

TokenSyntax TokenSyntaxNodeFactory::makeIdentifierString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_IDENTIFIER_STRING, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeVariable(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_VARIABLE, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEncapsedAndWhitespace(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeConstantEncapsedString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                               RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStringVarName(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_STRING_VARNAME, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeNumString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_NUM_STRING, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeWhiteSpace(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_WHITESPACE, OwnedString::makeUnowned(""),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePrefixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_PREFIX_OPERATOR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePostfixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_POSTFIX_OPERATOR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeBinaryOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_BINARY_OPERATOR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_COMMENT, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDocComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_DOC_COMMENT, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStartHereDoc(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_CLOSE_TAG);
}

TokenSyntax TokenSyntaxNodeFactory::makeEndHereDoc(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> arena)
{
   return make_token_by_kind(T_END_HEREDOC);
}

TokenSyntax TokenSyntaxNodeFactory::makeError(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                              RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_ERROR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeUnknown(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> arena)
{
   return makeToken(TokenKindType::T_UNKNOWN_MARK, OwnedString(),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

#undef make_token_by_kind

} // polar::syntax
