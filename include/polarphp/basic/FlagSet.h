//===--- FlagSet.h - Helper class for opaque flag types ---------*- C++ -*-===//
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
// This file defines the FlagSet template, a class which makes it easier to
// define opaque flag types.
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_FLAG_SET_H
#define POLARPHP_BASIC_FLAG_SET_H

#include <type_traits>
#include <assert.h>

namespace polar::basic {

// A convenient macro for defining a getter and setter for a flag.
 // Intended to be used in the body of a subclass of FlagSet.
#define FLAGSET_DEFINE_FLAG_ACCESSORS(BIT, GETTER, SETTER) \
 bool GETTER() const                                      \
 {                                                        \
   return this->template getFlag<BIT>();                  \
 }                                                        \
 void SETTER(bool value)                                  \
 {                                                        \
   this->template setFlag<BIT>(value);                    \
 }

 // A convenient macro for defining a getter and setter for a field.
 // Intended to be used in the body of a subclass of FlagSet.
#define FLAGSET_DEFINE_FIELD_ACCESSORS(BIT, WIDTH, TYPE, GETTER, SETTER) \
 TYPE GETTER() const                                                    \
 {                                                                      \
   return this->template getField<BIT, WIDTH, TYPE>();                  \
 }                                                                      \
 void SETTER(TYPE value)                                                \
 {                                                                      \
   this->template setField<BIT, WIDTH, TYPE>(value);                    \
 }

 // A convenient macro to expose equality operators.
 // These can't be provided directly by FlagSet because that would allow
 // different flag sets to be compared if they happen to have the same
 // underlying type.
#define FLAGSET_DEFINE_EQUALITY(TYPENAME)                               \
 friend bool operator==(TYPENAME lhs, TYPENAME rhs)                     \
 {                                                                      \
   return lhs.getOpaqueValue() == rhs.getOpaqueValue();                 \
 }                                                                      \
 friend bool operator!=(TYPENAME lhs, TYPENAME rhs)                     \
 {                                                                      \
   return lhs.getOpaqueValue() != rhs.getOpaqueValue();                 \
 }

/// A template designed to simplify the task of defining a wrapper type
/// for a flags bitfield.
///
/// Unfortunately, this doesn't currently support functional-style
/// building patterns, which means this can't practically be used for
/// types that need to be used in constant expressions.
template <typename StorageType>
class FlagSet
{
public:
   StorageType getOpaqueValue() const
   {
      return m_bits;
   }
protected:
   template <unsigned int BitWidth>
   static constexpr StorageType makeFieldMask()
   {
      return StorageType((1 << BitWidth) - 1);
   }

   template <unsigned int TargetBitOffset, unsigned int BitFieldWidth = 1>
   static constexpr StorageType makeTargetBitMask()
   {
      return makeFieldMask<BitFieldWidth>() << TargetBitOffset;
   }

   constexpr FlagSet(StorageType bits = 0)
      : m_bits(bits)
   {}

   template <unsigned int TargetBitOffset>
   bool getFlag() const
   {
      return m_bits & makeTargetBitMask<TargetBitOffset>();
   }

   template <unsigned int TargetBitOffset>
   void setFlag(bool flag)
   {
      if (flag) {
         m_bits |= makeTargetBitMask<TargetBitOffset>();
      } else {
         m_bits &= ~makeTargetBitMask<TargetBitOffset>();
      }
   }

   template <unsigned int FirstBitOffset, unsigned int BitWidth,
             typename FieldType = StorageType>
   FieldType getField() const
   {
      return FieldType((m_bits >> FirstBitOffset) & makeFieldMask<BitWidth>());
   }

   template <unsigned int FirstBitOffset, unsigned int BitWidth,
             typename FieldType = StorageType>
   void setField(typename std::enable_if<true, FieldType>::type value)
   {
      assert(static_cast<StorageType>(value) <= makeFieldMask<BitWidth>() && "value out of range");
      m_bits = (m_bits & ~makeTargetBitMask<FirstBitOffset, BitWidth>) | (value << FirstBitOffset);
   }

private:
   static_assert(std::is_integral_v<StorageType>,
                 "storage type for FlagSet must be an integral type");
   StorageType m_bits;
};

} // polar::basic

#endif // POLARPHP_BASIC_FLAG_SET_H
