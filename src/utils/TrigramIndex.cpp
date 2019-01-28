// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/TrigramIndex.h"
#include "polarphp/basic/adt/SmallVector.h"

#include <set>
#include <string>
#include <unordered_map>

namespace polar {
namespace utils {

static const char sg_regexAdvancedMetachars[] = "()^$|+?[]\\{}";

static bool is_advanced_metachar(unsigned c)
{
   return strchr(sg_regexAdvancedMetachars, c) != nullptr;
}

void TrigramIndex::insert(std::string regex)
{
   if (m_defeated) {
      return;
   }
   std::set<unsigned> was;
   unsigned cnt = 0;
   unsigned trigram = 0;
   unsigned length = 0;
   bool escaped = false;
   for (unsigned c : regex) {
      if (!escaped) {
         // Regular expressions allow escaping symbols by preceding it with '\'.
         if (c == '\\') {
            escaped = true;
            continue;
         }
         if (is_advanced_metachar(c)) {
            // This is a more complicated regex than we can handle here.
            m_defeated = true;
            return;
         }
         if (c == '.' || c == '*') {
            trigram = 0;
            length = 0;
            continue;
         }
      }
      if (escaped && c >= '1' && c <= '9') {
         m_defeated = true;
         return;
      }
      // We have already handled escaping and can reset the flag.
      escaped = false;
      trigram = ((trigram << 8) + c) & 0xFFFFFF;
      length++;
      if (length < 3) {
         continue;
      }
      // We don't want the index to grow too much for the popular trigrams,
      // as they are weak signals. It's ok to still require them for the
      // rules we have already processed. It's just a small additional
      // computational cost.
      if (m_index[trigram].getSize() >= 4) {
         continue;
      }

      cnt++;
      if (!was.count(trigram)) {
         // Adding the current rule to the index.
         m_index[trigram].push_back(m_counts.size());
         was.insert(trigram);
      }
   }
   if (!cnt) {
      // This rule does not have remarkable trigrams to rely on.
      // We have to always call the full regex chain.
      m_defeated = true;
      return;
   }
   m_counts.push_back(cnt);
}

bool TrigramIndex::isDefinitelyOut(StringRef query) const
{
   if (m_defeated) {
      return false;
   }
   std::vector<unsigned> curCounts(m_counts.size());
   unsigned trigram = 0;
   for (size_t index = 0; index < query.size(); index++) {
      trigram = ((trigram << 8) + query[index]) & 0xFFFFFF;
      if (index < 2) {
         continue;
      }
      const auto &iter = m_index.find(trigram);
      if (iter == m_index.end()) {
         continue;
      }

      for (size_t j : iter->second) {
         curCounts[j]++;
         // If we have reached a desired limit, we have to look at the query
         // more closely by running a full regex.
         if (curCounts[j] >= m_counts[j]) {
            return false;
         }
      }
   }
   return true;
}

} // utils
} // polar
