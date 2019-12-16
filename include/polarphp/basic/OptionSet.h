// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_BASIC_OPTION_SET_H
#define POLARPHP_BASIC_OPTION_SET_H

#include "llvm/ADT/None.h"

#include <type_traits>
#include <cstdint>

namespace polar {

using llvm::None;

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
public:
   /// Create an empty option set.
   OptionSet()
      : m_storage() {}

   /// Create an empty option set.
   OptionSet(llvm::NoneType)
      : m_storage() {}

   /// Create an option set with only the given option set.
   OptionSet(Flags flag)
      : m_storage(static_cast<StorageType>(flag)) {}

   /// Create an option set from raw storage.
   explicit OptionSet(StorageType storage)
      : m_storage(storage)
   {}

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

   /// Check if this option set contains the exact same options as the given set.
   bool containsOnly(OptionSet set)
   {
      return m_storage == set.m_storage;
   }

   // '==' and '!=' are deliberately not defined because they provide a pitfall
   // where someone might use '==' but really want 'contains'. If you actually
   // want '==' behavior, use 'containsOnly'.

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

   /// Produce the difference of two option sets.
   friend OptionSet &operator-=(OptionSet &lhs, OptionSet rhs)
   {
      lhs.m_storage &= ~rhs.m_storage;
      return lhs;
   }

private:
   template <typename T>
   static auto checkResultTypeOperatorOr(T t) -> decltype(t | t) { return T(); }

   static void checkResultTypeOperatorOr(...) {}

   static_assert(!std::is_same<decltype(checkResultTypeOperatorOr(Flags())),
                 Flags>::value,
                 "operator| should produce an OptionSet");

   StorageType m_storage;
};

} // polar

#endif // POLARPHP_BASIC_OPTION_SET_H
