// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_GLOB_PATTERN_H
#define POLARPHP_UTILS_GLOB_PATTERN_H

#include "polarphp/basic/adt/BitVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Error.h"
#include <vector>
#include <optional>

// This class represents a glob pattern. Supported metacharacters
// are "*", "?", "[<chars>]" and "[^<chars>]".

namespace polar {

namespace basic {
class BitVector;
template <typename T>
class ArrayRef;
} // basic

namespace utils {

using polar::basic::BitVector;
using polar::basic::ArrayRef;
using polar::basic::StringRef;

class GlobPattern
{
public:
   static Expected<GlobPattern> create(StringRef pattern);
   bool match(StringRef str) const;

private:
   bool matchOne(ArrayRef<BitVector> pattern, StringRef str) const;

   // Parsed glob pattern.
   std::vector<BitVector> m_tokens;

   // The following members are for optimization.
   std::optional<StringRef> m_exact;
   std::optional<StringRef> m_prefix;
   std::optional<StringRef> m_suffix;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_GLOB_PATTERN_H
