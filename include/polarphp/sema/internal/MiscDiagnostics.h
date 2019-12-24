//===--- MiscDiagnostics.h - AST-Level Diagnostics --------------*- C++ -*-===//
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

#ifndef POLARPHP_SEMA_INTERNAL_MISC_DIAGNOSTICS_H
#define POLARPHP_SEMA_INTERNAL_MISC_DIAGNOSTICS_H

#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/SourceLoc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"

namespace polar {

class AbstractFunctionDecl;
class ApplyExpr;
class CallExpr;
class DeclContext;
class Expr;
class InFlightDiagnostic;
class Stmt;
class TopLevelCodeDecl;
class TypeChecker;
class ValueDecl;

/// Emit diagnostics for syntactic restrictions on a given expression.
void performSyntacticExprDiagnostics(const Expr *E, const DeclContext *DC,
                                     bool isExprStmt);

/// Emit diagnostics for a given statement.
void performStmtDiagnostics(AstContext &ctx, const Stmt *S);

void performAbstractFuncDeclDiagnostics(AbstractFunctionDecl *AFD,
                                        BraceStmt *body);

/// Perform diagnostics on the top level code declaration.
void performTopLevelDeclDiagnostics(TopLevelCodeDecl *TLCD);

/// Emit a fix-it to set the access of \p VD to \p desiredAccess.
///
/// This actually updates \p VD as well.
void fixItAccess(InFlightDiagnostic &diag,
                 ValueDecl *VD,
                 AccessLevel desiredAccess,
                 bool isForSetter = false,
                 bool shouldUseDefaultAccess = false);

/// Emit fix-its to correct the argument labels in \p expr, which is the
/// argument tuple or single argument of a call.
///
/// If \p existingDiag is null, the fix-its will be attached to an appropriate
/// error diagnostic.
///
/// \returns true if the issue was diagnosed
bool diagnoseArgumentLabelError(AstContext &ctx,
                                Expr *expr,
                                ArrayRef<Identifier> newNames,
                                bool isSubscript,
                                InFlightDiagnostic *existingDiag = nullptr);

/// If \p assignExpr has a destination expression that refers to a declaration
/// with a non-owning attribute, such as 'weak' or 'unowned' and the initializer
/// expression refers to a class constructor, emit a warning that the assigned
/// instance will be immediately deallocated.
void diagnoseUnownedImmediateDeallocation(AstContext &ctx,
                                          const AssignExpr *assignExpr);

/// If \p pattern binds to a declaration with a non-owning attribute, such as
/// 'weak' or 'unowned' and \p initializer refers to a class constructor,
/// emit a warning that the bound instance will be immediately deallocated.
void diagnoseUnownedImmediateDeallocation(AstContext &ctx,
                                          const Pattern *pattern,
                                          SourceLoc equalLoc,
                                          const Expr *initializer);

/// Attempt to fix the type of \p decl so that it's a valid override for
/// \p base...but only if we're highly confident that we know what the user
/// should have written.
///
/// The \p diag closure allows the caller to control the diagnostic that is
/// emitted. It is passed true if the diagnostic will be emitted with fixits
/// attached, and false otherwise. If None is returned, no diagnostics are
/// emitted.  Else the fixits are attached to the returned diagnostic.
///
/// \returns true iff any fix-its were attached to \p diag.
bool computeFixitsForOverridenDeclaration(
   ValueDecl *decl, const ValueDecl *base,
   llvm::function_ref<Optional<InFlightDiagnostic>(bool)> diag);

/// Emit fix-its to enclose trailing closure in argument parens.
void fixItEncloseTrailingClosure(AstContext &ctx,
                                 InFlightDiagnostic &diag,
                                 const CallExpr *call,
                                 Identifier closureLabel);

} // namespace polar

#endif // POLARPHP_SEMA_INTERNAL_MISC_DIAGNOSTICS_H

