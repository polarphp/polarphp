// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/06.

#ifndef POLARPHP_UTILS_CFG_UPDATE_H
#define POLARPHP_UTILS_CFG_UPDATE_H

#include "polarphp/basic/adt/ApInt.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace cfg {

using polar::basic::PointerIntPair;
using polar::debug_stream;
using polar::utils::RawOutStream;
using polar::basic::ArrayRef;
using polar::basic::SmallDenseMap;
using polar::basic::SmallVectorImpl;

enum class UpdateKind : unsigned char
{
   Insert,
   Delete
};

template <typename NodePtr> class Update
{
   using NodeKindPair = PointerIntPair<NodePtr, 1, UpdateKind>;
   NodePtr m_from;
   NodeKindPair m_toAndKind;

public:
   Update(UpdateKind kind, NodePtr from, NodePtr to)
      : m_from(from),
        m_toAndKind(to, kind)
   {}

   UpdateKind getKind() const
   {
      return m_toAndKind.getInt();
   }

   NodePtr getFrom() const
   {
      return m_from;
   }

   NodePtr getTo() const
   {
      return m_toAndKind.getPointer();
   }

   bool operator==(const Update &other) const
   {
      return m_from == other.m_from && m_toAndKind == other.m_toAndKind;
   }

   void print(RawOutStream &outStream) const
   {
      outStream << (getKind() == UpdateKind::Insert ? "Insert " : "Delete ");
      getFrom()->printAsOperand(outStream, false);
      outStream << " -> ";
      getTo()->printAsOperand(outStream, false);
   }

#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)
   POLAR_DUMP_METHOD void dump() const
   {
      print(debug_stream());
   }
#endif
};

// legalize_updates function simplifies updates assuming a graph structure.
// This function serves double purpose:
// a) It removes redundant updates, which makes it easier to reverse-apply
//    them when traversing CFG.
// b) It optimizes away updates that cancel each other out, as the end result
//    is the same.
template <typename NodePtr>
void legalize_updates(ArrayRef<Update<NodePtr>> allUpdates,
                     SmallVectorImpl<Update<NodePtr>> &result,
                     bool inverseGraph)
{
   // Count the total number of inserions of each edge.
   // Each insertion adds 1 and deletion subtracts 1. The end number should be
   // one of {-1 (deletion), 0 (NOP), +1 (insertion)}. Otherwise, the sequence
   // of updates contains multiple updates of the same kind and we assert for
   // that case.
   SmallDenseMap<std::pair<NodePtr, NodePtr>, int, 4> operations;
   operations.reserve(allUpdates.size());

   for (const auto &U : allUpdates) {
      NodePtr from = U.getFrom();
      NodePtr to = U.getTo();
      if (inverseGraph)
         std::swap(from, to); // Reverse edge for postdominators.

      operations[std::make_pair(from, to)] += (U.getKind() == UpdateKind::Insert ? 1 : -1);
   }

   result.clear();
   result.reserve(operations.size());
   for (auto &op : operations) {
      const int numInsertions = op.second;
      assert(std::abs(numInsertions) <= 1 && "Unbalanced operations!");
      if (numInsertions == 0) {
         continue;
      }
      const UpdateKind UK =
            numInsertions > 0 ? UpdateKind::Insert : UpdateKind::Delete;
      result.push_back({UK, op.first.first, op.first.second});
   }

   // Make the order consistent by not relying on pointer values within the
   // set. Reuse the old operations map.
   // In the future, we should sort by something else to minimize the amount
   // of work needed to perform the series of updates.
   for (size_t i = 0, e = allUpdates.size(); i != e; ++i) {
      const auto &U = allUpdates[i];
      if (!inverseGraph) {
         operations[std::make_pair(U.getFrom(), U.getTo())] = int(i);
      } else {
          operations[std::make_pair(U.getTo(), U.getFrom())] = int(i);
      }
   }

   polar::basic::sort(result,
              [&operations](const Update<NodePtr> &lhs, const Update<NodePtr> &rhs) {
      return operations[{lhs.getFrom(), lhs.getTo()}] >
            operations[{rhs.getFrom(), rhs.getTo()}];
});
}

} // cfg
} // polar

#endif // POLARPHP_UTILS_CFG_UPDATE_H
