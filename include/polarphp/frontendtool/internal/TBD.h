//===--- TBD.h -- generates and validates TBD files -------------*- C++ -*-===//
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

#ifndef POLARPHP_INTERNAL_FRONTENDTOOL_TBD_H
#define POLARPHP_INTERNAL_FRONTENDTOOL_TBD_H

#include "polarphp/frontend/FrontendOptions.h"

namespace llvm {
class StringRef;
class Module;
}
namespace polar {
class ModuleDecl;
class FileUnit;
class FrontendOptions;
struct TBDGenOptions;

bool writeTBD(ModuleDecl *M, StringRef OutputFilename,
              const TBDGenOptions &Opts);
bool inputFileKindCanHaveTBDValidated(InputFileKind kind);
bool validateTBD(ModuleDecl *M, llvm::Module &IRModule,
                 const TBDGenOptions &opts,
                 bool diagnoseExtraSymbolsInTBD);
bool validateTBD(FileUnit *M, llvm::Module &IRModule,
                 const TBDGenOptions &opts,
                 bool diagnoseExtraSymbolsInTBD);
}

#endif // POLARPHP_INTERNAL_FRONTENDTOOL_TBD_H