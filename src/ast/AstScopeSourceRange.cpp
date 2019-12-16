//===--- AstScopeSourceRange.cpp - Swift Object-Oriented Ast Scope --------===//
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
/// This file implements the source range queries for the AstScopeImpl ontology.
///
//===----------------------------------------------------------------------===//
#include "polarphp/ast/AstScope.h"

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Initializer.h"
#include "polarphp/ast/LazyResolver.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeRepr.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/parser/Lexer.h"
#include "llvm/Support/Compiler.h"
#include <algorithm>

using namespace polar;
using namespace ast_scope;

static SourceLoc getStartOfFirstParam(ClosureExpr *closure);
static SourceLoc getLocEncompassingPotentialLookups(const SourceManager &,
                                                    SourceLoc endLoc);
static SourceLoc getLocAfterExtendedNominal(const ExtensionDecl *);

SourceRange AstScopeImpl::widenSourceRangeForIgnoredAstNodes(
    const SourceRange range) const {
  if (range.isInvalid())
    return sourceRangeOfIgnoredAstNodes;
  auto r = range;
  if (sourceRangeOfIgnoredAstNodes.isValid())
    r.widen(sourceRangeOfIgnoredAstNodes);
  return r;
}

SourceRange
AstScopeImpl::widenSourceRangeForChildren(const SourceRange range,
                                          const bool omitAssertions) const {
  if (getChildren().empty()) {
    ast_scope_assert(omitAssertions || range.start.isValid(), "Bad range.");
    return range;
  }
  const auto childStart =
      getChildren().front()->getSourceRangeOfScope(omitAssertions).start;
  const auto childEnd =
      getChildren().back()->getSourceRangeOfScope(omitAssertions).end;
  auto childRange = SourceRange(childStart, childEnd);
  ast_scope_assert(omitAssertions || childRange.isValid(), "Bad range.");

  if (range.isInvalid())
    return childRange;
  auto r = range;
  r.widen(childRange);
  return r;
}

bool AstScopeImpl::checkSourceRangeAfterExpansion(const AstContext &ctx) const {
  ast_scope_assert(getSourceRangeOfThisAstNode().isValid() ||
                     !getChildren().empty(),
                 "need to be able to find source range");
  ast_scope_assert(verifyThatChildrenAreContainedWithin(getSourceRangeOfScope()),
                 "Search will fail");
  ast_scope_assert(
      checkLazySourceRange(ctx),
      "Lazy scopes must have compatible ranges before and after expansion");

  return true;
}

#pragma mark validation & debugging

bool AstScopeImpl::hasValidSourceRange() const {
  const auto sourceRange = getSourceRangeOfScope();
  return sourceRange.start.isValid() && sourceRange.end.isValid() &&
         !getSourceManager().isBeforeInBuffer(sourceRange.end,
                                              sourceRange.start);
}

bool AstScopeImpl::hasValidSourceRangeOfIgnoredAstNodes() const {
  return sourceRangeOfIgnoredAstNodes.isValid();
}

bool AstScopeImpl::precedesInSource(const AstScopeImpl *next) const {
  if (!hasValidSourceRange() || !next->hasValidSourceRange())
    return false;
  return !getSourceManager().isBeforeInBuffer(
      next->getSourceRangeOfScope().start, getSourceRangeOfScope().end);
}

bool AstScopeImpl::verifyThatChildrenAreContainedWithin(
    const SourceRange range) const {
  // assumes children are already in order
  if (getChildren().empty())
    return true;
  const SourceRange rangeOfChildren =
      SourceRange(getChildren().front()->getSourceRangeOfScope().start,
                  getChildren().back()->getSourceRangeOfScope().end);
  if (getSourceManager().rangeContains(range, rangeOfChildren))
    return true;
  auto &out = verificationError() << "children not contained in its parent\n";
  if (getChildren().size() == 1) {
    out << "\n***Only Child node***\n";
    getChildren().front()->print(out);
  } else {
    out << "\n***First Child node***\n";
    getChildren().front()->print(out);
    out << "\n***Last Child node***\n";
    getChildren().back()->print(out);
  }
  out << "\n***Parent node***\n";
  this->print(out);
  abort();
}

bool AstScopeImpl::verifyThatThisNodeComeAfterItsPriorSibling() const {
  auto priorSibling = getPriorSibling();
  if (!priorSibling)
    return true;
  if (priorSibling.get()->precedesInSource(this))
    return true;
  auto &out = verificationError() << "unexpected out-of-order nodes\n";
  out << "\n***Penultimate child node***\n";
  priorSibling.get()->print(out);
  out << "\n***Last Child node***\n";
  print(out);
  out << "\n***Parent node***\n";
  getParent().get()->print(out);
  //  llvm::errs() << "\n\nsource:\n"
  //               << getSourceManager()
  //                      .getRangeForBuffer(
  //                          getSourceFile()->getBufferID().getValue())
  //                      .str();
  ast_scope_unreachable("unexpected out-of-order nodes");
  return false;
}

NullablePtr<AstScopeImpl> AstScopeImpl::getPriorSibling() const {
  auto parent = getParent();
  if (!parent)
    return nullptr;
  auto const &siblingsAndMe = parent.get()->getChildren();
  // find myIndex, which is probably the last one
  int myIndex = -1;
  for (int i = siblingsAndMe.size() - 1; i >= 0; --i) {
    if (siblingsAndMe[i] == this) {
      myIndex = i;
      break;
    }
  }
  ast_scope_assert(myIndex != -1, "I have been disowned!");
  if (myIndex == 0)
    return nullptr;
  return siblingsAndMe[myIndex - 1];
}

bool AstScopeImpl::doesRangeMatch(unsigned start, unsigned end, StringRef file,
                                  StringRef className) {
  if (!className.empty() && className != getClassName())
    return false;
  const auto &SM = getSourceManager();
  const auto r = getSourceRangeOfScope(true);
  if (start && start != SM.getLineNumber(r.start))
    return false;
  if (end && end != SM.getLineNumber(r.end))
    return false;
  if (file.empty())
    return true;
  const auto buf = SM.findBufferContainingLoc(r.start);
  return SM.getIdentifierForBuffer(buf).endswith(file);
}

#pragma mark getSourceRangeOfThisAstNode

SourceRange SpecializeAttributeScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return specializeAttr->getRange();
}

SourceRange AbstractFunctionBodyScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return decl->getBodySourceRange();
}

SourceRange TopLevelCodeScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return decl->getSourceRange();
}

SourceRange SubscriptDeclScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return decl->getSourceRange();
}

SourceRange
EnumElementScope::getSourceRangeOfThisAstNode(const bool omitAssertions) const {
  return decl->getSourceRange();
}

SourceRange WholeClosureScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return closureExpr->getSourceRange();
}

SourceRange AbstractStmtScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return getStmt()->getSourceRange();
}

SourceRange DefaultArgumentInitializerScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  if (auto *dv = decl->getStructuralDefaultExpr())
    return dv->getSourceRange();
  return SourceRange();
}

SourceRange PatternEntryDeclScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  // TODO: Once the creation of two PatternBindingDecls at same location is
  // eliminated, the following may be able to be simplified.
  if (!getChildren().empty()) { // why needed???
    bool hasOne = false;
    getPattern()->forEachVariable([&](VarDecl *) { hasOne = true; });
    if (!hasOne)
      return SourceRange(); // just the init
    if (!getPatternEntry().getInit())
      return SourceRange(); // just the var decls
  }
  return getPatternEntry().getSourceRange();
}

SourceRange PatternEntryInitializerScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  // TODO: Don't remove the initializer in the rest of the compiler:
  // Search for "When the initializer is removed we don't actually clear the
  // pointer" because we do!
  return initAsWrittenWhenCreated->getSourceRange();
}

SourceRange
VarDeclScope::getSourceRangeOfThisAstNode(const bool omitAssertions) const {
  const auto br = decl->getBracesRange();
  return br.isValid() ? br : decl->getSourceRange();
}

SourceRange GenericParamScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  auto nOrE = holder;
  // A protocol's generic parameter list is not written in source, and
  // is visible from the start of the body.
  if (auto *protoDecl = dyn_cast<InterfaceDecl>(nOrE))
    return SourceRange(protoDecl->getBraces().start, protoDecl->getEndLoc());
  const auto startLoc = paramList->getSourceRange().start;
  const auto validStartLoc =
      startLoc.isValid() ? startLoc : holder->getStartLoc();
  // Since ExtensionScope (whole portion) range doesn't start till after the
  // extended nominal, the range here must be pushed back, too.
  if (auto const *const ext = dyn_cast<ExtensionDecl>(holder)) {
    return SourceRange(getLocAfterExtendedNominal(ext), ext->getEndLoc());
  }
  return SourceRange(validStartLoc, holder->getEndLoc());
}

SourceRange AstSourceFileScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  if (auto bufferID = SF->getBufferID()) {
    auto charRange = getSourceManager().getRangeForBuffer(*bufferID);
    return SourceRange(charRange.getStart(), charRange.getEnd());
  }

  if (SF->Decls.empty())
    return SourceRange();

  // Use the source ranges of the declarations in the file.
  return SourceRange(SF->Decls.front()->getStartLoc(),
                     SF->Decls.back()->getEndLoc());
}

SourceRange GenericTypeOrExtensionScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return portion->getChildlessSourceRangeOf(this, omitAssertions);
}

SourceRange GenericTypeOrExtensionWholePortion::getChildlessSourceRangeOf(
    const GenericTypeOrExtensionScope *scope, const bool omitAssertions) const {
  auto *d = scope->getDecl();
  auto r = d->getSourceRangeIncludingAttrs();
  if (r.start.isValid()) {
    ast_scope_assert(r.end.isValid(), "Start valid imples end valid.");
    return scope->moveStartPastExtendedNominal(r);
  }
  return d->getSourceRange();
}

SourceRange
ExtensionScope::moveStartPastExtendedNominal(const SourceRange sr) const {
  const auto afterExtendedNominal = getLocAfterExtendedNominal(decl);
  // Illegal code can have an endLoc that is before the end of the
  // ExtendedNominal
  if (getSourceManager().isBeforeInBuffer(sr.end, afterExtendedNominal)) {
    // Must not have the source range returned include the extended nominal
    return SourceRange(sr.start, sr.start);
  }
  return SourceRange(afterExtendedNominal, sr.end);
}

SourceRange
GenericTypeScope::moveStartPastExtendedNominal(const SourceRange sr) const {
  // There is no extended nominal
  return sr;
}

SourceRange GenericTypeOrExtensionWherePortion::getChildlessSourceRangeOf(
    const GenericTypeOrExtensionScope *scope, const bool omitAssertions) const {
  return scope->getGenericContext()->getTrailingWhereClause()->getSourceRange();
}

SourceRange IterableTypeBodyPortion::getChildlessSourceRangeOf(
    const GenericTypeOrExtensionScope *scope, const bool omitAssertions) const {
  auto *d = scope->getDecl();
  if (auto *nt = dyn_cast<NominalTypeDecl>(d))
    return nt->getBraces();
  if (auto *e = dyn_cast<ExtensionDecl>(d))
    return e->getBraces();
  if (omitAssertions)
    return SourceRange();
  ast_scope_unreachable("No body!");
}

SourceRange AbstractFunctionDeclScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  // For a get/put accessor	 all of the parameters are implicit, so start
  // them at the start location of the accessor.
  auto r = decl->getSourceRangeIncludingAttrs();
  if (r.start.isValid()) {
    ast_scope_assert(r.end.isValid(), "Start valid imples end valid.");
    return r;
  }
  return decl->getBodySourceRange();
}

SourceRange ParameterListScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  const auto rangeForGoodInput =
      getSourceRangeOfEnclosedParamsOfAstNode(omitAssertions);
  auto r = SourceRange(rangeForGoodInput.start,
                       fixupEndForBadInput(rangeForGoodInput));
  ast_scope_assert(getSourceManager().rangeContains(
                     getParent().get()->getSourceRangeOfThisAstNode(true), r),
                 "Parameters not within function?!");
  return r;
}

SourceLoc ParameterListScope::fixupEndForBadInput(
    const SourceRange rangeForGoodInput) const {
  const auto s = rangeForGoodInput.start;
  const auto e = rangeForGoodInput.end;
  return getSourceManager().isBeforeInBuffer(s, e) ? e : s;
}

SourceRange ForEachPatternScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  // The scope of the pattern extends from the 'where' expression (if present)
  // until the end of the body.
  if (stmt->getWhere())
    return SourceRange(stmt->getWhere()->getStartLoc(),
                       stmt->getBody()->getEndLoc());

  // Otherwise, scope of the pattern covers the body.
  return stmt->getBody()->getSourceRange();
}

SourceRange
CatchStmtScope::getSourceRangeOfThisAstNode(const bool omitAssertions) const {
  // The scope of the pattern extends from the 'where' (if present)
  // to the end of the body.
  if (stmt->getGuardExpr())
    return SourceRange(stmt->getWhereLoc(), stmt->getBody()->getEndLoc());

  // Otherwise, the scope of the pattern encompasses the body.
  return stmt->getBody()->getSourceRange();
}
SourceRange
CaseStmtScope::getSourceRangeOfThisAstNode(const bool omitAssertions) const {
  // The scope of the case statement begins at the first guard expression,
  // if there is one, and extends to the end of the body.
  // FIXME: Figure out what to do about multiple pattern bindings. We might
  // want a more restrictive rule in those cases.
  for (const auto &caseItem : stmt->getCaseLabelItems()) {
    if (auto guardExpr = caseItem.getGuardExpr())
      return SourceRange(guardExpr->getStartLoc(),
                         stmt->getBody()->getEndLoc());
  }

  // Otherwise, it covers the body.
  return stmt->getBody()
      ->getSourceRange(); // The scope of the case statement begins
}

SourceRange
BraceStmtScope::getSourceRangeOfThisAstNode(const bool omitAssertions) const {
  // The brace statements that represent closures start their scope at the
  // 'in' keyword, when present.
  if (auto closure = parentClosureIfAny()) {
    if (closure.get()->getInLoc().isValid())
      return SourceRange(closure.get()->getInLoc(), stmt->getEndLoc());
  }
  return stmt->getSourceRange();
}

SourceRange ConditionalClauseScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  // From the start of this particular condition to the start of the
  // then/body part.
  const auto startLoc = getStmtConditionElement().getStartLoc();
  return startLoc.isValid()
         ? SourceRange(startLoc, endLoc)
         : SourceRange(endLoc);
}

SourceRange ConditionalClausePatternUseScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  // For a guard continuation, the scope extends from the end of the 'else'
  // to the end of the continuation.
  return SourceRange(startLoc);
}

SourceRange
CaptureListScope::getSourceRangeOfThisAstNode(const bool omitAssertions) const {
  auto *const closure = expr->getClosureBody();
  return SourceRange(expr->getStartLoc(), getStartOfFirstParam(closure));
}

SourceRange ClosureParametersScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  if (!omitAssertions)
    ast_scope_assert(closureExpr->getInLoc().isValid(),
                   "We don't create these if no in loc");
  return SourceRange(getStartOfFirstParam(closureExpr),
                     closureExpr->getInLoc());
}

SourceRange
ClosureBodyScope::getSourceRangeOfThisAstNode(const bool omitAssertions) const {
  if (closureExpr->getInLoc().isValid())
    return SourceRange(closureExpr->getInLoc(), closureExpr->getEndLoc());

  return closureExpr->getSourceRange();
}

SourceRange AttachedPropertyWrapperScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return sourceRangeWhenCreated;
}

SourceRange LookupParentDiversionScope::getSourceRangeOfThisAstNode(
    const bool omitAssertions) const {
  return SourceRange(startLoc);
}

#pragma mark source range caching

SourceRange
AstScopeImpl::getSourceRangeOfScope(const bool omitAssertions) const {
  if (!isSourceRangeCached(omitAssertions))
    computeAndCacheSourceRangeOfScope(omitAssertions);
  return *cachedSourceRange;
}

bool AstScopeImpl::isSourceRangeCached(const bool omitAssertions) const {
  const bool isCached = cachedSourceRange.hasValue();
  ast_scope_assert(omitAssertions || isCached ||
                     ensureNoAncestorsSourceRangeIsCached(),
                 "Cached ancestor's range likely is obsolete.");
  return isCached;
}

bool AstScopeImpl::ensureNoAncestorsSourceRangeIsCached() const {
  if (const auto *const p = getParent().getPtrOrNull()) {
    auto r = !p->isSourceRangeCached(true) &&
             p->ensureNoAncestorsSourceRangeIsCached();
    if (!r)
      ast_scope_unreachable("found a violation");
    return true;
  }
  return true;
}

void AstScopeImpl::computeAndCacheSourceRangeOfScope(
    const bool omitAssertions) const {
  // In order to satisfy the invariant that, if my range is uncached,
  // my parent's range is uncached, (which is needed to optimize invalidation
  // by obviating the need to uncache all the way to the root every time),
  // when caching a range, must ensure all children's ranges are cached.
  for (auto *c : getChildren())
    c->computeAndCacheSourceRangeOfScope(omitAssertions);

  cachedSourceRange = computeSourceRangeOfScope(omitAssertions);
}

bool AstScopeImpl::checkLazySourceRange(const AstContext &ctx) const {
  if (!ctx.LangOpts.LazyAstScopes)
    return true;
  const auto unexpandedRange = sourceRangeForDeferredExpansion();
  const auto expandedRange = computeSourceRangeOfScopeWithChildAstNodes();
  if (unexpandedRange.isInvalid() || expandedRange.isInvalid())
    return true;
  if (unexpandedRange == expandedRange)
    return true;

  llvm::errs() << "*** Lazy range problem. Parent unexpanded: ***\n";
  unexpandedRange.print(llvm::errs(), getSourceManager(), false);
  llvm::errs() << "\n";
  if (!getChildren().empty()) {
    llvm::errs() << "*** vs last child: ***\n";
    auto b = getChildren().back()->computeSourceRangeOfScope();
    b.print(llvm::errs(), getSourceManager(), false);
    llvm::errs() << "\n";
  }
  else if (hasValidSourceRangeOfIgnoredAstNodes()) {
    llvm::errs() << "*** vs ignored Ast nodes: ***\n";
    sourceRangeOfIgnoredAstNodes.print(llvm::errs(), getSourceManager(), false);
    llvm::errs() << "\n";
  }
  print(llvm::errs(), 0, false);
  llvm::errs() << "\n";

  return false;
}

SourceRange
AstScopeImpl::computeSourceRangeOfScope(const bool omitAssertions) const {
  // If we don't need to consider children, it's cheaper
  const auto deferredRange = sourceRangeForDeferredExpansion();
  return deferredRange.isValid()
             ? deferredRange
             : computeSourceRangeOfScopeWithChildAstNodes(omitAssertions);
}

SourceRange AstScopeImpl::computeSourceRangeOfScopeWithChildAstNodes(
    const bool omitAssertions) const {
  const auto rangeOfJustThisAstNode =
      getSourceRangeOfThisAstNode(omitAssertions);
  const auto rangeIncludingIgnoredNodes =
      widenSourceRangeForIgnoredAstNodes(rangeOfJustThisAstNode);
  const auto uncachedSourceRange =
      widenSourceRangeForChildren(rangeIncludingIgnoredNodes, omitAssertions);
  return uncachedSourceRange;
}

void AstScopeImpl::clearCachedSourceRangesOfMeAndAncestors() {
  // An optimization: if my range isn't cached, my ancestors must not be
  if (!isSourceRangeCached())
    return;
  cachedSourceRange = None;
  if (auto p = getParent())
    p.get()->clearCachedSourceRangesOfMeAndAncestors();
}

#pragma mark compensating for InterpolatedStringLiteralExprs and EditorPlaceHolders

static bool isInterpolatedStringLiteral(const Token& tok) {
/// @todo
//  SmallVector<Lexer::StringSegment, 1> Segments;
//  Lexer::getStringLiteralSegments(tok, Segments, nullptr);
//  return Segments.size() != 1 ||
//    Segments.front().Kind != Lexer::StringSegment::Literal;
}

/// If right brace is missing, the source range of the body will end
/// at the last token, which may be a one of the special cases below.
/// This work is only needed for *unexpanded* scopes because unioning the range
/// with the children will do the same thing for an expanded scope.
/// It is also needed for ignored \c AstNodes, which may be, e.g. \c
/// InterpolatedStringLiterals
static SourceLoc getLocEncompassingPotentialLookups(const SourceManager &SM,
                                                    const SourceLoc endLoc) {
                                                    /// @todo
//  const auto tok = Lexer::getTokenAtLocation(SM, endLoc);
//  switch (tok.getKind()) {
//  default:
//    return endLoc;
//  case tok::string_literal:
//    if (!isInterpolatedStringLiteral(tok))
//      return endLoc; // Just the start of the last token
//    break;
//  case tok::identifier:
//    // subtract one to get a closed-range endpoint from a half-open
//    if (!Identifier::isEditorPlaceholder(tok.getText()))
//      return endLoc;
//    break;
//  }
//  return tok.getRange().getEnd().getAdvancedLoc(-1);
}

SourceRange AstScopeImpl::sourceRangeForDeferredExpansion() const {
  return SourceRange();
}
SourceRange IterableTypeScope::sourceRangeForDeferredExpansion() const {
  return portion->sourceRangeForDeferredExpansion(this);
}
SourceRange AbstractFunctionBodyScope::sourceRangeForDeferredExpansion() const {
  const auto bsr = decl->getBodySourceRange();
  const SourceLoc endEvenIfNoCloseBraceAndEndsWithInterpolatedStringLiteral =
      getLocEncompassingPotentialLookups(getSourceManager(), bsr.end);
  return SourceRange(bsr.start,
                     endEvenIfNoCloseBraceAndEndsWithInterpolatedStringLiteral);
}

SourceRange GenericTypeOrExtensionWholePortion::sourceRangeForDeferredExpansion(
    const IterableTypeScope *s) const {
  const auto rangeOfThisNodeWithoutChildren =
      getChildlessSourceRangeOf(s, false);
  const auto rangeExtendedForFinalToken = SourceRange(
      rangeOfThisNodeWithoutChildren.start,
      getLocEncompassingPotentialLookups(s->getSourceManager(),
                                         rangeOfThisNodeWithoutChildren.end));
  const auto rangePastExtendedNominal =
      s->moveStartPastExtendedNominal(rangeExtendedForFinalToken);
  return rangePastExtendedNominal;
}

SourceRange GenericTypeOrExtensionWherePortion::sourceRangeForDeferredExpansion(
    const IterableTypeScope *) const {
  return SourceRange();
}

SourceRange IterableTypeBodyPortion::sourceRangeForDeferredExpansion(
    const IterableTypeScope *s) const {
  const auto bracesRange = getChildlessSourceRangeOf(s, false);
  return SourceRange(bracesRange.start,
                     getLocEncompassingPotentialLookups(s->getSourceManager(),
                                                        bracesRange.end));
}

SourceRange AstScopeImpl::getEffectiveSourceRange(const AstNode n) const {
  if (const auto *d = n.dyn_cast<Decl *>())
    return d->getSourceRange();
  if (const auto *s = n.dyn_cast<Stmt *>())
    return s->getSourceRange();
  auto *e = n.dyn_cast<Expr *>();
  return getLocEncompassingPotentialLookups(getSourceManager(), e->getEndLoc());
}

/// Some nodes (e.g. the error expression) cannot possibly contain anything to
/// be looked up and if included in a parent scope's source range would expand
/// it beyond an ancestor's source range. But if the ancestor is expanded
/// lazily, we check that its source range does not change when expanding it,
/// and this check would fail.
static bool sourceRangeWouldInterfereWithLaziness(const AstNode n) {
  return n.isExpr(ExprKind::Error);
}

static bool
shouldIgnoredAstNodeSourceRangeWidenEnclosingScope(const AstNode n) {
  if (n.isDecl(DeclKind::Var)) {
    // The pattern scopes will include the source ranges for VarDecls.
    // Using its range here would cause a pattern initializer scope's range
    // to overlap the pattern use scope's range.
    return false;
  }
  if (sourceRangeWouldInterfereWithLaziness(n))
    return false;
  return true;
}

void AstScopeImpl::widenSourceRangeForIgnoredAstNode(const AstNode n) {
  if (!shouldIgnoredAstNodeSourceRangeWidenEnclosingScope(n))
    return;

  // FIXME: why only do effectiveness bit for *ignored* nodes?
  SourceRange r = getEffectiveSourceRange(n);
  if (r.isInvalid())
    return;
  if (sourceRangeOfIgnoredAstNodes.isInvalid())
    sourceRangeOfIgnoredAstNodes = r;
  else
    sourceRangeOfIgnoredAstNodes.widen(r);
}

static SourceLoc getStartOfFirstParam(ClosureExpr *closure) {
  if (auto *parms = closure->getParameters()) {
    if (parms->size())
      return parms->get(0)->getStartLoc();
  }
  if (closure->getInLoc().isValid())
    return closure->getInLoc();
  if (closure->getBody())
    return closure->getBody()->getLBraceLoc();
  return closure->getStartLoc();
}

#pragma mark getSourceRangeOfEnclosedParamsOfAstNode

SourceRange AstScopeImpl::getSourceRangeOfEnclosedParamsOfAstNode(
    const bool omitAssertions) const {
  return getParent().get()->getSourceRangeOfEnclosedParamsOfAstNode(
      omitAssertions);
}

SourceRange EnumElementScope::getSourceRangeOfEnclosedParamsOfAstNode(
    bool omitAssertions) const {
  auto *pl = decl->getParameterList();
  return pl ? pl->getSourceRange() : SourceRange();
}

SourceRange SubscriptDeclScope::getSourceRangeOfEnclosedParamsOfAstNode(
    const bool omitAssertions) const {
  auto r = SourceRange(decl->getIndices()->getLParenLoc(), decl->getEndLoc());
  // Because of "subscript(x: MyStruct#^PARAM_1^#) -> Int { return 0 }"
  // Cannot just use decl->getEndLoc()
  r.widen(decl->getIndices()->getRParenLoc());
  return r;
}

SourceRange AbstractFunctionDeclScope::getSourceRangeOfEnclosedParamsOfAstNode(
    const bool omitAssertions) const {
  const auto s = getParmsSourceLocOfAFD(decl);
  const auto e = getSourceRangeOfThisAstNode(omitAssertions).end;
  return s.isInvalid() || e.isInvalid() ? SourceRange() : SourceRange(s, e);
}

SourceLoc
AbstractFunctionDeclScope::getParmsSourceLocOfAFD(AbstractFunctionDecl *decl) {
  if (auto *c = dyn_cast<ConstructorDecl>(decl))
    return c->getParameters()->getLParenLoc();

  if (auto *dd = dyn_cast<DestructorDecl>(decl))
    return dd->getNameLoc();

  auto *fd = cast<FuncDecl>(decl);
  // clang-format off
  return isa<AccessorDecl>(fd) ? fd->getLoc()
       : fd->isDeferBody()     ? fd->getNameLoc()
       :                         fd->getParameters()->getLParenLoc();
  // clang-format on
}

SourceLoc getLocAfterExtendedNominal(const ExtensionDecl *const ext) {
/// @todo
//  const auto *const etr = ext->getExtendedTypeRepr();
//  if (!etr)
//    return ext->getStartLoc();
//  const auto &SM = ext->getAstContext().SourceMgr;
//  return Lexer::getCharSourceRangeFromSourceRange(SM, etr->getSourceRange())
//      .getEnd();
}

SourceLoc ast_scope::extractNearestSourceLoc(
    std::tuple<AstScopeImpl *, ScopeCreator *> scopeAndCreator) {
  const AstScopeImpl *scope = std::get<0>(scopeAndCreator);
  return scope->getSourceRangeOfThisAstNode().start;
}

int AstScopeImpl::compare(const SourceRange lhs, const SourceRange rhs,
                          const SourceManager &SM, const bool ensureDisjoint) {
  ast_scope_assert(!SM.isBeforeInBuffer(lhs.end, lhs.start),
                 "Range is backwards.");
  ast_scope_assert(!SM.isBeforeInBuffer(rhs.end, rhs.start),
                 "Range is backwards.");

  auto cmpLoc = [&](const SourceLoc lhs, const SourceLoc rhs) {
    return lhs == rhs ? 0 : SM.isBeforeInBuffer(lhs, rhs) ? -1 : 1;
  };
  // Establish that we use end locations throughout AstScopes here
  const int endOrder = cmpLoc(lhs.end, rhs.end);

#ifndef NDEBUG
  if (ensureDisjoint) {
    const int startOrder = cmpLoc(lhs.start, rhs.start);

    if (startOrder * endOrder == -1) {
      llvm::errs() << "*** Start order contradicts end order between: ***\n";
      lhs.print(llvm::errs(), SM, false);
      llvm::errs() << "\n*** and: ***\n";
      rhs.print(llvm::errs(), SM, false);
    }
    ast_scope_assert(startOrder * endOrder != -1,
                   "Start order contradicts end order");
  }
#endif

  return endOrder;
}
