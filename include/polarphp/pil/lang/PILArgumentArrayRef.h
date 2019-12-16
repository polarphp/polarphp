//===--- PILArgumentArrayRef.h --------------------------------------------===//
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
///
/// \file
///
/// A header file to ensure that we do not create a dependency cycle in between
/// PILBasicBlock.h and PILInstruction.h.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILARGUMENTARRAYREF_H
#define POLARPHP_PIL_PILARGUMENTARRAYREF_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/StlExtras.h"

namespace polar {

using polar::TransformRange;

class PILArgument;

#define ARGUMENT(NAME, PARENT)                                                 \
  class NAME;                                                                  \
  using NAME##ArrayRef =                                                       \
      TransformRange<ArrayRef<PILArgument *>, NAME *(*)(PILArgument *)>;
#include "polarphp/pil/lang/PILNodesDef.h"

} // namespace polar

#endif // POLARPHP_PIL_PILARGUMENTARRAYREF_H
