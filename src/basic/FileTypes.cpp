//===--- FileTypes.cpp - Input & output formats used by the tools ---------===//
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

#include "polarphp/basic/FileTypes.h"
#include "polarphp/global/NameStrings.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

namespace polar::filetypes {

using filetypes::FileTypeId;

namespace {
struct TypeInfo
{
   const char *name;
   const char *flags;
   const char *extension;
};
} // end anonymous namespace

static const TypeInfo TypeInfos[] =
{
   #define TYPE(NAME, ID, EXTENSION, FLAGS) \
{ NAME, FLAGS, EXTENSION },
   #include "polarphp/basic/FileTypesDef.h"
};

static const TypeInfo &getInfo(unsigned id)
{
   assert(id >= 0 && id < TY_INVALID && "Invalid Type ID.");
   return TypeInfos[id];
}

StringRef get_type_name(FileTypeId id)
{
   return getInfo(id).name;
}

StringRef get_extension(FileTypeId id)
{
   return getInfo(id).extension;
}

FileTypeId lookup_type_for_extension(StringRef ext)
{
   if (ext.empty()) {
      return TY_INVALID;
   }
   assert(ext.front() == '.' && "not a file extension");
   return llvm::StringSwitch<FileTypeId>(ext.drop_front())
      #define TYPE(NAME, ID, EXTENSION, FLAGS) \
         .Case(EXTENSION, TY_##ID)
      #include "polarphp/basic/FileTypesDef.h"
         .Default(TY_INVALID);
}

FileTypeId lookup_type_for_name(StringRef name)
{
   return llvm::StringSwitch<FileTypeId>(name)
      #define TYPE(NAME, ID, EXTENSION, FLAGS) \
         .Case(NAME, TY_##ID)
      #include "polarphp/basic/FileTypesDef.h"
         .Default(TY_INVALID);
}

bool is_textual(FileTypeId id)
{
   switch (id) {
   case filetypes::TY_Polar:
   case filetypes::TY_PIL:
   case filetypes::TY_Dependencies:
   case filetypes::TY_Assembly:
   case filetypes::TY_ASTDump:
   case filetypes::TY_RawPIL:
   case filetypes::TY_LLVM_IR:
   case filetypes::TY_AutolinkFile:
   case filetypes::TY_ImportedModules:
   case filetypes::TY_TBD:
   case filetypes::TY_ModuleTrace:
   case filetypes::TY_OptRecord:
   case filetypes::TY_PolarParseableInterfaceFile:
      return true;
   case filetypes::TY_Image:
   case filetypes::TY_Object:
   case filetypes::TY_dSYM:
   case filetypes::TY_PCH:
   case filetypes::TY_PIB:
   case filetypes::TY_RawPIB:
   case filetypes::TY_PolarModuleFile:
   case filetypes::TY_PolarModuleDocFile:
   case filetypes::TY_LLVM_BC:
   case filetypes::TY_SerializedDiagnostics:
   case filetypes::TY_ClangModuleFile:
   case filetypes::TY_PolarDeps:
   case filetypes::TY_Nothing:
   case filetypes::TY_Remapping:
   case filetypes::TY_IndexData:
      return false;
   case filetypes::TY_INVALID:
      llvm_unreachable("Invalid type ID.");
   }

   // Work around MSVC warning: not all control paths return a value
   llvm_unreachable("All switch cases are covered");
}

bool is_after_llvm(FileTypeId id) {
   switch (id) {
   case filetypes::TY_Assembly:
   case filetypes::TY_LLVM_IR:
   case filetypes::TY_LLVM_BC:
   case filetypes::TY_Object:
      return true;
   case filetypes::TY_Polar:
   case filetypes::TY_PCH:
   case filetypes::TY_ImportedModules:
   case filetypes::TY_TBD:
   case filetypes::TY_PIL:
   case filetypes::TY_Dependencies:
   case filetypes::TY_ASTDump:
   case filetypes::TY_RawPIL:
   case filetypes::TY_ObjCHeader:
   case filetypes::TY_AutolinkFile:
   case filetypes::TY_Image:
   case filetypes::TY_dSYM:
   case filetypes::TY_PIB:
   case filetypes::TY_RawPIB:
   case filetypes::TY_PolarModuleFile:
   case filetypes::TY_PolarModuleDocFile:
   case filetypes::TY_SerializedDiagnostics:
   case filetypes::TY_ClangModuleFile:
   case filetypes::TY_PolarDeps:
   case filetypes::TY_Nothing:
   case filetypes::TY_Remapping:
   case filetypes::TY_IndexData:
   case filetypes::TY_ModuleTrace:
   case filetypes::TY_OptRecord:
   case filetypes::TY_PolarParseableInterfaceFile:
      return false;
   case filetypes::TY_INVALID:
      llvm_unreachable("Invalid type ID.");
   }

   // Work around MSVC warning: not all control paths return a value
   llvm_unreachable("All switch cases are covered");
}

bool is_part_of_polar_compilation(FileTypeId id)
{
   switch (id) {
   case filetypes::TY_Polar:
   case filetypes::TY_PIL:
   case filetypes::TY_RawPIL:
   case filetypes::TY_PIB:
   case filetypes::TY_RawPIB:
      return true;
   case filetypes::TY_Assembly:
   case filetypes::TY_LLVM_IR:
   case filetypes::TY_LLVM_BC:
   case filetypes::TY_Object:
   case filetypes::TY_Dependencies:
   case filetypes::TY_ObjCHeader:
   case filetypes::TY_AutolinkFile:
   case filetypes::TY_PCH:
   case filetypes::TY_ImportedModules:
   case filetypes::TY_TBD:
   case filetypes::TY_Image:
   case filetypes::TY_dSYM:
   case filetypes::TY_PolarModuleFile:
   case filetypes::TY_PolarModuleDocFile:
   case filetypes::TY_PolarParseableInterfaceFile:
   case filetypes::TY_SerializedDiagnostics:
   case filetypes::TY_ClangModuleFile:
   case filetypes::TY_PolarDeps:
   case filetypes::TY_Nothing:
   case filetypes::TY_ASTDump:
   case filetypes::TY_Remapping:
   case filetypes::TY_IndexData:
   case filetypes::TY_ModuleTrace:
   case filetypes::TY_OptRecord:
      return false;
   case filetypes::TY_INVALID:
      llvm_unreachable("Invalid type ID.");
   }

   // Work around MSVC warning: not all control paths return a value
   llvm_unreachable("All switch cases are covered");
}

} // polar::basic
