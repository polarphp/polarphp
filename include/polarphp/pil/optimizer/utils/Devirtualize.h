//===--- Devirtualize.h - Helper for devirtualizing apply -------*- C++ -*-===//
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
// This contains helper functions that perform the work of devirtualizing a
// given apply when possible.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_DEVIRTUALIZE_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_DEVIRTUALIZE_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Types.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILType.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/ClassHierarchyAnalysis.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/ArrayRef.h"

namespace polar {
namespace optremark {
class Emitter;
}

/// Compute all subclasses of a given class.
///
/// \p CHA class hierarchy analysis
/// \p CD class declaration
/// \p ClassType type of the instance
/// \p M PILModule
/// \p Subs a container to be used for storing the set of subclasses
void getAllSubclasses(ClassHierarchyAnalysis *CHA,
                      ClassDecl *CD,
                      CanType ClassType,
                      PILModule &M,
                      ClassHierarchyAnalysis::ClassList &Subs);

/// Given an apply instruction of a protocol requirement and a witness method
/// for the requirement, compute a substitution suitable for a direct call
/// to the witness method.
///
/// \p Module PILModule
/// \p AI ApplySite that applies a procotol method
/// \p F PILFunction with convention @convention(witness_method)
/// \p CRef a concrete InterfaceConformanceRef
SubstitutionMap getWitnessMethodSubstitutions(PILModule &Module, ApplySite AI,
                                              PILFunction *F,
                                              InterfaceConformanceRef CRef);

/// Attempt to devirtualize the given apply site.  If this fails,
/// the returned ApplySite will be null.
///
/// If this succeeds, the caller must call deleteDevirtualizedApply on
/// the original apply site.
ApplySite tryDevirtualizeApply(ApplySite AI,
                               ClassHierarchyAnalysis *CHA,
                               optremark::Emitter *ORE = nullptr);
bool canDevirtualizeApply(FullApplySite AI, ClassHierarchyAnalysis *CHA);
bool canDevirtualizeClassMethod(FullApplySite AI, ClassDecl *CD,
                                optremark::Emitter *ORE = nullptr,
                                bool isEffectivelyFinalMethod = false);
PILFunction *getTargetClassMethod(PILModule &M, ClassDecl *CD,
                                  MethodInst *MI);
CanType getSelfInstanceType(CanType ClassOrMetatypeType);

/// Devirtualize the given apply site, which is known to be devirtualizable.
///
/// The caller must call deleteDevirtualizedApply on the original apply site.
FullApplySite devirtualizeClassMethod(FullApplySite AI,
                                      PILValue ClassInstance,
                                      ClassDecl *CD,
                                      optremark::Emitter *ORE);

/// Attempt to devirtualize the given apply site, which is known to be
/// of a class method.  If this fails, the returned FullApplySite will be null.
///
/// If this succeeds, the caller must call deleteDevirtualizedApply on
/// the original apply site.
FullApplySite
tryDevirtualizeClassMethod(FullApplySite AI,
                           PILValue ClassInstance,
                           ClassDecl *CD,
                           optremark::Emitter *ORE,
                           bool isEffectivelyFinalMethod = false);

/// Attempt to devirtualize the given apply site, which is known to be
/// of a witness method.  If this fails, the returned FullApplySite
/// will be null.
///
/// If this succeeds, the caller must call deleteDevirtualizedApply on
/// the original apply site.
ApplySite tryDevirtualizeWitnessMethod(ApplySite AI, optremark::Emitter *ORE);

/// Delete a successfully-devirtualized apply site.  This must always be
/// called after devirtualizing an apply; not only is it not semantically
/// equivalent to leave the old apply in-place, but the PIL isn't necessary
/// well-formed.
///
/// Devirtualization is responsible for replacing uses of the original
/// apply site with uses of the new one.  The only thing this does is delete
/// the instruction and any now-trivially-dead operands; it is separated
/// from the actual devirtualization step only to apply the caller to log
/// information about the original apply site.  This is probably not a
/// good enough reason to complicate the API.
void deleteDevirtualizedApply(ApplySite AI);

} // end namespace polar

#endif
