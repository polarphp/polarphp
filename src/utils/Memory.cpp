//===- Memory.cpp - Memory Handling Support ---------------------*- C++ -*-===//
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
// Created by polarboy on 2019/09/23.

#include "polarphp/utils/Memory.h"

#ifndef NDEBUG
#include "polarphp/utils/RawOutStream.h"
#endif // ifndef NDEBUG

namespace polar::sys {

RawOutStream &operator<<(RawOutStream &outStream, const Memory::ProtectionFlags &flags)
{
   assert((flags & ~(Memory::MF_READ | Memory::MF_WRITE | Memory::MF_EXEC)) == 0 &&
          "Unrecognized flags");

   return outStream << (flags & Memory::MF_READ ? 'R' : '-')
                    << (flags & Memory::MF_WRITE ? 'W' : '-')
                    << (flags & Memory::MF_EXEC ? 'X' : '-');
}

RawOutStream &operator<<(RawOutStream &outStream, const MemoryBlock &memoryBlock)
{
   return outStream << "[ " << memoryBlock.getBase() << " .. "
                    << reinterpret_cast<void *>(reinterpret_cast<char *>(memoryBlock.getBase()) + memoryBlock.getAllocatedSize()) << " ] ("
                    << memoryBlock.getAllocatedSize() << " bytes)";
}

} // polar::sys
