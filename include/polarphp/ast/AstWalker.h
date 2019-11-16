//===--- AstWalker.h - Class for walking the AST ----------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AST_WALKER_H
#define POLARPHP_AST_AST_WALKER_H

#include "llvm/ADTPointerUnion.h"
#include <utility>

namespace polar::ast {

enum class AccessKind: unsigned char;

enum class SemaReferenceKind : uint8_t
{
   ModuleRef = 0,
   DeclRef,
   DeclMemberRef,
   DeclConstructorRef,
   TypeRef,
   EnumElementRef,
   SubscriptRef,
};

struct ReferenceMetaData
{
   SemaReferenceKind kind;
   std::optional<AccessKind> accKind;
   ReferenceMetaData(SemaReferenceKind kind, std::optional<AccessKind> accKind)
      : kind(kind),
        accKind(accKind)
   {}
};

/// An abstract class used to traverse an AST.
class AstWalker
{
public:
   enum class ParentKind
   {
      Module, Decl, Stmt, Expr, TypeRepr
   };

   class ParentType
   {
      ParentKind kind;
      void *ptr = nullptr;

   public:

      bool isNull() const
      {
         return ptr == nullptr;
      }

      ParentKind getKind() const
      {
         assert(!isNull());
         return kind;
      }
   };

   /// The parent of the node we are visiting.
   ParentType Parent;
protected:
   AstWalker() = default;
   AstWalker(const AstWalker &) = default;
   virtual ~AstWalker() = default;
   virtual void anchor();
};

} // polar::ast

#endif // POLARPHP_AST_AST_WALKER_H
