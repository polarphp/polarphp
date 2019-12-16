//===--- Notifications.cpp ------------------------------------------------===//
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

#define DEBUG_TYPE "pil-notifications"

#include "polarphp/pil/lang/Notifications.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace polar;

//===----------------------------------------------------------------------===//
// DeserializationNotificationHandler Impl of
// DeserializationNotificationHandlerSet
//===----------------------------------------------------------------------===//

#ifdef DNS_CHAIN_METHOD
#error "DNS_CHAIN_METHOD is defined?!"
#endif
#define DNS_CHAIN_METHOD(Name, FirstTy, SecondTy)                              \
  void DeserializationNotificationHandlerSet::did##Name(FirstTy first,         \
                                                        SecondTy second) {     \
    LLVM_DEBUG(llvm::dbgs()                                                    \
               << "*** Deserialization Notification: " << #Name << " ***\n");  \
    for (auto *p : getRange()) {                                               \
      LLVM_DEBUG(llvm::dbgs()                                                  \
                 << "    Begin Notifying: " << p->getName() << "\n");          \
      p->did##Name(first, second);                                             \
      LLVM_DEBUG(llvm::dbgs()                                                  \
                 << "    End Notifying: " << p->getName() << "\n");            \
    }                                                                          \
    LLVM_DEBUG(llvm::dbgs()                                                    \
               << "*** Completed Deserialization Notifications for " #Name     \
                  "\n");                                                       \
  }
DNS_CHAIN_METHOD(Deserialize, ModuleDecl *, PILFunction *)
DNS_CHAIN_METHOD(DeserializeFunctionBody, ModuleDecl *, PILFunction *)
DNS_CHAIN_METHOD(DeserializeWitnessTableEntries, ModuleDecl *,
                 PILWitnessTable *)
DNS_CHAIN_METHOD(DeserializeDefaultWitnessTableEntries, ModuleDecl *,
                 PILDefaultWitnessTable *)
DNS_CHAIN_METHOD(Deserialize, ModuleDecl *, PILGlobalVariable *)
DNS_CHAIN_METHOD(Deserialize, ModuleDecl *, PILVTable *)
DNS_CHAIN_METHOD(Deserialize, ModuleDecl *, PILWitnessTable *)
DNS_CHAIN_METHOD(Deserialize, ModuleDecl *, PILDefaultWitnessTable *)

#undef DNS_CHAIN_METHOD
