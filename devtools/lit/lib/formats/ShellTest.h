// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#ifndef POLAR_DEVLTOOLS_LIT_FORMATS_SHELLTEST_H
#define POLAR_DEVLTOOLS_LIT_FORMATS_SHELLTEST_H

#include "Base.h"
#include "../ForwardDefs.h"

namespace polar {
namespace lit {

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
   ResultPointer execute(TestPointer test, LitConfigPointer litConfig);
protected:
   bool m_executeExternal;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORMATS_SHELLTEST_H
