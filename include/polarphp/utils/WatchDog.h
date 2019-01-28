// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_WATCHDOG_H
#define POLARPHP_UTILS_WATCHDOG_H

#include "polarphp/global/Global.h"

namespace polar {
namespace sys {

/// This class provides an abstraction for a timeout around an operation
/// that must complete in a given amount of time. Failure to complete before
/// the timeout is an unrecoverable situation and no mechanisms to attempt
/// to handle it are provided.
class WatchDog
{
public:
   WatchDog(unsigned int seconds);
   ~WatchDog();
private:
   // Noncopyable.
   WatchDog(const WatchDog &other) = delete;
   WatchDog &operator=(const WatchDog &other) = delete;
};

} // sys
} // polar

#endif // POLARPHP_UTILS_WATCHDOG_H
