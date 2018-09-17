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

#ifndef POLAR_DEVLTOOLS_LIT_FORMATS_SHELLTEST_H
#define POLAR_DEVLTOOLS_LIT_FORMATS_SHELLTEST_H

#include "Base.h"

namespace polar {
namespace lit {

class Result;
class Test;
class LitConfig;
using TestPointer = std::shared_ptr<Test>;
using LitConfigPointer = std::shared_ptr<LitConfig>;

/**
 * @brief The ShTest class
 *
 * ShTest is a format with one file per test.
 *
 * This is the primary format for regression tests as described in the LLVM
 * testing guide:
 *
 * http://llvm.org/docs/TestingGuide.html
 *
 * The ShTest files contain some number of shell-like command pipelines, along
 * with assertions about what should be in the output.
 */
class ShTest : public FileBasedTest
{
public:
   ShTest(bool executeExternal = false);
   ExecResultTuple execute(TestPointer test, LitConfigPointer litConfig);
protected:
   bool m_executeExternal;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORMATS_SHELLTEST_H
