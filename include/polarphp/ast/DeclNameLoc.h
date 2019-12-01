//===--- DeclNameLoc.h - Declaration Name Location Info ---------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/12/01.
//===----------------------------------------------------------------------===//
//
// This file defines the DeclNameLoc class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DECL_NAME_LOC_H
#define POLARPHP_AST_DECL_NAME_LOC_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/SourceLoc.h"

namespace polar::ast {

class AstContext;
using polar::basic::SourceLoc;
using polar::basic::SourceRange;

/// Source location information for a declaration name (\c DeclName)
/// written in the source.
class DeclNameLoc
{
   /// Source location information.
   ///
   /// If \c m_numArgumentLabels == 0, this is the SourceLoc for the base name.
   /// Otherwise, it points to an array of SourceLocs, which contains:
   /// * The base name location
   /// * The left parentheses location
   /// * The right parentheses location
   /// * The locations of each of the argument labels.
   const void *m_locationInfo;

   /// The number of argument labels stored in the name.
   unsigned m_numArgumentLabels;

   enum {
      BaseNameIndex = 0,
      LParenIndex = 1,
      RParenIndex = 2,
      FirstArgumentLabelIndex = 3,
   };

   /// Retrieve a pointer to either the only source location that was
   /// stored or to the array of source locations that was stored.
   SourceLoc const * getSourceLocs() const
   {
      if (m_numArgumentLabels == 0) {
         return reinterpret_cast<SourceLoc const *>(&m_locationInfo);
      }
      return reinterpret_cast<SourceLoc const *>(m_locationInfo);
   }

public:
   /// Create an invalid declaration name location.
   DeclNameLoc()
      : m_locationInfo(nullptr),
        m_numArgumentLabels(0)
   {}

   /// Create declaration name location information for a base name.
   explicit DeclNameLoc(SourceLoc baseNameLoc)
      : m_locationInfo(baseNameLoc.getOpaquePointerValue()),
        m_numArgumentLabels(0)
   {}

   /// Create declaration name location information for a compound
   /// name.
   DeclNameLoc(AstContext &ctx, SourceLoc baseNameLoc,
               SourceLoc lParenLoc,
               ArrayRef<SourceLoc> argumentLabelLocs,
               SourceLoc rParenLoc);

   /// Whether the location information is valid.
   bool isValid() const
   {
      return getBaseNameLoc().isValid();
   }

   /// Whether the location information is invalid.
   bool isInvalid() const
   {
      return getBaseNameLoc().isInvalid();
   }

   /// Whether this was written as a compound name.
   bool isCompound() const
   {
      return m_numArgumentLabels > 0;
   }

   /// Retrieve the location of the base name.
   SourceLoc getBaseNameLoc() const
   {
      return getSourceLocs()[BaseNameIndex];
   }

   /// Retrieve the location of the left parentheses.
   SourceLoc getLParenLoc() const
   {
      if (m_numArgumentLabels == 0) return SourceLoc();
      return getSourceLocs()[LParenIndex];
   }

   /// Retrieve the location of the right parentheses.
   SourceLoc getRParenLoc() const
   {
      if (m_numArgumentLabels == 0) return SourceLoc();
      return getSourceLocs()[RParenIndex];
   }

   /// Retrieve the location of an argument label.
   SourceLoc getArgumentLabelLoc(unsigned index) const
   {
      if (index >= m_numArgumentLabels) {
         return SourceLoc();
      }
      return getSourceLocs()[FirstArgumentLabelIndex + index];
   }

   SourceLoc getStartLoc() const
   {
      return getBaseNameLoc();
   }

   SourceLoc getEndLoc() const
   {
      return m_numArgumentLabels == 0 ? getBaseNameLoc() : getRParenLoc();
   }

   /// Retrieve the complete source range for this declaration name.
   SourceRange getSourceRange() const
   {
      if (m_numArgumentLabels == 0) {
         return getBaseNameLoc();
      }
      return SourceRange(getBaseNameLoc(), getRParenLoc());
   }
};

} // polar::ast

#endif // POLARPHP_AST_DECL_NAME_LOC_H

