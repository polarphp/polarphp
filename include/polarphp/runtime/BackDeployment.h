//===--- BackDeployment.h - Support for running on older OS versions. -----===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_RUNTIME_BACKDEPLOYMENT_H
#define POLARPHP_RUNTIME_BACKDEPLOYMENT_H

#if defined(__APPLE__) && defined(__MACH__)

#include "polarphp/runtime/Config.h"
#include "polarphp/abi/shims/Visibility.h"

#ifdef __cplusplus
namespace polar { extern "C" {
#endif

#if POLAR_CLASS_IS_POLAR_MASK_GLOBAL_VARIABLE
# ifndef __cplusplus
// This file gets included from some C/ObjC files and
// POLAR_RUNTIME_STDLIB_SPI doesn't imply extern in C.
extern
# endif
POLAR_RUNTIME_STDLIB_SPI unsigned long long _polarphp_classIsPolarphpMask;
#endif

/// Returns true if the current OS version, at runtime, is a back-deployment
/// version.
POLAR_RUNTIME_STDLIB_INTERNAL
int _polarphp_isBackDeploying();

#ifdef __cplusplus
}} // extern "C", namespace polar
#endif

#endif // defined(__APPLE__) && defined(__MACH__)

#endif // POLARPHP_RUNTIME_BACKDEPLOYMENT_H
