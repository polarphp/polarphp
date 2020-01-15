//===--- IAMInference.h - Import as member inference system -----*- C++ -*-===//
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
// This file implements support for inferring when globals can be imported as
// members
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_CLANG_IMPORTER_INTERNAL_IMPORT_AS_MEMBER_INFERENCE_H
#define POLARPHP_CLANG_IMPORTER_INTERNAL_IMPORT_AS_MEMBER_INFERENCE_H

#include "polarphp/clangimporter/internal/TypePHPLookupTable.h"
#include "polarphp/ast/Identifier.h"

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"

namespace clang {
class Sema;
class NamedDecl;
class TypeDecl;
class FunctionDecl;
}

namespace polar {

// Import As Member -- attempt to import C global functions and variables as
// members on types or instances.

struct IAMOptions {
   // TODO: fine-grained control over how we infer
   static IAMOptions getDefault();
};

enum class IAMAccessorKind : uint8_t { None, Getter, Setter };

/// The result of out inference system
struct IAMResult {
   // The name to import as
   DeclName name = {};

   // If this is a getter/setter, the other accessor
   const clang::FunctionDecl *pairedAccessor = nullptr;

   IAMAccessorKind accessorKind = IAMAccessorKind::None;

   Optional<unsigned> selfIndex = None;

   // The canonical type decl we will import as a member of
   EffectiveClangContext effectiveDC = {};

   static IAMResult infer(AstContext &, clang::Sema &, const clang::NamedDecl *,
                          IAMOptions = IAMOptions::getDefault());

   IAMResult() = default;

   // Static members
   IAMResult(DeclName declName, EffectiveClangContext dc)
      : name(declName), effectiveDC(dc) {}

   IAMResult(DeclName declName, IAMAccessorKind kind, EffectiveClangContext dc)
      : name(declName), accessorKind(kind), effectiveDC(dc) {}

   // Instance members
   IAMResult(DeclName declName, unsigned selfIdx, EffectiveClangContext dc)
      : name(declName), selfIndex(selfIdx), effectiveDC(dc) {}

   IAMResult(DeclName declName, IAMAccessorKind kind, unsigned selfIdx,
             EffectiveClangContext dc)
      : name(declName), accessorKind(kind), selfIndex(selfIdx),
        effectiveDC(dc) {}

   bool isImportAsMember() const { return bool(effectiveDC); }

   bool isInstanceMember() const {
      return isImportAsMember() && selfIndex.hasValue();
   }

   bool isStaticMember() const {
      return isImportAsMember() && !selfIndex.hasValue();
   }

   bool isPropertyAccessor() const {
      return accessorKind != IAMAccessorKind::None;
   }

   bool isGetter() const {
      return accessorKind == IAMAccessorKind::Getter;
   }

   bool isSetter() const {
      return accessorKind == IAMAccessorKind::Setter;
   }

   bool isInit() const {
      return isStaticMember() && name.getBaseName() == "init";
   }
};
} // polar

#endif // POLARPHP_CLANG_IMPORTER_INTERNAL_IMPORT_AS_MEMBER_INFERENCE_H
