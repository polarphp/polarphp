//===--- ParseRequests.h - Parsing Requests ---------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file defines parsing requests.
//
//===----------------------------------------------------------------------===//
#ifndef POLARPHP_AST_PARSE_REQUESTS_H
#define POLARPHP_AST_PARSE_REQUESTS_H

#include "polarphp/ast/AstTypeIds.h"
#include "polarphp/ast/SimpleRequest.h"

namespace polar {

/// Report that a request of the given kind is being evaluated, so it
/// can be recorded by the stats reporter.
template<typename Request>
void reportEvaluatedRequest(UnifiedStatsReporter &stats,
                            const Request &request);

/// Parse the members of a nominal type declaration or extension.
class ParseMembersRequest :
    public SimpleRequest<ParseMembersRequest,
                         ArrayRef<Decl *>(IterableDeclContext *),
                         CacheKind::Cached>
{
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ArrayRef<Decl *> evaluate(Evaluator &evaluator,
                            IterableDeclContext *idc) const;

public:
  // Caching
  bool isCached() const { return true; }
};

/// Parse the body of a function, initializer, or deinitializer.
class ParseAbstractFunctionBodyRequest :
    public SimpleRequest<ParseAbstractFunctionBodyRequest,
                         BraceStmt *(AbstractFunctionDecl *),
                         CacheKind::SeparatelyCached>
{
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  BraceStmt *evaluate(Evaluator &evaluator, AbstractFunctionDecl *afd) const;

public:
  // Caching
  bool isCached() const { return true; }
  Optional<BraceStmt *> getCachedResult() const;
  void cacheResult(BraceStmt *value) const;
};

/// The zone number for the parser.
#define POLAR_TYPEID_ZONE Parse
#define POLAR_TYPEID_HEADER "polarphp/ast/ParseTypeIDZoneDef.h"
#include "polarphp/basic/DefineTypeIDZone.h"
#undef POLAR_TYPEID_ZONE
#undef POLAR_TYPEID_HEADER

// Set up reporting of evaluated requests.
#define POLAR_REQUEST(Zone, RequestType, Sig, Caching, LocOptions)             \
  template <>                                                                  \
  inline void reportEvaluatedRequest(UnifiedStatsReporter &stats,              \
                                     const RequestType &request) {             \
    ++stats.getFrontendCounters().RequestType;                                 \
  }
#include "polarphp/ast/ParseTypeIDZoneDef.h"
#undef POLAR_REQUEST

} // end namespace polar

#endif // POLARPHP_AST_PARSE_REQUESTS_H
