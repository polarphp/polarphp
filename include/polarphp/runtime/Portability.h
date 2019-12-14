//===--- Portability.h ------------------------------------------*- C++ -*-===//
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
// Includes stub APIs that make the portable runtime easier to write.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_RUNTIME_PORTABILITY_H
#define POLARPHP_RUNTIME_PORTABILITY_H
#include <cstddef>

size_t _polarphp_strlcpy(char *dst, const char *src, size_t maxlen);

#endif
