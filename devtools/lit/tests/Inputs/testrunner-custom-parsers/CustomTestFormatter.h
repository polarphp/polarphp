// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/18.

#ifndef LITTEST_CUSTOM_TEST_FORMATTER_H
#define LITTEST_CUSTOM_TEST_FORMATTER_H

#include "formats/Base.h"

namespace polar {
namespace lit {

class CustomTestFormatter : public FileBasedTest
{
public:
   ResultPointer execute(TestPointer test, LitConfigPointer litConfig);
};

} // lit
} // polar

#endif // LITTEST_CUSTOM_TEST_FORMATTER_H
