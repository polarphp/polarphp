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

#ifndef POLARPHP_UTILS_PRINTABLE_H
#define POLARPHP_UTILS_PRINTABLE_H

#include <functional>

namespace polar {
namespace utils {

class RawOutStream;

/// Simple wrapper around std::function<void(RawOutStream&)>.
/// This class is useful to construct print helpers for RawOutStream.
///
/// Example:
///     Printable PrintRegister(unsigned Register) {
///       return Printable([Register](RawOutStream &outStream) {
///         outStream << getRegisterName(Register);
///       }
///     }
///     ... outStream << PrintRegister(Register); ...
///
/// Implementation note: Ideally this would just be a typedef, but doing so
/// leads to operator << being ambiguous as function has matching constructors
/// in some STL versions. I have seen the problem on gcc 4.6 libstdc++ and
/// microsoft STL.
class Printable
{
public:
  std::function<void(RawOutStream &outStream)> m_print;
  Printable(std::function<void(RawOutStream &outStream)> print)
      : m_print(std::move(print))
  {}
};

inline RawOutStream &operator<<(RawOutStream &outStream, const Printable &printable)
{
  printable.m_print(outStream);
  return outStream;
}

} // utils
} // polar

#endif // POLARPHP_UTILS_PRINTABLE_H
