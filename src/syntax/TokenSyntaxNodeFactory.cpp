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

namespace polar::syntax {

TokenSyntax TokenSyntaxNodeFactory::makeLineKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LINE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LINE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeFileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FILE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FILE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDirKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DIR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DIR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLASS_C, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_C)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeTraitConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_TRAIT_C, OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRAIT_C)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeMethodConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_METHOD_C, OwnedString::makeUnowned(get_token_text(TokenKindType::T_METHOD_C)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeFuncConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FUNC_C, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FUNC_C)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeNamespaceConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_NS_C, OwnedString::makeUnowned(get_token_text(TokenKindType::T_NS_C)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

// decl keyword
TokenSyntax TokenSyntaxNodeFactory::makeNamespaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_NAMESPACE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_NAMESPACE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLASS, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeTraitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_TRAIT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRAIT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeInterfaceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INTERFACE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INTERFACE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeExtendsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_EXTENDS, OwnedString::makeUnowned(get_token_text(TokenKindType::T_EXTENDS)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeImplementsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IMPLEMENTS, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IMPLEMENTS)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeFunctionKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FUNCTION, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FUNCTION)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeConstKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CONST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeVarKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_VAR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_VAR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeUseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_USE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_USE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeInsteadofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INSTEADOF, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INSTEADOF)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeAsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_AS, OwnedString::makeUnowned(get_token_text(TokenKindType::T_AS)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeGlobalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_GLOBAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_GLOBAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_STATIC, OwnedString::makeUnowned(get_token_text(TokenKindType::T_STATIC)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeAbstractKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ABSTRACT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ABSTRACT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeFinalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FINAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FINAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePrivateKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PRIVATE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PRIVATE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeProtectedKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PROTECTED, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PROTECTED)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePublicKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PUBLIC, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PUBLIC)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeListKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LIST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LIST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeArrayKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ARRAY, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ARRAY)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCallableKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CALLABLE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CALLABLE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeThreadLocalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_THREAD_LOCAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_THREAD_LOCAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeModuleKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_MODULE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_MODULE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePackageKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PACKAGE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PACKAGE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeAsyncKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ASYNC, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ASYNC)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeExportKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_EXPORT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_EXPORT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

// stmt keyword
TokenSyntax TokenSyntaxNodeFactory::makeDeferKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DEFER, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFER)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IF, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IF)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeElseIfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ELSEIF, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELSEIF)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeElseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ELSE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELSE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEchoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ECHO, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ECHO)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DO, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DO)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeWhileKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_WHILE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeForKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FOR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FOR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeForeachKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FOREACH, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FOREACH)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeSwitchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SWITCH, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SWITCH)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCaseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CASE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CASE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDefaultKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DEFAULT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFAULT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeBreakKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_BREAK, OwnedString::makeUnowned(get_token_text(TokenKindType::T_BREAK)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeContinueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CONTINUE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONTINUE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeFallthroughKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FALLTHROUGH, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FALLTHROUGH)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeGotoKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_GOTO, OwnedString::makeUnowned(get_token_text(TokenKindType::T_GOTO)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeReturnKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_RETURN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_RETURN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeTryKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_TRY, OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRY)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCatchKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CATCH, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CATCH)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeFinallyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FINALLY, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FINALLY)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeThrowKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_THROW, OwnedString::makeUnowned(get_token_text(TokenKindType::T_THROW)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

// expr keyword
TokenSyntax TokenSyntaxNodeFactory::makeUnsetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_UNSET, OwnedString::makeUnowned(get_token_text(TokenKindType::T_UNSET)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIssetKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ISSET, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ISSET)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEmptyKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_EMPTY, OwnedString::makeUnowned(get_token_text(TokenKindType::T_EMPTY)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeHaltCompilerKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_HALT_COMPILER, OwnedString::makeUnowned(get_token_text(TokenKindType::T_HALT_COMPILER)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEvalKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_EVAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_EVAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIncludeKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INCLUDE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INCLUDE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIncludeOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INCLUDE_ONCE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INCLUDE_ONCE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeRequireKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_REQUIRE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_REQUIRE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeRequireOnceKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_REQUIRE_ONCE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_REQUIRE_ONCE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeLogicOrKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LOGICAL_OR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LOGICAL_OR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeLogicXorKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LOGICAL_XOR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LOGICAL_XOR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeLogicAndKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LOGICAL_AND, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LOGICAL_AND)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePrintKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PRINT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PRINT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeYieldKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_YIELD, OwnedString::makeUnowned(get_token_text(TokenKindType::T_YIELD)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeYieldFromKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_YIELD_FROM, OwnedString::makeUnowned(get_token_text(TokenKindType::T_YIELD_FROM)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeInstanceofKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INSTANCEOF, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INSTANCEOF)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIntCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INT_CAST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INT_CAST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoubleCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DOUBLE_CAST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DOUBLE_CAST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStringCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_STRING_CAST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_STRING_CAST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeArrayCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ARRAY_CAST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ARRAY_CAST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeObjectCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ARRAY_CAST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ARRAY_CAST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeBoolCatsKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_BOOL_CAST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_BOOL_CAST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeUnsetCastKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_UNSET_CAST, OwnedString::makeUnowned(get_token_text(TokenKindType::T_UNSET_CAST)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeNewKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_NEW, OwnedString::makeUnowned(get_token_text(TokenKindType::T_NEW)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCloneKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLONE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLONE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeExitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_EXIT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_EXIT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDeclareKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DECLARE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DECLARE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEndDeclareKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                          RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ENDDECLARE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ENDDECLARE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassRefStaticKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLASS_REF_STATIC, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassRefSelfKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLASS_REF_SELF, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeClassRefParentKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLASS_REF_PARENT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeObjRefKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_OBJ_REF, OwnedString::makeUnowned(get_token_text(TokenKindType::T_OBJ_REF)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeTrueKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_TRUE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRUE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeFalseKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_FALSE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_FALSE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeNullKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                    RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_NULL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_NULL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeAwaitKeyword(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_AWAIT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_AWAIT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

// punctuator keyword
TokenSyntax TokenSyntaxNodeFactory::makePlusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PLUS_SIGN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PLUS_SIGN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeMinusSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_MINUS_SIGN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_MINUS_SIGN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeMulSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_MUL_SIGN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_MUL_SIGN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDivSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DIV_SIGN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DIV_SIGN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeModSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_MOD_SIGN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_MOD_SIGN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStrConcatToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_STR_CONCAT, OwnedString::makeUnowned(get_token_text(TokenKindType::T_STR_CONCAT)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePlusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PLUS_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PLUS_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeMinusEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_MINUS_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_MINUS_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeMulEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_MUL_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_MUL_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDivEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DIV_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DIV_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStrConcatEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_STR_CONCAT_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_STR_CONCAT_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeModEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_MOD_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_MOD_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeAndEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_AND_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_AND_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_OR_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_OR_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeXorEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_XOR_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_XOR_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftLeftEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SL_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SL_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftRightEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                             RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SR_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SR_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCoalesceEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                           RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_COALESCE_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_COALESCE_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeBooleanOrToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_BOOLEAN_OR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_BOOLEAN_OR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeBooleanAndToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_BOOLEAN_AND, OwnedString::makeUnowned(get_token_text(TokenKindType::T_BOOLEAN_AND)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsNotEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_NOT_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_NOT_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_IDENTICAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_IDENTICAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsNotIdenticalToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_NOT_IDENTICAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_NOT_IDENTICAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsSmallerToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_SMALLER, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_SMALLER)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsSmallerOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_SMALLER_OR_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_SMALLER_OR_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsGreaterToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_GREATER, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_GREATER)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIsGreaterOrEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_IS_GREATER_OR_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_IS_GREATER_OR_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeSpaceshipToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SPACESHIP, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SPACESHIP)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftLeftToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeShiftRightToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeIncToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INC, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INC)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDecToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DEC, OwnedString::makeUnowned(get_token_text(TokenKindType::T_INC)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeNamespaceSeparatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_NS_SEPARATOR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_NS_SEPARATOR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEllipsisToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ELLIPSIS, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELLIPSIS)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCoalesceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_COALESCE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_COALESCE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_POW, OwnedString::makeUnowned(get_token_text(TokenKindType::T_POW)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePowEqualToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_POW_EQUAL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_POW_EQUAL)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeObjectOperatorToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_OBJECT_OPERATOR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_OBJECT_OPERATOR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoubleArrowToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DOUBLE_ARROW, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DOUBLE_ARROW)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDollarOpenCurlyBracesToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCurlyOpenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CURLY_OPEN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CURLY_OPEN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePaamayimNekudotayimTokenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PAAMAYIM_NEKUDOTAYIM, OwnedString::makeUnowned(get_token_text(TokenKindType::T_PAAMAYIM_NEKUDOTAYIM)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LEFT_PAREN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightParenToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_RIGHT_PAREN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LEFT_BRACE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightBraceToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_RIGHT_BRACE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                               RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LEFT_SQUARE_BRACKET, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_SQUARE_BRACKET)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightSquareBracketToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_RIGHT_SQUARE_BRACKET, OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_SQUARE_BRACKET)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeLeftAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LEFT_ANGLE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_ANGLE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeRightAngleToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_RIGHT_ANGLE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_ANGLE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCommaToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_COMMA, OwnedString::makeUnowned(get_token_text(TokenKindType::T_COMMA)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeColonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_COLON, OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeSemiColonToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SEMICOLON, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SEMICOLON)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeBacktickToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_BACKTICK, OwnedString::makeUnowned(get_token_text(TokenKindType::T_BACKTICK)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeSingleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_SINGLE_STR_QUOTE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SINGLE_STR_QUOTE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDoubleStrQuoteToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                            RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DOUBLE_STR_QUOTE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_DOUBLE_STR_QUOTE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeVerticalBarToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                         RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_VBAR, OwnedString::makeUnowned(get_token_text(TokenKindType::T_VBAR)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCaretToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CARET, OwnedString::makeUnowned(get_token_text(TokenKindType::T_CARET)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeExclamationMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                             RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_EXCLAMATION_MARK, OwnedString::makeUnowned(get_token_text(TokenKindType::T_EXCLAMATION_MARK)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeTildeToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_TILDE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_TILDE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePrefixQuestionMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_TILDE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_TILDE)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeInfixQuestionMarkToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                               RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_QUESTION_MARK, OwnedString::makeUnowned(get_token_text(TokenKindType::T_QUESTION_MARK)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeErrorSuppressSignToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                               RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ERROR_SUPPRESS_SIGN, OwnedString::makeUnowned(get_token_text(TokenKindType::T_ERROR_SUPPRESS_SIGN)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePrefixAmpersandToken(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                             RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_AMPERSAND, OwnedString::makeUnowned(get_token_text(TokenKindType::T_AMPERSAND)),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

// misc token
TokenSyntax TokenSyntaxNodeFactory::makeLNumber(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_LNUMBER, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDNumber(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DNUMBER, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                               RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_STRING, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeVariable(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_VARIABLE, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeInlineHtml(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_INLINE_HTML, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEncapsedAndWhitespace(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                              RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeConstantEncapsedString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                               RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStringVarName(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                      RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_STRING_VARNAME, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeNumString(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                  RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_NUM_STRING, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeWhiteSpace(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_WHITESPACE, OwnedString::makeUnowned(""),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePrefixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_PREFIX_OPERATOR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makePostfixOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_POSTFIX_OPERATOR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeBinaryOperator(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                       RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_BINARY_OPERATOR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_COMMENT, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeDocComment(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_DOC_COMMENT, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeOpenTag(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_OPEN_TAG, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeOpenTagWithEcho(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                        RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_OPEN_TAG_WITH_ECHO, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeCloseTag(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                 RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLOSE_TAG, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeStartHereDoc(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                     RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_CLOSE_TAG, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeEndHereDoc(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                   RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_END_HEREDOC, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeError(OwnedString text, const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                              RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_ERROR, text,
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

TokenSyntax TokenSyntaxNodeFactory::makeUnknown(const Trivia &leadingTrivia, const Trivia &trailingTrivia,
                                                RefCountPtr<SyntaxArena> &arena)
{
   return makeToken(TokenKindType::T_UNKOWN_MARK, OwnedString(),
                    leadingTrivia, trailingTrivia, SourcePresence::Present, arena);
}

} // polar::syntax
