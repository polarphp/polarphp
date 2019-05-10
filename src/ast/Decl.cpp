//===--- Decl.cpp - Swift Language Decl ASTs ------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
//
//  This file implements the Decl class and subclasses.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Decl.h"

#include "polarphp/ast/Decl.h"
//#include "polarphp/ast/AccessRequests.h"
//#include "polarphp/ast/AccessScope.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/NameLookupRequests.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeLoc.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/Statistic.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/basic/Range.h"
#include "polarphp/basic/LangStatistic.h"
#include "polarphp/basic/CharInfo.h"

#include <algorithm>

namespace polar::ast {

// Only allow allocation of Decls using the allocator in ASTContext.
void *Decl::operator new(size_t Bytes, const AstContext &C,
                         unsigned Alignment)
{
   return nullptr;
}

StringRef Decl::getKindName(DeclKind K)
{
}

DescriptiveDeclKind Decl::getDescriptiveKind() const
{
   polar_unreachable("bad DescriptiveDeclKind");
}

StringRef Decl::getDescriptiveKindName(DescriptiveDeclKind K) {
   polar_unreachable("bad DescriptiveDeclKind");
}


DeclContext *Decl::getInnermostDeclContext() const
{
}

void Decl::setDeclContext(DeclContext *DC)
{
}

bool Decl::isUserAccessible() const
{
   return true;
}

bool Decl::canHaveComment() const
{
}

ModuleDecl *Decl::getModuleContext() const
{
}

/// Retrieve the diagnostic engine for diagnostics emission.
DiagnosticEngine &Decl::getDiags() const
{
}

SourceLoc Decl::getLoc() const
{
   polar_unreachable("Unknown decl kind");
}

bool Decl::isPrivateStdlibDecl(bool treatNonBuiltinProtocolsAsPublic) const
{
   return false;
}

bool Decl::isWeakImported(ModuleDecl *fromModule) const
{
   return false;
}

RawOutStream &operator<<(RawOutStream &OS,
                         StaticSpellingKind SSK)
{
   polar_unreachable("bad StaticSpellingKind");
}

SourceRange Decl::getSourceRange() const
{
   polar_unreachable("Unknown decl kind");
}

} // polar::ast
