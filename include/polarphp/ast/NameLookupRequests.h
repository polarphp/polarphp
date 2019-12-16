//===--- NameLookupRequests.h - Name Lookup Requests ------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file defines name-lookup requests.
//
//===----------------------------------------------------------------------===//
#ifndef POLARPHP_NAME_LOOKUP_REQUESTS_H
#define POLARPHP_NAME_LOOKUP_REQUESTS_H

#include "polarphp/ast/SimpleRequest.h"
#include "polarphp/ast/AstTypeIds.h"
#include "polarphp/basic/Statistic.h"
#include "llvm/ADT/Hashing.h"
#include "polarphp/basic/Hashing.h"
#include "llvm/ADT/TinyPtrVector.h"

namespace polar {
class SourceLoc;
}

namespace polar {

class ClassDecl;
class DeclContext;
class DeclName;
class DestructorDecl;
class GenericContext;
class GenericParamList;
class LookupResult;
class TypeAliasDecl;
class TypeDecl;
enum class UnqualifiedLookupFlags;

using polar::SourceLoc;

namespace ast_scope {
class AstScopeImpl;
class ScopeCreator;
using polar::hash_value;
} // namespace ast_scope

/// Display a nominal type or extension thereof.
void simple_display(
       llvm::raw_ostream &out,
       const llvm::PointerUnion<TypeDecl *, ExtensionDecl *> &value);

/// Describes a set of type declarations that are "direct" referenced by
/// a particular type in the AST.
using DirectlyReferencedTypeDecls = llvm::TinyPtrVector<TypeDecl *>;

/// Request the set of declarations directly referenced by the an "inherited"
/// type of a type or extension declaration.
///
/// This request retrieves the set of declarations that are directly referenced
/// by a particular type in the "inherited" clause of a type or extension
/// declaration. For example:
///
/// \code
///   protocol P { }
///   protocol Q { }
///   typealias Alias = P & Q
///   class C { }
///
///   class D: C, Alias { }
/// \endcode
///
/// The inherited declaration of \c D at index 0 is the class declaration C.
/// The inherited declaration of \c D at index 1 is the typealias Alias.
class InheritedDeclsReferencedRequest :
  public SimpleRequest<InheritedDeclsReferencedRequest,
                       DirectlyReferencedTypeDecls(
                         llvm::PointerUnion<TypeDecl *, ExtensionDecl *>,
                         unsigned),
                       CacheKind::Uncached> // FIXME: Cache these
{
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  DirectlyReferencedTypeDecls evaluate(
      Evaluator &evaluator,
      llvm::PointerUnion<TypeDecl *, ExtensionDecl *> decl,
      unsigned index) const;

public:
  // Caching
  bool isCached() const { return true; }

  // Source location information.
  SourceLoc getNearestLoc() const;
};

/// Request the set of declarations directly referenced by the underlying
/// type of a typealias.
///
/// This request retrieves the set of type declarations that directly referenced
/// by the underlying type of a typealias. For example:
///
/// \code
///   protocol P { }
///   protocol Q { }
///   class C { }
///   typealias PQ = P & Q
///   typealias Alias = C & PQ
/// \endcode
///
/// The set of declarations referenced by the underlying type of \c PQ
/// contains both \c P and \c Q.
/// The set of declarations referenced by the underlying type of \c Alias
/// contains \c C and \c PQ. Clients can choose to look further into \c PQ
/// using another instance of this request.
class UnderlyingTypeDeclsReferencedRequest :
  public SimpleRequest<UnderlyingTypeDeclsReferencedRequest,
                       DirectlyReferencedTypeDecls(TypeAliasDecl *),
                       CacheKind::Uncached> // FIXME: Cache these
{
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  DirectlyReferencedTypeDecls evaluate(
      Evaluator &evaluator,
      TypeAliasDecl *typealias) const;

public:
  // Caching
  bool isCached() const { return true; }
};

/// Request the superclass declaration for the given class.
class SuperclassDeclRequest :
    public SimpleRequest<SuperclassDeclRequest,
                         ClassDecl *(NominalTypeDecl *),
                         CacheKind::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::Expected<ClassDecl *>
  evaluate(Evaluator &evaluator, NominalTypeDecl *subject) const;

public:
  // Caching
  bool isCached() const { return true; }
  Optional<ClassDecl *> getCachedResult() const;
  void cacheResult(ClassDecl *value) const;
};

/// Request the nominal declaration extended by a given extension declaration.
class ExtendedNominalRequest :
    public SimpleRequest<ExtendedNominalRequest,
                         NominalTypeDecl *(ExtensionDecl *),
                         CacheKind::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::Expected<NominalTypeDecl *>
  evaluate(Evaluator &evaluator, ExtensionDecl *ext) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<NominalTypeDecl *> getCachedResult() const;
  void cacheResult(NominalTypeDecl *value) const;
};

struct SelfBounds {
  llvm::TinyPtrVector<NominalTypeDecl *> decls;
  bool anyObject = false;
};

/// Request the nominal types that occur as the right-hand side of "Self: Foo"
/// constraints in the "where" clause of a protocol extension.
class SelfBoundsFromWhereClauseRequest :
    public SimpleRequest<SelfBoundsFromWhereClauseRequest,
                         SelfBounds(llvm::PointerUnion<TypeDecl *,
                                                       ExtensionDecl *>),
                         CacheKind::Uncached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  SelfBounds evaluate(Evaluator &evaluator,
                      llvm::PointerUnion<TypeDecl *, ExtensionDecl *>) const;
};


/// Request all type aliases and nominal types that appear in the "where"
/// clause of an extension.
class TypeDeclsFromWhereClauseRequest :
    public SimpleRequest<TypeDeclsFromWhereClauseRequest,
                         DirectlyReferencedTypeDecls(ExtensionDecl *),
                         CacheKind::Uncached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  DirectlyReferencedTypeDecls evaluate(Evaluator &evaluator,
                                       ExtensionDecl *ext) const;
};

/// Request the nominal type declaration to which the given custom attribute
/// refers.
class CustomAttrNominalRequest :
    public SimpleRequest<CustomAttrNominalRequest,
                         NominalTypeDecl *(CustomAttr *, DeclContext *),
                         CacheKind::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::Expected<NominalTypeDecl *>
  evaluate(Evaluator &evaluator, CustomAttr *attr, DeclContext *dc) const;

public:
  // Caching
  bool isCached() const { return true; }
};

/// Finds or synthesizes a destructor for the given class.
class GetDestructorRequest :
    public SimpleRequest<GetDestructorRequest,
                         DestructorDecl *(ClassDecl *),
                         CacheKind::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::Expected<DestructorDecl *>
  evaluate(Evaluator &evaluator, ClassDecl *classDecl) const;

public:
  // Caching
  bool isCached() const { return true; }
  Optional<DestructorDecl *> getCachedResult() const;
  void cacheResult(DestructorDecl *value) const;
};

class GenericParamListRequest :
    public SimpleRequest<GenericParamListRequest,
                         GenericParamList *(GenericContext *),
                         CacheKind::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::Expected<GenericParamList *>
  evaluate(Evaluator &evaluator, GenericContext *value) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<GenericParamList *> getCachedResult() const;
  void cacheResult(GenericParamList *value) const;
};

/// Expand the given ASTScope. Requestified to detect recursion.
class ExpandAstScopeRequest
    : public SimpleRequest<ExpandAstScopeRequest,
                           ast_scope::AstScopeImpl *(ast_scope::AstScopeImpl *,
                                                     ast_scope::ScopeCreator *),
                           CacheKind::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::Expected<ast_scope::AstScopeImpl *>
  evaluate(Evaluator &evaluator, ast_scope::AstScopeImpl *,
           ast_scope::ScopeCreator *) const;

public:
  // Separate caching.
  bool isCached() const;
  Optional<ast_scope::AstScopeImpl *> getCachedResult() const;
  void cacheResult(ast_scope::AstScopeImpl *) const {}
};

/// The input type for an unqualified lookup request.
class UnqualifiedLookupDescriptor {
  using LookupOptions = OptionSet<UnqualifiedLookupFlags>;

public:
  DeclName Name;
  DeclContext *DC;
  SourceLoc Loc;
  LookupOptions Options;

  UnqualifiedLookupDescriptor(DeclName name, DeclContext *dc,
                              SourceLoc loc = SourceLoc(),
                              LookupOptions options = {})
      : Name(name), DC(dc), Loc(loc), Options(options) {}

  friend llvm::hash_code hash_value(const UnqualifiedLookupDescriptor &desc) {
    return llvm::hash_combine(desc.Name, desc.DC, desc.Loc,
                              desc.Options.toRaw());
  }

  friend bool operator==(const UnqualifiedLookupDescriptor &lhs,
                         const UnqualifiedLookupDescriptor &rhs) {
    return lhs.Name == rhs.Name && lhs.DC == rhs.DC && lhs.Loc == rhs.Loc &&
           lhs.Options.toRaw() == rhs.Options.toRaw();
  }

  friend bool operator!=(const UnqualifiedLookupDescriptor &lhs,
                         const UnqualifiedLookupDescriptor &rhs) {
    return !(lhs == rhs);
  }
};

void simple_display(llvm::raw_ostream &out,
                    const UnqualifiedLookupDescriptor &desc);

SourceLoc extractNearestSourceLoc(const UnqualifiedLookupDescriptor &desc);

/// Performs unqualified lookup for a DeclName from a given context.
class UnqualifiedLookupRequest
    : public SimpleRequest<UnqualifiedLookupRequest,
                           LookupResult(UnqualifiedLookupDescriptor),
                           CacheKind::Uncached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::Expected<LookupResult> evaluate(Evaluator &evaluator,
                                        UnqualifiedLookupDescriptor desc) const;
};

#define POLAR_TYPEID_ZONE NameLookup
#define POLAR_TYPEID_HEADER "polarphp/ast/NameLookupTypeIDZoneDef.h"
#include "polarphp/basic/DefineTypeIdZone.h"
#undef POLAR_TYPEID_ZONE
#undef POLAR_TYPEID_HEADER

// Set up reporting of evaluated requests.
template<typename Request>
void reportEvaluatedRequest(UnifiedStatsReporter &stats,
                            const Request &request);

#define POLAR_REQUEST(Zone, RequestType, Sig, Caching, LocOptions)             \
  template <>                                                                  \
  inline void reportEvaluatedRequest(UnifiedStatsReporter &stats,              \
                                     const RequestType &request) {             \
    ++stats.getFrontendCounters().RequestType;                                 \
  }
#include "polarphp/ast/NameLookupTypeIDZoneDef.h"
#undef POLAR_REQUEST

} // end namespace polar

#endif // POLARPHP_NAME_LOOKUP_REQUESTS
