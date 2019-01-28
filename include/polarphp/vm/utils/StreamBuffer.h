// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#ifndef POLARPHP_VMAPI_UTILS_STREAM_BUFFER_H
#define POLARPHP_VMAPI_UTILS_STREAM_BUFFER_H

#include "polarphp/vm/ZendApi.h"

#include <streambuf>
#include <array>
#include <ostream>

namespace polar {
namespace vmapi {

class VMAPI_DECL_EXPORT StreamBuffer : public std::streambuf
{
public:
   StreamBuffer(int error);
   StreamBuffer(const StreamBuffer &buffer) = delete;
   StreamBuffer(StreamBuffer &&buffer) = delete;
   StreamBuffer &operator=(const StreamBuffer &buffer) = delete;
   StreamBuffer &operator=(StreamBuffer &&buffer) = delete;
   virtual ~StreamBuffer();
protected:
   virtual int overflow(int c = EOF) override;
   virtual int sync() override;

private:
   int m_error;
   std::array<char, 1024> m_buffer{};
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_STREAM_BUFFER_H
