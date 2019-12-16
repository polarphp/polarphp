//===- OptionalEnum.h - Space-efficient variant for enum values -*- C++ -*-===//
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
// Created by polarboy on 2019/12/02.


#ifndef POLARPHP_BASIC_OPTIONAL_ENUM_H
#define POLARPHP_BASIC_OPTIONAL_ENUM_H

#include <type_traits>
#include "llvm/ADT/Optional.h"

namespace polar {
template<typename T>
class OptionalEnum {
   using underlying_type = typename std::underlying_type<T>::type;
   static_assert(std::is_trivially_copyable<T>::value, "type is not trivial");
   static_assert(std::is_integral<underlying_type>::value,
                 "underlying type is not integral");
public:
   using value_type = T;
   using storage_type = typename std::make_unsigned<underlying_type>::type;

private:
   storage_type Storage;

public:
   /// Construct an empty instance.
   OptionalEnum() : Storage(0) { }

   /// Construct an empty instance.
   /*implicit*/ OptionalEnum(llvm::NoneType) : OptionalEnum() {}

   /// Construct an instance containing a value of type \c T constructed with
   /// the given argument.
   template<typename U>
   /*implicit*/ OptionalEnum(
         U &&value,
         typename std::enable_if<!std::is_integral<U>::value>::type * = {})
      : Storage(static_cast<storage_type>(T{std::forward<U>(value)}) + 1) {
      assert(hasValue() && "cannot store this value");
   }

   /// Construct the enum from its raw integral representation.
   ///
   /// This can be used to interoperate with PointerIntPair.
   template<typename I>
   explicit OptionalEnum(
         I rawValue,
         typename std::enable_if<std::is_integral<I>::value>::type * = {})
      : Storage(rawValue) {
      assert((uintptr_t)rawValue == (uintptr_t)(intptr_t)*this);
   }

   void reset() {
      Storage = 0;
   }

   bool hasValue() const { return Storage != 0; }
   explicit operator bool() const { return hasValue(); }

   T getValue() const {
      assert(hasValue());
      return static_cast<value_type>(Storage - 1);
   }
   T operator*() { return getValue(); }

   template <typename U>
   constexpr T getValueOr(U &&value) const {
      return hasValue() ? getValue() : value;
   }

   /// Converts the enum to its raw storage value, for interoperation with
   /// PointerIntPair.
   explicit operator intptr_t() const {
      assert(Storage == (storage_type)(intptr_t)Storage);
      return (intptr_t)Storage;
   }
};
} // end namespace polar

#endif // POLARPHP_BASIC_OPTIONAL_ENUM_H
