// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/21.

#include "gtest/gtest.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/syntax/Trivia.h"

using polar::basic::SmallString;
using polar::syntax::Trivia;
using polar::utils::RawSvectorOutStream;

TEST(TriviaTest, testEmpty)
{
   {
//      SmallString<1> scratch;
//      RawSvectorOutStream outStream(scratch);
//      Trivia::getSpace(0).print(outStream);
//      ASSERT_EQ(outStream.getStr(), "");
   }
}

TEST(TriviaTest, testEmptyEquivalence)
{

}

TEST(TriviaTest, testBacktick)
{

}

TEST(TriviaTest, testPrintingSpaces)
{

}

TEST(TriviaTest, testPrintingTabs)
{

}

TEST(TriviaTest, testPrintingNewlines)
{

}

TEST(TriviaTest, testPrintingLineComments)
{

}

TEST(TriviaTest, testPrintingBlockComments)
{

}


TEST(TriviaTest, testPrintingDocLineComments)
{

}

TEST(TriviaTest, testPrintingDocBlockComments)
{

}

TEST(TriviaTest, testPrintingCombinations)
{

}

TEST(TriviaTest, testContains)
{

}

TEST(TriviaTest, testIteration)
{

}


TEST(TriviaTest, testPushBack)
{

}

TEST(TriviaTest, testPushFront)
{

}

TEST(TriviaTest, testFront)
{

}


TEST(TriviaTest, testBack)
{

}

TEST(TriviaTest, testSize)
{

}
