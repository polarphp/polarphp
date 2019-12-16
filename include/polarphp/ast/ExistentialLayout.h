//===--- ExistentialLayout.h - Existential type decomposition ---*- C++ -*-===//
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
//
// This file defines the ExistentialLayout struct.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_EXISTENTIAL_LAYOUT_H
#define POLARPHP_AST_EXISTENTIAL_LAYOUT_H

#include "polarphp/basic/ArrayRefView.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/Type.h"
#include "llvm/ADT/SmallVector.h"

namespace polar {

class InterfaceDecl;
class InterfaceType;
class InterfaceCompositionType;

struct ExistentialLayout {
   enum Kind { Class, Error, Opaque };

   ExistentialLayout() {
      hasExplicitAnyObject = false;
      containsNonObjCInterface = false;
      singleInterface = nullptr;
   }

   ExistentialLayout(InterfaceType *type);
   ExistentialLayout(InterfaceCompositionType *type);

   /// The explicit superclass constraint, if any.
   Type explicitSuperclass;

   /// Whether the existential contains an explicit '& AnyObject' constraint.
   bool hasExplicitAnyObject : 1;

   /// Whether any protocol members are non-@objc.
   bool containsNonObjCInterface : 1;

   /// Return the kind of this existential (class/error/opaque).
   Kind getKind() {
      if (requiresClass())
         return Kind::Class;
      if (isErrorExistential())
         return Kind::Error;

      // The logic here is that opaque is the complement of class + error,
      // i.e. we don't have more concrete information guiding the layout
      // and it doesn't fall into the special-case Error representation.
      return Kind::Opaque;
   }

   bool isAnyObject() const;

   bool isObjC() const {
      // FIXME: Does the superclass have to be @objc?
      return ((explicitSuperclass ||
               hasExplicitAnyObject ||
               !getInterfaces().empty()) &&
              !containsNonObjCInterface);
   }

   /// Whether the existential requires a class, either via an explicit
   /// '& AnyObject' member or because of a superclass or protocol constraint.
   bool requiresClass() const;

   /// Returns the existential's superclass, if any; this is either an explicit
   /// superclass term in a composition type, or the superclass of one of the
   /// interfaces.
   Type getSuperclass() const;

   /// Does this existential contain the Error protocol?
   bool isExistentialWithError(AstContext &ctx) const;

   /// Does this existential consist of an Error protocol only with no other
   /// constraints?
   bool isErrorExistential() const;

   static inline InterfaceType *getInterfaceType(const Type &Ty) {
      return cast<InterfaceType>(Ty.getPointer());
   }
   typedef ArrayRefView<Type,InterfaceType*,getInterfaceType> InterfaceTypeArrayRef;

   InterfaceTypeArrayRef getInterfaces() const & {
      if (singleInterface)
         return llvm::makeArrayRef(&singleInterface, 1);
      return interfaces;
   }
   /// The returned ArrayRef may point directly to \c this->singleInterface, so
   /// calling this on a temporary is likely to be incorrect.
   InterfaceTypeArrayRef getInterfaces() const && = delete;

   LayoutConstraint getLayoutConstraint() const;

private:
   // The interface from a InterfaceType
   Type singleInterface;

   /// Zero or more protocol constraints from a InterfaceCompositionType
   ArrayRef<Type> interfaces;
};

}

#endif  // POLARPHP_AST_EXISTENTIAL_LAYOUT_H
