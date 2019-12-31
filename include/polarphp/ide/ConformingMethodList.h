//===--- ConformingMethodList.h --- -----------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IDE_CONFORMINGMETHODLIST_H
#define POLARPHP_IDE_CONFORMINGMETHODLIST_H

#include "polarphp/ast/Type.h"
#include "polarphp/basic/LLVM.h"

namespace polar::llparser {
class CodeCompletionCallbacksFactory;
}

namespace polar::ide {

using polar::llparser::CodeCompletionCallbacksFactory;

/// A result item for context info query.
class ConformingMethodListResult {
public:
   /// The decl context of the parsed expression.
   DeclContext *DC;

   /// The resolved type of the expression.
   Type ExprType;

   /// Methods which satisfy the criteria.
   SmallVector<ValueDecl *, 0> Members;

   ConformingMethodListResult(DeclContext *DC, Type ExprType)
      : DC(DC), ExprType(ExprType) {}
};

/// An abstract base class for consumers of context info results.
class ConformingMethodListConsumer {
public:
   virtual ~ConformingMethodListConsumer() {}
   virtual void handleResult(const ConformingMethodListResult &result) = 0;
};

/// Printing consumer
class PrintingConformingMethodListConsumer
   : public ConformingMethodListConsumer {
   llvm::raw_ostream &OS;

public:
   PrintingConformingMethodListConsumer(llvm::raw_ostream &OS) : OS(OS) {}

   void handleResult(const ConformingMethodListResult &result) override;
};

/// Create a factory for code completion callbacks.
CodeCompletionCallbacksFactory *makeConformingMethodListCallbacksFactory(
   ArrayRef<const char *> expectedTypeNames,
   ConformingMethodListConsumer &Consumer);

} // polar::ide

#endif // POLARPHP_IDE_CONFORMINGMETHODLIST_H
