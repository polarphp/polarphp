//===--- PILUndef.h - PIL Undef Value Representation ------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_UNDEF_H
#define POLARPHP_PIL_UNDEF_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/pil/lang/PILValue.h"

namespace polar {

class PILArgument;
class PILInstruction;
class PILModule;

class PILUndef : public ValueBase {
  ValueOwnershipKind ownershipKind;

  PILUndef(PILType type, ValueOwnershipKind ownershipKind);

public:
  void operator=(const PILArgument &) = delete;
  void operator delete(void *, size_t) POLAR_DELETE_OPERATOR_DELETED;

  static PILUndef *get(PILType ty, PILModule &m, ValueOwnershipKind ownershipKind);
  static PILUndef *get(PILType ty, const PILFunction &f);

  template <class OwnerTy>
  static PILUndef *getSentinelValue(PILType type, OwnerTy owner) {
    // Ownership kind isn't used here, the value just needs to have a unique
    // address.
    return new (*owner) PILUndef(type, ValueOwnershipKind::None);
  }

  ValueOwnershipKind getOwnershipKind() const { return ownershipKind; }

  static bool classof(const PILArgument *) = delete;
  static bool classof(const PILInstruction *) = delete;
  static bool classof(const PILNode *node) {
    return node->getKind() == PILNodeKind::PILUndef;
  }
};

} // end polar namespace

#endif

