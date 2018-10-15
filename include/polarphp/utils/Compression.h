// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/06/29.

#ifndef POLARPHP_UTILS_COMPRESSION_H
#define POLARPHP_UTILS_COMPRESSION_H

#include "polarphp/global/DataTypes.h"

namespace polar {

// forward declare class with namespace
namespace basic {
class StringRef;
template <typename T> class SmallVectorImpl;
} // basic

namespace utils {


class Error;

using polar::basic::StringRef;
using polar::basic::SmallVectorImpl;

namespace zlib {

enum CompressionLevel
{
   NoCompression,
   DefaultCompression,
   BestSpeedCompression,
   BestSizeCompression
};

bool is_available();

Error compress(StringRef inputBuffer, SmallVectorImpl<char> &compressedBuffer,
               CompressionLevel level = DefaultCompression);

Error uncompress(StringRef inputBuffer, char *uncompressedBuffer,
                 size_t &uncompressedSize);

Error uncompress(StringRef inputBuffer,
                 SmallVectorImpl<char> &uncompressedBuffer,
                 size_t uncompressedSize);

uint32_t crc32(StringRef buffer);

}  // End of namespace zlib

} // utils
} // polar

#endif // POLARPHP_UTILS_COMPRESSION_H
