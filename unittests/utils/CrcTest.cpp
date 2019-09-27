//===- llvm/unittest/Support/CRCTest.cpp - CRC tests ----------------------===//
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
// Created by polarboy on 2019/09/27.
//===----------------------------------------------------------------------===//
//
// This file implements unit tests for CRC calculation functions.
//
//===----------------------------------------------------------------------===//


#include "polarphp/utils/Crc.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

namespace {

TEST(CRCTest, testCRC32) {
   EXPECT_EQ(0x414FA339U,
             polar::utils::crc32(
                0, StringRef("The quick brown fox jumps over the lazy dog")));
   // CRC-32/ISO-HDLC test vector
   // http://reveng.sourceforge.net/crc-catalogue/17plus.htm#crc.cat.crc-32c
   EXPECT_EQ(0xCBF43926U, polar::utils::crc32(0, StringRef("123456789")));
}

} // end anonymous namespace
