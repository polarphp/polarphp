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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/30.

#ifndef POLARPHP_AST_ACCESSSCOPE_H
#define POLARPHP_AST_ACCESSSCOPE_H

#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/DeclContext.h"
#include "polarphp/basic/adt/PointerIntPair.h"

namespace polar::ast {

/// The wrapper around the outermost DeclContext from which
/// a particular declaration can be accessed.
class AccessScope
{
private:
   /// The declaration context (if not public) along with a bit saying
   /// whether this scope is private (or not).
   PointerIntPair<const DeclContext *, 1, bool> m_value;
public:
   AccessScope(const DeclContext *declContext, bool isPrivate = false);

   static AccessScope getPublic()
   {
      return AccessScope(nullptr);
   }

   /// Check if private access is allowed. This is a lexical scope check in Swift
   /// 3 mode. In Swift 4 mode, declarations and extensions of the same type will
   /// also allow access.
   static bool allowsPrivateAccess(const DeclContext *useDC, const DeclContext *sourceDC);

   /// Returns nullptr if access scope is public.
   const DeclContext *getDeclContext() const
   {
      return m_value.getPointer();
   }

   bool operator==(AccessScope other) const
   {
      return m_value == other.m_value;
   }

   bool operator!=(AccessScope other) const
   {
      return !(*this == other);
   }

   bool hasEqualDeclContextWith(AccessScope other) const
   {
      return getDeclContext() == other.getDeclContext();
   }

   bool isPublic() const
   {
      return !m_value.getPointer();
   }

   bool isPrivate() const
   {
      return m_value.getInt();
   }

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
   AccessLevel requiredAccessForDiagnostics() const
   {
      return isFileScope()
            ? AccessLevel::FilePrivate
            : accessLevelForDiagnostics();
   }

   /// Returns the narrowest access scope if this and the specified access scope
   /// have common intersection, or None if scopes don't intersect.
   const std::optional<AccessScope> intersectWith(AccessScope accessScope) const
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
      return std::nullopt;
   }

   void dump() const;
};

} // polar::ast

#endif // POLARPHP_AST_ACCESSSCOPE_H
