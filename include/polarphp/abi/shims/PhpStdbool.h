//===------------------------------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_STDLIB_SHIMS_POLARPHPSTDBOOL_H_
#define POLARPHP_STDLIB_SHIMS_POLARPHPSTDBOOL_H_

#ifdef __cplusplus
typedef bool __polarphp_bool;
#else
typedef _Bool __polarphp_bool;
#endif

#endif

