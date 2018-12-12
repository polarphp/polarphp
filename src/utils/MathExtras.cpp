// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/11.

#include "polarphp/utils/MathExtras.h"
#ifdef _MSC_VER
#include <limits>
#else
#include <math.h>
#endif

namespace polar {
namespace utils {

#if defined(_MSC_VER)
// Visual Studio defines the HUGE_VAL class of macros using purposeful
// constant arithmetic overflow, which it then warns on when encountered.
const float huge_valf = std::numeric_limits<float>::infinity();
#else
const float huge_valf = HUGE_VALF;
#endif

} // utils
} // polar
