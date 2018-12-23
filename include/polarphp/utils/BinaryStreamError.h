// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/28.

#ifndef POLARPHP_UTILS_BINARY_STREAM_ERROR_H
#define POLARPHP_UTILS_BINARY_STREAM_ERROR_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Error.h"
#include <string>

namespace polar {
namespace utils {

enum class StreamErrorCode
{
   unspecified,
   stream_too_short,
   invalid_array_size,
   invalid_offset,
   filesystem_error
};

/// Base class for errors originating when parsing raw PDB files
class BinaryStreamError : public ErrorInfo<BinaryStreamError>
{
public:
   static char sm_id;
   explicit BinaryStreamError(StreamErrorCode errorCode);
   explicit BinaryStreamError(StringRef context);
   BinaryStreamError(StreamErrorCode errorCode, StringRef context);

   void log(RawOutStream &outstream) const override;
   std::error_code convertToErrorCode() const override;
   StringRef getErrorMessage() const;
   StreamErrorCode getErrorCode() const
   {
      return m_code;
   }

private:
   std::string m_errorMsg;
   StreamErrorCode m_code;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_BINARY_STREAM_ERROR_H
