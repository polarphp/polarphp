//===--- SerializationFunctionBuilder.h -----------------------------------===//
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

#ifndef POLARPHP_SERIALIZATION_SERIALIZATIONFUNCTIONBUILDER_H
#define POLARPHP_SERIALIZATION_SERIALIZATIONFUNCTIONBUILDER_H

#include "polarphp/pil/lang/PILFunctionBuilder.h"

namespace polar {

class LLVM_LIBRARY_VISIBILITY PILSerializationFunctionBuilder {
   PILFunctionBuilder builder;

   public:
   PILSerializationFunctionBuilder(PILModule &mod) : builder(mod) {}

   /// Create a PILFunction declaration for use either as a forward reference or
   /// for the eventual deserialization of a function body.
   PILFunction *createDeclaration(StringRef name, PILType type,
   PILLocation loc) {
      return builder.createFunction(
         PILLinkage::Private, name, type.getAs<PILFunctionType>(), nullptr,
         loc, IsNotBare, IsNotTransparent,
         IsNotSerialized, IsNotDynamic, ProfileCounter(), IsNotThunk,
         SubclassScope::NotApplicable);
   }

   void setHasOwnership(PILFunction *f, bool newValue) {
      builder.setHasOwnership(f, newValue);
   }
};

} // namespace polar

#endif
