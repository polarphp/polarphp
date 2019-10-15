// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/07/05.

#ifndef POLARPHP_KERNEL_EXCEPTIONS_H
#define POLARPHP_KERNEL_EXCEPTIONS_H

#include <stdexcept>

namespace polar {

class CompileException : public std::runtime_error
{
public:
   CompileException(const char *msg, int code)
      : std::runtime_error(msg),
        m_code(code)
   {}
   virtual const char* what() const noexcept;
   int getCode() const
   {
      return m_code;
   }
private:
   int m_code;
};

class ParseException : public CompileException
{
public:
   using CompileException::CompileException;
};

} // polar

#endif // POLARPHP_KERNEL_EXCEPTIONS_H
