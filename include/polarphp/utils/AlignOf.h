// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/08.

#ifndef POLARPHP_UTILS_ALIGN_OF_H
#define POLARPHP_UTILS_ALIGN_OF_H

#include <cstddef>

namespace polar {
namespace utils {

/// \struct AlignedCharArray
/// Helper for building an aligned character array type.
///
/// This template is used to explicitly build up a collection of aligned
/// character array types. We have to build these up using a macro and explicit
/// specialization to cope with MSVC (at least till 2015) where only an
/// integer literal can be used to specify an alignment constraint. Once built
/// up here, we can then begin to indirect between these using normal C++
/// template parameters.

// MSVC requires special handling here.
#ifndef _MSC_VER

template<std::size_t Alignment, std::size_t Size>
struct AlignedCharArray
{
   alignas(Alignment) char m_buffer[Size];
};

#else // _MSC_VER
/// Create a type with an aligned char buffer.
template<std::size_t Alignment, std::size_t Size>
struct AlignedCharArray;

// We provide special variations of this template for the most common
// alignments because __declspec(align(...)) doesn't actually work when it is
// a member of a by-value function argument in MSVC, even if the alignment
// request is something reasonably like 8-byte or 16-byte. Note that we can't
// even include the declspec with the union that forces the alignment because
// MSVC warns on the existence of the declspec despite the union member forcing
// proper alignment.

template<std::size_t Size>
struct AlignedCharArray<1, Size>
{
   union {
      char m_aligned;
      char m_buffer[Size];
   };
};

template<std::size_t Size>
struct AlignedCharArray<2, Size>
{
   union {
      short m_aligned;
      char m_buffer[Size];
   };
};

template<std::size_t Size>
struct AlignedCharArray<4, Size>
{
   union {
      int m_aligned;
      char m_buffer[Size];
   };
};

template<std::size_t Size>
struct AlignedCharArray<8, Size>
{
   union {
      double m_aligned;
      char m_buffer[Size];
   };
};


// The rest of these are provided with a __declspec(align(...)) and we simply
// can't pass them by-value as function arguments on MSVC.

#define POLAR_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(x) \
   template<std::size_t Size> \
   struct AlignedCharArray<x, Size> { \
   __declspec(align(x)) char m_buffer[Size]; \
};

POLAR_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(16)
POLAR_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(32)
POLAR_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(64)
POLAR_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(128)

#undef POLAR_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT

#endif // _MSC_VER

namespace internal
{
   template <typename T1,
             typename T2 = char, typename T3 = char, typename T4 = char,
             typename T5 = char, typename T6 = char, typename T7 = char,
             typename T8 = char, typename T9 = char, typename T10 = char>
   class AlignerImpl
   {
      T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10;
      AlignerImpl() = delete;
   };

   template <typename T1,
             typename T2 = char, typename T3 = char, typename T4 = char,
             typename T5 = char, typename T6 = char, typename T7 = char,
             typename T8 = char, typename T9 = char, typename T10 = char>
   union SizerImpl
   {
      char arr1[sizeof(T1)], arr2[sizeof(T2)], arr3[sizeof(T3)], arr4[sizeof(T4)],
            arr5[sizeof(T5)], arr6[sizeof(T6)], arr7[sizeof(T7)], arr8[sizeof(T8)],
            arr9[sizeof(T9)], arr10[sizeof(T10)];
   };

} // internal

/// This union template exposes a suitably aligned and sized character
/// array member which can hold elements of any of up to ten types.
///
/// These types may be arrays, structs, or any other types. The goal is to
/// expose a char array buffer member which can be used as suitable storage for
/// a placement new of any of these types. Support for more than ten types can
/// be added at the cost of more boilersplate.
template <typename T1,
          typename T2 = char, typename T3 = char, typename T4 = char,
          typename T5 = char, typename T6 = char, typename T7 = char,
          typename T8 = char, typename T9 = char, typename T10 = char>
struct AlignedCharArrayUnion : polar::utils::AlignedCharArray<
      alignof(internal::AlignerImpl<T1, T2, T3, T4, T5,
              T6, T7, T8, T9, T10>),
sizeof(internal::SizerImpl<T1, T2, T3, T4, T5,
       T6, T7, T8, T9, T10>)>
{};

} // utils
} // polar

#endif // POLARPHP_UTILS_ALIGN_OF_H
