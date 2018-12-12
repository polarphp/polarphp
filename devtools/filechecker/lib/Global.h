// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/25.

#ifndef POLAR_DEVLTOOLS_FILECHECKER_GLOBAL_H
#define POLAR_DEVLTOOLS_FILECHECKER_GLOBAL_H

#include "FileCheckerConfig.h"
#include "polarphp/utils/ManagedStatics.h"
#include <cstddef>
#include <vector>
#include <string>

// forward declare with namespace
namespace CLI {
class App;
} // CLI

namespace polar {
namespace filechecker {

using polar::utils::ManagedStatic;

enum class CheckType
{
   CheckNone = 0,
   CheckPlain,
   CheckNext,
   CheckSame,
   CheckNot,
   CheckDAG,
   CheckLabel,
   CheckEmpty,
   /// Indicates the pattern only matches the end of file. This is used for
   /// trailing CHECK-NOTs.
   CheckEOF,
   /// Marks when parsing found a -NOT check combined with another CHECK suffix.
   CheckBadNot
};

extern CLI::App *sg_commandParser;
CLI::App &retrieve_command_parser();
extern std::vector<std::string> sg_checkPrefixes;
extern std::vector<std::string> sg_defines;
extern std::vector<std::string> sg_implicitCheckNot;

} // filechecker
} // polar

#endif // POLAR_DEVLTOOLS_FILECHECKER_GLOBAL_H
