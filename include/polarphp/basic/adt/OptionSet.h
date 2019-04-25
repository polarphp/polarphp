//===--- OptionSet.h - Sets of boolean options ------------------*- C++ -*-===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/25.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the OptionSet class template.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ADT_OPTION_SET_H
#define POLARPHP_BASIC_ADT_OPTION_SET_H

#include <optional>
#include <type_traits>
#include <cstdint>

namespace polar::basic {

/// The class template \c OptionSet captures a set of options stored as the
/// bits in an unsigned integral value.
///
/// Each option corresponds to a particular flag value in the provided
/// enumeration type (\c Flags). The option set provides ways to add options,
/// remove options, intersect sets, etc., providing a thin type-safe layer
/// over the underlying unsigned value.
///
/// \tparam Flags An enumeration type that provides the individual flags
/// for options. Each enumerator should have a power-of-two value, indicating
/// which bit it is associated with.
///
/// \tparam StorageType The unsigned integral type to use to store the flags
/// enabled within this option set. This defaults to the unsigned form of the
/// underlying type of the enumeration.
template<typename Flags,
         typename StorageType = typename std::make_unsigned<
            typename std::underlying_type<Flags>::type
            >::type>
class OptionSet
{
   StorageType m_storage;

public:
   /// Create an empty option set.
   OptionSet() : m_storage()
   {}

   /// Create an empty option set.
   OptionSet(llvm::NoneType) : m_storage()
   {}

   /// Create an option set with only the given option set.
   OptionSet(Flags flag)
      : m_storage(static_cast<StorageType>(flag))
   {}

   /// Create an option set from raw storage.
   explicit OptionSet(StorageType storage)
      : m_storage(storage) {}

   /// Check whether an option set is non-empty.
   explicit operator bool() const
   {
      return m_storage != 0;
   }

   /// Explicitly convert an option set to its underlying storage.
   explicit operator StorageType() const
   {
      return m_storage;
   }

   /// Explicitly convert an option set to intptr_t, for use in
   /// llvm::PointerIntPair.
   ///
   /// This member is not present if the underlying type is bigger than
   /// a pointer.
   template <typename T = std::intptr_t>
   explicit operator typename std::enable_if<sizeof(StorageType) <= sizeof(T),
   std::intptr_t>::type () const
   {
      return static_cast<intptr_t>(m_storage);
   }

   /// Retrieve the "raw" representation of this option set.
   StorageType toRaw() const
   {
      return m_storage;
   }

   /// Determine whether this option set contains all of the options in the
   /// given set.
   bool contains(OptionSet set) const
   {
      return !static_cast<bool>(set - *this);
   }

   /// Produce the union of two option sets.
   friend OptionSet operator|(OptionSet lhs, OptionSet rhs)
   {
      return OptionSet(lhs.m_storage | rhs.m_storage);
   }

   /// Produce the union of two option sets.
   friend OptionSet &operator|=(OptionSet &lhs, OptionSet rhs)
   {
      lhs.m_storage |= rhs.m_storage;
      return lhs;
   }

   /// Produce the intersection of two option sets.
   friend OptionSet operator&(OptionSet lhs, OptionSet rhs)
   {
      return OptionSet(lhs.m_storage & rhs.m_storage);
   }

   /// Produce the intersection of two option sets.
   friend OptionSet &operator&=(OptionSet &lhs, OptionSet rhs)
   {
      lhs.m_storage &= rhs.m_storage;
      return lhs;
   }

   /// Produce the difference of two option sets.
   friend OptionSet operator-(OptionSet lhs, OptionSet rhs)
   {
      return OptionSet(lhs.m_storage & ~rhs.m_storage);
   }

   /// Produce the intersection of two option sets.
   friend OptionSet &operator-=(OptionSet &lhs, OptionSet rhs)
   {
      lhs.m_storage &= ~rhs.m_storage;
      return lhs;
   }

private:
   template <typename T>
   static auto _checkResultTypeOperatorOr(T t) -> decltype(t | t)
   {
      return T();
   }

   static void _checkResultTypeOperatorOr(...)
   {}

   static_assert(!std::is_same<decltype(_checkResultTypeOperatorOr(Flags())),
                 Flags>::value,
                 "operator| should produce an OptionSet");
};

} // polar::basic

#endif // POLARPHP_BASIC_ADT_OPTION_SET_H
