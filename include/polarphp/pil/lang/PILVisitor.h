//===--- PILVisitor.h - Defines the PILVisitor class ------------*- C++ -*-===//
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
// This file defines the PILVisitor class, used for walking PIL code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILVISITOR_H
#define POLARPHP_PIL_PILVISITOR_H

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILUndef.h"
#include "llvm/Support/ErrorHandling.h"

namespace polar {

/// A helper class for all the PIL visitors.
/// You probably shouldn't use this directly.
template <typename ImplClass, typename RetTy = void, typename... ArgTys>
class PILVisitorBase {
public:
   ImplClass &asImpl() { return static_cast<ImplClass &>(*this); }

   void visitPILBasicBlock(PILBasicBlock *BB, ArgTys... args) {
      asImpl().visitBasicBlockArguments(BB, args...);

      for (auto &I : *BB)
         asImpl().visit(&I, args...);
   }
   void visitPILBasicBlock(PILBasicBlock &BB, ArgTys... args) {
      asImpl().visitPILBasicBlock(&BB, args...);
   }

   void visitPILFunction(PILFunction *F, ArgTys... args) {
      for (auto &BB : *F)
         asImpl().visitPILBasicBlock(&BB, args...);
   }
   void visitPILFunction(PILFunction &F, ArgTys... args) {
      asImpl().visitPILFunction(&F, args...);
   }
};

/// PILValueVisitor - This is a simple visitor class for Swift PIL nodes,
/// allowing clients to walk over entire PIL functions, blocks, or instructions.
template <typename ImplClass, typename RetTy = void, typename... ArgTys>
class PILValueVisitor
   : public PILVisitorBase<ImplClass, RetTy, ArgTys...> {
   using super = PILVisitorBase<ImplClass, RetTy, ArgTys...>;
public:
   using super::asImpl;

   RetTy visit(ValueBase *V, ArgTys... args) {
      switch (V->getKind()) {
#define VALUE(CLASS, PARENT)                                \
  case ValueKind::CLASS:                                    \
    return asImpl().visit##CLASS(static_cast<CLASS*>(V),    \
                                 std::forward<ArgTys>(args)...);
#include "polarphp/pil/lang/PILNodesDef.h"
      }
      llvm_unreachable("Not reachable, all cases handled");
   }

   // Define default dispatcher implementations chain to parent nodes.
#define VALUE(CLASS, PARENT)                                         \
  RetTy visit##CLASS(CLASS *I, ArgTys... args) {                     \
    return asImpl().visit##PARENT(I, std::forward<ArgTys>(args)...); \
  }
#define ABSTRACT_VALUE(CLASS, PARENT) VALUE(CLASS, PARENT)
#include "polarphp/pil/lang/PILNodesDef.h"
};

/// A visitor that should only visit PIL instructions.
template <typename ImplClass, typename RetTy = void, typename... ArgTys>
class PILInstructionVisitor
   : public PILVisitorBase<ImplClass, RetTy, ArgTys...> {
   using super = PILVisitorBase<ImplClass, RetTy, ArgTys...>;
public:
   using super::asImpl;

   // Perform any required pre-processing before visiting.
   // Sub-classes can override it to provide their custom
   // pre-processing steps.
   void beforeVisit(PILInstruction *inst) {}

   RetTy visit(PILInstruction *inst, ArgTys... args) {
      asImpl().beforeVisit(inst, args...);

      switch (inst->getKind()) {
#define INST(CLASS, PARENT)                                             \
    case PILInstructionKind::CLASS:                                     \
      return asImpl().visit##CLASS(static_cast<CLASS*>(inst),           \
                                   std::forward<ArgTys>(args)...);
#include "polarphp/pil/lang/PILNodesDef.h"
      }
      llvm_unreachable("Not reachable, all cases handled");
   }

   // Define default dispatcher implementations chain to parent nodes.
#define INST(CLASS, PARENT)                                             \
  RetTy visit##CLASS(CLASS *inst, ArgTys... args) {                     \
    return asImpl().visit##PARENT(inst, std::forward<ArgTys>(args)...); \
  }
#define ABSTRACT_INST(CLASS, PARENT) INST(CLASS, PARENT)
#include "polarphp/pil/lang/PILNodesDef.h"

   void visitBasicBlockArguments(PILBasicBlock *BB, ArgTys... args) {}
};

/// A visitor that should visit all PIL nodes.
template <typename ImplClass, typename RetTy = void, typename... ArgTys>
class PILNodeVisitor
   : public PILVisitorBase<ImplClass, RetTy, ArgTys...> {
   using super = PILVisitorBase<ImplClass, RetTy, ArgTys...>;
public:
   using super::asImpl;

   // Perform any required pre-processing before visiting.
   // Sub-classes can override it to provide their custom
   // pre-processing steps.
   void beforeVisit(PILNode *I, ArgTys... args) {}

   RetTy visit(PILNode *node, ArgTys... args) {
      asImpl().beforeVisit(node, args...);

      switch (node->getKind()) {
#define NODE(CLASS, PARENT)                                             \
    case PILNodeKind::CLASS:                                            \
      return asImpl().visit##CLASS(cast<CLASS>(node),                   \
                                   std::forward<ArgTys>(args)...);
#include "polarphp/pil/lang/PILNodesDef.h"
      }
      llvm_unreachable("Not reachable, all cases handled");
   }

   // Define default dispatcher implementations chain to parent nodes.
#define NODE(CLASS, PARENT)                                             \
  RetTy visit##CLASS(CLASS *node, ArgTys... args) {                     \
    return asImpl().visit##PARENT(node, std::forward<ArgTys>(args)...); \
  }
#define ABSTRACT_NODE(CLASS, PARENT) NODE(CLASS, PARENT)
#include "polarphp/pil/lang/PILNodesDef.h"

   void visitBasicBlockArguments(PILBasicBlock *BB, ArgTys... args) {
      for (auto argI = BB->args_begin(), argEnd = BB->args_end(); argI != argEnd;
           ++argI)
         asImpl().visit(*argI, args...);
   }
};

} // end namespace polar

#endif
