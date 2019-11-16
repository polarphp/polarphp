// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/13.

#include "polarphp/kernel/LangOptions.h"

namespace polar::kernel {

bool LangOptions::
checkPlatformConditionSupported(PlatformConditionKind kind, StringRef value,
                                std::vector<StringRef> &suggestions)
{
   return true;
   llvm_unreachable("Unhandled enum value");
}

StringRef LangOptions::getPlatformConditionValue(PlatformConditionKind kind) const
{
   //   // Last one wins.
   //   for (auto &Opt : reversed(PlatformConditionValues)) {
   //      if (Opt.first == Kind)
   //         return Opt.second;
   //   }
   return StringRef();
}

bool LangOptions::checkPlatformCondition(PlatformConditionKind Kind, StringRef Value) const
{
   return false;
}

bool LangOptions::isCustomConditionalCompilationFlagSet(StringRef name) const
{
   return false;
}

std::pair<bool, bool> LangOptions::setTarget(Triple triple)
{
   // If you add anything to this list, change the default size of
   // PlatformConditionValues to not require an extra allocation
   // in the common case.
   return { false, false };
}

} // polar::kernel
