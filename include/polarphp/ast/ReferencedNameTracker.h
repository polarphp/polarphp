//===--- ReferencedNameTracker.h - Records looked-up names ------*- C++ -*-===//
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

#ifndef POLARPHP_REFERENCED_NAME_TRACKER_H
#define POLARPHP_REFERENCED_NAME_TRACKER_H

#include "polarphp/ast/Identifier.h"
#include "llvm/ADT/DenseMap.h"

namespace polar {

class NominalTypeDecl;

class ReferencedNameTracker {
#define TRACKED_SET(KIND, NAME) \
private: \
   llvm::DenseMap<KIND, bool> NAME##s; \
   public: \
   void add##NAME(KIND new##NAME, bool isCascadingUse) { \
   NAME##s[new##NAME] |= isCascadingUse; \
   } \
   const decltype(NAME##s) &get##NAME##s() const { \
   return NAME##s; \
   }

   TRACKED_SET(DeclBaseName, TopLevelName)
   TRACKED_SET(DeclBaseName, DynamicLookupName)

   using MemberPair = std::pair<const NominalTypeDecl *, DeclBaseName>;
   TRACKED_SET(MemberPair, UsedMember)

#undef TRACKED_SET
};

} // end namespace polar

#endif // POLARPHP_REFERENCED_NAME_TRACKER_H
