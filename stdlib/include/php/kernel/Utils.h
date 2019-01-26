// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/01/26.

#ifndef POLARPHP_STDLIB_KERNEL_UTILS_H
#define POLARPHP_STDLIB_KERNEL_UTILS_H

#include <string>

namespace php {
namespace kernel {

std::string retrieve_version_str();
int retrieve_major_version();
int retrieve_minor_version();
int retrieve_patch_version();
int retrieve_version_id();

} // kernel
} // php

#endif // POLARPHP_STDLIB_KERNEL_UTILS_H
