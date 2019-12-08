//===--- DiagnosticsAll.def - Diagnostics Text Index ------------*- C++ -*-===//
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
//  This file imports all the other diagnostic files.
//
//===----------------------------------------------------------------------===//

#if !(defined(DIAG) || (defined(ERROR) && defined(WARNING) && defined(NOTE)))
#  error Must define either DIAG or the set {ERROR,WARNING,NOTE}
#endif

#ifndef ERROR
#  define ERROR(ID,Options,Text,Signature)   \
  DIAG(ERROR,ID,Options,Text,Signature)
#endif

#ifndef WARNING
#  define WARNING(ID,Options,Text,Signature) \
  DIAG(WARNING,ID,Options,Text,Signature)
#endif

#ifndef NOTE
#  define NOTE(ID,Options,Text,Signature) \
  DIAG(NOTE,ID,Options,Text,Signature)
#endif

#ifndef REMARK
#  define REMARK(ID,Options,Text,Signature) \
  DIAG(REMARK,ID,Options,Text,Signature)
#endif

#ifndef FIXIT
#  define FIXIT(ID, Text, Signature)
#endif

#define DIAG_NO_UNDEF

#include "polarphp/ast/DiagnosticsCommonDefs.h"
#include "polarphp/ast/DiagnosticsParseDefs.h"
#include "polarphp/ast/DiagnosticsSemaDefs.h"
#include "polarphp/ast/DiagnosticsClangImporterDefs.h"
#include "polarphp/ast/DiagnosticsPILDefs.h"
#include "polarphp/ast/DiagnosticsIRGenDefs.h"
#include "polarphp/ast/DiagnosticsFrontendDefs.h"
#include "polarphp/ast/DiagnosticsDriverDefs.h"
#include "polarphp/ast/DiagnosticsRefactoringDefs.h"
#include "polarphp/ast/DiagnosticsModuleDifferDefs.h"

#undef DIAG_NO_UNDEF

#if defined(DIAG)
#  undef DIAG
#endif
#undef NOTE
#undef WARNING
#undef ERROR
#undef REMARK
#undef FIXIT
