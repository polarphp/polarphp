//===--- Evaluator.h - Request Evaluator ------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
// This file defines the Evaluator class that evaluates and caches
// requests.
//
//===----------------------------------------------------------------------===//

#ifndef POLAR_AST_EVALUATOR_H
#define POLAR_AST_EVALUATOR_H

#include "polarphp/ast/AnyRequest.h"
#include "polarphp/basic/AnyValue.h"
#include "polarphp/Basic/CycleDiagnosticKind.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/basic/LangStatistic.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/SetVector.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/PrettyStackTrace.h"
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <optional>

namespace polar::utils {
class RawOutStream;
} // polar::utils

namespace polar::ast {

using polar::basic::ArrayRef;
using polar::utils::Expected;
using polar::utils::PrettyStackTraceEntry;
using polar::basic::simple_display;
using polar::utils::ErrorInfo;
using polar::basic::CycleDiagnosticKind;
using polar::basic::SetVector;
using polar::basic::DenseMap;
using polar::basic::AnyValue;
using polar::basic::StringRef;
using polar::basic::DenseSet;
using polar::basic::SmallVectorImpl;
using polar::basic::SmallVector;

class DiagnosticEngine;
class Evaluator;
class UnifiedStatsReporter;

/// An "abstract" m_request function pointer, which is the storage type
/// used for each of the
using AbstractRequestFunction = void(void);

/// Form the specific m_request function for the given m_request type.
template<typename Request>
using RequestFunction =
Expected<typename Request::OutputType>(const Request &, Evaluator &);

/// Pretty stack trace handler for an arbitrary m_request.
template<typename Request>
class PrettyStackTraceRequest : public PrettyStackTraceEntry
{
   const Request &m_request;

public:
   PrettyStackTraceRequest(const Request &request)
      : m_request(request)
   {}

   void print(RawOutStream &out) const
   {
      out << "While evaluating m_request ";
      simple_display(out, m_request);
      out << "\n";
   }
};

/// An llvm::ErrorInfo container for a m_request in which a cycle was detected
/// and diagnosed.
template <typename Request>
struct CyclicalRequestError :
      public ErrorInfo<CyclicalRequestError<Request>>
{
public:
   static char m_id;
   const Request &m_request;
   const Evaluator &m_evaluator;

   CyclicalRequestError(const Request &request, const Evaluator &evaluator)
      : m_request(request),
        m_evaluator(evaluator)
   {}

   virtual void log(RawOutStream &out) const;

   virtual std::error_code convertToErrorCode() const
   {
      // This is essentially unused, but is a temporary requirement for
      // llvm::ErrorInfo subclasses.
      polar_unreachable("shouldn't get std::error_code from CyclicalRequestError");
   }
};

template <typename Request>
char CyclicalRequestError<Request>::m_id = '\0';

/// Evaluates a given m_request or returns a default value if a cycle is detected.
template <typename Request>
typename Request::OutputType
evaluate_or_default(
      Evaluator &eval, Request req, typename Request::OutputType def)
{
   auto result = eval(req);
   if (auto err = result.takeError()) {
      polar::utils::handle_all_errors(std::move(err),
                                      [](const CyclicalRequestError<Request> &E) {
         // cycle detected
      });
      return def;
   }
   return *result;
}

/// Report that a m_request of the given kind is being evaluated, so it
/// can be recorded by the m_stats reporter.
template<typename Request>
void report_evaluated_request(UnifiedStatsReporter &m_stats,
                              const Request &request)
{}

/// Evaluation engine that evaluates and caches "requests", checking for cyclic
/// m_dependencies along the way.
///
/// Each m_request is a function object that accepts a reference to the m_evaluator
/// itself (through which it can m_request other values) and produces a
/// value. That value can then be cached by the m_evaluator for subsequent access,
/// using a policy dictated by the m_request itself.
///
/// The m_evaluator keeps track of all in-flight requests so that it can detect
/// and diagnose cyclic m_dependencies.
///
/// Each m_request should be its own function object, supporting the following
/// API:
///
///   - Copy constructor
///   - Equality operator (==)
///   - Hashing support (hash_value)
///   - TypeId support (see swift/Basic/TypeId.h)
///   - The output type (described via a nested type OutputType), which
///     must itself by a value type that supports TypeId.
///   - Evaluation via the function call operator:
///       OutputType operator()(Evaluator &m_evaluator) const;
///   - Cycle breaking and diagnostics operations:
///
///       void diagnoseCycle(DiagnosticEngine &m_diags) const;
///       void noteCycleStep(DiagnosticEngine &m_diags) const;
///   - Caching policy:
///
///     static const bool isEverCached;
///
///       When false, the m_request's result will never be cached. When true,
///       the result will be cached on completion. How it is cached depends on
///       the following.
///
///     bool isCached() const;
///
///       Dynamically indicates whether to m_cache this particular instance of the
///       m_request, so that (for example) requests for which a quick check
///       usually suffices can avoid caching a trivial result.
///
///     static const bool hasExternalCache;
///
///       When false, the results will be cached within the m_evaluator and
///       cannot be accessed except through the m_evaluator. This is the
///       best approach, because it ensures that all accesses to the result
///       are tracked.
///
///       When true, the m_request itself must provide an way to m_cache the
///       results, e.g., in some external data structure. External caching
///       should only be used when staging in the use of the m_evaluator into
///       existing mutable data structures; new computations should not depend
///       on it. Externally-cached requests must provide additional API:
///
///         Optional<OutputType> getCachedResult() const;
///
///           Retrieve the cached result, or \c None if there is no such
///           result.
///
///         void cacheResult(OutputType value) const;
///
///            Cache the given result.
class Evaluator
{
   /// The diagnostics engine through which any cyclic-dependency
   /// diagnostics will be emitted.
   DiagnosticEngine &m_diags;

   /// Whether to diagnose cycles or ignore them completely.
   CycleDiagnosticKind m_shouldDiagnoseCycles;

   /// Used to report statistics about which requests were evaluated, if
   /// non-null.
   UnifiedStatsReporter *m_stats = nullptr;

   /// A vector containing the abstract m_request functions that can compute
   /// the result of a particular m_request within a given zone. The
   /// \c uint8_t is the zone number of the m_request, and the array is
   /// indexed by the index of the m_request type within that zone. Each
   /// entry is a function pointer that will be reinterpret_cast'd to
   ///
   ///   RequestType::OutputType (*)(const RequestType &m_request,
   ///                               Evaluator &m_evaluator);
   /// and called to satisfy the m_request.
   std::vector<std::pair<uint8_t, ArrayRef<AbstractRequestFunction *>>>
   m_requestFunctionsByZone;

   /// A vector containing all of the active evaluation requests, which
   /// is treated as a stack and is used to detect cycles.
   SetVector<AnyRequest> m_activeRequests;

   /// A m_cache that stores the results of requests.
   DenseMap<AnyRequest, AnyValue> m_cache;

   /// Track the m_dependencies of each m_request.
   ///
   /// This is an adjacency-list representation expressing, for each known
   /// m_request, the requests that it directly depends on. It is populated
   /// lazily while the m_request is being evaluated.
   ///
   /// In a well-formed program, the graph should be a directed acycle graph
   /// (DAG). However, cyclic m_dependencies will be recorded within this graph,
   /// so all clients must cope with cycles.
   DenseMap<AnyRequest, std::vector<AnyRequest>> m_dependencies;

   /// Retrieve the m_request function for the given zone and m_request IDs.
   AbstractRequestFunction *getAbstractRequestFunction(uint8_t zoneID,
                                                       uint8_t requestID) const;

   /// Retrieve the m_request function for the given m_request type.
   template<typename Request>
   auto getRequestFunction() const -> RequestFunction<Request> *
   {
      auto abstractFn = getAbstractRequestFunction(TypeId<Request>::zoneID,
                                                   TypeId<Request>::localID);
      assert(abstractFn && "No m_request function for m_request");
      return reinterpret_cast<RequestFunction<Request> *>(abstractFn);
   }

public:
   /// Construct a new m_evaluator that can emit cyclic-dependency
   /// diagnostics through the given diagnostics engine.
   Evaluator(DiagnosticEngine &diags, CycleDiagnosticKind shouldDiagnoseCycles);

   /// Emit GraphViz output visualizing the m_request graph.
   void emitRequestEvaluatorGraphViz(StringRef graphVizPath);

   /// Set the unified m_stats reporter through which evaluated-m_request
   /// statistics will be recorded.
   void setStatsReporter(UnifiedStatsReporter *stats) { this->m_stats = stats; }

   /// Register the set of m_request functions for the given zone.
   ///
   /// These functions will be called to evaluate any requests within that
   /// zone.
   void registerRequestFunctions(uint8_t zoneID,
                                 ArrayRef<AbstractRequestFunction *> functions);

   /// Evaluate the given m_request and produce its result,
   /// consulting/populating the m_cache as required.
   template<typename Request>
   Expected<typename Request::OutputType>
   operator()(const Request &request)
   {
      // Check for a cycle.
      if (checkDependency(getCanonicalRequest(request))) {
         return polar::utils::Error(
                  polar::basic::make_unique<CyclicalRequestError<Request>>(request, *this));
      }

      // Make sure we remove this from the set of active requests once we're
      // done.
      POLAR_DEFER {
         assert(m_activeRequests.back().castTo<Request>() == request);
         m_activeRequests.pop_back();
      };

      // Get the result.
      return getResult(request);
   }

   /// Evaluate a set of requests and return their results as a tuple.
   ///
   /// Use this to describe cases where there are multiple (known)
   /// requests that all need to be satisfied.
   template<typename ...Requests>
   std::tuple<Expected<typename Requests::OutputType>...>
   operator()(const Requests &...requests)
   {
      return std::tuple<Expected<typename Requests::OutputType>...>(
               (*this)(requests)...);
   }

   /// Clear the m_cache stored within this m_evaluator.
   ///
   /// Note that this does not clear the caches of requests that use external
   /// caching.
   void clearCache()
   {
      m_cache.clear();
   }

private:
   template <typename Request>
   const AnyRequest &getCanonicalRequest(const Request &request)
   {
      // FIXME: DenseMap ought to let us do this with one hash lookup.
      auto iter = m_dependencies.find_as(request);
      if (iter != m_dependencies.end()) {
         return iter->first;
      }
      auto insertResult = m_dependencies.insert({AnyRequest(request), {}});
      assert(insertResult.second && "just checked if the key was already there");
      return insertResult.first->first;
   }

   /// Diagnose a cycle detected in the evaluation of the given
   /// m_request.
   void diagnoseCycle(const AnyRequest &request);

   /// Check the dependency from the current top of the stack to
   /// the given request, including cycle detection and diagnostics.
   ///
   /// \returns true if a cycle was detected, in which case this function has
   /// already diagnosed the cycle. Otherwise, returns \c false and adds this
   /// request to the \c m_activeRequests stack.
   bool checkDependency(const AnyRequest &request);

   /// Retrieve the result produced by evaluating a m_request that can
   /// be cached.
   template<typename Request,
            typename std::enable_if<Request::isEverCached>::type * = nullptr>
   Expected<typename Request::OutputType>
   getResult(const Request &request)
   {
      // The request can be cached, but check a predicate to determine
      // whether this particular instance is cached. This allows more
      // fine-grained control over which instances get m_cache.
      if (request.isCached()) {
         return getResultCached(request);
      }
      return getResultUncached(request);
   }

   /// Retrieve the result produced by evaluating a m_request that
   /// will never be cached.
   template<typename Request,
            typename std::enable_if<!Request::isEverCached>::type * = nullptr>
   Expected<typename Request::OutputType>
   getResult(const Request &request)
   {
      return getResultUncached(request);
   }

   /// Produce the result of the m_request without caching.
   template<typename Request>
   Expected<typename Request::OutputType>
   getResultUncached(const Request &request)
   {
      // Clear out the m_dependencies on this request; we're going to recompute
      // them now anyway.
      m_dependencies.find_as(request)->second.clear();

      PrettyStackTraceRequest<Request> prettyStackTrace(request);

      // Trace and/or count statistics.
      polar::basic::FrontendStatsTracer statsTracer = make_tracer(m_stats, request);
      if (m_stats) {
         report_evaluated_request(*m_stats, request);
      }
      return getRequestFunction<Request>()(request, *this);
   }

   /// Get the result of a request, consulting an external m_cache
   /// provided by the request to retrieve previously-computed results
   /// and detect recursion.
   template<typename Request,
            typename std::enable_if<Request::hasExternalCache>::type * = nullptr>
   Expected<typename Request::OutputType>
   getResultCached(const Request &request)
   {
      // If there is a cached result, return it.
      if (auto cached = request.getCachedResult()) {
         return *cached;
      }
      // Compute the result.
      auto result = getResultUncached(request);

      // Cache the result if applicable.
      if (!result) {
         return result;
      }
      request.cacheResult(*result);

      // Return it.
      return result;
   }

   /// Get the result of a request, consulting the general m_cache to
   /// retrieve previously-computed results and detect recursion.
   template<
         typename Request,
         typename std::enable_if<!Request::hasExternalCache>::type * = nullptr>
   Expected<typename Request::OutputType>
   getResultCached(const Request &request)
   {
      // If we already have an entry for this request in the m_cache, return it.
      auto known = m_cache.find_as(request);
      if (known != m_cache.end()) {
         return known->second.template castTo<typename Request::OutputType>();
      }

      // Compute the result.
      auto result = getResultUncached(request);
      if (!result) {
         return result;
      }

      // Cache the result.
      m_cache.insert({getCanonicalRequest(request), *result});
      return result;
   }

public:
   /// Print the m_dependencies of the given m_request as a tree.
   ///
   /// This is the core printing operation; most callers will want to use
   /// the other overload.
   void printDependencies(const AnyRequest &request,
                          RawOutStream &out,
                          DenseSet<AnyRequest> &visitedAnywhere,
                          SmallVectorImpl<AnyRequest> &visitedAlongPath,
                          ArrayRef<AnyRequest> highlightPath,
                          std::string &prefixStr,
                          bool lastChild) const;

   /// Print the m_dependencies of the given request as a tree.
   template <typename Request>
   void printDependencies(const Request &request, RawOutStream &out) const
   {
      std::string prefixStr;
      DenseSet<AnyRequest> visitedAnywhere;
      SmallVector<AnyRequest, 4> visitedAlongPath;
      printDependencies(AnyRequest(request), out, visitedAnywhere,
                        visitedAlongPath, { }, prefixStr, /*lastChild=*/true);
   }

   /// Dump the m_dependencies of the given request to the debugging stream
   /// as a tree.
   POLAR_ATTRIBUTE_DEPRECATED(
         void dumpDependencies(const AnyRequest &request) const POLAR_ATTRIBUTE_USED,
         "Only meant for use in the debugger");

   /// Print all m_dependencies known to the m_evaluator as a single Graphviz
   /// directed graph.
   void printDependenciesGraphviz(RawOutStream &out) const;

   POLAR_ATTRIBUTE_DEPRECATED(
         void dumpDependenciesGraphviz() const POLAR_ATTRIBUTE_USED,
         "Only meant for use in the debugger");
};

template <typename Request>
void CyclicalRequestError<Request>::log(RawOutStream &out) const
{
   out << "Cycle detected:\n";
   m_evaluator.printDependencies(m_request, out);
   out << "\n";
}

} // polar::ast

#endif // POLAR_AST_EVALUATOR_H
