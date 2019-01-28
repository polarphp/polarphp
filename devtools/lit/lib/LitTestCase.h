// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/14.

#ifndef POLAR_DEVLTOOLS_LIT_LITTESTCASE_H
#define POLAR_DEVLTOOLS_LIT_LITTESTCASE_H

#include <stdexcept>
#include "Test.h"

namespace polar {
namespace lit {

class Run;

class UnresolvedError : public std::runtime_error
{
   using std::runtime_error::runtime_error;
};

class LitTestCase
{
public:
   LitTestCase(TestPointer test, std::shared_ptr<Run> run)
      : m_test(test),
        m_run(run)
   {}
   const std::string getId()
   {
      return m_test->getFullName();
   }

   const std::string getShortDescription()
   {
      return m_test->getFullName();
   }

   void runTest()
   {

   }
protected:
   TestPointer m_test;
   std::shared_ptr<Run> m_run;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_LITTESTCASE_H
