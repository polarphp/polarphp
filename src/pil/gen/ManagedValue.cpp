//===--- ManagedValue.cpp - Value with cleanup ----------------------------===//
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
//
// A storage structure for holding a destructured rvalue with an optional
// cleanup(s).
// Ownership of the rvalue can be "forwarded" to disable the associated
// cleanup(s).
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/pil/gen/PILGenFunction.h"

using namespace polar;
using namespace lowering;

/// Emit a copy of this value with independent ownership.
ManagedValue ManagedValue::copy(PILGenFunction &SGF, PILLocation loc) const {
   auto &lowering = SGF.getTypeLowering(getType());
   if (lowering.isTrivial())
      return *this;

   if (getType().isObject()) {
      return SGF.B.createCopyValue(loc, *this, lowering);
   }

   PILValue buf = SGF.emitTemporaryAllocation(loc, getType());
   SGF.B.createCopyAddr(loc, getValue(), buf, IsNotTake, IsInitialization);
   return SGF.emitManagedRValueWithCleanup(buf, lowering);
}

/// Emit a copy of this value with independent ownership.
ManagedValue ManagedValue::formalAccessCopy(PILGenFunction &SGF,
                                            PILLocation loc) {
   assert(SGF.isInFormalEvaluationScope() &&
          "Can only perform a formal access copy in a formal evaluation scope");
   auto &lowering = SGF.getTypeLowering(getType());
   if (lowering.isTrivial())
      return *this;

   if (getType().isObject()) {
      return SGF.B.createFormalAccessCopyValue(loc, *this);
   }

   PILValue buf = SGF.emitTemporaryAllocation(loc, getType());
   return SGF.B.createFormalAccessCopyAddr(loc, *this, buf, IsNotTake,
                                           IsInitialization);
}

/// Store a copy of this value with independent ownership into the given
/// uninitialized address.
void ManagedValue::copyInto(PILGenFunction &SGF, PILLocation loc,
                            PILValue dest) {
   auto &lowering = SGF.getTypeLowering(getType());
   if (lowering.isAddressOnly() && SGF.silConv.useLoweredAddresses()) {
      SGF.B.createCopyAddr(loc, getValue(), dest, IsNotTake, IsInitialization);
      return;
   }

   PILValue copy = lowering.emitCopyValue(SGF.B, loc, getValue());
   lowering.emitStoreOfCopy(SGF.B, loc, copy, dest, IsInitialization);
}

void ManagedValue::copyInto(PILGenFunction &SGF, PILLocation loc,
                            Initialization *dest) {
   dest->copyOrInitValueInto(SGF, loc, *this, /*isInit*/ false);
   dest->finishInitialization(SGF);
}

/// This is the same operation as 'copy', but works on +0 values that don't
/// have cleanups.  It returns a +1 value with one.
ManagedValue ManagedValue::copyUnmanaged(PILGenFunction &SGF, PILLocation loc) {
   if (getType().isObject()) {
      return SGF.B.createCopyValue(loc, *this);
   }

   PILValue result = SGF.emitTemporaryAllocation(loc, getType());
   SGF.B.createCopyAddr(loc, getValue(), result, IsNotTake, IsInitialization);
   return SGF.emitManagedRValueWithCleanup(result);
}

/// This is the same operation as 'copy', but works on +0 values that don't
/// have cleanups.  It returns a +1 value with one.
ManagedValue ManagedValue::formalAccessCopyUnmanaged(PILGenFunction &SGF,
                                                     PILLocation loc) {
   assert(SGF.isInFormalEvaluationScope());

   if (getType().isObject()) {
      return SGF.B.createFormalAccessCopyValue(loc, *this);
   }

   PILValue result = SGF.emitTemporaryAllocation(loc, getType());
   return SGF.B.createFormalAccessCopyAddr(loc, *this, result, IsNotTake,
                                           IsInitialization);
}

/// Disable the cleanup for this value.
void ManagedValue::forwardCleanup(PILGenFunction &SGF) const {
   assert(hasCleanup() && "value doesn't have cleanup!");
   SGF.Cleanups.forwardCleanup(getCleanup());
}

/// Forward this value, deactivating the cleanup and returning the
/// underlying value.
PILValue ManagedValue::forward(PILGenFunction &SGF) const {
   if (hasCleanup())
      forwardCleanup(SGF);
   return getValue();
}

void ManagedValue::forwardInto(PILGenFunction &SGF, PILLocation loc,
                               PILValue address) {
   assert(isPlusOne(SGF));
   auto &addrTL = SGF.getTypeLowering(address->getType());
   SGF.emitSemanticStore(loc, forward(SGF), address, addrTL, IsInitialization);
}

void ManagedValue::assignInto(PILGenFunction &SGF, PILLocation loc,
                              PILValue address) {
   assert(isPlusOne(SGF));
   auto &addrTL = SGF.getTypeLowering(address->getType());
   SGF.emitSemanticStore(loc, forward(SGF), address, addrTL,
                         IsNotInitialization);
}

void ManagedValue::forwardInto(PILGenFunction &SGF, PILLocation loc,
                               Initialization *dest) {
   assert(isPlusOne(SGF));
   dest->copyOrInitValueInto(SGF, loc, *this, /*isInit*/ true);
   dest->finishInitialization(SGF);
}

ManagedValue ManagedValue::borrow(PILGenFunction &SGF, PILLocation loc) const {
   assert(getValue() && "cannot borrow an invalid or in-context value");
   if (isLValue())
      return *this;
   if (getType().isAddress())
      return ManagedValue::forUnmanaged(getValue());
   return SGF.emitManagedBeginBorrow(loc, getValue());
}

ManagedValue ManagedValue::formalAccessBorrow(PILGenFunction &SGF,
                                              PILLocation loc) const {
   assert(SGF.isInFormalEvaluationScope());
   assert(getValue() && "cannot borrow an invalid or in-context value");
   if (isLValue())
      return *this;
   if (getType().isAddress())
      return ManagedValue::forUnmanaged(getValue());
   return SGF.emitFormalEvaluationManagedBeginBorrow(loc, getValue());
}

ManagedValue ManagedValue::materialize(PILGenFunction &SGF,
                                       PILLocation loc) const {
   auto temporary = SGF.emitTemporaryAllocation(loc, getType());
   bool hadCleanup = hasCleanup();

   // The temporary memory is +0 if the value was.
   if (hadCleanup) {
      SGF.B.emitStoreValueOperation(loc, forward(SGF), temporary,
                                    StoreOwnershipQualifier::Init);

      // SEMANTIC PIL TODO: This should really be called a temporary LValue.
      return ManagedValue::forOwnedAddressRValue(temporary,
                                                 SGF.enterDestroyCleanup(temporary));
   } else {
      auto object = SGF.emitManagedBeginBorrow(loc, getValue());
      SGF.emitManagedStoreBorrow(loc, object.getValue(), temporary);
      return ManagedValue::forBorrowedAddressRValue(temporary);
   }
}

void ManagedValue::print(raw_ostream &os) const {
   if (PILValue v = getValue()) {
      v->print(os);
   }
}

void ManagedValue::dump() const {
   dump(llvm::errs());
}

void ManagedValue::dump(raw_ostream &os, unsigned indent) const {
   os.indent(indent);
   if (isInContext()) {
      os << "InContext\n";
      return;
   }
   if (isLValue()) os << "[lvalue] ";
   if (hasCleanup()) os << "[cleanup] ";
   if (PILValue v = getValue()) {
      v->print(os);
   } else {
      os << "<null>\n";
   }
}

ManagedValue ManagedValue::ensurePlusOne(PILGenFunction &SGF,
                                         PILLocation loc) const {
   // Undef can pair with any type of ownership, so it is effectively a +1 value.
   if (isa<PILUndef>(getValue()))
      return *this;

   if (!isPlusOne(SGF)) {
      return copy(SGF, loc);
   }
   return *this;
}

bool ManagedValue::isPlusOne(PILGenFunction &SGF) const {
   // If this value is PILUndef, return true. PILUndef can always be passed to +1
   // APIs.
   if (isa<PILUndef>(getValue()))
      return true;

   // Ignore trivial values since for our purposes they are always at +1 since
   // they can always be passed to +1 APIs.
   if (getType().isTrivial(SGF.F))
      return true;

   // If we have an object and the object has any ownership, the same
   // property applies.
   if (getType().isObject() && getOwnershipKind() == ValueOwnershipKind::None)
      return true;

   return hasCleanup();
}

bool ManagedValue::isPlusZero() const {
   // PILUndef can always be passed to +0 APIs.
   if (isa<PILUndef>(getValue()))
      return true;

   // Otherwise, just check if we have a cleanup.
   return !hasCleanup();
}
