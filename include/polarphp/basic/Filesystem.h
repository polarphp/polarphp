//===--- FileSystem.h - Extra helpers for manipulating files ----*- C++ -*-===//
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

#ifndef POLARPHP_BASIC_FILESYSTEM_H
#define POLARPHP_BASIC_FILESYSTEM_H

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include <system_error>

namespace llvm {
class raw_pwrite_stream;
class Twine;
}

namespace llvm::vfs {
class FileSystem;
} // llvm::vfs

namespace polar {

/// Invokes \p action with a raw_ostream that refers to a temporary file,
/// which is then renamed into place as \p outputPath when the action
/// completes.
///
/// If a temporary file cannot be created for whatever reason, \p action will
/// be invoked with a stream directly opened at \p outputPath. Otherwise, if
/// there is already a file at \p outputPath, it will not be overwritten if
/// the new contents are identical.
///
/// If the process is interrupted with a signal, any temporary file will be
/// removed.
///
/// As a special case, an output path of "-" is treated as referring to
/// stdout.
std::error_code atomically_writing_to_file(
      llvm::StringRef outputPath,
      llvm::function_ref<void(llvm::raw_pwrite_stream &)> action);

/// Moves a file from \p source to \p destination, unless there is already
/// a file at \p destination that contains the same data as \p source.
///
/// In the latter case, the file at \p source is deleted. If an error occurs,
/// the file at \p source will still be present at \p source.
std::error_code move_file_if_different(const llvm::Twine &source,
                                       const llvm::Twine &destination);

namespace vfs {
llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
get_file_or_stdin(llvm::vfs::FileSystem &filesystem,
                  const llvm::Twine &name, int64_t fileSize = -1,
                  bool requiresNullTerminator = true, bool isVolatile = false);
} // end namespace vfs
} // polar

#endif // POLARPHP_BASIC_FILESYSTEM_H
