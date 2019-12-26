//===--- IRGenPILPasses.h - The IRGen Prepare PIL Passes --------*- C++ -*-===//
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

#ifndef POLARPHP_IRGEN_PILPASSES_H
#define POLARPHP_IRGEN_PILPASSES_H

namespace polar {

class PILTransform;

namespace irgen {

/// Create a pass to hoist alloc_stack instructions with non-fixed size.
PILTransform *createAllocStackHoisting();
PILTransform *createLoadableByAddress();

} // end namespace irgen
} // end namespace polar

#endif // POLARPHP_IRGEN_PILPASSES_H