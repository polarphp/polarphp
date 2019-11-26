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

#ifndef TYPE
#error "Define TYPE prior to including this file!"
#endif


// TYPE(NAME, ID, SUFFIX, FLAGS)

// The first value is the type name as a string; this should be something which
// could be displayed to the user, or something which the user could provide.

// The second value is the type id, which will result in a
// polar::driver::filetypes::TY_XX enum constant.

// The third value is the extension to use when creating temporary files
// of this type. It is also used when inferring a type from an extension.
// If multiple types specify the same extension, the first one is chosen when
// performing type inference.

// The fourth value is a string containing option flags. For now, this is
// unused, and should always be the empty string.

// Input types
TYPE("polar",               Polar,                     "polar",           "")
TYPE("sil",                 PIL,                       "pil",             "")
TYPE("sib",                 PIB,                       "pib",             "")

// Output types
TYPE("ast-dump",            ASTDump,                   "ast",             "")
TYPE("image",               Image,                     "out",             "")
TYPE("object",              Object,                    "o",               "")
TYPE("dSYM",                dSYM,                      "dSYM",            "")
TYPE("dependencies",        Dependencies,              "d",               "")
TYPE("autolink",            AutolinkFile,              "autolink",        "")
TYPE("polarmodule",         PolarModuleFile,           "polarmodule",     "")
TYPE("polardoc",            PolarModuleDocFile,        "polardoc",        "")
TYPE("polarinterface",      PolarParseableInterfaceFile, \
                                                       "swiftinterface",  "")
TYPE("assembly",            Assembly,                  "s",               "")
TYPE("raw-pil",             RawPIL,                    "pil",             "")
TYPE("raw-pib",             RawPIB,                    "pib",             "")
TYPE("llvm-ir",             LLVM_IR,                   "ll",              "")
TYPE("llvm-bc",             LLVM_BC,                   "bc",              "")
TYPE("diagnostics",         SerializedDiagnostics,     "dia",             "")
TYPE("objc-header",         ObjCHeader,                "h",               "")
TYPE("polar-dependencies",  PolarDeps,                 "polardeps",       "")
TYPE("remap",               Remapping,                 "remap",           "")
TYPE("imported-modules",    ImportedModules,           "importedmodules", "")
TYPE("tbd",                 TBD,                       "tbd",             "")
TYPE("module-trace",        ModuleTrace,               "trace.json",      "")
TYPE("index-data",          IndexData,                 "",                "")
TYPE("opt-record",          OptRecord,                 "opt.yaml",        "")

// Misc types
TYPE("pcm",                 ClangModuleFile,           "pcm",             "")
TYPE("pch",                 PCH,                       "pch",             "")
TYPE("none",                Nothing,                   "",                "")

#undef TYPE
