// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/31.

#ifndef POLARPHP_BASIC_ADT_UNIQUE_VECTOR_H
#define POLARPHP_BASIC_ADT_UNIQUE_VECTOR_H

#include <cassert>
#include <cstddef>
#include <map>
#include <vector>

namespace polar {
namespace basic {

//===----------------------------------------------------------------------===//
/// UniqueVector - This class produces a sequential ID number (base 1) for each
/// unique entry that is added.  T is the type of entries in the vector. This
/// class should have an implementation of operator== and of operator<.
/// Entries can be fetched using operator[] with the entry ID.
template<typename T>
class UniqueVector
{
public:
   using VectorType = typename std::vector<T>;
   using iterator = typename VectorType::iterator;
   using const_iterator = typename VectorType::const_iterator;

private:
   // Map - Used to handle the correspondence of entry to ID.
   std::map<T, unsigned> m_map;

   // Vector - ID ordered vector of entries. Entries can be indexed by ID - 1.
   VectorType m_vector;

public:
   /// insert - Append entry to the vector if it doesn't already exist.  Returns
   /// the entry's index + 1 to be used as a unique ID.
   unsigned insert(const T &entry)
   {
      // Check if the entry is already in the map.
      unsigned &value = m_map[entry];

      // See if entry exists, if so return prior ID.
      if (value) {
         return value;
      }

      // Compute ID for entry.
      value = static_cast<unsigned>(m_vector.size()) + 1;

      // Insert in vector.
      m_vector.push_back(entry);
      return value;
   }

   /// idFor - return the ID for an existing entry.  Returns 0 if the entry is
   /// not found.
   unsigned idFor(const T &entry) const
   {
      // Search for entry in the map.
      typename std::map<T, unsigned>::const_iterator miter = m_map.find(entry);

      // See if entry exists, if so return ID.
      if (miter != m_map.end()) {
         return miter->second;
      }
      // No luck.
      return 0;
   }

   /// operator[] - Returns a reference to the entry with the specified ID.
   const T &operator[](unsigned id) const
   {
      assert(id - 1 < getSize() && "ID is 0 or out of range!");
      return m_vector[id - 1];
   }

   /// \brief Return an iterator to the start of the vector.
   iterator begin()
   {
      return m_vector.begin();
   }

   /// \brief Return an iterator to the start of the vector.
   const_iterator begin() const
   {
      return m_vector.begin();
   }

   /// \brief Return an iterator to the end of the vector.
   iterator end()
   {
      return m_vector.end();
   }

   /// \brief Return an iterator to the end of the vector.
   const_iterator end() const
   {
      return m_vector.end();
   }

   /// size - Returns the number of entries in the vector.
   size_t getSize() const
   {
      return m_vector.size();
   }

   /// empty - Returns true if the vector is empty.
   bool empty() const
   {
      return m_vector.empty();
   }

   /// reset - Clears all the entries.
   void reset()
   {
      m_map.clear();
      m_vector.resize(0, 0);
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_UNIQUE_VECTOR_H
