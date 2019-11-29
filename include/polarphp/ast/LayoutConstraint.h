//===-- LayoutConstraint.h - layout constraints types and APIs --*- context++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.
//===----------------------------------------------------------------------===//
//
// This file defines types and APIs for layout constraints.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_LAYOUT_CONSTRAINT_H
#define POLARPHP_AST_LAYOUT_CONSTRAINT_H

#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/ast/PrintOptions.h"
#include "polarphp/basic/SourceLoc.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/StringRef.h"

namespace polar::ast {

enum class AllocationArena;
class AstContext;
class ASTPrinter;
using polar::basic::SourceLoc;
using polar::basic::SourceRange;

/// Describes a layout constraint information.
enum class LayoutConstraintKind : uint8_t
{
   // It is not a known layout constraint.
   UnknownLayout,
   // It is a layout constraint representing a trivial type of an unknown size.
   TrivialOfExactSize,
   // It is a layout constraint representing a trivial type of an unknown size.
   TrivialOfAtMostSize,
   // It is a layout constraint representing a trivial type of an unknown size.
   Trivial,
   // It is a layout constraint representing a reference counted class instance.
   Class,
   // It is a layout constraint representing a reference counted native class
   // instance.
   NativeClass,
   // It is a layout constraint representing a reference counted object.
   RefCountedObject,
   // It is a layout constraint representing a native reference counted object.
   NativeRefCountedObject,
   LastLayout = NativeRefCountedObject,
};

/// This is a class representing the layout constraint.
class LayoutConstraintInfo : public llvm::FoldingSetNode
{
   friend class LayoutConstraint;
   // m_alignment of the layout in bytes.
   const unsigned m_alignment : 16;
   // Size of the layout in bits.
   const unsigned m_sizeInBits : 24;
   // kind of the layout.
   const LayoutConstraintKind m_kind;

   LayoutConstraintInfo()
      : m_alignment(0), m_sizeInBits(0),
        m_kind(LayoutConstraintKind::UnknownLayout)
   {
   }

   LayoutConstraintInfo(const LayoutConstraintInfo &layout)
      : m_alignment(layout.m_alignment),
        m_sizeInBits(layout.m_sizeInBits),
        m_kind(layout.m_kind)
   {
   }

   LayoutConstraintInfo(LayoutConstraintKind kind)
      : m_alignment(0),
        m_sizeInBits(0),
        m_kind(kind)
   {
      assert(!isKnownSizeTrivial() && "Size in bits should be specified");
   }

   LayoutConstraintInfo(LayoutConstraintKind kind, unsigned sizeInBits,
                        unsigned alignment)
      : m_alignment(alignment),
        m_sizeInBits(sizeInBits),
        m_kind(kind)
   {
      assert(
               isTrivial() &&
               "Size in bits should be specified only for trivial layout constraints");
   }
public:
   LayoutConstraintKind getKind() const
   {
      return m_kind;
   }

   bool isKnownLayout() const
   {
      return isKnownLayout(m_kind);
   }

   bool isFixedSizeTrivial() const
   {
      return isFixedSizeTrivial(m_kind);
   }

   bool isKnownSizeTrivial() const
   {
      return isKnownSizeTrivial(m_kind);
   }

   bool isAddressOnlyTrivial() const
   {
      return isAddressOnlyTrivial(m_kind);
   }

   bool isTrivial() const
   {
      return isTrivial(m_kind);
   }

   bool isRefCountedObject() const
   {
      return isRefCountedObject(m_kind);
   }

   bool isNativeRefCountedObject() const
   {
      return isNativeRefCountedObject(m_kind);
   }

   bool isClass() const
   {
      return isClass(m_kind);
   }

   bool isNativeClass() const
   {
      return isNativeClass(m_kind);
   }

   bool isRefCounted() const
   {
      return isRefCounted(m_kind);
   }

   bool isNativeRefCounted() const
   {
      return isNativeRefCounted(m_kind);
   }

   unsigned getTrivialSizeInBytes() const
   {
      assert(isKnownSizeTrivial());
      return (m_sizeInBits + 7) / 8;
   }

   unsigned getMaxTrivialSizeInBytes() const
   {
      assert(isKnownSizeTrivial());
      return (m_sizeInBits + 7) / 8;
   }

   unsigned getTrivialSizeInBits() const
   {
      assert(isKnownSizeTrivial());
      return m_sizeInBits;
   }

   unsigned getMaxTrivialSizeInBits() const
   {
      assert(isKnownSizeTrivial());
      return m_sizeInBits;
   }

   unsigned getAlignmentInBits() const
   {
      return m_alignment;
   }

   unsigned getAlignmentInBytes() const
   {
      assert(isKnownSizeTrivial());
      if (m_alignment) {
         return m_alignment;
      }
      // There is no explicitly defined alignment. Try to come up with a
      // reasonable one.
      // If the size is a power of 2, use it also for the default alignment.
      auto sizeInBytes = getTrivialSizeInBytes();
      if (llvm::isPowerOf2_32(sizeInBytes)) {
         return sizeInBytes * 8;
      }
      // Otherwise assume the alignment of 8 bytes.
      return 8*8;
   }

   operator bool() const
   {
      return isKnownLayout();
   }

   bool operator==(const LayoutConstraintInfo &rhs) const
   {
      return getKind() == rhs.getKind() && m_sizeInBits == rhs.m_sizeInBits &&
            m_alignment == rhs.m_alignment;
   }

   bool operator!=(LayoutConstraintInfo rhs) const
   {
      return !(*this == rhs);
   }

   void print(raw_ostream &OS, const PrintOptions &printOption = PrintOptions()) const;
   void print(ASTPrinter &printer, const PrintOptions &printOption) const;

   /// Return the layout constraint as a string, for use in diagnostics only.
   std::string getString(const PrintOptions &printOption = PrintOptions()) const;

   /// Return the name of this layout constraint.
   StringRef getName() const;

   /// Return the name of a layout constraint with a given kind.
   static StringRef getName(LayoutConstraintKind kind);

   static bool isKnownLayout(LayoutConstraintKind kind);

   static bool isFixedSizeTrivial(LayoutConstraintKind kind);

   static bool isKnownSizeTrivial(LayoutConstraintKind kind);

   static bool isAddressOnlyTrivial(LayoutConstraintKind kind);

   static bool isTrivial(LayoutConstraintKind kind);

   static bool isRefCountedObject(LayoutConstraintKind kind);

   static bool isNativeRefCountedObject(LayoutConstraintKind kind);

   static bool isAnyRefCountedObject(LayoutConstraintKind kind);

   static bool isClass(LayoutConstraintKind kind);

   static bool isNativeClass(LayoutConstraintKind kind);

   static bool isRefCounted(LayoutConstraintKind kind);

   static bool isNativeRefCounted(LayoutConstraintKind kind);

   /// Uniquing for the LayoutConstraintInfo.
   void profile(llvm::FoldingSetNodeID &id)
   {
      profile(id, m_kind, m_sizeInBits, m_alignment);
   }

   static void profile(llvm::FoldingSetNodeID &id,
                       LayoutConstraintKind kind,
                       unsigned sizeInBits,
                       unsigned alignment);
private:
   // Make vanilla new/delete illegal for LayoutConstraintInfo.
   void *operator new(size_t bytes) noexcept = delete;
   void operator delete(void *data) noexcept = delete;
public:
   // Only allow allocation of LayoutConstraintInfo using the allocator in
   // AstContext or by doing a placement new.
   void *operator new(size_t bytes, const AstContext &ctx,
                      AllocationArena arena, unsigned alignment = 8);
   void *operator new(size_t bytes, void *mem) noexcept { return mem; }

   // Representation of the non-parameterized layouts.
   static LayoutConstraintInfo UnknownLayoutConstraintInfo;
   static LayoutConstraintInfo RefCountedObjectConstraintInfo;
   static LayoutConstraintInfo NativeRefCountedObjectConstraintInfo;
   static LayoutConstraintInfo ClassConstraintInfo;
   static LayoutConstraintInfo NativeClassConstraintInfo;
   static LayoutConstraintInfo TrivialConstraintInfo;
};

/// A wrapper class containing a reference to the actual LayoutConstraintInfo
/// object.
class LayoutConstraint
{
   LayoutConstraintInfo *m_ptr;
public:
   /*implicit*/ LayoutConstraint(LayoutConstraintInfo *ptr = nullptr)
      : m_ptr(ptr)
   {}

   static LayoutConstraint getLayoutConstraint(const LayoutConstraint &layout,
                                               AstContext &context);

   static LayoutConstraint getLayoutConstraint(LayoutConstraintKind kind,
                                               AstContext &context);

   static LayoutConstraint getLayoutConstraint(LayoutConstraintKind kind);

   static LayoutConstraint getLayoutConstraint(LayoutConstraintKind kind,
                                               unsigned sizeInBits,
                                               unsigned alignment,
                                               AstContext &context);

   static LayoutConstraint getUnknownLayout();

   LayoutConstraintInfo *getPointer() const
   {
      return m_ptr;
   }

   bool isNull() const
   {
      return m_ptr == nullptr;
   }

   LayoutConstraintInfo *operator->() const
   {
      return m_ptr;
   }

   /// Merge these two constraints and return a more specific one
   /// or fail if they're incompatible and return an unknown constraint.
   LayoutConstraint merge(LayoutConstraint Other);

   explicit operator bool() const
   {
      return m_ptr != nullptr;
   }

   void dump() const;
   void dump(raw_ostream &os, unsigned indent = 0) const;

   void print(raw_ostream &OS, const PrintOptions &printOption = PrintOptions()) const;
   void print(ASTPrinter &printer, const PrintOptions &printOption) const;

   /// Return the layout constraint as a string, for use in diagnostics only.
   std::string getString(const PrintOptions &printOption = PrintOptions()) const;

   bool operator==(LayoutConstraint rhs) const
   {
      if (isNull() && rhs.isNull()) {
         return true;
      }
      return *getPointer() == *rhs.getPointer();
   }

   bool operator!=(LayoutConstraint rhs) const
   {
      return !(*this == rhs);
   }
};

// Permit direct uses of isa/cast/dyn_cast on LayoutConstraint.
template <class X>
inline bool isa(LayoutConstraint layoutConstraint)
{
   return isa<X>(layoutConstraint.getPointer());
}

template <class X>
inline X cast_or_null(LayoutConstraint layoutConstraint)
{
   return cast_or_null<X>(layoutConstraint.getPointer());
}

template <class X>
inline X dyn_cast(LayoutConstraint layoutConstraint)
{
   return dyn_cast<X>(layoutConstraint.getPointer());
}

template <class X>
inline X dyn_cast_or_null(LayoutConstraint layoutConstraint)
{
   return dyn_cast_or_null<X>(layoutConstraint.getPointer());
}

/// LayoutConstraintLoc - Provides source location information for a
/// parsed layout constraint.
struct LayoutConstraintLoc
{
private:
   LayoutConstraint m_layout;
   SourceLoc m_loc;

public:
   LayoutConstraintLoc(LayoutConstraint layout, SourceLoc loc)
      : m_layout(layout),
        m_loc(loc) {}

   bool isError() const;

   // FIXME: We generally shouldn't need to build LayoutConstraintLoc without
   // a location.
   static LayoutConstraintLoc withoutLoc(LayoutConstraint layout)
   {
      return LayoutConstraintLoc(layout, SourceLoc());
   }

   /// Get the representative location of this type, for diagnostic
   /// purposes.
   SourceLoc getLoc() const
   {
      return m_loc;
   }

   SourceRange getSourceRange() const;

   bool hasLocation() const
   {
      return m_loc.isValid();
   }

   LayoutConstraint getLayoutConstraint() const
   {
      return m_layout;
   }

   void setLayoutConstraint(LayoutConstraint value)
   {
      m_layout = value;
   }

   bool isNull() const
   {
      return m_layout.isNull();
   }

   LayoutConstraintLoc clone(AstContext &ctx) const
   {
      return *this;
   }
};

/// Checks if id is a name of a layout constraint and returns this
/// constraint. If id does not match any known layout constraint names,
/// returns UnknownLayout.
LayoutConstraint getLayoutConstraint(TokenSyntax id, AstContext &ctx);

} // end namespace swift

LLVM_DECLARE_TYPE_ALIGNMENT(polar::ast::LayoutConstraintInfo, polar::ast::TypeAlignInBits)

namespace llvm {
   static inline raw_ostream &
         operator<<(raw_ostream &OS, polar::ast::LayoutConstraint layoutConstraint)
   {
      layoutConstraint->print(OS);
      return OS;
   }

   // A LayoutConstraint casts like a LayoutConstraintInfo*.
   template <>
   struct simplify_type<const ::polar::ast::LayoutConstraint>
   {
      typedef ::polar::ast::LayoutConstraintInfo *SimpleType;
      static SimpleType getSimplifiedValue(const ::polar::ast::LayoutConstraint &value)
      {
         return value.getPointer();
      }
   };

   template <>
   struct simplify_type<::polar::ast::LayoutConstraint>
         : public simplify_type<const ::polar::ast::LayoutConstraint>
   {};

   // LayoutConstraint hashes just like pointers.
   template <> struct DenseMapInfo<::polar::ast::LayoutConstraint>
   {
      static polar::ast::LayoutConstraint getEmptyKey()
      {
         return llvm::DenseMapInfo<polar::ast::LayoutConstraintInfo *>::getEmptyKey();
      }

      static polar::ast::LayoutConstraint getTombstoneKey()
      {
         return llvm::DenseMapInfo<polar::ast::LayoutConstraintInfo *>::getTombstoneKey();
      }

      static unsigned getHashValue(polar::ast::LayoutConstraint value) {
         return DenseMapInfo<polar::ast::LayoutConstraintInfo *>::getHashValue(
                  value.getPointer());
      }

      static bool isEqual(polar::ast::LayoutConstraint lhs,
                          polar::ast::LayoutConstraint rhs)
      {
         return lhs.getPointer() == rhs.getPointer();
      }
   };

   // A LayoutConstraint is "pointer like".
   template <>
   struct PointerLikeTypeTraits<polar::ast::LayoutConstraint>
   {
   public:
      static inline void *getAsVoidPointer(polar::ast::LayoutConstraint lc)
      {
         return reinterpret_cast<void *>(lc.getPointer());
      }

      static inline polar::ast::LayoutConstraint getFromVoidPointer(void *ptr)
      {
         return reinterpret_cast<polar::ast::LayoutConstraintInfo *>(ptr);
      }

      enum { NumLowBitsAvailable = polar::ast::TypeAlignInBits };
   };

} // polar::ast

#endif // POLARPHP_AST_LAYOUT_CONSTRAINT_H
