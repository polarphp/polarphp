//===---- Watchdog.cpp - Implement Watchdog ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/05.

#include "polarphp/utils/WatchDog.h"
#ifdef POLAR_HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace polar::sys {

WatchDog::WatchDog(unsigned int seconds)
{
#ifdef POLAR_HAVE_UNISTD_H
   alarm(seconds);
#endif
}

WatchDog::~WatchDog()
{
#ifdef POLAR_HAVE_UNISTD_H
   alarm(0);
#endif
}

} // polar::sys
