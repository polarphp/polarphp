//===--- Types.def - Driver Type info ---------------------------*- C++ -*-===//
//
// This source file is part of the swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the driver type information. Users of this file
// must define the TYPE macro to make use of this information.
//
//===----------------------------------------------------------------------===//

#ifndef TYPE
#error "Define TYPE prior to including this file!"
#endif

// TYPE(NAME, ID, SUFFIX, FLAGS)

// The first value is the type name as a string; this should be something which
// could be displayed to the user, or something which the user could provide.

// The second value is the type id, which will result in a
// swift::driver::file_types::TY_XX enum constant.

// The third value is the extension to use when creating temporary files
// of this type. It is also used when inferring a type from an extension.
// If multiple types specify the same extension, the first one is chosen when
// performing type inference.

// The fourth value is a string containing option flags. For now, this is
// unused, and should always be the empty string.

// Input types
TYPE("php",                 PHP,                       "php",             "")
TYPE("pil",                 PIL,                       "pil",             "")
TYPE("pib",                 PIB,                       "pib",             "")

// Output types
TYPE("ast-dump",            ASTDump,                   "ast",             "")
TYPE("image",               Image,                     "out",             "")
TYPE("object",              Object,                    "o",               "")
TYPE("dSYM",                dSYM,                      "dSYM",            "")
TYPE("dependencies",        Dependencies,              "d",               "")
TYPE("autolink",            AutolinkFile,              "autolink",        "")
TYPE("phpmodule",           PHPModuleFile,             "phpmodule",       "")
TYPE("phpdoc",              PHPModuleDocFile,          "phpdoc",          "")
TYPE("phpinterface",        PHPModuleInterfaceFile,    "phpinterface",    "")
TYPE("phpsourceinfo",       PHPSourceInfoFile,         "phpsourceinfo",   "")
TYPE("assembly",            Assembly,                  "s",               "")
TYPE("raw-pil",             RawPIL,                    "pil",             "")
TYPE("raw-pib",             RawPIB,                    "pib",             "")
TYPE("llvm-ir",             LLVM_IR,                   "ll",              "")
TYPE("llvm-bc",             LLVM_BC,                   "bc",              "")
TYPE("diagnostics",         SerializedDiagnostics,     "dia",             "")
TYPE("php-dependencies",    PHPDeps,                   "phpdeps",         "")
TYPE("php-ranges",          PHPRanges,                 "phpranges",       "")
TYPE("compiled-source",     CompiledSource,            "compiledsource",  "")
TYPE("remap",               Remapping,                 "remap",           "")
TYPE("imported-modules",    ImportedModules,           "importedmodules", "")
TYPE("tbd",                 TBD,                       "tbd",             "")

// Module traces are used by Apple's internal build infrastructure. Apple
// engineers can see more details on the "Polarphp module traces" page in the
// Polarphp section of the internal wiki.
TYPE("module-trace",        ModuleTrace,               "trace.json",      "")

TYPE("index-data",          IndexData,                 "",                "")
TYPE("opt-record",          OptRecord,                 "opt.yaml",        "")

// Misc types
TYPE("pcm",                 ClangModuleFile,           "pcm",             "")
TYPE("pch",                 PCH,                       "pch",             "")
TYPE("none",                Nothing,                   "",                "")

#undef TYPE
