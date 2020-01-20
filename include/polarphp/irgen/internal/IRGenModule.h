//===--- IRGenModule.h - Swift Global IR Generation Module ------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the interface used
// the AST into LLVM IR.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_IRGENMODULE_H
#define POLARPHP_IRGEN_INTERNAL_IRGENMODULE_H

#include "polarphp/irgen/internal/IRGen.h"
#include "polarphp/irgen/internal/PolarphpTargetInfo.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/ReferenceCounting.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/basic/ClusteredBitVector.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/OptimizationMode.h"
#include "polarphp/basic/SuccessorMap.h"
#include "polarphp/irgen/ValueWitness.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Target/TargetMachine.h"

#include <atomic>

namespace llvm {
class Constant;
class DataLayout;
class Function;
class FunctionType;
class GlobalVariable;
class InlineAsm;
class IntegerType;
class LLVMContext;
class MDNode;
class Metadata;
class Module;
class PointerType;
class StructType;
class StringRef;
class Type;
class AttributeList;
}
namespace clang {
class ASTContext;
template <class> class CanQual;
class CodeGenerator;
class Decl;
class GlobalDecl;
class Type;
namespace CodeGen {
class CGFunctionInfo;
class CodeGenModule;
}
}

namespace polar {
class GenericSignature;
class GenericSignatureBuilder;
class AssociatedConformance;
class AssociatedType;
class AstContext;
class BaseConformance;
class BraceStmt;
class CanType;
class LinkLibrary;
class PILFunction;
class IRGenOptions;
class NormalInterfaceConformance;
class InterfaceConformance;
class InterfaceCompositionType;
class RootInterfaceConformance;
struct PILDeclRef;
class PILDefaultWitnessTable;
class PILGlobalVariable;
class PILModule;
class PILProperty;
class PILType;
class PILWitnessTable;
class SourceLoc;
class SourceFile;
class Type;
enum class TypeReferenceKind : unsigned;

namespace Lowering {
class TypeConverter;
}

namespace irgen {
class Address;
class ClangTypeConverter;
class ClassMetadataLayout;
class ConformanceDescription;
class ConformanceInfo;
class ConstantInitBuilder;
struct ConstantIntegerLiteral;
class ConstantIntegerLiteralMap;
class DebugTypeInfo;
class EnumImplStrategy;
class EnumMetadataLayout;
class ExplosionSchema;
class FixedTypeInfo;
class ForeignClassMetadataLayout;
class ForeignFunctionInfo;
class FormalType;
class HeapLayout;
class StructLayout;
class IRGenDebugInfo;
class IRGenFunction;
class LinkEntity;
class LoadableTypeInfo;
class MetadataLayout;
class NecessaryBindings;
class NominalMetadataLayout;
class OutliningMetadataCollector;
class InterfaceInfo;
enum class InterfaceInfoKind : uint8_t;
class Signature;
class StructMetadataLayout;
struct SymbolicMangling;
struct GenericRequirement;
class TypeConverter;
class TypeInfo;
enum class TypeMetadataAddress;
enum class ValueWitness : unsigned;
enum class ClassMetadataStrategy;

class IRGenModule;

/// A type descriptor for a field type accessor.
class FieldTypeInfo {
   llvm::PointerIntPair<CanType, 2, unsigned> Info;
   /// Bits in the "int" part of the Info pair.
   enum : unsigned {
      /// Flag indicates that the case is indirectly stored in a box.
         Indirect = 1,
      /// Indicates a weak optional reference
         Weak = 2,
   };

   static unsigned getFlags(bool indirect, bool weak) {
      return (indirect ? Indirect : 0)
             | (weak ? Weak : 0);
      //   | (blah ? Blah : 0) ...
   }

public:
   FieldTypeInfo(CanType type, bool indirect, bool weak)
      : Info(type, getFlags(indirect, weak))
   {}

   CanType getType() const { return Info.getPointer(); }
   bool isIndirect() const { return Info.getInt() & Indirect; }
   bool isWeak() const { return Info.getInt() & Weak; }
   bool hasFlags() const { return Info.getInt() != 0; }
};

enum RequireMetadata_t : bool {
   DontRequireMetadata = false,
   RequireMetadata = true
};

/// The principal singleton which manages all of IR generation.
///
/// The IRGenerator delegates the emission of different top-level entities
/// to different instances of IRGenModule, each of which creates a different
/// llvm::Module.
///
/// In single-threaded compilation, the IRGenerator creates only a single
/// IRGenModule. In multi-threaded compilation, it contains multiple
/// IRGenModules - one for each LLVM module (= one for each input/output file).
class IRGenerator {
public:
   IRGenOptions &Opts;

   PILModule &PIL;

private:
   llvm::DenseMap<SourceFile *, IRGenModule *> GenModules;

   // Stores the IGM from which a function is referenced the first time.
   // It is used if a function has no source-file association.
   llvm::DenseMap<PILFunction *, IRGenModule *> DefaultIGMForFunction;

   // The IGM of the first source file.
   IRGenModule *PrimaryIGM = nullptr;

   // The current IGM for which IR is generated.
   IRGenModule *CurrentIGM = nullptr;

   /// If this is true, adding anything to the below queues is an error.
   bool FinishedEmittingLazyDefinitions = false;

   /// A map recording if metadata can be emitted lazily for each nominal type.
   llvm::DenseMap<TypeDecl *, bool> HasLazyMetadata;

   struct LazyTypeGlobalsInfo {
      /// Is there a use of the type metadata?
      bool IsMetadataUsed = false;

      /// Is there a use of the nominal type descriptor?
      bool IsDescriptorUsed = false;

      /// Have we already emitted type metadata?
      bool IsMetadataEmitted = false;

      /// Have we already emitted a type context descriptor?
      bool IsDescriptorEmitted = false;
   };

   /// The set of type metadata that have been enqueued for lazy emission.
   llvm::DenseMap<NominalTypeDecl *, LazyTypeGlobalsInfo> LazyTypeGlobals;

   /// The queue of lazy type metadata to emit.
   llvm::SmallVector<NominalTypeDecl *, 4> LazyTypeMetadata;

   /// The queue of lazy type context descriptors to emit.
   llvm::SmallVector<NominalTypeDecl *, 4> LazyTypeContextDescriptors;

   /// Field metadata records that have already been lazily emitted, or are
   /// queued up.
   llvm::SmallPtrSet<NominalTypeDecl *, 4> LazilyEmittedFieldMetadata;

   struct LazyOpaqueInfo {
      bool IsDescriptorUsed = false;
      bool IsDescriptorEmitted = false;
   };
   /// The set of opaque types enqueued for lazy emission.
   llvm::DenseMap<OpaqueTypeDecl*, LazyOpaqueInfo> LazyOpaqueTypes;
   /// The queue of opaque type descriptors to emit.
   llvm::SmallVector<OpaqueTypeDecl*, 4> LazyOpaqueTypeDescriptors;

   /// The queue of lazy field metadata records to emit.
   llvm::SmallVector<NominalTypeDecl *, 4> LazyFieldDescriptors;

   llvm::SetVector<PILFunction *> DynamicReplacements;

   /// PIL functions that have already been lazily emitted, or are queued up.
   llvm::SmallPtrSet<PILFunction *, 4> LazilyEmittedFunctions;

   /// The queue of PIL functions to emit.
   llvm::SmallVector<PILFunction *, 4> LazyFunctionDefinitions;

   /// Witness tables that have already been lazily emitted, or are queued up.
   llvm::SmallPtrSet<PILWitnessTable *, 4> LazilyEmittedWitnessTables;

   /// The queue of lazy witness tables to emit.
   llvm::SmallVector<PILWitnessTable *, 4> LazyWitnessTables;

   llvm::SmallVector<ClassDecl *, 4> ClassesForEagerInitialization;

   /// The order in which all the PIL function definitions should
   /// appear in the translation unit.
   llvm::DenseMap<PILFunction*, unsigned> FunctionOrder;

   /// The queue of IRGenModules for multi-threaded compilation.
   SmallVector<IRGenModule *, 8> Queue;

   std::atomic<int> QueueIndex;

   friend class CurrentIGMPtr;
public:
   explicit IRGenerator(IRGenOptions &opts, PILModule &module);

   /// Attempt to create an llvm::TargetMachine for the current target.
   std::unique_ptr<llvm::TargetMachine> createTargetMachine();

   /// Add an IRGenModule for a source file.
   /// Should only be called from IRGenModule's constructor.
   void addGenModule(SourceFile *SF, IRGenModule *IGM);

   /// Get an IRGenModule for a source file.
   IRGenModule *getGenModule(SourceFile *SF) {
      IRGenModule *IGM = GenModules[SF];
      assert(IGM);
      return IGM;
   }

   /// Get an IRGenModule for a declaration context.
   /// Returns the IRGenModule of the containing source file, or if this cannot
   /// be determined, returns the primary IRGenModule.
   IRGenModule *getGenModule(DeclContext *ctxt);

   /// Get an IRGenModule for a function.
   /// Returns the IRGenModule of the containing source file, or if this cannot
   /// be determined, returns the IGM from which the function is referenced the
   /// first time.
   IRGenModule *getGenModule(PILFunction *f);

   /// Returns the primary IRGenModule. This is the first added IRGenModule.
   /// It is used for everything which cannot be correlated to a specific source
   /// file. And of course, in single-threaded compilation there is only the
   /// primary IRGenModule.
   IRGenModule *getPrimaryIGM() const {
      assert(PrimaryIGM);
      return PrimaryIGM;
   }

   bool hasMultipleIGMs() const { return GenModules.size() >= 2; }

   llvm::DenseMap<SourceFile *, IRGenModule *>::iterator begin() {
      return GenModules.begin();
   }

   llvm::DenseMap<SourceFile *, IRGenModule *>::iterator end() {
      return GenModules.end();
   }

   /// Emit functions, variables and tables which are needed anyway, e.g. because
   /// they are externally visible.
   void emitGlobalTopLevel();

   /// Emit references to each of the protocol descriptors defined in this
   /// IR module.
   void emitTypePHPInterfaces();

   /// Emit the protocol conformance records needed by each IR module.
   void emitInterfaceConformances();

   /// Emit type metadata records for types without explicit protocol conformance.
   void emitTypeMetadataRecords();

   /// Emit reflection metadata records for builtin and imported types referenced
   /// from this module.
   void emitBuiltinReflectionMetadata();

   /// Emit a symbol identifying the reflection metadata version.
   void emitReflectionMetadataVersion();

   void emitEagerClassInitialization();

   // Emit the code to replace dynamicReplacement(for:) functions.
   void emitDynamicReplacements();

   /// Checks if metadata for this type can be emitted lazily. This is true for
   /// non-public types as well as imported types, except for classes and
   /// protocols which are always emitted eagerly.
   bool hasLazyMetadata(TypeDecl *type);

   /// Emit everything which is reachable from already emitted IR.
   void emitLazyDefinitions();

   void addLazyFunction(PILFunction *f);

   void addDynamicReplacement(PILFunction *f) { DynamicReplacements.insert(f); }

   void forceLocalEmitOfLazyFunction(PILFunction *f) {
      DefaultIGMForFunction[f] = CurrentIGM;
   }

   void ensureRelativeSymbolCollocation(PILWitnessTable &wt);

   void ensureRelativeSymbolCollocation(PILDefaultWitnessTable &wt);

   void noteUseOfTypeMetadata(NominalTypeDecl *type) {
      noteUseOfTypeGlobals(type, true, RequireMetadata);
   }

   void noteUseOfTypeMetadata(CanType type) {
      type.visit([&](Type t) {
         if (auto *nominal = t->getAnyNominal())
            noteUseOfTypeMetadata(nominal);
      });
   }

   void noteUseOfTypeContextDescriptor(NominalTypeDecl *type,
                                       RequireMetadata_t requireMetadata) {
      noteUseOfTypeGlobals(type, false, requireMetadata);
   }

   void noteUseOfOpaqueTypeDescriptor(OpaqueTypeDecl *opaque);

   void noteUseOfFieldDescriptor(NominalTypeDecl *type);

   void noteUseOfFieldDescriptors(CanType type) {
      type.visit([&](Type t) {
         if (auto *nominal = t->getAnyNominal())
            noteUseOfFieldDescriptor(nominal);
      });
   }

private:
   void noteUseOfTypeGlobals(NominalTypeDecl *type,
                             bool isUseOfMetadata,
                             RequireMetadata_t requireMetadata);
public:

   /// Return true if \p wt can be emitted lazily.
   bool canEmitWitnessTableLazily(PILWitnessTable *wt);

   /// Adds \p Conf to LazyWitnessTables if it has not been added yet.
   void addLazyWitnessTable(const InterfaceConformance *Conf);

   // @todo
//   void addClassForEagerInitialization(ClassDecl *ClassDecl);

   unsigned getFunctionOrder(PILFunction *F) {
      auto it = FunctionOrder.find(F);
      assert(it != FunctionOrder.end() &&
             "no order number for PIL function definition?");
      return it->second;
   }

   /// In multi-threaded compilation fetch the next IRGenModule from the queue.
   IRGenModule *fetchFromQueue() {
      int idx = QueueIndex++;
      if (idx < (int)Queue.size()) {
         return Queue[idx];
      }
      return nullptr;
   }

   /// Return the effective triple used by clang.
   llvm::Triple getEffectiveClangTriple();

   const llvm::DataLayout &getClangDataLayout();
};

class ConstantReference {
public:
   enum Directness : bool { Direct, Indirect };
private:
   llvm::PointerIntPair<llvm::Constant *, 1, Directness> ValueAndIsIndirect;
public:
   ConstantReference() {}
   ConstantReference(llvm::Constant *value, Directness isIndirect)
      : ValueAndIsIndirect(value, isIndirect) {}

   Directness isIndirect() const { return ValueAndIsIndirect.getInt(); }
   llvm::Constant *getValue() const { return ValueAndIsIndirect.getPointer(); }

   llvm::Constant *getDirectValue() const {
      assert(!isIndirect());
      return getValue();
   }

   explicit operator bool() const {
      return ValueAndIsIndirect.getPointer() != nullptr;
   }
};

/// A reference to a declared type entity.
class TypeEntityReference {
   TypeReferenceKind Kind;
   llvm::Constant *Value;
public:
   TypeEntityReference(TypeReferenceKind kind, llvm::Constant *value)
      : Kind(kind), Value(value) {}

   TypeReferenceKind getKind() const { return Kind; }
   llvm::Constant *getValue() const { return Value; }
};

/// Describes the role of a mangled type reference string.
enum class MangledTypeRefRole {
   /// The mangled type reference is used for normal metadata.
      Metadata,
   /// The mangled type reference is used for reflection metadata.
      Reflection,
   /// The mangled type reference is used for a default associated type
   /// witness.
      DefaultAssociatedTypeWitness,
};

/// IRGenModule - Primary class for emitting IR for global declarations.
///
class IRGenModule {
public:
   // The ABI version of the Swift data generated by this file.
   static const uint32_t polarphpVersion = 7;

   IRGenerator &IRGen;
   AstContext &Context;
   std::unique_ptr<clang::CodeGenerator> ClangCodeGen;
   llvm::Module &Module;
   llvm::LLVMContext &LLVMContext;
   const llvm::DataLayout DataLayout;
   const llvm::Triple Triple;
   std::unique_ptr<llvm::TargetMachine> TargetMachine;
   ModuleDecl *getTypePHPModule() const;
   AvailabilityContext getAvailabilityContext() const;
   lowering::TypeConverter &getPILTypes() const;
   PILModule &getPILModule() const { return IRGen.PIL; }
   const IRGenOptions &getOptions() const { return IRGen.Opts; }
   PILModuleConventions silConv;
   ModuleDecl *ObjCModule = nullptr;
   ModuleDecl *ClangImporterModule = nullptr;
   SourceFile *CurSourceFile = nullptr;

   llvm::SmallString<128> OutputFilename;
   llvm::SmallString<128> MainInputFilenameForDebugInfo;

   /// Order dependency -- TargetInfo must be initialized after Opts.
   const PolarphpTargetInfo TargetInfo;
   /// Holds lexical scope info, etc. Is a nullptr if we compile without -g.
   std::unique_ptr<IRGenDebugInfo> DebugInfo;

   /// A global variable which stores the hash of the module. Used for
   /// incremental compilation.
   llvm::GlobalVariable *ModuleHash;

   /// Does the current target require Objective-C interoperation?
   bool ObjCInterop = true;

   /// Is the current target using the Darwin pre-stable ABI's class marker bit?
   bool UseDarwinPreStableABIBit = true;

   /// Should we add value names to local IR values?
   bool EnableValueNames = false;

   // Is polarphperror returned in a register by the target ABI.
   bool IsSwiftErrorInRegister;

   llvm::Type *VoidTy;                  /// void (usually {})
   llvm::IntegerType *Int1Ty;           /// i1
   llvm::IntegerType *Int8Ty;           /// i8
   llvm::IntegerType *Int16Ty;          /// i16
   llvm::IntegerType *Int32Ty;          /// i32
   llvm::PointerType *Int32PtrTy;       /// i32 *
   llvm::IntegerType *RelativeAddressTy;
   llvm::PointerType *RelativeAddressPtrTy;
   llvm::IntegerType *Int64Ty;          /// i64
   union {
      llvm::IntegerType *SizeTy;         /// usually i32 or i64
      llvm::IntegerType *IntPtrTy;
      llvm::IntegerType *MetadataKindTy;
      llvm::IntegerType *OnceTy;
      llvm::IntegerType *FarRelativeAddressTy;
      llvm::IntegerType *InterfaceDescriptorRefTy;
   };
   llvm::IntegerType *ObjCBoolTy;       /// i8 or i1
   union {
      llvm::PointerType *Int8PtrTy;      /// i8*
      llvm::PointerType *WitnessTableTy;
      llvm::PointerType *ObjCSELTy;
      llvm::PointerType *FunctionPtrTy;
      llvm::PointerType *CaptureDescriptorPtrTy;
   };
   union {
      llvm::PointerType *Int8PtrPtrTy;   /// i8**
      llvm::PointerType *WitnessTablePtrTy;
   };
   llvm::StructType *RefCountedStructTy;/// %polarphp.refcounted = type { ... }
   Size RefCountedStructSize;           /// sizeof(%polarphp.refcounted)
   llvm::PointerType *RefCountedPtrTy;  /// %polarphp.refcounted*
#define CHECKED_REF_STORAGE(Name, ...) \
  llvm::PointerType *Name##ReferencePtrTy; /// %polarphp. #name _reference*
#include "polarphp/ast/ReferenceStorageDef.h"
   llvm::Constant *RefCountedNull;      /// %polarphp.refcounted* null
   llvm::StructType *FunctionPairTy;    /// { i8*, %polarphp.refcounted* }
   llvm::StructType *NoEscapeFunctionPairTy;    /// { i8*, %polarphp.opaque* }
   llvm::FunctionType *DeallocatingDtorTy; /// void (%polarphp.refcounted*)
   llvm::StructType *TypeMetadataStructTy; /// %polarphp.type = type { ... }
   llvm::PointerType *TypeMetadataPtrTy;/// %polarphp.type*
   union {
      llvm::StructType *TypeMetadataResponseTy;   /// { %polarphp.type*, iSize }
      llvm::StructType *TypeMetadataDependencyTy; /// { %polarphp.type*, iSize }
   };
   llvm::StructType *OffsetPairTy;      /// { iSize, iSize }
   llvm::StructType *FullTypeLayoutTy;  /// %polarphp.full_type_layout = { ... }
   llvm::StructType *TypeLayoutTy;  /// %polarphp.type_layout = { ... }
   llvm::PointerType *TupleTypeMetadataPtrTy; /// %polarphp.tuple_type*
   llvm::StructType *FullHeapMetadataStructTy; /// %polarphp.full_heapmetadata = type { ... }
   llvm::PointerType *FullHeapMetadataPtrTy;/// %polarphp.full_heapmetadata*
   llvm::StructType *FullBoxMetadataStructTy; /// %polarphp.full_boxmetadata = type { ... }
   llvm::PointerType *FullBoxMetadataPtrTy;/// %polarphp.full_boxmetadata*
   llvm::StructType *FullTypeMetadataStructTy; /// %polarphp.full_type = type { ... }
   llvm::PointerType *FullTypeMetadataPtrTy;/// %polarphp.full_type*
   llvm::StructType *InterfaceDescriptorStructTy; /// %polarphp.protocol = type { ... }
   llvm::PointerType *InterfaceDescriptorPtrTy; /// %polarphp.protocol*
   llvm::StructType *InterfaceRequirementStructTy; /// %polarphp.protocol_requirement
   union {
      llvm::PointerType *ObjCPtrTy;      /// %objc_object*
      llvm::PointerType *UnknownRefCountedPtrTy;
   };
   llvm::PointerType *BridgeObjectPtrTy;/// %polarphp.bridge*
   llvm::StructType *OpaqueTy;          /// %polarphp.opaque
   llvm::PointerType *OpaquePtrTy;      /// %polarphp.opaque*
   llvm::StructType *ObjCClassStructTy; /// %objc_class
   llvm::PointerType *ObjCClassPtrTy;   /// %objc_class*
   llvm::StructType *ObjCSuperStructTy; /// %objc_super
   llvm::PointerType *ObjCSuperPtrTy;   /// %objc_super*
   llvm::StructType *ObjCBlockStructTy; /// %objc_block
   llvm::PointerType *ObjCBlockPtrTy;   /// %objc_block*
   llvm::FunctionType *ObjCUpdateCallbackTy;
   llvm::StructType *ObjCFullResilientClassStubTy;   /// %objc_full_class_stub
   llvm::StructType *ObjCResilientClassStubTy;   /// %objc_class_stub
   llvm::StructType *InterfaceRecordTy;
   llvm::PointerType *InterfaceRecordPtrTy;
   llvm::StructType *InterfaceConformanceDescriptorTy;
   llvm::PointerType *InterfaceConformanceDescriptorPtrTy;
   llvm::StructType *TypeContextDescriptorTy;
   llvm::PointerType *TypeContextDescriptorPtrTy;
   llvm::StructType *ClassContextDescriptorTy;
   llvm::StructType *MethodDescriptorStructTy; /// %polarphp.method_descriptor
   llvm::StructType *MethodOverrideDescriptorStructTy; /// %polarphp.method_override_descriptor
   llvm::StructType *TypeMetadataRecordTy;
   llvm::PointerType *TypeMetadataRecordPtrTy;
   llvm::StructType *FieldDescriptorTy;
   llvm::PointerType *FieldDescriptorPtrTy;
   llvm::PointerType *FieldDescriptorPtrPtrTy;
   llvm::PointerType *ErrorPtrTy;       /// %polarphp.error*
   llvm::StructType *OpenedErrorTripleTy; /// { %polarphp.opaque*, %polarphp.type*, i8** }
   llvm::PointerType *OpenedErrorTriplePtrTy; /// { %polarphp.opaque*, %polarphp.type*, i8** }*
   llvm::PointerType *WitnessTablePtrPtrTy;   /// i8***
   llvm::StructType *OpaqueTypeDescriptorTy;
   llvm::PointerType *OpaqueTypeDescriptorPtrTy;
   llvm::Type *FloatTy;
   llvm::Type *DoubleTy;
   llvm::StructType *DynamicReplacementsTy; // { i8**, i8* }
   llvm::PointerType *DynamicReplacementsPtrTy;

   llvm::StructType *DynamicReplacementLinkEntryTy; // %link_entry = { i8*, %link_entry*}
   llvm::PointerType
      *DynamicReplacementLinkEntryPtrTy; // %link_entry*
   llvm::StructType *DynamicReplacementKeyTy; // { i32, i32}

   llvm::GlobalVariable *TheTrivialPropertyDescriptor = nullptr;

   /// Used to create unique names for class layout types with tail allocated
   /// elements.
   unsigned TailElemTypeID = 0;

   unsigned InvariantMetadataID; /// !invariant.load
   unsigned DereferenceableID;   /// !dereferenceable
   llvm::MDNode *InvariantNode;

   llvm::CallingConv::ID C_CC;          /// standard C calling convention
   llvm::CallingConv::ID DefaultCC;     /// default calling convention
   llvm::CallingConv::ID SwiftCC;       /// polarphp calling convention

   Signature getAssociatedTypeWitnessTableAccessFunctionSignature();

   /// Get the bit width of an integer type for the target platform.
   unsigned getBuiltinIntegerWidth(BuiltinIntegerType *t);
   unsigned getBuiltinIntegerWidth(BuiltinIntegerWidth w);

   Size getPointerSize() const { return PtrSize; }
   Alignment getPointerAlignment() const {
      // We always use the pointer's width as its polarphp ABI alignment.
      return Alignment(PtrSize.getValue());
   }
   Alignment getWitnessTableAlignment() const {
      return getPointerAlignment();
   }
   Alignment getTypeMetadataAlignment() const {
      return getPointerAlignment();
   }

   /// Return the offset, relative to the address point, of the start of the
   /// type-specific members of an enum metadata.
   Size getOffsetOfEnumTypeSpecificMetadataMembers() {
      return getPointerSize() * 2;
   }

   /// Return the offset, relative to the address point, of the start of the
   /// type-specific members of a struct metadata.
   Size getOffsetOfStructTypeSpecificMetadataMembers() {
      return getPointerSize() * 2;
   }

   Size::int_type getOffsetInWords(Size offset) {
      assert(offset.isMultipleOf(getPointerSize()));
      return offset / getPointerSize();
   }

   llvm::Type *getReferenceType(ReferenceCounting style);

   static bool isLoadableReferenceAddressOnly(ReferenceCounting style) {
      switch (style) {
         case ReferenceCounting::Native:
            return false;

         case ReferenceCounting::Unknown:
//         case ReferenceCounting::ObjC:
         case ReferenceCounting::Block:
            return true;

         case ReferenceCounting::Bridge:
         case ReferenceCounting::Error:
            llvm_unreachable("loadable references to this type are not supported");
      }

      llvm_unreachable("Not a valid ReferenceCounting.");
   }

   /// Return the spare bit mask to use for types that comprise heap object
   /// pointers.
   const SpareBitVector &getHeapObjectSpareBits() const;

   const SpareBitVector &getFunctionPointerSpareBits() const;
   const SpareBitVector &getWitnessTablePtrSpareBits() const;

   /// Return runtime specific extra inhabitant and spare bits policies.
   unsigned getReferenceStorageExtraInhabitantCount(ReferenceOwnership ownership,
                                                    ReferenceCounting style) const;
   SpareBitVector getReferenceStorageSpareBits(ReferenceOwnership ownership,
                                               ReferenceCounting style) const;
   APInt getReferenceStorageExtraInhabitantValue(unsigned bits, unsigned index,
                                                 ReferenceOwnership ownership,
                                                 ReferenceCounting style) const;
   APInt getReferenceStorageExtraInhabitantMask(ReferenceOwnership ownership,
                                                ReferenceCounting style) const;

   llvm::Type *getFixedBufferTy();
   llvm::PointerType *getExistentialPtrTy(unsigned numTables);
   llvm::Type *getValueWitnessTy(ValueWitness index);
   Signature getValueWitnessSignature(ValueWitness index);

   llvm::StructType *getIntegerLiteralTy();

   llvm::StructType *getValueWitnessTableTy();
   llvm::StructType *getEnumValueWitnessTableTy();
   llvm::PointerType *getValueWitnessTablePtrTy();
   llvm::PointerType *getEnumValueWitnessTablePtrTy();

   void unimplemented(SourceLoc, StringRef Message);
   LLVM_ATTRIBUTE_NORETURN
   void fatal_unimplemented(SourceLoc, StringRef Message);
   void error(SourceLoc loc, const Twine &message);

   bool useDllStorage();

   Size getAtomicBoolSize() const { return AtomicBoolSize; }
   Alignment getAtomicBoolAlignment() const { return AtomicBoolAlign; }

   enum class ObjCLabelType {
      ClassName,
      MethodVarName,
      MethodVarType,
      PropertyName,
   };

   std::string GetObjCSectionName(StringRef Section, StringRef MachOAttributes);
   void SetCStringLiteralSection(llvm::GlobalVariable *GV, ObjCLabelType Type);

private:
   Size PtrSize;
   Size AtomicBoolSize;
   Alignment AtomicBoolAlign;
   llvm::Type *FixedBufferTy;          /// [N x i8], where N == 3 * sizeof(void*)

   llvm::Type *ValueWitnessTys[MaxNumValueWitnesses];
   llvm::FunctionType *AssociatedTypeWitnessTableAccessFunctionTy = nullptr;
   llvm::StructType *GenericWitnessTableCacheTy = nullptr;
   llvm::StructType *IntegerLiteralTy = nullptr;
   llvm::PointerType *ValueWitnessTablePtrTy = nullptr;
   llvm::PointerType *EnumValueWitnessTablePtrTy = nullptr;

   llvm::DenseMap<llvm::Type *, SpareBitVector> SpareBitsForTypes;

//--- Types -----------------------------------------------------------------
public:
   const InterfaceInfo &getInterfaceInfo(InterfaceDecl *D, InterfaceInfoKind kind);

   // Not strictly a type operation, but similar.
   const ConformanceInfo &
   getConformanceInfo(const InterfaceDecl *protocol,
                      const InterfaceConformance *conformance);

   PILType getLoweredType(AbstractionPattern orig, Type subst) const;
   PILType getLoweredType(Type subst) const;
   const lowering::TypeLowering &getTypeLowering(PILType type) const;
   bool isTypeABIAccessible(PILType type) const;

   const TypeInfo &getTypeInfoForUnlowered(AbstractionPattern orig,
                                           CanType subst);
   const TypeInfo &getTypeInfoForUnlowered(AbstractionPattern orig,
                                           Type subst);
   const TypeInfo &getTypeInfoForUnlowered(Type subst);
   const TypeInfo &getTypeInfoForLowered(CanType T);
   const TypeInfo &getTypeInfo(PILType T);
   const TypeInfo &getWitnessTablePtrTypeInfo();
   const TypeInfo &getTypeMetadataPtrTypeInfo();
   const TypeInfo &getObjCClassPtrTypeInfo();
   const LoadableTypeInfo &getOpaqueStorageTypeInfo(Size size, Alignment align);
   const LoadableTypeInfo &
   getReferenceObjectTypeInfo(ReferenceCounting refcounting);
   const LoadableTypeInfo &getNativeObjectTypeInfo();
   const LoadableTypeInfo &getUnknownObjectTypeInfo();
   const LoadableTypeInfo &getBridgeObjectTypeInfo();
   const LoadableTypeInfo &getRawPointerTypeInfo();
   llvm::Type *getStorageTypeForUnlowered(Type T);
   llvm::Type *getStorageTypeForLowered(CanType T);
   llvm::Type *getStorageType(PILType T);
   llvm::PointerType *getStoragePointerTypeForUnlowered(Type T);
   llvm::PointerType *getStoragePointerTypeForLowered(CanType T);
   llvm::PointerType *getStoragePointerType(PILType T);
   llvm::StructType *createNominalType(CanType type);
   llvm::StructType *createNominalType(InterfaceCompositionType *T);
   clang::CanQual<clang::Type> getClangType(CanType type);
   clang::CanQual<clang::Type> getClangType(PILType type);
   clang::CanQual<clang::Type> getClangType(PILParameterInfo param,
                                            CanPILFunctionType funcTy);

   const clang::ASTContext &getClangAstContext() {
      assert(ClangAstContext &&
                "requesting clang AST context without clang importer!");
      return *ClangAstContext;
   }

   clang::CodeGen::CodeGenModule &getClangCGM() const;

   CanType getRuntimeReifiedType(CanType type);
   CanType substOpaqueTypesWithUnderlyingTypes(CanType type);
   PILType substOpaqueTypesWithUnderlyingTypes(PILType type, CanGenericSignature genericSig);
   std::pair<CanType, InterfaceConformanceRef>
   substOpaqueTypesWithUnderlyingTypes(CanType type,
                                       InterfaceConformanceRef conformance);

   bool isResilient(NominalTypeDecl *decl, ResilienceExpansion expansion);
   bool hasResilientMetadata(ClassDecl *decl, ResilienceExpansion expansion);
   ResilienceExpansion getResilienceExpansionForAccess(NominalTypeDecl *decl);
   ResilienceExpansion getResilienceExpansionForLayout(NominalTypeDecl *decl);
   ResilienceExpansion getResilienceExpansionForLayout(PILGlobalVariable *var);

   TypeExpansionContext getMaximalTypeExpansionContext() const;

   bool isResilientConformance(const NormalInterfaceConformance *conformance);
   bool isResilientConformance(const RootInterfaceConformance *root);
   bool isDependentConformance(const RootInterfaceConformance *conformance);

   Alignment getCappedAlignment(Alignment alignment);

   SpareBitVector getSpareBitsForType(llvm::Type *scalarTy, Size size);

   MetadataLayout &getMetadataLayout(NominalTypeDecl *decl);
   NominalMetadataLayout &getNominalMetadataLayout(NominalTypeDecl *decl);
   StructMetadataLayout &getMetadataLayout(StructDecl *decl);
   ClassMetadataLayout &getClassMetadataLayout(ClassDecl *decl);
   EnumMetadataLayout &getMetadataLayout(EnumDecl *decl);
   ForeignClassMetadataLayout &getForeignMetadataLayout(ClassDecl *decl);

   ClassMetadataStrategy getClassMetadataStrategy(const ClassDecl *theClass);

private:
   TypeConverter &Types;
   friend TypeConverter;

   const clang::ASTContext *ClangAstContext;
   ClangTypeConverter *ClangTypes;
   void initClangTypeConverter();
   void destroyClangTypeConverter();

   llvm::DenseMap<Decl*, MetadataLayout*> MetadataLayouts;
   void destroyMetadataLayoutMap();

   llvm::DenseMap<const InterfaceConformance *,
      std::unique_ptr<const ConformanceInfo>> Conformances;

   friend class GenericContextScope;
   friend class LoweringModeScope;

//--- Globals ---------------------------------------------------------------
public:
   std::pair<llvm::GlobalVariable *, llvm::Constant *>
   createStringConstant(StringRef Str, bool willBeRelativelyAddressed = false,
                        StringRef sectionName = "");
   llvm::Constant *getAddrOfGlobalString(StringRef utf8,
                                         bool willBeRelativelyAddressed = false);
   llvm::Constant *getAddrOfGlobalUTF16String(StringRef utf8);
   llvm::Constant *getAddrOfObjCSelectorRef(StringRef selector);
   llvm::Constant *getAddrOfObjCSelectorRef(PILDeclRef method);
//   std::string getObjCSelectorName(PILDeclRef method);
//   llvm::Constant *getAddrOfObjCMethodName(StringRef methodName);
//   llvm::Constant *getAddrOfObjCInterfaceRecord(InterfaceDecl *proto,
//                                               ForDefinition_t forDefinition);
//   llvm::Constant *getAddrOfObjCInterfaceRef(InterfaceDecl *proto,
//                                            ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfKeyPathPattern(KeyPathPattern *pattern,
                                           PILLocation diagLoc);
   llvm::Constant *getAddrOfOpaqueTypeDescriptor(OpaqueTypeDecl *opaqueType,
                                                 ConstantInit forDefinition);
   ConstantReference getConstantReferenceForInterfaceDescriptor(InterfaceDecl *proto);

   ConstantIntegerLiteral getConstantIntegerLiteral(APInt value);

   void addUsedGlobal(llvm::GlobalValue *global);
   void addCompilerUsedGlobal(llvm::GlobalValue *global);
   void addObjCClass(llvm::Constant *addr, bool nonlazy);
   void addInterfaceConformance(ConformanceDescription &&conformance);

   llvm::Constant *emitTypePHPInterfaces();
   llvm::Constant *emitInterfaceConformances();
   llvm::Constant *emitTypeMetadataRecords();
   llvm::Constant *emitFieldDescriptors();

   llvm::Constant *getOrCreateHelperFunction(StringRef name,
                                             llvm::Type *resultType,
                                             ArrayRef<llvm::Type*> paramTypes,
                                             llvm::function_ref<void(IRGenFunction &IGF)> generate,
                                             bool setIsNoInline = false);

   llvm::Constant *getOrCreateRetainFunction(const TypeInfo &objectTI, PILType t,
                                             llvm::Type *llvmType);

   llvm::Constant *getOrCreateReleaseFunction(const TypeInfo &objectTI, PILType t,
                                              llvm::Type *llvmType);

   llvm::Constant *getOrCreateOutlinedInitializeWithTakeFunction(
      PILType objectType, const TypeInfo &objectTI,
      const OutliningMetadataCollector &collector);

   llvm::Constant *getOrCreateOutlinedInitializeWithCopyFunction(
      PILType objectType, const TypeInfo &objectTI,
      const OutliningMetadataCollector &collector);

   llvm::Constant *getOrCreateOutlinedAssignWithTakeFunction(
      PILType objectType, const TypeInfo &objectTI,
      const OutliningMetadataCollector &collector);

   llvm::Constant *getOrCreateOutlinedAssignWithCopyFunction(
      PILType objectType, const TypeInfo &objectTI,
      const OutliningMetadataCollector &collector);

   llvm::Constant *getOrCreateOutlinedDestroyFunction(
      PILType objectType, const TypeInfo &objectTI,
      const OutliningMetadataCollector &collector);

private:
   llvm::Constant *getAddrOfClangGlobalDecl(clang::GlobalDecl global,
                                            ForDefinition_t forDefinition);

   using CopyAddrHelperGenerator =
   llvm::function_ref<void(IRGenFunction &IGF, Address dest, Address src,
                           PILType objectType, const TypeInfo &objectTI)>;

   llvm::Constant *getOrCreateOutlinedCopyAddrHelperFunction(
      PILType objectType, const TypeInfo &objectTI,
      const OutliningMetadataCollector &collector,
      StringRef funcName,
      CopyAddrHelperGenerator generator);

   llvm::Constant *getOrCreateGOTEquivalent(llvm::Constant *global,
                                            LinkEntity entity);

   llvm::DenseMap<LinkEntity, llvm::Constant*> GlobalVars;
   llvm::DenseMap<LinkEntity, llvm::Constant*> GlobalGOTEquivalents;
   llvm::DenseMap<LinkEntity, llvm::Function*> GlobalFuncs;
   llvm::DenseSet<const clang::Decl *> GlobalClangDecls;
   llvm::StringMap<std::pair<llvm::GlobalVariable*, llvm::Constant*>>
      GlobalStrings;
   llvm::StringMap<llvm::Constant*> GlobalUTF16Strings;
   llvm::StringMap<std::pair<llvm::GlobalVariable*, llvm::Constant*>>
      StringsForTypeRef;
   llvm::DenseMap<CanType, llvm::GlobalVariable*> TypeRefs;
   llvm::StringMap<std::pair<llvm::GlobalVariable*, llvm::Constant*>> FieldNames;
   llvm::StringMap<llvm::Constant*> ObjCSelectorRefs;
   llvm::StringMap<llvm::Constant*> ObjCMethodNames;

   std::unique_ptr<ConstantIntegerLiteralMap> ConstantIntegerLiterals;

   /// Maps to constant polarphp 'String's.
   llvm::StringMap<llvm::Constant*> GlobalConstantStrings;
   llvm::StringMap<llvm::Constant*> GlobalConstantUTF16Strings;

   /// LLVMUsed - List of global values which are required to be
   /// present in the object file; bitcast to i8*. This is used for
   /// forcing visibility of symbols which may otherwise be optimized
   /// out.
   SmallVector<llvm::WeakTrackingVH, 4> LLVMUsed;

   /// LLVMCompilerUsed - List of global values which are required to be
   /// present in the object file; bitcast to i8*. This is used for
   /// forcing visibility of symbols which may otherwise be optimized
   /// out.
   ///
   /// Similar to LLVMUsed, but emitted as llvm.compiler.used.
   SmallVector<llvm::WeakTrackingVH, 4> LLVMCompilerUsed;

   /// Metadata nodes for autolinking info.
   SmallVector<llvm::MDNode *, 32> AutolinkEntries;

   /// List of Objective-C classes, bitcast to i8*.
   SmallVector<llvm::WeakTrackingVH, 4> ObjCClasses;
   /// List of Objective-C classes that require nonlazy realization, bitcast to
   /// i8*.
   SmallVector<llvm::WeakTrackingVH, 4> ObjCNonLazyClasses;
   /// List of Objective-C categories, bitcast to i8*.
   SmallVector<llvm::WeakTrackingVH, 4> ObjCCategories;
   /// List of Objective-C categories on class stubs, bitcast to i8*.
   SmallVector<llvm::WeakTrackingVH, 4> ObjCCategoriesOnStubs;
   /// List of non-ObjC protocols described by this module.
   SmallVector<InterfaceDecl *, 4> SwiftInterfaces;
   /// List of protocol conformances to generate descriptors for.
   std::vector<ConformanceDescription> InterfaceConformances;
   /// List of types to generate runtime-resolvable metadata records for.
   SmallVector<GenericTypeDecl *, 4> RuntimeResolvableTypes;
   /// List of ExtensionDecls corresponding to the generated
   /// categories.
   SmallVector<ExtensionDecl*, 4> ObjCCategoryDecls;

   /// List of fields descriptors to register in runtime.
   SmallVector<llvm::GlobalVariable *, 4> FieldDescriptors;

   /// Map of Objective-C protocols and protocol references, bitcast to i8*.
   /// The interesting global variables relating to an ObjC protocol.
   struct ObjCInterfacePair {
      /// The global variable that contains the protocol record.
      llvm::WeakTrackingVH record;
      /// The global variable that contains the indirect reference to the
      /// protocol record.
      llvm::WeakTrackingVH ref;
   };

   llvm::DenseMap<InterfaceDecl*, ObjCInterfacePair> ObjCInterfaces;
   llvm::SmallVector<InterfaceDecl*, 4> LazyObjCInterfaceDefinitions;
   llvm::DenseMap<KeyPathPattern*, llvm::GlobalVariable*> KeyPathPatterns;

   /// Uniquing key for a fixed type layout record.
   struct FixedLayoutKey {
      unsigned size;
      unsigned numExtraInhabitants;
      unsigned align: 16;
      unsigned pod: 1;
      unsigned bitwiseTakable: 1;
   };
   friend struct ::llvm::DenseMapInfo<polar::irgen::IRGenModule::FixedLayoutKey>;
   llvm::DenseMap<FixedLayoutKey, llvm::Constant *> PrivateFixedLayouts;

   /// A cache for layouts of statically initialized objects.
   llvm::DenseMap<PILGlobalVariable *, std::unique_ptr<StructLayout>>
      StaticObjectLayouts;

   /// A mapping from order numbers to the LLVM functions which we
   /// created for the PIL functions with those orders.
   SuccessorMap<unsigned, llvm::Function*> EmittedFunctionsByOrder;

   ObjCInterfacePair getObjCInterfaceGlobalVars(InterfaceDecl *proto);
   void emitLazyObjCInterfaceDefinitions();
   void emitLazyObjCInterfaceDefinition(InterfaceDecl *proto);

   void emitGlobalLists();
   void emitAutolinkInfo();
   void cleanupClangCodeGenMetadata();

//--- Remote reflection metadata --------------------------------------------
public:
   /// Section names.
   std::string FieldTypeSection;
   std::string BuiltinTypeSection;
   std::string AssociatedTypeSection;
   std::string CaptureDescriptorSection;
   std::string ReflectionStringsSection;
   std::string ReflectionTypeRefSection;

   /// Builtin types referenced by types in this module when emitting
   /// reflection metadata.
   llvm::SetVector<CanType> BuiltinTypes;

   std::pair<llvm::Constant *, unsigned>
   getTypeRef(Type type, GenericSignature genericSig, MangledTypeRefRole role);

   std::pair<llvm::Constant *, unsigned>
   getTypeRef(CanType type, CanGenericSignature sig, MangledTypeRefRole role);

   std::pair<llvm::Constant *, unsigned>
   getLoweredTypeRef(PILType loweredType, CanGenericSignature genericSig,
                     MangledTypeRefRole role);

   llvm::Constant *emitWitnessTableRefString(CanType type,
                                             InterfaceConformanceRef conformance,
                                             GenericSignature genericSig,
                                             bool shouldSetLowBit);
   llvm::Constant *getMangledAssociatedConformance(
      const NormalInterfaceConformance *conformance,
      const AssociatedConformance &requirement);
   llvm::Constant *getAddrOfStringForTypeRef(StringRef mangling,
                                             MangledTypeRefRole role);
   llvm::Constant *getAddrOfStringForTypeRef(const SymbolicMangling &mangling,
                                             MangledTypeRefRole role);

   /// Retrieve the address of a mangled string used for some kind of metadata
   /// reference.
   ///
   /// \param symbolName The name of the symbol that describes the metadata
   /// being referenced.
   /// \param alignment If non-zero, the alignment of the requested variable.
   /// \param shouldSetLowBit Whether to set the low bit of the result
   /// constant, which is used by some clients to indicate that the result is
   /// a mangled name.
   /// \param body The body of a function that will create the metadata value
   /// itself, given a constant building and producing a future for the
   /// initializer.
   /// \returns the address of the global variable describing this metadata.
   llvm::Constant *getAddrOfStringForMetadataRef(
      StringRef symbolName,
      unsigned alignment,
      bool shouldSetLowBit,
      llvm::function_ref<ConstantInitFuture(ConstantInitBuilder &)> body);

   llvm::Constant *getAddrOfFieldName(StringRef Name);
   llvm::Constant *getAddrOfCaptureDescriptor(PILFunction &caller,
                                              CanPILFunctionType origCalleeType,
                                              CanPILFunctionType substCalleeType,
                                              SubstitutionMap subs,
                                              const HeapLayout &layout);
   llvm::Constant *getAddrOfBoxDescriptor(PILType boxedType,
                                          CanGenericSignature genericSig);

   /// Produce an associated type witness that refers to the given type.
   llvm::Constant *getAssociatedTypeWitness(Type type, bool inInterfaceContext);

   void emitAssociatedTypeMetadataRecord(const RootInterfaceConformance *C);
   void emitFieldDescriptor(const NominalTypeDecl *Decl);

   /// Emit a reflection metadata record for a builtin type referenced
   /// from this module.
   void emitBuiltinTypeMetadataRecord(CanType builtinType);

   /// Emit reflection metadata records for builtin and imported types referenced
   /// from this module.
   void emitBuiltinReflectionMetadata();

   /// Emit a symbol identifying the reflection metadata version.
   void emitReflectionMetadataVersion();

   const char *getBuiltinTypeMetadataSectionName();
   const char *getFieldTypeMetadataSectionName();
   const char *getAssociatedTypeMetadataSectionName();
   const char *getCaptureDescriptorMetadataSectionName();
   const char *getReflectionStringsSectionName();
   const char *getReflectionTypeRefSectionName();

//--- Runtime ---------------------------------------------------------------
public:
   llvm::Constant *getEmptyTupleMetadata();
   llvm::Constant *getAnyExistentialMetadata();
   llvm::Constant *getAnyObjectExistentialMetadata();
   llvm::Constant *getObjCEmptyCachePtr();
   llvm::Constant *getObjCEmptyVTablePtr();
   llvm::InlineAsm *getObjCRetainAutoreleasedReturnValueMarker();
   ClassDecl *getObjCRuntimeBaseForPolarphpRootClass(ClassDecl *theClass);
   ClassDecl *getObjCRuntimeBaseClass(Identifier name, Identifier objcName);
   llvm::Module *getModule() const;
   llvm::Module *releaseModule();
   llvm::AttributeList getAllocAttrs();

   bool isStandardLibrary() const;

private:
   llvm::Constant *EmptyTupleMetadata = nullptr;
   llvm::Constant *AnyExistentialMetadata = nullptr;
   llvm::Constant *AnyObjectExistentialMetadata = nullptr;
   llvm::Constant *ObjCEmptyCachePtr = nullptr;
   llvm::Constant *ObjCEmptyVTablePtr = nullptr;
   llvm::Constant *ObjCISAMaskPtr = nullptr;
   Optional<llvm::InlineAsm*> ObjCRetainAutoreleasedReturnValueMarker;
   llvm::DenseMap<Identifier, ClassDecl*> PolarphpRootClasses;
   llvm::AttributeList AllocAttrs;

#define FUNCTION_ID(Id)             \
public:                             \
  llvm::Constant *get##Id##Fn();    \
private:                            \
  llvm::Constant *Id##Fn = nullptr;
#include "polarphp/runtime/RuntimeFunctionsDef.h"

   llvm::Constant *FixLifetimeFn = nullptr;

   mutable Optional<SpareBitVector> HeapPointerSpareBits;

//--- Generic ---------------------------------------------------------------
public:
   llvm::Constant *getFixLifetimeFn();

   /// The constructor used when generating code.
   ///
   /// The \p SF is the source file for which the llvm module is generated when
   /// doing multi-threaded whole-module compilation. Otherwise it is null.
   IRGenModule(IRGenerator &irgen, std::unique_ptr<llvm::TargetMachine> &&target,
               SourceFile *SF, llvm::LLVMContext &LLVMContext,
               StringRef ModuleName, StringRef OutputFilename,
               StringRef MainInputFilenameForDebugInfo);

   /// The constructor used when we just need an IRGenModule for type lowering.
   IRGenModule(IRGenerator &irgen, std::unique_ptr<llvm::TargetMachine> &&target,
               llvm::LLVMContext &LLVMContext)
      : IRGenModule(irgen, std::move(target), /*SF=*/nullptr, LLVMContext,
                    "<fake module name>", "<fake output filename>",
                    "<fake main input filename>") {}

   ~IRGenModule();

   llvm::LLVMContext &getLLVMContext() const { return LLVMContext; }

   void emitSourceFile(SourceFile &SF);
   void addLinkLibrary(const LinkLibrary &linkLib);

   /// Attempt to finalize the module.
   ///
   /// This can fail, in which it will return false and the module will be
   /// invalid.
   bool finalize();

   void constructInitialFnAttributes(llvm::AttrBuilder &Attrs,
                                     OptimizationMode FuncOptMode =
                                     OptimizationMode::NotSet);
   void setHasFramePointer(llvm::AttrBuilder &Attrs, bool HasFP);
   void setHasFramePointer(llvm::Function *F, bool HasFP);
   llvm::AttributeList constructInitialAttributes();

   void emitInterfaceDecl(InterfaceDecl *D);
   void emitEnumDecl(EnumDecl *D);
   void emitStructDecl(StructDecl *D);
   void emitClassDecl(ClassDecl *D);
   void emitExtension(ExtensionDecl *D);
   void emitOpaqueTypeDecl(OpaqueTypeDecl *D);
   void emitPILGlobalVariable(PILGlobalVariable *gv);
   void emitCoverageMapping();
   void emitPILFunction(PILFunction *f);
   void emitPILWitnessTable(PILWitnessTable *wt);
   void emitPILProperty(PILProperty *prop);
   void emitPILStaticInitializers();
   llvm::Constant *emitFixedTypeLayout(CanType t, const FixedTypeInfo &ti);
   void emitInterfaceConformance(const ConformanceDescription &record);
   void emitNestedTypeDecls(DeclRange members);
   void emitClangDecl(const clang::Decl *decl);
   void finalizeClangCodeGen();
   void finishEmitAfterTopLevel();

   Signature getSignature(CanPILFunctionType fnType);
   llvm::FunctionType *getFunctionType(CanPILFunctionType type,
                                       llvm::AttributeList &attrs,
                                       ForeignFunctionInfo *foreignInfo=nullptr);
   ForeignFunctionInfo getForeignFunctionInfo(CanPILFunctionType type);

   llvm::Constant *getInt32(uint32_t value);
   llvm::Constant *getSize(Size size);
   llvm::Constant *getAlignment(Alignment align);
   llvm::Constant *getBool(bool condition);

   /// Cast the given constant to i8*.
   llvm::Constant *getOpaquePtr(llvm::Constant *pointer);

   llvm::Function *getAddrOfDispatchThunk(PILDeclRef declRef,
                                          ForDefinition_t forDefinition);
   void emitDispatchThunk(PILDeclRef declRef);

   llvm::Function *getAddrOfMethodLookupFunction(ClassDecl *classDecl,
                                                 ForDefinition_t forDefinition);
   void emitMethodLookupFunction(ClassDecl *classDecl);

   llvm::GlobalValue *defineAlias(LinkEntity entity,
                                  llvm::Constant *definition);

   llvm::GlobalValue *defineMethodDescriptor(PILDeclRef declRef,
                                             NominalTypeDecl *nominalDecl,
                                             llvm::Constant *definition);
   llvm::Constant *getAddrOfMethodDescriptor(PILDeclRef declRef,
                                             ForDefinition_t forDefinition);

   Address getAddrOfEnumCase(EnumElementDecl *Case,
                             ForDefinition_t forDefinition);
   Address getAddrOfFieldOffset(VarDecl *D, ForDefinition_t forDefinition);
   llvm::Function *getAddrOfValueWitness(CanType concreteType,
                                         ValueWitness index,
                                         ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfValueWitnessTable(CanType concreteType,
                                              ConstantInit init = ConstantInit());
   Optional<llvm::Function*> getAddrOfIVarInitDestroy(ClassDecl *cd,
                                                      bool isDestroyer,
                                                      bool isForeign,
                                                      ForDefinition_t forDefinition);
   llvm::GlobalValue *defineTypeMetadata(CanType concreteType,
                                         bool isPattern,
                                         bool isConstant,
                                         ConstantInitFuture init,
                                         llvm::StringRef section = {});

   TypeEntityReference getTypeEntityReference(GenericTypeDecl *D);

   llvm::Constant *getAddrOfTypeMetadata(CanType concreteType);
   ConstantReference getAddrOfTypeMetadata(CanType concreteType,
                                           SymbolReferenceKind kind);
   llvm::Constant *getAddrOfTypeMetadataPattern(NominalTypeDecl *D);
   llvm::Constant *getAddrOfTypeMetadataPattern(NominalTypeDecl *D,
                                                ConstantInit init,
                                                StringRef section);
   llvm::Function *getAddrOfTypeMetadataCompletionFunction(NominalTypeDecl *D,
                                                           ForDefinition_t forDefinition);
   llvm::Function *getAddrOfTypeMetadataInstantiationFunction(NominalTypeDecl *D,
                                                              ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfTypeMetadataInstantiationCache(NominalTypeDecl *D,
                                                           ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfTypeMetadataSingletonInitializationCache(
      NominalTypeDecl *D,
      ForDefinition_t forDefinition);
   llvm::Function *getAddrOfTypeMetadataAccessFunction(CanType type,
                                                       ForDefinition_t forDefinition);
   llvm::Function *getAddrOfGenericTypeMetadataAccessFunction(
      NominalTypeDecl *nominal,
      ArrayRef<llvm::Type *> genericArgs,
      ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfTypeMetadataLazyCacheVariable(CanType type);
   llvm::Constant *getAddrOfTypeMetadataDemanglingCacheVariable(CanType type,
                                                                ConstantInit definition);

   llvm::Constant *getAddrOfClassMetadataBounds(ClassDecl *D,
                                                ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfTypeContextDescriptor(NominalTypeDecl *D,
                                                  RequireMetadata_t requireMetadata,
                                                  ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfAnonymousContextDescriptor(
      PointerUnion<DeclContext *, VarDecl *> Name,
      ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfExtensionContextDescriptor(ExtensionDecl *ED,
                                                       ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfModuleContextDescriptor(ModuleDecl *D,
                                                    ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfReflectionFieldDescriptor(CanType type,
                                                      ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfReflectionBuiltinDescriptor(CanType type,
                                                        ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfReflectionAssociatedTypeDescriptor(
      const InterfaceConformance *c,
      ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfObjCModuleContextDescriptor();
   llvm::Constant *getAddrOfClangImporterModuleContextDescriptor();
   ConstantReference getAddrOfParentContextDescriptor(DeclContext *from,
                                                      bool fromAnonymousContext);
   ConstantReference getAddrOfContextDescriptorForParent(DeclContext *parent,
                                                         DeclContext *ofChild,
                                                         bool fromAnonymousContext);
   llvm::Constant *getAddrOfGenericEnvironment(CanGenericSignature signature);
   llvm::Constant *getAddrOfInterfaceRequirementsBaseDescriptor(
      InterfaceDecl *proto);
   llvm::GlobalValue *defineInterfaceRequirementsBaseDescriptor(
      InterfaceDecl *proto,
      llvm::Constant *definition);
   llvm::Constant *getAddrOfAssociatedTypeDescriptor(
      AssociatedTypeDecl *assocType);
   llvm::GlobalValue *defineAssociatedTypeDescriptor(
      AssociatedTypeDecl *assocType,
      llvm::Constant *definition);
   llvm::Constant *getAddrOfAssociatedConformanceDescriptor(
      AssociatedConformance conformance);
   llvm::GlobalValue *defineAssociatedConformanceDescriptor(
      AssociatedConformance conformance,
      llvm::Constant *definition);
   llvm::Constant *getAddrOfBaseConformanceDescriptor(
      BaseConformance conformance);
   llvm::GlobalValue *defineBaseConformanceDescriptor(
      BaseConformance conformance,
      llvm::Constant *definition);

   llvm::Constant *getAddrOfInterfaceDescriptor(InterfaceDecl *D,
                                               ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfInterfaceConformanceDescriptor(
      const RootInterfaceConformance *conformance,
      ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfPropertyDescriptor(AbstractStorageDecl *D,
                                               ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfObjCClass(ClassDecl *D,
                                      ForDefinition_t forDefinition);
   Address getAddrOfObjCClassRef(ClassDecl *D);
   llvm::Constant *getAddrOfMetaclassObject(ClassDecl *D,
                                            ForDefinition_t forDefinition);

   llvm::Function *getAddrOfObjCMetadataUpdateFunction(ClassDecl *D,
                                                       ForDefinition_t forDefinition);

   llvm::Constant *getAddrOfObjCResilientClassStub(ClassDecl *D,
                                                   ForDefinition_t forDefinition,
                                                   TypeMetadataAddress addr);

   llvm::Function *
   getAddrOfPILFunction(PILFunction *f, ForDefinition_t forDefinition,
                        bool isDynamicallyReplaceableImplementation = false,
                        bool shouldCallPreviousImplementation = false);

   void emitDynamicReplacementOriginalFunctionThunk(PILFunction *f);

   llvm::Function *getAddrOfContinuationPrototype(CanPILFunctionType fnType);
   Address getAddrOfPILGlobalVariable(PILGlobalVariable *var,
                                      const TypeInfo &ti,
                                      ForDefinition_t forDefinition);
   llvm::Function *getAddrOfWitnessTableLazyAccessFunction(
      const NormalInterfaceConformance *C,
      CanType conformingType,
      ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfWitnessTableLazyCacheVariable(
      const NormalInterfaceConformance *C,
      CanType conformingType,
      ForDefinition_t forDefinition);
   llvm::Constant *getAddrOfWitnessTable(const RootInterfaceConformance *C,
                                         ConstantInit definition = ConstantInit());
   llvm::Constant *getAddrOfWitnessTablePattern(
      const NormalInterfaceConformance *C,
      ConstantInit definition = ConstantInit());

   llvm::Function *
   getAddrOfGenericWitnessTableInstantiationFunction(
      const NormalInterfaceConformance *C);
   llvm::Function *getAddrOfAssociatedTypeWitnessTableAccessFunction(
      const NormalInterfaceConformance *C,
      const AssociatedConformance &association);
   llvm::Function *getAddrOfDefaultAssociatedConformanceAccessor(
      AssociatedConformance requirement);

   Address getAddrOfObjCISAMask();

   /// Retrieve the generic signature for the current generic context, or null if no
   /// generic environment is active.
   CanGenericSignature getCurGenericContext();

   /// Retrieve the generic environment for the current generic context.
   ///
   /// Fails if there is no generic context.
   GenericEnvironment *getGenericEnvironment();

   ConstantReference
   getAddrOfLLVMVariableOrGOTEquivalent(LinkEntity entity,
                                        ConstantReference::Directness forceIndirect = ConstantReference::Direct);

   llvm::Constant *
   emitRelativeReference(ConstantReference target,
                         llvm::Constant *base,
                         ArrayRef<unsigned> baseIndices);

   llvm::Constant *
   emitDirectRelativeReference(llvm::Constant *target,
                               llvm::Constant *base,
                               ArrayRef<unsigned> baseIndices);

   /// Mark a global variable as true-const by putting it in the text section of
   /// the binary.
   void setTrueConstGlobal(llvm::GlobalVariable *var);

   /// Add the polarphpself attribute.
   void addSwiftSelfAttributes(llvm::AttributeList &attrs, unsigned argIndex);

   /// Add the polarphperror attribute.
   void addSwiftErrorAttributes(llvm::AttributeList &attrs, unsigned argIndex);

   void emitSharedContextDescriptor(DeclContext *dc);

   llvm::GlobalVariable *
   getGlobalForDynamicallyReplaceableThunk(LinkEntity &entity, llvm::Type *type,
                                           ForDefinition_t forDefinition);

   llvm::Function *getAddrOfOpaqueTypeDescriptorAccessFunction(
      OpaqueTypeDecl *decl, ForDefinition_t forDefinition, bool implementation);

   void createReplaceableProlog(IRGenFunction &IGF, PILFunction *f);

   void emitOpaqueTypeDescriptorAccessor(OpaqueTypeDecl *);

private:
   llvm::Constant *
   getAddrOfSharedContextDescriptor(LinkEntity entity,
                                    ConstantInit definition,
                                    llvm::function_ref<void()> emit);

   llvm::Constant *getAddrOfLLVMVariable(LinkEntity entity,
                                         ConstantInit definition,
                                         DebugTypeInfo debugType,
                                         llvm::Type *overrideDeclType = nullptr);
   llvm::Constant *getAddrOfLLVMVariable(LinkEntity entity,
                                         ForDefinition_t forDefinition,
                                         DebugTypeInfo debugType);
   ConstantReference getAddrOfLLVMVariable(LinkEntity entity,
                                           ConstantInit definition,
                                           DebugTypeInfo debugType,
                                           SymbolReferenceKind refKind,
                                           llvm::Type *overrideDeclType = nullptr);

   void emitLazyPrivateDefinitions();
   void addRuntimeResolvableType(GenericTypeDecl *nominal);
   void maybeEmitOpaqueTypeDecl(OpaqueTypeDecl *opaque);

   /// Add all conformances of the given \c DeclContext LazyWitnessTables.
   void addLazyConformances(DeclContext *dc);

//--- Global context emission --------------------------------------------------
public:
   void emitRuntimeRegistration();
   void emitVTableStubs();
   void emitTypeVerifier();

   /// Create llvm metadata which encodes the branch weights given by
   /// \p TrueCount and \p FalseCount.
   llvm::MDNode *createProfileWeights(uint64_t TrueCount,
                                      uint64_t FalseCount) const;

private:
   void emitGlobalDecl(Decl *D);
};

/// Stores a pointer to an IRGenModule.
/// As long as the CurrentIGMPtr is alive, the CurrentIGM in the dispatcher
/// is set to the containing IRGenModule.
class CurrentIGMPtr {
   IRGenModule *IGM;

public:
   CurrentIGMPtr(IRGenModule *IGM) : IGM(IGM) {
      assert(IGM);
      assert(!IGM->IRGen.CurrentIGM && "Another CurrentIGMPtr is alive");
      IGM->IRGen.CurrentIGM = IGM;
   }

   ~CurrentIGMPtr() {
      IGM->IRGen.CurrentIGM = nullptr;
   }

   IRGenModule *get() const { return IGM; }
   IRGenModule *operator->() const { return IGM; }
};

/// Workaround to disable thumb-mode until debugger support is there.
bool shouldRemoveTargetFeature(StringRef);

} // end namespace irgen
} // end namespace polar

namespace llvm {

template<>
struct DenseMapInfo<polar::irgen::IRGenModule::FixedLayoutKey> {
   using FixedLayoutKey = polar::irgen::IRGenModule::FixedLayoutKey;

   static inline FixedLayoutKey getEmptyKey() {
      return {0, 0xFFFFFFFFu, 0, 0, 0};
   }

   static inline FixedLayoutKey getTombstoneKey() {
      return {0, 0xFFFFFFFEu, 0, 0, 0};
   }

   static unsigned getHashValue(const FixedLayoutKey &key) {
      return hash_combine(key.size, key.numExtraInhabitants, key.align,
                          (bool)key.pod, (bool)key.bitwiseTakable);
   }
   static bool isEqual(const FixedLayoutKey &a, const FixedLayoutKey &b) {
      return a.size == b.size
             && a.numExtraInhabitants == b.numExtraInhabitants
             && a.align == b.align
             && a.pod == b.pod
             && a.bitwiseTakable == b.bitwiseTakable;
   }
};

} // llvm

#endif // POLARPHP_IRGEN_INTERNAL_IRGENMODULE_H
