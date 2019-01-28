// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/20.

#ifndef POLARPHP_RUNTIME_SCANDIR_H
#define POLARPHP_RUNTIME_SCANDIR_H

#include "polarphp/vm/zend/zend_config.h"
#include "polarphp/global/SystemDetection.h"
#include <sys/types.h>

#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif

#ifdef POLAR_WIN32
#include "config.w32.h"
#include "win32/readdir.h"
#else
#include "polarphp/global/php_config.h"
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

namespace polar {
namespace runtime {

#ifdef HAVE_SCANDIR
#define php_scandir		scandir
#else
PHPAPI int php_scandir(const char *dirname, struct dirent **namelist[], int (*selector) (const struct dirent *entry), int (*compare) (const struct dirent **a, const struct dirent **b));
#endif

#ifdef HAVE_ALPHASORT
#define php_alphasort	alphasort
#else
PHPAPI int php_alphasort(const struct dirent **a, const struct dirent **b);
#endif

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_SCANDIR_H
