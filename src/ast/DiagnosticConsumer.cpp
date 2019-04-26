//===--- DiagnosticConsumer.cpp - Diagnostic Consumer Impl ----------------===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the DiagnosticConsumer class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "polarphp-ast"

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/StringSet.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/Defer.h"

#include "polarphp/ast/DiagnosticConsumer.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/parser/SourceMgr.h"

namespace polar::ast {

DiagnosticConsumer::~DiagnosticConsumer()
{}

SMLocation DiagnosticConsumer::getRawLoc(SourceLoc loc)
{
   return loc.m_loc;
}

} // polar::ast
