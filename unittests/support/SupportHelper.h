// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/12.

#ifndef POLAR_UNITTEST_SUPPORT_HELPER_H
#define POLAR_UNITTEST_SUPPORT_HELPER_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/RawOsOutStream.h"
#include "gtest/gtest-printers.h"

namespace polar {
namespace unittest {
namespace internal {

using polar::utils::Expected;
using polar::basic::StringRef;
using polar::utils::ErrorInfoBase;
using polar::utils::RawOsOutStream;

struct ErrorHolder
{
   std::vector<std::shared_ptr<ErrorInfoBase>> m_infos;

   bool getSuccess() const
   {
      return m_infos.empty();
   }
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
   RawOsOutStream out(*outstream);
   out << (error.getSuccess() ? "succeeded" : "failed");
   if (!error.getSuccess()) {
      const char *delim = "  (";
      for (const auto &info : error.m_infos) {
         out << delim;
         delim = "; ";
         info->log(out);
      }
      out << ")";
   }
}

template <typename T>
void print_to(const ExpectedHolder<T> &item, std::ostream *outstream)
{
   if (item.getSuccess()) {
      *outstream << "succeeded with value " << ::testing::PrintToString(*item.m_expected);
   } else {
      print_to(static_cast<const ErrorHolder &>(item), outstream);
   }
}

inline void PrintTo(const ErrorHolder &error, std::ostream *outstream)
{
   print_to(error, outstream);
}

template <typename T>
void PrintTo(const ExpectedHolder<T> &item, std::ostream *outstream)
{
   print_to(item, outstream);
}

} // namespace internal
} // unittest
} // polar

#endif // POLAR_UNITTEST_SUPPORT_HELPER_H
