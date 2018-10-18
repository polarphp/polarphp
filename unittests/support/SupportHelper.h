// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by softboy on 2018/07/12.

#ifndef POLAR_UNITTEST_SUPPORT_HELPER_H
#define POLAR_UNITTEST_SUPPORT_HELPER_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Error.h"
#include "gtest/gtest-printers.h"

namespace polar {
namespace unittest {
namespace internal {

using polar::utils::Expected;
using polar::basic::StringRef;

struct ErrorHolder
{
   bool m_success;
   std::string m_message;
};

template <typename T> struct ExpectedHolder : public ErrorHolder
{
   ExpectedHolder(ErrorHolder error, Expected<T> &expected)
      : ErrorHolder(std::move(error)), m_expected(expected)
   {}

   Expected<T> &m_expected;
};

inline void print_to(const ErrorHolder &error, std::ostream *outstream)
{
   *outstream << (error.m_success ? "succeeded" : "failed");
   if (!error.m_success) {
      *outstream << "  (" << StringRef(error.m_message).trim().getStr() << ")";
   }
}

template <typename T>
void print_to(const ExpectedHolder<T> &item, std::ostream *outstream)
{
   if (item.m_success) {
      *outstream << "succeeded with value " << ::testing::PrintToString(*item.m_expected);
   } else {
      print_to(static_cast<const ErrorHolder &>(item), outstream);
   }
}

} // namespace internal
} // unittest
} // polar

#endif // POLAR_UNITTEST_SUPPORT_HELPER_H
