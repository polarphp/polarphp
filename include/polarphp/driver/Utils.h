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

#ifndef POLARPHP_DRIVER_UTILS_H
#define POLARPHP_DRIVER_UTILS_H

#include "polarphp/basic/FileTypes.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm::opt {
class Arg;
} // end namespace llvm::opt

namespace polar::driver {

/// An input argument from the command line and its inferred type.
using InputPair = std::pair<filetypes::FileTypeId, const llvm::opt::Arg *>;
/// Type used for a list of input arguments.
using InputFileList = SmallVector<InputPair, 16>;

enum class LinkKind
{
   None,
   Executable,
   DynamicLibrary,
   StaticLibrary
};

/// Used by a Job to request a "filelist": a file containing a list of all
/// input or output files of a certain type.
///
/// The Compilation is responsible for generating this file before running
/// the Job this info is attached to.
struct FilelistInfo
{
   enum class WhichFiles : unsigned
   {
      Input,
      PrimaryInputs,
      Output,
      /// Batch mode frontend invocations may have so many supplementary
      /// outputs that they don't comfortably fit as command-line arguments.
      /// In that case, add a FilelistInfo to record the path to the file.
      /// The type is ignored.
      SupplementaryOutput,
   };

   StringRef path;
   filetypes::FileTypeId type;
   WhichFiles whichFiles;
};

} // polar::driver

#endif // POLARPHP_DRIVER_UTILS_H
