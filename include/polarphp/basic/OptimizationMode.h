// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_BASIC_OPTIMIZATION_MODE_H
#define POLARPHP_BASIC_OPTIMIZATION_MODE_H

#include "polarphp/basic/InlineBitfield.h"
#include "llvm/Support/DataTypes.h"

namespace polar {
// The optimization mode specified on the command line or with function
// attributes.
enum class OptimizationMode : uint8_t
{
   NotSet = 0,
   NoOptimization = 1,  // -Onone
   ForSpeed = 2,        // -Ospeed == -O
   ForSize = 3,         // -Osize
   LastMode = ForSize
};

enum : unsigned
{
   NumOptimizationModeBits =
   count_bits_used(static_cast<unsigned>(OptimizationMode::LastMode))
};

} // polar

#endif // POLARPHP_BASIC_OPTIMIZATION_MODE_H
