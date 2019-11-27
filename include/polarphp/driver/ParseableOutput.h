//===--- ParseableOutput.h - Helpers for parseable output -------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_PARSEABLE_OUTPUT_H
#define POLARPHP_PARSEABLE_OUTPUT_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/TaskQueue.h"

namespace polar::driver {

class Job;

namespace parseable_output {

/// Emits a "began" message to the given stream.
void emit_began_message(raw_ostream &os, const Job &job, int64_t pid,
                        sys::TaskProcessInformation procInfo);

/// Emits a "finished" message to the given stream.
void emit_finished_message(raw_ostream &os, const Job &job, int64_t pid,
                           int exitStatus, StringRef output,
                           sys::TaskProcessInformation procInfo);

/// Emits a "signalled" message to the given stream.
void emit_signalled_message(raw_ostream &os, const Job &job, int64_t pid,
                            StringRef errorMsg, StringRef output,
                            Optional<int> signal,
                            sys::TaskProcessInformation procInfo);

/// Emits a "skipped" message to the given stream.
void emit_skipped_message(raw_ostream &os, const Job &job);

} // polar::driver

#endif // POLARPHP_PARSEABLE_OUTPUT_H
