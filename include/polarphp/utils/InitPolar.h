// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/21.

#ifndef POLARPHP_UTILS_INIT_POLAR_H
#define POLARPHP_UTILS_INIT_POLAR_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/PrettyStackTrace.h"

// forward declare class with namespace
namespace CLI {
class App;
} // CLI

namespace polar {

// The main() functions in typical polarphp tools start with InitPolar which does
// the following one-time initializations:
//
//  1. Setting up a signal handler so that pretty stack trace is printed out
//     if a process crashes.
//
//  2. If running on Windows, obtain command line arguments using a
//     multibyte character-aware API and convert arguments into UTF-8
//     encoding, so that you can assume that command line arguments are
//     always encoded in UTF-8 on any platform.
//
// InitPolar calls managed_statics_shutdown() on destruction, which cleans up
// ManagedStatic objects.

class InitPolar {
public:
   InitPolar(int &argc, const char **&argv);
   InitPolar(int &argc, char **&argv)
      : InitPolar(argc, const_cast<const char **&>(argv))
   {}
   void initNgOpts(CLI::App &parser);
   ~InitPolar();
private:
   llvm::BumpPtrAllocator m_alloc;
   llvm::SmallVector<const char *, 0> m_args;
   llvm::PrettyStackTraceProgram m_stackPrinter;
};

} // polar

#endif // POLARPHP_UTILS_INIT_POLAR_H
