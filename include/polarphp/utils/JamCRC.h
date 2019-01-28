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

#ifndef POLARPHP_UTILS_JAMCRC_H
#define POLARPHP_UTILS_JAMCRC_H

#include "polarphp/global/DataTypes.h"

namespace polar {

// forard declare class with namespace
namespace basic {
template <typename T>
class ArrayRef;
} // basic

namespace utils {

using polar::basic::ArrayRef;

class JamCRC
{
public:
   JamCRC(uint32_t init = 0xFFFFFFFFU)
      : m_crc(init)
   {}

   // \brief Update the m_crc calculation with data.
   void update(ArrayRef<char> data);

   uint32_t getCRC() const
   {
      return m_crc;
   }

private:
   uint32_t m_crc;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_JAMCRC_H
