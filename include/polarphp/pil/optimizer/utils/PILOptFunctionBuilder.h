//===--- PILOptFunctionBuilder.h --------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_PILOPTFUNCTIONBUILDER_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_PILOPTFUNCTIONBUILDER_H

#include "polarphp/pil/lang/PILFunctionBuilder.h"
#include "polarphp/pil/optimizer/passmgr/PassManager.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

namespace polar {

class PILOptFunctionBuilder {
   PILTransform &transform;
   PILFunctionBuilder builder;

public:
   PILOptFunctionBuilder(PILTransform &transform)
      : transform(transform),
        builder(*transform.getPassManager()->getModule()) {}

   template <class... ArgTys>
   PILFunction *getOrCreateSharedFunction(ArgTys &&... args) {
      PILFunction *f =
         builder.getOrCreateSharedFunction(std::forward<ArgTys>(args)...);
      notifyAddFunction(f);
      return f;
   }

   template <class... ArgTys>
   PILFunction *getOrCreateFunction(ArgTys &&... args) {
      PILFunction *f = builder.getOrCreateFunction(std::forward<ArgTys>(args)...);
      notifyAddFunction(f);
      return f;
   }

   template <class... ArgTys> PILFunction *createFunction(ArgTys &&... args) {
      PILFunction *f = builder.createFunction(std::forward<ArgTys>(args)...);
      notifyAddFunction(f);
      return f;
   }

   void eraseFunction(PILFunction *f) {
      auto &pm = getPassManager();
      pm.notifyWillDeleteFunction(f);
      pm.getModule()->eraseFunction(f);
   }

   PILModule &getModule() const { return *getPassManager().getModule(); }

private:
   PILPassManager &getPassManager() const {
      return *transform.getPassManager();
   }

   void notifyAddFunction(PILFunction *f) {
      auto &pm = getPassManager();
      pm.notifyOfNewFunction(f, &transform);
      pm.notifyAnalysisOfFunction(f);
   }
};

} // namespace polar

#endif
