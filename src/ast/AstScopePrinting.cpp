//===--- AstScopePrinting.cpp - Swift Object-Oriented Ast Scope -----------===//
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
/// This file implements the printing functions of the AstScopeImpl ontology.
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
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeRepr.h"
#include "polarphp/basic/StlExtras.h"
#include <algorithm>

using namespace polar;
using namespace ast_scope;

#pragma mark dumping

void AstScopeImpl::dump() const { print(llvm::errs(), 0, false); }

void AstScopeImpl::dumpOneScopeMapLocation(
   std::pair<unsigned, unsigned> lineColumn) {
   auto bufferID = getSourceFile()->getBufferID();
   if (!bufferID) {
      llvm::errs() << "***No buffer, dumping all scopes***";
      print(llvm::errs());
      return;
   }
   SourceLoc loc = getSourceManager().getLocForLineCol(
      *bufferID, lineColumn.first, lineColumn.second);
   if (loc.isInvalid())
      return;

   llvm::errs() << "***Scope at " << lineColumn.first << ":" << lineColumn.second
                << "***\n";
   auto *locScope = findInnermostEnclosingScope(loc, &llvm::errs());
   locScope->print(llvm::errs(), 0, false, false);

   // Dump the Ast context, too.
   if (auto *dc = locScope->getDeclContext().getPtrOrNull())
      dc->printContext(llvm::errs());

   namelookup::AstScopeDeclGatherer gatherer;
   // Print the local bindings introduced by this scope.
   locScope->lookupLocalsOrMembers({this}, gatherer);
   if (!gatherer.getDecls().empty()) {
      llvm::errs() << "Local bindings: ";
      interleave(gatherer.getDecls().begin(), gatherer.getDecls().end(),
                 [&](ValueDecl *value) { llvm::errs() << value->getFullName(); },
                 [&]() { llvm::errs() << " "; });
      llvm::errs() << "\n";
   }
}

llvm::raw_ostream &AstScopeImpl::verificationError() const {
   return llvm::errs() << "AstScopeImpl verification error in source file '"
                       << getSourceFile()->getFilename() << "': ";
}

#pragma mark printing

void AstScopeImpl::print(llvm::raw_ostream &out, unsigned level, bool lastChild,
                         bool printChildren) const {
   // Indent for levels 2+.
   if (level > 1)
      out.indent((level - 1) * 2);

   // Print child marker and leading '-' for levels 1+.
   if (level > 0)
      out << (lastChild ? '`' : '|') << '-';

   out << getClassName();
   if (auto *a = addressForPrinting().getPtrOrNull())
      out << " " << a;
   out << ", ";
   if (auto *d = getDeclIfAny().getPtrOrNull()) {
      if (d->isImplicit())
         out << "implicit ";
   }
   printRange(out);
   out << " ";
   printSpecifics(out);
   out << "\n";

   if (printChildren) {
      for (unsigned i : indices(getChildren())) {
         getChildren()[i]->print(out, level + 1,
            /*lastChild=*/i == getChildren().size() - 1);
      }
   }
}

static void printSourceRange(llvm::raw_ostream &out, const SourceRange range,
                             SourceManager &SM) {
   if (range.isInvalid()) {
      out << "[invalid source range]";
      return;
   }

   auto startLineAndCol = SM.getLineAndColumn(range.start);
   auto endLineAndCol = SM.getLineAndColumn(range.end);

   out << "[" << startLineAndCol.first << ":" << startLineAndCol.second << " - "
       << endLineAndCol.first << ":" << endLineAndCol.second << "]";
}

void AstScopeImpl::printRange(llvm::raw_ostream &out) const {
   if (!isSourceRangeCached(true))
      out << "(uncached) ";
   SourceRange range = computeSourceRangeOfScope(/*omitAssertions=*/true);
   printSourceRange(out, range, getSourceManager());
}

#pragma mark printSpecifics


void AstSourceFileScope::printSpecifics(
   llvm::raw_ostream &out) const {
   out << "'" << SF->getFilename() << "'";
}

NullablePtr<const void> AstScopeImpl::addressForPrinting() const {
   if (auto *p = getDeclIfAny().getPtrOrNull())
      return p;
   if (auto *p = getStmtIfAny().getPtrOrNull())
      return p;
   if (auto *p = getExprIfAny().getPtrOrNull())
      return p;
   return nullptr;
}

void GenericTypeOrExtensionScope::printSpecifics(llvm::raw_ostream &out) const {
   if (shouldHaveABody() && !doesDeclHaveABody())
      out << "<no body>";
   // Sadly, the following can trip assertions
   //  else if (auto *n = getCorrespondingNominalTypeDecl().getPtrOrNull())
   //    out << "'" << n->getFullName() << "'";
   //  else
   //    out << "<no extended nominal?!>";
}

void GenericParamScope::printSpecifics(llvm::raw_ostream &out) const {
   out << "param " << index;
   auto *genericTypeParamDecl = paramList->getParams()[index];
   out << " '";
   genericTypeParamDecl->print(out);
   out << "'";
}

void AbstractFunctionDeclScope::printSpecifics(llvm::raw_ostream &out) const {
   out << "'" << decl->getFullName() << "'";
}

void AbstractPatternEntryScope::printSpecifics(llvm::raw_ostream &out) const {
   out << "entry " << patternEntryIndex;
   getPattern()->forEachVariable([&](VarDecl *vd) {
      out << " '" << vd->getName() << "'";
   });
}

void ConditionalClauseScope::printSpecifics(llvm::raw_ostream &out) const {
   AstScopeImpl::printSpecifics(out);
   out << "index " << index;
}

void SubscriptDeclScope::printSpecifics(llvm::raw_ostream &out) const {
   decl->dumpRef(out);
}

void VarDeclScope::printSpecifics(llvm::raw_ostream &out) const {
   decl->dumpRef(out);
}

void ConditionalClausePatternUseScope::printSpecifics(
   llvm::raw_ostream &out) const {
   pattern->print(out);
}

bool GenericTypeOrExtensionScope::doesDeclHaveABody() const { return false; }

bool IterableTypeScope::doesDeclHaveABody() const {
   return getBraces().start != getBraces().end;
}

void ast_scope::simple_display(llvm::raw_ostream &out,
                               const AstScopeImpl *scope) {
   // Cannot call scope->print(out) because printing an AstFunctionBodyScope
   // gets the source range which can cause a request to parse it.
   // That in turn causes the request dependency printing code to blow up
   // as the AnyRequest ends up with a null.
   out << scope->getClassName() << "\n";
}
