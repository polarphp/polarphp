//===--- GenExistential.h - IR generation for existentials ------*- C++ -*-===//
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
//  This file provides the private interface to the existential emission code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_GENEXISTENTIAL_H
#define POLARPHP_IRGEN_INTERNAL_GENEXISTENTIAL_H

#include "polarphp/irgen/internal/Address.h"
#include "polarphp/ast/Types.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/pil/lang/PILInstruction.h"

namespace llvm {
class Value;
}

namespace polar {

class InterfaceConformanceRef;
class PILType;

namespace irgen {

class Address;
class Explosion;
class IRGenFunction;

/// Emit the metadata and witness table initialization for an allocated
/// opaque existential container.
Address emitOpaqueExistentialContainerInit(IRGenFunction &IGF,
                                           Address dest,
                                           PILType destType,
                                           CanType formalSrcType,
                                           PILType loweredSrcType,
                                           ArrayRef<InterfaceConformanceRef> conformances);

/// Emit an existential metatype container from a metatype value
/// as an explosion.
void emitExistentialMetatypeContainer(IRGenFunction &IGF,
                                      Explosion &out,
                                      PILType outType,
                                      llvm::Value *metatype,
                                      PILType metatypeType,
                                      ArrayRef<InterfaceConformanceRef> conformances);


/// Emit a class existential container from a class instance value
/// as an explosion.
void emitClassExistentialContainer(IRGenFunction &IGF,
                                   Explosion &out,
                                   PILType outType,
                                   llvm::Value *instance,
                                   CanType instanceFormalType,
                                   PILType instanceLoweredType,
                                   ArrayRef<InterfaceConformanceRef> conformances);

/// Allocate a boxed existential container with uninitialized space to hold a
/// value of a given type.
OwnedAddress emitBoxedExistentialContainerAllocation(IRGenFunction &IGF,
                                                     PILType destType,
                                                     CanType formalSrcType,
                                                     ArrayRef<InterfaceConformanceRef> conformances);

/// Deallocate a boxed existential container with uninitialized space to hold
/// a value of a given type.
void emitBoxedExistentialContainerDeallocation(IRGenFunction &IGF,
                                               Explosion &container,
                                               PILType containerType,
                                               CanType valueType);

/// Allocate the storage for an opaque existential in the existential
/// container.
/// If the value is not inline, this will allocate a box for the value and
/// store the reference to the box in the existential container's buffer.
Address emitAllocateBoxedOpaqueExistentialBuffer(IRGenFunction &IGF,
                                                 PILType destType,
                                                 PILType valueType,
                                                 Address existentialContainer,
                                                 GenericEnvironment *genEnv,
                                                 bool isOutlined);
/// Deallocate the storage for an opaque existential in the existential
/// container.
/// If the value is not stored inline, this will deallocate the box for the
/// value.
void emitDeallocateBoxedOpaqueExistentialBuffer(IRGenFunction &IGF,
                                                PILType existentialType,
                                                Address existentialContainer);
Address emitOpaqueBoxedExistentialProjection(
   IRGenFunction &IGF, OpenedExistentialAccess accessKind, Address base,
   PILType existentialType, CanArchetypeType openedArchetype);

/// Extract the instance pointer from a class existential value.
///
/// \param openedArchetype If non-null, the archetype that will capture the
/// metadata and witness tables produced by projecting the archetype.
llvm::Value *emitClassExistentialProjection(IRGenFunction &IGF,
                                            Explosion &base,
                                            PILType baseTy,
                                            CanArchetypeType openedArchetype);

/// Extract the metatype pointer from an existential metatype value.
///
/// \param openedTy If non-null, a metatype of the archetype that
///   will capture the metadata and witness tables
llvm::Value *emitExistentialMetatypeProjection(IRGenFunction &IGF,
                                               Explosion &base,
                                               PILType baseTy,
                                               CanType openedTy);

/// Project the address of the value inside a boxed existential container.
ContainedAddress emitBoxedExistentialProjection(IRGenFunction &IGF,
                                                Explosion &base,
                                                PILType baseTy,
                                                CanType projectedType);

/// Project the address of the value inside a boxed existential container,
/// and open an archetype to its contained type.
Address emitOpenExistentialBox(IRGenFunction &IGF,
                               Explosion &base,
                               PILType baseTy,
                               CanArchetypeType openedArchetype);

/// Emit the existential metatype of an opaque existential value.
void emitMetatypeOfOpaqueExistential(IRGenFunction &IGF, Address addr,
                                     PILType type, Explosion &out);

/// Emit the existential metatype of a class existential value.
void emitMetatypeOfClassExistential(IRGenFunction &IGF,
                                    Explosion &value, PILType metatypeType,
                                    PILType existentialType, Explosion &out);

/// Emit the existential metatype of a boxed existential value.
void emitMetatypeOfBoxedExistential(IRGenFunction &IGF, Explosion &value,
                                    PILType type, Explosion &out);

/// Emit the existential metatype of a metatype.
void emitMetatypeOfMetatype(IRGenFunction &IGF, Explosion &value,
                            PILType existentialType, Explosion &out);

} // end namespace irgen
} // end namespace polar

#endif // POLARPHP_IRGEN_INTERNAL_GENEXISTENTIAL_H
