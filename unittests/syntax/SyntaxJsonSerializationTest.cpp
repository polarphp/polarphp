// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/19.

#include "polarphp/syntax/internal/TokenEnumDefs.h"
#include "polarphp/syntax/serialization/SyntaxJsonSerialization.h"
#include "polarphp/syntax/SyntaxNodeBuilders.h"
#include "polarphp/syntax/SyntaxNodeFactory.h"
#include "polarphp/parser/Token.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <list>

using polar::syntax::internal::TokenKindType;
using polar::syntax::TokenSyntax;
using polar::syntax::Trivia;
using nlohmann::json;
using polar::syntax::SyntaxNodeFactory;
using polar::syntax::EmptyStmtSyntax;

namespace {

TEST(SyntaxJsonSerializationTest, testSyntaxNode)
{
   Trivia leftTrivia;
   Trivia rightTrivia;
   TokenSyntax semicolon = SyntaxNodeFactory::makeSemicolonToken(leftTrivia, rightTrivia);
   EmptyStmtSyntax emptyStmt = SyntaxNodeFactory::makeEmptyStmt(semicolon);
   json emptyStmtJson = emptyStmt;
   std::cout << emptyStmtJson.dump(3) << std::endl;
}

}
