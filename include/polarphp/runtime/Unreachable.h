//===--- Unreachable.h - Implements swift_runtime_unreachable ---*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/29.
//
//===----------------------------------------------------------------------===//
//
//  This file defines swift_runtime_unreachable, an LLVM-independent
//  implementation of llvm_unreachable.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_RUNTIME_UNREACHABLE_H
#define POLARPHP_RUNTIME_UNREACHABLE_H

#include <assert.h>
#include <stdlib.h>

[[noreturn]]
inline static void polarphp_runtime_unreachable(const char *msg)
{
   assert(false && msg);
   (void)msg;
   abort();
}

#endif // POLARPHP_RUNTIME_UNREACHABLE_H
