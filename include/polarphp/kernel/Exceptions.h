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

#include <exception>

namespace polar {

class CompileException : public std::exception
{
public:
   virtual const char* what() const noexcept;
};

class ParseException : public CompileException
{
public:
   virtual const char* what() const noexcept;
};

} // polar

#endif // POLARPHP_KERNEL_EXCEPTIONS_H
