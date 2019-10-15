//===-- llvm/Support/JamCRC.h - Cyclic Redundancy Check ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_JAMCRC_H
#define POLARPHP_UTILS_JAMCRC_H

#include "polarphp/global/DataTypes.h"

namespace polar::basic {
// forard declare class with namespace
template <typename T>
class ArrayRef;
} // polar::basic

namespace polar::utils {

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

} // polar::utils

#endif // POLARPHP_UTILS_JAMCRC_H
