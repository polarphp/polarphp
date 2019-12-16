// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_BASIC_SUPPLEMENTARYOUTPUTPATHS_H
#define POLARPHP_BASIC_SUPPLEMENTARYOUTPUTPATHS_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/Optional.h"

namespace polar {

struct SupplementaryOutputPaths
{

   /// The path to which we should emit a serialized module.
   /// It is valid whenever there are any inputs.
   ///
   /// This binary format is used to describe the interface of a module when
   /// imported by client source code. The polarmodule format is described in
   /// docs/Serialization.rst.
   ///
   /// \sa polar::serialize
   std::string moduleOutputPath;

   /// The path to which we should emit a module documentation file.
   /// It is valid whenever there are any inputs.
   ///
   /// This binary format stores doc comments and other information about the
   /// declarations in a module.
   ///
   /// \sa polar::serialize
   std::string moduleDocOutputPath;

   /// The path to which we should output a Make-style dependencies file.
   /// It is valid whenever there are any inputs.
   ///
   /// polar's compilation model means that Make-style dependencies aren't
   /// well-suited to model fine-grained dependencies. See docs/Driver.md for
   /// more information.
   ///
   /// \sa ReferenceDependenciesFilePath
   std::string dependenciesFilePath;

   /// The path to which we should output a polar "reference dependencies" file.
   /// It is valid whenever there are any inputs.
   ///
   /// "Reference dependencies" track dependencies on a more fine-grained level
   /// than just "this file depends on that file". With polar's "implicit
   /// visibility" within a module, that becomes very important for any sort of
   /// incremental build. These files are consumed by the polar driver to decide
   /// whether a source file needs to be recompiled during a build. See
   /// docs/DependencyAnalysis.rst for more information.
   ///
   /// \sa polar::emitReferenceDependencies
   /// \sa DependencyGraph
   std::string referenceDependenciesFilePath;

   /// Path to a file which should contain serialized diagnostics for this
   /// frontend invocation.
   ///
   /// This uses the same serialized diagnostics format as Clang, for tools that
   /// want machine-parseable diagnostics. There's a bit more information on
   /// how clients might use this in docs/Driver.md.
   ///
   /// \sa polar::serialized_diagnostics::createConsumer
   std::string serializedDiagnosticsPath;

   /// The path to which we should output fix-its as source edits.
   ///
   /// This is a JSON-based format that is used by the migrator, but is not
   /// really vetted for anything else.
   ///
   /// \sa polar::writeEditsInJson
   std::string fixItsOutputPath;

   /// The path to which we should output a loaded module trace file.
   /// It is valid whenever there are any inputs.
   ///
   /// The file is appended to, and consists of line-delimited JSON objects,
   /// where each line is of the form `{ "name": NAME, "target": TARGET,
   /// "polarmodules": [PATH, PATH, ...] }`, representing the (real-path) PATHs
   /// to each .polarmodule that was loaded while building module NAME for target
   /// TARGET. This format is subject to arbitrary change, however.
   std::string loadedModuleTracePath;

   /// The path to which we should output a TBD file.
   ///
   /// "TBD" stands for "text-based dylib". It's a YAML-based format that
   /// describes the public ABI of a library, which clients can link against
   /// without having an actual dynamic library binary.
   ///
   /// Only makes sense when the compiler has whole-module knowledge.
   ///
   /// \sa polar::writeTBDFile
   std::string tbdPath;

   /// The path to which we should emit a parseable module interface, which can
   /// be used by a client source file to import this module.
   ///
   /// This format is similar to the binary format used for #ModuleOutputPath,
   /// but is intended to be stable across compiler versions.
   ///
   /// Currently only makes sense when the compiler has whole-module knowledge.
   ///
   /// \sa polar::emitParseableInterface
   std::string parseableInterfaceOutputPath;

   SupplementaryOutputPaths() = default;
   SupplementaryOutputPaths(const SupplementaryOutputPaths &) = default;

   bool empty() const
   {
      return moduleOutputPath.empty() &&
            moduleDocOutputPath.empty() && dependenciesFilePath.empty() &&
            referenceDependenciesFilePath.empty() &&
            serializedDiagnosticsPath.empty() && loadedModuleTracePath.empty() &&
            tbdPath.empty() && parseableInterfaceOutputPath.empty();
   }
};

} // polar

#endif // POLARPHP_BASIC_SUPPLEMENTARYOUTPUTPATHS_H
