// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/17.

#include "polarphp/syntax/RawSyntax.h"
#include "llvm/ADT/SmallString.h"
#include "polarphp/syntax/TokenKinds.h"
#include "gtest/gtest.h"

using polar::syntax::RawSyntax;
using polar::syntax::TokenKindType;
using polar::syntax::TriviaPiece;
using polar::syntax::SourcePresence;
using polar::syntax::AbsolutePosition;
using polar::basic::OwnedString;

TEST(RawSyntaxTest, accumulateAbsolutePosition1)
{
   auto token = RawSyntax::make(TokenKindType::T_LNUMBER,
                                OwnedString("aaa"),
   {
                                   TriviaPiece::getNewlines(2),
                                   TriviaPiece::getCarriageReturns(2),
                                   TriviaPiece::getCarriageReturnLineFeeds(2)
                                },
   {},
                                SourcePresence::Present);
   AbsolutePosition pos;
   token->accumulateAbsolutePosition(pos);
   ASSERT_EQ(7u, pos.getLine());
   ASSERT_EQ(4u, pos.getColumn());
   ASSERT_EQ(11u, pos.getOffset());
}

TEST(RawSyntaxTest, accumulateAbsolutePosition2)
{
   auto token = RawSyntax::make(TokenKindType::T_LNUMBER,
                                OwnedString("aaa"),
   {TriviaPiece::getBlockComment("/* \n\r\r\n */")},
   {},
                                SourcePresence::Present);
   AbsolutePosition pos;
   token->accumulateAbsolutePosition(pos);
   ASSERT_EQ(4u, pos.getLine());
   ASSERT_EQ(7u, pos.getColumn());
   ASSERT_EQ(13u, pos.getOffset());
}
