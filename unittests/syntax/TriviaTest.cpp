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
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getSpaces(0).print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getTabs(0).print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getNewlines(0).print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
#ifdef POLAR_DEBUG_BUILD
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getLineComment("").print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getBlockComment("").print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getDocLineComment("").print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getDocBlockComment("").print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia::getGarbageText("").print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
#endif
   {
      SmallString<1> scratch;
      RawSvectorOutStream outStream(scratch);
      Trivia().print(outStream);
      ASSERT_EQ(outStream.getStr(), "");
   }
}

TEST(TriviaTest, testEmptyEquivalence)
{
   ASSERT_EQ(Trivia(), Trivia::getSpaces(0));
   ASSERT_TRUE(Trivia().empty());
   ASSERT_TRUE((Trivia() + Trivia()).empty());
   Trivia() == Trivia::getTabs(0);
   ASSERT_EQ(Trivia(), Trivia::getTabs(0));
   ASSERT_EQ(Trivia(), Trivia::getNewlines(0));
   ASSERT_EQ(Trivia() + Trivia(), Trivia());
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
