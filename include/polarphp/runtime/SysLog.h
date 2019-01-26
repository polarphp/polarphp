// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/18.

#ifndef POLARPHP_RUNTIME_SYSLOG_H
#define POLARPHP_RUNTIME_SYSLOG_H

#include "polarphp/global/CompilerFeature.h"

#ifdef POLAR_OS_WIN32
#include "win32/syslog.h"
#else
#include "polarphp/global/php_config.h"
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif
#endif

namespace polar {
namespace runtime {

void php_syslog(int, const char *format, ...);
void php_openlog(const char *, int, int);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_SYSLOG_H
