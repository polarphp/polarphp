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

#ifndef POLARPHP_UTILS_ENDIAN_STREAM_H
#define POLARPHP_UTILS_ENDIAN_STREAM_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/Endian.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {
namespace endian {

using polar::basic::ArrayRef;

template <typename value_type>
inline void write(RawOutStream &out, value_type value, Endianness endian)
{
   value = byte_swap<value_type>(value, endian);
   out.write((const char *)&value, sizeof(value_type));
}

template <>
inline void write<float>(RawOutStream &out, float value, Endianness endian)
{
   write(out, float_to_bits(value), endian);
}

template <>
inline void write<double>(RawOutStream &out, double value,
                          Endianness endian)
{
   write(out, double_to_bits(value), endian);
}

template <typename value_type>
inline void write(RawOutStream &out, ArrayRef<value_type> vals,
                  Endianness endian)
{
   for (value_type v : vals) {
       write(out, v, endian);
   }
}

/// Adapter to write values to a stream in a particular byte order.
struct Writer
{
   RawOutStream &m_out;
   Endianness m_endian;
   Writer(RawOutStream &out, Endianness endianType)
      : m_out(out),
        m_endian(endianType)
   {}
   template <typename value_type> void write(ArrayRef<value_type> value)
   {
      endian::write(m_out, value, m_endian);
   }

   template <typename value_type> void write(value_type value)
   {
      endian::write(m_out, value, m_endian);
   }
};

} // endian
} // utils
} // polar

#endif // POLARPHP_UTILS_ENDIAN_STREAM_H
