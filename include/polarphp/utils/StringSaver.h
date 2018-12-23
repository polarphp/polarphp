// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/11.

#ifndef POLARPHP_UTILS_STRING_SAVER_H
#define POLARPHP_UTILS_STRING_SAVER_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/DenseSet.h"
#include "polarphp/utils/Allocator.h"

namespace polar {
namespace utils {

using polar::basic::StringRef;
using polar::basic::Twine;

/// Saves strings in the inheritor's stable storage and returns a
/// StringRef with a stable character pointer.
class StringSaver final
{
   BumpPtrAllocator &m_alloc;
public:
   StringSaver(BumpPtrAllocator &alloc) : m_alloc(alloc)
   {}

   StringRef save(const char *str)
   {
      return save(StringRef(str));
   }

   StringRef save(StringRef str);
   StringRef save(const Twine &str)
   {
      return save(StringRef(str.getStr()));
   }

   StringRef save(const std::string &str)
   {
      return save(StringRef(str));
   }
};

/// Saves strings in the provided stable storage and returns a StringRef with a
/// stable character pointer. Saving the same string yields the same StringRef.
///
/// Compared to StringSaver, it does more work but avoids saving the same string
/// multiple times.
///
/// Compared to StringPool, it performs fewer allocations but doesn't support
/// refcounting/deletion.
class UniqueStringSaver final
{
   StringSaver m_strings;
   polar::basic::DenseSet<StringRef> m_unique;

public:
   UniqueStringSaver(BumpPtrAllocator &alloc)
      : m_strings(alloc)
   {}

   // All returned strings are null-terminated: *save(S).end() == 0.
   StringRef save(const char *str)
   {
      return save(StringRef(str));
   }

   StringRef save(StringRef str);
   StringRef save(const Twine &str)
   {
      return save(StringRef(str.getStr()));
   }

   StringRef save(const std::string &str)
   {
      return save(StringRef(str));
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_STRING_SAVER_H
