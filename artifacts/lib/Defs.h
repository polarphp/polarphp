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
#include "polarphp/global/Config.h"
#include "lib/PolarVersion.h"

// forward declare with namespace
namespace CLI {
class App;
} // CLI

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

#endif // POLARPHP_ARTIFACTS_DEFS_H
