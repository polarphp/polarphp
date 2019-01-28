// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/24.

#include "polarphp/basic/adt/IntervalMap.h"

namespace polar {
namespace basic {
namespace intervalmapimpl {

void Path::replaceRoot(void *root, unsigned size, IdxPair offsets)
{
   assert(!m_path.empty() && "Can't replace missing root");
   m_path.getFront() = Entry(root, size, offsets.first);
   m_path.insert(m_path.begin() + 1, Entry(subtree(0), offsets.second));
}

NodeRef Path::getLeftSibling(unsigned level) const
{
   // The root has no siblings.
   if (level == 0) {
      return NodeRef();
   }
   // Go up the tree until we can go left.
   unsigned l = level - 1;
   while (l && m_path[l].m_offset == 0) {
      --l;
   }
   // We can't go left.
   if (m_path[l].m_offset == 0) {
      return NodeRef();
   }
   // NR is the subtree containing our left sibling.
   NodeRef nodeRef = m_path[l].subtree(m_path[l].m_offset - 1);
   // Keep right all the way down.
   for (++l; l != level; ++l) {
      nodeRef = nodeRef.subtree(nodeRef.size() - 1);
   }
   return nodeRef;
}

void Path::moveLeft(unsigned level)
{
   assert(level != 0 && "Cannot move the root node");

   // Go up the tree until we can go left.
   unsigned l = 0;
   if (valid()) {
      l = level - 1;
      while (m_path[l].m_offset == 0) {
         assert(l != 0 && "Cannot move beyond begin()");
         --l;
      }
   } else if (getHeight() < level) {
      // end() may have created a height=0 path.
      m_path.resize(level + 1, Entry(nullptr, 0, 0));
   }

   // NR is the subtree containing our left sibling.
   --m_path[l].m_offset;
   NodeRef nodeRef = subtree(l);

   // Get the rightmost node in the subtree.
   for (++l; l != level; ++l) {
      m_path[l] = Entry(nodeRef, nodeRef.size() - 1);
      nodeRef = nodeRef.subtree(nodeRef.size() - 1);
   }
   m_path[l] = Entry(nodeRef, nodeRef.size() - 1);
}

NodeRef Path::getRightSibling(unsigned level) const
{
   // The root has no siblings.
   if (level == 0) {
      return NodeRef();
   }
   // Go up the tree until we can go right.
   unsigned l = level - 1;
   while (l && atLastEntry(l)) {
      --l;
   }
   // We can't go right.
   if (atLastEntry(l)) {
      return NodeRef();
   }
   // NR is the subtree containing our right sibling.
   NodeRef nodeRef = m_path[l].subtree(m_path[l].m_offset + 1);

   // Keep left all the way down.
   for (++l; l != level; ++l) {
      nodeRef = nodeRef.subtree(0);
   }
   return nodeRef;
}

void Path::moveRight(unsigned level)
{
   assert(level != 0 && "Cannot move the root node");

   // Go up the tree until we can go right.
   unsigned l = level - 1;
   while (l && atLastEntry(l)) {
      --l;
   }
   // NR is the subtree containing our right sibling. If we hit end(), we have
   // offset(0) == node(0).size().
   if (++m_path[l].m_offset == m_path[l].m_size) {
      return;
   }

   NodeRef nodeRef = subtree(l);

   for (++l; l != level; ++l) {
      m_path[l] = Entry(nodeRef, 0);
      nodeRef = nodeRef.subtree(0);
   }
   m_path[l] = Entry(nodeRef, 0);
}


IdxPair distribute(unsigned nodes, unsigned elements, unsigned capacity,
                   const unsigned *curSize, unsigned newSize[],
                   unsigned position, bool grow)
{
   assert(elements + grow <= nodes * capacity && "Not enough room for elements");
   assert(position <= elements && "Invalid position");
   if (!nodes) {
      return IdxPair();
   }
   // Trivial algorithm: left-leaning even distribution.
   const unsigned perNode = (elements + grow) / nodes;
   const unsigned extra = (elements + grow) % nodes;
   IdxPair posPair = IdxPair(nodes, 0);
   unsigned sum = 0;
   for (unsigned n = 0; n != nodes; ++n) {
      sum += newSize[n] = perNode + (n < extra);
      if (posPair.first == nodes && sum > position) {
         posPair = IdxPair(n, position - (sum - newSize[n]));
      }
   }
   assert(sum == elements + grow && "Bad distribution sum");
   // Subtract the Grow element that was added.
   if (grow) {
      assert(posPair.first < nodes && "Bad algebra");
      assert(newSize[posPair.first] && "Too few elements to need Grow");
      --newSize[posPair.first];
   }

#ifndef NDEBUG
   sum = 0;
   for (unsigned n = 0; n != nodes; ++n) {
      assert(newSize[n] <= capacity && "Overallocated node");
      sum += newSize[n];
   }
   assert(sum == elements && "Bad distribution sum");
#endif
   return posPair;
}

} // intervalmapimpl
} // basic
} // polar
