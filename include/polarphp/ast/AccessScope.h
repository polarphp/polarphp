//===--- AccessScope.h - Swift Access Scope ---------------------*- C++ -*-===//
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

#ifndef POLARPHP_AST_ACCESSSCOPE_H
#define POLARPHP_AST_ACCESSSCOPE_H

#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/DeclContext.h"
#include "polarphp/basic/Debug.h"
#include "llvm/ADT/PointerIntPair.h"

namespace polar {

using llvm::None;

/// The wrapper around the outermost DeclContext from which
/// a particular declaration can be accessed.
class AccessScope
{
   /// The declaration context (if not public) along with a bit saying
   /// whether this scope is private (or not).
   llvm::PointerIntPair<const DeclContext *, 1, bool> Value;
public:
   AccessScope(const DeclContext *DC, bool isPrivate = false);

   static AccessScope getPublic() { return AccessScope(nullptr); }

   /// Check if private access is allowed. This is a lexical scope check in Swift
   /// 3 mode. In Swift 4 mode, declarations and extensions of the same type will
   /// also allow access.
   static bool allowsPrivateAccess(const DeclContext *useDC, const DeclContext *sourceDC);

   /// Returns nullptr if access scope is public.
   const DeclContext *getDeclContext() const { return Value.getPointer(); }

   bool operator==(AccessScope other) const { return Value == other.Value; }
   bool operator!=(AccessScope other) const { return !(*this == other); }
   bool hasEqualDeclContextWith(AccessScope other) const {
      return getDeclContext() == other.getDeclContext();
   }

   bool isPublic() const { return !Value.getPointer(); }
   bool isPrivate() const { return Value.getInt(); }
   bool isFileScope() const;
   bool isInternal() const;

   /// Returns true if this is a child scope of the specified other access scope.
   ///
   /// \see DeclContext::isChildContextOf
   bool isChildOf(AccessScope accessScope) const
   {
      if (!isPublic() && !accessScope.isPublic()) {
         return allowsPrivateAccess(getDeclContext(), accessScope.getDeclContext());
      }
      if (isPublic() && accessScope.isPublic()) {
         return false;
      }
      return accessScope.isPublic();
   }

   /// Returns the associated access level for diagnostic purposes.
   AccessLevel accessLevelForDiagnostics() const;

   /// Returns the minimum access level required to access
   /// associated DeclContext for diagnostic purposes.
   AccessLevel requiredAccessForDiagnostics() const {
      return isFileScope()
            ? AccessLevel::FilePrivate
            : accessLevelForDiagnostics();
   }

   /// Returns the narrowest access scope if this and the specified access scope
   /// have common intersection, or None if scopes don't intersect.
   const Optional<AccessScope> intersectWith(AccessScope accessScope) const
   {
      if (hasEqualDeclContextWith(accessScope)) {
         if (isPrivate()) {
            return *this;
         }
         return accessScope;
      }
      if (isChildOf(accessScope)) {
         return *this;
      }
      if (accessScope.isChildOf(*this)) {
         return accessScope;
      }
      return None;
   }
   POLAR_DEBUG_DUMP;
};


} // polar

#endif // POLARPHP_AST_ACCESSSCOPE_H
