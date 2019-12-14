//===--- Exclusivity.h - Swift exclusivity-checking support -----*- C++ -*-===//
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
// Swift runtime support for dynamic checking of the Law of Exclusivity.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_RUNTIME_EXCLUSIVITY_H
#define POLARPHP_RUNTIME_EXCLUSIVITY_H

#include <cstdint>

#include "polarphp/runtime/Config.h"

namespace polar:runtime {

enum class ExclusivityFlags : uintptr_t;
template <typename Runtime> struct TargetValueBuffer;
struct InProcess;
using ValueBuffer = TargetValueBuffer<InProcess>;

/// Begin dynamically tracking an access.
///
/// The buffer is opaque scratch space that the runtime may use for
/// the duration of the access.
///
/// The PC argument is an instruction pointer to associate with the start
/// of the access.  If it is null, the return address of the call to
/// polarphp_beginAccess will be used.
POLAR_RUNTIME_EXPORT
void polarphp_beginAccess(void *pointer, ValueBuffer *buffer,
                          ExclusivityFlags flags, void *pc);

/// Loads the replacement function pointer from \p ReplFnPtr and returns the
/// replacement function if it should be called.
/// Returns null if the original function (which is passed in \p CurrFn) should
/// be called.
#ifdef __APPLE__
__attribute__((weak_import))
#endif
POLAR_RUNTIME_EXPORT
char *polarphp_getFunctionReplacement(char **ReplFnPtr, char *CurrFn);

/// Returns the original function of a replaced function, which is loaded from
/// \p OrigFnPtr.
/// This function is called from a replacement function to call the original
/// function.
#ifdef __APPLE__
__attribute__((weak_import))
#endif
POLAR_RUNTIME_EXPORT
char *polarphp_getOrigOfReplaceable(char **OrigFnPtr);

/// Stop dynamically tracking an access.
POLAR_RUNTIME_EXPORT
void polarphp_endAccess(ValueBuffer *buffer);

/// A flag which, if set, causes access tracking to be suspended.
/// Accesses which begin while this flag is set will not be tracked,
/// will not cause exclusivity failures, and do not need to be ended.
///
/// This is here to support tools like debuggers.  Debuggers need to
/// be able to run code at breakpoints that does things like read
/// from a variable while there are ongoing formal accesses to it.
/// Such code may also crash, and we need to be able to recover
/// without leaving various objects in a permanent "accessed"
/// state.  (We also need to not leave references to scratch
/// buffers on the stack sitting around in the runtime.)
POLAR_RUNTIME_EXPORT
bool _polarphp_disableExclusivityChecking;

#ifndef NDEBUG

/// Dump all accesses currently tracked by the runtime.
///
/// This is a debug routine that is intended to be used from the debugger and is
/// compiled out when asserts are disabled. The intention is that it allows one
/// to dump the access state to easily see if/when exclusivity violations will
/// happen. This eases debugging.
POLAR_RUNTIME_EXPORT
void polarphp_dumpTrackedAccesses();

#endif

} // end namespace swift

#endif
