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

#include "Run.h"
#include "Utils.h"
#include <iostream>

namespace polar {
namespace lit {

Run::Run(LitConfigPointer litConfig, const TestList &tests)
   : m_litConfig(litConfig),
     m_tests(tests)
{
}

const TestList &Run::getTests() const
{
   return m_tests;
}

TestList &Run::getTests()
{
   return m_tests;
}

/// Run one test in a multiprocessing.Pool
///
/// Side effects in this function and functions it calls are not visible in the
/// main lit process.
///
/// Arguments and results of this function are pickled, so they should be cheap
/// to copy. For efficiency, we copy all data needed to execute all tests into
/// each worker and store it in the child_* global variables. This reduces the
/// cost of each task.
///
/// Returns an index and a Result, which the parent process uses to update
/// the display.
///
void worker_run_one_test(int testIndex, TestPointer test)
{

}

} // lit
} // polar


