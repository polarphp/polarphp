// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/13.

#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/ErrorCode.h"
#include "gtest/gtest.h"
#include <memory>

using namespace polar;
using namespace polar::utils;

namespace {

OptionalError<int> t1() { return 1; }
OptionalError<int> t2() { return ErrorCode::invalid_argument; }

TEST(OptionalErrorTest, testSimpleValue)
{
   OptionalError<int> a = t1();
   // FIXME: This is probably a bug in gtest. EXPECT_TRUE should expand to
   // include the !! to make it friendly to explicit bool operators.
   EXPECT_TRUE(!!a);
   EXPECT_EQ(1, *a);

   OptionalError<int> b = a;
   EXPECT_EQ(1, *b);

   a = t2();
   EXPECT_FALSE(a);
   EXPECT_EQ(a.getError(), ErrorCode::invalid_argument);
#ifdef EXPECT_DEBUG_DEATH
   EXPECT_DEBUG_DEATH(*a, "Cannot get value when an error exists");
#endif
}

OptionalError<std::unique_ptr<int> > t3()
{
   return std::unique_ptr<int>(new int(3));
}

TEST(OptionalErrorTest, testTypes)
{
   int x;
   OptionalError<int&> a(x);
   *a = 42;
   EXPECT_EQ(42, x);

   // Move only types.
   EXPECT_EQ(3, **t3());
}

struct B {};
struct D : B {};

TEST(OptionalErrorTest, testCovariant)
{
   OptionalError<B*> b(OptionalError<D*>(nullptr));
   b = OptionalError<D*>(nullptr);

   OptionalError<std::unique_ptr<B> > b1(OptionalError<std::unique_ptr<D> >(nullptr));
   b1 = OptionalError<std::unique_ptr<D> >(nullptr);

   OptionalError<std::unique_ptr<int>> b2(OptionalError<int *>(nullptr));
   OptionalError<int *> b3(nullptr);
   OptionalError<std::unique_ptr<int>> b4(b3);
}

TEST(OptionalErrorTest, testComparison)
{
   OptionalError<int> x(ErrorCode::no_such_file_or_directory);
   EXPECT_EQ(x, ErrorCode::no_such_file_or_directory);
}

TEST(OptionalErrorTest, testImplicitConversion)
{
   OptionalError<std::string> x("string literal");
   EXPECT_TRUE(!!x);
}

TEST(OptionalErrorTest, testImplicitConversionCausesMove)
{
   struct Source {};
   struct Destination {
      Destination(const Source&) {}
      Destination(Source&&) = delete;
   };
   Source s;
   OptionalError<Destination> x = s;
   EXPECT_TRUE(!!x);
}

TEST(OptionalErrorTest, testImplicitConversionNoAmbiguity)
{
   struct CastsToErrorCode {
      CastsToErrorCode() = default;
      CastsToErrorCode(std::error_code) {}
      operator std::error_code() { return ErrorCode::invalid_argument; }
   } casts_to_error_code;
   OptionalError<CastsToErrorCode> x1(casts_to_error_code);
   OptionalError<CastsToErrorCode> x2 = casts_to_error_code;
   OptionalError<CastsToErrorCode> x3 = {casts_to_error_code};
   OptionalError<CastsToErrorCode> x4{casts_to_error_code};
   OptionalError<CastsToErrorCode> x5(ErrorCode::no_such_file_or_directory);
   OptionalError<CastsToErrorCode> x6 = ErrorCode::no_such_file_or_directory;
   OptionalError<CastsToErrorCode> x7 = {ErrorCode::no_such_file_or_directory};
   OptionalError<CastsToErrorCode> x8{ErrorCode::no_such_file_or_directory};
   EXPECT_TRUE(!!x1);
   EXPECT_TRUE(!!x2);
   EXPECT_TRUE(!!x3);
   EXPECT_TRUE(!!x4);
   EXPECT_FALSE(x5);
   EXPECT_FALSE(x6);
   EXPECT_FALSE(x7);
   EXPECT_FALSE(x8);
}

// OptionalError<int*> x(nullptr);
// OptionalError<std::unique_ptr<int>> y = x; // invalid conversion
static_assert(
      !std::is_convertible<const OptionalError<int *> &,
      OptionalError<std::unique_ptr<int>>>::value,
      "do not invoke explicit ctors in implicit conversion from lvalue");

// OptionalError<std::unique_ptr<int>> y = OptionalError<int*>(nullptr); // invalid
//                                                           // conversion
static_assert(
      !std::is_convertible<OptionalError<int *> &&,
      OptionalError<std::unique_ptr<int>>>::value,
      "do not invoke explicit ctors in implicit conversion from rvalue");

// OptionalError<int*> x(nullptr);
// OptionalError<std::unique_ptr<int>> y;
// y = x; // invalid conversion
static_assert(!std::is_assignable<OptionalError<std::unique_ptr<int>>,
              const OptionalError<int *> &>::value,
              "do not invoke explicit ctors in assignment");

// OptionalError<std::unique_ptr<int>> x;
// x = OptionalError<int*>(nullptr); // invalid conversion
static_assert(!std::is_assignable<OptionalError<std::unique_ptr<int>>,
              OptionalError<int *> &&>::value,
              "do not invoke explicit ctors in assignment");

} // anonymous namespace
