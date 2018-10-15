// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/07/03.

#include "polarphp/utils/Compression.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/ErrorHandling.h"

#if POLAR_ENABLE_ZLIB == 1 && HAVE_ZLIB_H
#include <zlib.h>
#endif

namespace polar {
namespace utils {
namespace zlib {

#if POLAR_ENABLE_ZLIB == 1 && HAVE_LIBZ

namespace {

Error create_error(StringRef error)
{
   return make_error<StringError>(error, inconvertible_error_code());
}

int encode_zlib_compression_level(zlib::CompressionLevel level)
{
   switch (level) {
   case zlib::NoCompression: return 0;
   case zlib::BestSpeedCompression: return 1;
   case zlib::DefaultCompression: return Z_DEFAULT_COMPRESSION;
   case zlib::BestSizeCompression: return 9;
   }
   polar_unreachable("Invalid zlib::CompressionLevel!");
}

StringRef convert_zlib_code_to_string(int code)
{
   switch (code) {
   case Z_MEM_ERROR:
      return "zlib error: Z_MEM_ERROR";
   case Z_BUF_ERROR:
      return "zlib error: Z_BUF_ERROR";
   case Z_STREAM_ERROR:
      return "zlib error: Z_STREAM_ERROR";
   case Z_DATA_ERROR:
      return "zlib error: Z_DATA_ERROR";
   case Z_OK:
   default:
      polar_unreachable("unknown or unexpected zlib status code");
   }
}


} // anonymous namespace

bool is_available()
{
   return true;
}

Error compress(StringRef inputBuffer,
               SmallVectorImpl<char> &compressedBuffer,
               CompressionLevel level)
{
   unsigned long compressedSize = ::compressBound(inputBuffer.getSize());
   compressedBuffer.resize(compressedSize);
   int clevel = encode_zlib_compression_level(level);
   int res = ::compress2((Bytef *)compressedBuffer.getData(), &compressedSize,
                         (const Bytef *)inputBuffer.getData(), inputBuffer.getSize(),
                         clevel);
   // Tell MemorySanitizer that zlib output buffer is fully initialized.
   // This avoids a false report when running polarVM with uninstrumented ZLib.
   __msan_unpoison(compressedBuffer.getData(), compressedSize);
   compressedBuffer.resize(compressedSize);
   return res ? create_error(convert_zlib_code_to_string(res)) : Error::getSuccess();
}

Error uncompress(StringRef inputBuffer, char *uncompressedBuffer,
                 size_t &uncompressedSize)
{
   int res =
         ::uncompress((Bytef *)uncompressedBuffer, (uLongf *)&uncompressedSize,
                      (const Bytef *)inputBuffer.getData(), inputBuffer.getSize());
   // Tell MemorySanitizer that zlib output buffer is fully initialized.
   // This avoids a false report when running LLVM with uninstrumented ZLib.
   __msan_unpoison(uncompressedBuffer, uncompressedSize);
   return res ? create_error(convert_zlib_code_to_string(res)) : Error::getSuccess();
}

Error uncompress(StringRef inputBuffer,
                 SmallVectorImpl<char> &uncompressedBuffer,
                 size_t uncompressedSize)
{
   uncompressedBuffer.resize(uncompressedSize);
   Error error =
         uncompress(inputBuffer, uncompressedBuffer.getData(), uncompressedSize);
   uncompressedBuffer.resize(uncompressedSize);
   return error;
}

uint32_t crc32(StringRef buffer)
{
   return ::crc32(0, (const Bytef *)buffer.getData(), buffer.getSize());
}

#else
bool is_available()
{
   return false;
}

Error compress(StringRef inputBuffer,
               SmallVectorImpl<char> &compressedBuffer,
               CompressionLevel level)
{
   polar_unreachable("zlib::compress is unavailable");
}

Error uncompress(StringRef inputBuffer, char *uncompressedBuffer,
                       size_t &uncompressedSize)
{
   polar_unreachable("zlib::uncompress is unavailable");
}

Error uncompress(StringRef inputBuffer,
                       SmallVectorImpl<char> &uncompressedBuffer,
                       size_t uncompressedSize)
{
   polar_unreachable("zlib::uncompress is unavailable");
}

uint32_t crc32(StringRef buffer)
{
   polar_unreachable("zlib::crc32 is unavailable");
}
#endif
} // zlib
} // utils
} // polar
