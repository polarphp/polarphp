//===--- CaptureInfo.h - Data Structure for Capture Lists -------*- C++ -*-===//
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
// Created by polarboy on 2019/04/27.

#ifndef POLARPHP_AST_CAPTURE_INFO_H
#define POLARPHP_AST_CAPTURE_INFO_H

#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/PointerIntPair.h"
#include <vector>

/// forward declare class with namespace
namespace polar {
namespace ast {
class CapturedValue;
} // ast
namespace utils {
class RawOutStream;
} // utils
namespace basic {
using polar::ast::CapturedValue;
template <>
struct DenseMapInfo<CapturedValue>;
} // basic
} // polar

namespace polar::ast {

using polar::basic::PointerIntPair;
class ValueDecl;
class FuncDecl;

/// CapturedValue includes both the declaration being captured, along with flags
/// that indicate how it is captured.
class CapturedValue
{
public:
   friend struct DenseMapInfo<CapturedValue>;
   enum {
      /// IsDirect is set when a VarDecl with storage *and* accessors is captured
      /// by its storage address.  This happens in the accessors for the VarDecl.
      IsDirect = 1 << 0,

      /// IsNoEscape is set when a vardecl is captured by a noescape closure, and
      /// thus has its lifetime guaranteed.  It can be closed over by a fixed
      /// address if it has storage.
      IsNoEscape = 1 << 1
   };

   CapturedValue(ValueDecl *vdecl, unsigned flags)
      : m_value(vdecl, flags)
   {}

   static CapturedValue getDynamicSelfMetadata()
   {
      return CapturedValue(nullptr, 0);
   }

   bool isDirect() const
   {
      return m_value.getInt() & IsDirect;
   }

   bool isNoEscape() const
   {
      return m_value.getInt() & IsNoEscape;
   }

   bool isDynamicSelfMetadata() const
   {
      return !m_value.getPointer();
   }

   CapturedValue mergeFlags(CapturedValue cv)
   {
      assert(m_value.getPointer() == cv.m_value.getPointer() &&
             "merging flags on two different value decls");
      return CapturedValue(m_value.getPointer(), getFlags() & cv.getFlags());
   }

   ValueDecl *getDecl() const
   {
      assert(m_value.getPointer() && "dynamic Self metadata capture does not "
                                     "have a value");
      return m_value.getPointer();
   }

   unsigned getFlags() const
   {
      return m_value.getInt();
   }

   bool operator==(CapturedValue other) const
   {
      return m_value == other.m_value;
   }

   bool operator!=(CapturedValue other) const
   {
      return m_value != other.m_value;
   }

   bool operator<(CapturedValue other) const
   {
      return m_value < other.m_value;
   }
private:
   PointerIntPair<ValueDecl*, 2, unsigned> m_value;
   explicit CapturedValue(PointerIntPair<ValueDecl*, 2, unsigned> V)
      : m_value(V)
   {}
};

} // polar::ast

namespace polar::basic {

using polar::ast::ValueDecl;

template <>
struct DenseMapInfo<polar::ast::CapturedValue>
{
   using CapturedValue = polar::ast::CapturedValue;

   using PtrIntPairDenseMapInfo =
   DenseMapInfo<PointerIntPair<ValueDecl *, 2, unsigned>>;

   static inline CapturedValue getEmptyKey()
   {
      return CapturedValue{PtrIntPairDenseMapInfo::getEmptyKey()};
   }

   static inline CapturedValue getTombstoneKey()
   {
      return CapturedValue{PtrIntPairDenseMapInfo::getTombstoneKey()};
   }

   static unsigned getHashValue(const CapturedValue &value)
   {
      return PtrIntPairDenseMapInfo::getHashValue(value.m_value);
   }

   static bool isEqual(const CapturedValue &lhs, const CapturedValue &rhs)
   {
      return PtrIntPairDenseMapInfo::isEqual(lhs.m_value, rhs.m_value);
   }
};
} // polar::basic

namespace polar::ast {

using polar::basic::ArrayRef;
using polar::basic::SmallVectorImpl;
using polar::utils::RawOutStream;

class DynamicSelfType;

/// Stores information about captured variables.
class CaptureInfo
{
   const CapturedValue *m_captures;
   DynamicSelfType *m_dynamicSelf;
   unsigned m_count = 0;
   bool m_genericParamCaptures : 1;
   bool m_computed : 1;

public:
   CaptureInfo()
      : m_captures(nullptr),
        m_dynamicSelf(nullptr),
        m_count(0),
        m_genericParamCaptures(0),
        m_computed(0)
   {}

   bool hasBeenComputed()
   {
      return m_computed;
   }

   bool isTrivial()
   {
      return m_count == 0 && !m_genericParamCaptures && !m_dynamicSelf;
   }

   ArrayRef<CapturedValue> getCaptures() const
   {
      return polar::basic::make_array_ref(m_captures, m_count);
   }

   void setCaptures(ArrayRef<CapturedValue> captures)
   {
      m_captures = captures.data();
      m_computed = true;
      m_count = captures.getSize();
   }

   /// Return a filtered list of the captures for this function,
   /// filtering out global variables.  This function returns the list that
   /// actually needs to be closed over.
   ///
   void getLocalCaptures(SmallVectorImpl<CapturedValue> &result) const;

   /// \returns true if getLocalCaptures() will return a non-empty list.
   bool hasLocalCaptures() const;

   /// \returns true if the function captures any generic type parameters.
   bool hasGenericParamCaptures() const
   {
      return m_genericParamCaptures;
   }

   void setGenericParamCaptures(bool genericParamCaptures)
   {
      m_genericParamCaptures = genericParamCaptures;
   }

   /// \returns true if the function captures the dynamic Self type.
   bool hasDynamicSelfCapture() const
   {
      return m_dynamicSelf != nullptr;
   }

   /// \returns the captured dynamic Self type, if any.
   DynamicSelfType *getDynamicSelfType() const
   {
      return m_dynamicSelf;
   }

   void setDynamicSelfType(DynamicSelfType *dynamicSelf)
   {
      m_dynamicSelf = dynamicSelf;
   }

   void dump() const;
   void print(RawOutStream &outStream) const;
};
} // polar::ast

#endif // POLARPHP_AST_CAPTURE_INFO_H
