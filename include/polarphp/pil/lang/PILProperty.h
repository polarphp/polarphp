//===--- PILProperty.h - Defines the PILProperty class ----------*- C++ -*-===//
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
// This file defines the PILProperty class, which is used to capture the
// metadata about a property definition necessary for it to be resiliently
// included in KeyPaths across modules.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILPROPERTY_H
#define POLARPHP_PIL_PILPROPERTY_H

#include "polarphp/ast/GenericSignature.h"
#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/ilist.h"

namespace polar {

class PILPrintContext;

/// A descriptor for a public property or subscript that can be resiliently
/// referenced from key paths in external modules.
class PILProperty : public llvm::ilist_node<PILProperty>,
                    public PILAllocated<PILProperty>
{
private:
   /// True if serialized.
   bool Serialized;

   /// The declaration the descriptor represents.
   AbstractStorageDecl *Decl;

   /// The key path component that represents its implementation.
   Optional<KeyPathPatternComponent> Component;

   PILProperty(bool Serialized,
               AbstractStorageDecl *Decl,
               Optional<KeyPathPatternComponent> Component)
      : Serialized(Serialized), Decl(Decl), Component(Component)
   {}

public:
   static PILProperty *create(PILModule &M,
                              bool Serialized,
                              AbstractStorageDecl *Decl,
                              Optional<KeyPathPatternComponent> Component);

   bool isSerialized() const { return Serialized; }

   AbstractStorageDecl *getDecl() const { return Decl; }

   bool isTrivial() const {
      return !Component.hasValue();
   }

   const Optional<KeyPathPatternComponent> &getComponent() const {
      return Component;
   }

   void print(PILPrintContext &Ctx) const;
   void dump() const;

   void verify(const PILModule &M) const;
};

} // end namespace polar

namespace llvm {

//===----------------------------------------------------------------------===//
// ilist_traits for PILProperty
//===----------------------------------------------------------------------===//

template <>
struct ilist_traits<::polar::PILProperty>
   : public ilist_node_traits<::polar::PILProperty> {
   using PILProperty = ::polar::PILProperty;

public:
   static void deleteNode(PILProperty *VT) { VT->~PILProperty(); }

private:
   void createNode(const PILProperty &);
};

} // namespace llvm

#endif
