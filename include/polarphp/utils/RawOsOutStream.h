// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/30.

#ifndef POLARPHP_UTILS_RAW_OS_OUT_STREAM_H
#define POLARPHP_UTILS_RAW_OS_OUT_STREAM_H

#include "polarphp/utils/RawOutStream.h"
#include <iosfwd>

namespace polar {
namespace utils {

/// raw_os_ostream - A RawOutStream that writes to an std::ostream.  This is a
/// simple adaptor class.  It does not check for output errors; clients should
/// use the underlying stream to detect errors.
class RawOsOutStream : public RawOutStream
{
   std::ostream &m_outStream;

   /// write_impl - See RawOutStream::write_impl.
   void writeImpl(const char *ptr, size_t size) override;

   /// current_pos - Return the current position within the stream, not
   /// counting the bytes currently in the buffer.
   uint64_t getCurrentPos() const override;

public:
   RawOsOutStream(std::ostream &outStream)
      : m_outStream(outStream)
   {}

   ~RawOsOutStream() override;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_RAW_OS_OUT_STREAM_H
