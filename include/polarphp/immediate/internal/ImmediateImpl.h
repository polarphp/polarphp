//===--- ImmediateImpl.h - Support functions for immediate mode -*- C++ -*-===//
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

#ifndef POLARPHP_INTERNAL_IMMEDIATEIMPL_H
#define POLARPHP_INTERNAL_IMMEDIATEIMPL_H

#include "polarphp/ast/LinkLibrary.h"
#include "polarphp/ast/SearchPathOptions.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {
class Function;
class Module;
}

namespace polar {
class CompilerInstance;
class DiagnosticEngine;
class IRGenOptions;
class ModuleDecl;
class PILOptions;

namespace immediate {

/// Returns a handle to the runtime suitable for other \c dlsym or \c dlclose
/// calls or \c null if an error occurred.
///
/// \param runtimeLibPaths Paths to search for stdlib dylibs.
void *loadPHPRuntime(ArrayRef<std::string> runtimeLibPaths);
bool tryLoadLibraries(ArrayRef<LinkLibrary> LinkLibraries,
                      SearchPathOptions SearchPathOpts,
                      DiagnosticEngine &Diags);
bool linkLLVMModules(llvm::Module *Module,
                     std::unique_ptr<llvm::Module> SubModule);
bool autolinkImportedModules(ModuleDecl *M, IRGenOptions &IRGenOpts);

} // end namespace immediate
} // end namespace polar

#endif // POLARPHP_INTERNAL_IMMEDIATEIMPL_H

