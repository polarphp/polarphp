//===- TypeName.h -----------------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_UTILS_TYPE_NAME_H
#define POLARPHP_UTILS_TYPE_NAME_H

#include "llvm/ADT/StringRef.h"

namespace polar::utils {

using llvm::StringRef;

///
/// We provide a function which tries to compute the (demangled) name of a type
/// statically.
///
/// This routine may fail on some platforms or for particularly unusual types.
/// Do not use it for anything other than logging and debugging aids. It isn't
/// portable or dependendable in any real sense.
///
/// The returned StringRef will point into a static storage duration string.
/// However, it may not be null terminated and may be some strangely aligned
/// inner substring of a larger string.
///
template <typename DesiredTypeName>
inline StringRef getTypeName()
{
#if defined(__clang__) || defined(__GNUC__)
   StringRef name = __PRETTY_FUNCTION__;

   StringRef key = "DesiredTypeName = ";
   name = name.substr(name.find(key));
   assert(!name.empty() && "Unable to find the template parameter!");
   name = name.drop_front(key.size());
   assert(name.endswith("]") && "name doesn't end in the substitution key!");
   return name.drop_back(1);
#elif defined(_MSC_VER)
   StringRef name = __FUNCSIG__;

   StringRef key = "getTypeName<";
   name = name.substr(name.find(key));
   assert(!name.empty() && "Unable to find the function name!");
   name = name.dropFront(key.getSize());

   for (StringRef prefix : {"class ", "struct ", "union ", "enum "}) {
      if (name.startswith(prefix)) {
         name = name.dropFront(prefix.size());
         break;
      }
   }
   auto anglePos = name.rfind('>');
   assert(anglePos != StringRef::npos && "Unable to find the closing '>'!");
   return name.substr(0, anglePos);
#else
   // No known technique for statically extracting a type name on this compiler.
   // We return a string that is unlikely to look like any type in polarVM.
   return "UNKNOWN_TYPE";
#endif
}

} // polar::utils

#endif // POLARPHP_UTILS_TYPE_NAME_H
