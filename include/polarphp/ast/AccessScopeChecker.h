//===--- AccessScopeChecker.h - Access calculation helpers -----*- C++ -*-===//
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
// Created by polarboy on 2019/04/30.
//===----------------------------------------------------------------------===//
//
//  This file defines helpers for access-control calculation.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ACCESS_SCOPE_CHECKER_H
#define POLARPHP_AST_ACCESS_SCOPE_CHECKER_H

#include "polarphp/ast/AccessScope.h"
#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/TypeWalker.h"

namespace polar::ast {

class AbstractStorageDecl;
class ExtensionDecl;
class SourceFile;
class ValueDecl;

class AccessScopeChecker
{
   const SourceFile *File;
   bool TreatUsableFromInlineAsPublic;

protected:
   AstContext &Context;
   std::optional<AccessScope> Scope = AccessScope::getPublic();

   AccessScopeChecker(const DeclContext *useDC,
                      bool treatUsableFromInlineAsPublic);
   bool visitDecl(ValueDecl *visitDecl);
};

class TypeReprAccessScopeChecker : private AstWalker, AccessScopeChecker {
   TypeReprAccessScopeChecker(const DeclContext *useDC,
                              bool treatUsableFromInlineAsPublic);

   bool walkToTypeReprPre(TypeRepr *typeRepr) override;
   bool walkToTypeReprPost(TypeRepr *typeRepr) override;

public:
   static std::optional<AccessScope>
   getAccessScope(TypeRepr *typeRepr, const DeclContext *useDC,
                  bool treatUsableFromInlineAsPublic = false);
};

class TypeAccessScopeChecker : private TypeWalker, AccessScopeChecker
{
   TypeAccessScopeChecker(const DeclContext *useDC,
                          bool treatUsableFromInlineAsPublic);

   Action walkToTypePre(Type type);

public:
   static std::optional<AccessScope>
   getAccessScope(Type T, const DeclContext *useDC,
                  bool treatUsableFromInlineAsPublic = false);
};


} // polar::ast

#endif // POLARPHP_AST_ACCESS_SCOPE_CHECKER_H
