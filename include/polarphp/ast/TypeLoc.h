//===--- TypeLoc.h - Swift Language Type Locations --------------*- C++ -*-===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
//
// This file defines the TypeLoc struct and related structs.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_TYPE_LOC_H
#define POLARPHP_AST_TYPE_LOC_H

#include "polarphp/parser/SourceLoc.h"
#include "polarphp/ast/Type.h"
#include "polarphp/basic/adt/PointerIntPair.h"

namespace polar::ast {

using polar::parser::SourceLoc;
using polar::parser::SourceRange;

class AstContext;
class TypeRepr;

/// TypeLoc - Provides source location information for a parsed type.
/// A TypeLoc is stored in AST nodes which use an explicitly written type.
struct TypeLoc
{
public:
   TypeLoc() {}
   TypeLoc(TypeRepr *typeRepr)
      : m_typeRepr(typeRepr)
   {}

   TypeLoc(TypeRepr *typeRepr, Type type)
      : m_typeRepr(typeRepr)
   {
      setType(type);
   }

   bool wasValidated() const
   {
      return !m_type.isNull();
   }

   bool isError() const;

   // FIXME: We generally shouldn't need to build TypeLocs without a location.
   static TypeLoc withoutLoc(Type type)
   {
      TypeLoc result;
      result.m_type = type;
      return result;
   }

   /// Get the representative location of this type, for diagnostic
   /// purposes.
   /// This location is not necessarily the start location of the type repr.
   SourceLoc getLoc() const;
   SourceRange getSourceRange() const;

   bool hasLocation() const
   {
      return m_typeRepr != nullptr;
   }

   TypeRepr *getTypeRepr() const
   {
      return m_typeRepr;
   }

   Type getType() const
   {
      return m_type;
   }

   bool isNull() const
   {
      return getType().isNull() && m_typeRepr == nullptr;
   }

   void setInvalidType(AstContext &ctx);
   void setType(Type type);
   TypeLoc clone(AstContext &ctx) const;

private:
   Type m_type;
   TypeRepr *m_typeRepr = nullptr;
};

} // polar::ast

#endif // POLARPHP_AST_TYPE_LOC_H
