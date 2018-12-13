// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#ifndef POLARPHP_ARTIFACTS_DEFS_H
#define POLARPHP_ARTIFACTS_DEFS_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/SystemDetection.h"
#include "polarphp/global/Config.h"
#include "lib/PolarVersion.h"
#include "ZendHeaders.h"

#define PHP_DEFAULT_CHARSET "UTF-8"
/* PHP's DEBUG value must match Zend's ZEND_DEBUG value */
#undef PHP_DEBUG
#define PHP_DEBUG ZEND_DEBUG

#ifdef POLAR_OS_WIN32
#	define PHP_DIR_SEPARATOR '\\'
#	define PHP_EOL "\r\n"
#else
#	define PHP_DIR_SEPARATOR '/'
#	define PHP_EOL "\n"
#endif

#define PHP_MT_RAND_MAX ((zend_long) (0x7FFFFFFF)) /* (1<<31) - 1 */
/* System Rand functions */
#ifndef RAND_MAX
#define RAND_MAX PHP_MT_RAND_MAX
#endif

#define PHP_RAND_MAX PHP_MT_RAND_MAX

// forward declare with namespace
namespace CLI {
class App;
} // CLI

namespace polar {

enum class ExecMode
{
   Standard = 1,
   HighLight,
   Lint,
   Strip,
   CliDirect,
   ProcessStdin,
   ReflectionFunction,
   ReflectionClass,
   ReflectionExtension,
   ReflectionExtInfo,
   ReflectionZendExtension,
   ShowIniConfig
};

extern CLI::App *sg_commandParser;

} // polar

#endif // POLARPHP_ARTIFACTS_DEFS_H
