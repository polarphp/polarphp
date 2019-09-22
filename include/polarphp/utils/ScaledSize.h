//===- scalableSize.h - scalable vector size info ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/18.

#ifndef POLARPHP_UTILS_SCALED_SIZE_H
#define POLARPHP_UTILS_SCALED_SIZE_H

namespace polar::utils {

class ElementCount
{
public:
   unsigned min;  // minimum number of vector elements.
   bool scalable; // If true, NumElements is a multiple of 'min' determined
   // at runtime rather than compile time.

   ElementCount(unsigned min, bool scalable)
      : min(min), scalable(scalable)
   {}

   ElementCount operator*(unsigned rhs)
   {
      return { min * rhs, scalable };
   }

   ElementCount operator/(unsigned rhs)
   {
      return { min / rhs, scalable };
   }

   bool operator==(const ElementCount& rhs) const
   {
      return min == rhs.min && scalable == rhs.scalable;
   }
};

} // polar::utils

#endif // POLARPHP_UTILS_SCALED_SIZE_H
