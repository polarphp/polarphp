
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/03.

#ifndef POLARPHP_UTILS_FORMAT_ADAPTERS_H
#define POLARPHP_UTILS_FORMAT_ADAPTERS_H

#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/FormatCommon.h"
#include "polarphp/utils/FormatVariadicDetail.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

template <typename T>
class FormatAdapter : public internal::FormatAdapterImpl
{
protected:
   explicit FormatAdapter(T &&item) : m_item(std::forward<T>(item))
   {}

   T m_item;
};

namespace internal {
template <typename T>
class AlignAdapter final : public FormatAdapter<T>
{
   AlignStyle m_where;
   size_t m_amount;
   char m_fill;

public:
   AlignAdapter(T &&item, AlignStyle where, size_t amount, char fill)
      : FormatAdapter<T>(std::forward<T>(item)), m_where(where), m_amount(amount),
        m_fill(fill)
   {}

   void format(polar::utils::RawOutStream &stream, StringRef style)
   {
      auto adapter = internal::build_format_adapter(std::forward<T>(this->m_item));
      FmtAlign(adapter, m_where, m_amount, m_fill).format(stream, style);
   }
};

template <typename T>
class PadAdapter final : public FormatAdapter<T>
{
   size_t m_left;
   size_t m_right;

public:
   PadAdapter(T &&item, size_t left, size_t right)
      : FormatAdapter<T>(std::forward<T>(item)), m_left(left), m_right(right)
   {}

   void format(polar::utils::RawOutStream &stream, StringRef style)
   {
      auto adapter = internal::build_format_adapter(std::forward<T>(this->m_item));
      stream.indent(m_left);
      adapter.format(stream, style);
      stream.indent(m_right);
   }
};

template <typename T>
class RepeatAdapter final : public FormatAdapter<T>
{
   size_t m_count;

public:
   RepeatAdapter(T &&item, size_t count)
      : FormatAdapter<T>(std::forward<T>(item)), m_count(count)
   {}

   void format(polar::utils::RawOutStream &stream, StringRef style)
   {
      auto adapter = internal::build_format_adapter(std::forward<T>(this->m_item));
      for (size_t index = 0; index < m_count; ++index) {
         adapter.format(stream, style);
      }
   }
};

class ErrorAdapter : public FormatAdapter<Error>
{
public:
   ErrorAdapter(Error &&item) : FormatAdapter(std::move(item))
   {}
   ErrorAdapter(ErrorAdapter &&) = default;
   ~ErrorAdapter()
   {
      consume_error(std::move(m_item));
   }
   void format(RawOutStream &stream, StringRef)
   {
      stream << m_item;
   }
};

} // internal

template <typename T>
internal::AlignAdapter<T> fmt_align(T &&item, AlignStyle where, size_t amount,
                                    char fill = ' ')
{
   return internal::AlignAdapter<T>(std::forward<T>(item), where, amount, fill);
}

template <typename T>
internal::PadAdapter<T> fmt_pad(T &&item, size_t left, size_t right)
{
   return internal::PadAdapter<T>(std::forward<T>(item), left, right);
}

template <typename T>
internal::RepeatAdapter<T> fmt_repeat(T &&item, size_t count)
{
   return internal::RepeatAdapter<T>(std::forward<T>(item), count);
}

// llvm::Error values must be consumed before being destroyed.
// Wrapping an error in fmt_consume explicitly indicates that the formatv_object
// should take ownership and consume it.
inline internal::ErrorAdapter fmt_consume(Error &&item)
{
   return internal::ErrorAdapter(std::move(item));
}

} // utils
} // polar

#endif // POLARPHP_UTILS_FORMAT_ADAPTERS_H
