//===--- GenStruct.h - Swift IR generation for structs ----------*- C++ -*-===//
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
//  This file provides the private interface to the struct-emission code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_GENSTRUCT_H
#define POLARPHP_IRGEN_INTERNAL_GENSTRUCT_H

#include "llvm/ADT/Optional.h"

namespace llvm {
class Constant;
}

namespace polar {

class CanType;
class PILType;
class VarDecl;

namespace irgen {

class Address;
class Explosion;
class IRGenFunction;
class IRGenModule;
class MemberAccessStrategy;

Address projectPhysicalStructMemberAddress(IRGenFunction &IGF,
                                           Address base,
                                           PILType baseType,
                                           VarDecl *field);

void projectPhysicalStructMemberFromExplosion(IRGenFunction &IGF,
                                              PILType baseType,
                                              Explosion &base,
                                              VarDecl *field,
                                              Explosion &out);

/// Return the constant offset of the given stored property in a struct,
/// or return None if the field does not have fixed layout.
llvm::Constant *emitPhysicalStructMemberFixedOffset(IRGenModule &IGM,
                                                    PILType baseType,
                                                    VarDecl *field);

/// Return a strategy for accessing the given stored struct property.
///
/// This API is used by RemoteAST.
MemberAccessStrategy
getPhysicalStructMemberAccessStrategy(IRGenModule &IGM,
                                      PILType baseType, VarDecl *field);

/// Returns the index of the element in the llvm struct type which represents
/// \p field in \p baseType.
///
/// Returns None if \p field has an empty type and therefore has no
/// corresponding element in the llvm type.
llvm::Optional<unsigned> getPhysicalStructFieldIndex(IRGenModule &IGM,
                                                     PILType baseType,
                                                     VarDecl *field);

} // end namespace irgen
} // end namespace polar

#endif // POLARPHP_IRGEN_INTERNAL_GENSTRUCT_H
