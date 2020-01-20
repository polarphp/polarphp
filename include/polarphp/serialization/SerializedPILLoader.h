//===--- SerializedPILLoader.h - Handle PIL section in modules --*- C++ -*-===//
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

#ifndef POLAR_SERIALIZATION_PILLOADER_H
#define POLAR_SERIALIZATION_PILLOADER_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/pil/lang/Notifications.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILLinkage.h"
#include <memory>
#include <vector>

namespace polar {

class AstContext;
class FileUnit;
class ModuleDecl;
class PILDeserializer;
class PILFunction;
class PILGlobalVariable;
class PILModule;
class PILVTable;
class PILWitnessTable;
class PILDefaultWitnessTable;

/// Maintains a list of PILDeserializer, one for each serialized modules
/// in AstContext. It provides lookupPILFunction that will perform lookup
/// on each PILDeserializer.
class SerializedPILLoader {
private:
  std::vector<std::unique_ptr<PILDeserializer>> LoadedPILSections;

  explicit SerializedPILLoader(
      AstContext &ctx, PILModule *PILMod,
      DeserializationNotificationHandlerSet *callbacks);

public:
  /// Create a new loader.
  ///
  /// \param callbacks - not owned by the loader
  static std::unique_ptr<SerializedPILLoader>
  create(AstContext &ctx, PILModule *PILMod,
         DeserializationNotificationHandlerSet *callbacks) {
      /// TODO
      return nullptr;
//    return std::unique_ptr<SerializedPILLoader>(
//        new SerializedPILLoader(ctx, PILMod, callbacks));
  }
  ~SerializedPILLoader();

  PILFunction *lookupPILFunction(PILFunction *Callee, bool onlyUpdateLinkage);
  PILFunction *
  lookupPILFunction(StringRef Name, bool declarationOnly = false,
                    Optional<PILLinkage> linkage = None);
  bool hasPILFunction(StringRef Name, Optional<PILLinkage> linkage = None);
  PILVTable *lookupVTable(const ClassDecl *C);
  PILWitnessTable *lookupWitnessTable(PILWitnessTable *C);
  PILDefaultWitnessTable *lookupDefaultWitnessTable(PILDefaultWitnessTable *C);

  /// Invalidate the cached entries for deserialized PILFunctions.
  void invalidateCaches();

  bool invalidateFunction(PILFunction *F);

  /// Deserialize all PILFunctions, VTables, and WitnessTables in all
  /// PILModules.
  void getAll();

  /// Deserialize all PILFunctions, VTables, and WitnessTables for
  /// a given Module.
  ///
  /// If PrimaryFile is nullptr, all definitions are brought in with
  /// definition linkage.
  ///
  /// Otherwise, definitions not in the primary file are brought in
  /// with external linkage.
  void getAllForModule(Identifier Mod, FileUnit *PrimaryFile);

  /// Deserialize all PILFunctions in all PILModules.
  void getAllPILFunctions();

  /// Deserialize all VTables in all PILModules.
  void getAllVTables();

  /// Deserialize all WitnessTables in all PILModules.
  void getAllWitnessTables();

  /// Deserialize all DefaultWitnessTables in all PILModules.
  void getAllDefaultWitnessTables();

  /// Deserialize all Properties in all PILModules.
  void getAllProperties();

  SerializedPILLoader(const SerializedPILLoader &) = delete;
  SerializedPILLoader(SerializedPILLoader &&) = delete;
  SerializedPILLoader &operator=(const SerializedPILLoader &) = delete;
  SerializedPILLoader &operator=(SerializedPILLoader &&) = delete;
};

} // end namespace polar

#endif
