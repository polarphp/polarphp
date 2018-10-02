// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#ifndef POLAR_DEVLTOOLS_LIT_RUN_H
#define POLAR_DEVLTOOLS_LIT_RUN_H

#include "Test.h"

namespace polar {
namespace lit {

class LitConfig;
class Run;
using RunPointer = std::shared_ptr<Run>;
using LitConfigPointer = std::shared_ptr<LitConfig>;

class Run
{
public:
   Run(LitConfigPointer litConfig, const TestList &tests);
   const TestList &getTests() const;
   TestList &getTests();
protected:
   LitConfigPointer m_litConfig;
   TestList m_tests;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_RUN_H
