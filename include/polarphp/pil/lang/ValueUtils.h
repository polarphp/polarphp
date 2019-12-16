//===--- ValueUtils.h -----------------------------------------------------===//
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

#ifndef POLARPHP_PIL_VALUEUTILS_H
#define POLARPHP_PIL_VALUEUTILS_H

#include "polarphp/pil/lang/PILValue.h"

namespace polar {

/// Attempt to merge the ValueOwnershipKind of the passed in range's
/// PILValues. Returns Optional<None> if we found an incompatibility.
///
/// NOTE: This assumes that the passed in PILValues are not values used as type
/// dependent operands.
Optional<ValueOwnershipKind> mergePILValueOwnership(ArrayRef<PILValue> values);

} // namespace polar

#endif
