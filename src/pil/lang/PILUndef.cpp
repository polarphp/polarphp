//===--- PILUndef.cpp -----------------------------------------------------===//
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

#include "polarphp/pil/lang/PILUndef.h"
#include "polarphp/pil/lang/PILModule.h"

using namespace polar;

static ValueOwnershipKind getOwnershipKindForUndef(PILType type, const PILFunction &f) {
   if (type.isAddress() || type.isTrivial(f))
      return ValueOwnershipKind::None;
   return ValueOwnershipKind::Owned;
}

PILUndef::PILUndef(PILType type, ValueOwnershipKind ownershipKind)
   : ValueBase(ValueKind::PILUndef, type, IsRepresentative::Yes),
     ownershipKind(ownershipKind) {}

PILUndef *PILUndef::get(PILType ty, PILModule &m, ValueOwnershipKind ownershipKind) {
   PILUndef *&entry = m.UndefValues[std::make_pair(ty, unsigned(ownershipKind))];
   if (entry == nullptr)
      entry = new (m) PILUndef(ty, ownershipKind);
   return entry;
}

PILUndef *PILUndef::get(PILType ty, const PILFunction &f) {
   auto ownershipKind = getOwnershipKindForUndef(ty, f);
   return PILUndef::get(ty, f.getModule(), ownershipKind);
}
