//===--- PrettyStackTrace.h - Crash trace information -----------*- C++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
// This file defines RAII classes that give better diagnostic output
// about when, exactly, a crash is occurring.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PRETTYSTACKTRACE_H
#define POLARPHP_AST_PRETTYSTACKTRACE_H

#include "llvm/Support/PrettyStackTrace.h"
#include "polarphp/parser/SourceLoc.h"

namespace polar::ast {

using llvm::PrettyStackTraceEntry;
using llvm::raw_ostream;
using polar::parser::SourceLoc;

class AstContext;

} // polar::ast

#endif // POLARPHP_AST_PRETTYSTACKTRACE_H
