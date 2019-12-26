//===--- TopCollection.h - A size-limiting top-N collection -----*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/12/02.
//===----------------------------------------------------------------------===//
// This file defines the TopCollection class, a data structure which,
// given a size limit, keeps the best-scoring (i.e. lowest) N values
// added to it.
//
// The current implementation of this is only suited for small values
// of maxSize.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_TOPCOLLECTION_H
#define POLARPHP_BASIC_TOPCOLLECTION_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/SmallVector.h"

namespace polar {

template <class ScoreType, class T, unsigned InlineCapacity = 16>
class TopCollection
{
public:
   struct Entry
   {
      ScoreType score;
      T value;
   };

private:
   SmallVector<Entry, InlineCapacity> m_data;

   unsigned m_maxSize;
   unsigned m_endOfAccepted = 0;
public:
   explicit TopCollection(unsigned maxSize)
      : m_maxSize(maxSize)
   {
      assert(maxSize > 0 && "creating collection with a maximum size of 0?");
      m_data.reserve(maxSize);
   }

   // The invariants work fine with these.
   TopCollection(const TopCollection &other) = default;
   TopCollection &operator=(const TopCollection &other) = default;

   // We need to clear m_endOfAccepted to preserve the invariants here.
   TopCollection(TopCollection &&other)
      : m_data(std::move(other.m_data)),
        m_maxSize(other.m_maxSize),
        m_endOfAccepted(other.m_endOfAccepted)
   {
      other.m_endOfAccepted = 0;
   }

   TopCollection &operator=(TopCollection &&other)
   {
      m_data = std::move(other.m_data);
      m_maxSize = other.m_maxSize;
      m_endOfAccepted = other.m_endOfAccepted;
      other.m_endOfAccepted = 0;
      return *this;
   }

   ~TopCollection() {}

   bool empty() const
   {
      return m_endOfAccepted == 0;
   }

   size_t size() const
   {
      return m_endOfAccepted;
   }

   using iterator = const Entry *;
   iterator begin() const
   {
      return m_data.begin();
   }

   iterator end() const
   {
      return m_data.begin() + m_endOfAccepted;
   }

   /// Return a score beyond which scores are uninteresting.  Inserting
   /// a value with this score will never change the collection.
   ScoreType getMinUninterestingScore(ScoreType defaultBound) const
   {
      assert(m_endOfAccepted <= m_maxSize);
      assert(m_endOfAccepted <= m_data.size());

      // If we've accepted as many values as we can, then all scores up (and
      // including) that value are interesting.
      if (m_endOfAccepted == m_maxSize) {
         return m_data[m_endOfAccepted - 1].score + 1;
      }

      // Otherwise, if there are values in the collection that we've rejected,
      // any score up to that is still interesting.
      if (m_endOfAccepted != m_data.size()) {
         return m_data[m_endOfAccepted].score;
      }
      // Otherwise, use the default bound.
      return defaultBound;
   }

   /// Try to add a scored value to the collection.
   ///
   /// \return true if the insertion was successful
   bool insert(ScoreType score, T &&value)
   {
      assert(m_endOfAccepted <= m_maxSize);
      assert(m_endOfAccepted <= m_data.size());

      // Find the index of the last entry whose score is larger than 'score'.
      auto i = m_endOfAccepted;
      while (i != 0 && score < m_data[i - 1].score) {
         --i;
      }
      assert(0 <= i && i <= m_endOfAccepted);
      assert(i == 0 || score >= m_data[i - 1].score);

      // If we skipped anything, it's definitely safe to insert.
      if (i != m_endOfAccepted) {
         // fall through

         // If there's a tie with the previous thing, we might have to declare
         // an existing tier off-bounds.
      } else if (i != 0 && score == m_data[i - 1].score) {
         // Only if there isn't space to expand the tier.
         if (i == m_maxSize) {
            for (--i; i != 0; --i) {
               if (m_data[i - 1].score != m_data[i].score) {
                  break;
               }
            }
            m_endOfAccepted = i;
            return false;
         }

         // Otherwise, there's a non-trivial prefix that the new element has a
         // strictly higher score than.  We can still insert if there's space.
      } else {
         // Don't insert if there's no room.
         if (i == m_maxSize) {
            return false;
         }
         // Don't insert if we're at least as high as things we've previously
         // rejected.
         if (i != m_data.size() && score >= m_data[i].score) {
            return false;
         }
      }

      // We don't care about any of the actual values after m_endOfAccepted
      // *except* that we need to remember the minimum value following
      // m_endOfAccepted if that's less than m_maxSize so that we continue to
      // drop values with that score.
      //
      // Note that all of the values between m_endOfAccepted and m_maxSize
      // should have the same score, because otherwise there's a tier we
      // shouldn't have marked dead.

      // Just overwrite the next element instead of inserting if possible.
      if (i == m_endOfAccepted && i != m_data.size()) {
         m_data[i].score = score;
         m_data[i].value = std::move(value);
      } else {
         if (m_data.size() == m_maxSize) {
            m_data.pop_back();
            if (m_endOfAccepted == m_maxSize) {
               m_endOfAccepted--;
            }
         }
         m_data.insert(m_data.begin() + i, { score, std::move(value) });
      }

      m_endOfAccepted++;
      assert(m_endOfAccepted <= m_data.size());
      assert(m_endOfAccepted <= m_maxSize);
      return true;
   }

   /// Drop any values which a score more than the given value from the
   /// minimum score.
   template <class Range>
   void filterMaxScoreRange(Range difference) {
      if (m_endOfAccepted < 2) return;
      for (unsigned i = 1; i != m_endOfAccepted; ++i) {
         if (m_data[i].score > m_data[0].score + difference) {
            m_endOfAccepted = i;
            return;
         }
      }
   }
};

} // polar

#endif // POLARPHP_BASIC_TOPCOLLECTION_H

