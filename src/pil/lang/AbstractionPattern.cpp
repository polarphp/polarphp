//===--- AbstractionPattern.cpp - Abstraction patterns --------------------===//
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
// This file defines routines relating to abstraction patterns.
// working in concert with the Clang importer.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "libpil"

#include "polarphp/pil/lang/AbstractionPattern.h"
//#include "polarphp/pil/TypeLowering.h"

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/ForeignErrorConvention.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/ModuleLoader.h"
#include "clang/AST/AstContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/PrettyPrinter.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"