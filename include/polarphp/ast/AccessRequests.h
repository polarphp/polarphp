//===--- AccessRequests.h - Access{Level,Scope} Requests -----*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/05.
//===----------------------------------------------------------------------===//
//  This file defines access-control requests.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ACCESS_REQUESTS_H
#define POLARPHP_AST_ACCESS_REQUESTS_H

#include "polarphp/ast/AccessScope.h"
#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/Evaluator.h"
#include "polarphp/ast/SimpleRequest.h"
#include "polarphp/basic/Statistic.h"
#include "llvm/ADT/Hashing.h"

namespace polar {
class AbstractStorageDecl;
class ExtensionDecl;
class ValueDecl;

/// Request the AccessLevel of the given ValueDecl.
class AccessLevelRequest : public SimpleRequest<AccessLevelRequest,
      AccessLevel(ValueDecl *),
CacheKind::SeparatelyCached>
{
   public:
   using SimpleRequest::SimpleRequest;

   private:
   friend SimpleRequest;

   // Evaluation.
   llvm::Expected<AccessLevel> evaluate(Evaluator &evaluator,
                                        ValueDecl *decl) const;

   public:
   // Separate caching.
   bool isCached() const { return true; }
   Optional<AccessLevel> getCachedResult() const;
   void cacheResult(AccessLevel value) const;
};

/// Request the setter AccessLevel of the given AbstractStorageDecl,
/// which may be lower than its normal AccessLevel, and determines
/// the accessibility of mutating accessors.
class SetterAccessLevelRequest :
      public SimpleRequest<SetterAccessLevelRequest,
      AccessLevel(AbstractStorageDecl *),
CacheKind::SeparatelyCached>
{
public:
   using SimpleRequest::SimpleRequest;

private:
   friend SimpleRequest;

   // Evaluation.
   llvm::Expected<AccessLevel>
         evaluate(Evaluator &evaluator, AbstractStorageDecl *decl) const;

public:
   // Separate caching.
   bool isCached() const { return true; }
   Optional<AccessLevel> getCachedResult() const;
   void cacheResult(AccessLevel value) const;
};

using DefaultAndMax = std::pair<AccessLevel, AccessLevel>;

/// Request the Default and Max AccessLevels of the given ExtensionDecl.
class DefaultAndMaxAccessLevelRequest :
      public SimpleRequest<DefaultAndMaxAccessLevelRequest, DefaultAndMax(ExtensionDecl *), CacheKind::SeparatelyCached>
{
   public:
   using SimpleRequest::SimpleRequest;
   private:
   friend SimpleRequest;

   // Evaluation.
   llvm::Expected<DefaultAndMax>
         evaluate(Evaluator &evaluator, ExtensionDecl *decl) const;

   public:
   // Separate caching.
   bool isCached() const { return true; }
   Optional<DefaultAndMax> getCachedResult() const;
   void cacheResult(DefaultAndMax value) const;
};

#define POLAR_TYPEID_ZONE AccessControl
#define POLAR_TYPEID_HEADER "polarphp/ast/AccessTypeIdZoneDef.h"
#include "polarphp/basic/DefineTypeIdZone.h"
#undef POLAR_TYPEID_ZONE
#undef POLAR_TYPEID_HEADER

// Set up reporting of evaluated requests.
#define POLAR_REQUEST(Zone, RequestType, Sig, Caching, LocOptions)             \
   template <>                                                                  \
   inline void reportEvaluatedRequest(UnifiedStatsReporter &stats,              \
   const RequestType &request) {             \
   ++stats.getFrontendCounters().RequestType;                                 \
}
#include "polarphp/ast/AccessTypeIdZoneDef.h"
#undef POLAR_REQUEST

} // polar

#endif // POLARPHP_AST_ACCESS_REQUESTS_H
