// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/25.

#ifndef POLARPHP_BASIC_ADT_POINTER_UNION_H
#define POLARPHP_BASIC_ADT_POINTER_UNION_H

#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/utils/PointerLikeTypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace polar {
namespace basic {

template <typename T> struct PointerUnionTypeSelectorReturn
{
   using Return = T;
};

/// Get a type based on whether two types are the same or not.
///
/// For:
///
/// \code
///   using Ret = typename PointerUnionTypeSelector<T1, T2, EQ, NE>::Return;
/// \endcode
///
/// Ret will be EQ type if T1 is same as T2 or NE type otherwise.
template <typename T1, typename T2, typename RET_EQ, typename RET_NE>
struct PointerUnionTypeSelector
{
   using Return = typename PointerUnionTypeSelectorReturn<RET_NE>::Return;
};

template <typename T, typename RET_EQ, typename RET_NE>
struct PointerUnionTypeSelector<T, T, RET_EQ, RET_NE>
{
   using Return = typename PointerUnionTypeSelectorReturn<RET_EQ>::Return;
};

template <typename T1, typename T2, typename RET_EQ, typename RET_NE>
struct PointerUnionTypeSelectorReturn<
      PointerUnionTypeSelector<T1, T2, RET_EQ, RET_NE>>
{
   using Return =
   typename PointerUnionTypeSelector<T1, T2, RET_EQ, RET_NE>::Return;
};

/// Provide PointerLikeTypeTraits for void* that is used by PointerUnion
/// for the two template arguments.
template <typename PT1, typename PT2>
class PointerUnionUIntTraits
{
public:
   static inline void *getAsVoidPointer(void *ptr)
   {
      return ptr;
   }

   static inline void *getFromVoidPointer(void *ptr)
   {
      return ptr;
   }

   enum {
      PT1BitsAv = (int)(PointerLikeTypeTraits<PT1>::NumLowBitsAvailable),
      PT2BitsAv = (int)(PointerLikeTypeTraits<PT2>::NumLowBitsAvailable),
      NumLowBitsAvailable = PT1BitsAv < PT2BitsAv ? PT1BitsAv : PT2BitsAv
   };
};

/// A discriminated union of two pointer types, with the discriminator in the
/// low bit of the pointer.
///
/// This implementation is extremely efficient in space due to leveraging the
/// low bits of the pointer, while exposing a natural and type-safe API.
///
/// Common use patterns would be something like this:
///    PointerUnion<int*, float*> P;
///    P = (int*)0;
///    printf("%d %d", P.is<int*>(), P.is<float*>());  // prints "1 0"
///    X = P.get<int*>();     // ok.
///    Y = P.get<float*>();   // runtime assertion failure.
///    Z = P.get<double*>();  // compile time failure.
///    P = (float*)0;
///    Y = P.get<float*>();   // ok.
///    X = P.get<int*>();     // runtime assertion failure.
template <typename PT1, typename PT2>
class PointerUnion
{
public:
   using ValueType =
   PointerIntPair<void *, 1, bool, PointerUnionUIntTraits<PT1, PT2>>;

private:
   ValueType m_value;

   struct IsPT1
   {
      static const int Num = 0;
   };
   struct IsPT2
   {
      static const int Num = 1;
   };
   template <typename T> struct UNION_DOESNT_CONTAIN_TYPE {};

public:
   PointerUnion() = default;
   PointerUnion(PT1 value)
      : m_value(const_cast<void *>(
                   PointerLikeTypeTraits<PT1>::getAsVoidPointer(value))) {}
   PointerUnion(PT2 value)
      : m_value(const_cast<void *>(PointerLikeTypeTraits<PT2>::getAsVoidPointer(value)),
                1)
   {}

   /// Test if the pointer held in the union is null, regardless of
   /// which type it is.
   bool isNull() const
   {
      // Convert from the void* to one of the pointer types, to make sure that
      // we recursively strip off low bits if we have a nested PointerUnion.
      return !PointerLikeTypeTraits<PT1>::getFromVoidPointer(m_value.getPointer());
   }

   explicit operator bool() const
   {
      return !isNull();
   }

   /// Test if the Union currently holds the type matching T.
   template <typename T> int is() const
   {
      using Ty = typename ::polar::basic::PointerUnionTypeSelector<
      PT1, T, IsPT1,
      ::polar::basic::PointerUnionTypeSelector<PT2, T, IsPT2,
      UNION_DOESNT_CONTAIN_TYPE<T>>>::Return;
      int TyNo = Ty::Num;
      return static_cast<int>(m_value.getInt()) == TyNo;
   }

   /// Returns the value of the specified pointer type.
   ///
   /// If the specified pointer type is incorrect, assert.
   template <typename T> T get() const
   {
      assert(is<T>() && "Invalid accessor called");
      return PointerLikeTypeTraits<T>::getFromVoidPointer(m_value.getPointer());
   }

   /// Returns the current pointer if it is of the specified pointer type,
   /// otherwises returns null.
   template <typename T> T dynamicCast() const
   {
      if (is<T>()) {
         return get<T>();
      }
      return T();
   }

   /// If the union is set to the first pointer type get an address pointing to
   /// it.
   PT1 const *getAddrOfPtr1() const
   {
      return const_cast<PointerUnion *>(this)->getAddrOfPtr1();
   }

   /// If the union is set to the first pointer type get an address pointing to
   /// it.
   PT1 *getAddrOfPtr1()
   {
      assert(is<PT1>() && "Val is not the first pointer");
      assert(
               get<PT1>() == m_value.getPointer() &&
               "Can't get the address because PointerLikeTypeTraits changes the ptr");
      return const_cast<PT1 *>(
               reinterpret_cast<const PT1 *>(m_value.getAddrOfPointer()));
   }

   /// Assignment from nullptr which just clears the union.
   const PointerUnion &operator=(std::nullptr_t)
   {
      m_value.initWithPointer(nullptr);
      return *this;
   }

   /// Assignment operators - Allow assigning into this union from either
   /// pointer type, setting the discriminator to remember what it came from.
   const PointerUnion &operator=(const PT1 &other)
   {
      m_value.initWithPointer(
               const_cast<void *>(PointerLikeTypeTraits<PT1>::getAsVoidPointer(other)));
      return *this;
   }
   const PointerUnion &operator=(const PT2 &other) {
      m_value.setPointerAndInt(
               const_cast<void *>(PointerLikeTypeTraits<PT2>::getAsVoidPointer(other)),
               1);
      return *this;
   }

   void *getOpaqueValue() const
   {
      return m_value.getOpaqueValue();
   }

   static inline PointerUnion getFromOpaqueValue(void *ptr)
   {
      PointerUnion value;
      value.m_value = ValueType::getFromOpaqueValue(ptr);
      return value;
   }
};

template <typename PT1, typename PT2>
bool operator==(PointerUnion<PT1, PT2> lhs, PointerUnion<PT1, PT2> rhs)
{
   return lhs.getOpaqueValue() == rhs.getOpaqueValue();
}

template <typename PT1, typename PT2>
bool operator!=(PointerUnion<PT1, PT2> lhs, PointerUnion<PT1, PT2> rhs)
{
   return lhs.getOpaqueValue() != rhs.getOpaqueValue();
}

template <typename PT1, typename PT2>
bool operator<(PointerUnion<PT1, PT2> lhs, PointerUnion<PT1, PT2> rhs)
{
   return lhs.getOpaqueValue() < rhs.getOpaqueValue();
}

} // basic

namespace utils {

using polar::basic::PointerUnion;

// Teach SmallPtrSet that PointerUnion is "basically a pointer", that has
// # low bits available = min(PT1bits,PT2bits)-1.
template <typename PT1, typename PT2>
struct PointerLikeTypeTraits<PointerUnion<PT1, PT2>>
{
   static inline void *getAsVoidPointer(const PointerUnion<PT1, PT2> &ptrUnion)
   {
      return ptrUnion.getOpaqueValue();
   }

   static inline PointerUnion<PT1, PT2> getFromVoidPointer(void *ptr)
   {
      return PointerUnion<PT1, PT2>::getFromOpaqueValue(ptr);
   }

   // The number of bits available are the min of the two pointer types.
   enum {
      NumLowBitsAvailable = PointerLikeTypeTraits<
      typename PointerUnion<PT1, PT2>::ValueType>::NumLowBitsAvailable
   };
};

} // utils

namespace basic {

/// A pointer union of three pointer types. See documentation for PointerUnion
/// for usage.
template <typename PT1, typename PT2, typename PT3> class PointerUnion3
{
public:
   using InnerUnion = PointerUnion<PT1, PT2>;
   using ValueType = PointerUnion<InnerUnion, PT3>;

private:
   ValueType m_value;

   struct IsInnerUnion
   {
      ValueType m_value;

      IsInnerUnion(ValueType value) : m_value(value)
      {}

      template <typename T> int is() const
      {
         return m_value.template is<InnerUnion>() &&
               m_value.template get<InnerUnion>().template is<T>();
      }

      template <typename T> T get() const
      {
         return m_value.template get<InnerUnion>().template get<T>();
      }
   };

   struct IsPT3
   {
      ValueType m_value;

      IsPT3(ValueType value) : m_value(value)
      {}

      template <typename T>
      int is() const
      {
         return m_value.template is<T>();
      }

      template <typename T>
      T get() const
      {
         return m_value.template get<T>();
      }
   };

public:
   PointerUnion3() = default;
   PointerUnion3(PT1 value)
   {
      m_value = InnerUnion(value);
   }

   PointerUnion3(PT2 value)
   {
      m_value = InnerUnion(value);
   }

   PointerUnion3(PT3 value)
   {
      m_value = value;
   }

   /// Test if the pointer held in the union is null, regardless of
   /// which type it is.
   bool isNull() const
   {
      return m_value.isNull();
   }

   explicit operator bool() const
   {
      return !isNull();
   }

   /// Test if the Union currently holds the type matching T.
   template <typename T> int is() const
   {
      // If T is PT1/PT2 choose IsInnerUnion otherwise choose IsPT3.
      using Ty = typename ::polar::basic::PointerUnionTypeSelector<
      PT1, T, IsInnerUnion,
      ::polar::basic::PointerUnionTypeSelector<PT2, T, IsInnerUnion, IsPT3>>::Return;
      return Ty(m_value).template is<T>();
   }

   /// Returns the value of the specified pointer type.
   ///
   /// If the specified pointer type is incorrect, assert.
   template <typename T> T get() const
   {
      assert(is<T>() && "Invalid accessor called");
      // If T is PT1/PT2 choose IsInnerUnion otherwise choose IsPT3.
      using Ty = typename ::polar::basic::PointerUnionTypeSelector<
      PT1, T, IsInnerUnion,
      ::polar::basic::PointerUnionTypeSelector<PT2, T, IsInnerUnion, IsPT3>>::Return;
      return Ty(m_value).template get<T>();
   }

   /// Returns the current pointer if it is of the specified pointer type,
   /// otherwises returns null.
   template <typename T> T dynamicCast() const
   {
      if (is<T>()) {
         return get<T>();
      }
      return T();
   }

   /// Assignment from nullptr which just clears the union.
   const PointerUnion3 &operator=(std::nullptr_t)
   {
      m_value = nullptr;
      return *this;
   }

   /// Assignment operators - Allow assigning into this union from either
   /// pointer type, setting the discriminator to remember what it came from.
   const PointerUnion3 &operator=(const PT1 &other)
   {
      m_value = InnerUnion(other);
      return *this;
   }
   const PointerUnion3 &operator=(const PT2 &other)
   {
      m_value = InnerUnion(other);
      return *this;
   }

   const PointerUnion3 &operator=(const PT3 &other)
   {
      m_value = other;
      return *this;
   }

   void *getOpaqueValue() const
   {
      return m_value.getOpaqueValue();
   }

   static inline PointerUnion3 getFromOpaqueValue(void *ptr)
   {
      PointerUnion3 value;
      value.m_value = ValueType::getFromOpaqueValue(ptr);
      return value;
   }
};

} // basic

namespace utils {

using polar::basic::PointerUnion3;

// Teach SmallPtrSet that PointerUnion3 is "basically a pointer", that has
// # low bits available = min(PT1bits,PT2bits,PT2bits)-2.
template <typename PT1, typename PT2, typename PT3>
struct PointerLikeTypeTraits<PointerUnion3<PT1, PT2, PT3>>
{
   static inline void *getAsVoidPointer(const PointerUnion3<PT1, PT2, PT3> &ptr)
   {
      return ptr.getOpaqueValue();
   }

   static inline PointerUnion3<PT1, PT2, PT3> getFromVoidPointer(void *ptr)
   {
      return PointerUnion3<PT1, PT2, PT3>::getFromOpaqueValue(ptr);
   }

   // The number of bits available are the min of the two pointer types.
   enum {
      NumLowBitsAvailable = PointerLikeTypeTraits<
      typename PointerUnion3<PT1, PT2, PT3>::ValueType>::NumLowBitsAvailable
   };
};

} // utils

namespace basic {

template <typename PT1, typename PT2, typename PT3>
bool operator<(PointerUnion3<PT1, PT2, PT3> lhs,
               PointerUnion3<PT1, PT2, PT3> rhs)
{
   return lhs.getOpaqueValue() < rhs.getOpaqueValue();
}

/// A pointer union of four pointer types. See documentation for PointerUnion
/// for usage.
template <typename PT1, typename PT2, typename PT3, typename PT4>
class PointerUnion4
{
public:
   using InnerUnion1 = PointerUnion<PT1, PT2>;
   using InnerUnion2 = PointerUnion<PT3, PT4>;
   using ValueType = PointerUnion<InnerUnion1, InnerUnion2>;

private:
   ValueType m_value;

public:
   PointerUnion4() = default;
   PointerUnion4(PT1 value)
   {
      m_value = InnerUnion1(value);
   }

   PointerUnion4(PT2 value)
   {
      m_value = InnerUnion1(value);
   }

   PointerUnion4(PT3 value)
   {
      m_value = InnerUnion2(value);
   }

   PointerUnion4(PT4 value)
   {
      m_value = InnerUnion2(value);
   }

   /// Test if the pointer held in the union is null, regardless of
   /// which type it is.
   bool isNull() const
   {
      return m_value.isNull();
   }

   explicit operator bool() const
   {
      return !isNull();
   }

   /// Test if the Union currently holds the type matching T.
   template <typename T> int is() const
   {
      // If T is PT1/PT2 choose InnerUnion1 otherwise choose InnerUnion2.
      using Ty = typename ::polar::basic::PointerUnionTypeSelector<
      PT1, T, InnerUnion1,
      ::polar::basic::PointerUnionTypeSelector<PT2, T, InnerUnion1,
      InnerUnion2>>::Return;
      return m_value.template is<Ty>() && m_value.template get<Ty>().template is<T>();
   }

   /// Returns the value of the specified pointer type.
   ///
   /// If the specified pointer type is incorrect, assert.
   template <typename T> T get() const
   {
      assert(is<T>() && "Invalid accessor called");
      // If T is PT1/PT2 choose InnerUnion1 otherwise choose InnerUnion2.
      using Ty = typename ::polar::basic::PointerUnionTypeSelector<
      PT1, T, InnerUnion1,
      ::polar::basic::PointerUnionTypeSelector<PT2, T, InnerUnion1,
      InnerUnion2>>::Return;
      return m_value.template get<Ty>().template get<T>();
   }

   /// Returns the current pointer if it is of the specified pointer type,
   /// otherwises returns null.
   template <typename T> T dynamicCast() const
   {
      if (is<T>()) {
         return get<T>();
      }
      return T();
   }

   /// Assignment from nullptr which just clears the union.
   const PointerUnion4 &operator=(std::nullptr_t)
   {
      m_value = nullptr;
      return *this;
   }

   /// Assignment operators - Allow assigning into this union from either
   /// pointer type, setting the discriminator to remember what it came from.
   const PointerUnion4 &operator=(const PT1 &other)
   {
      m_value = InnerUnion1(other);
      return *this;
   }

   const PointerUnion4 &operator=(const PT2 &other)
   {
      m_value = InnerUnion1(other);
      return *this;
   }

   const PointerUnion4 &operator=(const PT3 &other)
   {
      m_value = InnerUnion2(other);
      return *this;
   }

   const PointerUnion4 &operator=(const PT4 &other)
   {
      m_value = InnerUnion2(other);
      return *this;
   }

   void *getOpaqueValue() const
   {
      return m_value.getOpaqueValue();
   }

   static inline PointerUnion4 getFromOpaqueValue(void *valuePtr)
   {
      PointerUnion4 value;
      value.m_value = ValueType::getFromOpaqueValue(valuePtr);
      return value;
   }
};

} // basic

namespace utils {

using polar::basic::PointerUnion4;

// Teach SmallPtrSet that PointerUnion4 is "basically a pointer", that has
// # low bits available = min(PT1bits,PT2bits,PT2bits)-2.
template <typename PT1, typename PT2, typename PT3, typename PT4>
struct PointerLikeTypeTraits<PointerUnion4<PT1, PT2, PT3, PT4>>
{
   static inline void *
   getAsVoidPointer(const PointerUnion4<PT1, PT2, PT3, PT4> &ptr)
   {
      return ptr.getOpaqueValue();
   }

   static inline PointerUnion4<PT1, PT2, PT3, PT4> getFromVoidPointer(void *ptr)
   {
      return PointerUnion4<PT1, PT2, PT3, PT4>::getFromOpaqueValue(ptr);
   }

   // The number of bits available are the min of the two pointer types.
   enum {
      NumLowBitsAvailable = PointerLikeTypeTraits<
      typename PointerUnion4<PT1, PT2, PT3, PT4>::ValueType>::NumLowBitsAvailable
   };
};

} // utils

namespace basic {

// Teach DenseMap how to use PointerUnions as keys.
template <typename T, typename U>
struct DenseMapInfo<PointerUnion<T, U>>
{
   using Pair = PointerUnion<T, U>;
   using FirstInfo = DenseMapInfo<T>;
   using SecondInfo = DenseMapInfo<U>;

   static inline Pair getEmptyKey()
   {
      return Pair(FirstInfo::getEmptyKey());
   }

   static inline Pair getTombstoneKey()
   {
      return Pair(FirstInfo::getTombstoneKey());
   }

   static unsigned getHashValue(const Pair &pairVal)
   {
      intptr_t key = (intptr_t)pairVal.getOpaqueValue();
      return DenseMapInfo<intptr_t>::getHashValue(key);
   }

   static bool isEqual(const Pair &lhs, const Pair &rhs)
   {
      return lhs.template is<T>() == rhs.template is<T>() &&
            (lhs.template is<T>() ? FirstInfo::isEqual(lhs.template get<T>(),
                                                       rhs.template get<T>())
                                  : SecondInfo::isEqual(lhs.template get<U>(),
                                                        rhs.template get<U>()));
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_POINTER_UNION_H
