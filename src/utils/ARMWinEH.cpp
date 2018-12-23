// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/18.

#include "polarphp/utils/ARMWinEH.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace arm {
namespace wineh {

std::pair<uint16_t, uint32_t> saved_register_mask(const RuntimeFunction &rf)
{
   uint8_t numRegisters = rf.reg();
   uint8_t registersVFP = rf.r();
   uint8_t linkRegister = rf.l();
   uint8_t chainedFrame = rf.c();

   uint16_t gprMask = (chainedFrame << 11) | (linkRegister << 14);
   uint32_t vfpMask = 0;

   if (registersVFP) {
      vfpMask |= (((1 << ((numRegisters + 1) % 8)) - 1) << 8);
   } else {
       gprMask |= (((1 << (numRegisters + 1)) - 1) << 4);
   }
   if (prologue_folding(rf)) {
      gprMask |= (((1 << (numRegisters + 1)) - 1) << (~rf.getStackAdjust() & 0x3));
   }
   return std::make_pair(gprMask, vfpMask);
}

} // wineh
} // arm
} // polar
