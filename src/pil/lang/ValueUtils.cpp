//===--- ValueUtils.cpp ---------------------------------------------------===//
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

#include "polarphp/pil/lang/ValueUtils.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/pil/lang/PILFunction.h"

using namespace polar;

Optional<ValueOwnershipKind>
polar::mergePILValueOwnership(ArrayRef<PILValue> values) {
   auto range = makeTransformRange(values,
                                   [](PILValue v) {
                                      assert(v->getType().isObject());
                                      return v.getOwnershipKind();
                                   });
   return ValueOwnershipKind::merge(range);
}
