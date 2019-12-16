//===--- Dwarf.h - DWARF constants ------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
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
// Created by polarboy on 2019/12/02.
//===----------------------------------------------------------------------===//
// This file defines several temporary Swift-specific DWARF constants.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_DWARF_H
#define POLARPHP_BASIC_DWARF_H

#include "llvm/BinaryFormat/Dwarf.h"

namespace polar {
/// The DWARF version emitted by the Swift compiler.
const unsigned DWARFVersion = 4;
static const char MachOASTSegmentName[] = "__POLARPHP";
static const char MachOASTSectionName[] = "__ast";
static const char ELFASTSectionName[] = ".polarphp_ast";
static const char COFFASTSectionName[] = "polarphpast";
static const char WasmASTSectionName[] = ".polarphp_ast";
} // polar

#endif // POLARPHP_BASIC_DWARF_H
