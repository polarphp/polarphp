//===--- AstScope.h - Swift AST Scope ---------------------------*- C++ -*-===//
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
// This file defines the AstScope class and related functionality, which
// describes the scopes that exist within a Polarphp AST.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AST_SCOPE_H
#define POLARPHP_AST_AST_SCOPE_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/SourceMgr.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"

namespace polar::ast {

using polar::basic::SourceRange;
using polar::basic::SourceLoc;
using polar::syntax::Syntax;

class AbstractStorageDecl;
class AstContext;
class BraceStmt;
class CaseStmt;
class CatchStmt;
class ClosureExpr;
class Decl;
class DoCatchStmt;
class Expr;
class ForEachStmt;
class GenericParamList;
class GuardStmt;
class IfStmt;
class IterableDeclContext;
class LabeledConditionalStmt;
class ParamDecl;
class PatternBindingDecl;
class RepeatWhileStmt;
class SourceFile;
class Stmt;
class StmtConditionElement;
class SwitchStmt;
class TopLevelCodeDecl;
class TypeDecl;
class WhileStmt;

/// Describes kind of scope that occurs within the AST.
enum class AstScopeKind : std::uint8_t
{
   /// A pre-expanded scope in which we know a priori the children.
   ///
   /// This is a convenience scope that has no direct bearing on the AST.
   Preexpanded,
   /// A source file, which is the root of a scope.
   SourceFile,
   /// The declaration of a type.
   TypeDecl,
   /// The generic parameters of an extension declaration.
   ExtensionGenericParams,
   /// The body of a type or extension thereof.
   TypeOrExtensionBody,
   /// The generic parameters of a declaration.
   GenericParams,
   /// A function/initializer/deinitializer.
   AbstractFunctionDecl,
   /// The parameters of a function/initializer/deinitializer.
   AbstractFunctionParams,
   /// The default argument for a parameter.
   DefaultArgument,
   /// The body of a function.
   AbstractFunctionBody,
   /// A specific pattern binding.
   PatternBinding,
   /// The scope introduced for an initializer of a pattern binding.
   PatternInitializer,
   /// The scope following a particular clause in a pattern binding declaration,
   /// which is the outermost scope in which the variables introduced by that
   /// clause will be visible.
   AfterPatternBinding,
   /// The scope introduced by a brace statement.
   BraceStmt,
   /// Node describing an "if" statement.
   IfStmt,
   /// The scope introduced by a conditional clause in an if/guard/while
   /// statement.
   ConditionalClause,
   /// Node describing a "guard" statement.
   GuardStmt,
   /// Node describing a repeat...while statement.
   RepeatWhileStmt,
   /// Node describing a for-each statement.
   ForEachStmt,
   /// Describes the scope of the pattern of the for-each statement.
   ForEachPattern,
   /// Describes a do-catch statement.
   DoCatchStmt,
   /// Describes the a catch statement.
   CatchStmt,
   /// Describes a switch statement.
   SwitchStmt,
   /// Describes a 'case' statement.
   CaseStmt,
   /// Scope for the accessors of an abstract storage declaration.
   Accessors,
   /// Scope for a closure.
   Closure,
   /// Scope for top-level code.
   TopLevelCode,
};

/// Describes a lexical scope within a source file.
///
/// Each \c AstScope is a node within a tree that describes all of the lexical
/// scopes within a particular source range. The root of this scope tree is
/// always a \c SourceFile node, and the tree covers the entire source file.
/// The children of a particular node are the lexical scopes immediately
/// nested within that node, and have source ranges that are enclosed within
/// the source range of their parent node. At the leaves are lexical scopes
/// that cannot be subdivided further.
///
/// The tree provides source-location-based query operations, allowing one to
/// find the innermost scope that contains a given source location. Navigation
/// to parent nodes from that scope allows one to walk the lexically enclosing
/// scopes outward to the source file. Given a scope, one can also query the
/// associated \c DeclContext for additional contextual information.
///
/// As an implementation detail, the scope tree is lazily constructed as it is
/// queried, and only the relevant subtrees (i.e., trees whose source ranges
/// enclose the queried source location or whose children were explicitly
/// requested by the client) will be constructed. The \c expandAll() operation
/// can be used to fully-expand the tree, constructing all of its nodes, but
/// should only be used for testing or debugging purposes, e.g., via the
/// frontend option
/// \code
/// -dump-scope-maps expanded
/// \endcode
class AstScope
{
   /// The kind of scope this represents.
   AstScopeKind kind;

   /// The parent scope of this particular scope along with a bit indicating
   /// whether the children of this node have already been expanded.
   mutable llvm::PointerIntPair<const AstScope *, 1, bool> parentAndExpanded;

   /// Describes the kind of continuation stored in the continuation field.
   enum class ContinuationKind
   {
      /// The continuation is historical: if the continuation is non-null, we
      /// preserve it so we know which scope to look at to compute the end of the
      /// source range.
      Historical = 0,
      /// The continuation is active.
      Active = 1,
      /// The continuation stored in the pointer field is active, and replaced a
      /// \c SourceFile continuation.
      ActiveThenSourceFile = 2,
   };

   /// The scope from which the continuation child nodes will be populated.
   ///
   /// The enumeration bits indicate whether the continuation pointer represents
   /// an active continuation (vs. a historical one) and whether the former
   /// continuation was for a \c SourceFile (which can be stacked behind another
   /// continuation).
   mutable llvm::PointerIntPair<const AstScope *, 2, ContinuationKind>
   continuation = { nullptr, ContinuationKind::Historical };

   /// Union describing the various kinds of AST nodes that can introduce
   /// scopes.
   union {
      /// For \c kind == AstScopeKind::SourceFile.
      struct {
         // The actual source file.
         SourceFile *file;

         /// The next element that should be considered in the source file.
         ///
         /// This accommodates the expansion of source files.
         mutable unsigned nextElement;
      } sourceFile;

      /// A type declaration, for \c kind == AstScopeKind::TypeDecl.
      TypeDecl *typeDecl;

      /// An iterable declaration context, which covers nominal type declarations
      /// and extension bodies.
      ///
      /// For \c kind == AstScopeKind::TypeOrExtensionBody.
      IterableDeclContext *iterableDeclContext;

      /// The parameter whose default argument is being described, i.e.,
      /// \c kind == AstScopeKind::DefaultArgument.
      ParamDecl *parameter;

      /// For \c kind == AstScopeKind::BraceStmt.
      struct {
         BraceStmt *stmt;

         /// The next element in the brace statement that should be expanded.
         mutable unsigned nextElement;
      } braceStmt;

      /// The 'if' statement, for \c kind == AstScopeKind::IfStmt.
      IfStmt *ifStmt;

      /// For \c kind == AstScopeKind::ConditionalClause.
      struct {
         /// The statement that contains the conditional clause.
         LabeledConditionalStmt *stmt;

         /// The index of the conditional clause.
         unsigned index;

         /// Whether this conditional clause is being used for the 'guard'
         /// continuation.
         bool isGuardContinuation;
      } conditionalClause;

      /// The 'guard' statement, for \c kind == AstScopeKind::GuardStmt.
      GuardStmt *guard;

      /// The repeat...while statement, for
      /// \c kind == AstScopeKind::RepeatWhileStmt.
      RepeatWhileStmt *repeatWhile;

      /// The for-each statement, for
      /// \c kind == AstScopeKind::ForEachStmt or
      /// \c kind == AstScopeKind::ForEachPattern.
      ForEachStmt *forEach;

      /// A do-catch statement, for \c kind == AstScopeKind::DoCatchStmt.
      DoCatchStmt *doCatch;

      /// A catch statement, for \c kind == AstScopeKind::CatchStmt.
      CatchStmt *catchStmt;

      /// A switch statement, for \c kind == AstScopeKind::SwitchStmt.
      SwitchStmt *switchStmt;

      /// A case statement, for \c kind == AstScopeKind::CaseStmt;
      CaseStmt *caseStmt;

      /// An abstract storage declaration, for
      /// \c kind == AstScopeKind::Accessors.
      AbstractStorageDecl *abstractStorageDecl;

      /// The closure, for \c kind == AstScopeKind::Closure.
      ClosureExpr *closure;

      /// The top-level code declaration for
      /// \c kind == AstScopeKind::TopLevelCodeDecl.
      TopLevelCodeDecl *topLevelCode;
   };

   /// Child scopes, sorted by source range.
   mutable SmallVector<AstScope *, 4> storedChildren;

   /// Retrieve the active continuation.
   const AstScope *getActiveContinuation() const;

   /// Retrieve the historical continuation (which might also be active).
   ///
   /// This is the oldest historical continuation, so a \c SourceFile
   /// continuation will be returned even if it's been replaced by a more local
   /// continuation.
   const AstScope *getHistoricalContinuation() const;

   /// Set the active continuation.
   void addActiveContinuation(const AstScope *newContinuation) const;

   /// Remove the active continuation.
   void removeActiveContinuation() const;

   /// Clear out the continuation, because it has been stolen been transferred to
   /// a child node.
   void clearActiveContinuation() const;

   /// Constructor that only initializes the kind and parent, leaving the
   /// pieces to be initialized by the caller.
   AstScope(AstScopeKind kind, const AstScope *parent)
      : kind(kind), parentAndExpanded(parent, false)
   {}

   AstScope(SourceFile *sourceFile, unsigned nextElement)
      : AstScope(AstScopeKind::SourceFile, nullptr)
   {
      this->sourceFile.file = sourceFile;
      this->sourceFile.nextElement = nextElement;
   }

   /// Constructor that initializes a preexpanded node.
   AstScope(const AstScope *parent, ArrayRef<AstScope *> children);

   AstScope(const AstScope *parent, TypeDecl *typeDecl)
      : AstScope(AstScopeKind::TypeDecl, parent)
   {
      this->typeDecl = typeDecl;
   }

   AstScope(const AstScope *parent, IterableDeclContext *idc)
      : AstScope(AstScopeKind::TypeOrExtensionBody, parent)
   {
      this->iterableDeclContext = idc;
   }

   AstScope(const AstScope *parent, ParamDecl *param)
      : AstScope(AstScopeKind::DefaultArgument, parent)
   {
      this->parameter = param;
   }

   AstScope(const AstScope *parent, BraceStmt *braceStmt)
      : AstScope(AstScopeKind::BraceStmt, parent)
   {
      this->braceStmt.stmt = braceStmt;
      this->braceStmt.nextElement = 0;
   }

   AstScope(const AstScope *parent, IfStmt *ifStmt)
      : AstScope(AstScopeKind::IfStmt, parent)
   {
      this->ifStmt = ifStmt;
   }

   AstScope(const AstScope *parent, LabeledConditionalStmt *stmt,
            unsigned index, bool isGuardContinuation)
      : AstScope(AstScopeKind::ConditionalClause, parent)
   {
      this->conditionalClause.stmt = stmt;
      this->conditionalClause.index = index;
      this->conditionalClause.isGuardContinuation = isGuardContinuation;
   }

   AstScope(const AstScope *parent, GuardStmt *guard)
      : AstScope(AstScopeKind::GuardStmt, parent)
   {
      this->guard = guard;
   }

   AstScope(const AstScope *parent, RepeatWhileStmt *repeatWhile)
      : AstScope(AstScopeKind::RepeatWhileStmt, parent)
   {
      this->repeatWhile = repeatWhile;
   }

   AstScope(AstScopeKind kind, const AstScope *parent, ForEachStmt *forEach)
      : AstScope(kind, parent)
   {
      assert(kind == AstScopeKind::ForEachStmt ||
             kind == AstScopeKind::ForEachPattern);
      this->forEach = forEach;
   }

   AstScope(const AstScope *parent, DoCatchStmt *doCatch)
      : AstScope(AstScopeKind::DoCatchStmt, parent)
   {
      this->doCatch = doCatch;
   }

   AstScope(const AstScope *parent, CatchStmt *catchStmt)
      : AstScope(AstScopeKind::CatchStmt, parent)
   {
      this->catchStmt = catchStmt;
   }

   AstScope(const AstScope *parent, SwitchStmt *switchStmt)
      : AstScope(AstScopeKind::SwitchStmt, parent)
   {
      this->switchStmt = switchStmt;
   }

   AstScope(const AstScope *parent, CaseStmt *caseStmt)
      : AstScope(AstScopeKind::CaseStmt, parent)
   {
      this->caseStmt = caseStmt;
   }

   AstScope(const AstScope *parent, AbstractStorageDecl *abstractStorageDecl)
      : AstScope(AstScopeKind::Accessors, parent)
   {
      this->abstractStorageDecl = abstractStorageDecl;
   }

   AstScope(const AstScope *parent, ClosureExpr *closure)
      : AstScope(AstScopeKind::Closure, parent) {
      this->closure = closure;
   }

   AstScope(const AstScope *parent, TopLevelCodeDecl *topLevelCode)
      : AstScope(AstScopeKind::TopLevelCode, parent) {
      this->topLevelCode = topLevelCode;
   }

   ~AstScope();

   AstScope(AstScope &&) = delete;
   AstScope &operator=(AstScope &&) = delete;
   AstScope(const AstScope &) = delete;
   AstScope &operator=(const AstScope &) = delete;

   /// Expand the children of this AST scope so they can be queried.
   void expand() const;

   /// Determine whether the given scope has already been completely expanded,
   /// and cannot create any new children.
   bool isExpanded() const;

   /// Create a new AST scope if one is needed for the given declaration.
   ///
   /// \returns the newly-created AST scope, or \c null if there is no scope
   /// introduced by this declaration.
   static AstScope *createIfNeeded(const AstScope *parent, Decl *decl);

   /// Create a new AST scope if one is needed for the given statement.
   ///
   /// \returns the newly-created AST scope, or \c null if there is no scope
   /// introduced by this statement.
   static AstScope *createIfNeeded(const AstScope *parent, Stmt *stmt);

   /// Create a new AST scope if one is needed for the given child expression(s).
   /// In the first variant, the expression can be \c null.
   ///
   /// \returns the newly-created AST scope, or \c null if there is no scope
   /// introduced by this expression.
   static AstScope *createIfNeeded(const AstScope *parent, Expr *expr);
   static AstScope *createIfNeeded(const AstScope *parent,
                                   ArrayRef<Expr *> exprs);

   /// Create a new AST scope if one is needed for the given AST node.
   ///
   /// \returns the newly-created AST scope, or \c null if there is no scope
   /// introduced by this AST node.
   static AstScope *createIfNeeded(const AstScope *parent, Syntax node);

   /// Determine whether this scope can steal a continuation from its parent,
   /// because (e.g.) it introduces some name binding that should be visible
   /// in the continuation.
   bool canStealContinuation() const;

   /// Enumerate the continuation child scopes for the given scope.
   ///
   /// \param addChild Function that will be invoked to add the continuation
   /// child. This function should return true if the child steals the
   /// continuation, which terminates the enumeration.
   void enumerateContinuationScopes(
         llvm::function_ref<bool(AstScope *)> addChild) const;

   /// Compute the source range of this scope.
   SourceRange getSourceRangeImpl() const;

   /// Retrieve the AstContext in which this scope exists.
   AstContext &getASTContext() const;

   /// Retrieve the source file scope, which is the root of the tree.
   const AstScope *getSourceFileScope() const;

   /// Retrieve the source file in which this scope exists.
   SourceFile &getSourceFile() const;

public:
   /// Create the AST scope for a source file, which is the root of the scope
   /// tree.
   static AstScope *createRoot(SourceFile *sourceFile);

   /// Determine the kind of AST scope we have.
   AstScopeKind getKind() const
   {
      return kind;
   }

   /// Retrieve the parent scope that encloses this one.
   const AstScope *getParent() const
   {
      return parentAndExpanded.getPointer();
   }

   /// Retrieve the children of this AST scope, expanding if necessary.
   ArrayRef<AstScope *> children() const {
      if (!isExpanded()) expand();
      return storedChildren;
   }

   /// Determine the source range covered by this scope.
   SourceRange getSourceRange() const
   {
      SourceRange range = getSourceRangeImpl();

      // If there was ever a continuation, use it's end range.
      if (auto historical = getHistoricalContinuation()) {
         if (historical != this) {
            range.getEnd() = historical->getSourceRange().getEnd();
         }
      }
      return range;
   }

   /// Retrieve the type declaration when \c getKind() == AstScopeKind::TypeDecl.
   TypeDecl *getTypeDecl() const
   {
      assert(getKind() == AstScopeKind::TypeDecl);
      return typeDecl;
   }

   /// Retrieve the abstract storage declaration when
   /// \c getKind() == AstScopeKind::Accessors;
   AbstractStorageDecl *getAbstractStorageDecl() const
   {
      assert(getKind() == AstScopeKind::Accessors);
      return abstractStorageDecl;
   }

   /// Find the innermost enclosing scope that contains this source location.
   const AstScope *findInnermostEnclosingScope(SourceLoc loc) const;


//   /// Retrieve the declarations whose names are directly bound by this scope.
//   ///
//   /// The declarations bound in this scope aren't available in the immediate
//   /// parent of this scope, but will still be visible in child scopes (unless
//   /// shadowed there).
//   ///
//   /// Note that this routine does not produce bindings for anything that can
//   /// be found via qualified name lookup in a \c DeclContext, such as nominal
//   /// type declarations or extensions thereof, or the source file itself. The
//   /// client can perform such lookups using the result of \c getDeclContext().
//   SmallVector<ValueDecl *, 4> getLocalBindings() const;

   /// Expand the entire scope map.
   ///
   /// Normally, the scope map will be expanded only as needed by its queries,
   /// but complete expansion can be useful for debugging.
   void expandAll() const;

   /// Print out this scope for debugging/reporting purposes.
   void print(llvm::raw_ostream &out, unsigned level = 0,
              bool lastChild = false,
              bool printChildren = true) const;

   LLVM_ATTRIBUTE_DEPRECATED(void dump() const LLVM_ATTRIBUTE_USED,
                             "only for use within the debugger");

   // Make vanilla new/delete illegal for Decls.
   void *operator new(size_t bytes) = delete;
   void operator delete(void *data) = delete;

   // Only allow allocation of scopes using the allocator of a particular source
   // file.
   void *operator new(size_t bytes, const AstContext &ctx,
                      unsigned alignment = alignof(AstScope));
   void *operator new(size_t Bytes, void *Mem)
   {
      assert(Mem);
      return Mem;
   }
};

} // polar::ast

#endif // POLARPHP_AST_AST_SCOPE_H
