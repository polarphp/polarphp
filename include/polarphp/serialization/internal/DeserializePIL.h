//===--- DeserializePIL.h - Read PIL ----------------------------*- C++ -*-===//
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

#include "polarphp/serialization/internal/PILFormat.h"
#include "polarphp/serialization/internal/ModuleFile.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/serialization/SerializedPILLoader.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/SaveAndRestore.h"

namespace llvm {
template <typename Info> class OnDiskIterableChainedHashTable;
}

namespace polar {
class PILDeserializer {
   using TypeID = serialization::TypeID;

   ModuleFile *MF;
   PILModule &PILMod;
   DeserializationNotificationHandlerSet *Callback;

   /// The cursor used to lazily load PILFunctions.
   llvm::BitstreamCursor PILCursor;
   llvm::BitstreamCursor PILIndexCursor;

   class FuncTableInfo;
   using SerializedFuncTable =
   llvm::OnDiskIterableChainedHashTable<FuncTableInfo>;

   std::unique_ptr<SerializedFuncTable> FuncTable;
   MutableArrayRef<ModuleFile::PartiallySerialized<PILFunction*>> Funcs;

   std::unique_ptr<SerializedFuncTable> VTableList;
   MutableArrayRef<ModuleFile::Serialized<PILVTable*>> VTables;

   std::unique_ptr<SerializedFuncTable> GlobalVarList;
   MutableArrayRef<ModuleFile::Serialized<PILGlobalVariable*>> GlobalVars;

   std::unique_ptr<SerializedFuncTable> WitnessTableList;
   MutableArrayRef<ModuleFile::PartiallySerialized<PILWitnessTable *>>
      WitnessTables;

   std::unique_ptr<SerializedFuncTable> DefaultWitnessTableList;
   MutableArrayRef<ModuleFile::PartiallySerialized<PILDefaultWitnessTable *>>
      DefaultWitnessTables;

   MutableArrayRef<ModuleFile::PartiallySerialized<PILProperty *>>
      Properties;

   /// A declaration will only
   llvm::DenseMap<NormalInterfaceConformance *, PILWitnessTable *>
      ConformanceToWitnessTableMap;

   /// Data structures used to perform name lookup for local values.
   llvm::DenseMap<uint32_t, ValueBase*> LocalValues;
   llvm::DenseMap<uint32_t, ValueBase*> ForwardLocalValues;

   /// The first two local values are reserved for PILUndef.
   serialization::ValueID LastValueID = 1;

   /// Data structures used to perform lookup of basic blocks.
   llvm::DenseMap<unsigned, PILBasicBlock*> BlocksByID;
   llvm::DenseMap<PILBasicBlock*, unsigned> UndefinedBlocks;
   unsigned BasicBlockID = 0;

   /// Return the PILBasicBlock of a given ID.
   PILBasicBlock *getBBForReference(PILFunction *Fn, unsigned ID);
   PILBasicBlock *getBBForDefinition(PILFunction *Fn, PILBasicBlock *Prev,
                                     unsigned ID);

   /// Read a PIL function.
   PILFunction *readPILFunction(serialization::DeclID, PILFunction *InFunc,
                                StringRef Name, bool declarationOnly,
                                bool errorIfEmptyBody = true);
   /// Read a PIL function.
   llvm::Expected<PILFunction *>
   readPILFunctionChecked(serialization::DeclID, PILFunction *InFunc,
                          StringRef Name, bool declarationOnly,
                          bool errorIfEmptyBody = true);

   /// Read a PIL basic block within a given PIL function.
   PILBasicBlock *readPILBasicBlock(PILFunction *Fn,
                                    PILBasicBlock *Prev,
                                    SmallVectorImpl<uint64_t> &scratch);
   /// Read a PIL instruction within a given PIL basic block.
   bool readPILInstruction(PILFunction *Fn, PILBasicBlock *BB,
                           PILBuilder &Builder,
                           unsigned RecordKind,
                           SmallVectorImpl<uint64_t> &scratch);

   /// Read the PIL function table.
   std::unique_ptr<SerializedFuncTable>
   readFuncTable(ArrayRef<uint64_t> fields, StringRef blobData);

   /// When an instruction or block argument is defined, this method is used to
   /// register it and update our symbol table.
   void setLocalValue(ValueBase *Value, serialization::ValueID Id);
   /// Get a reference to a local value with the specified ID and type.
   PILValue getLocalValue(serialization::ValueID Id,
                          PILType Type);

   PILType getPILType(Type ty, PILValueCategory category,
                      PILFunction *inContext);

   PILFunction *getFuncForReference(StringRef Name, PILType Ty);
   PILFunction *getFuncForReference(StringRef Name);
   PILVTable *readVTable(serialization::DeclID);
   PILGlobalVariable *getGlobalForReference(StringRef Name);
   PILGlobalVariable *readGlobalVar(StringRef Name);
   PILWitnessTable *readWitnessTable(serialization::DeclID,
                                     PILWitnessTable *existingWt);
   void readWitnessTableEntries(
      llvm::BitstreamEntry &entry,
      std::vector<PILWitnessTable::Entry> &witnessEntries,
      std::vector<PILWitnessTable::ConditionalConformance>
      &conditionalConformances);
   PILProperty *readProperty(serialization::DeclID);
   PILDefaultWitnessTable *
   readDefaultWitnessTable(serialization::DeclID,
                           PILDefaultWitnessTable *existingWt);

   Optional<KeyPathPatternComponent>
   readKeyPathComponent(ArrayRef<uint64_t> ListOfValues, unsigned &nextValue);

public:
   Identifier getModuleIdentifier() const {
      return MF->getAssociatedModule()->getName();
   }
   FileUnit *getFile() const {
      return MF->getFile();
   }
   PILFunction *lookupPILFunction(PILFunction *InFunc, bool onlyUpdateLinkage);
   PILFunction *lookupPILFunction(StringRef Name,
                                  bool declarationOnly = false);
   bool hasPILFunction(StringRef Name, Optional<PILLinkage> Linkage = None);
   PILVTable *lookupVTable(StringRef MangledClassName);
   PILWitnessTable *lookupWitnessTable(PILWitnessTable *wt);
   PILDefaultWitnessTable *
   lookupDefaultWitnessTable(PILDefaultWitnessTable *wt);

   /// Invalidate all cached PILFunctions.
   void invalidateFunctionCache();

   /// Invalidate a specific cached PILFunction.
   bool invalidateFunction(PILFunction *F);

   /// Deserialize all PILFunctions, VTables, WitnessTables, and
   /// DefaultWitnessTables inside the module, and add them to PILMod.
   ///
   /// TODO: Globals.
   void getAll(bool UseCallback = true) {
      llvm::SaveAndRestore<DeserializationNotificationHandlerSet *> SaveCB(
         Callback);

      if (!UseCallback)
         Callback = nullptr;

      getAllPILFunctions();
      getAllPILGlobalVariables();
      getAllVTables();
      getAllWitnessTables();
      getAllDefaultWitnessTables();
      getAllProperties();
   }

   /// Deserialize all PILFunctions inside the module and add them to PILMod.
   void getAllPILFunctions();

   /// Deserialize all PILGlobalVariables inside the module and add them to
   /// PILMod.
   void getAllPILGlobalVariables();

   /// Deserialize all VTables inside the module and add them to PILMod.
   void getAllVTables();

   /// Deserialize all WitnessTables inside the module and add them to PILMod.
   void getAllWitnessTables();

   /// Deserialize all DefaultWitnessTables inside the module and add them
   /// to PILMod.
   void getAllDefaultWitnessTables();

   /// Deserialize all Property descriptors inside the module and add them
   /// to PILMod.
   void getAllProperties();

   PILDeserializer(ModuleFile *MF, PILModule &M,
                   DeserializationNotificationHandlerSet *callback);

   // Out of line to avoid instantiation OnDiskChainedHashTable here.
   ~PILDeserializer();
};
} // end namespace polar
