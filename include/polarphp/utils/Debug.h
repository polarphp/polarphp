// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

#ifndef POLARPHP_UTILS_DEBUG_H
#define POLARPHP_UTILS_DEBUG_H

#include "Stream.h"

namespace polar {

inline std::ostream &debug_stream()
{
   return error_stream();
}

} // polar

#endif // POLARPHP_UTILS_DEBUG_H
