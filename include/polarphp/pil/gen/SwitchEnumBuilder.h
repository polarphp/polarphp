//===--- SwitchEnumBuilder.h ----------------------------------------------===//
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

#ifndef POLARPHP_PIL_GEN_PILGEN_SWITCHENUMBUILDER_H
#define POLARPHP_PIL_GEN_PILGEN_SWITCHENUMBUILDER_H

#include "polarphp/pil/gen/Scope.h"

namespace polar::lowering {

class PILGenFunction;

/// TODO: std::variant.
struct SwitchCaseBranchDest {
  Optional<JumpDest> jumpDest;
  NullablePtr<PILBasicBlock> block;

  SwitchCaseBranchDest() : jumpDest(), block() {}

  /// Implicit conversion.
  SwitchCaseBranchDest(JumpDest jumpDest) : jumpDest(jumpDest), block() {}

  /// Implicit conversion.
  SwitchCaseBranchDest(PILBasicBlock *block) : jumpDest(), block(block) {}

  explicit operator bool() const {
    return jumpDest.hasValue() || block.isNonNull();
  }

  bool hasJumpDest() const { return jumpDest.hasValue(); }
  bool hasBlock() const { return bool(block); }

  PILBasicBlock *getBlock() { return block.getPtrOrNull(); }
  JumpDest &getJumpDest() { return jumpDest.getValue(); }
};

/// A cleanup scope RAII object, like FullExpr, that comes with a JumpDest for a
/// continuation block. It is intended to be used to handle switch cases.
///
/// You *must* call exit() at some point.
///
/// This scope is also exposed to the debug info.
class SwitchCaseFullExpr {
  PILGenFunction &SGF;
  Scope scope;
  CleanupLocation loc;
  SwitchCaseBranchDest branchDest;

public:
  SwitchCaseFullExpr(PILGenFunction &SGF, CleanupLocation loc);
  SwitchCaseFullExpr(PILGenFunction &SGF, CleanupLocation loc,
                     SwitchCaseBranchDest branchDest);

  ~SwitchCaseFullExpr();

  SwitchCaseFullExpr(const SwitchCaseFullExpr &) = delete;
  SwitchCaseFullExpr &operator=(const SwitchCaseFullExpr &) = delete;

  /// Pop the scope and branch to the branch destination. If this case has an
  /// associated JumpDest as its branch destination, then this will cause
  /// cleanups associated with the jump dest to be emitted.
  void exitAndBranch(PILLocation loc, ArrayRef<PILValue> result = {});

  /// Pop the scope and do not branch to the branch destination. This is
  /// intended to be used in situations where one wants to model a switch region
  /// that ends midway through one of the case blocks.
  void exit();

  /// Do not pop the scope and do not branch to the branch destination. But do
  /// invalidate the scope. This can occur when emitting unreachables.
  void unreachableExit();
};

/// A class for building switch enums that handles all of the ownership
/// requirements for the user.
///
/// It assumes that the user passes in a block that takes in a ManagedValue and
/// returns a ManagedValue for the blocks exit argument. Should return an empty
/// ManagedValue to signal no result.
///
/// TODO: Allow cases to take JumpDest as continuation blocks and then
/// force exitBranchAndCleanup to be run.
class SwitchEnumBuilder {
public:
  using NormalCaseHandler =
      std::function<void(ManagedValue, SwitchCaseFullExpr &&)>;
  using DefaultCaseHandler =
      std::function<void(ManagedValue, SwitchCaseFullExpr &&)>;

  enum class DefaultDispatchTime { BeforeNormalCases, AfterNormalCases };

private:
  struct NormalCaseData {
    EnumElementDecl *decl;
    PILBasicBlock *block;
    SwitchCaseBranchDest branchDest;
    NormalCaseHandler handler;
    ProfileCounter count;

    NormalCaseData(EnumElementDecl *decl, PILBasicBlock *block,
                   SwitchCaseBranchDest branchDest, NormalCaseHandler handler,
                   ProfileCounter count)
        : decl(decl), block(block), branchDest(branchDest), handler(handler),
          count(count) {}
    ~NormalCaseData() = default;
  };

  struct DefaultCaseData {
    PILBasicBlock *block;
    SwitchCaseBranchDest branchDest;
    DefaultCaseHandler handler;
    DefaultDispatchTime dispatchTime;
    ProfileCounter count;

    DefaultCaseData(PILBasicBlock *block, SwitchCaseBranchDest branchDest,
                    DefaultCaseHandler handler,
                    DefaultDispatchTime dispatchTime, ProfileCounter count)
        : block(block), branchDest(branchDest), handler(handler),
          dispatchTime(dispatchTime), count(count) {}
    ~DefaultCaseData() = default;
  };

  PILGenBuilder &builder;
  PILLocation loc;
  ManagedValue optional;
  llvm::Optional<DefaultCaseData> defaultBlockData;
  llvm::SmallVector<NormalCaseData, 8> caseDataArray;

public:
  SwitchEnumBuilder(PILGenBuilder &builder, PILLocation loc,
                    ManagedValue optional)
      : builder(builder), loc(loc), optional(optional) {}

  void addDefaultCase(
      PILBasicBlock *defaultBlock, SwitchCaseBranchDest branchDest,
      DefaultCaseHandler handle,
      DefaultDispatchTime dispatchTime = DefaultDispatchTime::AfterNormalCases,
      ProfileCounter count = ProfileCounter()) {
    defaultBlockData.emplace(defaultBlock, branchDest, handle, dispatchTime,
                             count);
  }

  void addCase(EnumElementDecl *decl, PILBasicBlock *caseBlock,
               SwitchCaseBranchDest branchDest, NormalCaseHandler handle,
               ProfileCounter count = ProfileCounter()) {
    caseDataArray.emplace_back(decl, caseBlock, branchDest, handle, count);
  }

  void addOptionalSomeCase(PILBasicBlock *caseBlock) {
    auto *decl = getSGF().getAstContext().getOptionalSomeDecl();
    caseDataArray.emplace_back(
        decl, caseBlock, nullptr,
        [](ManagedValue mv, SwitchCaseFullExpr &&expr) { expr.exit(); },
        ProfileCounter());
  }

  void addOptionalNoneCase(PILBasicBlock *caseBlock) {
    auto *decl = getSGF().getAstContext().getOptionalNoneDecl();
    caseDataArray.emplace_back(
        decl, caseBlock, nullptr,
        [](ManagedValue mv, SwitchCaseFullExpr &&expr) { expr.exit(); },
        ProfileCounter());
  }

  void addOptionalSomeCase(PILBasicBlock *caseBlock,
                           SwitchCaseBranchDest branchDest,
                           NormalCaseHandler handle,
                           ProfileCounter count = ProfileCounter()) {
    auto *decl = getSGF().getAstContext().getOptionalSomeDecl();
    caseDataArray.emplace_back(decl, caseBlock, branchDest, handle, count);
  }

  void addOptionalNoneCase(PILBasicBlock *caseBlock,
                           SwitchCaseBranchDest branchDest,
                           NormalCaseHandler handle,
                           ProfileCounter count = ProfileCounter()) {
    auto *decl = getSGF().getAstContext().getOptionalNoneDecl();
    caseDataArray.emplace_back(decl, caseBlock, branchDest, handle, count);
  }

  void emit() &&;

private:
  PILGenFunction &getSGF() const { return builder.getPILGenFunction(); }
};

} // end polar::lowering  namespace

#endif // POLARPHP_PIL_GEN_PILGEN_SWITCHENUMBUILDER_H
