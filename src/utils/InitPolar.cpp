// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/21.

#include "polarphp/utils/InitPolar.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/PrettyStackTrace.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/Signals.h"
#include "polarphp/utils/ManagedStatics.h"
#include "CLI/CLI.hpp"
#include <string>

#ifdef _WIN32
#include "Windows/WindowsSupport.h"
#endif

namespace polar {

InitPolar::InitPolar(int &argc, const char **&argv)
   : m_stackPrinter(argc, argv)
{
   utils::print_stack_trace_on_error_signal(argv[0]);

#ifdef _WIN32
   // We use UTF-8 as the internal character encoding. On Windows,
   // arguments passed to main() may not be encoded in UTF-8. In order
   // to reliably detect encoding of command line arguments, we use an
   // Windows API to obtain arguments, convert them to UTF-8, and then
   // write them back to the argv vector.
   //
   // There's probably other way to do the same thing (e.g. using
   // wmain() instead of main()), but this way seems less intrusive
   // than that.
   std::string Banner = std::string(argv[0]) + ": ";
   ExitOnError ExitOnErr(Banner);

   ExitOnErr(errorCodeToError(windows::GetCommandLineArguments(args, Alloc)));

   // GetCommandLineArguments doesn't terminate the vector with a
   // nullptr.  Do it to make it compatible with the real argv.
   args.push_back(nullptr);

   argc = args.size() - 1;
   argv = args.data();
#endif
}

void InitPolar::initNgOpts(CLI::App &parser)
{

}

InitPolar::~InitPolar()
{
   utils::managed_statics_shutdown();
}

} // polar
