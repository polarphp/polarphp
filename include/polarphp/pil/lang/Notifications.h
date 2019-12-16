//===--- Notifications.h - PIL Undef Value Representation -------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_NOTIFICATIONS_H
#define POLARPHP_PIL_NOTIFICATIONS_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/StlExtras.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/SmallVector.h"
#include <memory>

namespace polar {

class PILNode;
class ModuleDecl;
class PILFunction;
class PILWitnessTable;
class PILDefaultWitnessTable;
class PILGlobalVariable;
class PILVTable;

/// An abstract class for handling PIL deserialization notifications.
///
/// This is an interface that should be implemented by clients that wish to
/// maintain a list of notification handlers. In contrast, handler
/// implementations should instead subclass DeserializationNotificationHandler
/// so that default no-op implementations can be inherited and so that it can be
/// passed into DeserializationNotificationHandlerSet.
class DeserializationNotificationHandlerBase {
public:
  /// Observe that we deserialized a function declaration.
  virtual void didDeserialize(ModuleDecl *mod, PILFunction *fn) = 0;

  /// Observe that we successfully deserialized a function body.
  virtual void didDeserializeFunctionBody(ModuleDecl *mod, PILFunction *fn) = 0;

  /// Observe that we successfully deserialized a witness table's entries.
  virtual void didDeserializeWitnessTableEntries(ModuleDecl *mod,
                                                 PILWitnessTable *wt) = 0;

  /// Observe that we successfully deserialized a default witness table's
  /// entries.
  virtual void
  didDeserializeDefaultWitnessTableEntries(ModuleDecl *mod,
                                           PILDefaultWitnessTable *wt) = 0;

  /// Observe that we deserialized a global variable declaration.
  virtual void didDeserialize(ModuleDecl *mod, PILGlobalVariable *var) = 0;

  /// Observe that we deserialized a v-table declaration.
  virtual void didDeserialize(ModuleDecl *mod, PILVTable *vtable) = 0;

  /// Observe that we deserialized a witness-table declaration.
  virtual void didDeserialize(ModuleDecl *mod, PILWitnessTable *wtable) = 0;

  /// Observe that we deserialized a default witness-table declaration.
  virtual void didDeserialize(ModuleDecl *mod,
                              PILDefaultWitnessTable *wtable) = 0;

  virtual ~DeserializationNotificationHandlerBase() = default;
};

/// A no-op implementation of DeserializationNotificationHandlerBase. Intended
/// to allow for users to implement only one of the relevant methods and have
/// all other methods be no-ops.
class DeserializationNotificationHandler
    : public DeserializationNotificationHandlerBase {
public:
  /// Observe that we deserialized a function declaration.
  virtual void didDeserialize(ModuleDecl *mod, PILFunction *fn) override {}

  /// Observe that we successfully deserialized a function body.
  virtual void didDeserializeFunctionBody(ModuleDecl *mod,
                                          PILFunction *fn) override {}

  /// Observe that we successfully deserialized a witness table's entries.
  virtual void didDeserializeWitnessTableEntries(ModuleDecl *mod,
                                                 PILWitnessTable *wt) override {
  }

  /// Observe that we successfully deserialized a default witness table's
  /// entries.
  virtual void didDeserializeDefaultWitnessTableEntries(
      ModuleDecl *mod, PILDefaultWitnessTable *wt) override {}

  /// Observe that we deserialized a global variable declaration.
  virtual void didDeserialize(ModuleDecl *mod,
                              PILGlobalVariable *var) override {}

  /// Observe that we deserialized a v-table declaration.
  virtual void didDeserialize(ModuleDecl *mod, PILVTable *vtable) override {}

  /// Observe that we deserialized a witness-table declaration.
  virtual void didDeserialize(ModuleDecl *mod,
                              PILWitnessTable *wtable) override {}

  /// Observe that we deserialized a default witness-table declaration.
  virtual void didDeserialize(ModuleDecl *mod,
                              PILDefaultWitnessTable *wtable) override {}

  virtual StringRef getName() const = 0;

  virtual ~DeserializationNotificationHandler() = default;
};

/// A notification handler that only overrides didDeserializeFunctionBody and
/// calls the passed in function pointer.
class FunctionBodyDeserializationNotificationHandler final
    : public DeserializationNotificationHandler {
public:
  using Handler = void (*)(ModuleDecl *, PILFunction *);

private:
  Handler handler;

public:
  FunctionBodyDeserializationNotificationHandler(Handler handler)
      : handler(handler) {}
  virtual ~FunctionBodyDeserializationNotificationHandler() {}

  void didDeserializeFunctionBody(ModuleDecl *mod, PILFunction *fn) override {
    (*handler)(mod, fn);
  }

  StringRef getName() const override {
    return "FunctionBodyDeserializationNotificationHandler";
  }
};

/// A type that contains a set of unique DeserializationNotificationHandlers and
/// implements DeserializationNotificationHandlerBase by iterating over the
/// stored handlers and calling each handler's implementation.
class DeserializationNotificationHandlerSet final
    : public DeserializationNotificationHandlerBase {
public:
  using NotificationUniquePtr =
    std::unique_ptr<DeserializationNotificationHandler>;

private:
  /// A list of deserialization callbacks that update the PILModule and other
  /// parts of PIL as deserialization occurs.
  ///
  /// We use 3 here since that is the most that will ever be used today in the
  /// compiler. If that changed, that number should be changed as well. The
  /// specific users are:
  ///
  /// 1. PILModule's serialization callback.
  /// 2. PILPassManager notifications.
  /// 3. Access Enforcement Stripping notification.
  SmallVector<NotificationUniquePtr, 3> handlerSet;

public:
  DeserializationNotificationHandlerSet() = default;
  ~DeserializationNotificationHandlerSet() = default;

  bool erase(DeserializationNotificationHandler *handler) {
    auto iter = find_if(handlerSet, [&](const NotificationUniquePtr &h) {
      return handler == h.get();
    });
    if (iter == handlerSet.end())
      return false;
    handlerSet.erase(iter);
    return true;
  }

  void add(NotificationUniquePtr &&handler) {
    // Since we store unique_ptrs and are accepting a movable rvalue here, we
    // should never have a case where we have a notification added twice. But
    // just to be careful, lets use an assert.
    assert(!count_if(handlerSet, [&](const NotificationUniquePtr &h) {
             return handler.get() == h.get();
           }) && "Two unique ptrs pointing at the same memory?!");
    handlerSet.emplace_back(std::move(handler));
  }

  static DeserializationNotificationHandler *
  getUnderlyingHandler(const NotificationUniquePtr &h) {
    return h.get();
  }

  /// An iterator into the notification set that returns a bare
  /// 'DeserializationNotificationHandler *' projected from one of the
  /// underlying std::unique_ptr<DeserializationNotificationHandler>.
  using iterator = llvm::mapped_iterator<
      decltype(handlerSet)::const_iterator,
      decltype(&DeserializationNotificationHandlerSet::getUnderlyingHandler)>;
  using range = llvm::iterator_range<iterator>;

  /// Returns an iterator to the first element of the handler set.
  ///
  /// NOTE: This iterator has a value_type of
  /// 'DeserializationNotificationHandler'.
  iterator begin() const {
    auto *fptr = &DeserializationNotificationHandlerSet::getUnderlyingHandler;
    return llvm::map_iterator(handlerSet.begin(), fptr);
  }

  /// Returns an iterator to the end of the handler set.
  ///
  /// NOTE: This iterator has a value_type of
  /// 'DeserializationNotificationHandler'.
  iterator end() const {
    auto *fptr = &DeserializationNotificationHandlerSet::getUnderlyingHandler;
    return llvm::map_iterator(handlerSet.end(), fptr);
  }

  /// Returns a range that iterates over bare pointers projected from the
  /// internal owned memory pointers in the handlerSet.
  ///
  /// NOTE: The underlying iterator value_type here is
  /// 'DeserializationNotificationHandler *'.
  auto getRange() const -> range { return llvm::make_range(begin(), end()); }

  //===--------------------------------------------------------------------===//
  // DeserializationNotificationHandler implementation via chaining to the
  // handlers we contain.
  //===--------------------------------------------------------------------===//

  void didDeserialize(ModuleDecl *mod, PILFunction *fn) override;
  void didDeserializeFunctionBody(ModuleDecl *mod, PILFunction *fn) override;
  void didDeserializeWitnessTableEntries(ModuleDecl *mod,
                                         PILWitnessTable *wt) override;
  void
  didDeserializeDefaultWitnessTableEntries(ModuleDecl *mod,
                                           PILDefaultWitnessTable *wt) override;
  void didDeserialize(ModuleDecl *mod, PILGlobalVariable *var) override;
  void didDeserialize(ModuleDecl *mod, PILVTable *vtable) override;
  void didDeserialize(ModuleDecl *mod, PILWitnessTable *wtable) override;
  void didDeserialize(ModuleDecl *mod,
                      PILDefaultWitnessTable *wtable) override;
};

/// A protocol (or interface) for handling value deletion notifications.
///
/// This class is used as a base class for any class that need to accept
/// instruction deletion notification messages. This is used by passes and
/// analysis that need to invalidate data structures that contain pointers.
/// This is similar to LLVM's ValueHandle.
struct DeleteNotificationHandler {
  DeleteNotificationHandler() { }
  virtual ~DeleteNotificationHandler() {}

  /// Handle the invalidation message for the value \p Value.
  virtual void handleDeleteNotification(PILNode *value) { }

  /// Returns True if the pass, analysis or other entity wants to receive
  /// notifications. This callback is called once when the class is being
  /// registered, and not once per notification. Entities that implement
  /// this callback should always return a constant answer (true/false).
  virtual bool needsNotifications() { return false; }
};

} // namespace polar

#endif
