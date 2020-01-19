//===--- OSLogOptimizer.cpp - Optimizes calls to OS Log ===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// This pass implements PIL-level optimizations and diagnostics for the
/// os log APIs based on string interpolations. The APIs are implemented
/// in the files: OSLogMessage.swift, OSLog.swift. This pass constant evaluates
/// the log calls along with the auto-generated calls to the custom string
/// interpolation methods, which processes the string interpolation
/// passed to the log calls, and folds the constants found during the
/// evaluation. The constants that are folded include the C format string that
/// is constructed by the custom string interpolation methods from the string
/// interpolation, and the size and headers of the byte buffer into which
/// arguments are packed. This pass is closely tied to the implementation of
/// the log APIs.
///
/// Pass Dependencies:  This pass depends on MandatoryInlining and Mandatory
/// Linking happening before this pass and ConstantPropagation happening after
/// this pass. This pass also uses `ConstExprStepEvaluator` defined in
/// `Utils/ConstExpr.cpp`.
///
/// Algorithm Overview:
///
/// This pass implements a function-level transformation that collects calls
/// to the initializer of the custom string interpolation type: OSLogMessage,
/// which are annotated with an @_semantics attribute, and performs the
/// following steps on each such call.
///
///  1. Determines the range of instructions to constant evaluate.
///     The range starts from the first PIL instruction that begins the
///     construction of the custom string interpolation type: OSLogMessage to
///     the last transitive users of OSLogMessage. The log call which is marked
///     as @_transparent will be inlined into the caller before this pass
///     begins.
///
///  2. Constant evaluates the range of instruction identified in Step 1 and
///     collects string and integer-valued instructions who values were found
///     to be constants. The evaluation uses 'ConsExprStepEvaluator' utility.
///
///  3. After constant evaluation, the string and integer-value properties
///     of the custom string interpolation type: `OSLogInterpolation` must be
///     constants. This property is checked and any violations are diagnosed.
///     The errors discovered here may arise from the implementation of the
///     log APIs in the  overlay or could be because of wrong usage of the
///     log APIs.
///     TODO: these errors will be diagnosed by a separate, dedicated pass.
///
///  4. The constant instructions that were found in step 2 are folded by
///     generating PIL code that produces the constants. This also removes
///     instructions that are dead after folding.
///
/// Code Overview:
///
/// The function 'OSLogOptimization::run' implements the overall driver for
/// steps 1 to 4. The function 'beginOfInterpolation' identifies the begining of
/// interpolation (step 1) and the function 'getEndPointsOfDataDependentChain'
/// identifies the last transitive users of the OSLogMessage instance (step 1).
/// The function 'constantFold' is a driver for the steps 2 to 4. Step 2 is
/// implemented by the function 'collectConstants', step 3 by
/// 'detectAndDiagnoseErrors' and 'checkOSLogMessageIsConstant', and step 4 by
/// 'substituteConstants' and 'emitCodeForSymbolicValue'. The remaining
/// functions in the file implement the subtasks and utilities needed by the
/// above functions.

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/basic/OptimizationMode.h"
#include "polarphp/ast/SemanticAttrs.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/pil/lang/BasicBlockUtils.h"
#include "polarphp/pil/lang/OwnershipUtils.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILConstants.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/TypeLowering.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/ConstExpr.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/pil/optimizer/utils/PILInliner.h"
#include "polarphp/pil/optimizer/utils/PILOptFunctionBuilder.h"
#include "polarphp/pil/optimizer/utils/ValueLifetime.h"
#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/ADT/MapVector.h"

using namespace polar;
using namespace lowering;

template <typename... T, typename... U>
static void diagnose(AstContext &Context, SourceLoc loc, Diag<T...> diag,
                     U &&... args) {
   Context.Diags.diagnose(loc, diag, std::forward<U>(args)...);
}

namespace {

/// If the given instruction is a call to the compiler-intrinsic initializer
/// of String that accepts string literals, return the called function.
/// Otherwise, return nullptr.
static PILFunction *getStringMakeUTF8Init(PILInstruction *inst) {
   auto *apply = dyn_cast<ApplyInst>(inst);
   if (!apply)
      return nullptr;

   PILFunction *callee = apply->getCalleeFunction();
   if (!callee || !callee->hasSemanticsAttr(semantics::STRING_MAKE_UTF8))
      return nullptr;
   return callee;
}

// A cache of string-related, PIL information that is needed to create and
// initalize strings from raw string literals. This information is
// extracted from instructions while they are constant evaluated. Though the
// information contained here can be constructed from scratch, extracting it
// from existing instructions is more efficient.
class StringPILInfo {
   /// PILFunction corresponding to an intrinsic string initializer that
   /// constructs a Swift String from a string literal.
   PILFunction *stringInitIntrinsic = nullptr;

   /// PIL metatype of String.
   PILType stringMetatype = PILType();

public:
   /// Extract and cache the required string-related information from the
   /// given instruction, if possible.
   void extractStringInfoFromInstruction(PILInstruction *inst) {
      // If the cache is already initialized do nothing.
      if (stringInitIntrinsic)
         return;

      PILFunction *callee = getStringMakeUTF8Init(inst);
      if (!callee)
         return;

      this->stringInitIntrinsic = callee;

      MetatypeInst *stringMetatypeInst =
         dyn_cast<MetatypeInst>(inst->getOperand(4)->getDefiningInstruction());
      this->stringMetatype = stringMetatypeInst->getType();
   }

   PILFunction *getStringInitIntrinsic() const {
      assert(stringInitIntrinsic);
      return stringInitIntrinsic;
   }

   PILType getStringMetatype() const {
      assert(stringMetatype);
      return stringMetatype;
   }
};

/// State needed for constant folding.
class FoldState {
public:
   /// Storage for symbolic values constructed during interpretation.
   SymbolicValueBumpAllocator allocator;

   /// Evaluator for evaluating instructions one by one.
   ConstExprStepEvaluator constantEvaluator;

   /// Information needed for folding strings.
   StringPILInfo stringInfo;

   /// Instruction from where folding must begin.
   PILInstruction *beginInstruction;

   /// Instructions that mark the end points of constant evaluation.
   SmallSetVector<PILInstruction *, 2> endInstructions;

private:
   /// PIL values that were found to be constants during
   /// constant evaluation.
   SmallVector<PILValue, 4> constantPILValues;

public:
   FoldState(PILFunction *fun, unsigned assertConfig, PILInstruction *beginInst,
             ArrayRef<PILInstruction *> endInsts)
      : constantEvaluator(allocator, fun, assertConfig),
        beginInstruction(beginInst),
        endInstructions(endInsts.begin(), endInsts.end()) {}

   void addConstantPILValue(PILValue value) {
      constantPILValues.push_back(value);
   }

   ArrayRef<PILValue> getConstantPILValues() {
      return ArrayRef<PILValue>(constantPILValues);
   }
};

/// Return true if and only if the given nominal type declaration is that of
/// a stdlib Int or stdlib Bool.
static bool isStdlibIntegerOrBoolDecl(NominalTypeDecl *numberDecl,
                                      AstContext &astCtx) {
   return (numberDecl == astCtx.getIntDecl() ||
           numberDecl == astCtx.getInt8Decl() ||
           numberDecl == astCtx.getInt16Decl() ||
           numberDecl == astCtx.getInt32Decl() ||
           numberDecl == astCtx.getInt64Decl() ||
           numberDecl == astCtx.getUIntDecl() ||
           numberDecl == astCtx.getUInt8Decl() ||
           numberDecl == astCtx.getUInt16Decl() ||
           numberDecl == astCtx.getUInt32Decl() ||
           numberDecl == astCtx.getUInt64Decl() ||
           numberDecl == astCtx.getBoolDecl());
}

/// Return true if and only if the given PIL type represents a Stdlib or builtin
/// integer type or a Bool type.
static bool isIntegerOrBoolType(PILType silType, AstContext &astContext) {
   if (silType.is<BuiltinIntegerType>()) {
      return true;
   }
   NominalTypeDecl *nominalDecl = silType.getNominalOrBoundGenericNominal();
   return nominalDecl && isStdlibIntegerOrBoolDecl(nominalDecl, astContext);
}

/// Return true if and only if the given PIL type represents a String type.
static bool isStringType(PILType silType, AstContext &astContext) {
   NominalTypeDecl *nominalDecl = silType.getNominalOrBoundGenericNominal();
   return nominalDecl && nominalDecl == astContext.getStringDecl();
}

/// Return true if and only if the given PIL type represents an Array type.
static bool isArrayType(PILType silType, AstContext &astContext) {
   NominalTypeDecl *nominalDecl = silType.getNominalOrBoundGenericNominal();
   return nominalDecl && nominalDecl == astContext.getArrayDecl();
}

/// Decide if the given instruction (which could possibly be a call) should
/// be constant evaluated.
///
/// \returns true iff the given instruction is not a call or if it is, it calls
/// a known constant-evaluable function such as string append etc., or calls
/// a function annotate as "constant_evaluable".
static bool shouldAttemptEvaluation(PILInstruction *inst) {
   auto *apply = dyn_cast<ApplyInst>(inst);
   if (!apply)
      return true;
   PILFunction *calleeFun = apply->getCalleeFunction();
   if (!calleeFun)
      return false;
   return isKnownConstantEvaluableFunction(calleeFun) ||
          isConstantEvaluable(calleeFun);
}

/// Skip or evaluate the given instruction based on the evaluation policy and
/// handle errors. The policy is to evaluate all non-apply instructions as well
/// as apply instructions that are marked as "constant_evaluable".
static std::pair<Optional<PILBasicBlock::iterator>, Optional<SymbolicValue>>
evaluateOrSkip(ConstExprStepEvaluator &stepEval,
PILBasicBlock::iterator instI) {
PILInstruction *inst = &(*instI);

// Note that skipping a call conservatively approximates its effects on the
// interpreter state.
if (shouldAttemptEvaluation(inst)) {
return stepEval.tryEvaluateOrElseMakeEffectsNonConstant(instI);
}
return stepEval.skipByMakingEffectsNonConstant(instI);
}

/// Return true iff the given value is a stdlib Int or Bool and it not a direct
/// construction of Int or Bool.
static bool isFoldableIntOrBool(PILValue value, AstContext &astContext) {
   return isIntegerOrBoolType(value->getType(), astContext) &&
          !isa<StructInst>(value);
}

/// Return true iff the given value is a string and is not an initialization
/// of an string from a string literal.
static bool isFoldableString(PILValue value, AstContext &astContext) {
   return isStringType(value->getType(), astContext) &&
          (!isa<ApplyInst>(value) ||
           !getStringMakeUTF8Init(cast<ApplyInst>(value)));
}

/// Return true iff the given value is an array and is not an initialization
/// of an array from an array literal.
static bool isFoldableArray(PILValue value, AstContext &astContext) {
   if (!isArrayType(value->getType(), astContext))
      return false;
   // If value is an initialization of an array from a literal or an empty array
   // initializer, it need not be folded. Arrays constructed from literals use a
   // function with semantics: "array.uninitialized_intrinsic" that returns
   // a pair, where the first element of the pair is the array.
   PILInstruction *definingInst = value->getDefiningInstruction();
   if (!definingInst)
      return true;
   PILInstruction *constructorInst = definingInst;
   if (isa<DestructureTupleInst>(definingInst) ||
       isa<TupleExtractInst>(definingInst)) {
      constructorInst = definingInst->getOperand(0)->getDefiningInstruction();
   }
   if (!constructorInst || !isa<ApplyInst>(constructorInst))
      return true;
   PILFunction *callee = cast<ApplyInst>(constructorInst)->getCalleeFunction();
   return !callee ||
          (!callee->hasSemanticsAttr("array.init.empty") &&
           !callee->hasSemanticsAttr("array.uninitialized_intrinsic"));
}

/// Return true iff the given value is a closure but is not a creation of a
/// closure e.g., through partial_apply or thin_to_thick_function or
/// convert_function.
static bool isFoldableClosure(PILValue value) {
   return value->getType().is<PILFunctionType>() &&
          (!isa<FunctionRefInst>(value) && !isa<PartialApplyInst>(value) &&
           !isa<ThinToThickFunctionInst>(value) &&
           !isa<ConvertFunctionInst>(value));
}

/// Check whether a PILValue is foldable. String, integer, array and
/// function values are foldable with the following exceptions:
///  - Addresses cannot be folded.
///  - Literals need not be folded.
///  - Results of ownership instructions like load_borrow/copy_value need not
///  be folded
///  - Constructors such as \c struct Int or \c string.init() need not be folded.
static bool isPILValueFoldable(PILValue value) {
   PILInstruction *definingInst = value->getDefiningInstruction();
   if (!definingInst)
      return false;
   AstContext &astContext = definingInst->getFunction()->getAstContext();
   PILType silType = value->getType();
   return (!silType.isAddress() && !isa<LiteralInst>(definingInst) &&
           !isa<LoadBorrowInst>(definingInst) &&
           !isa<BeginBorrowInst>(definingInst) &&
           !isa<CopyValueInst>(definingInst) &&
           (isFoldableIntOrBool(value, astContext) ||
            isFoldableString(value, astContext) ||
            isFoldableArray(value, astContext) || isFoldableClosure(value)));
}

/// Diagnose failure during evaluation of a call to a constant-evaluable
/// function. Note that all auto-generated 'appendInterpolation' calls are
/// constant evaluable. This function detects and specially handles such
/// functions to present better diagnostic messages.
static void diagnoseErrorInConstantEvaluableFunction(ApplyInst *call,
                                                     SymbolicValue errorInfo) {
   PILNode *unknownNode = errorInfo.getUnknownNode();
   UnknownReason unknownReason = errorInfo.getUnknownReason();

   PILFunction *callee = call->getCalleeFunction();
   assert(callee);
   PILLocation loc = call->getLoc();
   SourceLoc sourceLoc = loc.getSourceLoc();
   AstContext &astContext = callee->getAstContext();

   std::string demangledCalleeName = demangling::demangleSymbolAsString(
      callee->getName(),
      demangling::DemangleOptions::SimplifiedUIDemangleOptions());

   // If an 'appendInterpolation' evaluation failed, it is probably due to
   // invalid privacy or format specifiers. These are the only possible errors
   // that the users of the log API could make. The rest are for library authors
   // or users who extend the log APIs.
   if (unknownReason.getKind() == UnknownReason::CallArgumentUnknown &&
       dyn_cast<ApplyInst>(unknownNode) == call) {
      if (StringRef(demangledCalleeName)
         .contains(astContext.Id_appendInterpolation.str())) {
         // TODO: extract and report the label of the parameter that is not a
         // constant.
         diagnose(astContext, sourceLoc,
                  diag::oslog_non_const_interpolation_options);
         return;
      }
   }
   diagnose(astContext, sourceLoc, diag::oslog_const_evaluable_fun_error,
            demangledCalleeName);
   errorInfo.emitUnknownDiagnosticNotes(loc);
   return;
}

/// Detect and emit diagnostics for errors found during evaluation. Errors
/// can happen due to incorrect implementation of the os log API in the
/// overlay or due to incorrect use of the os log API.
/// TODO: errors due to incorrect use of the API should be diagnosed by a
/// dedicated diagnostics pass that will happen before this optimization starts.
static bool detectAndDiagnoseErrors(SymbolicValue errorInfo,
                                    PILInstruction *unevaluableInst) {
   PILFunction *parentFun = unevaluableInst->getFunction();
   AstContext &astContext = parentFun->getAstContext();

   // If evaluation of any other constant_evaluable function call fails, point
   // to that failed function along with a reason: such as that a parameter is
   // non-constant parameter or that body is not constant evaluable.
   ApplyInst *call = dyn_cast<ApplyInst>(unevaluableInst);
   if (call) {
      PILFunction *callee = call->getCalleeFunction();
      if (callee && isConstantEvaluable(callee)) {
         diagnoseErrorInConstantEvaluableFunction(call, errorInfo);
         return true; // abort evaluation.
      }
   }

   // Every other error must happen in the body of the os_log function which
   // is inlined in the 'parentFun' before this pass. In this case, if we have a
   // fail-stop error, point to the error and abort evaluation. Otherwise, just
   // ignore the error and continue evaluation as this error might not affect the
   // constant value of the OSLogMessage instance.
   if (isFailStopError(errorInfo)) {
      assert(errorInfo.getKind() == SymbolicValue::Unknown);
      PILLocation loc = unevaluableInst->getLoc();
      SourceLoc sourceLoc = loc.getSourceLoc();
      diagnose(astContext, sourceLoc, diag::oslog_fail_stop_error);
      errorInfo.emitUnknownDiagnosticNotes(loc);
      return true;
   }
   return false;
}

/// Given a 'foldState', constant evaluate instructions from
/// 'foldState.beginInstruction' until an instruction in
/// 'foldState.endInstructions' is seen. Add foldable, constant-valued
/// instructions discovered during the evaluation to
/// 'foldState.constantPILValues'.
/// \returns error information if the evaluation failed.
static Optional<SymbolicValue> collectConstants(FoldState &foldState) {

   ConstExprStepEvaluator &constantEvaluator = foldState.constantEvaluator;
   PILBasicBlock::iterator currI = foldState.beginInstruction->getIterator();
   auto &endInstructions = foldState.endInstructions;

   // The loop will break when it sees a return instruction or an instruction in
   // endInstructions.
   while (true) {
      PILInstruction *currInst = &(*currI);
      if (endInstructions.count(currInst))
         break;

      // Initialize string info from this instruction if possible.
      foldState.stringInfo.extractStringInfoFromInstruction(currInst);

      Optional<SymbolicValue> errorInfo = None;
      Optional<PILBasicBlock::iterator> nextI = None;

      std::tie(nextI, errorInfo) = evaluateOrSkip(constantEvaluator, currI);

      // If the evaluation of this instruction failed, check whether it should be
      // diagnosed and reported. If so, abort evaluation. Otherwise, continue
      // evaluation if possible as this error could be due to an instruction that
      // doesn't affect the OSLogMessage value.
      if (errorInfo && detectAndDiagnoseErrors(errorInfo.getValue(), currInst)) {
         return errorInfo;
      }

      if (!nextI) {
         // We cannnot find the next instruction to continue evaluation, and we
         // haven't seen any reportable errors during evaluation. Therefore,
         // consider this the end point of evaluation.
         return None; // No error.
      }

      // Set the next instruction to continue evaluation from.
      currI = nextI.getValue();

      // If the instruction results are foldable and if we found a constant value
      // for the results, record it.
      for (PILValue instructionResult : currInst->getResults()) {
         if (!isPILValueFoldable(instructionResult))
            continue;

         Optional<SymbolicValue> constantVal =
            constantEvaluator.lookupConstValue(instructionResult);
         if (constantVal.hasValue()) {
            foldState.addConstantPILValue(instructionResult);
         }
      }
   }
   return None; // No error.
}

/// Generate PIL code to create an array of constant size from the given
/// PILValues \p elements. This function creates the same sequence of PIL
/// instructions that would be generated for initializing an array from an array
/// literal of the form [element1, element2, ..., elementn].
///
/// \param elements PILValues that the array should contain
/// \param arrayType the type of the array that must be created.
/// \param builder PILBuilder that provides the context for emitting the code
/// for the array.
/// \param loc PILLocation to use in the emitted instructions.
/// \return the PILValue of the array that is created with the given \c
/// elements.
static PILValue emitCodeForConstantArray(ArrayRef<PILValue> elements,
                                         CanType arrayType, PILBuilder &builder,
                                         PILLocation loc) {
   AstContext &astContext = builder.getAstContext();
   assert(astContext.getArrayDecl() ==
          arrayType->getNominalOrBoundGenericNominal());
   PILModule &module = builder.getModule();

   // Create a PILValue for the number of elements.
   unsigned numElements = elements.size();
   PILValue numElementsPIL = builder.createIntegerLiteral(
      loc, PILType::getBuiltinWordType(astContext), numElements);

   // Find the PILFunction that corresponds to _allocateUninitializedArray.
   FuncDecl *arrayAllocateDecl = astContext.getAllocateUninitializedArray();
   assert(arrayAllocateDecl);
   std::string allocatorMangledName =
      PILDeclRef(arrayAllocateDecl, PILDeclRef::Kind::Func).mangle();
   PILFunction *arrayAllocateFun =
      module.findFunction(allocatorMangledName, PILLinkage::PublicExternal);
   assert(arrayAllocateFun);

   // Call the _allocateUninitializedArray function with numElementsPIL. The
   // call returns a two-element tuple, where the first element is the newly
   // created array and the second element is a pointer to the internal storage
   // of the array.
   SubstitutionMap subMap = arrayType->getContextSubstitutionMap(
      module.getTypePHPModule(), astContext.getArrayDecl());
   FunctionRefInst *arrayAllocateRef =
      builder.createFunctionRef(loc, arrayAllocateFun);
   ApplyInst *applyInst = builder.createApply(
      loc, arrayAllocateRef, subMap, ArrayRef<PILValue>(numElementsPIL), false);

   // Extract the elements of the tuple returned by the call to the allocator.
   DestructureTupleInst *destructureInst =
      builder.createDestructureTuple(loc, applyInst);
   PILValue arrayPIL = destructureInst->getResults()[0];
   PILValue storagePointerPIL = destructureInst->getResults()[1];

   if (elements.empty()) {
      // Nothing more to be done if we are creating an empty array.
      return arrayPIL;
   }

   // Convert the pointer to the storage to an address. The elements will be
   // stored into offsets from this address.
   PILType elementPILType = elements[0]->getType();
   PointerToAddressInst *storageAddr = builder.createPointerToAddress(
      loc, storagePointerPIL, elementPILType.getAddressType(),
      /*isStrict*/ true,
      /*isInvariant*/ false);

   // Iterate over the elements and store them into the storage address
   // after offsetting it appropriately.

   // Create a TypeLowering for emitting stores. Note that TypeLowering
   // provides a utility for emitting stores for storing trivial and
   // non-trivial values, and also handles OSSA and non-OSSA.
   const TypeLowering &elementTypeLowering =
      builder.getTypeLowering(elementPILType);

   unsigned elementIndex = 0;
   for (PILValue elementPIL : elements) {
      // Compute the address where the element must be stored.
      PILValue currentStorageAddr;
      if (elementIndex != 0) {
         PILValue indexPIL = builder.createIntegerLiteral(
            loc, PILType::getBuiltinWordType(astContext), elementIndex);
         currentStorageAddr = builder.createIndexAddr(loc, storageAddr, indexPIL);
      } else {
         currentStorageAddr = storageAddr;
      }
      // Store the generated element into the currentStorageAddr. This is an
      // initializing store and therefore there is no need to free any existing
      // element.
      elementTypeLowering.emitStore(builder, loc, elementPIL, currentStorageAddr,
                                    StoreOwnershipQualifier::Init);
      elementIndex++;
   }
   return arrayPIL;
}

/// Given a PILValue \p value, return the instruction immediately following the
/// definition of the value. That is, if the value is defined by an
/// instruction, return the instruction following the definition. Otherwise, if
/// the value is a basic block parameter, return the first instruction of the
/// basic block.
PILInstruction *getInstructionFollowingValueDefinition(PILValue value) {
   PILInstruction *definingInst = value->getDefiningInstruction();
   if (definingInst) {
      return &*std::next(definingInst->getIterator());
   }
   // Here value must be a basic block argument.
   PILBasicBlock *bb = value->getParentBlock();
   return &*bb->begin();
}

/// Given a PILValue \p value, create a copy of the value using copy_value in
/// OSSA or retain in non-OSSA, if \p value is a non-trivial type. Otherwise, if
/// \p value is a trivial type, return the value itself.
PILValue makeOwnedCopyOfPILValue(PILValue value, PILFunction &fun) {
   PILType type = value->getType();
   if (type.isTrivial(fun))
      return value;
   assert(!type.isAddress() && "cannot make owned copy of addresses");

   PILInstruction *instAfterValueDefinition =
      getInstructionFollowingValueDefinition(value);
   PILLocation copyLoc = instAfterValueDefinition->getLoc();
   PILBuilderWithScope builder(instAfterValueDefinition);
   const TypeLowering &typeLowering = builder.getTypeLowering(type);
   PILValue copy = typeLowering.emitCopyValue(builder, copyLoc, value);
   return copy;
}

/// Generate PIL code that computes the constant given by the symbolic value
/// `symVal`. Note that strings and struct-typed constant values will require
/// multiple instructions to be emitted.
/// \param symVal symbolic value for which PIL code needs to be emitted.
/// \param expectedType the expected type of the instruction that would be
/// computing the symbolic value `symVal`. The type is accepted as a
/// parameter as some symbolic values like integer constants can inhabit more
/// than one type.
/// \param builder PILBuilder that provides the context for emitting the code
/// for the symbolic value
/// \param loc PILLocation to use in the emitted instructions.
/// \param stringInfo String.init and metatype information for generating code
/// for string literals.
static PILValue emitCodeForSymbolicValue(SymbolicValue symVal,
                                         Type expectedType, PILBuilder &builder,
                                         PILLocation &loc,
                                         StringPILInfo &stringInfo) {
   AstContext &astContext = expectedType->getAstContext();

   switch (symVal.getKind()) {
      case SymbolicValue::String: {
         assert(astContext.getStringDecl() ==
                expectedType->getNominalOrBoundGenericNominal());

         StringRef stringVal = symVal.getStringValue();
         StringLiteralInst *stringLitInst = builder.createStringLiteral(
            loc, stringVal, StringLiteralInst::Encoding::UTF8);

         // Create a builtin word for the size of the string
         IntegerLiteralInst *sizeInst = builder.createIntegerLiteral(
            loc, PILType::getBuiltinWordType(astContext), stringVal.size());
         // Set isAscii to false.
         IntegerLiteralInst *isAscii = builder.createIntegerLiteral(
            loc, PILType::getBuiltinIntegerType(1, astContext), 0);
         // Create a metatype inst.
         MetatypeInst *metatypeInst =
            builder.createMetatype(loc, stringInfo.getStringMetatype());

         auto args = SmallVector<PILValue, 4>();
         args.push_back(stringLitInst);
         args.push_back(sizeInst);
         args.push_back(isAscii);
         args.push_back(metatypeInst);

         FunctionRefInst *stringInitRef =
            builder.createFunctionRef(loc, stringInfo.getStringInitIntrinsic());
         ApplyInst *applyInst = builder.createApply(
            loc, stringInitRef, SubstitutionMap(), ArrayRef<PILValue>(args), false);
         return applyInst;
      }
      case SymbolicValue::Integer: { // Builtin integer types.
         APInt resInt = symVal.getIntegerValue();
         assert(expectedType->is<BuiltinIntegerType>());

         PILType builtinIntType =
            PILType::getPrimitiveObjectType(expectedType->getCanonicalType());
         IntegerLiteralInst *intLiteralInst =
            builder.createIntegerLiteral(loc, builtinIntType, resInt);
         return intLiteralInst;
      }
      case SymbolicValue::Aggregate: {
         // Support only stdlib integer or bool structs.
         StructDecl *structDecl = expectedType->getStructOrBoundGenericStruct();
         assert(structDecl);
         assert(isStdlibIntegerOrBoolDecl(structDecl, astContext));
         assert(symVal.getAggregateType()->isEqual(expectedType) &&
                "aggregate symbolic value's type and expected type do not match");

         VarDecl *propertyDecl = structDecl->getStoredProperties().front();
         Type propertyType = expectedType->getTypeOfMember(
            propertyDecl->getModuleContext(), propertyDecl);
         SymbolicValue propertyVal = symVal.lookThroughSingleElementAggregates();
         PILValue newPropertyPIL = emitCodeForSymbolicValue(
            propertyVal, propertyType, builder, loc, stringInfo);
         // The lowered PIL type of an integer/bool type is just the primitive
         // object type containing the Swift type.
         PILType aggregateType =
            PILType::getPrimitiveObjectType(expectedType->getCanonicalType());
         StructInst *newStructInst = builder.createStruct(
            loc, aggregateType, ArrayRef<PILValue>(newPropertyPIL));
         return newStructInst;
      }
      case SymbolicValue::Array: {
         assert(expectedType->isEqual(symVal.getArrayType()));
         CanType elementType;
         ArrayRef<SymbolicValue> arrayElements =
            symVal.getStorageOfArray().getStoredElements(elementType);

         // Emit code for the symbolic values corresponding to the array elements.
         SmallVector<PILValue, 8> elementPILValues;
         for (SymbolicValue elementSymVal : arrayElements) {
            PILValue elementPIL = emitCodeForSymbolicValue(elementSymVal, elementType,
                                                           builder, loc, stringInfo);
            elementPILValues.push_back(elementPIL);
         }
         PILValue arrayPIL = emitCodeForConstantArray(
            elementPILValues, expectedType->getCanonicalType(), builder, loc);
         return arrayPIL;
      }
      case SymbolicValue::Closure: {
         assert(expectedType->is<AnyFunctionType>() ||
                expectedType->is<PILFunctionType>());

         SymbolicClosure *closure = symVal.getClosure();
         SubstitutionMap callSubstMap = closure->getCallSubstitutionMap();
         PILModule &module = builder.getModule();
         ArrayRef<SymbolicClosureArgument> captures = closure->getCaptures();

         // Recursively emit code for all captured values that are mapped to a
         // symbolic value. If there is a captured value that is not mapped
         // to a symbolic value, use the captured value as such (after possibly
         // copying non-trivial captures).
         SmallVector<PILValue, 4> capturedPILVals;
         for (SymbolicClosureArgument capture : captures) {
            PILValue captureOperand = capture.first;
            Optional<SymbolicValue> captureSymVal = capture.second;
            if (!captureSymVal) {
               PILFunction &fun = builder.getFunction();
               assert(captureOperand->getFunction() == &fun &&
                      "non-constant captured arugment not defined in this function");
               // If the captureOperand is a non-trivial value, it should be copied
               // as it now used in a new folded closure.
               PILValue captureCopy = makeOwnedCopyOfPILValue(captureOperand, fun);
               capturedPILVals.push_back(captureCopy);
               continue;
            }
            // Here, we have a symbolic value for the capture. Therefore, use it to
            // create a new constant at this point. Note that the captured operand
            // type may have generic parameters which has to be substituted with the
            // substitution map that was inferred by the constant evaluator at the
            // partial-apply site.
            PILType operandType = captureOperand->getType();
            PILType captureType = operandType.subst(module, callSubstMap);
            PILValue capturePILVal = emitCodeForSymbolicValue(
               captureSymVal.getValue(), captureType.getAstType(), builder, loc,
               stringInfo);
            capturedPILVals.push_back(capturePILVal);
         }

         FunctionRefInst *functionRef =
            builder.createFunctionRef(loc, closure->getTarget());
         PILType closureType = closure->getClosureType();
         ParameterConvention convention =
            closureType.getAs<PILFunctionType>()->getCalleeConvention();
         PartialApplyInst *papply = builder.createPartialApply(
            loc, functionRef, callSubstMap, capturedPILVals, convention);
         // The type of the created closure must be a lowering of the expected type.
         PILType resultType = papply->getType();
         CanType expectedCanType = expectedType->getCanonicalType();
         assert(expectedType->is<PILFunctionType>()
                ? resultType.getAstType() == expectedCanType
                : resultType.is<PILFunctionType>());
         return papply;
      }
      default: {
         llvm_unreachable("Symbolic value kind is not supported");
      }
   }
}

/// Collect the end points of the instructions that are data dependent on \c
/// value. A instruction is data dependent on \c value if its result may
/// transitively depends on \c value. Note that data dependencies through
/// addresses are not tracked by this function.
///
/// \param value PILValue that is not an address.
/// \param fun PILFunction that defines \c value.
/// \param endUsers buffer for storing the found end points of the data
/// dependence chain.
static void
getEndPointsOfDataDependentChain(PILValue value, PILFunction *fun,
                                 SmallVectorImpl<PILInstruction *> &endUsers) {
   assert(!value->getType().isAddress());

   // Collect the instructions that are data dependent on the value using a
   // fix point iteration.
   SmallPtrSet<PILInstruction *, 16> visitedUsers;
   SmallVector<PILValue, 16> worklist;
   worklist.push_back(value);

   while (!worklist.empty()) {
      PILValue currVal = worklist.pop_back_val();
      for (Operand *use : currVal->getUses()) {
         PILInstruction *user = use->getUser();
         if (visitedUsers.count(user))
            continue;
         visitedUsers.insert(user);
         llvm::copy(user->getResults(), std::back_inserter(worklist));
      }
   }

   // At this point, visitedUsers have all the transitive, data-dependent uses.
   // Compute the lifetime frontier of all the uses which are the instructions
   // following the last uses. Every exit from the last uses will have a
   // lifetime frontier.
   PILInstruction *valueDefinition = value->getDefiningInstruction();
   PILInstruction *def =
      valueDefinition ? valueDefinition : &(value->getParentBlock()->front());
   ValueLifetimeAnalysis lifetimeAnalysis =
      ValueLifetimeAnalysis(def, SmallVector<PILInstruction *, 16>(
         visitedUsers.begin(), visitedUsers.end()));
   ValueLifetimeAnalysis::Frontier frontier;
   bool hasCriticlEdges = lifetimeAnalysis.computeFrontier(
      frontier, ValueLifetimeAnalysis::DontModifyCFG);
   endUsers.append(frontier.begin(), frontier.end());
   if (!hasCriticlEdges)
      return;
   // If there are some lifetime frontiers on the critical edges, take the
   // first instruction of the target of the critical edge as the frontier. This
   // will suffice as every exit from the visitedUsers must go through one of
   // them.
   for (auto edgeIndexPair : lifetimeAnalysis.getCriticalEdges()) {
      PILBasicBlock *targetBB =
         edgeIndexPair.first->getSuccessors()[edgeIndexPair.second];
      endUsers.push_back(&targetBB->front());
   }
}

/// Given a guaranteed PILValue \p value, return a borrow-scope introducing
/// value, if there is exactly one such introducing value. Otherwise, return
/// None. There can be multiple borrow scopes for a PILValue iff it is derived
/// from a guaranteed basic block parameter representing a phi node.
static Optional<BorrowScopeIntroducingValue>
getUniqueBorrowScopeIntroducingValue(PILValue value) {
   assert(value.getOwnershipKind() == ValueOwnershipKind::Guaranteed &&
          "parameter must be a guarenteed value");
   SmallVector<BorrowScopeIntroducingValue, 4> borrowIntroducers;
   getUnderlyingBorrowIntroducingValues(value, borrowIntroducers);
   assert(borrowIntroducers.size() > 0 &&
          "folding guaranteed value with no borrow introducer");
   if (borrowIntroducers.size() > 1)
      return None;
   return borrowIntroducers[0];
}

/// Replace all uses of \c originalVal by \c foldedVal and adjust lifetimes of
/// original and folded values by emitting required destory/release instructions
/// at the right places. Note that this function does not remove any
/// instruction.
///
/// \param originalVal the PIL value that is replaced.
/// \param foldedVal the PIL value that replaces the \c originalVal.
/// \param fun the PIL function containing the \c foldedVal and \c originalVal
static void replaceAllUsesAndFixLifetimes(PILValue foldedVal,
                                          PILValue originalVal,
                                          PILFunction *fun) {
   PILInstruction *originalInst = originalVal->getDefiningInstruction();
   PILInstruction *foldedInst = foldedVal->getDefiningInstruction();
   assert(originalInst &&
             "cannot constant fold function or basic block parameter");
   assert(!isa<TermInst>(originalInst) &&
          "cannot constant fold a terminator instruction");
   assert(foldedInst && "constant value does not have a defining instruction");

   if (originalVal->getType().isTrivial(*fun)) {
      assert(foldedVal->getType().isTrivial(*fun));
      // Just replace originalVal by foldedVal.
      originalVal->replaceAllUsesWith(foldedVal);
      return;
   }
   assert(!foldedVal->getType().isTrivial(*fun));
   assert(fun->hasOwnership());
   assert(foldedVal.getOwnershipKind() == ValueOwnershipKind::Owned &&
          "constant value must have owned ownership kind");

   if (originalVal.getOwnershipKind() == ValueOwnershipKind::Owned) {
      originalVal->replaceAllUsesWith(foldedVal);
      // Destroy originalVal, which is now unused, immediately after its
      // definition. Note that originalVal's destorys are now transferred to
      // foldedVal.
      PILInstruction *insertionPoint = &(*std::next(originalInst->getIterator()));
      PILBuilderWithScope builder(insertionPoint);
      PILLocation loc = insertionPoint->getLoc();
      builder.emitDestroyValueOperation(loc, originalVal);
      return;
   }

   // Here, originalVal is guaranteed. It must belong to a borrow scope that
   // begins at a scope introducing instruction e.g. begin_borrow or load_borrow.
   // The foldedVal should also have been inserted at the beginning of the scope.
   // Therefore, create a borrow of foldedVal at the beginning of the scope and
   // use the borrow in place of the originalVal. Also, end the borrow and
   // destroy foldedVal at the end of the borrow scope.
   assert(originalVal.getOwnershipKind() == ValueOwnershipKind::Guaranteed);

   Optional<BorrowScopeIntroducingValue> originalScopeBegin =
      getUniqueBorrowScopeIntroducingValue(originalVal);
   assert(originalScopeBegin &&
             "value without a unique borrow scope should not have been folded");
   PILInstruction *scopeBeginInst =
      originalScopeBegin->value->getDefiningInstruction();
   assert(scopeBeginInst);

   PILBuilderWithScope builder(scopeBeginInst);
   PILValue borrow =
      builder.emitBeginBorrowOperation(scopeBeginInst->getLoc(), foldedVal);

   originalVal->replaceAllUsesWith(borrow);

   SmallVector<PILInstruction *, 4> scopeEndingInsts;
   originalScopeBegin->getLocalScopeEndingInstructions(scopeEndingInsts);

   for (PILInstruction *scopeEndingInst : scopeEndingInsts) {
      PILBuilderWithScope builder(scopeEndingInst);
      builder.emitEndBorrowOperation(scopeEndingInst->getLoc(), borrow);
      builder.emitDestroyValueOperation(scopeEndingInst->getLoc(), foldedVal);
   }
   return;
}

/// Given a fold state with constant-valued instructions, substitute the
/// instructions with the constant values. The constant values could be strings
/// or Stdlib integer-struct values or builtin integers.
static void substituteConstants(FoldState &foldState) {
   ConstExprStepEvaluator &evaluator = foldState.constantEvaluator;
   // Instructions that are possibly dead since their results are folded.
   SmallVector<PILInstruction *, 4> possiblyDeadInsts;

   for (PILValue constantPILValue : foldState.getConstantPILValues()) {
      SymbolicValue constantSymbolicVal =
         evaluator.lookupConstValue(constantPILValue).getValue();

      PILInstruction *definingInst = constantPILValue->getDefiningInstruction();
      assert(definingInst);
      PILFunction *fun = definingInst->getFunction();

      // Find an insertion point for inserting the new constant value. If we are
      // folding a value like struct_extract within a borrow scope, we need to
      // insert the constant value at the beginning of the borrow scope. This
      // is because the borrowed value is expected to be alive during its entire
      // borrow scope and could be stored into memory and accessed indirectly
      // without a copy e.g. using store_borrow within the borrow scope. On the
      // other hand, if we are folding an owned value, we can insert the constant
      // value at the point where the owned value is defined.
      PILInstruction *insertionPoint = definingInst;
      if (constantPILValue.getOwnershipKind() == ValueOwnershipKind::Guaranteed) {
         Optional<BorrowScopeIntroducingValue> borrowIntroducer =
            getUniqueBorrowScopeIntroducingValue(constantPILValue);
         if (!borrowIntroducer) {
            // This case happens only if constantPILValue is derived from a
            // guaranteed basic block parameter. This is unlikley because the values
            // that have to be folded should just be a struct-extract of an owned
            // instance of OSLogMessage.
            continue;
         }
         insertionPoint = borrowIntroducer->value->getDefiningInstruction();
         assert(insertionPoint && "borrow scope begnning is a parameter");
      }

      PILBuilderWithScope builder(insertionPoint);
      PILLocation loc = insertionPoint->getLoc();
      CanType instType = constantPILValue->getType().getAstType();
      PILValue foldedPILVal = emitCodeForSymbolicValue(
         constantSymbolicVal, instType, builder, loc, foldState.stringInfo);

      // Replace constantPILValue with foldedPILVal and adjust the lifetime and
      // ownership of the values appropriately.
      replaceAllUsesAndFixLifetimes(foldedPILVal, constantPILValue, fun);
      possiblyDeadInsts.push_back(definingInst);
   }
   recursivelyDeleteTriviallyDeadInstructions(possiblyDeadInsts, /*force*/ false,
                                              [&](PILInstruction *DeadI) {});
}

/// Check whether OSLogMessage and OSLogInterpolation instances and all their
/// stored properties are constants. If not, it indicates errors that are due to
/// incorrect implementation OSLogMessage either in the overlay or in the
/// extensions created by users. Detect and emit diagnostics for such errors.
/// The diagnostics here are for os log library authors.
static bool checkOSLogMessageIsConstant(SingleValueInstruction *osLogMessage,
                                        FoldState &foldState) {
   ConstExprStepEvaluator &constantEvaluator = foldState.constantEvaluator;
   PILLocation loc = osLogMessage->getLoc();
   SourceLoc sourceLoc = loc.getSourceLoc();
   PILFunction *fn = osLogMessage->getFunction();
   PILModule &module = fn->getModule();
   AstContext &astContext = fn->getAstContext();

   Optional<SymbolicValue> osLogMessageValueOpt =
      constantEvaluator.lookupConstValue(osLogMessage);
   if (!osLogMessageValueOpt ||
       osLogMessageValueOpt->getKind() != SymbolicValue::Aggregate) {
      diagnose(astContext, sourceLoc, diag::oslog_non_constant_message);
      return true;
   }

   // The first (and only) property of OSLogMessage is the OSLogInterpolation
   // instance.
   SymbolicValue osLogInterpolationValue =
      osLogMessageValueOpt->getAggregateMembers()[0];
   if (!osLogInterpolationValue.isConstant()) {
      diagnose(astContext, sourceLoc, diag::oslog_non_constant_interpolation);
      return true;
   }

   // Check if every proprety of the OSLogInterpolation instance has a constant
   // value.
   PILType osLogMessageType = osLogMessage->getType();
   StructDecl *structDecl = osLogMessageType.getStructOrBoundGenericStruct();
   assert(structDecl);

   auto typeExpansionContext =
      TypeExpansionContext(*osLogMessage->getFunction());
   VarDecl *interpolationPropDecl = structDecl->getStoredProperties().front();
   PILType osLogInterpolationType = osLogMessageType.getFieldType(
      interpolationPropDecl, module, typeExpansionContext);
   StructDecl *interpolationStruct =
      osLogInterpolationType.getStructOrBoundGenericStruct();
   assert(interpolationStruct);

   auto propertyDecls = interpolationStruct->getStoredProperties();
   ArrayRef<SymbolicValue> propertyValues =
      osLogInterpolationValue.getAggregateMembers();
   auto propValueI = propertyValues.begin();
   bool errorDetected = false;

   for (auto *propDecl : propertyDecls) {
      SymbolicValue propertyValue = *(propValueI++);
      if (!propertyValue.isConstant()) {
         diagnose(astContext, sourceLoc, diag::oslog_property_not_constant,
                  propDecl->getNameStr());
         errorDetected = true;
         break;
      }
   }
   return errorDetected;
}

/// Constant evaluate instructions starting from 'start' and fold the uses
/// of the value 'oslogMessage'. Stop when oslogMessageValue is released.
static bool constantFold(PILInstruction *start,
                         SingleValueInstruction *oslogMessage,
                         unsigned assertConfig) {
   PILFunction *fun = start->getFunction();
   assert(fun->hasOwnership() && "function not in ownership PIL");

   // Initialize fold state.
   SmallVector<PILInstruction *, 2> endUsersOfOSLogMessage;
   getEndPointsOfDataDependentChain(oslogMessage, fun, endUsersOfOSLogMessage);
   assert(!endUsersOfOSLogMessage.empty());

   FoldState state(fun, assertConfig, start, endUsersOfOSLogMessage);

   auto errorInfo = collectConstants(state);
   if (errorInfo) // Evaluation failed with diagnostics.
      return false;

   // At this point, the `OSLogMessage` instance should be mapped to a constant
   // value in the interpreter state. If this is not the case, it means the
   // overlay implementation of OSLogMessage (or its extensions by users) are
   // incorrect. Detect and diagnose this scenario.
   bool errorDetected = checkOSLogMessageIsConstant(oslogMessage, state);
   if (errorDetected)
      return false;

   substituteConstants(state);
   return true;
}

/// Given a call to the initializer of OSLogMessage, which conforms to
/// 'ExpressibleByStringInterpolation', find the first instruction, if any, that
/// marks the begining of the string interpolation that is used to create an
/// OSLogMessage instance. This function traverses the backward data-dependence
/// chain of the given OSLogMessage initializer: \p oslogInit. As a special case
/// it avoids chasing the data-dependencies from the captured values of
/// partial-apply instructions, as a partial apply instruction is considered as
/// a constant regardless of the constantness of its captures.
static PILInstruction *beginOfInterpolation(ApplyInst *oslogInit) {
   auto oslogInitCallSite = FullApplySite(oslogInit);
   PILFunction *callee = oslogInitCallSite.getCalleeFunction();

   assert (callee->hasSemanticsAttrThatStartsWith("oslog.message.init"));
   // The initializer must return the OSLogMessage instance directly.
   assert(oslogInitCallSite.getNumArguments() >= 1 &&
          oslogInitCallSite.getNumIndirectPILResults() == 0);

   // List of backward dependencies that needs to be analyzed.
   SmallVector<PILInstruction *, 4> worklist = { oslogInit };
   SmallPtrSet<PILInstruction *, 4> seenInstructions = { oslogInit };
   // List of instructions that could potentially mark the beginning of the
   // interpolation.
   SmallPtrSet<PILInstruction *, 4> candidateStartInstructions;

   unsigned i = 0;
   while (i < worklist.size()) {
      PILInstruction *inst = worklist[i++];

      if (isa<PartialApplyInst>(inst)) {
         // Partial applies are used to capture the dynamic arguments passed to
         // the string interpolation. Their arguments are not required to be
         // known at compile time and they need not be constant evaluated.
         // Therefore, follow only the dependency chain along function ref operand.
         PILInstruction *definingInstruction =
            inst->getOperand(0)->getDefiningInstruction();
         assert(definingInstruction && "no function-ref operand in partial-apply");
         if (seenInstructions.insert(definingInstruction).second) {
            worklist.push_back(definingInstruction);
            candidateStartInstructions.insert(definingInstruction);
         }
         continue;
      }

      for (Operand &operand : inst->getAllOperands()) {
         if (PILInstruction *definingInstruction =
                operand.get()->getDefiningInstruction()) {
            if (seenInstructions.count(definingInstruction))
               continue;
            worklist.push_back(definingInstruction);
            seenInstructions.insert(definingInstruction);
            candidateStartInstructions.insert(definingInstruction);
         }
         // If there is no definining instruction for this operand, it could be a
         // basic block or function parameter. Such operands are not considered
         // in the backward slice. Dependencies through them are safe to ignore
         // in this context.
      }

      // If the instruction: `inst` has an operand, its definition should precede
      // `inst` in the control-flow order. Therefore, remove `inst` from the
      // candidate start instructions.
      if (inst->getNumOperands() > 0) {
         candidateStartInstructions.erase(inst);
      }

      if (!isa<AllocStackInst>(inst)) {
         continue;
      }

      // If we have an alloc_stack instruction, include stores into it into the
      // backward dependency list. However, whether alloc_stack precedes the
      // definitions of values stored into the location in the control-flow order
      // can only be determined by traversing the instrutions in the control-flow
      // order.
      AllocStackInst *allocStackInst = cast<AllocStackInst>(inst);
      for (StoreInst *storeInst : allocStackInst->getUsersOfType<StoreInst>()) {
         worklist.push_back(storeInst);
         candidateStartInstructions.insert(storeInst);
      }
   }

   // Find the first basic block in the control-flow order. Typically, if
   // formatting and privacy options are literals, all candidate instructions
   // must be in the same basic block. But, this code doesn't rely on that
   // assumption.
   SmallPtrSet<PILBasicBlock *, 4> candidateBBs;
   for (auto *candidate: candidateStartInstructions) {
      PILBasicBlock *candidateBB = candidate->getParent();
      candidateBBs.insert(candidateBB);
   }

   PILBasicBlock *firstBB = nullptr;
   PILBasicBlock *entryBB = oslogInit->getFunction()->getEntryBlock();
   for (PILBasicBlock *bb: llvm::breadth_first<PILBasicBlock *>(entryBB)) {
      if (candidateBBs.count(bb)) {
         firstBB = bb;
         break;
      }
   }
   assert(firstBB);

   // Iterate over the instructions in the firstBB and find the instruction that
   // starts the interpolation.
   PILInstruction *startInst = nullptr;
   for (PILInstruction &inst : *firstBB) {
      if (candidateStartInstructions.count(&inst)) {
         startInst = &inst;
         break;
      }
   }
   assert(startInst);
   return startInst;
}

/// If the PILInstruction is an initialization of OSLogMessage, return the
/// initialization call as an ApplyInst. Otherwise, return nullptr.
static ApplyInst *getAsOSLogMessageInit(PILInstruction *inst) {
   auto *applyInst = dyn_cast<ApplyInst>(inst);
   if (!applyInst) {
      return nullptr;
   }

   PILFunction *callee = applyInst->getCalleeFunction();
   if (!callee ||
       !callee->hasSemanticsAttrThatStartsWith("oslog.message.init")) {
      return nullptr;
   }

   // Default argument generators created for a function also inherit
   // the semantics attribute of the function. Therefore, check that there are
   // at least two operands for this apply instruction.
   if (applyInst->getNumOperands() > 1) {
      return applyInst;
   }
   return nullptr;
}

/// Return true iff the PIL function \c fun is a method of the \c OSLogMessage
/// type.
bool isMethodOfOSLogMessage(PILFunction &fun) {
   DeclContext *declContext = fun.getDeclContext();
   if (!declContext)
      return false;
   Decl *decl = declContext->getAsDecl();
   if (!decl)
      return false;
   ConstructorDecl *ctor = dyn_cast<ConstructorDecl>(decl);
   if (!ctor)
      return false;
   DeclContext *parentContext = ctor->getParent();
   if (!parentContext)
      return false;
   NominalTypeDecl *typeDecl = parentContext->getSelfNominalTypeDecl();
   if (!typeDecl)
      return false;
   return typeDecl->getName() == fun.getAstContext().Id_OSLogMessage;
}

class OSLogOptimization : public PILFunctionTransform {

   ~OSLogOptimization() override {}

   /// The entry point to the transformation.
   void run() override {
      auto &fun = *getFunction();
      unsigned assertConfig = getOptions().AssertConfig;

      // Don't rerun optimization on deserialized functions or stdlib functions.
      if (fun.wasDeserializedCanonical()) {
         return;
      }

      // Skip methods of OSLogMessage type. This avoid unnecessary work and also
      // avoids falsely diagnosing the auto-generated (transparent) witness method
      // of OSLogMessage, which ends up invoking the OSLogMessage initializer:
      // "oslog.message.init_interpolation" without an interpolated string
      // literal that is expected by this pass.
      if (isMethodOfOSLogMessage(fun)) {
         return;
      }

      // Collect all 'OSLogMessage.init' in the function. 'OSLogMessage' is a
      // custom string interpolation type used by the new OS log APIs.
      SmallVector<ApplyInst *, 4> oslogMessageInits;
      for (auto &bb : fun) {
         for (auto &inst : bb) {
            auto init = getAsOSLogMessageInit(&inst);
            if (!init)
               continue;
            oslogMessageInits.push_back(init);
         }
      }

      bool madeChange = false;

      // Constant fold the uses of properties of OSLogMessage instance. Note that
      // the function body will change due to constant folding, after each
      // iteration.
      for (auto *oslogInit : oslogMessageInits) {
         PILInstruction *interpolationStart = beginOfInterpolation(oslogInit);
         assert(interpolationStart);
         madeChange |= constantFold(interpolationStart, oslogInit, assertConfig);
      }

      // TODO: Can we be more conservative here with our invalidation?
      if (madeChange) {
         invalidateAnalysis(PILAnalysis::InvalidationKind::FunctionBody);
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createOSLogOptimization() {
   return new OSLogOptimization();
}
