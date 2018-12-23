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

#ifndef POLARPHP_UTILS_RAW_SHA1_OUT_STREAM_H
#define POLARPHP_UTILS_RAW_SHA1_OUT_STREAM_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/Sha1.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

/// A RawOutStream that hash the content using the getSha1 algorithm.
class RawSha1OutStream : public RawOutStream
{
   Sha1 m_state;

   /// See RawOutStream::writeImpl.
   void writeImpl(const char *ptr, size_t size) override
   {
      m_state.update(ArrayRef<uint8_t>((const uint8_t *)ptr, size));
   }

public:
   /// Return the current SHA1 hash for the content of the stream
   StringRef getSha1()
   {
      flush();
      return m_state.result();
   }

   /// Reset the internal state to start over from scratch.
   void resetHash()
   {
      m_state.init();
   }

   uint64_t getCurrentPos() const override
   {
      return 0;
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_RAW_SHA1_OUT_STREAM_H
