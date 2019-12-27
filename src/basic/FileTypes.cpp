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

#include "polarphp/basic/FileTypes.h"

#include "polarphp/global/NameStrings.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

using namespace polar;
using namespace polar::filetypes;

namespace {
struct TypeInfo {
   const char *Name;
   const char *Flags;
   const char *Extension;
};
} // end anonymous namespace

static const TypeInfo TypeInfos[] = {
#define TYPE(NAME, ID, EXTENSION, FLAGS) \
  { NAME, FLAGS, EXTENSION },
#include "polarphp/basic/FileTypesDef.h"
};

static const TypeInfo &getInfo(unsigned Id) {
   assert(Id >= 0 && Id < TY_INVALID && "Invalid Type ID.");
   return TypeInfos[Id];
}

StringRef filetypes::get_type_name (FileTypeId Id) { return getInfo(Id).Name; }

StringRef filetypes::get_extension(FileTypeId Id) {
   return getInfo(Id).Extension;
}

FileTypeId filetypes::lookup_type_for_extension(StringRef Ext) {
   if (Ext.empty())
      return TY_INVALID;
   assert(Ext.front() == '.' && "not a file extension");
   return llvm::StringSwitch<filetypes::FileTypeId>(Ext.drop_front())
#define TYPE(NAME, ID, EXTENSION, FLAGS) \
           .Case(EXTENSION, TY_##ID)
#include "polarphp/basic/FileTypesDef.h"
      .Default(TY_INVALID);
}

FileTypeId filetypes::lookup_type_for_name(StringRef Name) {
   return llvm::StringSwitch<filetypes::FileTypeId>(Name)
#define TYPE(NAME, ID, EXTENSION, FLAGS) \
           .Case(NAME, TY_##ID)
#include "polarphp/basic/FileTypesDef.h"
      .Default(TY_INVALID);
}

bool filetypes::is_textual(FileTypeId Id) {
   switch (Id) {
      case filetypes::TY_PHP:
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
      case filetypes::TY_PHPModuleInterfaceFile:
         return true;
      case filetypes::TY_Image:
      case filetypes::TY_Object:
      case filetypes::TY_dSYM:
      case filetypes::TY_PCH:
      case filetypes::TY_PIB:
      case filetypes::TY_RawPIB:
      case filetypes::TY_PHPModuleFile:
      case filetypes::TY_PHPModuleDocFile:
      case filetypes::TY_PHPSourceInfoFile:
      case filetypes::TY_LLVM_BC:
      case filetypes::TY_SerializedDiagnostics:
      case filetypes::TY_ClangModuleFile:
      case filetypes::TY_PHPDeps:
      case filetypes::TY_PHPRanges:
      case filetypes::TY_CompiledSource:
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

bool filetypes::is_after_llvm(FileTypeId Id) {
   switch (Id) {
      case filetypes::TY_Assembly:
      case filetypes::TY_LLVM_IR:
      case filetypes::TY_LLVM_BC:
      case filetypes::TY_Object:
         return true;
      case filetypes::TY_PHP:
      case filetypes::TY_PCH:
      case filetypes::TY_ImportedModules:
      case filetypes::TY_TBD:
      case filetypes::TY_PIL:
      case filetypes::TY_Dependencies:
      case filetypes::TY_ASTDump:
      case filetypes::TY_RawPIL:
      case filetypes::TY_AutolinkFile:
      case filetypes::TY_Image:
      case filetypes::TY_dSYM:
      case filetypes::TY_PIB:
      case filetypes::TY_RawPIB:
      case filetypes::TY_PHPModuleFile:
      case filetypes::TY_PHPModuleDocFile:
      case filetypes::TY_PHPSourceInfoFile:
      case filetypes::TY_SerializedDiagnostics:
      case filetypes::TY_ClangModuleFile:
      case filetypes::TY_PHPDeps:
      case filetypes::TY_PHPRanges:
      case filetypes::TY_CompiledSource:
      case filetypes::TY_Nothing:
      case filetypes::TY_Remapping:
      case filetypes::TY_IndexData:
      case filetypes::TY_ModuleTrace:
      case filetypes::TY_OptRecord:
      case filetypes::TY_PHPModuleInterfaceFile:
         return false;
      case filetypes::TY_INVALID:
         llvm_unreachable("Invalid type ID.");
   }

   // Work around MSVC warning: not all control paths return a value
   llvm_unreachable("All switch cases are covered");
}

bool filetypes::is_part_of_php_compilation(FileTypeId Id) {
   switch (Id) {
      case filetypes::TY_PHP:
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
      case filetypes::TY_AutolinkFile:
      case filetypes::TY_PCH:
      case filetypes::TY_ImportedModules:
      case filetypes::TY_TBD:
      case filetypes::TY_Image:
      case filetypes::TY_dSYM:
      case filetypes::TY_PHPModuleFile:
      case filetypes::TY_PHPModuleDocFile:
      case filetypes::TY_PHPModuleInterfaceFile:
      case filetypes::TY_PHPSourceInfoFile:
      case filetypes::TY_SerializedDiagnostics:
      case filetypes::TY_ClangModuleFile:
      case filetypes::TY_PHPDeps:
      case filetypes::TY_PHPRanges:
      case filetypes::TY_CompiledSource:
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
