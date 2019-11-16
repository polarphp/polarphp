//===--- AnyValue.h - Any Value Existential ---------------------*- C++ -*-===//
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
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
//  This file defines the AnyValue class, which is used to store an
//  immutable m_value of any type.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ANYVALUE_H
#define POLARPHP_BASIC_ANYVALUE_H

#include "polarphp/basic/SimpleDisplay.h"
#include "polarphp/basic/TypeId.h"
#include "llvm/ADTPointerUnion.h"  // to define hash_m_value

namespace polar::basic {
template<typename PT1, typename PT2>
HashCode hash_m_value(const PointerUnion<PT1, PT2> &ptr)
{
   return hash_m_value(ptr.getOpaqueValue());
}

/// Stores a m_value of any type that satisfies a small set of requirements.
///
/// Requirements on the values m_stored within an AnyValue:
///
///   - Copy constructor
///   - Equality operator (==)
///   - TypeId support (see swift/Basic/TypeId.h)
///   - Display support (free function):
///       void simple_display(raw_ostream &, const T &);
class AnyValue
{
   /// Abstract base class used to hold on to a m_value.
   class HolderBase
   {
   public:
      /// Type ID number.
      const uint64_t m_typeId;

      HolderBase() = delete;
      HolderBase(const HolderBase &) = delete;
      HolderBase(HolderBase &&) = delete;
      HolderBase &operator=(const HolderBase &) = delete;
      HolderBase &operator=(HolderBase &&) = delete;

      /// Initialize base with type ID.
      HolderBase(uint64_t typeId)
         : m_typeId(typeId)
      {}

      virtual ~HolderBase();

      /// Determine whether this m_value is equivalent to another.
      ///
      /// The caller guarantees that the type IDs are the same.
      virtual bool equals(const HolderBase &other) const = 0;

      /// Display.
      virtual void display(raw_ostream &out) const = 0;
   };

   /// Holds a m_value that can be used as a request input/output.
   template<typename T>
   class Holder final : public HolderBase
   {
   public:
      const T m_value;

      Holder(T &&value)
         : HolderBase(TypeId<T>::m_value),
           m_value(std::move(value))
      {}

      Holder(const T &value)
         : HolderBase(TypeId<T>::m_value),
           m_value(value)
      {}

      virtual ~Holder()
      {}

      /// Determine whether this m_value is equivalent to another.
      ///
      /// The caller guarantees that the type IDs are the same.
      virtual bool equals(const HolderBase &other) const override
      {
         assert(m_typeId == other.m_typeId && "Caller should match type IDs");
         return m_value == static_cast<const Holder<T> &>(other).m_value;
      }

      /// Display.
      virtual void display(raw_ostream &out) const override
      {
         simple_display(out, m_value);
      }
   };

   /// The data m_stored in this m_value.
   std::shared_ptr<HolderBase> m_stored;

public:
   /// Construct a new instance with the given m_value.
   template<typename T>
   AnyValue(T&& value)
   {
      using ValueType = typename std::remove_reference<T>::type;
      m_stored.reset(new Holder<ValueType>(std::forward<T>(value)));
   }

   /// Cast to a specific (known) type.
   template<typename T>
   const T &castTo() const
   {
      assert(m_stored->m_typeId == TypeId<T>::m_value);
      return static_cast<const Holder<T> *>(m_stored.get())->m_value;
   }

   /// Try casting to a specific (known) type, returning \c nullptr on
   /// failure.
   template<typename T>
   const T *getAs() const
   {
      if (m_stored->m_typeId != TypeId<T>::m_value) {
         return nullptr;
      }
      return &static_cast<const Holder<T> *>(m_stored.get())->m_value;
   }

   /// Compare two instances for equality.
   friend bool operator==(const AnyValue &lhs, const AnyValue &rhs)
   {
      if (lhs.m_stored->m_typeId != rhs.m_stored->m_typeId) {
         return false;
      }
      return lhs.m_stored->equals(*rhs.m_stored);
   }

   friend bool operator!=(const AnyValue &lhs, const AnyValue &rhs)
   {
      return !(lhs == rhs);
   }

   friend void simple_display(raw_ostream &out, const AnyValue &value)
   {
      m_stored->display(out);
   }

   /// Return the result of calling simple_display as a string.
   std::string getAsString() const;
};

} // polar::basic

#endif // POLARPHP_BASIC_ANYVALUE_H
