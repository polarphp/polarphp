// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/03.

#include "polarphp/basic/adt/Hashing.h"

namespace polar {
namespace basic {

// Provide a definition and static initializer for the fixed seed. This
// initializer should always be zero to ensure its value can never appear to be
// non-zero, even during dynamic initialization.
uint64_t hashing::internal::fixed_seed_override = 0;

// Implement the function for forced setting of the fixed seed.
// FIXME: Use atomic operations here so that there is no data race.
void set_fixed_execution_hash_seed(uint64_t fixedValue)
{
   hashing::internal::fixed_seed_override = fixedValue;
}

} // basic
} // polar
