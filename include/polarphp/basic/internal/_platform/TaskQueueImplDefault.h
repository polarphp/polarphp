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

class Task {
public:
   /// The path to the executable which this Task will execute.
   const char *execPath;

   /// Any arguments which should be passed during execution.
   ArrayRef<const char *> args;

   /// The environment which should be used during execution. If empty,
   /// the current process's environment will be used instead.
   ArrayRef<const char *> env;

   /// True if the errors of the Task should be stored in Errors instead of Output.
   bool separateErrors;

   /// context associated with this Task.
   void *context;

   Task(const char *execPath, ArrayRef<const char *> args,
        ArrayRef<const char *> env = llvm::None, void *context = nullptr, bool separateErrors = false)
      : execPath(execPath),
        args(args),
        env(env),
        separateErrors(separateErrors),
        context(context)
   {}
};

} // polar::sys

#endif // POLARPHP_BASIC_INTERNAL_TASKQUEUE_IMPL_H
