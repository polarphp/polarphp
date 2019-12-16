//===--- FileSystem.h - File helpers that interact with Diags ---*- C++ -*-===//
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
// Created by polarboy on 2019/12/01.

#ifndef POLARPHP_AST_FILESYSTEM_H
#define POLARPHP_AST_FILESYSTEM_H

#include "polarphp/basic/Filesystem.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsCommon.h"

namespace polar {

/// A wrapper around swift::atomicallyWritingToFile that handles diagnosing any
/// filesystem errors and asserts the output path is nonempty.
///
/// \returns true if there were any errors, either from the filesystem
/// operations or from \p action returning true.
inline bool
withOutputFile(DiagnosticEngine &diags, StringRef outputPath,
               llvm::function_ref<bool(llvm::raw_pwrite_stream &)> action)
{
   assert(!outputPath.empty());

   bool actionFailed = false;
   std::error_code errorCode = polar::atomically_writing_to_file(
            outputPath,
            [&](llvm::raw_pwrite_stream &out) { actionFailed = action(out); });
   if (errorCode) {
      diags.diagnose(SourceLoc(), diag::error_opening_output, outputPath,
                     errorCode.message());
      return true;
   }
   return actionFailed;
}

} // polar

#endif // POLARPHP_AST_FILESYSTEM_H
