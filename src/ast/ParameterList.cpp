//===--- Parameter.cpp - Functions & closures parameters ------------------===//
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
// This file defines the Parameter class, the ParameterList class and support
// logic.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Types.h"

namespace polar::ast {

/// TODO: unique and reuse the () parameter list in AstContext, it is common to
/// many methods.  Other parameter lists cannot be uniqued because the decls
/// within them are always different anyway (they have different DeclContext's).
ParameterList *
ParameterList::create(const AstContext &context, SourceLoc lparenLoc,
                      ArrayRef<ParamDecl*> params, SourceLoc rparenLoc)
{
//   assert(lparenLoc.isValid() == rparenLoc.isValid() &&
//          "Either both paren locs are valid or neither are");

//   auto byteSize = totalSizeToAlloc<ParamDecl *>(params.getSize());
//   auto rawMem = context.allocate(byteSize, alignof(ParameterList));

//   //  Placement initialize the ParameterList and the Parameter's.
//   auto params = ::new (rawMem) ParameterList(lparenLoc, params.size(), rparenLoc);
//   std::uninitialized_copy(params.begin(), params.end(), params->getArray().begin());
//   return params;
   return nullptr;
}

/// Change the DeclContext of any contained parameters to the specified
/// DeclContext.
void ParameterList::setDeclContextOfParamDecls(DeclContext *DC)
{
   for (auto P : *this)
      P->setDeclContext(DC);
}

/// Make a duplicate copy of this parameter list.  This allocates copies of
/// the ParamDecls, so they can be reparented into a new DeclContext.
ParameterList *ParameterList::clone(const AstContext &context,
                                    OptionSet<CloneFlags> options) const {
   // If this list is empty, don't actually bother with a copy.
//   if (size() == 0)
//      return const_cast<ParameterList*>(this);

//   SmallVector<ParamDecl*, 8> params(begin(), end());

//   // Remap the ParamDecls inside of the ParameterList.
//   bool withTypes = !options.contains(ParameterList::WithoutTypes);
//   for (auto &decl : params) {
//      bool hadDefaultArgument =
//            decl->getDefaultArgumentKind() == DefaultArgumentKind::Normal;

//      decl = new (context) ParamDecl(decl, withTypes);
//      if (options & Implicit)
//         decl->setImplicit();

//      // If the argument isn't named, and we're cloning for an inherited
//      // constructor, give the parameter a name so that silgen will produce a
//      // value for it.
//      if (decl->getName().empty() && (options & Inherited))
//         decl->setName(context.getIdentifier("argument"));

//      // If we're inheriting a default argument, mark it as such.
//      // FIXME: Figure out how to clone default arguments as well.
//      if (hadDefaultArgument) {
//         if (options & Inherited)
//            decl->setDefaultArgumentKind(DefaultArgumentKind::Inherited);
//         else
//            decl->setDefaultArgumentKind(DefaultArgumentKind::None);
//      }
//   }

//   return create(context, params);
   return nullptr;
}

//void ParameterList::getParams(
//      SmallVectorImpl<AnyFunctionType::Param> &params) const {
//   getParams(params,
//             [](ParamDecl *decl) { return decl->getInterfaceType(); });
//}

//void ParameterList::getParams(
//      SmallVectorImpl<AnyFunctionType::Param> &params,
//      FunctionRef<Type(ParamDecl *)> getType) const
//{
//   if (size() == 0)
//      return;

//   for (auto P : *this) {
//      auto type = getType(P);

//      if (P->isVariadic())
//         type = ParamDecl::getVarargBaseTy(type);

//      auto label = P->getArgumentName();
//      auto flags = ParameterTypeFlags::fromParameterType(type,
//                                                         P->isVariadic(),
//                                                         P->isAutoClosure(),
//                                                         P->getValueOwnership());
//      params.emplace_back(type, label, flags);
//   }
//}


///// Return the full source range of this parameter list.
SourceRange ParameterList::getSourceRange() const
{
   // If we have locations for the parens, then they define our range.
   if (lParenLoc.isValid()) {
      return { lParenLoc, rParenLoc };
   }
   // Otherwise, try the first and last parameter.
   if (size() != 0) {
      auto start = get(0)->getStartLoc();
      auto end = getArray().back()->getEndLoc();
      if (start.isValid() && end.isValid()) {
         return { start, end };
      }
   }
   return SourceRange();
}


} // polar::ast
