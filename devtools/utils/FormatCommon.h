// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/11.

#ifndef POLAR_DEVLTOOLS_UTILS_FORMAT_COMMON_H
#define POLAR_DEVLTOOLS_UTILS_FORMAT_COMMON_H

#include <string>
#include <string_view>
#include <ostream>
#include <sstream>

#include "FormatVariadicDetail.h"

namespace polar {
namespace utils {

enum class AlignStyle { Left, Center, Right };

struct FmtAlign
{
   internal::FormatAdapter &m_adapter;
   AlignStyle m_where;
   size_t m_amount;
   char m_fill;

   FmtAlign(internal::FormatAdapter &adapter, AlignStyle where, size_t amount,
            char fill = ' ')
      : m_adapter(adapter), m_where(where),
        m_amount(amount), m_fill(fill)
   {}

   void format(std::ostream &out, std::string_view options)
   {
      // If we don't need to align, we can format straight into the underlying
      // stream.  Otherwise we have to go through an intermediate stream first
      // in order to calculate how long the output is so we can align it.
      // TODO: Make the format method return the number of bytes written, that
      // way we can also skip the intermediate stream for left-aligned output.
      if (m_amount == 0) {
         m_adapter.format(out, options);
         return;
      }
      std::string item;
      std::stringstream stream(item);
      m_adapter.format(stream, options);
      if (m_amount <= item.size()) {
         out << item;
         return;
      }

      size_t padAmount = m_amount - item.size();
      switch (m_where) {
      case AlignStyle::Left:
         out << item;
         fill(out, padAmount);
         break;
      case AlignStyle::Center: {
         size_t x = padAmount / 2;
         fill(out, x);
         out << item;
         fill(out, padAmount - x);
         break;
      }
      default:
         fill(out, padAmount);
         out << item;
         break;
      }
   }

private:
   void fill(std::ostream &out, uint32_t count)
   {
      for (uint32_t index = 0; index < count; ++index) {
         out << m_fill;
      }
   }
};

} // utils
} // polar

#endif // POLAR_DEVLTOOLS_UTILS_FORMAT_COMMON_H
