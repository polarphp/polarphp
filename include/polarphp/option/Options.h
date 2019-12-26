//===--- Options.h - Option info & table ------------------------*- C++ -*-===//
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

#ifndef POLARPHP_OPTION_OPTIONS_H
#define POLARPHP_OPTION_OPTIONS_H

#include <memory>

namespace llvm::opt {
class OptTable;
} // llvm::opt

namespace polar::options {

/// Flags specifically for Swift driver options.  Must not overlap with
/// llvm::opt::DriverFlag.
enum PolarphpFlags
{
   FrontendOption = (1 << 4),
   NoDriverOption = (1 << 5),
   NoInteractiveOption = (1 << 6),
   NoBatchOption = (1 << 7),
   DoesNotAffectIncrementalBuild = (1 << 8),
   AutolinkExtractOption = (1 << 9),
   ModuleWrapOption = (1 << 10),
   PolarphpIndentOption = (1 << 11),
   ArgumentIsPath = (1 << 12),
   ModuleInterfaceOption = (1 << 13),
};

enum FileTypeId {
   OPT_INVALID = 0, // This is not an option ID.
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
   HELPTEXT, METAVAR, VALUES)                                      \
   OPT_##ID,
#include "polarphp/option/OptionsDef.h"
   LastOption
#undef OPTION
};

} // polar::options

namespace polar {
std::unique_ptr<llvm::opt::OptTable> create_polarphp_opt_table();
} // polar

#endif // POLARPHP_OPTION_OPTIONS_H
