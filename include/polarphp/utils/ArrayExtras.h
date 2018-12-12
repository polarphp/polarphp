// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/11.

#ifndef POLARPHP_UTILS_ARRAY_EXTRAS_H
#define POLARPHP_UTILS_ARRAY_EXTRAS_H

#include <vector>

namespace polar {
namespace utils {

template <typename T>
std::vector<T> vector_slice(const std::vector<T> &vector, std::size_t start, std::size_t size)
{
   assert(start + size <= vector.size() && "Invalid specifier");
   return std::vector<T>(vector.begin() + start, vector.begin() + start + size);
}

template <typename T>
std::vector<T> vector_slice(const std::vector<T> &vector, std::size_t size)
{
   return vector_slice(vector, size, vector.size() - size);
}

/// Drop the first \p N elements of the array.
template <typename T>
std::vector<T> vector_drop_front(const std::vector<T> &vector, std::size_t size = 1)
{
   assert(vector.size() >= size && "Dropping more elements than exist");
   return vector_slice(vector, size, vector.size() - size);
}

/// Drop the last \p N elements of the array.
template <typename T>
std::vector<T> vector_drop_back(const std::vector<T> &vector, std::size_t size = 1)
{
   assert(vector.size() >= size && "Dropping more elements than exist");
   return vector_slice(vector, 0, vector.size() - size);
}

template <typename T>
std::vector<T> vector_take_front(const std::vector<T> &vector, std::size_t size = 1)
{
   if (size > vector.size()) {
      return vector;
   }
   return vector_drop_back(vector, vector.size() - size);
}

template <typename T>
std::vector<T> vector_take_back(const std::vector<T> &vector, std::size_t size = 1)
{
   if (size > vector.size()) {
      return vector;
   }
   return vector_drop_front(vector, vector.size() - size);
}

} // utils
} // polar

#endif // POLARPHP_UTILS_ARRAY_EXTRAS_H
