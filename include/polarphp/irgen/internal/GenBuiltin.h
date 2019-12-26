//===--- GenBuiltin.h - IR generation for builtin functions -----*- C++ -*-===//
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
//
//  This file provides the private interface to the emission of builtin
//  functions in polarphp.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_GENBUILTIN_H
#define POLARPHP_IRGEN_INTERNAL_GENBUILTIN_H

#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/basic/LLVM.h"

namespace polar {

class BuiltinInfo;
class Identifier;
class PILType;

namespace irgen {

class Explosion;
class IRGenFunction;

/// Emit a call to a builtin function.
void emitBuiltinCall(IRGenFunction &IGF, const BuiltinInfo &builtin,
                     Identifier fnId, PILType resultType,
                     Explosion &args, Explosion &result,
                     SubstitutionMap substitutions);

} // end namespace irgen
} // end namespace polar

#endif // POLARPHP_IRGEN_INTERNAL_GENBUILTIN_H
