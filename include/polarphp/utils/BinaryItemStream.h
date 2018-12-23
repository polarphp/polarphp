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

#ifndef POLARPHP_UTILS_BINARY_ITEM_STREAM_H
#define POLARPHP_UTILS_BINARY_ITEM_STREAM_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/BinaryStream.h"
#include "polarphp/utils/BinaryStreamError.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/Endian.h"
#include <cstddef>
#include <cstdint>

namespace polar {
namespace utils {

template <typename T> struct BinaryItemTraits
{
   static size_t getLength(const T &item) = delete;
   static ArrayRef<uint8_t> getBytes(const T &item) = delete;
};

/// BinaryItemStream represents a sequence of objects stored in some kind of
/// external container but for which it is useful to view as a stream of
/// contiguous bytes.  An example of this might be if you have a collection of
/// records and you serialize each one into a buffer, and store these serialized
/// records in a container.  The pointers themselves are not laid out
/// contiguously in memory, but we may wish to read from or write to these
/// records as if they were.
template <typename T, typename Traits = BinaryItemTraits<T>>
class BinaryItemStream : public BinaryStream {
public:
   explicit BinaryItemStream(Endianness endian)
      : m_endian(endian) {}

   Endianness getEndian() const override
   {
      return m_endian;
   }

   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) override
   {
      auto expectedIndex = translateOffsetIndex(offset);
      if (!expectedIndex) {
         return expectedIndex.takeError();
      }
      const auto &item = m_items[*expectedIndex];
      if (auto errorCode = checkOffsetForRead(offset, size)) {
         return errorCode;
      }
      if (size > Traits::getLength(item)) {
         return make_error<BinaryStreamError>(StreamErrorCode::stream_too_short);
      }
      buffer = Traits::getBytes(item).takeFront(size);
      return Error::getSuccess();
   }

   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) override
   {
      auto expectedIndex = translateOffsetIndex(offset);
      if (!expectedIndex) {
         return expectedIndex.takeError();
      }
      buffer = Traits::getBytes(m_items[*expectedIndex]);
      return Error::getSuccess();
   }

   void setItems(ArrayRef<T> itemArray)
   {
      m_items = itemArray;
      computeItemOffsets();
   }

   uint32_t getLength() override
   {
      return m_itemEndOffsets.empty() ? 0 : m_itemEndOffsets.back();
   }

private:
   void computeItemOffsets()
   {
      m_itemEndOffsets.clear();
      m_itemEndOffsets.reserve(m_items.getSize());
      uint32_t m_currentOffset = 0;
      for (const auto &item : m_items) {
         uint32_t length = Traits::getLength(item);
         assert(length > 0 && "no empty items");
         m_currentOffset += length;
         m_itemEndOffsets.push_back(m_currentOffset);
      }
   }

   Expected<uint32_t> translateOffsetIndex(uint32_t offset)
   {
      // Make sure the offset is somewhere in our items array.
      if (offset >= getLength()) {
         return make_error<BinaryStreamError>(StreamErrorCode::stream_too_short);
      }
      ++offset;
      auto iter =
            std::lower_bound(m_itemEndOffsets.begin(), m_itemEndOffsets.end(), offset);
      size_t idx = std::distance(m_itemEndOffsets.begin(), iter);
      assert(idx < m_items.getSize() && "binary search for offset failed");
      return idx;
   }

   Endianness m_endian;
   ArrayRef<T> m_items;

   // Sorted vector of offsets to accelerate lookup.
   std::vector<uint32_t> m_itemEndOffsets;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_BINARY_ITEM_STREAM_H
