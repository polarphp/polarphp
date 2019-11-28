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
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_AST_CAPTURE_INFO_H
#define POLARPHP_AST_CAPTURE_INFO_H

#include "polarphp/basic/LLVM.h"
//#include "polarphp/ast/TypeAlignments.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/PointerUnion.h"
#include <vector>

namespace polar::ast {
class CapturedValue;
} // polar::ast

namespace llvm {
class raw_ostream;
template <> struct DenseMapInfo<polar::ast::CapturedValue>;
} // llvm

namespace polar::ast {

using llvm::PointerUnion;
using llvm::PointerIntPair;

class ValueDecl;
class FuncDecl;
class OpaqueValueExpr;

/// CapturedValue includes both the declaration being captured, along with flags
/// that indicate how it is captured.
class CapturedValue
{
public:
   using Storage =
   llvm::PointerIntPair<llvm::PointerUnion<ValueDecl*, OpaqueValueExpr*>, 2,
   unsigned>;

private:
   Storage m_value;

   explicit CapturedValue(Storage V)
      : m_value(V) {}

public:
   friend struct llvm::DenseMapInfo<CapturedValue>;

   enum {
      /// IsDirect is set when a VarDecl with storage *and* accessors is captured
      /// by its storage address.  This happens in the accessors for the VarDecl.
      IsDirect = 1 << 0,

      /// IsNoEscape is set when a vardecl is captured by a noescape closure, and
      /// thus has its lifetime guaranteed.  It can be closed over by a fixed
      /// address if it has storage.
      IsNoEscape = 1 << 1
   };

   CapturedValue(PointerUnion<ValueDecl*, OpaqueValueExpr*> ptr,
                 unsigned flags)
      : m_value(ptr, flags) {}

   static CapturedValue getDynamicSelfMetadata()
   {
      return CapturedValue((ValueDecl *)nullptr, 0);
   }

   bool isDirect() const { return m_value.getInt() & IsDirect; }
   bool isNoEscape() const { return m_value.getInt() & IsNoEscape; }

   bool isDynamicSelfMetadata() const
   {
      return !m_value.getPointer();
   }

   bool isOpaqueValue() const
   {
      return m_value.getPointer().is<OpaqueValueExpr *>();
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
      return m_value.getPointer().dyn_cast<ValueDecl *>();
   }

   OpaqueValueExpr *getOpaqueValue() const
   {
      assert(m_value.getPointer() && "dynamic Self metadata capture does not "
                                   "have a value");
      return m_value.getPointer().dyn_cast<OpaqueValueExpr *>();
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
};

} // polar::ast

#endif // POLARPHP_AST_CAPTURE_INFO_H
