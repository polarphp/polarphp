//===--- AstScopeCreation.cpp - Swift Object-Oriented Ast Scope -----------===//
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
///
/// This file implements the creation methods of the AstScopeImpl ontology.
///
//===----------------------------------------------------------------------===//
#include "polarphp/ast/AstScope.h"

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/Attr.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Initializer.h"
#include "polarphp/ast/LazyResolver.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/NameLookupRequests.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeRepr.h"
#include "polarphp/basic/Debug.h"
#include "polarphp/basic/StlExtras.h"
#include "llvm/Support/Compiler.h"
#include <algorithm>
#include <unordered_set>

using namespace polar;
using namespace polar::ast_scope;
using namespace llvm;

/// If true, nest scopes so a variable is out of scope before its declaration
/// Does not handle capture rules for local functions properly.
/// If false don't push uses down into subscopes after decls.
static const bool handleUseBeforeDef = false;

#pragma mark source range utilities
static bool rangeableIsIgnored(const Decl *d) { return d->isImplicit(); }
static bool rangeableIsIgnored(const Expr *d) {
  return false; // implicit expr may contain closures
}
static bool rangeableIsIgnored(const Stmt *d) {
  return false; // ??
}
static bool rangeableIsIgnored(const AstNode n) {
  return (n.is<Decl *>() && rangeableIsIgnored(n.get<Decl *>())) ||
         (n.is<Stmt *>() && rangeableIsIgnored(n.get<Stmt *>())) ||
         (n.is<Expr *>() && rangeableIsIgnored(n.get<Expr *>()));
}

template <typename Rangeable>
static SourceRange getRangeableSourceRange(const Rangeable *const p) {
  return p->getSourceRange();
}
static SourceRange getRangeableSourceRange(const SpecializeAttr *a) {
  return a->getRange();
}
static SourceRange getRangeableSourceRange(const AstNode n) {
  return n.getSourceRange();
}

template <typename Rangeable>
static bool isLocalizable(const Rangeable astElement) {
  return !rangeableIsIgnored(astElement) &&
         getRangeableSourceRange(astElement).isValid();
}

template <typename Rangeable>
static void dumpRangeable(const Rangeable r, llvm::raw_ostream &f) {
  r.dump(f);
}
template <typename Rangeable>
static void dumpRangeable(const Rangeable *r, llvm::raw_ostream &f) {
  r->dump(f);
}
template <typename Rangeable>
static void dumpRangeable(Rangeable *r, llvm::raw_ostream &f) {
  r->dump(f);
}

static void dumpRangeable(const SpecializeAttr *r,
                          llvm::raw_ostream &f) LLVM_ATTRIBUTE_USED;
static void dumpRangeable(const SpecializeAttr *r, llvm::raw_ostream &f) {
  llvm::errs() << "SpecializeAttr\n";
}
static void dumpRangeable(SpecializeAttr *r,
                          llvm::raw_ostream &f) LLVM_ATTRIBUTE_USED;
static void dumpRangeable(SpecializeAttr *r, llvm::raw_ostream &f) {
  llvm::errs() << "SpecializeAttr\n";
}

/// For Debugging
template <typename T>
bool doesRangeableRangeMatch(const T *x, const SourceManager &SM,
                             unsigned start, unsigned end,
                             StringRef file = "") {
  auto const r = getRangeableSourceRange(x);
  if (r.isInvalid())
    return false;
  if (start && SM.getLineNumber(r.Start) != start)
    return false;
  if (end && SM.getLineNumber(r.End) != end)
    return false;
  if (file.empty())
    return true;
  const auto buf = SM.findBufferContainingLoc(r.Start);
  return SM.getIdentifierForBuffer(buf).endswith(file);
}

#pragma mark end of rangeable

static std::vector<AstNode> asNodeVector(DeclRange dr) {
  std::vector<AstNode> nodes;
  llvm::transform(dr, std::back_inserter(nodes),
                  [&](Decl *d) { return AstNode(d); });
  return nodes;
}

namespace polar {
namespace ast_scope {

namespace {
/// Use me with any AstNode, Expr*, Decl*, or Stmt*
/// I will yield a void* that is the same, even when given an Expr* and a
/// ClosureExpr* because I take the Expr*, figure its real class, then up
/// cast.
/// Useful for duplicate checking.
class UniquePointerCalculator
    : public AstVisitor<UniquePointerCalculator, void *, void *, void *, void *,
                        void *, void *> {
public:
  template <typename T> const void *visit(const T *x) {
    return const_cast<T *>(x);
  }

  // Call these only from the superclass
  void *visitDecl(Decl *e) { return e; }
  void *visitStmt(Stmt *e) { return e; }
  void *visitExpr(Expr *e) { return e; }
  void *visitPattern(Pattern *e) { return e; }
  void *visitDeclAttribute(DeclAttribute *e) { return e; }

// Provide default implementations for statements as AstVisitor does for Exprs
#define STMT(CLASS, PARENT)                                                    \
  void *visit##CLASS##Stmt(CLASS##Stmt *S) { return visitStmt(S); }
#include "polarphp/ast/StmtNodesDef.h"

// Provide default implementations for patterns as AstVisitor does for Exprs
#define PATTERN(CLASS, PARENT)                                                 \
  void *visit##CLASS##Pattern(CLASS##Pattern *S) { return visitPattern(S); }
#include "polarphp/ast/PatternNodesDef.h"
};

/// A set that does the right pointer calculation for comparing Decls to
/// DeclContexts, and Exprs
class NodeSet {
  ::llvm::DenseSet<const void *> pointers;

public:
  bool contains(const AstScopeImpl *const s) {
    if (auto *r = s->getReferrent().getPtrOrNull())
      return pointers.count(r);
    return false; // never exclude a non-checkable scope
  }
  bool insert(const AstScopeImpl *const s) {
    if (auto *r = s->getReferrent().getPtrOrNull())
      return pointers.insert(r).second;
    return true;
  }
  void erase(const AstScopeImpl *const s) {
    if (auto *r = s->getReferrent().getPtrOrNull())
      pointers.erase(r);
  }
};
} // namespace

#pragma mark ScopeCreator

class ScopeCreator final {
  friend class AstSourceFileScope;
  /// For allocating scopes.
  AstContext &ctx;

public:
  AstSourceFileScope *const sourceFileScope;
  AstContext &getAstContext() const { return ctx; }

  /// The Ast can have duplicate nodes, and we don't want to create scopes for
  /// those.
  NodeSet scopedNodes;

  ScopeCreator(SourceFile *SF)
      : ctx(SF->getAstContext()),
        sourceFileScope(new (ctx) AstSourceFileScope(SF, this)) {
    ctx.addDestructorCleanup(scopedNodes);
  }

  ScopeCreator(const ScopeCreator &) = delete;  // ensure no copies
  ScopeCreator(const ScopeCreator &&) = delete; // ensure no moves

  /// Given an array of AstNodes or Decl pointers, add them
  /// Return the resultant insertionPoint
  AstScopeImpl *
  addSiblingsToScopeTree(AstScopeImpl *const insertionPoint,
                         AstScopeImpl *const organicInsertionPoint,
                         ArrayRef<AstNode> nodesOrDeclsToAdd) {
    auto *ip = insertionPoint;
    for (auto nd : expandIfConfigClausesThenCullAndSortElementsOrMembers(
             nodesOrDeclsToAdd)) {
      if (!shouldThisNodeBeScopedWhenFoundInSourceFileBraceStmtOrType(nd)) {
        // FIXME: Could the range get lost if the node is ever reexpanded?
        ip->widenSourceRangeForIgnoredAstNode(nd);
      } else {
        const unsigned preCount = ip->getChildren().size();
        auto *const newIP =
            addToScopeTreeAndReturnInsertionPoint(nd, ip).getPtrOr(ip);
        if (ip != organicInsertionPoint)
          ip->increaseAstAncestorScopeCount(ip->getChildren().size() -
                                            preCount);
        ip = newIP;
      }
    }
    return ip;
  }

public:
  /// For each of searching, call this unless the insertion point is needed
  void addToScopeTree(AstNode n, AstScopeImpl *parent) {
    (void)addToScopeTreeAndReturnInsertionPoint(n, parent);
  }
  /// Return new insertion point if the scope was not a duplicate
  /// For ease of searching, don't call unless insertion point is needed
  NullablePtr<AstScopeImpl>
  addToScopeTreeAndReturnInsertionPoint(AstNode, AstScopeImpl *parent);

  bool isWorthTryingToCreateScopeFor(AstNode n) const {
    if (!n)
      return false;
    if (n.is<Expr *>())
      return true;
    // Cannot ignore implicit statements because implict return can contain
    // scopes in the expression, such as closures.
    // But must ignore other implicit statements, e.g. brace statments
    // if they can have no children and no stmt source range.
    // Deal with it in visitBraceStmt
    if (n.is<Stmt *>())
      return true;

    auto *const d = n.get<Decl *>();
    // Implicit nodes may not have source information for name lookup.
    if (!isLocalizable(d))
      return false;
    /// In \c Parser::parseDeclVarGetSet fake PBDs are created. Ignore them.
    /// Example:
    /// \code
    /// class SR10903 { static var _: Int { 0 } }
    /// \endcode

    // Commented out for
    // validation-test/compiler_crashers_fixed/27962-swift-rebindselfinconstructorexpr-getcalledconstructor.swift
    // In that test the invalid PBD -> var decl which contains the desired
    // closure scope
    //    if (const auto *PBD = dyn_cast<PatternBindingDecl>(d))
    //      if (!isLocalizable(PBD))
    //        return false;
    /// In
    /// \code
    /// @propertyWrapper
    /// public struct Wrapper<T> {
    ///   public var value: T
    ///
    ///   public init(body: () -> T) {
    ///     self.value = body()
    ///   }
    /// }
    ///
    /// let globalInt = 17
    ///
    /// @Wrapper(body: { globalInt })
    /// public var y: Int
    /// \endcode
    /// I'm seeing a dumped Ast include:
    /// (pattern_binding_decl range=[test.swift:13:8 - line:12:29]
    const auto &SM = d->getAstContext().SourceMgr;

    // Once we allow invalid PatternBindingDecls (see
    // isWorthTryingToCreateScopeFor), then
    // IDE/complete_property_delegate_attribute.swift fails because we try to
    // expand a member whose source range is backwards.
    (void)SM;
    ast_scope_assert(d->getStartLoc().isInvalid() ||
                       !SM.isBeforeInBuffer(d->getEndLoc(), d->getStartLoc()),
                   "end-before-start will break tree search via location");
    return true;
  }

  /// Create a new scope of class ChildScope initialized with a ChildElement,
  /// expandScope it,
  /// add it as a child of the receiver, and return the child and the scope to
  /// receive more decls.
  template <typename Scope, typename... Args>
  AstScopeImpl *constructExpandAndInsertUncheckable(AstScopeImpl *parent,
                                                    Args... args) {
    ast_scope_assert(!Scope(args...).getReferrent(),
                   "Not checking for duplicate AstNode but class supports it");
    return constructExpandAndInsert<Scope>(parent, args...);
  }

  template <typename Scope, typename... Args>
  NullablePtr<AstScopeImpl>
  ifUniqueConstructExpandAndInsert(AstScopeImpl *parent, Args... args) {
    Scope dryRun(args...);
    ast_scope_assert(
        dryRun.getReferrent(),
        "Checking for duplicate AstNode but class does not support it");
    if (scopedNodes.insert(&dryRun))
      return constructExpandAndInsert<Scope>(parent, args...);
    return nullptr;
  }

  template <typename Scope, typename... Args>
  AstScopeImpl *ensureUniqueThenConstructExpandAndInsert(AstScopeImpl *parent,
                                                         Args... args) {
    if (auto s = ifUniqueConstructExpandAndInsert<Scope>(parent, args...))
      return s.get();
    ast_scope_unreachable("Scope should have been unique");
  }

private:
  template <typename Scope, typename... Args>
  AstScopeImpl *constructExpandAndInsert(AstScopeImpl *parent, Args... args) {
    auto *child = new (ctx) Scope(args...);
    parent->addChild(child, ctx);
    if (shouldBeLazy()) {
      if (auto *ip = child->insertionPointForDeferredExpansion().getPtrOrNull())
        return ip;
    }
    AstScopeImpl *insertionPoint =
        child->expandAndBeCurrentDetectingRecursion(*this);
    ast_scope_assert(child->verifyThatThisNodeComeAfterItsPriorSibling(),
                   "Ensure search will work");
    return insertionPoint;
  }

public:
  template <typename Scope, typename PortionClass, typename... Args>
  AstScopeImpl *constructWithPortionExpandAndInsert(AstScopeImpl *parent,
                                                    Args... args) {
    const Portion *portion = new (ctx) PortionClass();
    return constructExpandAndInsertUncheckable<Scope>(parent, portion, args...);
  }

  template <typename Scope, typename PortionClass, typename... Args>
  NullablePtr<AstScopeImpl>
  ifUniqueConstructWithPortionExpandAndInsert(AstScopeImpl *parent,
                                              Args... args) {
    const Portion *portion = new (ctx) PortionClass();
    return ifUniqueConstructExpandAndInsert<Scope>(parent, portion, args...);
  }

  void addExprToScopeTree(Expr *expr, AstScopeImpl *parent) {
    // Use the AstWalker to find buried captures and closures
    forEachClosureIn(expr, [&](NullablePtr<CaptureListExpr> captureList,
                               ClosureExpr *closureExpr) {
      ifUniqueConstructExpandAndInsert<WholeClosureScope>(parent, closureExpr,
                                                          captureList);
    });
  }

private:
  /// Find all of the (non-nested) closures (and associated capture lists)
  /// referenced within this expression.
  void forEachClosureIn(
      Expr *expr,
      llvm::function_ref<void(NullablePtr<CaptureListExpr>, ClosureExpr *)>
          foundClosure);

  // A safe way to discover this, without creating a circular request.
  // Cannot call getAttachedPropertyWrappers.
  static bool hasAttachedPropertyWrapper(VarDecl *vd) {
    return AttachedPropertyWrapperScope::getSourceRangeOfVarDecl(vd).isValid();
  }

public:
  /// If the pattern has an attached property wrapper, create a scope for it
  /// so it can be looked up.

  void
  addAnyAttachedPropertyWrappersToScopeTree(PatternBindingDecl *patternBinding,
                                            AstScopeImpl *parent) {
    patternBinding->getPattern(0)->forEachVariable([&](VarDecl *vd) {
      if (hasAttachedPropertyWrapper(vd))
        constructExpandAndInsertUncheckable<AttachedPropertyWrapperScope>(
            parent, vd);
    });
  }

public:
  /// Create the matryoshka nested generic param scopes (if any)
  /// that are subscopes of the receiver. Return
  /// the furthest descendant.
  /// Last GenericParamsScope includes the where clause
  AstScopeImpl *addNestedGenericParamScopesToTree(Decl *parameterizedDecl,
                                                  GenericParamList *generics,
                                                  AstScopeImpl *parent) {
    if (!generics)
      return parent;
    auto *s = parent;
    for (unsigned i : indices(generics->getParams()))
      s = ifUniqueConstructExpandAndInsert<GenericParamScope>(
              s, parameterizedDecl, generics, i)
              .getPtrOr(s);
    return s;
  }

  void
  addChildrenForAllLocalizableAccessorsInSourceOrder(AbstractStorageDecl *asd,
                                                     AstScopeImpl *parent);

  void
  forEachSpecializeAttrInSourceOrder(Decl *declBeingSpecialized,
                                     function_ref<void(SpecializeAttr *)> fn) {
    std::vector<SpecializeAttr *> sortedSpecializeAttrs;
    for (auto *attr : declBeingSpecialized->getAttrs()) {
      if (auto *specializeAttr = dyn_cast<SpecializeAttr>(attr))
        sortedSpecializeAttrs.push_back(specializeAttr);
    }
    // TODO: rm extra copy
    for (auto *specializeAttr : sortBySourceRange(sortedSpecializeAttrs))
      fn(specializeAttr);
  }

  std::vector<AstNode> expandIfConfigClausesThenCullAndSortElementsOrMembers(
      ArrayRef<AstNode> input) const {
    auto cleanedupNodes = sortBySourceRange(cull(expandIfConfigClauses(input)));
    // TODO: uncomment when working on not creating two pattern binding decls at
    // same location.
    //    findCollidingPatterns(cleanedupNodes);
    return cleanedupNodes;
  }

public:
  /// When AstScopes are enabled for code completion,
  /// IfConfigs will pose a challenge because we may need to field lookups into
  /// the inactive clauses, but the Ast contains redundancy: the active clause's
  /// elements are present in the members or elements of an IterableTypeDecl or
  /// BraceStmt alongside of the IfConfigDecl. In addition there are two more
  /// complications:
  ///
  /// 1. The active clause's elements may be nested inside an init self
  /// rebinding decl (as in StringObject.self).
  ///
  /// 2. The active clause may be before or after the inactive ones
  ///
  /// So, when encountering an IfConfigDecl, we will expand  the inactive
  /// elements. Also, always sort members or elements so that the child scopes
  /// are in source order (Just one of several reasons we need to sort.)
  ///
  static const bool includeInactiveIfConfigClauses = false;

private:
  static std::vector<AstNode> expandIfConfigClauses(ArrayRef<AstNode> input) {
    std::vector<AstNode> expansion;
    expandIfConfigClausesInto(expansion, input, /*isInAnActiveNode=*/true);
    return expansion;
  }

  static void expandIfConfigClausesInto(std::vector<AstNode> &expansion,
                                        ArrayRef<AstNode> input,
                                        const bool isInAnActiveNode) {
    for (auto n : input) {
      if (!n.isDecl(DeclKind::IfConfig)) {
        expansion.push_back(n);
        continue;
      }
      auto *const icd = cast<IfConfigDecl>(n.get<Decl *>());
      for (auto &clause : icd->getClauses()) {
        if (auto *const cond = clause.Cond)
          expansion.push_back(cond);
        if (clause.isActive) {
          // TODO: Move this check into AstVerifier
          ast_scope_assert(isInAnActiveNode, "Clause should not be marked active "
                                           "unless it's context is active");
          // get inactive nodes that nest in active clauses
          for (auto n : clause.Elements) {
            if (auto *const d = n.dyn_cast<Decl *>())
              if (auto *const icd = dyn_cast<IfConfigDecl>(d))
                expandIfConfigClausesInto(expansion, {d}, true);
          }
        } else if (includeInactiveIfConfigClauses) {
          expandIfConfigClausesInto(expansion, clause.Elements,
                                    /*isInAnActiveNode=*/false);
        }
      }
    }
  }

  /// Remove VarDecls because we'll find them when we expand the
  /// PatternBindingDecls. Remove EnunCases
  /// because they overlap EnumElements and Ast includes the elements in the
  /// members.
  std::vector<AstNode> cull(ArrayRef<AstNode> input) const {
    // TODO: Investigate whether to move the real EndLoc tracking of
    // SubscriptDecl up into AbstractStorageDecl. May have to cull more.
    std::vector<AstNode> culled;
    llvm::copy_if(input, std::back_inserter(culled), [&](AstNode n) {
      ast_scope_assert(
          !n.isDecl(DeclKind::Accessor),
          "Should not find accessors in iterable types or brace statements");
      return isLocalizable(n) && !n.isDecl(DeclKind::Var) &&
             !n.isDecl(DeclKind::EnumCase);
    });
    return culled;
  }

  /// TODO: The parser yields two decls at the same source loc with the same
  /// kind. TODO:  me when fixing parser's proclivity to create two
  /// PatternBindingDecls at the same source location, then move this to
  /// AstVerifier.
  ///
  /// In all cases the first pattern seems to carry the initializer, and the
  /// second, the accessor
  void findCollidingPatterns(ArrayRef<AstNode> input) const {
    auto dumpPBD = [&](PatternBindingDecl *pbd, const char *which) {
      llvm::errs() << "*** " << which
                   << " pbd isImplicit: " << pbd->isImplicit()
                   << ", #entries: " << pbd->getNumPatternEntries() << " :";
      pbd->getSourceRange().print(llvm::errs(), pbd->getAstContext().SourceMgr,
                                  false);
      llvm::errs() << "\n";
      llvm::errs() << "init: " << pbd->getInit(0) << "\n";
      if (pbd->getInit(0)) {
        llvm::errs() << "SR (init): ";
        pbd->getInit(0)->getSourceRange().print(
            llvm::errs(), pbd->getAstContext().SourceMgr, false);
        llvm::errs() << "\n";
        pbd->getInit(0)->dump(llvm::errs(), 0);
      }
      llvm::errs() << "vars:\n";
      pbd->getPattern(0)->forEachVariable([&](VarDecl *vd) {
        llvm::errs() << "  " << vd->getName()
                     << " implicit: " << vd->isImplicit()
                     << " #accs: " << vd->getAllAccessors().size()
                     << "\nSR (var):";
        vd->getSourceRange().print(llvm::errs(), pbd->getAstContext().SourceMgr,
                                   false);
        llvm::errs() << "\nSR (braces)";
        vd->getBracesRange().print(llvm::errs(), pbd->getAstContext().SourceMgr,
                                   false);
        llvm::errs() << "\n";
        for (auto *a : vd->getAllAccessors()) {
          llvm::errs() << "SR (acc): ";
          a->getSourceRange().print(llvm::errs(),
                                    pbd->getAstContext().SourceMgr, false);
          llvm::errs() << "\n";
          a->dump(llvm::errs(), 0);
        }
      });
    };

    Decl *lastD = nullptr;
    for (auto n : input) {
      auto *d = n.dyn_cast<Decl *>();
      if (!d || !lastD || lastD->getStartLoc() != d->getStartLoc() ||
          lastD->getKind() != d->getKind()) {
        lastD = d;
        continue;
      }
      if (auto *pbd = dyn_cast<PatternBindingDecl>(lastD))
        dumpPBD(pbd, "prev");
      if (auto *pbd = dyn_cast<PatternBindingDecl>(d)) {
        dumpPBD(pbd, "curr");
        ast_scope_unreachable("found colliding pattern binding decls");
      }
      llvm::errs() << "Two same kind decls at same loc: \n";
      lastD->dump(llvm::errs());
      llvm::errs() << "and\n";
      d->dump(llvm::errs());
      ast_scope_unreachable("Two same kind decls; unexpected kinds");
    }
  }

  /// Templated to work on either AstNodes, Decl*'s, or whatnot.
  template <typename Rangeable>
  std::vector<Rangeable>
  sortBySourceRange(std::vector<Rangeable> toBeSorted) const {
    auto compareNodes = [&](Rangeable n1, Rangeable n2) {
      return isNotAfter(n1, n2);
    };
    std::stable_sort(toBeSorted.begin(), toBeSorted.end(), compareNodes);
    return toBeSorted;
  }

  template <typename Rangeable>
  bool isNotAfter(Rangeable n1, Rangeable n2) const {
    const auto r1 = getRangeableSourceRange(n1);
    const auto r2 = getRangeableSourceRange(n2);

    const int signum = AstScopeImpl::compare(r1, r2, ctx.SourceMgr,
                                             /*ensureDisjoint=*/true);
    return -1 == signum;
  }

  static bool isVarDeclInPatternBindingDecl(AstNode n1, AstNode n2) {
    if (auto *d1 = n1.dyn_cast<Decl *>())
      if (auto *vd = dyn_cast<VarDecl>(d1))
        if (auto *d2 = n2.dyn_cast<Decl *>())
          if (auto *pbd = dyn_cast<PatternBindingDecl>(d2))
            return vd->getParentPatternBinding() == pbd;
    return false;
  }

public:
  bool shouldThisNodeBeScopedWhenFoundInSourceFileBraceStmtOrType(AstNode n) {
    // Do not scope VarDecls because
    // they get created directly by the pattern code.
    // Doing otherwise distorts the source range
    // of their parents.
    ast_scope_assert(!n.isDecl(DeclKind::Accessor),
                   "Should not see accessors here");
    // Can occur in illegal code
    if (auto *const s = n.dyn_cast<Stmt *>()) {
      if (auto *const bs = dyn_cast<BraceStmt>(s))
        ast_scope_assert(bs->empty(), "Might mess up insertion point");
    }
    return !n.isDecl(DeclKind::Var);
  }

  bool shouldBeLazy() const { return ctx.LangOpts.LazyAstScopes; }

public:
  /// For debugging. Return true if scope tree contains all the decl contexts in
  /// the Ast May modify the scope tree in order to update obsolete scopes.
  /// Likely slow.
  bool containsAllDeclContextsFromAst() {
    auto allDeclContexts = findLocalizableDeclContextsInAst();
    llvm::DenseMap<const DeclContext *, const AstScopeImpl *> bogusDCs;
    sourceFileScope->preOrderDo([&](AstScopeImpl *scope) {
      scope->expandAndBeCurrentDetectingRecursion(*this);
    });
    sourceFileScope->postOrderDo([&](AstScopeImpl *scope) {
      if (auto *dc = scope->getDeclContext().getPtrOrNull()) {
        auto iter = allDeclContexts.find(dc);
        if (iter != allDeclContexts.end())
          ++iter->second;
        else
          bogusDCs.insert({dc, scope});
      }
    });

    auto printDecl = [&](const Decl *d) {
      llvm::errs() << "\ngetAsDecl() -> " << d << " ";
      d->getSourceRange().print(llvm::errs(), ctx.SourceMgr);
      llvm::errs() << " : ";
      d->dump(llvm::errs());
      llvm::errs() << "\n";
    };
    bool foundOmission = false;
    for (const auto &p : allDeclContexts) {
      if (p.second == 0) {
        if (auto *d = p.first->getAsDecl()) {
          if (isLocalizable(d)) {
            llvm::errs() << "\nAstScope tree omitted DeclContext: " << p.first
                         << " "
                         << ":\n";
            p.first->printContext(llvm::errs());
            printDecl(d);
            foundOmission = true;
          }
        } else {
          // If no decl, no source range, so no scope
        }
      }
    }
    for (const auto dcAndScope : bogusDCs) {
      llvm::errs() << "AstScope tree confabulated: " << dcAndScope.getFirst()
                   << ":\n";
      dcAndScope.getFirst()->printContext(llvm::errs());
      if (auto *d = dcAndScope.getFirst()->getAsDecl())
        printDecl(d);
      dcAndScope.getSecond()->print(llvm::errs(), 0, false);
    }
    return !foundOmission && bogusDCs.empty();
  }

private:
  /// Return a map of every DeclContext in the Ast, and zero in the 2nd element.
  /// For debugging.
  llvm::DenseMap<const DeclContext *, unsigned>
  findLocalizableDeclContextsInAst() const;

public:
  POLAR_DEBUG_DUMP { print(llvm::errs()); }

  void print(raw_ostream &out) const {
    out << "(swift::AstSourceFileScope*) " << sourceFileScope << "\n";
  }

  // Make vanilla new illegal.
  void *operator new(size_t bytes) = delete;

  // Only allow allocation of scopes using the allocator of a particular source
  // file.
  void *operator new(size_t bytes, const AstContext &ctx,
                     unsigned alignment = alignof(ScopeCreator));
  void *operator new(size_t Bytes, void *Mem) {
    ast_scope_assert(Mem, "Allocation failed");
    return Mem;
  }
};
} // ast_scope
} // namespace polar

#pragma mark Scope tree creation and extension

AstScope::AstScope(SourceFile *SF) : impl(createScopeTree(SF)) {}

void AstScope::buildFullyExpandedTree() { impl->buildFullyExpandedTree(); }

void AstScope::
    buildEnoughOfTreeForTopLevelExpressionsButDontRequestGenericsOrExtendedNominals() {
  impl->buildEnoughOfTreeForTopLevelExpressionsButDontRequestGenericsOrExtendedNominals();
}

bool AstScope::areInactiveIfConfigClausesSupported() {
  return ScopeCreator::includeInactiveIfConfigClauses;
}

void AstScope::expandFunctionBody(AbstractFunctionDecl *AFD) {
  auto *const SF = AFD->getParentSourceFile();
  if (SF->isSuitableForAstScopes())
    SF->getScope().expandFunctionBodyImpl(AFD);
}

void AstScope::expandFunctionBodyImpl(AbstractFunctionDecl *AFD) {
  impl->expandFunctionBody(AFD);
}

AstSourceFileScope *AstScope::createScopeTree(SourceFile *SF) {
  ScopeCreator *scopeCreator = new (SF->getAstContext()) ScopeCreator(SF);
  return scopeCreator->sourceFileScope;
}

void AstSourceFileScope::buildFullyExpandedTree() {
  expandAndBeCurrentDetectingRecursion(*scopeCreator);
  preOrderChildrenDo([&](AstScopeImpl *s) {
    s->expandAndBeCurrentDetectingRecursion(*scopeCreator);
  });
}

void AstSourceFileScope::
    buildEnoughOfTreeForTopLevelExpressionsButDontRequestGenericsOrExtendedNominals() {
      expandAndBeCurrentDetectingRecursion(*scopeCreator);
}

void AstSourceFileScope::expandFunctionBody(AbstractFunctionDecl *AFD) {
  if (!AFD)
    return;
  auto sr = AFD->getBodySourceRange();
  if (sr.isInvalid())
    return;
  AstScopeImpl *bodyScope = findInnermostEnclosingScope(sr.start, nullptr);
  bodyScope->expandAndBeCurrentDetectingRecursion(*scopeCreator);
}

AstSourceFileScope::AstSourceFileScope(SourceFile *SF,
                                       ScopeCreator *scopeCreator)
    : SF(SF), scopeCreator(scopeCreator), insertionPoint(this) {}

#pragma mark NodeAdder

namespace polar {
namespace ast_scope {

class NodeAdder
    : public AstVisitor<NodeAdder, NullablePtr<AstScopeImpl>,
                        NullablePtr<AstScopeImpl>, NullablePtr<AstScopeImpl>,
                        void, void, void, AstScopeImpl *, ScopeCreator &> {
public:

#pragma mark AstNodes that do not create scopes

  // Even ignored Decls and Stmts must extend the source range of a scope:
  // E.g. a braceStmt with some definitions that ends in a statement that
  // accesses such a definition must resolve as being IN the scope.

#define VISIT_AND_IGNORE(What)                                                 \
  NullablePtr<AstScopeImpl> visit##What(What *w, AstScopeImpl *p,              \
                                        ScopeCreator &) {                      \
    p->widenSourceRangeForIgnoredAstNode(w);                                   \
    return p;                                                                  \
  }

  VISIT_AND_IGNORE(ImportDecl)
  VISIT_AND_IGNORE(EnumCaseDecl)
  VISIT_AND_IGNORE(PrecedenceGroupDecl)
  VISIT_AND_IGNORE(InfixOperatorDecl)
  VISIT_AND_IGNORE(PrefixOperatorDecl)
  VISIT_AND_IGNORE(PostfixOperatorDecl)
  VISIT_AND_IGNORE(GenericTypeParamDecl)
  VISIT_AND_IGNORE(AssociatedTypeDecl)
  VISIT_AND_IGNORE(ModuleDecl)
  VISIT_AND_IGNORE(ParamDecl)
  VISIT_AND_IGNORE(PoundDiagnosticDecl)
  VISIT_AND_IGNORE(MissingMemberDecl)

  // This declaration is handled from the PatternBindingDecl
  VISIT_AND_IGNORE(VarDecl)

  // These contain nothing to scope.
  VISIT_AND_IGNORE(BreakStmt)
  VISIT_AND_IGNORE(ContinueStmt)
  VISIT_AND_IGNORE(FallthroughStmt)
  VISIT_AND_IGNORE(FailStmt)

#undef VISIT_AND_IGNORE

#pragma mark simple creation ignoring deferred nodes

#define VISIT_AND_CREATE(What, ScopeClass)                                     \
  NullablePtr<AstScopeImpl> visit##What(What *w, AstScopeImpl *p,              \
                                        ScopeCreator &scopeCreator) {          \
    return scopeCreator.ifUniqueConstructExpandAndInsert<ScopeClass>(p, w);    \
  }

  VISIT_AND_CREATE(SubscriptDecl, SubscriptDeclScope)
  VISIT_AND_CREATE(IfStmt, IfStmtScope)
  VISIT_AND_CREATE(WhileStmt, WhileStmtScope)
  VISIT_AND_CREATE(RepeatWhileStmt, RepeatWhileScope)
  VISIT_AND_CREATE(DoCatchStmt, DoCatchStmtScope)
  VISIT_AND_CREATE(SwitchStmt, SwitchStmtScope)
  VISIT_AND_CREATE(ForEachStmt, ForEachStmtScope)
  VISIT_AND_CREATE(CatchStmt, CatchStmtScope)
  VISIT_AND_CREATE(CaseStmt, CaseStmtScope)
  VISIT_AND_CREATE(AbstractFunctionDecl, AbstractFunctionDeclScope)

#undef VISIT_AND_CREATE

#pragma mark 2D simple creation (ignoring deferred nodes)

#define VISIT_AND_CREATE_WHOLE_PORTION(What, WhatScope)                        \
  NullablePtr<AstScopeImpl> visit##What(What *w, AstScopeImpl *p,              \
                                        ScopeCreator &scopeCreator) {          \
    return scopeCreator.ifUniqueConstructWithPortionExpandAndInsert<           \
        WhatScope, GenericTypeOrExtensionWholePortion>(p, w);                  \
  }

  VISIT_AND_CREATE_WHOLE_PORTION(ExtensionDecl, ExtensionScope)
  VISIT_AND_CREATE_WHOLE_PORTION(StructDecl, NominalTypeScope)
  VISIT_AND_CREATE_WHOLE_PORTION(ClassDecl, NominalTypeScope)
  VISIT_AND_CREATE_WHOLE_PORTION(InterfaceDecl, NominalTypeScope)
  VISIT_AND_CREATE_WHOLE_PORTION(EnumDecl, NominalTypeScope)
  VISIT_AND_CREATE_WHOLE_PORTION(TypeAliasDecl, TypeAliasScope)
  VISIT_AND_CREATE_WHOLE_PORTION(OpaqueTypeDecl, OpaqueTypeScope)
#undef VISIT_AND_CREATE_WHOLE_PORTION

  // This declaration is handled from
  // addChildrenForAllLocalizableAccessorsInSourceOrder
  NullablePtr<AstScopeImpl> visitAccessorDecl(AccessorDecl *ad, AstScopeImpl *p,
                                              ScopeCreator &scopeCreator) {
    return visitAbstractFunctionDecl(ad, p, scopeCreator);
  }

#pragma mark simple creation with deferred nodes

  // Each of the following creates a new scope, so that nodes which were parsed
  // after them need to be placed in scopes BELOW them in the tree. So pass down
  // the deferred nodes.
  NullablePtr<AstScopeImpl> visitGuardStmt(GuardStmt *e, AstScopeImpl *p,
                                           ScopeCreator &scopeCreator) {
    return scopeCreator.ifUniqueConstructExpandAndInsert<GuardStmtScope>(p, e);
  }
  NullablePtr<AstScopeImpl> visitDoStmt(DoStmt *ds, AstScopeImpl *p,
                                        ScopeCreator &scopeCreator) {
    scopeCreator.addToScopeTreeAndReturnInsertionPoint(ds->getBody(), p);
    return p; // Don't put subsequent decls inside the "do"
  }
  NullablePtr<AstScopeImpl> visitTopLevelCodeDecl(TopLevelCodeDecl *d,
                                                  AstScopeImpl *p,
                                                  ScopeCreator &scopeCreator) {
    return scopeCreator.ifUniqueConstructExpandAndInsert<TopLevelCodeScope>(p,
                                                                            d);
  }

#pragma mark special-case creation

  AstScopeImpl *visitSourceFile(SourceFile *, AstScopeImpl *, ScopeCreator &) {
    ast_scope_unreachable("SourceFiles are orphans.");
  }

  NullablePtr<AstScopeImpl> visitYieldStmt(YieldStmt *ys, AstScopeImpl *p,
                                           ScopeCreator &scopeCreator) {
    for (Expr *e : ys->getYields())
      visitExpr(e, p, scopeCreator);
    return p;
  }

  NullablePtr<AstScopeImpl> visitDeferStmt(DeferStmt *ds, AstScopeImpl *p,
                                           ScopeCreator &scopeCreator) {
    visitFuncDecl(ds->getTempDecl(), p, scopeCreator);
    return p;
  }

  NullablePtr<AstScopeImpl> visitBraceStmt(BraceStmt *bs, AstScopeImpl *p,
                                           ScopeCreator &scopeCreator) {
    auto maybeBraceScope =
        scopeCreator.ifUniqueConstructExpandAndInsert<BraceStmtScope>(p, bs);
    if (auto *s = scopeCreator.getAstContext().Stats)
      ++s->getFrontendCounters().NumBraceStmtAstScopes;
    return maybeBraceScope.getPtrOr(p);
  }

  NullablePtr<AstScopeImpl>
  visitPatternBindingDecl(PatternBindingDecl *patternBinding,
                          AstScopeImpl *parentScope,
                          ScopeCreator &scopeCreator) {
    scopeCreator.addAnyAttachedPropertyWrappersToScopeTree(patternBinding,
                                                           parentScope);

    const bool isInTypeDecl = parentScope->isATypeDeclScope();

    const DeclVisibilityKind vis =
        isInTypeDecl ? DeclVisibilityKind::MemberOfCurrentNominal
                     : DeclVisibilityKind::LocalVariable;
    auto *insertionPoint = parentScope;
    for (auto i : range(patternBinding->getNumPatternEntries())) {
      // TODO: Won't need to do so much work to avoid creating one without
      // a SourceRange once parser is fixed to not create two
      // PatternBindingDecls with same locaiton and getSourceRangeOfThisAstNode
      // for PatternEntryDeclScope is simplified to use the PatternEntry's
      // source range.
      if (!patternBinding->getOriginalInit(i)) {
        bool found = false;
        patternBinding->getPattern(i)->forEachVariable([&](VarDecl *vd) {
          if (!vd->isImplicit())
            found = true;
          else
            found |= llvm::any_of(vd->getAllAccessors(), [&](AccessorDecl *a) {
              return isLocalizable(a);
            });
        });
        if (!found)
          continue;
      }
      insertionPoint =
          scopeCreator
              .ifUniqueConstructExpandAndInsert<PatternEntryDeclScope>(
                  insertionPoint, patternBinding, i, vis)
              .getPtrOr(insertionPoint);
    }
    // If in a type decl, the type search will find these,
    // but if in a brace stmt, must continue under the last binding.
    return isInTypeDecl ? parentScope : insertionPoint;
  }

  NullablePtr<AstScopeImpl> visitEnumElementDecl(EnumElementDecl *eed,
                                                 AstScopeImpl *p,
                                                 ScopeCreator &scopeCreator) {
    scopeCreator.constructExpandAndInsertUncheckable<EnumElementScope>(p, eed);
    return p;
  }

  NullablePtr<AstScopeImpl> visitIfConfigDecl(IfConfigDecl *icd,
                                              AstScopeImpl *p,
                                              ScopeCreator &scopeCreator) {
    ast_scope_unreachable(
        "Should be handled inside of "
        "expandIfConfigClausesThenCullAndSortElementsOrMembers");
  }

  NullablePtr<AstScopeImpl> visitReturnStmt(ReturnStmt *rs, AstScopeImpl *p,
                                            ScopeCreator &scopeCreator) {
    if (rs->hasResult())
      visitExpr(rs->getResult(), p, scopeCreator);
    return p;
  }

  NullablePtr<AstScopeImpl> visitThrowStmt(ThrowStmt *ts, AstScopeImpl *p,
                                           ScopeCreator &scopeCreator) {
    visitExpr(ts->getSubExpr(), p, scopeCreator);
    return p;
  }

  NullablePtr<AstScopeImpl> visitPoundAssertStmt(PoundAssertStmt *pas,
                                                 AstScopeImpl *p,
                                                 ScopeCreator &scopeCreator) {
    visitExpr(pas->getCondition(), p, scopeCreator);
    return p;
  }

  NullablePtr<AstScopeImpl> visitExpr(Expr *expr, AstScopeImpl *p,
                                      ScopeCreator &scopeCreator) {
    if (expr) {
      p->widenSourceRangeForIgnoredAstNode(expr);
      scopeCreator.addExprToScopeTree(expr, p);
    }
    return p;
  }
};
} // namespace ast_scope
} // namespace polar

// These definitions are way down here so it can call into
// NodeAdder
NullablePtr<AstScopeImpl>
ScopeCreator::addToScopeTreeAndReturnInsertionPoint(AstNode n,
                                                    AstScopeImpl *parent) {
  if (!isWorthTryingToCreateScopeFor(n))
    return parent;
  if (auto *p = n.dyn_cast<Decl *>())
    return NodeAdder().visit(p, parent, *this);
  if (auto *p = n.dyn_cast<Expr *>())
    return NodeAdder().visit(p, parent, *this);
  auto *p = n.get<Stmt *>();
  return NodeAdder().visit(p, parent, *this);
}

void ScopeCreator::addChildrenForAllLocalizableAccessorsInSourceOrder(
    AbstractStorageDecl *asd, AstScopeImpl *parent) {
  // Accessors are always nested within their abstract storage
  // declaration. The nesting may not be immediate, because subscripts may
  // have intervening scopes for generics.
  AbstractStorageDecl *const enclosingAbstractStorageDecl =
      parent->getEnclosingAbstractStorageDecl().get();

  std::vector<AccessorDecl *> accessorsToScope;
  // Assume we don't have to deal with inactive clauses of IfConfigs here
  llvm::copy_if(asd->getAllAccessors(), std::back_inserter(accessorsToScope),
                [&](AccessorDecl *ad) {
                  return enclosingAbstractStorageDecl == ad->getStorage();
                });

  // Sort in order to include synthesized ones, which are out of order.
  for (auto *accessor : sortBySourceRange(accessorsToScope))
    addToScopeTree(accessor, parent);
}

#pragma mark creation helpers

void AstScopeImpl::addChild(AstScopeImpl *child, AstContext &ctx) {
  // If this is the first time we've added children, notify the AstContext
  // that there's a SmallVector that needs to be cleaned up.
  // FIXME: If we had access to SmallVector::isSmall(), we could do better.
  if (storedChildren.empty() && !haveAddedCleanup) {
    ctx.addDestructorCleanup(storedChildren);
    haveAddedCleanup = true;
  }
  storedChildren.push_back(child);
  ast_scope_assert(!child->getParent(), "child should not already have parent");
  child->parent = this;
  clearCachedSourceRangesOfMeAndAncestors();
}

void AstScopeImpl::removeChildren() {
  clearCachedSourceRangesOfMeAndAncestors();
  storedChildren.clear();
}

void AstScopeImpl::disownDescendants(ScopeCreator &scopeCreator) {
  for (auto *c : getChildren()) {
    c->disownDescendants(scopeCreator);
    c->emancipate();
    scopeCreator.scopedNodes.erase(c);
  }
  removeChildren();
}

#pragma mark implementations of expansion

AstScopeImpl *
AstScopeImpl::expandAndBeCurrentDetectingRecursion(ScopeCreator &scopeCreator) {
  assert(scopeCreator.getAstContext().LangOpts.EnableAstScopeLookup &&
         "Should not be getting here if AstScopes are disabled");
  return evaluateOrDefault(scopeCreator.getAstContext().evaluator,
                           ExpandAstScopeRequest{this, &scopeCreator}, nullptr);
}

llvm::Expected<AstScopeImpl *>
ExpandAstScopeRequest::evaluate(Evaluator &evaluator, AstScopeImpl *parent,
                                ScopeCreator *scopeCreator) const {
  auto *insertionPoint = parent->expandAndBeCurrent(*scopeCreator);
  ast_scope_assert(insertionPoint,
                 "Used to return a null pointer if the insertion point would "
                 "not be used, but it breaks the request dependency hashing");
  return insertionPoint;
}

bool AstScopeImpl::doesExpansionOnlyAddNewDeclsAtEnd() const { return false; }
bool AstSourceFileScope::doesExpansionOnlyAddNewDeclsAtEnd() const {
  return true;
}

AstScopeImpl *AstScopeImpl::expandAndBeCurrent(ScopeCreator &scopeCreator) {

  // We might be reexpanding, so save any scopes that were inserted here from
  // above it in the Ast
  auto astAncestorScopes = rescueAstAncestorScopesForReuseFromMeOrDescendants();
  ast_scope_assert(astAncestorScopes.empty() ||
                     !doesExpansionOnlyAddNewDeclsAtEnd(),
                 "AstSourceFileScope has no ancestors to be rescued.");

  // If reexpanding, we need to remove descendant decls from the duplication set
  // in order to re-add them as sub-scopes. Since expansion only adds new Decls
  // at end, don't bother with descendants
  if (!doesExpansionOnlyAddNewDeclsAtEnd())
    disownDescendants(scopeCreator);

  auto *insertionPoint = expandSpecifically(scopeCreator);
  if (scopeCreator.shouldBeLazy()) {
    ast_scope_assert(!insertionPointForDeferredExpansion() ||
                       insertionPointForDeferredExpansion().get() ==
                           insertionPoint,
                   "In order for lookups into lazily-expanded scopes to be "
                   "accurate before expansion, the insertion point before "
                   "expansion must be the same as after expansion.");
  }
  replaceAstAncestorScopes(astAncestorScopes);
  setWasExpanded();
  beCurrent();
  ast_scope_assert(checkSourceRangeAfterExpansion(scopeCreator.getAstContext()),
                 "Bad range.");
  return insertionPoint;
}

  // Do this whole bit so it's easy to see which type of scope is which

#define CREATES_NEW_INSERTION_POINT(Scope)                                     \
  AstScopeImpl *Scope::expandSpecifically(ScopeCreator &scopeCreator) {        \
    return expandAScopeThatCreatesANewInsertionPoint(scopeCreator)             \
        .insertionPoint;                                                       \
  }

#define NO_NEW_INSERTION_POINT(Scope)                                          \
  AstScopeImpl *Scope::expandSpecifically(ScopeCreator &scopeCreator) {        \
    expandAScopeThatDoesNotCreateANewInsertionPoint(scopeCreator);             \
    return getParent().get();                                                  \
  }

// Return this in particular for GenericParamScope so body is scoped under it
#define NO_EXPANSION(Scope)                                                    \
  AstScopeImpl *Scope::expandSpecifically(ScopeCreator &) { return this; }

CREATES_NEW_INSERTION_POINT(AstSourceFileScope)
CREATES_NEW_INSERTION_POINT(ParameterListScope)
CREATES_NEW_INSERTION_POINT(ConditionalClauseScope)
CREATES_NEW_INSERTION_POINT(GuardStmtScope)
CREATES_NEW_INSERTION_POINT(PatternEntryDeclScope)
CREATES_NEW_INSERTION_POINT(PatternEntryInitializerScope)
CREATES_NEW_INSERTION_POINT(GenericTypeOrExtensionScope)
CREATES_NEW_INSERTION_POINT(BraceStmtScope)
CREATES_NEW_INSERTION_POINT(TopLevelCodeScope)

NO_NEW_INSERTION_POINT(AbstractFunctionBodyScope)
NO_NEW_INSERTION_POINT(AbstractFunctionDeclScope)
NO_NEW_INSERTION_POINT(AttachedPropertyWrapperScope)
NO_NEW_INSERTION_POINT(EnumElementScope)

NO_NEW_INSERTION_POINT(CaptureListScope)
NO_NEW_INSERTION_POINT(CaseStmtScope)
NO_NEW_INSERTION_POINT(CatchStmtScope)
NO_NEW_INSERTION_POINT(ClosureBodyScope)
NO_NEW_INSERTION_POINT(DefaultArgumentInitializerScope)
NO_NEW_INSERTION_POINT(DoCatchStmtScope)
NO_NEW_INSERTION_POINT(ForEachPatternScope)
NO_NEW_INSERTION_POINT(ForEachStmtScope)
NO_NEW_INSERTION_POINT(IfStmtScope)
NO_NEW_INSERTION_POINT(RepeatWhileScope)
NO_NEW_INSERTION_POINT(SubscriptDeclScope)
NO_NEW_INSERTION_POINT(SwitchStmtScope)
NO_NEW_INSERTION_POINT(VarDeclScope)
NO_NEW_INSERTION_POINT(WhileStmtScope)
NO_NEW_INSERTION_POINT(WholeClosureScope)

NO_EXPANSION(GenericParamScope)
NO_EXPANSION(ClosureParametersScope)
NO_EXPANSION(SpecializeAttributeScope)
NO_EXPANSION(ConditionalClausePatternUseScope)
NO_EXPANSION(LookupParentDiversionScope)

#undef CREATES_NEW_INSERTION_POINT
#undef NO_NEW_INSERTION_POINT

AnnotatedInsertionPoint
AstSourceFileScope::expandAScopeThatCreatesANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  ast_scope_assert(SF, "Must already have a SourceFile.");
  ArrayRef<Decl *> decls = SF->Decls;
  // Assume that decls are only added at the end, in source order
  ArrayRef<Decl *> newDecls = decls.slice(numberOfDeclsAlreadySeen);
  std::vector<AstNode> newNodes(newDecls.begin(), newDecls.end());
  insertionPoint =
      scopeCreator.addSiblingsToScopeTree(insertionPoint, this, newNodes);
  // Too slow to perform all the time:
  //    ast_scope_assert(scopeCreator->containsAllDeclContextsFromAst(),
  //           "AstScope tree missed some DeclContexts or made some up");
  return {insertionPoint, "Next time decls are added they go here."};
}

AnnotatedInsertionPoint
ParameterListScope::expandAScopeThatCreatesANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  // Each initializer for a function parameter is its own, sibling, scope.
  // Unlike generic parameters or pattern initializers, it cannot refer to a
  // previous parameter.
  for (ParamDecl *pd : params->getArray()) {
    if (pd->hasDefaultExpr())
      scopeCreator
          .constructExpandAndInsertUncheckable<DefaultArgumentInitializerScope>(
              this, pd);
  }
  return {this, "body of func goes under me"};
}

AnnotatedInsertionPoint
PatternEntryDeclScope::expandAScopeThatCreatesANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  // Initializers come before VarDecls, e.g. PCMacro/didSet.swift 19
  auto patternEntry = getPatternEntry();
  // Create a child for the initializer, if present.
  // Cannot trust the source range given in the AstScopeImpl for the end of the
  // initializer (because of InterpolatedLiteralStrings and EditorPlaceHolders),
  // so compute it ourselves.
  // Even if this predicate fails, there may be an initContext but
  // we cannot make a scope for it, since no source range.
  if (patternEntry.getOriginalInit() &&
      isLocalizable(patternEntry.getOriginalInit())) {
    ast_scope_assert(
        !getSourceManager().isBeforeInBuffer(
            patternEntry.getOriginalInit()->getStartLoc(), decl->getStartLoc()),
        "Original inits are always after the '='");
    scopeCreator
        .constructExpandAndInsertUncheckable<PatternEntryInitializerScope>(
            this, decl, patternEntryIndex, vis);
  }
  // Add accessors for the variables in this pattern.
  forEachVarDeclWithLocalizableAccessors(scopeCreator, [&](VarDecl *var) {
    scopeCreator.ifUniqueConstructExpandAndInsert<VarDeclScope>(this, var);
  });
  ast_scope_assert(!handleUseBeforeDef,
                 "next line is wrong otherwise; would need a use scope");

  return {getParent().get(), "When not handling use-before-def, succeeding "
                             "code just goes in the same scope as this one"};
}

AnnotatedInsertionPoint
PatternEntryInitializerScope::expandAScopeThatCreatesANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  // Create a child for the initializer expression.
  scopeCreator.addToScopeTree(AstNode(getPatternEntry().getOriginalInit()),
                              this);
  if (handleUseBeforeDef)
    return {this, "PatternEntryDeclScope::expand.* needs initializer scope to "
                  "get its endpoint in order to push back start of "
                  "PatternEntryUseScope"};

  // null pointer here blows up request printing
  return {getParent().get(), "Unused"};
}

AnnotatedInsertionPoint
ConditionalClauseScope::expandAScopeThatCreatesANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  const StmtConditionElement &sec = getStmtConditionElement();
  switch (sec.getKind()) {
  case StmtConditionElement::CK_Availability:
    return {this, "No introduced variables"};
  case StmtConditionElement::CK_Boolean:
    scopeCreator.addToScopeTree(sec.getBoolean(), this);
    return {this, "No introduced variables"};
  case StmtConditionElement::CK_PatternBinding:
    scopeCreator.addToScopeTree(sec.getInitializer(), this);
    auto *const ccPatternUseScope =
        scopeCreator.constructExpandAndInsertUncheckable<
            ConditionalClausePatternUseScope>(this, sec.getPattern(), endLoc);
    return {ccPatternUseScope,
            "Succeeding code must be in scope of conditional variables"};
  }
  ast_scope_unreachable("Unhandled StmtConditionKind in switch");
}

AnnotatedInsertionPoint
GuardStmtScope::expandAScopeThatCreatesANewInsertionPoint(ScopeCreator &
                                                          scopeCreator) {

  AstScopeImpl *conditionLookupParent =
      createNestedConditionalClauseScopes(scopeCreator, stmt->getBody());
  // Add a child for the 'guard' body, which always exits.
  // Parent is whole guard stmt scope, NOT the cond scopes
  scopeCreator.addToScopeTree(stmt->getBody(), this);

  auto *const lookupParentDiversionScope =
      scopeCreator
          .constructExpandAndInsertUncheckable<LookupParentDiversionScope>(
              this, conditionLookupParent, stmt->getEndLoc());
  return {lookupParentDiversionScope,
          "Succeeding code must be in scope of guard variables"};
}

AnnotatedInsertionPoint
GenericTypeOrExtensionScope::expandAScopeThatCreatesANewInsertionPoint(
    ScopeCreator & scopeCreator) {
  return {portion->expandScope(this, scopeCreator),
          "<X: Foo, Y: X> is legal, so nest these"};
}

AnnotatedInsertionPoint
BraceStmtScope::expandAScopeThatCreatesANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  // TODO: remove the sort after fixing parser to create brace statement
  // elements in source order
  auto *insertionPoint =
      scopeCreator.addSiblingsToScopeTree(this, this, stmt->getElements());
  if (auto *s = scopeCreator.getAstContext().Stats)
    ++s->getFrontendCounters().NumBraceStmtAstScopeExpansions;
  return {
      insertionPoint,
      "For top-level code decls, need the scope under, say a guard statment."};
}

AnnotatedInsertionPoint
TopLevelCodeScope::expandAScopeThatCreatesANewInsertionPoint(ScopeCreator &
                                                             scopeCreator) {

  if (auto *body =
          scopeCreator
              .addToScopeTreeAndReturnInsertionPoint(decl->getBody(), this)
              .getPtrOrNull())
    return {body, "So next top level code scope and put its decls in its body "
                  "under a guard statement scope (etc) from the last top level "
                  "code scope"};
  return {this, "No body"};
}

#pragma mark expandAScopeThatDoesNotCreateANewInsertionPoint

// Create child scopes for every declaration in a body.

void AbstractFunctionDeclScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  // Create scopes for specialize attributes
  scopeCreator.forEachSpecializeAttrInSourceOrder(
      decl, [&](SpecializeAttr *specializeAttr) {
        scopeCreator.ifUniqueConstructExpandAndInsert<SpecializeAttributeScope>(
            this, specializeAttr, decl);
      });
  // Create scopes for generic and ordinary parameters.
  // For a subscript declaration, the generic and ordinary parameters are in an
  // ancestor scope, so don't make them here.
  AstScopeImpl *leaf = this;
  if (!isa<AccessorDecl>(decl)) {
    leaf = scopeCreator.addNestedGenericParamScopesToTree(
        decl, decl->getGenericParams(), leaf);
    if (isLocalizable(decl) && getParmsSourceLocOfAFD(decl).isValid()) {
      // swift::createDesignatedInitOverride just clones the parameters, so they
      // end up with a bogus SourceRange, maybe *before* the start of the
      // function.
      if (!decl->isImplicit()) {
        leaf = scopeCreator
                   .constructExpandAndInsertUncheckable<ParameterListScope>(
                       leaf, decl->getParameters(), nullptr);
      }
    }
  }
  // Create scope for the body.
  // We create body scopes when there is no body for source kit to complete
  // erroneous code in bodies.
  if (decl->getBodySourceRange().isValid()) {
    if (AbstractFunctionBodyScope::isAMethod(decl))
      scopeCreator.constructExpandAndInsertUncheckable<MethodBodyScope>(leaf,
                                                                        decl);
    else
      scopeCreator.constructExpandAndInsertUncheckable<PureFunctionBodyScope>(
          leaf, decl);
  }
}

void EnumElementScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  if (auto *pl = decl->getParameterList())
    scopeCreator.constructExpandAndInsertUncheckable<ParameterListScope>(
        this, pl, nullptr);
  // The invariant that the raw value expression can never introduce a new scope
  // is checked in Parse.  However, this guarantee is not future-proof.  Compute
  // and add the raw value expression anyways just to be defensive.
  //
  // FIXME: Re-enable this.  It currently crashes for malformed enum cases.
  // scopeCreator.addToScopeTree(decl->getStructuralRawValueExpr(), this);
}

void AbstractFunctionBodyScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  expandBody(scopeCreator);
}

void IfStmtScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  AstScopeImpl *insertionPoint =
      createNestedConditionalClauseScopes(scopeCreator, stmt->getThenStmt());

  // The 'then' branch
  scopeCreator.addToScopeTree(stmt->getThenStmt(), insertionPoint);

  // Add the 'else' branch, if needed.
  scopeCreator.addToScopeTree(stmt->getElseStmt(), this);
}

void WhileStmtScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  AstScopeImpl *insertionPoint =
      createNestedConditionalClauseScopes(scopeCreator, stmt->getBody());
  scopeCreator.addToScopeTree(stmt->getBody(), insertionPoint);
}

void RepeatWhileScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(stmt->getBody(), this);
  scopeCreator.addToScopeTree(stmt->getCond(), this);
}

void DoCatchStmtScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(stmt->getBody(), this);

  for (auto catchClause : stmt->getCatches())
    scopeCreator.addToScopeTree(catchClause, this);
}

void SwitchStmtScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(stmt->getSubjectExpr(), this);

  for (auto caseStmt : stmt->getCases()) {
    if (isLocalizable(caseStmt))
      scopeCreator.ifUniqueConstructExpandAndInsert<CaseStmtScope>(this,
                                                                   caseStmt);
  }
}

void ForEachStmtScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(stmt->getSequence(), this);

  // Add a child describing the scope of the pattern.
  // In error cases such as:
  //    let v: C { for b : Int -> S((array: P { }
  // the body is implicit and it would overlap the source range of the expr
  // above.
  if (!stmt->getBody()->isImplicit()) {
    if (isLocalizable(stmt->getBody()))
      scopeCreator.constructExpandAndInsertUncheckable<ForEachPatternScope>(
          this, stmt);
  }
}

void ForEachPatternScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(stmt->getWhere(), this);
  scopeCreator.addToScopeTree(stmt->getBody(), this);
}

void CatchStmtScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(stmt->getGuardExpr(), this);
  scopeCreator.addToScopeTree(stmt->getBody(), this);
}

void CaseStmtScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  for (auto &caseItem : stmt->getMutableCaseLabelItems())
    scopeCreator.addToScopeTree(caseItem.getGuardExpr(), this);

  // Add a child for the case body.
  scopeCreator.addToScopeTree(stmt->getBody(), this);
}

void VarDeclScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addChildrenForAllLocalizableAccessorsInSourceOrder(decl, this);
}

void SubscriptDeclScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  auto *sub = decl;
  auto *leaf = scopeCreator.addNestedGenericParamScopesToTree(
      sub, sub->getGenericParams(), this);
  auto *params =
      scopeCreator.constructExpandAndInsertUncheckable<ParameterListScope>(
          leaf, sub->getIndices(), sub->getAccessor(AccessorKind::Get));
  scopeCreator.addChildrenForAllLocalizableAccessorsInSourceOrder(sub, params);
}

void WholeClosureScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  if (auto *cl = captureList.getPtrOrNull())
    scopeCreator.ensureUniqueThenConstructExpandAndInsert<CaptureListScope>(
        this, cl);
  AstScopeImpl *bodyParent = this;
  if (closureExpr->getInLoc().isValid())
    bodyParent =
        scopeCreator
            .constructExpandAndInsertUncheckable<ClosureParametersScope>(
                this, closureExpr, captureList);
  scopeCreator.constructExpandAndInsertUncheckable<ClosureBodyScope>(
      bodyParent, closureExpr, captureList);
}

void CaptureListScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  // Patterns here are implicit, so need to dig out the intializers
  for (const CaptureListEntry &captureListEntry : expr->getCaptureList()) {
    for (unsigned patternEntryIndex = 0;
         patternEntryIndex < captureListEntry.Init->getNumPatternEntries();
         ++patternEntryIndex) {
      Expr *init = captureListEntry.Init->getInit(patternEntryIndex);
      scopeCreator.addExprToScopeTree(init, this);
    }
  }
}

void ClosureBodyScope::expandAScopeThatDoesNotCreateANewInsertionPoint(
    ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(closureExpr->getBody(), this);
}

void DefaultArgumentInitializerScope::
    expandAScopeThatDoesNotCreateANewInsertionPoint(
        ScopeCreator &scopeCreator) {
  auto *initExpr = decl->getStructuralDefaultExpr();
  ast_scope_assert(initExpr,
                 "Default argument initializer must have an initializer.");
  scopeCreator.addToScopeTree(initExpr, this);
}

void AttachedPropertyWrapperScope::
    expandAScopeThatDoesNotCreateANewInsertionPoint(
        ScopeCreator &scopeCreator) {
  for (auto *attr : decl->getAttrs().getAttributes<CustomAttr>()) {
    if (auto *expr = attr->getArg())
      scopeCreator.addToScopeTree(expr, this);
  }
}

#pragma mark expandScope

AstScopeImpl *GenericTypeOrExtensionWholePortion::expandScope(
    GenericTypeOrExtensionScope *scope, ScopeCreator &scopeCreator) const {
  // Get now in case recursion emancipates scope
  auto *const ip = scope->getParent().get();

  // Prevent circular request bugs caused by illegal input and
  // doing lookups that getExtendedNominal in the midst of getExtendedNominal.
  if (scope->shouldHaveABody() && !scope->doesDeclHaveABody())
    return ip;

  auto *deepestScope = scopeCreator.addNestedGenericParamScopesToTree(
      scope->getDecl(), scope->getGenericContext()->getGenericParams(), scope);
  if (scope->getGenericContext()->getTrailingWhereClause())
    scope->createTrailingWhereClauseScope(deepestScope, scopeCreator);
  scope->createBodyScope(deepestScope, scopeCreator);
  return ip;
}

AstScopeImpl *
IterableTypeBodyPortion::expandScope(GenericTypeOrExtensionScope *scope,
                                     ScopeCreator &scopeCreator) const {
  // Get it now in case of recursion and this one gets emancipated
  auto *const ip = scope->getParent().get();
  scope->expandBody(scopeCreator);
  return ip;
}

AstScopeImpl *GenericTypeOrExtensionWherePortion::expandScope(
    GenericTypeOrExtensionScope *scope, ScopeCreator &) const {
  return scope->getParent().get();
}

#pragma mark createBodyScope

void IterableTypeScope::countBodies(ScopeCreator &scopeCreator) const {
  if (auto *s = scopeCreator.getAstContext().Stats)
    ++s->getFrontendCounters().NumIterableTypeBodyAstScopes;
}

void ExtensionScope::createBodyScope(AstScopeImpl *leaf,
                                     ScopeCreator &scopeCreator) {
  scopeCreator.constructWithPortionExpandAndInsert<ExtensionScope,
                                                   IterableTypeBodyPortion>(
      leaf, decl);
  countBodies(scopeCreator);
}
void NominalTypeScope::createBodyScope(AstScopeImpl *leaf,
                                       ScopeCreator &scopeCreator) {
  scopeCreator.constructWithPortionExpandAndInsert<NominalTypeScope,
                                                   IterableTypeBodyPortion>(
      leaf, decl);
  countBodies(scopeCreator);
}

#pragma mark createTrailingWhereClauseScope

AstScopeImpl *GenericTypeOrExtensionScope::createTrailingWhereClauseScope(
    AstScopeImpl *parent, ScopeCreator &scopeCreator) {
  return parent;
}

AstScopeImpl *
ExtensionScope::createTrailingWhereClauseScope(AstScopeImpl *parent,
                                               ScopeCreator &scopeCreator) {
  return scopeCreator.constructWithPortionExpandAndInsert<
      ExtensionScope, GenericTypeOrExtensionWherePortion>(parent, decl);
}
AstScopeImpl *
NominalTypeScope::createTrailingWhereClauseScope(AstScopeImpl *parent,
                                                 ScopeCreator &scopeCreator) {
  return scopeCreator.constructWithPortionExpandAndInsert<
      NominalTypeScope, GenericTypeOrExtensionWherePortion>(parent, decl);
}
AstScopeImpl *
TypeAliasScope::createTrailingWhereClauseScope(AstScopeImpl *parent,
                                               ScopeCreator &scopeCreator) {
  return scopeCreator.constructWithPortionExpandAndInsert<
      TypeAliasScope, GenericTypeOrExtensionWherePortion>(parent, decl);
}

#pragma mark misc

AstScopeImpl *LabeledConditionalStmtScope::createNestedConditionalClauseScopes(
    ScopeCreator &scopeCreator, const Stmt *const afterConds) {
  auto *stmt = getLabeledConditionalStmt();
  AstScopeImpl *insertionPoint = this;
  for (unsigned i = 0; i < stmt->getCond().size(); ++i) {
    insertionPoint =
        scopeCreator
            .constructExpandAndInsertUncheckable<ConditionalClauseScope>(
                insertionPoint, stmt, i, afterConds->getStartLoc());
  }
  return insertionPoint;
}

AbstractPatternEntryScope::AbstractPatternEntryScope(
    PatternBindingDecl *declBeingScoped, unsigned entryIndex,
    DeclVisibilityKind vis)
    : decl(declBeingScoped), patternEntryIndex(entryIndex), vis(vis) {
  ast_scope_assert(entryIndex < declBeingScoped->getPatternList().size(),
                 "out of bounds");
}

void AbstractPatternEntryScope::forEachVarDeclWithLocalizableAccessors(
    ScopeCreator &scopeCreator, function_ref<void(VarDecl *)> foundOne) const {
  getPatternEntry().getPattern()->forEachVariable([&](VarDecl *var) {
    if (llvm::any_of(var->getAllAccessors(),
                     [&](AccessorDecl *a) { return isLocalizable(a); }))
      foundOne(var);
  });
}

bool AbstractPatternEntryScope::isLastEntry() const {
  return patternEntryIndex + 1 == decl->getPatternList().size();
}

// Following must be after uses to ensure templates get instantiated
#pragma mark getEnclosingAbstractStorageDecl

NullablePtr<AbstractStorageDecl>
AstScopeImpl::getEnclosingAbstractStorageDecl() const {
  return nullptr;
}

NullablePtr<AbstractStorageDecl>
SpecializeAttributeScope::getEnclosingAbstractStorageDecl() const {
  return getParent().get()->getEnclosingAbstractStorageDecl();
}
NullablePtr<AbstractStorageDecl>
AbstractFunctionDeclScope::getEnclosingAbstractStorageDecl() const {
  return getParent().get()->getEnclosingAbstractStorageDecl();
}
NullablePtr<AbstractStorageDecl>
ParameterListScope::getEnclosingAbstractStorageDecl() const {
  return getParent().get()->getEnclosingAbstractStorageDecl();
}
NullablePtr<AbstractStorageDecl>
GenericParamScope::getEnclosingAbstractStorageDecl() const {
  return getParent().get()->getEnclosingAbstractStorageDecl();
}

bool AstScopeImpl::isATypeDeclScope() const {
  Decl *const pd = getDeclIfAny().getPtrOrNull();
  return pd && (isa<NominalTypeDecl>(pd) || isa<ExtensionDecl>(pd));
}

void ScopeCreator::forEachClosureIn(
    Expr *expr, function_ref<void(NullablePtr<CaptureListExpr>, ClosureExpr *)>
                    foundClosure) {
  ast_scope_assert(expr,
                 "If looking for closures, must have an expression to search.");

  /// Ast walker that finds top-level closures in an expression.
  class ClosureFinder : public AstWalker {
    function_ref<void(NullablePtr<CaptureListExpr>, ClosureExpr *)>
        foundClosure;

  public:
    ClosureFinder(
        function_ref<void(NullablePtr<CaptureListExpr>, ClosureExpr *)>
            foundClosure)
        : foundClosure(foundClosure) {}

    std::pair<bool, Expr *> walkToExprPre(Expr *E) override {
      if (auto *closure = dyn_cast<ClosureExpr>(E)) {
        foundClosure(nullptr, closure);
        return {false, E};
      }
      if (auto *capture = dyn_cast<CaptureListExpr>(E)) {
        foundClosure(capture, capture->getClosureBody());
        return {false, E};
      }
      return {true, E};
    }
    std::pair<bool, Stmt *> walkToStmtPre(Stmt *S) override {
      if (auto *bs = dyn_cast<BraceStmt>(S)) { // closures hidden in here
        return {true, S};
      }
      return {false, S};
    }
    std::pair<bool, Pattern *> walkToPatternPre(Pattern *P) override {
      return {false, P};
    }
    bool walkToDeclPre(Decl *D) override { return false; }
    bool walkToTypeLocPre(TypeLoc &TL) override { return false; }
    bool walkToTypeReprPre(TypeRepr *T) override { return false; }
    bool walkToParameterListPre(ParameterList *PL) override { return false; }
  };

  expr->walk(ClosureFinder(foundClosure));
}

#pragma mark new operators
void *AstScopeImpl::operator new(size_t bytes, const AstContext &ctx,
                                 unsigned alignment) {
  return ctx.Allocate(bytes, alignment);
}

void *Portion::operator new(size_t bytes, const AstContext &ctx,
                             unsigned alignment) {
  return ctx.Allocate(bytes, alignment);
}
void *AstScope::operator new(size_t bytes, const AstContext &ctx,
                             unsigned alignment) {
  return ctx.Allocate(bytes, alignment);
}
void *ScopeCreator::operator new(size_t bytes, const AstContext &ctx,
                                 unsigned alignment) {
  return ctx.Allocate(bytes, alignment);
}

#pragma mark - expandBody

void AbstractFunctionBodyScope::expandBody(ScopeCreator &scopeCreator) {
  scopeCreator.addToScopeTree(decl->getBody(), this);
}

void GenericTypeOrExtensionScope::expandBody(ScopeCreator &) {}

void IterableTypeScope::expandBody(ScopeCreator &scopeCreator) {
  auto nodes = asNodeVector(getIterableDeclContext().get()->getMembers());
  scopeCreator.addSiblingsToScopeTree(this, this, nodes);
  if (auto *s = scopeCreator.getAstContext().Stats)
    ++s->getFrontendCounters().NumIterableTypeBodyAstScopeExpansions;
}

#pragma mark getScopeCreator
ScopeCreator &AstScopeImpl::getScopeCreator() {
  return getParent().get()->getScopeCreator();
}

ScopeCreator &AstSourceFileScope::getScopeCreator() { return *scopeCreator; }

#pragma mark getReferrent

  // These are the scopes whose AstNodes (etc) might be duplicated in the Ast
  // getReferrent is the cookie used to dedup them

#define GET_REFERRENT(Scope, x)                                                \
  NullablePtr<const void> Scope::getReferrent() const {                        \
    return UniquePointerCalculator().visit(x);                                 \
  }

GET_REFERRENT(AbstractFunctionDeclScope, getDecl())
// If the PatternBindingDecl is a dup, detect it for the first
// PatternEntryDeclScope; the others are subscopes.
GET_REFERRENT(PatternEntryDeclScope, getPattern())
GET_REFERRENT(TopLevelCodeScope, getDecl())
GET_REFERRENT(SubscriptDeclScope, getDecl())
GET_REFERRENT(VarDeclScope, getDecl())
GET_REFERRENT(GenericParamScope, paramList->getParams()[index])
GET_REFERRENT(AbstractStmtScope, getStmt())
GET_REFERRENT(CaptureListScope, getExpr())
GET_REFERRENT(WholeClosureScope, getExpr())
GET_REFERRENT(SpecializeAttributeScope, specializeAttr)
GET_REFERRENT(GenericTypeOrExtensionScope, portion->getReferrentOfScope(this))

const Decl *
Portion::getReferrentOfScope(const GenericTypeOrExtensionScope *s) const {
  return nullptr;
}

const Decl *GenericTypeOrExtensionWholePortion::getReferrentOfScope(
    const GenericTypeOrExtensionScope *s) const {
  return s->getDecl();
}

#undef GET_REFERRENT

#pragma mark currency
NullablePtr<AstScopeImpl> AstScopeImpl::insertionPointForDeferredExpansion() {
  return nullptr;
}

NullablePtr<AstScopeImpl>
AbstractFunctionBodyScope::insertionPointForDeferredExpansion() {
  return getParent().get();
}

NullablePtr<AstScopeImpl>
IterableTypeScope::insertionPointForDeferredExpansion() {
  return portion->insertionPointForDeferredExpansion(this);
}

NullablePtr<AstScopeImpl>
GenericTypeOrExtensionWholePortion::insertionPointForDeferredExpansion(
    IterableTypeScope *s) const {
  return s->getParent().get();
}
NullablePtr<AstScopeImpl>
GenericTypeOrExtensionWherePortion::insertionPointForDeferredExpansion(
    IterableTypeScope *) const {
  return nullptr;
}
NullablePtr<AstScopeImpl>
IterableTypeBodyPortion::insertionPointForDeferredExpansion(
    IterableTypeScope *s) const {
  return s->getParent().get();
}

bool AstScopeImpl::isExpansionNeeded(const ScopeCreator &scopeCreator) const {
  return !isCurrent() ||
         scopeCreator.getAstContext().LangOpts.StressAstScopeLookup;
}

bool AstScopeImpl::isCurrent() const {
  return getWasExpanded() && isCurrentIfWasExpanded();
}

void AstScopeImpl::beCurrent() {}
bool AstScopeImpl::isCurrentIfWasExpanded() const { return true; }

void AstSourceFileScope::beCurrent() {
  numberOfDeclsAlreadySeen = SF->Decls.size();
}
bool AstSourceFileScope::isCurrentIfWasExpanded() const {
  return SF->Decls.size() == numberOfDeclsAlreadySeen;
}

void IterableTypeScope::beCurrent() { portion->beCurrent(this); }
bool IterableTypeScope::isCurrentIfWasExpanded() const {
  return portion->isCurrentIfWasExpanded(this);
}

void GenericTypeOrExtensionWholePortion::beCurrent(IterableTypeScope *s) const {
  s->makeWholeCurrent();
}
bool GenericTypeOrExtensionWholePortion::isCurrentIfWasExpanded(
    const IterableTypeScope *s) const {
  return s->isWholeCurrent();
}
void GenericTypeOrExtensionWherePortion::beCurrent(IterableTypeScope *) const {}
bool GenericTypeOrExtensionWherePortion::isCurrentIfWasExpanded(
    const IterableTypeScope *) const {
  return true;
}
void IterableTypeBodyPortion::beCurrent(IterableTypeScope *s) const {
  s->makeBodyCurrent();
}
bool IterableTypeBodyPortion::isCurrentIfWasExpanded(
    const IterableTypeScope *s) const {
  return s->isBodyCurrent();
}

void IterableTypeScope::makeWholeCurrent() {
  ast_scope_assert(getWasExpanded(), "Should have been expanded");
}
bool IterableTypeScope::isWholeCurrent() const {
  // Whole starts out unexpanded, and is lazily built but will have at least a
  // body scope child
  return getWasExpanded();
}
void IterableTypeScope::makeBodyCurrent() {
  memberCount = getIterableDeclContext().get()->getMemberCount();
}
bool IterableTypeScope::isBodyCurrent() const {
  return memberCount == getIterableDeclContext().get()->getMemberCount();
}

void AbstractFunctionBodyScope::beCurrent() {
  bodyWhenLastExpanded = decl->getBody(false);
}
bool AbstractFunctionBodyScope::isCurrentIfWasExpanded() const {
  // Pass in false to keep the compiler from synthesizing one.
  return bodyWhenLastExpanded == decl->getBody(false);
}

void TopLevelCodeScope::beCurrent() { bodyWhenLastExpanded = decl->getBody(); }
bool TopLevelCodeScope::isCurrentIfWasExpanded() const {
  return bodyWhenLastExpanded == decl->getBody();
}

// Try to avoid the work of counting
static const bool assumeVarsDoNotGetAdded = true;

void PatternEntryDeclScope::beCurrent() {
  initWhenLastExpanded = getPatternEntry().getOriginalInit();
  if (assumeVarsDoNotGetAdded && varCountWhenLastExpanded)
    return;
  varCountWhenLastExpanded = getPatternEntry().getNumBoundVariables();
}
bool PatternEntryDeclScope::isCurrentIfWasExpanded() const {
  if (initWhenLastExpanded != getPatternEntry().getOriginalInit())
    return false;
  if (assumeVarsDoNotGetAdded && varCountWhenLastExpanded) {
    ast_scope_assert(varCountWhenLastExpanded ==
                       getPatternEntry().getNumBoundVariables(),
                   "Vars were not supposed to be added to a pattern entry.");
    return true;
  }
  return getPatternEntry().getNumBoundVariables() == varCountWhenLastExpanded;
}

void WholeClosureScope::beCurrent() {
  bodyWhenLastExpanded = closureExpr->getBody();
}
bool WholeClosureScope::isCurrentIfWasExpanded() const {
  return bodyWhenLastExpanded == closureExpr->getBody();
}

#pragma mark getParentOfAstAncestorScopesToBeRescued
NullablePtr<AstScopeImpl>
AstScopeImpl::getParentOfAstAncestorScopesToBeRescued() {
  return this;
}
NullablePtr<AstScopeImpl>
AbstractFunctionBodyScope::getParentOfAstAncestorScopesToBeRescued() {
  // Reexpansion always creates a new body as the first child
  // That body contains the scopes to be rescued.
  return getChildren().empty() ? nullptr : getChildren().front();
}
NullablePtr<AstScopeImpl>
TopLevelCodeScope::getParentOfAstAncestorScopesToBeRescued() {
  // Reexpansion always creates a new body as the first child
  // That body contains the scopes to be rescued.
  return getChildren().empty() ? nullptr : getChildren().front();
}

#pragma mark rescuing & reusing
std::vector<AstScopeImpl *>
AstScopeImpl::rescueAstAncestorScopesForReuseFromMeOrDescendants() {
  if (auto *p = getParentOfAstAncestorScopesToBeRescued().getPtrOrNull()) {
    return p->rescueAstAncestorScopesForReuseFromMe();
  }
  ast_scope_assert(
      getAstAncestorScopeCount() == 0,
      "If receives AstAncestor scopes, must know where to find parent");
  return {};
}

void AstScopeImpl::replaceAstAncestorScopes(
    ArrayRef<AstScopeImpl *> scopesToAdd) {
  auto *p = getParentOfAstAncestorScopesToBeRescued().getPtrOrNull();
  if (!p) {
    ast_scope_assert(scopesToAdd.empty(), "Non-empty body disappeared?!");
    return;
  }
  auto &ctx = getAstContext();
  for (auto *s : scopesToAdd) {
    p->addChild(s, ctx);
    ast_scope_assert(s->verifyThatThisNodeComeAfterItsPriorSibling(),
                   "Ensure search will work");
  }
  p->increaseAstAncestorScopeCount(scopesToAdd.size());
}

std::vector<AstScopeImpl *>
AstScopeImpl::rescueAstAncestorScopesForReuseFromMe() {
  std::vector<AstScopeImpl *> astAncestorScopes;
  for (unsigned i = getChildren().size() - getAstAncestorScopeCount();
       i < getChildren().size(); ++i)
    astAncestorScopes.push_back(getChildren()[i]);
  // So they don't get disowned and children cleared.
  for (unsigned i = 0; i < getAstAncestorScopeCount(); ++i) {
    storedChildren.back()->emancipate();
    storedChildren.pop_back();
  }
  resetAstAncestorScopeCount();
  return astAncestorScopes;
}

bool AbstractFunctionDeclScope::shouldCreateAccessorScope(
    const AccessorDecl *const ad) {
  return isLocalizable(ad);
}

#pragma mark verification

namespace {
class LocalizableDeclContextCollector : public AstWalker {

public:
  llvm::DenseMap<const DeclContext *, unsigned> declContexts;

  void record(const DeclContext *dc) {
    if (dc)
      declContexts.insert({dc, 0});
  }

  bool walkToDeclPre(Decl *D) override {
    //    catchForDebugging(D, "DictionaryBridging.swift", 694);
    if (const auto *dc = dyn_cast<DeclContext>(D))
      record(dc);
    if (auto *icd = dyn_cast<IfConfigDecl>(D)) {
      walkToClauses(icd);
      return false;
    }
    if (auto *pd = dyn_cast<ParamDecl>(D))
      record(pd->getDefaultArgumentInitContext());
    else if (auto *pbd = dyn_cast<PatternBindingDecl>(D))
      recordInitializers(pbd);
    else if (auto *vd = dyn_cast<VarDecl>(D))
      for (auto *ad : vd->getAllAccessors())
        ad->walk(*this);
    return AstWalker::walkToDeclPre(D);
  }

  std::pair<bool, Expr *> walkToExprPre(Expr *E) override {
    if (const auto *ce = dyn_cast<ClosureExpr>(E))
      record(ce);
    return AstWalker::walkToExprPre(E);
  }

private:
  void walkToClauses(IfConfigDecl *icd) {
    for (auto &clause : icd->getClauses()) {
      // Generate scopes for any closures in the condition
      if (ScopeCreator::includeInactiveIfConfigClauses && clause.isActive) {
        if (clause.Cond)
          clause.Cond->walk(*this);
        for (auto n : clause.Elements)
          n.walk(*this);
      }
    }
  }

  void recordInitializers(PatternBindingDecl *pbd) {
    for (auto idx : range(pbd->getNumPatternEntries()))
      record(pbd->getInitContext(idx));
  }

  void catchForDebugging(Decl *D, const char *file, const unsigned line) {
    auto &SM = D->getAstContext().SourceMgr;
    auto loc = D->getStartLoc();
    if (!loc.isValid())
      return;
    auto bufID = SM.findBufferContainingLoc(loc);
    auto f = SM.getIdentifierForBuffer(bufID);
    auto lin = SM.getLineNumber(loc);
    if (f.endswith(file) && lin == line)
      if (auto *v = dyn_cast<PatternBindingDecl>(D))
        llvm::errs() << "*** catchForDebugging: " << lin << " ***\n";
  }
};
} // end namespace

llvm::DenseMap<const DeclContext *, unsigned>
ScopeCreator::findLocalizableDeclContextsInAst() const {
  LocalizableDeclContextCollector collector;
  sourceFileScope->SF->walk(collector);
  // Walker omits the top
  collector.record(sourceFileScope->SF);
  return collector.declContexts;
}

bool AstSourceFileScope::crossCheckWithAst() {
  return scopeCreator->containsAllDeclContextsFromAst();
}

void ast_scope::simple_display(llvm::raw_ostream &out,
                               const ScopeCreator *scopeCreator) {
  scopeCreator->print(out);
}

//----------------------------------------------------------------------------//
// ExpandAstScopeRequest computation.
//----------------------------------------------------------------------------//

bool ExpandAstScopeRequest::isCached() const {
  AstScopeImpl *scope = std::get<0>(getStorage());
  ScopeCreator *scopeCreator = std::get<1>(getStorage());
  return !scope->isExpansionNeeded(*scopeCreator);
}

Optional<AstScopeImpl *> ExpandAstScopeRequest::getCachedResult() const {
  return std::get<0>(getStorage());
}
