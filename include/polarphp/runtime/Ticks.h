// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/25.

#ifndef POLARPHP_RUNTIME_TICKS_H
#define POLARPHP_RUNTIME_TICKS_H

#include "polarphp/global/CompilerDetection.h"

namespace polar {
namespace runtime {

bool startup_ticks(void);
void deactivate_ticks(void);
void shutdown_ticks(void);
void run_ticks(int count);

POLAR_DECL_EXPORT void add_tick_function(void (*func)(int, void *), void *arg);
POLAR_DECL_EXPORT void remove_tick_function(void (*func)(int, void *), void * arg);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_TICKS_H
