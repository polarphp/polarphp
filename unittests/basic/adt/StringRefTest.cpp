// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors

// Created by softboy on 2018/07/06.

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

namespace polar {
namespace basic {

std::ostream &operator<<(std::ostream &outstream,
                         const std::pair<StringRef, StringRef> &pair)
{
   outstream << "(" << pair.first << ", " << pair.second << ")";
   return outstream;
}

} // basic
} // polar

// Check that we can't accidentally assign a temporary std::string to a
// StringRef. (Unfortunately we can't make use of the same thing with
// constructors.)
//
// Disable this check under MSVC; even MSVC 2015 isn't consistent between
// std::is_assignable and actually writing such an assignment.
#if !defined(_MSC_VER)
static_assert(
      !std::is_assignable<StringRef&, std::string>::value,
      "Assigning from prvalue std::string");
static_assert(
      !std::is_assignable<StringRef&, std::string &&>::value,
      "Assigning from xvalue std::string");
static_assert(
      std::is_assignable<StringRef&, std::string &>::value,
      "Assigning from lvalue std::string");
static_assert(
      std::is_assignable<StringRef&, const char *>::value,
      "Assigning from prvalue C string");
static_assert(
      std::is_assignable<StringRef&, const char * &&>::value,
      "Assigning from xvalue C string");
static_assert(
      std::is_assignable<StringRef&, const char * &>::value,
      "Assigning from lvalue C string");
#endif

namespace {

TEST(StringRefTest, testConstruction)
{
   EXPECT_EQ("", StringRef());
   EXPECT_EQ("hello", StringRef("hello"));
   EXPECT_EQ("hello", StringRef("hello world", 5));
   EXPECT_EQ("hello", StringRef(std::string("hello")));
}

}
