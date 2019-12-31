//===--- ImportedModules.h -- generates the list of imported modules ------===//
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

#ifndef POLARPHP_INTERNAL_FRONTENDTOOL_IMPORTEDMODULES_H
#define POLARPHP_INTERNAL_FRONTENDTOOL_IMPORTEDMODULES_H

namespace polar {

class AstContext;
class FrontendOptions;
class ModuleDecl;

/// Emit the names of the modules imported by \c mainModule.
bool emitImportedModules(AstContext &Context, ModuleDecl *mainModule,
                         const FrontendOptions &opts);

} // end namespace polar

#endif
