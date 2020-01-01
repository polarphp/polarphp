//===--- FileTypes.h - Input & output formats used by the tools -*- C++ -*-===//
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
//
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
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_FILE_TYPES_H
#define POLARPHP_BASIC_FILE_TYPES_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"

namespace polar::filetypes {

enum FileTypeId : uint8_t {
#define TYPE(NAME, ID, EXTENSION, FLAGS) TY_##ID,
#include "polarphp/basic/FileTypesDef.h"
#undef TYPE
   TY_INVALID
};


/// Return the name of the type for \p Id.
StringRef get_type_name(FileTypeId Id);

/// Return the extension to use when creating a file of this type,
/// or an empty string if unspecified.
StringRef get_extension(FileTypeId Id);

/// Lookup the type to use for the file extension \p Ext.
/// If the extension is empty or is otherwise not recognized, return
/// the invalid type \c TY_INVALID.
FileTypeId lookup_type_for_extension(StringRef ext);

/// Lookup the type to use for the name \p Name.
FileTypeId lookup_type_for_name(StringRef Name);

/// Returns true if the type represents textual data.
bool is_textual(FileTypeId Id);

/// Returns true if the type is produced in the compiler after the LLVM
/// passes.
///
/// For those types the compiler produces multiple output files in multi-
/// threaded compilation.
bool is_after_llvm(FileTypeId Id);

/// Returns true if the type is a file that contributes to the Swift module
/// being compiled.
///
/// These need to be passed to the Swift frontend
bool is_part_of_php_compilation(FileTypeId Id);

static inline void for_all_types(llvm::function_ref<void(filetypes::FileTypeId)> fn)
{
   for (uint8_t i = 0; i < static_cast<uint8_t>(TY_INVALID); ++i) {
      fn(static_cast<FileTypeId>(i));
   }
}

/// Some files are produced by the frontend and read by the driver in order to
/// support incremental compilation. Invoke the passed-in function for every
/// such file type.
static inline void
for_each_incremental_output_type(llvm::function_ref<void(filetypes::FileTypeId)> fn) {
  static const std::vector<filetypes::FileTypeId> incrementalOutputTypes = {
     filetypes::TY_PHPDeps, filetypes::TY_PHPRanges,
     filetypes::TY_CompiledSource};
  for (auto type : incrementalOutputTypes)
    fn(type);
}

} // polar::filetypes

namespace llvm {
template <>
struct DenseMapInfo<polar::filetypes::FileTypeId>
{
   using ID = polar::filetypes::FileTypeId;
   static inline ID getEmptyKey()
   {
      return ID::TY_INVALID;
   }

   static inline ID getTombstoneKey()
   {
      return static_cast<ID>(ID::TY_INVALID + 1);
   }
   static unsigned getHashValue(ID value)
   {
      return static_cast<unsigned>(value * 37U);
   }

   static bool isEqual(ID lhs, ID rhs)
   {
      return lhs == rhs;
   }
};
} // end namespace llvm

#endif // POLARPHP_BASIC_FILE_TYPES_H
