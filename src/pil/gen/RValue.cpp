//===--- RValue.cpp - Exploded RValue Representation ----------------------===//
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

#include "polarphp/pil/gen/RValue.h"
#include "polarphp/pil/gen/Initialization.h"
#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/ast/CanTypeVisitor.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/pil/lang/AbstractionPattern.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/TypeLowering.h"

using namespace polar;
using namespace lowering;

//===----------------------------------------------------------------------===//
//                              Helper Routines
//===----------------------------------------------------------------------===//

static unsigned getTupleSize(CanType t) {
   if (auto tt = dyn_cast<TupleType>(t))
      return tt->getNumElements();
   return 1;
}

unsigned RValue::getRValueSize(AbstractionPattern pattern, CanType formalType) {
   if (pattern.isTuple()) {
      unsigned count = 0;
      auto formalTupleType = cast<TupleType>(formalType);
      for (auto i : indices(formalTupleType.getElementTypes())) {
         count += getRValueSize(pattern.getTupleElementType(i),
                                formalTupleType.getElementType(i));
      }
      return count;
   }

   return 1;
}

/// Return the number of rvalue elements in the given canonical type.
unsigned RValue::getRValueSize(CanType type) {
   if (auto tupleType = dyn_cast<TupleType>(type)) {
      unsigned count = 0;
      for (auto eltType : tupleType.getElementTypes())
         count += getRValueSize(eltType);
      return count;
   }

   return 1;
}

namespace {

class ExplodeTupleValue
   : public CanTypeVisitor<ExplodeTupleValue,
      /*RetTy=*/ void,
      /*Args...=*/ ManagedValue>
{
public:
   std::vector<ManagedValue> &values;
   PILGenFunction &SGF;
   PILLocation loc;

   ExplodeTupleValue(std::vector<ManagedValue> &values,
                     PILGenFunction &SGF, PILLocation loc)
      : values(values), SGF(SGF), loc(loc)
   {
   }

   void visitType(CanType formalType, ManagedValue v) {
      // If we have a loadable type that has not been loaded, actually load it.
      if (!v.getType().isObject() && v.getType().isLoadable(SGF.F)) {
         if (v.isPlusOne(SGF)) {
            v = SGF.B.createLoadTake(loc, v);
         } else {
            v = SGF.B.createLoadBorrow(loc, v);
         }
      }

      values.push_back(v);
   }

   void visitObjectTupleType(CanTupleType tupleFormalType, ManagedValue tuple) {
      // If we have an object, destructure the object using ownership APIs to
      // propagate cleanups.
      SGF.B.emitDestructureValueOperation(
         loc, tuple, [&](unsigned index, ManagedValue elt) {
            CanType eltFormalType = tupleFormalType.getElementType(index);
            assert(eltFormalType->isMaterializable());

            auto eltTy = tuple.getType().getTupleElementType(index);
            assert(eltTy.isAddress() == tuple.getType().isAddress());
            auto &eltTI = SGF.getTypeLowering(eltTy);
            (void)eltTI;
            assert(eltTI.isLoadable() || !SGF.silConv.useLoweredAddresses());

            // Project the element.
            visit(eltFormalType, elt);
         });
   }

   void visitAddressTupleType(CanTupleType tupleFormalType, ManagedValue tuple) {
      bool isPlusOne = tuple.isPlusOne(SGF);

      for (unsigned i : indices(tupleFormalType->getElements())) {
         CanType eltFormalType = tupleFormalType.getElementType(i);
         assert(eltFormalType->isMaterializable());

         auto eltTy = tuple.getType().getTupleElementType(i);
         assert(eltTy.isAddress() == tuple.getType().isAddress());
         auto &eltTI = SGF.getTypeLowering(eltTy);

         // Project the element.
         ManagedValue elt = SGF.B.createTupleElementAddr(loc, tuple, i, eltTy);

         // RValue has an invariant that loadable values have been loaded. Except
         // it's not really an invariant, because argument emission likes to lie
         // sometimes.
         if (eltTI.isLoadable()) {
            if (isPlusOne) {
               elt = SGF.B.createLoadTake(loc, elt);
            } else {
               elt = SGF.B.createLoadBorrow(loc, elt);
            }
         } else {
            // In contrast if we have an address only type, we can not rely on
            // ownership APIs to help us. So, manually create a cleanup to make up
            // for the cleanup that we will forward on the tuple.
            if (isPlusOne)
               elt = SGF.emitManagedRValueWithCleanup(elt.getValue(), eltTI);
         }

         visit(eltFormalType, elt);
      }

      // Then forward the underlying tuple's cleanup since we have appropriately
      // pushed its cleanups onto its subcomponents.
      tuple.forward(SGF);
   }

   void visitTupleType(CanTupleType tupleFormalType, ManagedValue tuple) {
      if (tuple.getType().isObject()) {
         return visitObjectTupleType(tupleFormalType, tuple);
      }

      visitAddressTupleType(tupleFormalType, tuple);
   }
};

enum class ImplodeKind { Unmanaged, Forward, Copy };

template <ImplodeKind KIND>
class ImplodeLoadableTupleValue
   : public CanTypeVisitor<ImplodeLoadableTupleValue<KIND>,
      /*RetTy=*/ManagedValue,
      /*Args...=*/PILLocation> {
public:
   ArrayRef<ManagedValue> values;
   PILGenFunction &SGF;

   static ManagedValue getValue(PILGenFunction &SGF, ManagedValue v,
                                PILLocation l) {
      switch (KIND) {
         case ImplodeKind::Unmanaged:
            assert(!v.hasCleanup());
            return v.unmanagedBorrow();
         case ImplodeKind::Forward:
            return v.ensurePlusOne(SGF, l);
         case ImplodeKind::Copy:
            return v.copy(SGF, l);
      }

      llvm_unreachable("Unhandled ImplodeKind in switch.");
   }

   ImplodeLoadableTupleValue(ArrayRef<ManagedValue> values,
                             PILGenFunction &SGF)
      : values(values), SGF(SGF)
   {}

   ManagedValue visitType(CanType t, PILLocation l) {
      ManagedValue result = getValue(SGF, values[0], l);
      values = values.slice(1);
      return result;
   }

   ManagedValue visitTupleType(CanTupleType t, PILLocation l) {
      SmallVector<ManagedValue, 4> elts;
      for (auto fieldTy : t.getElementTypes())
         elts.push_back(this->visit(fieldTy, l));
      PILType ty = SGF.getLoweredLoadableType(t);
      return SGF.B.createTuple(l, ty, elts);
   }

   ~ImplodeLoadableTupleValue() {
   }
};

template <ImplodeKind KIND>
class ImplodeAddressOnlyTuple
   : public CanTypeVisitor<ImplodeAddressOnlyTuple<KIND>,
      /*RetTy=*/void,
      /*Args...=*/Initialization *, PILLocation> {
public:
   ArrayRef<ManagedValue> values;
   PILGenFunction &SGF;

   ImplodeAddressOnlyTuple(ArrayRef<ManagedValue> values,
                           PILGenFunction &SGF)
      : values(values), SGF(SGF)
   {}

   void visitType(CanType t, Initialization *address, PILLocation l) {
      ManagedValue v = values[0];
      switch (KIND) {
         case ImplodeKind::Unmanaged:
            llvm_unreachable("address-only types always managed!");

         case ImplodeKind::Forward:
            // If a value is forwarded into, we require the value to be at +1. If the
            // the value is already at +1, we just forward. Otherwise, we perform the
            // copy.
            address->copyOrInitValueInto(SGF, l, v.ensurePlusOne(SGF, l),
                                         true /*isInit*/);
            break;

         case ImplodeKind::Copy:
            address->copyOrInitValueInto(SGF, l, v, false /*isInit*/);
            break;
      }

      address->finishInitialization(SGF);
      values = values.slice(1);
   }

   void visitTupleType(CanTupleType t, Initialization *address, PILLocation l) {
      assert(address->canSplitIntoTupleElements());
      llvm::SmallVector<InitializationPtr, 4> buf;
      auto bufResult = address->splitIntoTupleElements(SGF, l, t, buf);

      for (unsigned i : range(t->getNumElements())) {
         CanType fieldCanTy = t.getElementType(i);
         this->visit(fieldCanTy, bufResult[i].get(), l);
      }

      address->finishInitialization(SGF);
   }

   ~ImplodeAddressOnlyTuple() {
      assert(values.empty() && "values not exhausted imploding tuple?!");
   }
};

} // end anonymous namespace

template <ImplodeKind KIND>
static ManagedValue implodeTupleValues(ArrayRef<ManagedValue> values,
                                       PILGenFunction &SGF, CanType tupleType,
                                       PILLocation l) {
   // Non-tuples don't need to be imploded.
   if (!isa<TupleType>(tupleType)) {
      assert(values.size() == 1 && "exploded non-tuple value?!");
      return ImplodeLoadableTupleValue<KIND>::getValue(SGF, values[0], l);
   }

   const auto &TL = SGF.getTypeLowering(tupleType);

   // To implode an address-only tuple, we need to create a buffer to hold the
   // result tuple.
   if (TL.isAddressOnly() && SGF.silConv.useLoweredAddresses()) {
      assert(KIND != ImplodeKind::Unmanaged &&
             "address-only values are always managed!");
      auto buffer = SGF.emitTemporary(l, TL);
      ImplodeAddressOnlyTuple<KIND>(values, SGF)
         .visit(tupleType, buffer.get(), l);
      return buffer->getManagedAddress();
   }

   // To implode loadable tuples, we just need to combine the elements with
   // TupleInsts.
   return ImplodeLoadableTupleValue<KIND>(values, SGF).visit(tupleType, l);
}

/// Perform a copy or init operation from an array of ManagedValue (from an
/// RValue) into an initialization. The RValue will have one scalar ManagedValue
/// for each exploded tuple element in the RValue, so this needs to make the
/// shape of the initialization match the available elements.  This can be done
/// one of two ways:
///
///  1) recursively scalarize down the initialization on demand if the type of
///     the RValue is tuple type and the initialization supports it.
///  2) implode the corresponding values in the RValue to a scalar value of
///     tuple type and process them as a unit.
///
/// We prefer to use approach #1 since it generates better code.
///
template <ImplodeKind KIND>
static void copyOrInitValuesInto(Initialization *init,
                                 ArrayRef<ManagedValue> &values, CanType type,
                                 PILLocation loc, PILGenFunction &SGF) {
   static_assert(KIND == ImplodeKind::Forward ||
                 KIND == ImplodeKind::Copy, "Not handled by init");
   bool isInit = (KIND == ImplodeKind::Forward);

   // If the element has non-tuple type, just serve it up to the initialization.
   auto tupleType = dyn_cast<TupleType>(type);
   if (!tupleType) {
      // We take the first value.
      ManagedValue result = values[0];
      values = values.slice(1);
      init->copyOrInitValueInto(SGF, loc, result, isInit);
      init->finishInitialization(SGF);
      return;
   }

   bool implodeTuple = false;

   if (init->canPerformInPlaceInitialization() &&
       init->isInPlaceInitializationOfGlobal() &&
       SGF.getTypeLowering(type).isTrivial()) {
      // Implode tuples in initialization of globals if they are
      // of trivial types.
      implodeTuple = true;
   }

   // If we can satisfy the tuple type by breaking up the aggregate
   // initialization, do so.
   if (!implodeTuple && init->canSplitIntoTupleElements()) {
      SmallVector<InitializationPtr, 4> subInitBuf;
      auto subInits = init->splitIntoTupleElements(SGF, loc, type, subInitBuf);

      assert(subInits.size() == tupleType->getNumElements() &&
             "initialization does not match tuple?!");

      for (unsigned i = 0, e = subInits.size(); i < e; ++i)
         copyOrInitValuesInto<KIND>(subInits[i].get(), values,
                                    tupleType.getElementType(i), loc, SGF);

      init->finishInitialization(SGF);
      return;
   }

   // Otherwise, process this by turning the values corresponding to the tuple
   // into a single value (through an implosion) and then binding that value to
   // our initialization.
   ManagedValue scalar = implodeTupleValues<KIND>(values, SGF, type, loc);

   // This will have just used up the first values in the list, pop them off.
   values = values.slice(RValue::getRValueSize(type));

   init->copyOrInitValueInto(SGF, loc, scalar, isInit);
   init->finishInitialization(SGF);
}

LLVM_ATTRIBUTE_UNUSED
static unsigned
expectedExplosionSize(CanType type) {
   auto tuple = dyn_cast<TupleType>(type);
   if (!tuple)
      return 1;
   unsigned total = 0;
   for (unsigned i = 0; i < tuple->getNumElements(); ++i) {
      total += expectedExplosionSize(tuple.getElementType(i));
   }
   return total;
}

/// This is separate from the main verification routine, so I can minimize the
/// amount of places that need to use PILGenFunction &SGF.
static void verifyHelper(ArrayRef<ManagedValue> values,
                         NullablePtr<PILGenFunction> SGF = nullptr) {
// This is a no-op in non-assert builds.
#ifndef NDEBUG
   auto result = Optional<ValueOwnershipKind>(ValueOwnershipKind::None);
   Optional<bool> sameHaveCleanups;
   for (ManagedValue v : values) {
      assert((!SGF || !v.getType().isLoadable(SGF.get()->F) ||
              v.getType().isObject()) &&
             "All loadable values in an RValue must be an object");

      ValueOwnershipKind kind = v.getOwnershipKind();
      if (kind == ValueOwnershipKind::None)
         continue;

      // Merge together whether or not the RValue has cleanups.
      if (!sameHaveCleanups.hasValue()) {
         sameHaveCleanups = v.hasCleanup();
      } else {
         assert(*sameHaveCleanups == v.hasCleanup());
      }

      // This variable is here so that if the assert below fires, the current
      // reduction value is still available.
      auto newResult = result.getValue().merge(kind);
      assert(newResult.hasValue());
      result = newResult;
   }
#endif
}

//===----------------------------------------------------------------------===//
//                           RValue Implementation
//===----------------------------------------------------------------------===//

// Private helper constructor. Please see RValue.h for more information.
RValue::RValue(PILGenFunction *SGF, ArrayRef<ManagedValue> values, CanType type)
   : values(values.begin(), values.end()), type(type), elementsToBeAdded(0) {

   assert(values.size() == expectedExplosionSize(type)
          && "creating rvalue with wrong number of pre-exploded elements");

   if (values.size() == 1 && values[0].isInContext()) {
      values = ArrayRef<ManagedValue>();
      type = CanType();
      elementsToBeAdded = InContext;
      return;
   }

   verifyHelper(values, SGF);
}

RValue::RValue(PILGenFunction &SGF, PILLocation l, CanType formalType,
               ManagedValue v)
   : type(formalType), elementsToBeAdded(0)
{
   assert(v && "creating r-value with consumed value");

   if (v.isInContext()) {
      type = CanType();
      elementsToBeAdded = InContext;
      return;
   }

   ExplodeTupleValue(values, SGF, l).visit(formalType, v);
   assert(values.size() == getRValueSize(type));
   verify(SGF);
}

RValue::RValue(PILGenFunction &SGF, Expr *expr, ManagedValue v)
   : type(expr->getType()->getCanonicalType()), elementsToBeAdded(0) {

   if (v.isInContext()) {
      type = CanType();
      elementsToBeAdded = InContext;
      return;
   }

   assert(v && "creating r-value with consumed value");
   ExplodeTupleValue(values, SGF, expr).visit(type, v);
   assert(values.size() == getRValueSize(type));
   verify(SGF);
}

RValue::RValue(CanType type)
   : type(type), elementsToBeAdded(getTupleSize(type)) {
}

RValue::RValue(AbstractionPattern pattern, CanType type)
   : type(type), elementsToBeAdded(getRValueSize(pattern, type)) {
}

void RValue::addElement(RValue &&element) & {
   assert(!element.isUsed() && "adding consumed value to r-value");
   assert(!element.isInSpecialState() && "adding special value to r-value");
   assert(!isComplete() && "rvalue already complete");
   assert(!isInSpecialState() && "cannot add elements to a special r-value");
   --elementsToBeAdded;
   values.insert(values.end(),
                 element.values.begin(), element.values.end());
   element.makeUsed();

   assert(!isComplete() || values.size() == getRValueSize(type));
   // Call into the verifier helper directly without an SGF since we know that
   // all of our loadable values are already loaded and thus we do not need to
   // recheck that. On the other hand, we need to check the consistency of
   // cleanups and ownership.
   verifyHelper(values);
}

void RValue::addElement(PILGenFunction &SGF, ManagedValue element,
                        CanType formalType, PILLocation l) & {
   assert(element && "adding consumed value to r-value");
   assert(!element.isInContext() && "adding in-context value to r-value");
   assert(!isComplete() && "rvalue already complete");
   assert(!isInSpecialState() && "cannot add elements to an in-context r-value");
   --elementsToBeAdded;

   ExplodeTupleValue(values, SGF, l).visit(formalType, element);

   assert(!isComplete() || values.size() == getRValueSize(type));
   verify(SGF);
}

PILValue RValue::forwardAsSingleValue(PILGenFunction &SGF, PILLocation l) && {
   assert(isComplete() && "rvalue is not complete");
   assert(!isUsed() && "rvalue was used?!");
   ManagedValue mv = std::move(*this).getAsSingleValue(SGF, l);
   makeUsed();
   return mv.forward(SGF);
}

PILValue RValue::forwardAsSingleStorageValue(PILGenFunction &SGF,
                                             PILType storageType,
                                             PILLocation l) && {
   assert(isComplete() && "rvalue is not complete");
   // Conversions must always be done at +1.
   PILValue result =
      std::move(*this).ensurePlusOne(SGF, l).forwardAsSingleValue(SGF, l);
   return SGF.emitConversionFromSemanticValue(l, result, storageType);
}

void RValue::forwardInto(PILGenFunction &SGF, PILLocation loc,
                         Initialization *I) && {
   assert(isComplete() && "rvalue is not complete");
   assert(isPlusOne(SGF) && "Can not forward borrowed RValues");
   ArrayRef<ManagedValue> elts = values;
   copyOrInitValuesInto<ImplodeKind::Forward>(I, elts, type, loc, SGF);
}

void RValue::copyInto(PILGenFunction &SGF, PILLocation loc,
                      Initialization *I) const & {
   assert(isComplete() && "rvalue is not complete");
   ArrayRef<ManagedValue> elts = values;
   copyOrInitValuesInto<ImplodeKind::Copy>(I, elts, type, loc, SGF);
}

static void assignRecursive(PILGenFunction &SGF, PILLocation loc,
                            CanType type, ArrayRef<ManagedValue> &srcValues,
                            PILValue destAddr) {
   // Recurse into tuples.
   if (auto srcTupleType = dyn_cast<TupleType>(type)) {
      assert(destAddr->getType().castTo<TupleType>()->getNumElements()
             == srcTupleType->getNumElements());
      for (auto eltIndex : indices(srcTupleType.getElementTypes())) {
         auto eltDestAddr = SGF.B.createTupleElementAddr(loc, destAddr, eltIndex);
         assignRecursive(SGF, loc, srcTupleType.getElementType(eltIndex),
                         srcValues, eltDestAddr);
      }
      return;
   }

   // Otherwise, pull the front value off the list.
   auto srcValue = srcValues.front();
   srcValues = srcValues.slice(1);

   srcValue.assignInto(SGF, loc, destAddr);
}

void RValue::assignInto(PILGenFunction &SGF, PILLocation loc,
                        PILValue destAddr) && {
   assert(isComplete() && "rvalue is not complete");
   assert(isPlusOne(SGF) && "Can not assign borrowed RValues");
   ArrayRef<ManagedValue> srcValues = values;
   assignRecursive(SGF, loc, type, srcValues, destAddr);
   assert(srcValues.empty() && "didn't claim all elements!");
}

ManagedValue RValue::getAsSingleValue(PILGenFunction &SGF, PILLocation loc) && {
   assert(!isUsed() && "r-value already used");
   POLAR_DEFER {
                  makeUsed();
               };

   if (isInContext()) {
      return ManagedValue::forInContext();
   }

   // Avoid killing and re-emitting the cleanup if the enclosed value isn't a
   // tuple.
   if (!isa<TupleType>(type)) {
      assert(values.size() == 1 && "exploded non-tuple?!");
      return values[0];
   }

   // *NOTE* Inside implodeTupleValues, we copy our values if they are not at +1.
   return implodeTupleValues<ImplodeKind::Forward>(values, SGF, type, loc);
}

PILValue RValue::getUnmanagedSingleValue(PILGenFunction &SGF,
                                         PILLocation l) const & {
   assert(isComplete() && "rvalue is not complete");
   ManagedValue mv =
      implodeTupleValues<ImplodeKind::Unmanaged>(values, SGF, type, l);
   return mv.getValue();
}

void RValue::forwardAll(PILGenFunction &SGF,
                        SmallVectorImpl<PILValue> &dest) && {
   assert(isComplete() && "rvalue is not complete");

   for (auto value : values)
      dest.push_back(value.forward(SGF));

   makeUsed();
}

void RValue::getAll(SmallVectorImpl<ManagedValue> &dest) && {
   assert(isComplete() && "rvalue is not complete");

   dest.append(values.begin(), values.end());
   makeUsed();
}

void RValue::getAllUnmanaged(SmallVectorImpl<PILValue> &dest) const & {
   assert(isComplete() && "rvalue is not complete");

   for (auto value : values)
      dest.push_back(value.getUnmanagedValue());
}

/// Return the range of indexes for the given tuple type element.
static std::pair<unsigned,unsigned>
getElementRange(CanTupleType tupleType, unsigned eltIndex) {
   assert(eltIndex < tupleType->getNumElements());
   unsigned begin = 0;
   for (unsigned i = 0; i < eltIndex; ++i) {
      begin += RValue::getRValueSize(tupleType.getElementType(i));
   }
   unsigned end =
      begin + RValue::getRValueSize(tupleType.getElementType(eltIndex));
   return { begin, end };
}

RValue RValue::extractElement(unsigned n) && {
   assert(isComplete() && "rvalue is not complete");

   CanTupleType tupleTy = dyn_cast<TupleType>(type);
   if (!tupleTy) {
      assert(n == 0);
      unsigned to = getRValueSize(type);
      assert(to == values.size());
      RValue element(nullptr, llvm::makeArrayRef(values).slice(0, to), type);
      makeUsed();
      return element;
   }

   auto range = getElementRange(tupleTy, n);
   unsigned from = range.first, to = range.second;

   CanType eltType = cast<TupleType>(type).getElementType(n);
   RValue element(nullptr, llvm::makeArrayRef(values).slice(from, to - from), eltType);
   makeUsed();
   return element;
}

void RValue::extractElements(SmallVectorImpl<RValue> &elements) && {
   assert(isComplete() && "rvalue is not complete");

   CanTupleType tupleTy = dyn_cast<TupleType>(type);
   if (!tupleTy) {
      unsigned to = getRValueSize(type);
      assert(to == values.size());
      // We use push_back instead of emplace_back since emplace_back can not
      // invoke the private constructor we are attempting to invoke.
      elements.push_back({nullptr, llvm::makeArrayRef(values).slice(0, to), type});
      makeUsed();
      return;
   }

   unsigned from = 0;
   for (auto eltType : tupleTy.getElementTypes()) {
      unsigned to = from + getRValueSize(eltType);
      // We use push_back instead of emplace_back since emplace_back can not
      // invoke the private constructor we are attempting to invoke.
      elements.push_back({nullptr, llvm::makeArrayRef(values).slice(from, to - from),
                          eltType});
      from = to;
   }
   assert(from == values.size());

   makeUsed();
}

RValue RValue::copy(PILGenFunction &SGF, PILLocation loc) const & {
   assert((isComplete() || isInSpecialState()) &&
          "can't copy an incomplete rvalue");
   std::vector<ManagedValue> copiedValues;
   copiedValues.reserve(values.size());
   for (ManagedValue v : values) {
      copiedValues.emplace_back(v.copy(SGF, loc));
   }
   return RValue(SGF, std::move(copiedValues), type, elementsToBeAdded);
}

RValue RValue::ensurePlusOne(PILGenFunction &SGF, PILLocation loc) && {
   if (!isPlusOne(SGF))
      return copy(SGF, loc);
   return std::move(*this);
}

RValue RValue::borrow(PILGenFunction &SGF, PILLocation loc) const & {
   assert((isComplete() || isInSpecialState()) &&
          "can't borrow incomplete rvalue");
   std::vector<ManagedValue> borrowedValues;
   borrowedValues.reserve(values.size());
   for (ManagedValue v : values) {
      borrowedValues.emplace_back(v.borrow(SGF, loc));
   }
   return RValue(SGF, std::move(borrowedValues), type, elementsToBeAdded);
}

ManagedValue RValue::materialize(PILGenFunction &SGF, PILLocation loc) && {
   assert(isPlusOne(SGF) && "Can not materialize a non-plus one RValue");
   auto &paramTL = SGF.getTypeLowering(getType());

   // If we're already materialized, we're done.
   if (values.size() == 1 &&
       values[0].getType() == paramTL.getLoweredType().getAddressType()) {
      auto value = values[0];
      makeUsed();
      return value;
   }

   // Otherwise, emit to a temporary.
   auto temp = SGF.emitTemporary(loc, paramTL);
   std::move(*this).forwardInto(SGF, loc, temp.get());
   return temp->getManagedAddress();
}

bool RValue::isObviouslyEqual(const RValue &rhs) const {
   assert(isComplete() && rhs.isComplete() && "Comparing incomplete rvalues");

   // Compare the count of elements instead of the type.
   if (values.size() != rhs.values.size())
      return false;

   return std::equal(values.begin(), values.end(), rhs.values.begin(),
                     [](const ManagedValue &lhs, const ManagedValue &rhs) -> bool {
                        return areObviouslySameValue(lhs.getValue(), rhs.getValue());
                     });
}

static PILValue getCanonicalValueSource(PILValue value) {
   while (true) {
      if (auto access = dyn_cast<BeginAccessInst>(value)) {
         value = access->getSource();
      } else {
         return value;
      }
   }
}

bool RValue::areObviouslySameValue(PILValue lhs, PILValue rhs) {
   return getCanonicalValueSource(lhs) == getCanonicalValueSource(rhs);
}

void RValue::dump() const {
   dump(llvm::errs());
}

void RValue::dump(raw_ostream &OS, unsigned indent) const {
   if (isInContext()) {
      OS.indent(indent) << "InContext\n";
      return;
   }

   getType().dump(OS, indent);
   for (auto &value : values) {
      value.dump(OS, indent + 2);
   }
}

void RValue::verify(PILGenFunction &SGF) const & {
// This is a no-op in non-assert builds.
#ifndef NDEBUG
   verifyHelper(values, &SGF);
#endif
}

bool RValue::isPlusOne(PILGenFunction &SGF) const & {
   return llvm::all_of(
      values, [&SGF](ManagedValue mv) -> bool { return mv.isPlusOne(SGF); });
}

bool RValue::isPlusZero(PILGenFunction &SGF) const & {
   return llvm::none_of(values,
                        [](ManagedValue mv) -> bool { return mv.isPlusZero(); });
}

const TypeLowering &RValue::getTypeLowering(PILGenFunction &SGF) const & {
   return SGF.getTypeLowering(getType());
}

PILType RValue::getLoweredType(PILGenFunction &SGF) const & {
   return getTypeLowering(SGF).getLoweredType();
}

PILType RValue::getLoweredImplodedTupleType(PILGenFunction &SGF) const & {
   PILType loweredType = getLoweredType(SGF);
   if (loweredType.isAddressOnly(SGF.F) &&
       SGF.silConv.useLoweredAddresses())
      return loweredType.getAddressType();
   return loweredType.getObjectType();
}

RValue RValue::copyForDiagnostics() const {
   assert(!isInSpecialState());
   assert(isComplete());
   RValue result(type);
   for (auto value : values)
      result.values.push_back(value);
   result.elementsToBeAdded = 0;
   return result;
}
