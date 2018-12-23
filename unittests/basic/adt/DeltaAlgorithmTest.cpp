// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/20.

#include "polarphp/basic/adt/DeltaAlgorithm.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstdarg>

using polar::basic::DeltaAlgorithm;

namespace std {

std::ostream &operator<<(std::ostream &out,
                         const std::set<unsigned> &S) {
  out << "{";
  for (std::set<unsigned>::const_iterator it = S.begin(),
         ie = S.end(); it != ie; ++it) {
    if (it != S.begin())
      out << ",";
    out << *it;
  }
  out << "}";
  return out;
}

}

namespace {

class FixedDeltaAlgorithm final : public DeltaAlgorithm
{
  ChangeSetType m_failingSet;
  unsigned m_numTests;

protected:
  bool executeOneTest(const ChangeSetType &changes) override {
    ++m_numTests;
    return std::includes(changes.begin(), changes.end(),
                         m_failingSet.begin(), m_failingSet.end());
  }

public:
  FixedDeltaAlgorithm(const ChangeSetType &failingSet)
    : m_failingSet(failingSet),
      m_numTests(0) {}

  unsigned getNumTests() const { return m_numTests; }
};

std::set<unsigned> fixed_set(unsigned N, ...) {
  std::set<unsigned> S;
  va_list ap;
  va_start(ap, N);
  for (unsigned i = 0; i != N; ++i)
    S.insert(va_arg(ap, unsigned));
  va_end(ap);
  return S;
}

std::set<unsigned> range(unsigned Start, unsigned End) {
  std::set<unsigned> S;
  while (Start != End)
    S.insert(Start++);
  return S;
}

std::set<unsigned> range(unsigned N) {
  return range(0, N);
}

TEST(DeltaAlgorithmTest, testBasic)
{
  // P = {3,5,7} \in S
  //   [0, 20) should minimize to {3,5,7} in a reasonable number of tests.
  std::set<unsigned> Fails = fixed_set(3, 3, 5, 7);
  FixedDeltaAlgorithm fda(Fails);
  EXPECT_EQ(fixed_set(3, 3, 5, 7), fda.run(range(20)));
  EXPECT_GE(33U, fda.getNumTests());

  // P = {3,5,7} \in S
  //   [10, 20) should minimize to [10,20)
  EXPECT_EQ(range(10,20), fda.run(range(10,20)));

  // P = [0,4) \in S
  //   [0, 4) should minimize to [0,4) in 11 tests.
  //
  // 11 = |{ {},
  //         {0}, {1}, {2}, {3},
  //         {1, 2, 3}, {0, 2, 3}, {0, 1, 3}, {0, 1, 2},
  //         {0, 1}, {2, 3} }|
  fda = FixedDeltaAlgorithm(range(10));
  EXPECT_EQ(range(4), fda.run(range(4)));
  EXPECT_EQ(11U, fda.getNumTests());
}

}

