// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/15.

#include "polarphp/syntax/factory/ExprSyntaxNodeFactory.h"

namespace polar::syntax {

Syntax ExprSyntaxNodeFactory::makeBlankCollectionSyntax(SyntaxKind kind)
{

}

ExprListSyntax ExprSyntaxNodeFactory::makeExprListSyntax(const std::vector<ExprSyntax> elements,
                                                         RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ExprSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> rawExprListSyntax = RawSyntax::make(SyntaxKind::ExprList, layout, SourcePresence::Present,
                                                              arena);
   return make<ExprListSyntax>(rawExprListSyntax);
}

///
/// make normal nodes
///
NullExprSyntax ExprSyntaxNodeFactory::makeNullExprSyntax(TokenSyntax nullKeyword, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawNullExprSyntax = RawSyntax::make(SyntaxKind::NullExpr, {
                                                                 nullKeyword.getRaw()
                                                              }, SourcePresence::Present, arena);
   return make<NullExprSyntax>(rawNullExprSyntax);
}

ClassRefParentExprSyntax ExprSyntaxNodeFactory::makeClassRefParentExprSyntax(TokenSyntax parentKeyword,
                                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawClassRefParentExprSyntax = RawSyntax::make(SyntaxKind::ClassRefParentExpr, {
                                                                           parentKeyword.getRaw()
                                                                        }, SourcePresence::Present, arena);
   return make<ClassRefParentExprSyntax>(rawClassRefParentExprSyntax);
}

ClassRefSelfExprSyntax ExprSyntaxNodeFactory::makeClassRefSelfExprSyntax(TokenSyntax selfKeyword, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawClassRefSelfExprSyntax = RawSyntax::make(SyntaxKind::ClassRefSelfExpr, {
                                                                         selfKeyword.getRaw()
                                                                      }, SourcePresence::Present, arena);
   return make<ClassRefSelfExprSyntax>(rawClassRefSelfExprSyntax);
}

ClassRefStaticExprSyntax ExprSyntaxNodeFactory::makeClassRefStaticExprSyntax(TokenSyntax staticKeyword,
                                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawClassRefStaticExprSyntax = RawSyntax::make(SyntaxKind::ClassRefStaticExpr, {
                                                                           staticKeyword.getRaw()
                                                                        }, SourcePresence::Present, arena);
   return make<ClassRefStaticExprSyntax>(rawClassRefStaticExprSyntax);
}

IntegerLiteralExprSyntax ExprSyntaxNodeFactory::makeIntegerLiteralExprSyntax(TokenSyntax digits,
                                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawIntegerLiteralExprSyntax = RawSyntax::make(SyntaxKind::IntegerLiteralExpr, {
                                                                           digits.getRaw()
                                                                        }, SourcePresence::Present, arena);
   return make<IntegerLiteralExprSyntax>(rawIntegerLiteralExprSyntax);
}

FloatLiteralExprSyntax ExprSyntaxNodeFactory::makeFloatLiteralExprSyntax(TokenSyntax floatDigits,
                                                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawFloatLiteralExprSyntax = RawSyntax::make(SyntaxKind::FloatLiteralExpr, {
                                                                         floatDigits.getRaw()
                                                                      }, SourcePresence::Present, arena);
   return make<FloatLiteralExprSyntax>(rawFloatLiteralExprSyntax);
}

StringLiteralExprSyntax ExprSyntaxNodeFactory::makeStringLiteralExprSyntax(TokenSyntax str,
                                                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawStringLiteralExprSyntax = RawSyntax::make(SyntaxKind::StringLiteralExpr, {
                                                                          str.getRaw()
                                                                       }, SourcePresence::Present, arena);
   return make<StringLiteralExprSyntax>(rawStringLiteralExprSyntax);
}

BooleanLiteralExprSyntax ExprSyntaxNodeFactory::makeBooleanLiteralExprSyntax(TokenSyntax boolean,
                                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawBooleanLiteralExprSyntax = RawSyntax::make(SyntaxKind::BooleanLiteralExpr, {
                                                                           boolean.getRaw()
                                                                        }, SourcePresence::Present, arena);
   return make<BooleanLiteralExprSyntax>(rawBooleanLiteralExprSyntax);
}

TernaryExprSyntax ExprSyntaxNodeFactory::makeTernaryExprSyntax(ExprSyntax conditionExpr, TokenSyntax questionMark,
                                                               ExprSyntax firstChoice, TokenSyntax colonMark,
                                                               ExprSyntax secondChoice, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawTernaryExprSyntax = RawSyntax::make(SyntaxKind::TernaryExpr, {
                                                                    conditionExpr.getRaw(),
                                                                    questionMark.getRaw(),
                                                                    firstChoice.getRaw(),
                                                                    colonMark.getRaw(),
                                                                    secondChoice.getRaw()
                                                                 }, SourcePresence::Present, arena);
   return make<TernaryExprSyntax>(rawTernaryExprSyntax);
}

AssignmentExprSyntax ExprSyntaxNodeFactory::makeAssignmentExprSyntax(TokenSyntax assignToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawAssignmentExprSyntax = RawSyntax::make(SyntaxKind::AssignmentExpr, {
                                                                       assignToken.getRaw()
                                                                    }, SourcePresence::Present, arena);
   return make<AssignmentExprSyntax>(rawAssignmentExprSyntax);
}

SequenceExprSyntax ExprSyntaxNodeFactory::makeSequenceExprSyntax(ExprListSyntax elements, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawSequenceExprSyntax = RawSyntax::make(SyntaxKind::SequenceExpr, {
                                                                     elements.getRaw()
                                                                  }, SourcePresence::Present, arena);
   return make<SequenceExprSyntax>(rawSequenceExprSyntax);
}

PrefixOperatorExprSyntax ExprSyntaxNodeFactory::makePrefixOperatorExprSyntax(std::optional<TokenSyntax> operatorToken, ExprSyntax expr,
                                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawPrefixOperatorExprSyntax = RawSyntax::make(SyntaxKind::PrefixOperatorExpr, {
                                                                           operatorToken.has_value() ? operatorToken->getRaw() : nullptr,
                                                                           expr.getRaw()
                                                                        }, SourcePresence::Present, arena);
   return make<PrefixOperatorExprSyntax>(rawPrefixOperatorExprSyntax);
}

PostfixOperatorExprSyntax ExprSyntaxNodeFactory::makePostfixOperatorExprSyntax(ExprSyntax expr, TokenSyntax operatorToken,
                                                                               RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawPostfixOperatorExprSyntax = RawSyntax::make(SyntaxKind::PostfixOperatorExpr, {
                                                                            expr.getRaw(),
                                                                            operatorToken.getRaw()
                                                                         }, SourcePresence::Present, arena);
   return make<PostfixOperatorExprSyntax>(rawPostfixOperatorExprSyntax);
}

BinaryOperatorExprSyntax ExprSyntaxNodeFactory::makeBinaryOperatorExprSyntax(TokenSyntax operatorToken,
                                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawBinaryOperatorExprSyntax = RawSyntax::make(SyntaxKind::BinaryOperatorExpr, {
                                                                           operatorToken.getRaw()
                                                                        }, SourcePresence::Present, arena);
   return make<BinaryOperatorExprSyntax>(rawBinaryOperatorExprSyntax);
}

/// make blank nodes
ExprListSyntax ExprSyntaxNodeFactory::makeBlankExprListSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawExprListSyntax = RawSyntax::make(SyntaxKind::ExprList, {},
                                                              SourcePresence::Present, arena);
   return make<ExprListSyntax>(rawExprListSyntax);
}

NullExprSyntax ExprSyntaxNodeFactory::makeNullExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawNullExprSyntax = RawSyntax::make(SyntaxKind::NullExpr, {
                                                                 RawSyntax::missing(TokenKindType::T_NULL, OwnedString::makeUnowned(get_token_text(TokenKindType::T_NULL)))
                                                              }, SourcePresence::Present, arena);
   return make<NullExprSyntax>(rawNullExprSyntax);
}

ClassRefParentExprSyntax ExprSyntaxNodeFactory::makeBlankClassRefParentExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawClassRefParentExprSyntax = RawSyntax::make(SyntaxKind::ClassRefParentExpr, {
                                                                           RawSyntax::missing(TokenKindType::T_CLASS_REF_PARENT,
                                                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)))
                                                                        }, SourcePresence::Present, arena);
   return make<ClassRefParentExprSyntax>(rawClassRefParentExprSyntax);
}

ClassRefSelfExprSyntax ExprSyntaxNodeFactory::makeBlankClassRefSelfExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawClassRefSelfExprSyntax = RawSyntax::make(SyntaxKind::ClassRefSelfExpr, {
                                                                         RawSyntax::missing(TokenKindType::T_CLASS_REF_SELF,
                                                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)))
                                                                      }, SourcePresence::Present, arena);
   return make<ClassRefSelfExprSyntax>(rawClassRefSelfExprSyntax);
}

ClassRefStaticExprSyntax ExprSyntaxNodeFactory::makeBlankClassRefStaticExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawClassRefStaticExprSyntax = RawSyntax::make(SyntaxKind::ClassRefStaticExpr, {
                                                                           RawSyntax::missing(TokenKindType::T_CLASS_REF_STATIC,
                                                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)))
                                                                        }, SourcePresence::Present, arena);
   return make<ClassRefStaticExprSyntax>(rawClassRefStaticExprSyntax);
}

IntegerLiteralExprSyntax ExprSyntaxNodeFactory::makeBlankIntegerLiteralExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawIntegerLiteralExprSyntax = RawSyntax::make(SyntaxKind::IntegerLiteralExpr, {
                                                                           RawSyntax::missing(TokenKindType::T_LNUMBER,
                                                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_LNUMBER)))
                                                                        }, SourcePresence::Present, arena);
   return make<IntegerLiteralExprSyntax>(rawIntegerLiteralExprSyntax);
}

FloatLiteralExprSyntax ExprSyntaxNodeFactory::makeBlankFloatLiteralExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawFloatLiteralExprSyntax = RawSyntax::make(SyntaxKind::FloatLiteralExpr, {
                                                                         RawSyntax::missing(TokenKindType::T_DNUMBER,
                                                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_DNUMBER)))
                                                                      }, SourcePresence::Present, arena);
   return make<FloatLiteralExprSyntax>(rawFloatLiteralExprSyntax);
}

///
/// TODO
///
StringLiteralExprSyntax ExprSyntaxNodeFactory::makeBlankStringLiteralExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawStringLiteralExprSyntax = RawSyntax::make(SyntaxKind::StringLiteralExpr, {
                                                                          RawSyntax::missing(TokenKindType::T_STRING,
                                                                          OwnedString::makeUnowned(get_token_text(TokenKindType::T_STRING)))
                                                                       }, SourcePresence::Present, arena);
   return make<StringLiteralExprSyntax>(rawStringLiteralExprSyntax);
}

BooleanLiteralExprSyntax ExprSyntaxNodeFactory::makeBlankBooleanLiteralExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawBooleanLiteralExprSyntax = RawSyntax::make(SyntaxKind::BooleanLiteralExpr, {
                                                                           RawSyntax::missing(TokenKindType::T_TRUE,
                                                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRUE)))
                                                                        }, SourcePresence::Present, arena);
   return make<BooleanLiteralExprSyntax>(rawBooleanLiteralExprSyntax);
}

TernaryExprSyntax ExprSyntaxNodeFactory::makeBlankTernaryExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawTernaryExprSyntax = RawSyntax::make(SyntaxKind::TernaryExpr, {
                                                                    RawSyntax::missing(SyntaxKind::Expr),
                                                                    RawSyntax::missing(TokenKindType::T_QUESTION_MARK,
                                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_QUESTION_MARK))),
                                                                    RawSyntax::missing(SyntaxKind::Expr),
                                                                    RawSyntax::missing(TokenKindType::T_COLON,
                                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON))),
                                                                    RawSyntax::missing(SyntaxKind::Expr)
                                                                 }, SourcePresence::Present, arena);
   return make<TernaryExprSyntax>(rawTernaryExprSyntax);
}

AssignmentExprSyntax ExprSyntaxNodeFactory::makeBlankAssignmentExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawAssignmentExprSyntax = RawSyntax::make(SyntaxKind::AssignmentExpr, {
                                                                       RawSyntax::missing(TokenKindType::T_EQUAL,
                                                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_EQUAL)))
                                                                    }, SourcePresence::Present, arena);
   return make<AssignmentExprSyntax>(rawAssignmentExprSyntax);
}

SequenceExprSyntax ExprSyntaxNodeFactory::makeBlankSequenceExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawSequenceExprSyntax = RawSyntax::make(SyntaxKind::SequenceExpr, {
                                                                     RawSyntax::missing(SyntaxKind::ExprList)
                                                                  }, SourcePresence::Present, arena);
   return make<SequenceExprSyntax>(rawSequenceExprSyntax);
}

PrefixOperatorExprSyntax ExprSyntaxNodeFactory::makeBlankPrefixOperatorExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawPrefixOperatorExprSyntax = RawSyntax::make(SyntaxKind::PrefixOperatorExpr, {
                                                                           nullptr,
                                                                           RawSyntax::missing(SyntaxKind::Expr)
                                                                        }, SourcePresence::Present, arena);
   return make<PrefixOperatorExprSyntax>(rawPrefixOperatorExprSyntax);
}

PostfixOperatorExprSyntax ExprSyntaxNodeFactory::makeBlankPostfixOperatorExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawPostfixOperatorExprSyntax = RawSyntax::make(SyntaxKind::PrefixOperatorExpr, {
                                                                            RawSyntax::missing(SyntaxKind::Expr),
                                                                            RawSyntax::missing(TokenKindType::T_POSTFIX_OPERATOR,
                                                                            OwnedString::makeUnowned(""))
                                                                         }, SourcePresence::Present, arena);
   return make<PostfixOperatorExprSyntax>(rawPostfixOperatorExprSyntax);
}

BinaryOperatorExprSyntax ExprSyntaxNodeFactory::makeBlankBinaryOperatorExprSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawBinaryOperatorExprSyntax = RawSyntax::make(SyntaxKind::BinaryOperatorExpr, {
                                                                           RawSyntax::missing(TokenKindType::T_BINARY_OPERATOR,
                                                                           OwnedString::makeUnowned(""))
                                                                        }, SourcePresence::Present, arena);
   return make<BinaryOperatorExprSyntax>(rawBinaryOperatorExprSyntax);
}

} // polar::syntax
