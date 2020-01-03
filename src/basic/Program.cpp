//===--- Program.cpp - Implement OS Program Concept -----------------------===//
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
// Created by polarboy on 2019/12/02.

#include "polarphp/basic/Program.h"
#include "polarphp/global/Config.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Program.h"

#ifdef LLVM_ON_UNIX
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

namespace polar {

int execute_in_place(const char *program, const char **args,
                     const char **env) {
#if LLVM_ON_UNIX
   int result;
   if (env) {
      result = execve(program, const_cast<char **>(args),
                      const_cast<char **>(env));
   } else {
      result = execv(program, const_cast<char **>(args));
   }
   return result;
#else
   llvm::Optional<llvm::ArrayRef<llvm::StringRef>> envOpt = llvm::None;
   if (env) {
      envOpt = llvm::toStringRefArray(env);
   }
   int result =
         llvm::sys::ExecuteAndWait(Program, llvm::toStringRefArray(args), envOpt);
   if (result >= 0) {
      exit(result);
   }
   return result;
#endif
}

} // polar
