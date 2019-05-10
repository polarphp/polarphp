//===--- DeclContext.cpp - DeclContext implementation ---------------------===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//

#include "polarphp/ast/DeclContext.h"
//#include "polarphp/AST/AccessScope.h"
#include "polarphp/AST/AstContext.h"
#include "polarphp/AST/AstWalker.h"
#include "polarphp/AST/Expr.h"
#include "polarphp/AST/Types.h"
#include "polarphp/parser/SourceMgr.h"
#include "polarphp/basic/LangStatistic.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/Statistic.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/SaveAndRestore.h"

#define DEBUG_TYPE "Name lookup"

STATISTIC(NumLazyIterableDeclContexts,
          "# of serialized iterable declaration contexts");
STATISTIC(NumUnloadedLazyIterableDeclContexts,
          "# of serialized iterable declaration contexts never loaded");

namespace polar::ast {

// Only allow allocation of DeclContext using the allocator in AstContext.
void *DeclContext::operator new(size_t Bytes, AstContext &C,
                                unsigned Alignment)
{
   return nullptr;
}

AstContext &DeclContext::getAstContext() const
{
}

GenericTypeDecl *DeclContext::getSelfTypeDecl() const
{
}

/// If this DeclContext is a NominalType declaration or an
/// extension thereof, return the NominalTypeDecl.
NominalTypeDecl *DeclContext::getSelfNominalTypeDecl() const
{
}


ClassDecl *DeclContext::getSelfClassDecl() const
{
}

EnumDecl *DeclContext::getSelfEnumDecl() const
{
}

StructDecl *DeclContext::getSelfStructDecl() const
{
}

ProtocolDecl *DeclContext::getSelfProtocolDecl() const
{
}

ProtocolDecl *DeclContext::getExtendedProtocolDecl() const
{
   return nullptr;
}

GenericTypeParamType *DeclContext::getProtocolSelfType() const
{
   assert(getSelfProtocolDecl() && "not a protocol");
}

Type DeclContext::getDeclaredTypeInContext() const
{
   return Type();
}

Type DeclContext::getDeclaredInterfaceType() const
{
   return Type();
}

void DeclContext::forEachGenericContext(
      FunctionRef<void (GenericParamList *)> fn) const
{
}

unsigned DeclContext::getGenericContextDepth() const
{
}

GenericSignature *DeclContext::getGenericSignatureOfContext() const
{
   return nullptr;
}

GenericEnvironment *DeclContext::getGenericEnvironmentOfContext() const
{
   return nullptr;
}

bool DeclContext::contextHasLazyGenericEnvironment() const
{
   return false;
}

Type DeclContext::mapTypeIntoContext(Type type) const
{

}

DeclContext *DeclContext::getLocalContext()
{
}

AbstractFunctionDecl *DeclContext::getInnermostMethodContext()
{
   return nullptr;
}

bool DeclContext::isTypeContext() const
{
   return false;
}

DeclContext *DeclContext::getInnermostTypeContext()
{
   return nullptr;
}

Decl *DeclContext::getInnermostDeclarationDeclContext()
{
   return nullptr;
}

DeclContext *DeclContext::getParentForLookup() const
{
   return nullptr;
}

ModuleDecl *DeclContext::getParentModule() const
{
   return nullptr;
}

SourceFile *DeclContext::getParentSourceFile() const
{
   return nullptr;
}

DeclContext *DeclContext::getModuleScopeContext() const
{
   return nullptr;
}

/// Determine whether the given context is generic at any level.
bool DeclContext::isGenericContext() const
{

   return false;
}

/// Determine whether the innermost context is generic.
bool DeclContext::isInnermostContextGeneric() const
{

}

bool
DeclContext::isCascadingContextForLookup(bool functionsAreNonCascading) const
{

}

unsigned DeclContext::getSyntacticDepth() const
{

}

unsigned DeclContext::getSemanticDepth() const
{
}

bool DeclContext::walkContext(AstWalker &Walker)
{
   polar_unreachable("bad DeclContextKind");
}

void DeclContext::dumpContext() const
{
}


template <typename DCType>
static unsigned getLineNumber(DCType *DC)
{
}

unsigned DeclContext::printContext(RawOutStream &OS, const unsigned indent,
                                   const bool onlyAPartialLine) const
{
}

const Decl *
IterableDeclContext::getDecl() const
{
   polar_unreachable("Unhandled IterableDeclContextKind in switch.");
}

AstContext &IterableDeclContext::getAstContext() const
{
}

DeclRange IterableDeclContext::getMembers() const
{
}

/// Add a member to this context.
void IterableDeclContext::addMember(Decl *member, Decl *Hint)
{
}

void IterableDeclContext::addMemberSilently(Decl *member, Decl *hint) const
{
}

void IterableDeclContext::setMemberLoader(LazyMemberLoader *loader,
                                          uint64_t contextData)
{
}

void IterableDeclContext::loadAllMembers() const
{

}

bool IterableDeclContext::wasDeserialized() const
{
   return false;
}

bool IterableDeclContext::classof(const Decl *D)
{
   return false;
}

IterableDeclContext *
IterableDeclContext::castDeclToIterableDeclContext(const Decl *D)
{
   return nullptr;
}

/// Return the DeclContext to compare when checking private access in
/// Swift 4 mode. The context returned is the type declaration if the context
/// and the type declaration are in the same file, otherwise it is the types
/// last extension in the source file. If the context does not refer to a
/// declaration or extension, the supplied context is returned.
static const DeclContext *
getPrivateDeclContext(const DeclContext *DC, const SourceFile *useSF)
{
   return nullptr;
}


DeclContextKind DeclContext::getContextKind() const
{
   //  switch (ParentAndKind.getInt()) {
   //  case AstHierarchy::Expr:
   //    return DeclContextKind::AbstractClosureExpr;
   //  case AstHierarchy::Initializer:
   //    return DeclContextKind::Initializer;
   //  case AstHierarchy::SerializedLocal:
   //    return DeclContextKind::SerializedLocal;
   //  case AstHierarchy::FileUnit:
   //    return DeclContextKind::FileUnit;
   //  case AstHierarchy::Decl: {
   //    auto decl = reinterpret_cast<const Decl*>(this + 1);
   //    if (isa<AbstractFunctionDecl>(decl))
   //      return DeclContextKind::AbstractFunctionDecl;
   //    if (isa<GenericTypeDecl>(decl))
   //      return DeclContextKind::GenericTypeDecl;
   //    switch (decl->getKind()) {
   //    case DeclKind::Module:
   //      return DeclContextKind::Module;
   //    case DeclKind::TopLevelCode:
   //      return DeclContextKind::TopLevelCodeDecl;
   //    case DeclKind::Subscript:
   //      return DeclContextKind::SubscriptDecl;
   //    case DeclKind::EnumElement:
   //      return DeclContextKind::EnumElementDecl;
   //    case DeclKind::Extension:
   //      return DeclContextKind::ExtensionDecl;
   //    default:
   //      polar_unreachable("Unhandled Decl kind");
   //    }
   //  }
   //  }
   //  polar_unreachable("Unhandled DeclContext AstHierarchy");
}

//#define DECL(Id, Parent) \
//  static_assert(!std::is_base_of<DeclContext, Id##Decl>::value, \
//                "Non-context Decl node has context?");
//#define CONTEXT_DECL(Id, Parent) \
//  static_assert(alignof(DeclContext) == alignof(Id##Decl), "Alignment error"); \
//  static_assert(std::is_base_of<DeclContext, Id##Decl>::value, \
//                "CONTEXT_DECL nodes must inherit from DeclContext");
//#define CONTEXT_VALUE_DECL(Id, Parent) \
//  static_assert(alignof(DeclContext) == alignof(Id##Decl), "Alignment error"); \
//  static_assert(std::is_base_of<DeclContext, Id##Decl>::value, \
//                "CONTEXT_VALUE_DECL nodes must inherit from DeclContext");
//#include "swift/AST/DeclNodes.def"

//#define EXPR(Id, Parent) \
//  static_assert(!std::is_base_of<DeclContext, Id##Expr>::value, \
//                "Non-context Expr node has context?");
//#define CONTEXT_EXPR(Id, Parent) \
//  static_assert(alignof(DeclContext) == alignof(Id##Expr), "Alignment error"); \
//  static_assert(std::is_base_of<DeclContext, Id##Expr>::value, \
//                "CONTEXT_EXPR nodes must inherit from DeclContext");
//#include "swift/AST/ExprNodes.def"

} // polar::ast
