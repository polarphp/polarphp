// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/12.

#include "polarphp/syntax/SyntaxData.h"

namespace polar::syntax {

RefCountPtr<SyntaxData> SyntaxData::make(RefCountPtr<RawSyntax> raw,
                                         const SyntaxData *parent,
                                         CursorIndex indexInParent)
{
   auto size = totalSizeToAlloc<AtomicCache<SyntaxData>>(raw->getNumChildren());
   void *data = ::operator new(size);
   return RefCountPtr<SyntaxData>{new (data) SyntaxData(raw, parent, indexInParent)};
}

bool SyntaxData::isStmt() const
{
   return getRaw()->isStmt();
}

bool SyntaxData::isDecl() const
{
   return getRaw()->isDecl();
}

bool SyntaxData::isExpr() const
{
   return getRaw()->isExpr();
}

bool SyntaxData::isUnknown() const
{
   return getRaw()->isUnknown();
}

void SyntaxData::dump(raw_ostream &outStream) const
{
   m_raw->dump(outStream, 0);
   outStream << '\n';
}

void SyntaxData::dump() const
{
   dump(llvm::errs());
}

RefCountPtr<SyntaxData> SyntaxData::getPreviousNode() const
{
   if (size_t end = getIndexInParent()) {
      if (hasParent()) {
         for (size_t index = end - 1; ; index--) {
            if (auto C = getParent()->getChild(index)) {
               if (C->getRaw()->isPresent() && C->getFirstToken())
                  return C;
            }
            if (index == 0)
               break;
         }
      }
   }
   return hasParent() ? m_parent->getPreviousNode() : nullptr;
}

RefCountPtr<SyntaxData> SyntaxData::getNextNode() const
{
   if (hasParent()) {
      for (size_t index = getIndexInParent() + 1, end = m_parent->getNumChildren();
           index != end; index++) {
         if (auto C = getParent()->getChild(index)) {
            if (C->getRaw()->isPresent() && C->getFirstToken())
               return C;
         }
      }
      return m_parent->getNextNode();
   }
   return nullptr;
}

RefCountPtr<SyntaxData> SyntaxData::getFirstToken() const
{
   if (getRaw()->isToken()) {
      // Get a reference counted version of this
      assert(hasParent() && "The syntax tree should not conisist only of the root");
      return getParent()->getChild(getIndexInParent());
   }

   for (size_t index = 0, end = getNumChildren(); index < end; ++index) {
      if (auto child = getChild(index)) {
         if (child->getRaw()->isMissing())
            continue;
         if (child->getRaw()->isToken()) {
            return child;
         } else if (auto Token = child->getFirstToken()) {
            return Token;
         }
      }
   }
   return nullptr;
}

AbsolutePosition SyntaxData::getAbsolutePositionBeforeLeadingTrivia() const
{
   if (m_positionCache.has_value()) {
      return *m_positionCache;
   }
   if (auto prev = getPreviousNode()) {
      auto result = prev->getAbsolutePositionBeforeLeadingTrivia();
      prev->getRaw()->accumulateAbsolutePosition(result);
      // FIXME: avoid using const_cast.
      const_cast<SyntaxData*>(this)->m_positionCache = result;
   } else {
      const_cast<SyntaxData*>(this)->m_positionCache = AbsolutePosition();
   }
   return *m_positionCache;
}

AbsolutePosition SyntaxData::getAbsolutePosition() const
{
   auto result = getAbsolutePositionBeforeLeadingTrivia();
   getRaw()->accumulateLeadingTrivia(result);
   return result;
}

AbsolutePosition SyntaxData::getAbsoluteEndPositionAfterTrailingTrivia() const
{
   if (auto end = getNextNode()) {
      return end->getAbsolutePositionBeforeLeadingTrivia();
   } else {
      auto result = getAbsolutePositionBeforeLeadingTrivia();
      getRaw()->accumulateAbsolutePosition(result);
      return result;
   }
}

} // polar::syntax
