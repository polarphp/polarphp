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

#include "gtest/gtest.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/syntax/Trivia.h"

using llvm::SmallString;
using llvm::raw_svector_ostream;
using polar::syntax::Trivia;
using polar::syntax::TriviaKind;
using polar::syntax::TriviaPiece;

TEST(TriviaTest, testEmpty)
{
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getSpaces(0).print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getTabs(0).print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getNewlines(0).print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
#ifdef POLAR_DEBUG_BUILD
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getLineComment("").print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getBlockComment("").print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getDocLineComment("").print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getDocBlockComment("").print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia::getGarbageText("").print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
#endif
   {
      SmallString<1> scratch;
      raw_svector_ostream outStream(scratch);
      Trivia().print(outStream);
      ASSERT_EQ(outStream.str(), "");
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
   SmallString<1> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia::getBacktick().print(outStream);
   ASSERT_EQ(outStream.str(), "`");
}

TEST(TriviaTest, testPrintingSpaces)
{
   SmallString<4> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia::getSpaces(4).print(outStream);
   ASSERT_EQ(outStream.str(), "    ");
}

TEST(TriviaTest, testPrintingTabs)
{
   SmallString<4> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia::getTabs(4).print(outStream);
   ASSERT_EQ(outStream.str(), "\t\t\t\t");
}

TEST(TriviaTest, testPrintingNewlines)
{
   SmallString<4> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia::getNewlines(4).print(outStream);
   ASSERT_EQ(outStream.str(), "\n\n\n\n");
}

TEST(TriviaTest, testPrintingLineComments)
{
   SmallString<256> scratch;
   raw_svector_ostream outStream(scratch);
   auto lines = Trivia::getLineComment("// Line 1") +
         Trivia::getNewlines(1) +
         Trivia::getLineComment("// Line 2");
   lines.print(outStream);
   ASSERT_EQ(outStream.str(), "// Line 1\n// Line 2");
}

TEST(TriviaTest, testPrintingBlockComments)
{
   SmallString<256> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia::getBlockComment("/* Block Line 1\n\n  Block Line 2 */").print(outStream);
   ASSERT_EQ(outStream.str(), "/* Block Line 1\n\n  Block Line 2 */");
}


TEST(TriviaTest, testPrintingDocLineComments)
{
   SmallString<256> scratch;
   raw_svector_ostream outStream(scratch);
   auto lines = Trivia::getDocLineComment("/// Line 1") +
         Trivia::getNewlines(1) +
         Trivia::getDocLineComment("/// Line 2");
   lines.print(outStream);
   ASSERT_EQ(outStream.str(), "/// Line 1\n/// Line 2");
}

TEST(TriviaTest, testPrintingDocBlockComments)
{
   SmallString<256> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia::getDocBlockComment("/** Block Line 1\n\n  Block Line 2 */").print(outStream);
   ASSERT_EQ(outStream.str(), "/** Block Line 1\n\n  Block Line 2 */");
}

TEST(TriviaTest, testPrintingCombinations)
{
   {
      SmallString<4> scratch;
      raw_svector_ostream outStream(scratch);
      (Trivia() + Trivia()).print(outStream);
      ASSERT_EQ(outStream.str(), "");
   }
   {
      SmallString<4> scratch;
      raw_svector_ostream outStream(scratch);
      (Trivia::getNewlines(2) + Trivia::getSpaces(2)).print(outStream);
      ASSERT_EQ(outStream.str(), "\n\n  ");
   }
   {
      SmallString<48> scratch;
      raw_svector_ostream outStream(scratch);
      auto CCCCombo = Trivia::getSpaces(1) +
            Trivia::getTabs(1) +
            Trivia::getNewlines(1) +
            Trivia::getBacktick();
      CCCCombo.print(outStream);
      ASSERT_EQ(outStream.str(), " \t\n`");
   }
   {
      SmallString<48> scratch;
      raw_svector_ostream outStream(scratch);
      auto CCCCombo = Trivia::getSpaces(1) +
            Trivia::getTabs(1) +
            Trivia::getNewlines(1) +
            Trivia::getBacktick() +
            Trivia::getLineComment("// Line comment");
      CCCCombo.print(outStream);
      ASSERT_EQ(outStream.str(), " \t\n`// Line comment");
   }
}

TEST(TriviaTest, testContains)
{
   ASSERT_FALSE(Trivia().contains(TriviaKind::Backtick));
   ASSERT_FALSE(Trivia().contains(TriviaKind::BlockComment));
   ASSERT_FALSE(Trivia().contains(TriviaKind::DocBlockComment));
   ASSERT_FALSE(Trivia().contains(TriviaKind::DocLineComment));
   ASSERT_FALSE(Trivia().contains(TriviaKind::Formfeed));
   ASSERT_FALSE(Trivia().contains(TriviaKind::GarbageText));
   ASSERT_FALSE(Trivia().contains(TriviaKind::LineComment));
   ASSERT_FALSE(Trivia().contains(TriviaKind::Newline));
   ASSERT_FALSE(Trivia().contains(TriviaKind::Space));

   ASSERT_TRUE(Trivia::getBacktick().contains(TriviaKind::Backtick));
   ASSERT_TRUE(Trivia::getBlockComment("/**/").contains(TriviaKind::BlockComment));
   ASSERT_TRUE(Trivia::getDocBlockComment("/***/")
               .contains(TriviaKind::DocBlockComment));
   ASSERT_TRUE(Trivia::getDocLineComment("///")
               .contains(TriviaKind::DocLineComment));
   ASSERT_TRUE(Trivia::getGarbageText("#!swift").contains(TriviaKind::GarbageText));
   ASSERT_TRUE(Trivia::getLineComment("//").contains(TriviaKind::LineComment));
   ASSERT_TRUE(Trivia::getNewlines(1).contains(TriviaKind::Newline));
   ASSERT_TRUE(Trivia::getSpaces(1).contains(TriviaKind::Space));

   auto combo = Trivia::getSpaces(1) + Trivia::getBacktick() + Trivia::getNewlines(3)
         + Trivia::getSpaces(1);

   ASSERT_TRUE(combo.contains(TriviaKind::Space));
   ASSERT_TRUE(combo.contains(TriviaKind::Newline));
   ASSERT_TRUE(combo.contains(TriviaKind::Backtick));
   ASSERT_FALSE(combo.contains(TriviaKind::Tab));
   ASSERT_FALSE(combo.contains(TriviaKind::LineComment));
   ASSERT_FALSE(combo.contains(TriviaKind::Formfeed));
}

TEST(TriviaTest, testIteration)
{
   SmallString<6> wholeScratch;
   raw_svector_ostream wholeScratchOutStream(wholeScratch);
   auto trivia = Trivia::getSpaces(2) + Trivia::getNewlines(2) + Trivia::getSpaces(2);
   trivia.print(wholeScratchOutStream);
   SmallString<6> piecesScratch;
   raw_svector_ostream piecesOutStream(piecesScratch);
   for (const auto &piece : trivia) {
      piece.print(piecesOutStream);
   }
   ASSERT_EQ(wholeScratchOutStream.str(), piecesOutStream.str());
}

TEST(TriviaTest, testPushBack)
{
   SmallString<3> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia trivia;
   trivia.pushBack(TriviaPiece::getBacktick());
   trivia.pushBack(TriviaPiece::getBacktick());
   trivia.pushBack(TriviaPiece::getBacktick());
   trivia.print(outStream);
   ASSERT_EQ(outStream.str(), "```");
}

TEST(TriviaTest, testPushFront)
{
   SmallString<3> scratch;
   raw_svector_ostream outStream(scratch);
   Trivia trivia;
   trivia.pushBack(TriviaPiece::getBacktick());
   trivia.pushFront(TriviaPiece::getSpaces(1));
   trivia.pushBack(TriviaPiece::getSpaces(1));
   trivia.pushFront(TriviaPiece::getBacktick());
   trivia.print(outStream);
   ASSERT_EQ(outStream.str(), "` ` ");
}

TEST(TriviaTest, testFront)
{
#ifdef POLAR_DEBUG_BUILD
   ASSERT_DEATH({
                   Trivia().front();
                }, "");
#endif
   ASSERT_EQ(Trivia::getSpaces(1).front(), TriviaPiece::getSpaces(1));

   ASSERT_EQ((Trivia::getSpaces(1) + Trivia::getNewlines(1)).front(),
             TriviaPiece::getSpaces(1));
}

TEST(TriviaTest, testBack)
{
#ifdef POLAR_DEBUG_BUILD
   ASSERT_DEATH({
                   Trivia().back();
                }, "");
#endif
   ASSERT_EQ(Trivia::getSpaces(1).back(), TriviaPiece::getSpaces(1));
   ASSERT_EQ((Trivia::getSpaces(1) + Trivia::getNewlines(1)).back(),
             TriviaPiece::getNewlines(1));
}

TEST(TriviaTest, testSize)
{
   ASSERT_EQ(Trivia().size(), size_t(0));
   ASSERT_EQ(Trivia::getSpaces(1).size(), size_t(1));
   // Trivia doesn't currently coalesce on its own.
   ASSERT_EQ((Trivia::getSpaces(1) + Trivia::getSpaces(1)).size(), size_t(2));
}
