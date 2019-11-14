//===- SMLoc.h - Source location for use with diagnostics -------*- C++ -*-===//
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
// Created by polarboy on 2018/06/26.

//===----------------------------------------------------------------------===//
//
// This file declares the SMLocation class.  This class encapsulates a location in
// source code for use in diagnostics.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_SOURCE_LOCATION_H
#define POLARPHP_UTILS_SOURCE_LOCATION_H

#include <cassert>
#include <optional>

namespace polar::utils {

/// Represents a location in source code.
class SMLocation
{
public:
   SMLocation() = default;

   bool isValid() const
   {
      return m_ptr != nullptr;
   }

   bool operator==(const SMLocation &other) const
   {
      return other.m_ptr == m_ptr;
   }

   bool operator!=(const SMLocation &other) const
   {
      return other.m_ptr != m_ptr;
   }

   const char *getPointer() const
   {
      return m_ptr;
   }

   static SMLocation getFromPointer(const char *ptr)
   {
      SMLocation location;
      location.m_ptr = ptr;
      return location;
   }

private:
   const char *m_ptr = nullptr;
};

/// Represents a range in source code.
///
/// SMRange is implemented using a half-open range, as is the convention in C++.
/// In the string "abc", the range [1,3) represents the substring "bc", and the
/// range [2,2) represents an empty range between the characters "b" and "c".
class SMRange
{
public:
   SMRange() = default;
   SMRange(std::nullopt_t) {}

   SMRange(SMLocation startLoc, SMLocation endLoc)
      : start(startLoc),
        end(endLoc)
   {
      assert(start.isValid() == end.isValid() &&
             "start and end should either both be valid or both be invalid!");
   }

   bool isValid() const
   {
      return start.isValid();
   }

public:
   SMLocation start;
   SMLocation end;
};

} // polar::utils

#endif // POLARPHP_UTILS_SOURCE_LOCATION_H
