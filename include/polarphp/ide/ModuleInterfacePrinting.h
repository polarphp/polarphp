//===--- ModuleInterfacePrinting.h - Routines to print module interface ---===//
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

#ifndef POLARPHP_IDE_MODULE_INTERFACE_PRINTING_H
#define POLARPHP_IDE_MODULE_INTERFACE_PRINTING_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/OptionSet.h"

#include <string>
#include <vector>

namespace polar {

class AstContext;
class AstPrinter;
class ModuleDecl;
class SourceFile;
class Type;
struct PrintOptions;

namespace ide {

/// Flags used when traversing a module for printing.
enum class ModuleTraversal : unsigned {
   /// Visit modules even if their contents wouldn't be visible to name lookup.
      VisitHidden     = 0x01,
   /// Visit submodules.
      VisitSubmodules = 0x02,
   /// Skip the declarations in a Swift overlay module.
      SkipOverlay     = 0x04,
};

/// Options used to describe the traversal of a module for printing.
using ModuleTraversalOptions = OptionSet<ModuleTraversal>;

ArrayRef<StringRef> collectModuleGroups(ModuleDecl *M,
                                        std::vector<StringRef> &Scratch);

Optional<StringRef>
findGroupNameForUSR(ModuleDecl *M, StringRef USR);

bool printTypeInterface(ModuleDecl *M, Type Ty, AstPrinter &Printer,
                        std::string &TypeName, std::string &Error);

bool printTypeInterface(ModuleDecl *M, StringRef TypeUSR, AstPrinter &Printer,
                        std::string &TypeName, std::string &Error);

void printModuleInterface(ModuleDecl *M, Optional<StringRef> Group,
                          ModuleTraversalOptions TraversalOptions,
                          AstPrinter &Printer, const PrintOptions &Options,
                          const bool PrintSynthesizedExtensions);

// FIXME: this API should go away when Swift can represent Clang submodules as
// 'swift::ModuleDecl *' objects.
void printSubmoduleInterface(ModuleDecl *M, ArrayRef<StringRef> FullModuleName,
                             ArrayRef<StringRef> GroupNames,
                             ModuleTraversalOptions TraversalOptions,
                             AstPrinter &Printer, const PrintOptions &Options,
                             const bool PrintSynthesizedExtensions);

/// Print the interface for a header that has been imported via the implicit
/// objc header importing feature.
void printHeaderInterface(StringRef Filename, AstContext &Ctx,
                          AstPrinter &Printer, const PrintOptions &Options);


/// Print the interface for a given swift source file.
void printPHPSourceInterface(SourceFile &File, AstPrinter &Printer,
                               const PrintOptions &Options);

} // namespace ide
} // namespace polar

#endif // POLARPHP_IDE_MODULE_INTERFACE_PRINTING_H
