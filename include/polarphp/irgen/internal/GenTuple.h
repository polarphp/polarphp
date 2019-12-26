//===--- GenTuple.h - Swift IR generation for tuples ------------*- C++ -*-===//
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
//  This file provides the private interface to the tuple-emission code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_GENTUPLE_H
#define POLARPHP_IRGEN_INTERNAL_GENTUPLE_H

#include "polarphp/basic/LLVM.h"

namespace polar {

class CanType;

namespace irgen {

class Address;
class Explosion;
class IRGenFunction;
class Size;

/// Project the address of a tuple element.
Address projectTupleElementAddress(IRGenFunction &IGF,
                                   Address base,
                                   PILType tupleType,
                                   unsigned fieldNo);

/// Project a tuple element rvalue from an already-exploded tuple rvalue.
void projectTupleElementFromExplosion(IRGenFunction &IGF,
                                      PILType tupleType,
                                      Explosion &tuple,
                                      unsigned fieldNo,
                                      Explosion &out);

/// Return the offset to the given tuple element, if it's fixed.
///
/// This API is used by RemoteAST.
Optional<Size> getFixedTupleElementOffset(IRGenModule &IGM,
                                          PILType tupleType,
                                          unsigned fieldNo);

/// Returns the index of the element in the llvm struct type which represents
/// \p fieldNo in \p tupleType.
///
/// Returns None if the tuple element is an empty type and therefore has no
/// corresponding element in the llvm type.
Optional<unsigned> getPhysicalTupleElementStructIndex(IRGenModule &IGM,
                                                      PILType tupleType,
                                                      unsigned fieldNo);
} // end namespace irgen
} // end namespace swift

#endif // POLARPHP_IRGEN_INTERNAL_GENTUPLE_H
