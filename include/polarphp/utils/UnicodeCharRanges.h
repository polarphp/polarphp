// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_UNICODE_CHARRANGES_H
#define POLARPHP_UTILS_UNICODE_CHARRANGES_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>

#define DEBUG_TYPE "unicode"

namespace polar {
namespace sys {

using polar::basic::ArrayRef;
using polar::debug_stream;

/// \brief Represents a closed range of Unicode code points [m_lower, m_upper].
struct UnicodeCharRange
{
   uint32_t m_lower;
   uint32_t m_upper;
};

inline bool operator<(uint32_t value, UnicodeCharRange range)
{
   return value < range.m_lower;
}

inline bool operator<(UnicodeCharRange range, uint32_t value)
{
   return range.m_upper < value;
}

/// \brief Holds a reference to an ordered array of UnicodeCharRange and allows
/// to quickly check if a code point is contained in the set represented by this
/// array.
class UnicodeCharSet
{
public:
   typedef ArrayRef<UnicodeCharRange> CharRanges;

   /// \brief Constructs a UnicodeCharSet instance from an array of
   /// UnicodeCharRanges.
   ///
   /// Array pointed by \p ranges should have the lifetime at least as long as
   /// the UnicodeCharSet instance, and should not change. Array is validated by
   /// the constructor, so it makes sense to create as few UnicodeCharSet
   /// instances per each array of ranges, as possible.
#ifdef NDEBUG

   // FIXME: This could use constexpr + static_assert. This way we
   // may get rid of NDEBUG in this header. Unfortunately there are some
   // problems to get this working with MSVC 2013. Change this when
   // the support for MSVC 2013 is dropped.
   constexpr UnicodeCharSet(CharRanges ranges) : ranges(ranges) {}
#else
   UnicodeCharSet(CharRanges ranges)
      : m_ranges(ranges)
   {
      assert(rangesAreValid());
   }
#endif

   /// \brief Returns true if the character set contains the Unicode code point
   /// \p C.
   bool contains(uint32_t value) const
   {
      return std::binary_search(m_ranges.begin(), m_ranges.end(), value);
   }

private:
   /// \brief Returns true if each of the ranges is a proper closed range
   /// [min, max], and if the ranges themselves are ordered and non-overlapping.
   bool rangesAreValid() const
   {
      uint32_t prev = 0;
      for (CharRanges::const_iterator iter = m_ranges.begin(), end = m_ranges.end();
           iter != end; ++iter)
      {
         if (iter != m_ranges.begin() && prev >= iter->m_lower) {
            POLAR_DEBUG(debug_stream() << "m_upper bound 0x");
            POLAR_DEBUG(debug_stream().writeHex(prev));
            POLAR_DEBUG(debug_stream() << " should be less than succeeding lower bound 0x");
            POLAR_DEBUG(debug_stream().writeHex(iter->m_lower) << "\n");
            return false;
         }
         if (iter->m_upper < iter->m_lower) {
            POLAR_DEBUG(debug_stream() << "m_upper bound 0x");
            POLAR_DEBUG(debug_stream().writeHex(iter->m_lower));
            POLAR_DEBUG(debug_stream() << " should not be less than lower bound 0x");
            POLAR_DEBUG(debug_stream().writeHex(iter->m_upper) << "\n");
            return false;
         }
         prev = iter->m_upper;
      }

      return true;
   }

   const CharRanges m_ranges;
};
} // sys
} // polar

#undef DEBUG_TYPE // "unicode"

#endif // POLARPHP_UTILS_UNICODE_CHARRANGES_H
