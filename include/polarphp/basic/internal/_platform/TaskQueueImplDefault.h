// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/30.

#ifndef POLARPHP_BASIC_INTERNAL_TASKQUEUE_IMPL_H
#define POLARPHP_BASIC_INTERNAL_TASKQUEUE_IMPL_H

#include "llvm/ADT/ArrayRef.h"

namespace polar::sys {

using llvm::ArrayRef;

// Platform-independent implementation of Task,
// a particular platform can provide its own more efficient version.
class Task {
public:
   /// The path to the executable which this Task will execute.
   const char *ExecPath;

   /// Any arguments which should be passed during execution.
   ArrayRef<const char *> Args;

   /// The environment which should be used during execution. If empty,
   /// the current process's environment will be used instead.
   ArrayRef<const char *> Env;

   /// Context associated with this Task.
   void *Context;

   /// True if the errors of the Task should be stored in Errors instead of
   /// Output.
   bool SeparateErrors;

   SmallString<64> StdoutPath;

   SmallString<64> StderrPath;

   llvm::sys::ProcessInfo PI;

   Task(const char *ExecPath, ArrayRef<const char *> Args,
        ArrayRef<const char *> Env = None, void *Context = nullptr,
        bool SeparateErrors = false)
      : ExecPath(ExecPath), Args(Args), Env(Env), Context(Context),
        SeparateErrors(SeparateErrors) {}

   /// Begins execution of this Task.
   /// \returns true on error.
   bool execute();
};

} // polar::sys

#endif // POLARPHP_BASIC_INTERNAL_TASKQUEUE_IMPL_H
