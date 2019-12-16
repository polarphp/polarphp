//===--- FormalEvaluation.h -------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_GEN_FORMALEVALUATION_H
#define POLARPHP_PIL_GEN_FORMALEVALUATION_H

#include "polarphp/pil/gen/Cleanup.h"
#include "polarphp/basic/DiverseStack.h"
#include "polarphp/pil/lang/PILValue.h"
#include "llvm/ADT/Optional.h"

namespace polar::lowering {

class PILGenFunction;
class LogicalPathComponent;
class FormalEvaluationScope;

class FormalAccess {
public:
   enum Kind { Shared, Exclusive, Owned, Unenforced };

private:
   unsigned allocatedSize;
   Kind kind;

protected:
   PILLocation loc;
   CleanupHandle cleanup;
   bool finished;

   FormalAccess(unsigned allocatedSize, Kind kind, PILLocation loc,
                CleanupHandle cleanup)
      : allocatedSize(allocatedSize), kind(kind), loc(loc), cleanup(cleanup),
        finished(false) {}

public:
   virtual ~FormalAccess() {}

   // This anchor method serves three purposes: it aligns the class to
   // a pointer boundary, it makes the class a primary base so that
   // subclasses will be at offset zero, and it anchors the v-table
   // to a specific file.
   virtual void _anchor();

   /// Return the allocated size of this object.  This is required by
   /// DiverseStack for iteration.
   size_t allocated_size() const { return allocatedSize; }

   CleanupHandle getCleanup() const { return cleanup; }

   Kind getKind() const { return kind; }

   void finish(PILGenFunction &SGF) { finishImpl(SGF); }

   void setFinished() { finished = true; }

   bool isFinished() const { return finished; }

   void verify(PILGenFunction &SGF) const;

protected:
   virtual void finishImpl(PILGenFunction &SGF) = 0;
};

// FIXME: Misnomer. This is not used for borrowing a formal memory location
// (ExclusiveBorrowFormalAccess is always used for that). This is only used for
// formal access from a +0 value, which requires producing a "borrowed"
// PILValue.
class SharedBorrowFormalAccess : public FormalAccess {
   PILValue originalValue;
   PILValue borrowedValue;

public:
   SharedBorrowFormalAccess(PILLocation loc, CleanupHandle cleanup,
                            PILValue originalValue, PILValue borrowedValue)
      : FormalAccess(sizeof(*this), FormalAccess::Shared, loc, cleanup),
        originalValue(originalValue), borrowedValue(borrowedValue) {}

   PILValue getBorrowedValue() const { return borrowedValue; }
   PILValue getOriginalValue() const { return originalValue; }

private:
   void finishImpl(PILGenFunction &SGF) override;
};

class OwnedFormalAccess : public FormalAccess {
   PILValue value;

public:
   OwnedFormalAccess(PILLocation loc, CleanupHandle cleanup, PILValue value)
      : FormalAccess(sizeof(*this), FormalAccess::Owned, loc, cleanup),
        value(value) {}

   PILValue getValue() const { return value; }

private:
   void finishImpl(PILGenFunction &SGF) override;
};

class FormalEvaluationContext {
   DiverseStack<FormalAccess, 128> stack;
   FormalEvaluationScope *innermostScope = nullptr;

   friend class FormalEvaluationScope;

public:
   using stable_iterator = decltype(stack)::stable_iterator;
   using iterator = decltype(stack)::iterator;

   FormalEvaluationContext() : stack() {}

   // This is a type that can only be embedded in other types, it can not be
   // moved or copied.
   FormalEvaluationContext(const FormalEvaluationContext &) = delete;
   FormalEvaluationContext(FormalEvaluationContext &&) = delete;
   FormalEvaluationContext &operator=(const FormalEvaluationContext &) = delete;
   FormalEvaluationContext &operator=(FormalEvaluationContext &&) = delete;

   ~FormalEvaluationContext() {
      assert(stack.empty() &&
             "entries remaining on formal evaluation cleanup stack at end of function!");
   }

   iterator begin() { return stack.begin(); }
   iterator end() { return stack.end(); }
   stable_iterator stabilize(iterator iter) const {
      return stack.stabilize(iter);
   }
   stable_iterator stable_begin() { return stabilize(begin()); }
   iterator find(stable_iterator iter) { return stack.find(iter); }

   FormalAccess &findAndAdvance(stable_iterator &stable) {
      return stack.findAndAdvance(stable);
   }

   template <class U, class... ArgTypes> void push(ArgTypes &&... args) {
      stack.push<U>(std::forward<ArgTypes>(args)...);
   }

   void pop() { stack.pop(); }

   /// Pop objects off of the stack until \p the object pointed to by stable_iter
   /// is the top element of the stack.
   void pop(stable_iterator stable_iter) { stack.pop(stable_iter); }

   bool isInFormalEvaluationScope() const { return innermostScope != nullptr; }

   void dump(PILGenFunction &SGF);

#ifndef NDEBUG
   void checkCleanupDeactivation(CleanupHandle handle);
#endif
};

/// A scope associated with the beginning of the evaluation of an lvalue.
///
/// The evaluation of an l-value is split into two stages: its formal
/// evaluation, which evaluates any independent r-values embedded in the l-value
/// expression (e.g. class references and subscript indices), and its formal
/// access duration, which delimits the span of time for which the referenced
/// storage is actually accessed.
///
/// Note that other evaluations can be interleaved between the formal evaluation
/// and the beginning of the formal access.  For example, in a simple assignment
/// statement, the left-hand side of the assignment is first formally evaluated
/// as an l-value, then the right-hand side is evaluated as an r-value, and only
/// then does the write access begin to the l-value.
///
/// Note also that the formal evaluation of an l-value will sometimes require
/// its component l-values to be formally accessed.  For example, the formal
/// access of the l-value `x?.prop` will initiate an access to `x` immediately
/// because the downstream evaluation must be skipped if `x` has no value, which
/// cannot be determined without beginning the access.
///
/// *NOTE* All formal access contain a pointer to a cleanup in the normal
/// cleanup stack. This is to ensure that when PILGen calls
/// Cleanups.emitBranchAndCleanups (and other special cleanup code along error
/// edges), writebacks are properly created. What is key to notice is that all
/// of these cleanup emission types are non-destructive. Contrast this with
/// normal scope popping. In such a case, the scope pop is destructive. This
/// means that any pointers from the formal access to the cleanup stack is now
/// invalid.
///
/// In order to avoid this issue, it is important to /never/ create a formal
/// access cleanup when the "top level" scope is not a formal evaluation scope.
class FormalEvaluationScope {
   PILGenFunction &SGF;
   llvm::Optional<FormalEvaluationContext::stable_iterator> savedDepth;

   /// The immediate outer evaluation scope.  This scope is only inserted
   /// into the chain if it wasn't in an inout conversion scope on creation.
   FormalEvaluationScope *previous;
   bool wasInInOutConversionScope;

   friend class FormalEvaluationContext;

public:
   FormalEvaluationScope(PILGenFunction &SGF);
   ~FormalEvaluationScope() {
      if (!savedDepth.hasValue())
         return;
      popImpl();
   }

   bool isPopped() const { return !savedDepth.hasValue(); }

   void pop() {
      if (wasInInOutConversionScope)
         return;

      assert(!isPopped() && "popping an already-popped scope!");
      popImpl();
      savedDepth.reset();
   }

   FormalEvaluationScope(const FormalEvaluationScope &) = delete;
   FormalEvaluationScope &operator=(const FormalEvaluationScope &) = delete;

   FormalEvaluationScope(FormalEvaluationScope &&o);
   FormalEvaluationScope &operator=(FormalEvaluationScope &&o) = delete;

   /// Verify that we can successfully access all of the inner lexical scopes
   /// that would be popped by this scope.
   void verify() const;

private:
   void popImpl();
};

#ifndef NDEBUG
inline void
FormalEvaluationContext::checkCleanupDeactivation(CleanupHandle handle) {
   for (auto &access : *this) {
      assert((access.isFinished() || access.getCleanup() != handle) &&
             "popping active formal-evaluation cleanup");
   }
}
#endif

} // namespace polar::lowering

#endif // POLARPHP_PIL_GEN_FORMALEVALUATION_H
