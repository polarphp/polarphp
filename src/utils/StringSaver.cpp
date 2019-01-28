// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of pola project authors
//
// Created by polarboy on 2018/06/11.

#include "polarphp/utils/StringSaver.h"

namespace polar {
namespace utils {

StringRef StringSaver::save(StringRef str)
{
   char *ptr = m_alloc.allocate<char>(str.getSize() + 1);
   memcpy(ptr, str.getData(), str.getSize());
   if (!str.empty()) {
      memcpy(ptr, str.getData(), str.getSize());
   }
   ptr[str.getSize()] = '\0';
   return StringRef(ptr, str.getSize());
}

StringRef UniqueStringSaver::save(StringRef str)
{
   auto r = m_unique.insert(str);
   if (r.second) {                // cache miss, need to actually save the string
      *r.first = m_strings.save(str); // safe replacement with equal value
   }
   return *r.first;
}

} // utils
} // polar
