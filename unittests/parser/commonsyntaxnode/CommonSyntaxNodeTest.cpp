// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/07/24.

#include "../AbstractParserTestCase.h"
#include <string>

using polar::unittest::AbstractParserTestCase;
using polar::syntax::Syntax;

class CommonSyntaxNodeTest : public AbstractParserTestCase
{
public:
   void SetUp()
   {
   }
};

TEST_F(CommonSyntaxNodeTest, testBasic)
{
   std::string source =
         R"(
         $name = "polarphp";
         function name ($name, $version)
         {

         }
         )";
   std::shared_ptr<Syntax> ast = parseSource(source);
}
