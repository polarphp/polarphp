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

#ifndef POLARPHP_UNITTEST_SUPPORT_ERROR_H
#define POLARPHP_UNITTEST_SUPPORT_ERROR_H

#include "polarphp/utils/Error.h"
#include "SupportHelper.h"

#include "gmock/gmock.h"
#include <ostream>

namespace polar {
namespace unittest {

using polar::utils::Error;

namespace internal {

ErrorHolder take_error(Error error);

template <typename T> ExpectedHolder<T> take_expected(Expected<T> &expected)
{
   return {take_error(expected.takeError()), expected};
}

template <typename T> ExpectedHolder<T> take_expected(Expected<T> &&expected)
{
   return take_expected(expected);
}

template <typename T>
class ValueMatchesMono
      : public testing::MatcherInterface<const ExpectedHolder<T> &>
{
public:
   explicit ValueMatchesMono(const testing::Matcher<T> &m_matcher)
      : m_matcher(m_matcher) {}

   bool MatchAndExplain(const ExpectedHolder<T> &holder,
                        testing::MatchResultListener *listener) const override
   {
      if (!holder.getSuccess()) {
         return false;
      }
      bool result = m_matcher.MatchAndExplain(*holder.m_expected, listener);
      if (result) {
         return result;
      }
      *listener << "(";
      m_matcher.DescribeNegationTo(listener->stream());
      *listener << ")";
      return result;
   }

   void DescribeTo(std::ostream *outstream) const override
   {
      *outstream << "succeeded with value (";
      m_matcher.DescribeTo(outstream);
      *outstream << ")";
   }

   void DescribeNegationTo(std::ostream *outstream) const override
   {
      *outstream << "did not succeed or value (";
      m_matcher.DescribeNegationTo(outstream);
      *outstream << ")";
   }

private:
   testing::Matcher<T> m_matcher;
};

template<typename M>
class ValueMatchesPoly
{
public:
   explicit ValueMatchesPoly(const M &matcher) : m_matcher(matcher) {}

   template <typename T>
   operator testing::Matcher<const ExpectedHolder<T> &>() const {
      return MakeMatcher(
               new ValueMatchesMono<T>(testing::SafeMatcherCast<T>(m_matcher)));
   }

private:
   M m_matcher;
};

template <typename InfoT>
class ErrorMatchesMono : public testing::MatcherInterface<const ErrorHolder &>
{
public:
   explicit ErrorMatchesMono(std::optional<testing::Matcher<InfoT &>> matcher)
      : m_matcher(std::move(matcher))
   {}

   bool MatchAndExplain(const ErrorHolder &holder,
                        testing::MatchResultListener *listener) const override {
      if (holder.getSuccess()) {
         return false;
      }

      if (holder.m_infos.size() > 1) {
         *listener << "multiple errors";
         return false;
      }

      auto &info = *holder.m_infos[0];
      if (!info.isA<InfoT>()) {
         *listener << "Error was not of given type";
         return false;
      }

      if (!m_matcher) {
         return true;
      }
      return m_matcher->MatchAndExplain(static_cast<InfoT &>(info), listener);
   }

   void DescribeTo(std::ostream *OS) const override {
      *OS << "failed with Error of given type";
      if (m_matcher) {
         *OS << " and the error ";
         m_matcher->DescribeTo(OS);
      }
   }

   void DescribeNegationTo(std::ostream *OS) const override {
      *OS << "succeeded or did not fail with the error of given type";
      if (m_matcher) {
         *OS << " or the error ";
         m_matcher->DescribeNegationTo(OS);
      }
   }

private:
   std::optional<testing::Matcher<InfoT &>> m_matcher;
};


} // namespace internal

#define EXPECT_THAT_ERROR(error, matcher)                                        \
   EXPECT_THAT(polar::unittest::internal::take_error(error), matcher)
#define ASSERT_THAT_ERROR(error, matcher)                                        \
   ASSERT_THAT(polar::unittest::internal::take_error(error), matcher)

#define EXPECT_THAT_EXPECTED(error, matcher)                                     \
   EXPECT_THAT(polar::unittest::internal::take_expected(error), matcher)
#define ASSERT_THAT_EXPECTED(error, matcher)                                     \
   ASSERT_THAT(polar::unittest::internal::take_expected(error), matcher)

MATCHER(Succeeded, "") { return arg.getSuccess(); }
MATCHER(Failed, "") { return !arg.getSuccess(); }

template <typename M>
internal::ValueMatchesPoly<M> has_value(M matcher)
{
   return internal::ValueMatchesPoly<M>(matcher);
}

template <typename InfoT>
testing::Matcher<const internal::ErrorHolder &> Failed()
{
   return MakeMatcher(new internal::ErrorMatchesMono<InfoT>(std::nullopt));
}

template <typename InfoT, typename M>
testing::Matcher<const internal::ErrorHolder &> Failed(M matcher)
{
   return MakeMatcher(new internal::ErrorMatchesMono<InfoT>(
                         testing::SafeMatcherCast<InfoT &>(matcher)));
}

template <typename M>
internal::ValueMatchesPoly<M> HasValue(M matcher)
{
   return internal::ValueMatchesPoly<M>(matcher);
}

} // unittest
} // polar

#endif // POLARPHP_UNITTEST_SUPPORT_ERROR_H
