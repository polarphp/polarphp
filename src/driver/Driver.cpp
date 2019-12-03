//===--- Driver.cpp - Swift compiler driver -------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
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
//
// This file contains implementations of parts of the compiler driver.
//
//===----------------------------------------------------------------------===//


#include "polarphp/driver/Driver.h"

#include "polarphp/driver/internal/ToolChains.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsDriver.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/OutputFileMap.h"
#include "polarphp/basic/Range.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/TaskQueue.h"
#include "polarphp/kernel/Version.h"
#include "polarphp/global/Config.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Compilation.h"
#include "polarphp/driver/Job.h"
#include "polarphp/driver/PrettyStackTrace.h"
#include "polarphp/driver/ToolChain.h"
#include "polarphp/option/Options.h"
#include "polarphp/option/SanitizerOptions.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/global/NameStrings.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/MD5.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/driver/internal/CompilationRecord.h"

#include <memory>
