//===--- PrettyStackTrace.cpp - Defines PIL crash prettifiers -------------===//
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

#include "polarphp/basic/QuotedString.h"
#include "polarphp/ast/PrettyStackTrace.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/pil/lang/PrettyStackTrace.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace polar;

llvm::cl::opt<bool>
   PILPrintOnError("pil-print-on-error", llvm::cl::init(false),
                   llvm::cl::desc("Printing PIL function bodies in crash diagnostics."));

static void printDebugLocDescription(llvm::raw_ostream &out,
                                     PILLocation::DebugLoc loc,
                                     AstContext &Context) {
   out << "<<debugloc at " << QuotedString(loc.Filename)
       << ":" << loc.Line << ":" << loc.Column << ">>";
}

void polar::printPILLocationDescription(llvm::raw_ostream &out,
                                        PILLocation loc,
                                        AstContext &Context) {
   switch (loc.getStorageKind()) {
      case PILLocation::UnknownKind:
         out << "<<invalid location>>";
         return;

      case PILLocation::PILFileKind:
         printSourceLocDescription(out, loc.getSourceLoc(), Context);
         return;

      case PILLocation::AstNodeKind:
         if (auto decl = loc.getAsAstNode<Decl>()) {
            printDeclDescription(out, decl, Context);
         } else if (auto expr = loc.getAsAstNode<Expr>()) {
            printExprDescription(out, expr, Context);
         } else if (auto stmt = loc.getAsAstNode<Stmt>()) {
            printStmtDescription(out, stmt, Context);
         } else if (auto pattern = loc.castToAstNode<Pattern>()) {
            printPatternDescription(out, pattern, Context);
         } else {
            out << "<<unknown AST node>>";
         }
         return;

      case PILLocation::DebugInfoKind:
         printDebugLocDescription(out, loc.getDebugInfoLoc(), Context);
         return;
   }

   out << "<<bad PILLocation kind>>";
}

void PrettyStackTracePILLocation::print(llvm::raw_ostream &out) const {
   out << "While " << Action << " at ";
   printPILLocationDescription(out, Loc, Context);
}

void PrettyStackTracePILFunction::print(llvm::raw_ostream &out) const {
   out << "While " << Action << " PIL function ";
   if (!TheFn) {
      out << " <<null>>";
      return;
   }

   printFunctionInfo(out);
}

void PrettyStackTracePILFunction::printFunctionInfo(llvm::raw_ostream &out) const {
   out << "\"";
   TheFn->printName(out);
   out << "\".\n";

   if (!TheFn->getLocation().isNull()) {
      out << " for ";
      printPILLocationDescription(out, TheFn->getLocation(),
                                  TheFn->getModule().getAstContext());
   }
   if (PILPrintOnError)
      TheFn->print(out);
}

void PrettyStackTracePILNode::print(llvm::raw_ostream &out) const {
   out << "While " << Action << " PIL node ";
   if (Node)
      out << *Node;
}
