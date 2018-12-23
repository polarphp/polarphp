// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
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

namespace polar {
namespace sys {

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

} // sys
} // polar
