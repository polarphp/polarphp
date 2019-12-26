//===--- PrimarySpecificPaths.h ---------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_BASIC_PRIMARYSPECIFICPATHS_H
#define POLARPHP_BASIC_PRIMARYSPECIFICPATHS_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/SupplementaryOutputPaths.h"
#include "llvm/ADT/StringRef.h"

#include <string>

namespace polar {

/// Holds all of the output paths, and debugging-info path that are
/// specific to which primary file is being compiled at the moment.
///
class PrimarySpecificPaths
{
public:
   /// The name of the main output file,
   /// that is, the .o file for this input (or a file specified by -o).
   /// If there is no such file, contains an empty string. If the output
   /// is to be written to stdout, contains "-".
   std::string outputFilename;

   SupplementaryOutputPaths supplementaryOutputs;

   /// The name of the "main" input file, used by the debug info.
   std::string mainInputFilenameForDebugInfo;

   PrimarySpecificPaths(StringRef filename = StringRef(),
                        StringRef debugInfo = StringRef(),
                        SupplementaryOutputPaths outputs =
         SupplementaryOutputPaths())
      : outputFilename(filename),
        supplementaryOutputs(outputs),
        mainInputFilenameForDebugInfo(debugInfo)
   {}

   bool haveModuleOrModuleDocOutputPaths() const
   {
      return !supplementaryOutputs.ModuleOutputPath.empty() ||
            !supplementaryOutputs.ModuleDocOutputPath.empty();
   }
};

} // polar

#endif // POLARPHP_BASIC_PRIMARYSPECIFICPATHS_H

