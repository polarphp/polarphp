// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/28.

/// Lightweight arrays that are backed by an arbitrary BinaryStream.  This file
/// provides two different m_array implementations.
///
///     VarStreamArray - Arrays of variable length records.  The user specifies
///       an Extractor type that can extract a record from a given offset and
///       return the number of bytes consumed by the record.
///
///     FixedStreamArray - Arrays of fixed length records.  This is similar in
///       spirit to ArrayRef<T>, but since it is backed by a BinaryStream, the
///       elements of the m_array need not be laid out in contiguous memory.

#ifndef POLARPHP_UTILS_BINARY_STREAM_ARRAY_H
#define POLARPHP_UTILS_BINARY_STREAM_ARRAY_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/utils/BinaryStreamRef.h"
#include "polarphp/utils/Error.h"
#include <cassert>
#include <cstdint>

namespace polar {
namespace utils {

using polar::basic::IteratorFacadeBase;
using polar::basic::ArrayRef;

/// VarStreamArrayExtractor is intended to be specialized to provide customized
/// extraction logic.  On input it receives a BinaryStreamRef pointing to the
/// beginning of the next record, but where the length of the record is not yet
/// known.  Upon completion, it should return an appropriate Error instance if
/// a record could not be extracted, or if one could be extracted it should
/// return success and set Len to the number of bytes this record occupied in
/// the underlying stream, and it should fill out the fields of the value type
/// Item appropriately to represent the current record.
///
/// You can specialize this template for your own custom value types to avoid
/// having to specify a second template argument to VarStreamArray (documented
/// below).
template <typename T>
struct VarStreamArrayExtractor
{
   // Method intentionally deleted.  You must provide an explicit specialization
   // with the following method implemented.
   Error operator()(BinaryStreamRef stream, uint32_t &len,
                    T &item) const = delete;
};

/// VarStreamArray represents an m_array of variable length records backed by a
/// stream.  This could be a contiguous sequence of bytes in memory, it could
/// be a file on disk, or it could be a PDB stream where bytes are stored as
/// discontiguous blocks in a file.  Usually it is desirable to treat arrays
/// as contiguous blocks of memory, but doing so with large PDB files, for
/// example, could mean allocating huge amounts of memory just to allow
/// re-ordering of stream data to be contiguous before iterating over it.  By
/// abstracting this out, we need not duplicate this memory, and we can
/// iterate over arrays in arbitrarily formatted streams.  Elements are parsed
/// lazily on iteration, so there is no upfront cost associated with building
/// or copying a VarStreamArray, no matter how large it may be.
///
/// You create a VarStreamArray by specifying a ValueType and an Extractor type.
/// If you do not specify an Extractor type, you are expected to specialize
/// VarStreamArrayExtractor<T> for your ValueType.
///
/// By default an Extractor is default constructed in the class, but in some
/// cases you might find it useful for an Extractor to maintain state across
/// extractions.  In this case you can provide your own Extractor through a
/// secondary constructor.  The following examples show various ways of
/// creating a VarStreamArray.
///
///       // Will use VarStreamArrayExtractor<MyType> as the extractor.
///       VarStreamArray<MyType> MyTypeArray;
///
///       // Will use a default-constructed MyExtractor as the extractor.
///       VarStreamArray<MyType, MyExtractor> MyTypeArray2;
///
///       // Will use the specific instance of MyExtractor provided.
///       // MyExtractor need not be default-constructible in this case.
///       MyExtractor E(SomeContext);
///       VarStreamArray<MyType, MyExtractor> MyTypeArray3(E);
///

template <typename ValueType, typename Extractor>
class VarStreamArrayIterator;

template <typename ValueType,
          typename Extractor = VarStreamArrayExtractor<ValueType>>
class VarStreamArray
{
   friend class VarStreamArrayIterator<ValueType, Extractor>;

public:
   typedef VarStreamArrayIterator<ValueType, Extractor> Iterator;

   VarStreamArray() = default;

   explicit VarStreamArray(const Extractor &extractor) : m_extractor(extractor)
   {}

   explicit VarStreamArray(BinaryStreamRef m_stream) : m_stream(m_stream)
   {}

   VarStreamArray(BinaryStreamRef m_stream, const Extractor &extractor)
      : m_stream(m_stream), m_extractor(extractor)
   {}

   Iterator begin(bool *hadError = nullptr) const
   {
      return Iterator(*this, m_extractor, hadError);
   }

   bool valid() const
   {
      return m_stream.valid();
   }

   Iterator end() const
   {
      return Iterator(m_extractor);
   }

   bool empty() const
   {
      return m_stream.getLength() == 0;
   }

   /// \brief given an offset into the m_array's underlying stream, return an
   /// iterator to the record at that offset.  This is considered unsafe
   /// since the behavior is undefined if \p Offset does not refer to the
   /// beginning of a valid record.
   Iterator at(uint32_t offset) const
   {
      return Iterator(*this, m_extractor, offset, nullptr);
   }

   const Extractor &getExtractor() const
   {
      return m_extractor;
   }

   Extractor &getExtractor()
   {
      return m_extractor;
   }

   BinaryStreamRef getUnderlyingStream() const
   {
      return m_stream;
   }

   void setUnderlyingStream(BinaryStreamRef stream)
   {
      m_stream = stream;
   }

   void drop_front()
   {
      m_stream = m_stream.dropFront(begin()->length());
   }

   inline void dropFront()
   {
      drop_front();
   }

private:
   BinaryStreamRef m_stream;
   Extractor m_extractor;
};

template <typename ValueType, typename Extractor>
class VarStreamArrayIterator
      : public IteratorFacadeBase<VarStreamArrayIterator<ValueType, Extractor>,
      std::forward_iterator_tag, ValueType>
{
   typedef VarStreamArrayIterator<ValueType, Extractor> IterType;
   typedef VarStreamArray<ValueType, Extractor> ArrayType;

public:
   VarStreamArrayIterator(const ArrayType &array, const Extractor &extractor,
                          bool *hadError)
      : VarStreamArrayIterator(array, extractor, 0, hadError)
   {}

   VarStreamArrayIterator(const ArrayType &array, const Extractor &extractor,
                          uint32_t offset, bool *hadError)
      : m_iterRef(array.m_stream.dropFront(offset)), m_extractor(extractor),
        m_m_array(&array), m_absOffset(offset), m_hadError(hadError)
   {
      if (m_iterRef.getLength() == 0) {
         moveToEnd();
      } else {
         auto errorCode = m_extractor(m_iterRef, m_thisLen, m_thisValue);
         if (errorCode) {
            consume_error(std::move(errorCode));
            markError();
         }
      }
   }

   VarStreamArrayIterator() = default;
   explicit VarStreamArrayIterator(const Extractor &extractor) : m_extractor(extractor)
   {}
   ~VarStreamArrayIterator() = default;

   bool operator==(const IterType &other) const
   {
      if (m_m_array && other.m_m_array) {
         // Both have a valid m_array, make sure they're same.
         assert(m_m_array == other.m_m_array);
         return m_iterRef == other.m_iterRef;
      }

      // Both iterators are at the end.
      if (!m_m_array && !other.m_m_array) {
         return true;
      }

      // One is not at the end and one is.
      return false;
   }

   const ValueType &operator*() const
   {
      assert(m_m_array && !m_hasError);
      return m_thisValue;
   }

   ValueType &operator*()
   {
      assert(m_m_array && !m_hasError);
      return m_thisValue;
   }

   IterType &operator+=(unsigned size) {
      for (unsigned idx = 0; idx < size; ++idx) {
         // We are done with the current record, discard it so that we are
         // positioned at the next record.
         m_absOffset += m_thisLen;
         m_iterRef = m_iterRef.dropFront(m_thisLen);
         if (m_iterRef.getLength() == 0) {
            // There is nothing after the current record, we must make this an end
            // iterator.
            moveToEnd();
         } else {
            // There is some data after the current record.
            auto errorCode = m_extractor(m_iterRef, m_thisLen, m_thisValue);
            if (errorCode) {
               consume_error(std::move(errorCode));
               markError();
            } else if (m_thisLen == 0) {
               // An empty record? Make this an end iterator.
               moveToEnd();
            }
         }
      }
      return *this;
   }

   uint32_t offset() const
   {
      return m_absOffset;
   }

   uint32_t getRecordLength() const
   {
      return m_thisLen;
   }

private:
   void moveToEnd()
   {
      m_m_array = nullptr;
      m_thisLen = 0;
   }

   void markError()
   {
      moveToEnd();
      m_hasError = true;
      if (m_hadError != nullptr) {
         *m_hadError = true;
      }
   }

   ValueType m_thisValue;
   BinaryStreamRef m_iterRef;
   Extractor m_extractor;
   const ArrayType *m_m_array{nullptr};
   uint32_t m_thisLen{0};
   uint32_t m_absOffset{0};
   bool m_hasError{false};
   bool *m_hadError{nullptr};
};

template <typename T> class FixedStreamArrayIterator;

/// FixedStreamArray is similar to VarStreamArray, except with each record
/// having a fixed-length.  As with VarStreamArray, them_hadErrorre is no upfront
/// cost associated with building or copying a FixedStreamArray, as the
/// memory for each element is not read from the backing stream until that
/// element is iterated.
template <typename T>
class FixedStreamArray
{
   friend class FixedStreamArrayIterator<T>;

public:
   typedef FixedStreamArrayIterator<T> Iterator;

   FixedStreamArray() = default;
   explicit FixedStreamArray(BinaryStreamRef m_stream) : m_stream(m_stream)
   {
      assert(m_stream.getLength() % sizeof(T) == 0);
   }

   bool operator==(const FixedStreamArray<T> &other) const
   {
      return m_stream == other.m_stream;
   }

   bool operator!=(const FixedStreamArray<T> &other) const
   {
      return !(*this == other);
   }

   FixedStreamArray &operator=(const FixedStreamArray &) = default;

   const T &operator[](uint32_t idx) const
   {
      assert(idx < getSize());
      uint32_t offset = idx * sizeof(T);
      ArrayRef<uint8_t> data;
      if (auto errorCode = m_stream.readBytes(offset, sizeof(T), data)) {
         assert(false && "Unexpected failure reading from stream");
         // This should never happen since we asserted that the stream length was
         // an exact multiple of the element size.
         consume_error(std::move(errorCode));
      }
      assert(polar::utils::alignment_adjustment(data.getData(), alignof(T)) == 0);
      return *reinterpret_cast<const T *>(data.getData());
   }

   uint32_t getSize() const
   {
      return m_stream.getLength() / sizeof(T);
   }

   bool empty() const
   {
      return getSize() == 0;
   }

   FixedStreamArrayIterator<T> begin() const
   {
      return FixedStreamArrayIterator<T>(*this, 0);
   }

   FixedStreamArrayIterator<T> end() const
   {
      return FixedStreamArrayIterator<T>(*this, getSize());
   }

   const T &front() const
   {
      return *begin();
   }

   const T &back() const
   {
      FixedStreamArrayIterator<T> iter = end();
      return *(--iter);
   }

   BinaryStreamRef getUnderlyingStream() const
   {
      return m_stream;
   }

private:
   BinaryStreamRef m_stream;
};

template <typename T>
class FixedStreamArrayIterator
      : public IteratorFacadeBase<FixedStreamArrayIterator<T>,
      std::random_access_iterator_tag, const T>
{

public:
   FixedStreamArrayIterator(const FixedStreamArray<T> &array, uint32_t idx)
      : m_array(array), m_idx(idx)
   {}

   FixedStreamArrayIterator<T> &
   operator=(const FixedStreamArrayIterator<T> &other)
   {
      m_array = other.m_array;
      m_idx = other.m_idx;
      return *this;
   }

   const T &operator*() const
   {
      return m_array[m_idx];
   }

   const T &operator*()
   {
      return m_array[m_idx];
   }

   bool operator==(const FixedStreamArrayIterator<T> &other) const
   {
      assert(m_array == other.m_array);
      return (m_idx == other.m_idx) && (m_array == other.m_array);
   }

   FixedStreamArrayIterator<T> &operator+=(std::ptrdiff_t size)
   {
      m_idx += size;
      return *this;
   }

   FixedStreamArrayIterator<T> &operator-=(std::ptrdiff_t size)
   {
      assert(std::ptrdiff_t(m_idx) >= size);
      m_idx -= size;
      return *this;
   }

   std::ptrdiff_t operator-(const FixedStreamArrayIterator<T> &other) const
   {
      assert(m_array == other.m_array);
      assert(m_idx >= other.m_idx);
      return m_idx - other.m_idx;
   }

   bool operator<(const FixedStreamArrayIterator<T> &other) const
   {
      assert(m_array == other.m_array);
      return m_idx < other.m_idx;
   }

private:
   FixedStreamArray<T> m_array;
   uint32_t m_idx;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_BINARY_STREAM_ARRAY_H
