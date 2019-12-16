//===--- PILGenFunctionBuilder.h ------------------------------------------===//
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

#ifndef POLARPHP_PIL_GEN_PILGEN_PILGENFUNCTIONBUILDER_H
#define POLARPHP_PIL_GEN_PILGEN_PILGENFUNCTIONBUILDER_H

#include "polarphp/pil/lang/PILFunctionBuilder.h"

namespace polar::lowering {

class LLVM_LIBRARY_VISIBILITY PILGenFunctionBuilder {
   PILFunctionBuilder builder;

   public:
   PILGenFunctionBuilder(PILGenModule &SGM) : builder(SGM.M) {}
   PILGenFunctionBuilder(PILGenFunction &SGF) : builder(SGF.SGM.M) {}

   template <class... ArgTys>
   PILFunction *getOrCreateSharedFunction(ArgTys &&... args) {
      return builder.getOrCreateSharedFunction(std::forward<ArgTys>(args)...);
   }

   template <class... ArgTys>
   PILFunction *getOrCreateFunction(ArgTys &&... args) {
      return builder.getOrCreateFunction(std::forward<ArgTys>(args)...);
   }

   template <class... ArgTys> PILFunction *createFunction(ArgTys &&... args) {
      return builder.createFunction(std::forward<ArgTys>(args)...);
   }
};

} // namespace polar::lowering

#endif // POLARPHP_PIL_GEN_PILGEN_PILGENFUNCTIONBUILDER_H
