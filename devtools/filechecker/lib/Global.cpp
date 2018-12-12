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

#include "Global.h"
#include "CLI/CLI.hpp"
#include <cassert>

namespace polar {
namespace filechecker {

CLI::App *sg_commandParser = nullptr;
std::vector<std::string> sg_checkPrefixes{};
std::vector<std::string> sg_defines{};
std::vector<std::string> sg_implicitCheckNot;

CLI::App &retrieve_command_parser()
{
   assert(sg_commandParser != nullptr);
   return *sg_commandParser;
}

} // filechecker
} // polar
