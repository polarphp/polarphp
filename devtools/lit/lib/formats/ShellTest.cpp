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

#include "ShellTest.h"
#include "../Test.h"
#include "../TestRunner.h"

namespace polar {
namespace lit {

ShTest::ShTest(bool executeExternal)
   : m_executeExternal(executeExternal)
{}

Result ShTest::execute(TestPointer test, LitConfigPointer litConfig)
{
   return execute_shtest(test, litConfig, m_executeExternal);
}

} // lit
} // polar
