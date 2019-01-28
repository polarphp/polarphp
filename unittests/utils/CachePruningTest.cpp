// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/12.

#include "polarphp/utils/CachePruning.h"
#include "polarphp/utils/Error.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

namespace {

TEST(CachePruningPolicyParserTest, testEmpty)
{
   auto P = parse_cache_pruning_policy("");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(std::chrono::seconds(1200), P->m_interval);
   EXPECT_EQ(std::chrono::hours(7 * 24), P->m_expiration);
   EXPECT_EQ(75u, P->m_maxSizePercentageOfAvailableSpace);
}

TEST(CachePruningPolicyParserTest, testInterval)
{
   auto P = parse_cache_pruning_policy("prune_interval=1s");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(std::chrono::seconds(1), P->m_interval);
   P = parse_cache_pruning_policy("prune_interval=2m");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(std::chrono::minutes(2), *P->m_interval);
   P = parse_cache_pruning_policy("prune_interval=3h");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(std::chrono::hours(3), *P->m_interval);
}

TEST(CachePruningPolicyParserTest, testExpiration)
{
   auto P = parse_cache_pruning_policy("prune_after=1s");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(std::chrono::seconds(1), P->m_expiration);
}

TEST(CachePruningPolicyParserTest, testMaxSizePercentageOfAvailableSpace)
{
   auto P = parse_cache_pruning_policy("cache_size=100%");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(100u, P->m_maxSizePercentageOfAvailableSpace);
   EXPECT_EQ(0u, P->m_maxSizeBytes);
}

TEST(CachePruningPolicyParserTest, testMaxSizeBytes)
{
   auto P = parse_cache_pruning_policy("cache_size_bytes=1");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(75u, P->m_maxSizePercentageOfAvailableSpace);
   EXPECT_EQ(1u, P->m_maxSizeBytes);
   P = parse_cache_pruning_policy("cache_size_bytes=2k");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(75u, P->m_maxSizePercentageOfAvailableSpace);
   EXPECT_EQ(2u * 1024u, P->m_maxSizeBytes);
   P = parse_cache_pruning_policy("cache_size_bytes=3m");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(75u, P->m_maxSizePercentageOfAvailableSpace);
   EXPECT_EQ(3u * 1024u * 1024u, P->m_maxSizeBytes);
   P = parse_cache_pruning_policy("cache_size_bytes=4G");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(75u, P->m_maxSizePercentageOfAvailableSpace);
   EXPECT_EQ(4ull * 1024ull * 1024ull * 1024ull, P->m_maxSizeBytes);
}

TEST(CachePruningPolicyParserTest, testMultiple)
{
   auto P = parse_cache_pruning_policy("prune_after=1s:cache_size=50%");
   ASSERT_TRUE(bool(P));
   EXPECT_EQ(std::chrono::seconds(1200), P->m_interval);
   EXPECT_EQ(std::chrono::seconds(1), P->m_expiration);
   EXPECT_EQ(50u, P->m_maxSizePercentageOfAvailableSpace);
}

TEST(CachePruningPolicyParserTest, testErrors)
{
   EXPECT_EQ("duration must not be empty",
             to_string(parse_cache_pruning_policy("prune_interval=").takeError()));
   EXPECT_EQ("'foo' not an integer",
             to_string(parse_cache_pruning_policy("prune_interval=foos").takeError()));
   EXPECT_EQ("'24x' must end with one of 's', 'm' or 'h'",
             to_string(parse_cache_pruning_policy("prune_interval=24x").takeError()));
   EXPECT_EQ("'foo' must be a percentage",
             to_string(parse_cache_pruning_policy("cache_size=foo").takeError()));
   EXPECT_EQ("'foo' not an integer",
             to_string(parse_cache_pruning_policy("cache_size=foo%").takeError()));
   EXPECT_EQ("'101' must be between 0 and 100",
             to_string(parse_cache_pruning_policy("cache_size=101%").takeError()));
   EXPECT_EQ(
            "'foo' not an integer",
            to_string(parse_cache_pruning_policy("cache_size_bytes=foo").takeError()));
   EXPECT_EQ(
            "'foo' not an integer",
            to_string(parse_cache_pruning_policy("cache_size_bytes=foom").takeError()));
   EXPECT_EQ("Unknown key: 'foo'",
             to_string(parse_cache_pruning_policy("foo=bar").takeError()));
}

} // anonymous namespace
