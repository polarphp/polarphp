// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/07.

//===----------------------------------------------------------------------===//
//
// This implements support adapting RawOutStream to std::ostream.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/RawOsOutStream.h"
#include <ostream>

namespace polar {
namespace utils {

RawOsOutStream::~RawOsOutStream()
{
   flush();
}

void RawOsOutStream::writeImpl(const char *ptr, size_t size)
{
   m_outStream.write(ptr, size);
}

uint64_t RawOsOutStream::getCurrentPos() const
{
   return m_outStream.tellp();
}

} // utils
} // polar
