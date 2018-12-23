// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/27.

#ifndef POLARPHP_BASIC_ADT_HASING_H
#define POLARPHP_BASIC_ADT_HASING_H

#include "polarphp/global/ProcessorDetection.h"
#include "polarphp/global/DataTypes.h"
#include "polarphp/utils/SwapByteOrder.h"
#include "polarphp/utils/TypeTraits.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <utility>

namespace polar {
namespace basic {

namespace utils = polar::utils;

/// An opaque object representing a hash code.
///
/// This object represents the result of hashing some entity. It is intended to
/// be used to implement hashtables or other hashing-based data structures.
/// While it wraps and exposes a numeric value, this value should not be
/// trusted to be stable or predictable across processes or executions.
///
/// In order to obtain the HashCode for an object 'x':
/// \code
///   using polar::basic::hash_value;
///   polar::basic::HashCode code = hash_value(x);
/// \endcode
class HashCode
{
   size_t value;

public:
   /// \brief Default construct a HashCode.
   /// Note that this leaves the value uninitialized.
   HashCode() = default;

   /// \brief Form a hash code directly from a numerical value.
   HashCode(size_t value) : value(value)
   {}

   /// \brief Convert the hash code to its numerical value for use.
   /*explicit*/ operator size_t() const { return value; }

   friend bool operator==(const HashCode &lhs, const HashCode &rhs)
   {
      return lhs.value == rhs.value;
   }

   friend bool operator!=(const HashCode &lhs, const HashCode &rhs)
   {
      return lhs.value != rhs.value;
   }

   /// \brief Allow a HashCode to be directly run through hash_value.
   friend size_t hash_value(const HashCode &code)
   {
      return code.value;
   }
};

/// \brief Compute a HashCode for any integer value.
///
/// Note that this function is intended to compute the same HashCode for
/// a particular value without regard to the pre-promotion type. This is in
/// contrast to hash_combine which may produce different HashCodes for
/// differing argument types even if they would implicit promote to a common
/// type without changing the value.
template <typename T>
typename std::enable_if<utils::is_integral_or_enum<T>::value, HashCode>::type
hash_value(T value);

/// \brief Compute a HashCode for a pointer's address.
///
/// N.B.: This hashes the *address*. Not the value and not the type.
template <typename T> HashCode hash_value(const T *ptr);

/// \brief Compute a HashCode for a pair of objects.
template <typename T, typename U>
HashCode hash_value(const std::pair<T, U> &arg);

/// \brief Compute a HashCode for a standard string.
template <typename T>
HashCode hash_value(const std::basic_string<T> &arg);


/// Override the execution seed with a fixed value.
///
/// This hashing library uses a per-execution seed designed to change on each
/// run with high probability in order to ensure that the hash codes are not
/// attackable and to ensure that output which is intended to be stable does
/// not rely on the particulars of the hash codes produced.
///
/// That said, there are use cases where it is important to be able to
/// reproduce *exactly* a specific behavior. To that end, we provide a function
/// which will forcibly set the seed to a fixed value. This must be done at the
/// start of the program, before any hashes are computed. Also, it cannot be
/// undone. This makes it thread-hostile and very hard to use outside of
/// immediately on start of a simple program designed for reproducible
/// behavior.
void set_fixed_execution_hash_seed(uint64_t fixedValue);


// All of the implementation details of actually computing the various hash
// code values are held within this namespace. These routines are included in
// the header file mainly to allow inlining and constant propagation.
namespace hashing {
namespace internal {

inline uint64_t fetch64(const char *p)
{
   uint64_t result;
   memcpy(&result, p, sizeof(result));
   if (POLAR_BYTE_ORDER == POLAR_BIG_ENDIAN)
   {
      utils::swap_byte_order(result);
   }
   return result;
}

inline uint32_t fetch32(const char *p)
{
   uint32_t result;
   memcpy(&result, p, sizeof(result));
   if (POLAR_BYTE_ORDER == POLAR_BIG_ENDIAN) {
      utils::swap_byte_order(result);
   }
   return result;
}

/// Some primes between 2^63 and 2^64 for various uses.
static const uint64_t k0 = 0xc3a5c85c97cb3127ULL;
static const uint64_t k1 = 0xb492b66fbe98f273ULL;
static const uint64_t k2 = 0x9ae16a3b2f90404fULL;
static const uint64_t k3 = 0xc949d7c7509e6557ULL;

/// \brief Bitwise right rotate.
/// Normally this will compile to a single instruction, especially if the
/// shift is a manifest constant.
inline uint64_t rotate(uint64_t val, size_t shift)
{
   // Avoid shifting by 64: doing so yields an undefined result.
   return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

inline uint64_t shift_mix(uint64_t val)
{
   return val ^ (val >> 47);
}

inline uint64_t hash_16_bytes(uint64_t low, uint64_t high)
{
   // Murmur-inspired hashing.
   const uint64_t kMul = 0x9ddfea08eb382d69ULL;
   uint64_t a = (low ^ high) * kMul;
   a ^= (a >> 47);
   uint64_t b = (high ^ a) * kMul;
   b ^= (b >> 47);
   b *= kMul;
   return b;
}

inline uint64_t hash_1to3_bytes(const char *s, size_t len, uint64_t seed)
{
   uint8_t a = s[0];
   uint8_t b = s[len >> 1];
   uint8_t c = s[len - 1];
   uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
   uint32_t z = len + (static_cast<uint32_t>(c) << 2);
   return shift_mix(y * k2 ^ z * k3 ^ seed) * k2;
}

inline uint64_t hash_4to8_bytes(const char *s, size_t len, uint64_t seed)
{
   uint64_t a = fetch32(s);
   return hash_16_bytes(len + (a << 3), seed ^ fetch32(s + len - 4));
}

inline uint64_t hash_9to16_bytes(const char *s, size_t len, uint64_t seed)
{
   uint64_t a = fetch64(s);
   uint64_t b = fetch64(s + len - 8);
   return hash_16_bytes(seed ^ a, rotate(b + len, len)) ^ b;
}

inline uint64_t hash_17to32_bytes(const char *s, size_t len, uint64_t seed)
{
   uint64_t a = fetch64(s) * k1;
   uint64_t b = fetch64(s + 8);
   uint64_t c = fetch64(s + len - 8) * k2;
   uint64_t d = fetch64(s + len - 16) * k0;
   return hash_16_bytes(rotate(a - b, 43) + rotate(c ^ seed, 30) + d,
                        a + rotate(b ^ k3, 20) - c + len + seed);
}

inline uint64_t hash_33to64_bytes(const char *s, size_t len, uint64_t seed)
{
   uint64_t z = fetch64(s + 24);
   uint64_t a = fetch64(s) + (len + fetch64(s + len - 16)) * k0;
   uint64_t b = rotate(a + z, 52);
   uint64_t c = rotate(a, 37);
   a += fetch64(s + 8);
   c += rotate(a, 7);
   a += fetch64(s + 16);
   uint64_t vf = a + z;
   uint64_t vs = b + rotate(a, 31) + c;
   a = fetch64(s + 16) + fetch64(s + len - 32);
   z = fetch64(s + len - 8);
   b = rotate(a + z, 52);
   c = rotate(a, 37);
   a += fetch64(s + len - 24);
   c += rotate(a, 7);
   a += fetch64(s + len - 16);
   uint64_t wf = a + z;
   uint64_t ws = b + rotate(a, 31) + c;
   uint64_t r = shift_mix((vf + ws) * k2 + (wf + vs) * k0);
   return shift_mix((seed ^ (r * k0)) + vs) * k2;
}

inline uint64_t hash_short(const char *s, size_t length, uint64_t seed)
{
   if (length >= 4 && length <= 8)
      return hash_4to8_bytes(s, length, seed);
   if (length > 8 && length <= 16)
      return hash_9to16_bytes(s, length, seed);
   if (length > 16 && length <= 32)
      return hash_17to32_bytes(s, length, seed);
   if (length > 32)
      return hash_33to64_bytes(s, length, seed);
   if (length != 0)
      return hash_1to3_bytes(s, length, seed);

   return k2 ^ seed;
}

/// The intermediate state used during hashing.
/// Currently, the algorithm for computing hash codes is based on CityHash and
/// keeps 56 bytes of arbitrary state.
struct HashState
{
   uint64_t h0, h1, h2, h3, h4, h5, h6;

   /// \brief Create a new HashState structure and initialize it based on the
   /// seed and the first 64-byte chunk.
   /// This effectively performs the initial mix.
   static HashState create(const char *s, uint64_t seed)
   {
      HashState state = {
         0, seed, hash_16_bytes(seed, k1), rotate(seed ^ k1, 49),
         seed * k1, shift_mix(seed), 0 };
      state.h6 = hash_16_bytes(state.h4, state.h5);
      state.mix(s);
      return state;
   }

   /// \brief Mix 32-bytes from the input sequence into the 16-bytes of 'a'
   /// and 'b', including whatever is already in 'a' and 'b'.
   static void mix32Byte(const char *s, uint64_t &a, uint64_t &b)
   {
      a += fetch64(s);
      uint64_t c = fetch64(s + 24);
      b = rotate(b + a + c, 21);
      uint64_t d = a;
      a += fetch64(s + 8) + fetch64(s + 16);
      b += rotate(a, 44) + d;
      a += c;
   }

   /// Mix in a 64-byte buffer of data.
   /// We mix all 64 bytes even when the chunk length is smaller, but we
   /// record the actual length.
   void mix(const char *s)
   {
      h0 = rotate(h0 + h1 + h3 + fetch64(s + 8), 37) * k1;
      h1 = rotate(h1 + h4 + fetch64(s + 48), 42) * k1;
      h0 ^= h6;
      h1 += h3 + fetch64(s + 40);
      h2 = rotate(h2 + h5, 33) * k1;
      h3 = h4 * k1;
      h4 = h0 + h5;
      mix32Byte(s, h3, h4);
      h5 = h2 + h6;
      h6 = h1 + fetch64(s + 16);
      mix32Byte(s + 32, h5, h6);
      std::swap(h2, h0);
   }

   /// Compute the final 64-bit hash code value based on the current
   /// state and the length of bytes hashed.
   uint64_t finalize(size_t length)
   {
      return hash_16_bytes(hash_16_bytes(h3, h5) + shift_mix(h1) * k1 + h2,
                           hash_16_bytes(h4, h6) + shift_mix(length) * k1 + h0);
   }
};


/// A global, fixed seed-override variable.
///
/// This variable can be set using the \see llvm::set_fixed_execution_seed
/// function. See that function for details. Do not, under any circumstances,
/// set or read this variable.
extern uint64_t fixed_seed_override;

inline uint64_t get_execution_seed()
{
   // FIXME: This needs to be a per-execution seed. This is just a placeholder
   // implementation. Switching to a per-execution seed is likely to flush out
   // instability bugs and so will happen as its own commit.
   //
   // However, if there is a fixed seed override set the first time this is
   // called, return that instead of the per-execution seed.
   const uint64_t seedPrime = 0xff51afd7ed558ccdULL;
   static uint64_t seed = fixed_seed_override ? fixed_seed_override
                                            : (size_t)seedPrime;
   return seed;
}


/// \brief Trait to indicate whether a type's bits can be hashed directly.
///
/// A type trait which is true if we want to combine values for hashing by
/// reading the underlying data. It is false if values of this type must
/// first be passed to hash_value, and the resulting HashCodes combined.
//
// FIXME: We want to replace is_integral_or_enum and is_pointer here with
// a predicate which asserts that comparing the underlying storage of two
// values of the type for equality is equivalent to comparing the two values
// for equality. For all the platforms we care about, this holds for integers
// and pointers, but there are platforms where it doesn't and we would like to
// support user-defined types which happen to satisfy this property.
template <typename T>
struct is_hashable_data : std::integral_constant<bool, ((utils::is_integral_or_enum<T>::value ||
                                                         std::is_pointer<T>::value) &&
                                                        64 % sizeof(T) == 0)>
{};

// Special case std::pair to detect when both types are viable and when there
// is no alignment-derived padding in the pair. This is a bit of a lie because
// std::pair isn't truly POD, but it's close enough in all reasonable
// implementations for our use case of hashing the underlying data.
template <typename T, typename U>
struct is_hashable_data<std::pair<T, U>>
      : std::integral_constant<bool, (is_hashable_data<T>::value &&
                                      is_hashable_data<U>::value &&
                                      (sizeof(T) + sizeof(U)) ==
                                      sizeof(std::pair<T, U>))>
{};

/// Helper to get the hashable data representation for a type.
/// This variant is enabled when the type itself can be used.
template <typename T>
typename std::enable_if<is_hashable_data<T>::value, T>::type
get_hashable_data(const T &value)
{
   return value;
}
/// Helper to get the hashable data representation for a type.
/// This variant is enabled when we must first call hash_value and use the
/// result as our data.
template <typename T>
typename std::enable_if<!is_hashable_data<T>::value, size_t>::type
get_hashable_data(const T &value)
{
   using ::polar::basic::hash_value;
   return hash_value(value);
}

/// Helper to store data from a value into a buffer and advance the
/// pointer into that buffer.
///
/// This routine first checks whether there is enough space in the provided
/// buffer, and if not immediately returns false. If there is space, it
/// copies the underlying bytes of value into the buffer, advances the
/// bufferPtr past the copied bytes, and returns true.
template <typename T>
bool store_and_advance(char *&bufferPtr, char *bufferEnd, const T& value,
                       size_t offset = 0)
{
   size_t storeSize = sizeof(value) - offset;
   if (bufferPtr + storeSize > bufferEnd) {
      return false;
   }
   const char *valueData = reinterpret_cast<const char *>(&value);
   memcpy(bufferPtr, valueData + offset, storeSize);
   bufferPtr += storeSize;
   return true;
}

/// Implement the combining of integral values into a HashCode.
///
/// This overload is selected when the value type of the iterator is
/// integral. Rather than computing a HashCode for each object and then
/// combining them, this (as an optimization) directly combines the integers.
template <typename InputIteratorT>
HashCode hash_combine_range_impl(InputIteratorT first, InputIteratorT last)
{
   const uint64_t seed = get_execution_seed();
   char buffer[64], *bufferPtr = buffer;
   char *const bufferEnd = std::end(buffer);
   while (first != last && store_and_advance(bufferPtr, bufferEnd,
                                             get_hashable_data(*first))) {
      ++first;
   }
   if (first == last) {
      return hash_short(buffer, bufferPtr - buffer, seed);
   }
   assert(bufferPtr == bufferEnd);

   HashState state = state.create(buffer, seed);
   size_t length = 64;
   while (first != last) {
      // Fill up the buffer. We don't clear it, which re-mixes the last round
      // when only a partial 64-byte chunk is left.
      bufferPtr = buffer;
      while (first != last && store_and_advance(bufferPtr, bufferEnd,
                                                get_hashable_data(*first))) {
         ++first;
      }
      // Rotate the buffer if we did a partial fill in order to simulate doing
      // a mix of the last 64-bytes. That is how the algorithm works when we
      // have a contiguous byte sequence, and we want to emulate that here.
      std::rotate(buffer, bufferPtr, bufferEnd);

      // Mix this chunk into the current state.
      state.mix(buffer);
      length += bufferPtr - buffer;
   };

   return state.finalize(length);
}

/// Implement the combining of integral values into a HashCode.
///
/// This overload is selected when the value type of the iterator is integral
/// and when the input iterator is actually a pointer. Rather than computing
/// a HashCode for each object and then combining them, this (as an
/// optimization) directly combines the integers. Also, because the integers
/// are stored in contiguous memory, this routine avoids copying each value
/// and directly reads from the underlying memory.
template <typename ValueType>
typename std::enable_if<is_hashable_data<ValueType>::value, HashCode>::type
hash_combine_range_impl(ValueType *first, ValueType *last)
{
   const size_t seed = get_execution_seed();
   const char *strBegin = reinterpret_cast<const char *>(first);
   const char *strEnd = reinterpret_cast<const char *>(last);
   const size_t length = std::distance(strBegin, strEnd);
   if (length <= 64) {
      return hash_short(strBegin, length, seed);
   }
   const char *strAlignedEnd = strBegin + (length & ~63);
   HashState state = state.create(strBegin, seed);
   strBegin += 64;
   while (strBegin != strAlignedEnd) {
      state.mix(strBegin);
      strBegin += 64;
   }
   if (length & 63) {
      state.mix(strEnd - 64);
   }
   return state.finalize(length);
}

} // namespace internal
} // namespace hashing


/// Compute a HashCode for a sequence of values.
///
/// This hashes a sequence of values. It produces the same HashCode as
/// 'hash_combine(a, b, c, ...)', but can run over arbitrary sized sequences
/// and is significantly faster given pointers and types which can be hashed as
/// a sequence of bytes.
template <typename InputIteratorT>
HashCode hash_combine_range(InputIteratorT first, InputIteratorT last)
{
   return ::polar::basic::hashing::internal::hash_combine_range_impl(first, last);
}

// Implementation details for hash_combine.
namespace hashing {
namespace internal {

/// \brief Helper class to manage the recursive combining of hash_combine
/// arguments.
///
/// This class exists to manage the state and various calls involved in the
/// recursive combining of arguments used in hash_combine. It is particularly
/// useful at minimizing the code in the recursive calls to ease the pain
/// caused by a lack of variadic functions.
struct HashCombineRecursiveHelper
{
   char buffer[64];
   HashState state;
   const uint64_t seed;

public:
   /// \brief Construct a recursive hash combining helper.
   ///
   /// This sets up the state for a recursive hash combine, including getting
   /// the seed and buffer setup.
   HashCombineRecursiveHelper()
      : seed(get_execution_seed())
   {}

   /// Combine one chunk of data into the current in-flight hash.
   ///
   /// This merges one chunk of data into the hash. First it tries to buffer
   /// the data. If the buffer is full, it hashes the buffer into its
   /// HashState, empties it, and then merges the new chunk in. This also
   /// handles cases where the data straddles the end of the buffer.
   template <typename T>
   char *combineData(size_t &length, char *bufferPtr, char *bufferEnd, T data)
   {
      if (!store_and_advance(bufferPtr, bufferEnd, data)) {
         // Check for skew which prevents the buffer from being packed, and do
         // a partial store into the buffer to fill it. This is only a concern
         // with the variadic combine because that formation can have varying
         // argument types.
         size_t partialStoreSize = bufferEnd - bufferPtr;
         memcpy(bufferPtr, &data, partialStoreSize);

         // If the store fails, our buffer is full and ready to hash. We have to
         // either initialize the hash state (on the first full buffer) or mix
         // this buffer into the existing hash state. Length tracks the *hashed*
         // length, not the buffered length.
         if (length == 0) {
            state = state.create(buffer, seed);
            length = 64;
         } else {
            // Mix this chunk into the current state and bump length up by 64.
            state.mix(buffer);
            length += 64;
         }
         // Reset the bufferPtr to the head of the buffer for the next chunk of
         // data.
         bufferPtr = buffer;

         // Try again to store into the buffer -- this cannot fail as we only
         // store types smaller than the buffer.
         if (!store_and_advance(bufferPtr, bufferEnd, data,
                                partialStoreSize)) {
            abort();
         }
      }
      return bufferPtr;
   }

   /// \brief Recursive, variadic combining method.
   ///
   /// This function recurses through each argument, combining that argument
   /// into a single hash.
   template <typename T, typename ...Ts>
   HashCode combine(size_t length, char *bufferPtr, char *bufferEnd,
                    const T &arg, const Ts &...args)
   {
      bufferPtr = combineData(length, bufferPtr, bufferEnd, get_hashable_data(arg));

      // Recurse to the next argument.
      return combine(length, bufferPtr, bufferEnd, args...);
   }

   /// Base case for recursive, variadic combining.
   ///
   /// The base case when combining arguments recursively is reached when all
   /// arguments have been handled. It flushes the remaining buffer and
   /// constructs a HashCode.
   HashCode combine(size_t length, char *bufferPtr, char *bufferEnd)
   {
      // Check whether the entire set of values fit in the buffer. If so, we'll
      // use the optimized short hashing routine and skip state entirely.
      if (length == 0) {
         return hash_short(buffer, bufferPtr - buffer, seed);
      }
      // Mix the final buffer, rotating it if we did a partial fill in order to
      // simulate doing a mix of the last 64-bytes. That is how the algorithm
      // works when we have a contiguous byte sequence, and we want to emulate
      // that here.
      std::rotate(buffer, bufferPtr, bufferEnd);
      // Mix this chunk into the current state.
      state.mix(buffer);
      length += bufferPtr - buffer;
      return state.finalize(length);
   }
};

} // namespace internal
} // namespace hashing

/// Combine values into a single HashCode.
///
/// This routine accepts a varying number of arguments of any type. It will
/// attempt to combine them into a single HashCode. For user-defined types it
/// attempts to call a \see hash_value overload (via ADL) for the type. For
/// integer and pointer types it directly combines their data into the
/// resulting HashCode.
///
/// The result is suitable for returning from a user's hash_value
/// *implementation* for their user-defined type. Consumers of a type should
/// *not* call this routine, they should instead call 'hash_value'.
template <typename ...Ts> HashCode hash_combine(const Ts &...args)
{
   // Recursively hash each argument using a helper class.
   ::polar::basic::hashing::internal::HashCombineRecursiveHelper helper;
   return helper.combine(0, helper.buffer, helper.buffer + 64, args...);
}

// Implementation details for implementations of hash_value overloads provided
// here.
namespace hashing {
namespace internal {

/// Helper to hash the value of a single integer.
///
/// Overloads for smaller integer types are not provided to ensure consistent
/// behavior in the presence of integral promotions. Essentially,
/// "hash_value('4')" and "hash_value('0' + 4)" should be the same.
inline HashCode hash_integer_value(uint64_t value)
{
   // Similar to hash_4to8_bytes but using a seed instead of length.
   const uint64_t seed = get_execution_seed();
   const char *s = reinterpret_cast<const char *>(&value);
   const uint64_t a = fetch32(s);
   return hash_16_bytes(seed + (a << 3), fetch32(s + 4));
}

} // namespace internal
} // namespace hashing

// Declared and documented above, but defined here so that any of the hashing
// infrastructure is available.
template <typename T>
typename std::enable_if<utils::is_integral_or_enum<T>::value, HashCode>::type
hash_value(T value)
{
   return ::polar::basic::hashing::internal::hash_integer_value(
            static_cast<uint64_t>(value));
}

// Declared and documented above, but defined here so that any of the hashing
// infrastructure is available.
template <typename T> HashCode hash_value(const T *ptr)
{
   return ::polar::basic::hashing::internal::hash_integer_value(
            reinterpret_cast<uintptr_t>(ptr));
}

// Declared and documented above, but defined here so that any of the hashing
// infrastructure is available.
template <typename T, typename U>
HashCode hash_value(const std::pair<T, U> &arg)
{
   return hash_combine(arg.first, arg.second);
}

// Declared and documented above, but defined here so that any of the hashing
// infrastructure is available.
template <typename T>
HashCode hash_value(const std::basic_string<T> &arg)
{
   return hash_combine_range(arg.begin(), arg.end());
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_HASING_H
