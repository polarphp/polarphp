//===--- AstScopeImpl.cpp - Swift Object-Oriented AST Scope ---------------===//
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
/// This file implements the common functions of the 49 ontology.
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
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeRepr.h"
#include "polarphp/basic/NullablePtr.h"
#include "polarphp/basic/StlExtras.h"
#include "llvm/Support/Compiler.h"
#include <algorithm>

#pragma mark AstScope

namespace polar {

using namespace ast_scope;

using ast_scope::AstScopeImpl;

llvm::SmallVector<const AstScopeImpl *, 0> AstScope::unqualifiedLookup(
   SourceFile *SF, DeclName name, SourceLoc loc,
   const DeclContext *startingContext,
   namelookup::AbstractAstScopeDeclConsumer &consumer) {
   if (auto *s = SF->getAstContext().Stats)
      ++s->getFrontendCounters().NumAstScopeLookups;
   return AstScopeImpl::unqualifiedLookup(SF, name, loc, startingContext,
                                          consumer);
}

Optional<bool> AstScope::computeIsCascadingUse(
   ArrayRef<const ast_scope::AstScopeImpl *> history,
   Optional<bool> initialIsCascadingUse) {
   return AstScopeImpl::computeIsCascadingUse(history, initialIsCascadingUse);
}

#if POLAR_CC_MSVC
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

void AstScope::dump() const { impl->dump(); }

#if POLAR_CC_MSVC
#pragma warning(pop)
#endif

void AstScope::print(llvm::raw_ostream &out) const { impl->print(out); }
void AstScope::dumpOneScopeMapLocation(std::pair<unsigned, unsigned> lineCol) {
   impl->dumpOneScopeMapLocation(lineCol);
}

#pragma mark AstScopeImpl


const PatternBindingEntry &AbstractPatternEntryScope::getPatternEntry() const {
   return decl->getPatternList()[patternEntryIndex];
}

Pattern *AbstractPatternEntryScope::getPattern() const {
   return getPatternEntry().getPattern();
}

NullablePtr<ClosureExpr> BraceStmtScope::parentClosureIfAny() const {
   return !getParent() ? nullptr : getParent().get()->getClosureIfClosureScope();
}

NullablePtr<ClosureExpr> AstScopeImpl::getClosureIfClosureScope() const {
   return nullptr;
}
NullablePtr<ClosureExpr>
AbstractClosureScope::getClosureIfClosureScope() const {
   return closureExpr;
}

// Conservative, because using precise info would be circular
SourceRange
AttachedPropertyWrapperScope::getSourceRangeOfVarDecl(const VarDecl *const vd) {
   SourceRange sr;
   for (auto *attr : vd->getAttrs().getAttributes<CustomAttr>()) {
      if (sr.isInvalid())
         sr = attr->getTypeLoc().getSourceRange();
      else
         sr.widen(attr->getTypeLoc().getSourceRange());
   }
   return sr;
}

SourceManager &AstScopeImpl::getSourceManager() const {
   return getAstContext().SourceMgr;
}

Stmt *LabeledConditionalStmtScope::getStmt() const {
   return getLabeledConditionalStmt();
}

bool AbstractFunctionBodyScope::isAMethod(
   const AbstractFunctionDecl *const afd) {
   // What makes this interesting is that a method named "init" which is not
   // in a nominal type or extension decl body still gets an implicit self
   // parameter (even though the program is illegal).
   // So when choosing between creating a MethodBodyScope and a
   // PureFunctionBodyScope do we go by the enclosing Decl (i.e.
   // "afd->getDeclContext()->isTypeContext()") or by
   // "bool(afd->getImplicitSelfDecl())"?
   //
   // Since the code uses \c getImplicitSelfDecl, use that.
   return afd->getImplicitSelfDecl();
}

#pragma mark getLabeledConditionalStmt
LabeledConditionalStmt *IfStmtScope::getLabeledConditionalStmt() const {
   return stmt;
}
LabeledConditionalStmt *WhileStmtScope::getLabeledConditionalStmt() const {
   return stmt;
}
LabeledConditionalStmt *GuardStmtScope::getLabeledConditionalStmt() const {
   return stmt;
}


#pragma mark getAstContext

AstContext &AstScopeImpl::getAstContext() const {
   if (auto d = getDeclIfAny())
      return d.get()->getAstContext();
   if (auto dc = getDeclContext())
      return dc.get()->getAstContext();
   return getParent().get()->getAstContext();
}

#pragma mark getDeclContext

NullablePtr<DeclContext> AstScopeImpl::getDeclContext() const {
   return nullptr;
}

NullablePtr<DeclContext> AstSourceFileScope::getDeclContext() const {
   return NullablePtr<DeclContext>(SF);
}

NullablePtr<DeclContext> GenericTypeOrExtensionScope::getDeclContext() const {
   return getGenericContext();
}

NullablePtr<DeclContext> GenericParamScope::getDeclContext() const {
   return dyn_cast<DeclContext>(holder);
}

NullablePtr<DeclContext> PatternEntryInitializerScope::getDeclContext() const {
   return getPatternEntry().getInitContext();
}

NullablePtr<DeclContext> BraceStmtScope::getDeclContext() const {
   return getParent().get()->getDeclContext();
}

NullablePtr<DeclContext>
DefaultArgumentInitializerScope::getDeclContext() const {
   auto *dc = decl->getDefaultArgumentInitContext();
   ast_scope_assert(dc, "If scope exists, this must exist");
   return dc;
}

// When asked for a loc in an intializer in a capture list, the asked-for
// context is the closure.
NullablePtr<DeclContext> CaptureListScope::getDeclContext() const {
   return expr->getClosureBody();
}

NullablePtr<DeclContext> AttachedPropertyWrapperScope::getDeclContext() const {
   return decl->getParentPatternBinding()->getInitContext(0);
}

NullablePtr<DeclContext> AbstractFunctionDeclScope::getDeclContext() const {
   return decl;
}

NullablePtr<DeclContext> ParameterListScope::getDeclContext() const {
   return matchingContext;
}

#pragma mark getClassName

std::string GenericTypeOrExtensionScope::getClassName() const {
   return declKindName() + portionName() + "Scope";
}

#define DEFINE_GET_CLASS_NAME(Name)                                            \
  std::string Name::getClassName() const { return #Name; }

DEFINE_GET_CLASS_NAME(AstSourceFileScope)
DEFINE_GET_CLASS_NAME(GenericParamScope)
DEFINE_GET_CLASS_NAME(AbstractFunctionDeclScope)
DEFINE_GET_CLASS_NAME(ParameterListScope)
DEFINE_GET_CLASS_NAME(MethodBodyScope)
DEFINE_GET_CLASS_NAME(PureFunctionBodyScope)
DEFINE_GET_CLASS_NAME(DefaultArgumentInitializerScope)
DEFINE_GET_CLASS_NAME(AttachedPropertyWrapperScope)
DEFINE_GET_CLASS_NAME(PatternEntryDeclScope)
DEFINE_GET_CLASS_NAME(PatternEntryInitializerScope)
DEFINE_GET_CLASS_NAME(ConditionalClauseScope)
DEFINE_GET_CLASS_NAME(ConditionalClausePatternUseScope)
DEFINE_GET_CLASS_NAME(CaptureListScope)
DEFINE_GET_CLASS_NAME(WholeClosureScope)
DEFINE_GET_CLASS_NAME(ClosureParametersScope)
DEFINE_GET_CLASS_NAME(ClosureBodyScope)
DEFINE_GET_CLASS_NAME(TopLevelCodeScope)
DEFINE_GET_CLASS_NAME(SpecializeAttributeScope)
DEFINE_GET_CLASS_NAME(SubscriptDeclScope)
DEFINE_GET_CLASS_NAME(VarDeclScope)
DEFINE_GET_CLASS_NAME(EnumElementScope)
DEFINE_GET_CLASS_NAME(IfStmtScope)
DEFINE_GET_CLASS_NAME(WhileStmtScope)
DEFINE_GET_CLASS_NAME(GuardStmtScope)
DEFINE_GET_CLASS_NAME(LookupParentDiversionScope)
DEFINE_GET_CLASS_NAME(RepeatWhileScope)
DEFINE_GET_CLASS_NAME(DoCatchStmtScope)
DEFINE_GET_CLASS_NAME(SwitchStmtScope)
DEFINE_GET_CLASS_NAME(ForEachStmtScope)
DEFINE_GET_CLASS_NAME(ForEachPatternScope)
DEFINE_GET_CLASS_NAME(CatchStmtScope)
DEFINE_GET_CLASS_NAME(CaseStmtScope)
DEFINE_GET_CLASS_NAME(BraceStmtScope)

#undef DEFINE_GET_CLASS_NAME

#pragma mark getSourceFile

const SourceFile *AstScopeImpl::getSourceFile() const {
   return getParent().get()->getSourceFile();
}

const SourceFile *AstSourceFileScope::getSourceFile() const { return SF; }

SourceRange ExtensionScope::getBraces() const { return decl->getBraces(); }

SourceRange NominalTypeScope::getBraces() const { return decl->getBraces(); }

NullablePtr<NominalTypeDecl>
ExtensionScope::getCorrespondingNominalTypeDecl() const {
   return decl->getExtendedNominal();
}

void AstScopeImpl::preOrderDo(function_ref<void(AstScopeImpl *)> fn) {
   fn(this);
   preOrderChildrenDo(fn);
}
void AstScopeImpl::preOrderChildrenDo(function_ref<void(AstScopeImpl *)> fn) {
   for (auto *child : getChildren())
      child->preOrderDo(fn);
}

void AstScopeImpl::postOrderDo(function_ref<void(AstScopeImpl *)> fn) {
   for (auto *child : getChildren())
      child->postOrderDo(fn);
   fn(this);
}

ArrayRef<StmtConditionElement> ConditionalClauseScope::getCond() const {
   return stmt->getCond();
}

const StmtConditionElement &
ConditionalClauseScope::getStmtConditionElement() const {
   return getCond()[index];
}

unsigned AstScopeImpl::countDescendants() const {
   unsigned count = 0;
   const_cast<AstScopeImpl *>(this)->preOrderDo(
      [&](AstScopeImpl *) { ++count; });
   return count - 1;
}

// Can fail if a subscope is lazy and not reexpanded
void AstScopeImpl::assertThatTreeDoesNotShrink(function_ref<void()> fn) {
#ifndef NDEBUG
   unsigned beforeCount = countDescendants();
#endif
   fn();
#ifndef NDEBUG
   unsigned afterCount = countDescendants();
   ast_scope_assert(beforeCount <= afterCount, "shrank?!");
#endif
}

} // polar