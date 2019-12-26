//===--- PILParserFunctionBuilder.h ---------------------------------------===//
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

#ifndef POLARPHP_PIL_PARSER_INTERNAL_PILPARSERFUNCTIONBUILDER_H
#define POLARPHP_PIL_PARSER_INTERNAL_PILPARSERFUNCTIONBUILDER_H

#include "polarphp/pil/lang/PILFunctionBuilder.h"

namespace polar {

class LLVM_LIBRARY_VISIBILITY PILParserFunctionBuilder {
   PILFunctionBuilder builder;

public:
   PILParserFunctionBuilder(PILModule &mod) : builder(mod) {}

   PILFunction *createFunctionForForwardReference(StringRef name,
                                                  CanPILFunctionType ty,
                                                  PILLocation loc) {
      auto *result = builder.createFunction(
         PILLinkage::Private, name, ty, nullptr, loc, IsNotBare,
         IsNotTransparent, IsNotSerialized, IsNotDynamic);
      result->setDebugScope(new (builder.mod) PILDebugScope(loc, result));
      return result;
   }
};

} // namespace polar

#endif
