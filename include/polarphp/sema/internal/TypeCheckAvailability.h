//===--- TypeCheckAvailability.h - Availability Diagnostics -----*- C++ -*-===//
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

#ifndef POLARPHP_SEMA_INTERNAL_TYPE_CHECK_AVAILABILITY_H
#define POLARPHP_SEMA_INTERNAL_TYPE_CHECK_AVAILABILITY_H

#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/SourceLoc.h"
#include "polarphp/basic/OptionSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"

namespace polar {

class ApplyExpr;
class AvailableAttr;
class DeclContext;
class Expr;
class InFlightDiagnostic;
class Decl;
class ValueDecl;

/// Diagnose uses of unavailable declarations.
void diagAvailability(const Expr *E, DeclContext *DC);

enum class DeclAvailabilityFlag : uint8_t {
   /// Do not diagnose uses of protocols in versions before they were introduced.
   /// Used when type-checking protocol conformances, since conforming to a
   /// protocol that doesn't exist yet is allowed.
      AllowPotentiallyUnavailableInterface = 1 << 0,

   /// Diagnose uses of declarations in versions before they were introduced, but
   /// do not return true to indicate that a diagnostic was emitted.
      ContinueOnPotentialUnavailability = 1 << 1,

   /// If a diagnostic must be emitted, use a variant indicating that the usage
   /// is inout and both the getter and setter must be available.
      ForInout = 1 << 2,

   /// Do not diagnose uses of declarations in versions before they were
   /// introduced. Used to work around availability-checker bugs.
      AllowPotentiallyUnavailable = 1 << 3,
};
using DeclAvailabilityFlags = OptionSet<DeclAvailabilityFlag>;

/// Run the Availability-diagnostics algorithm otherwise used in an expr
/// context, but for non-expr contexts such as TypeDecls referenced from
/// TypeReprs.
bool diagnoseDeclAvailability(const ValueDecl *Decl,
                              DeclContext *DC,
                              SourceRange R,
                              DeclAvailabilityFlags Options);

void diagnoseUnavailableOverride(ValueDecl *override,
                                 const ValueDecl *base,
                                 const AvailableAttr *attr);

/// Emit a diagnostic for references to declarations that have been
/// marked as unavailable, either through "unavailable" or "obsoleted:".
bool diagnoseExplicitUnavailability(const ValueDecl *D,
                                    SourceRange R,
                                    const DeclContext *DC,
                                    const ApplyExpr *call);

/// Emit a diagnostic for references to declarations that have been
/// marked as unavailable, either through "unavailable" or "obsoleted:".
bool diagnoseExplicitUnavailability(
   const ValueDecl *D,
   SourceRange R,
   const DeclContext *DC,
   llvm::function_ref<void(InFlightDiagnostic &)> attachRenameFixIts);

/// Check if \p decl has a introduction version required by -require-explicit-availability
void checkExplicitAvailability(Decl *decl);

} // namespace polar

#endif // POLARPHP_SEMA_INTERNAL_TYPE_CHECK_AVAILABILITY_H

