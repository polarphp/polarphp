//===--- ParseRequests.cpp - Parsing Requests -----------------------------===//
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
// Lazy parsing requests
//
//===----------------------------------------------------------------------===//
#include "polarphp/ast/ParseRequests.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/DeclContext.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/llparser/Parser.h"
#include "polarphp/global/Subsystems.h"

using namespace polar;
using namespace polar::llparser;

namespace polar {
// Implement the type checker type zone (zone 10).
#define POLAR_TYPEID_ZONE Parse
#define POLAR_TYPEID_HEADER "polarphp/ast/ParseTypeIDZoneDef.h"
#include "polarphp/basic/ImplementTypeIdZone.h"
#undef POLAR_TYPEID_ZONE
#undef POLAR_TYPEID_HEADER
}

ArrayRef<Decl *>
ParseMembersRequest::evaluate(Evaluator &evaluator,
                              IterableDeclContext *idc) const {
   SourceFile &sf = *idc->getDecl()->getDeclContext()->getParentSourceFile();
   unsigned bufferID = *sf.getBufferID();

   // Lexer diaganostics have been emitted during skipping, so we disable lexer's
   // diagnostic engine here.
   Parser parser(bufferID, sf, /*No Lexer Diags*/nullptr, nullptr, nullptr);
   // Disable libSyntax creation in the delayed parsing.
   /// TODO
//   parser.SyntaxContext->disable();
   AstContext &ctx = idc->getDecl()->getAstContext();
   return ctx.AllocateCopy(
      llvm::makeArrayRef(parser.parseDeclListDelayed(idc)));
}

BraceStmt *ParseAbstractFunctionBodyRequest::evaluate(
   Evaluator &evaluator, AbstractFunctionDecl *afd) const {
   using BodyKind = AbstractFunctionDecl::BodyKind;

   switch (afd->getBodyKind()) {
      case BodyKind::Deserialized:
      case BodyKind::MemberwiseInitializer:
      case BodyKind::None:
      case BodyKind::Skipped:
         return nullptr;

      case BodyKind::TypeChecked:
      case BodyKind::Parsed:
         return afd->Body;

      case BodyKind::Synthesize: {
         BraceStmt *body;
         bool isTypeChecked;

         std::tie(body, isTypeChecked) = (afd->Synthesizer.Fn)(
            afd, afd->Synthesizer.Context);
         assert(body && "cannot synthesize a null body");
         afd->setBodyKind(isTypeChecked ? BodyKind::TypeChecked : BodyKind::Parsed);
         return body;
      }

      case BodyKind::Unparsed: {
         // FIXME: How do we configure code completion?
         SourceFile &sf = *afd->getDeclContext()->getParentSourceFile();
         SourceManager &sourceMgr = sf.getAstContext().SourceMgr;
         unsigned bufferID = sourceMgr.findBufferContainingLoc(afd->getLoc());
         Parser parser(bufferID, sf, static_cast<PILParserTUStateBase *>(nullptr),
                       nullptr/*, nullptr*/);
         /// TODO
//         parser.SyntaxContext->disable();
         auto body = parser.parseAbstractFunctionBodyDelayed(afd);
         afd->setBodyKind(BodyKind::Parsed);
         return body;
      }
   }
   llvm_unreachable("Unhandled BodyKind in switch");
}


// Define request evaluation functions for each of the type checker requests.
static AbstractRequestFunction *parseRequestFunctions[] = {
#define POLAR_REQUEST(Zone, Name, Sig, Caching, LocOptions)                    \
  reinterpret_cast<AbstractRequestFunction *>(&Name::evaluateRequest),
#include "polarphp/ast/ParseTypeIDZoneDef.h"
#undef POLAR_REQUEST
};

void polar::registerParseRequestFunctions(Evaluator &evaluator) {
   evaluator.registerRequestFunctions(Zone::Parse,
                                      parseRequestFunctions);
}
