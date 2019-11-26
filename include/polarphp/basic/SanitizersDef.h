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

#ifndef SANITIZER
#error "Define SANITIZER prior to including this file!"
#endif

// SANITIZER(enum_bit, kind, name, file)

SANITIZER(0, Address,   address,    asan)
SANITIZER(1, Thread,    thread,     tsan)
SANITIZER(2, Undefined, undefined,  ubsan)
SANITIZER(3, Fuzzer,    fuzzer,     fuzzer)

#undef SANITIZER
