// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors

// Created by polarboy on 2018/07/13.

#include "polarphp/utils/LineIterator.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;
using namespace polar;

namespace {

TEST(LineIteratorTest, testBasic)
{
   std::unique_ptr<MemoryBuffer> buffer = MemoryBuffer::getMemBuffer("line 1\n"
                                                                     "line 2\n"
                                                                     "line 3");

   LineIterator iter = LineIterator(*buffer), E;

   EXPECT_FALSE(iter.isAtEof());
   EXPECT_NE(E, iter);

   EXPECT_EQ("line 1", *iter);
   EXPECT_EQ(1, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 2", *iter);
   EXPECT_EQ(2, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 3", *iter);
   EXPECT_EQ(3, iter.getLineNumber());
   ++iter;

   EXPECT_TRUE(iter.isAtEof());
   EXPECT_EQ(E, iter);
}

TEST(LineIteratorTest, testCommentAndBlankSkipping)
{
   std::unique_ptr<MemoryBuffer> buffer(
            MemoryBuffer::getMemBuffer("line 1\n"
                                       "line 2\n"
                                       "# Comment 1\n"
                                       "\n"
                                       "line 5\n"
                                       "\n"
                                       "# Comment 2"));

   LineIterator iter = LineIterator(*buffer, true, '#'), E;

   EXPECT_FALSE(iter.isAtEof());
   EXPECT_NE(E, iter);

   EXPECT_EQ("line 1", *iter);
   EXPECT_EQ(1, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 2", *iter);
   EXPECT_EQ(2, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 5", *iter);
   EXPECT_EQ(5, iter.getLineNumber());
   ++iter;

   EXPECT_TRUE(iter.isAtEof());
   EXPECT_EQ(E, iter);
}

TEST(LineIteratorTest, testCommentSkippingKeepBlanks)
{
   std::unique_ptr<MemoryBuffer> buffer(
            MemoryBuffer::getMemBuffer("line 1\n"
                                       "line 2\n"
                                       "# Comment 1\n"
                                       "# Comment 2\n"
                                       "\n"
                                       "line 6\n"
                                       "\n"
                                       "# Comment 3"));

   LineIterator iter = LineIterator(*buffer, false, '#'), E;

   EXPECT_FALSE(iter.isAtEof());
   EXPECT_NE(E, iter);

   EXPECT_EQ("line 1", *iter);
   EXPECT_EQ(1, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 2", *iter);
   EXPECT_EQ(2, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("", *iter);
   EXPECT_EQ(5, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 6", *iter);
   EXPECT_EQ(6, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("", *iter);
   EXPECT_EQ(7, iter.getLineNumber());
   ++iter;

   EXPECT_TRUE(iter.isAtEof());
   EXPECT_EQ(E, iter);
}


TEST(LineIteratorTest, testBlankSkipping)
{
   std::unique_ptr<MemoryBuffer> buffer = MemoryBuffer::getMemBuffer("\n\n\n"
                                                                     "line 1\n"
                                                                     "\n\n\n"
                                                                     "line 2\n"
                                                                     "\n\n\n");

   LineIterator iter = LineIterator(*buffer), E;

   EXPECT_FALSE(iter.isAtEof());
   EXPECT_NE(E, iter);

   EXPECT_EQ("line 1", *iter);
   EXPECT_EQ(4, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 2", *iter);
   EXPECT_EQ(8, iter.getLineNumber());
   ++iter;

   EXPECT_TRUE(iter.isAtEof());
   EXPECT_EQ(E, iter);
}

TEST(LineIteratorTest, testBlankKeeping)
{
   std::unique_ptr<MemoryBuffer> buffer = MemoryBuffer::getMemBuffer("\n\n"
                                                                     "line 3\n"
                                                                     "\n"
                                                                     "line 5\n"
                                                                     "\n\n");
   LineIterator iter = LineIterator(*buffer, false), E;

   EXPECT_FALSE(iter.isAtEof());
   EXPECT_NE(E, iter);

   EXPECT_EQ("", *iter);
   EXPECT_EQ(1, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("", *iter);
   EXPECT_EQ(2, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 3", *iter);
   EXPECT_EQ(3, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("", *iter);
   EXPECT_EQ(4, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("line 5", *iter);
   EXPECT_EQ(5, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("", *iter);
   EXPECT_EQ(6, iter.getLineNumber());
   ++iter;
   EXPECT_EQ("", *iter);
   EXPECT_EQ(7, iter.getLineNumber());
   ++iter;

   EXPECT_TRUE(iter.isAtEof());
   EXPECT_EQ(E, iter);
}

TEST(LineIteratorTest, testEmptyBuffers)
{
   std::unique_ptr<MemoryBuffer> buffer = MemoryBuffer::getMemBuffer("");
   EXPECT_TRUE(LineIterator(*buffer).isAtEof());
   EXPECT_EQ(LineIterator(), LineIterator(*buffer));
   EXPECT_TRUE(LineIterator(*buffer, false).isAtEof());
   EXPECT_EQ(LineIterator(), LineIterator(*buffer, false));

   buffer = MemoryBuffer::getMemBuffer("\n\n\n");
   EXPECT_TRUE(LineIterator(*buffer).isAtEof());
   EXPECT_EQ(LineIterator(), LineIterator(*buffer));

   buffer = MemoryBuffer::getMemBuffer("# foo\n"
                                       "\n"
                                       "# bar");
   EXPECT_TRUE(LineIterator(*buffer, true, '#').isAtEof());
   EXPECT_EQ(LineIterator(), LineIterator(*buffer, true, '#'));

   buffer = MemoryBuffer::getMemBuffer("\n"
                                       "# baz\n"
                                       "\n");
   EXPECT_TRUE(LineIterator(*buffer, true, '#').isAtEof());
   EXPECT_EQ(LineIterator(), LineIterator(*buffer, true, '#'));
}

} // anonymous namespace
