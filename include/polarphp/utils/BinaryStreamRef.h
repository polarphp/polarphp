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

#ifndef POLARPHP_UTILS_BINARY_STREAM_REF_H
#define POLARPHP_UTILS_BINARY_STREAM_REF_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/BinaryStream.h"
#include "polarphp/utils/BinaryStreamError.h"
#include "polarphp/utils/Error.h"
#include <algorithm>
#include <cstdint>
#include <memory>

namespace polar {
namespace utils {

using polar::basic::MutableArrayRef;

/// Common stuff for mutable and immutable StreamRefs.
template <typename RefType, typename StreamType>
class BinaryStreamRefBase
{
protected:
   BinaryStreamRefBase() = default;
   explicit BinaryStreamRefBase(StreamType &borrowedImpl)
      : m_borrowedImpl(&borrowedImpl), m_viewOffset(0)
   {
      if (!(borrowedImpl.getFlags() & BSF_Append)) {
         m_length = borrowedImpl.getLength();
      }
   }

   BinaryStreamRefBase(std::shared_ptr<StreamType> sharedImpl, uint32_t offset,
                       std::optional<uint32_t> length)
      : m_sharedImpl(sharedImpl), m_borrowedImpl(sharedImpl.get()),
        m_viewOffset(offset), m_length(length) {}
   BinaryStreamRefBase(StreamType &borrowedImpl, uint32_t offset,
                       std::optional<uint32_t> length)
      : m_borrowedImpl(&borrowedImpl), m_viewOffset(offset), m_length(length)
   {}

   BinaryStreamRefBase(const BinaryStreamRefBase &other) = default;
   BinaryStreamRefBase &operator=(const BinaryStreamRefBase &other) = default;

   BinaryStreamRefBase &operator=(BinaryStreamRefBase &&other) = default;
   BinaryStreamRefBase(BinaryStreamRefBase &&other) = default;

public:
   Endianness getEndian() const
   {
      return m_borrowedImpl->getEndian();
   }

   uint32_t getLength() const
   {
      if (m_length.has_value()) {
         return *m_length;
      }
      return m_borrowedImpl ? (m_borrowedImpl->getLength() - m_viewOffset) : 0;
   }

   /// Return a new BinaryStreamRef with the first \p N elements removed.  If
   /// this BinaryStreamRef is length-tracking, then the resulting one will be
   /// too.
   RefType dropFront(uint32_t size) const
   {
      if (!m_borrowedImpl) {
         return RefType();
      }
      size = std::min(size, getLength());
      RefType result(static_cast<const RefType &>(*this));
      if (size == 0) {
         return result;
      }
      result.m_viewOffset += size;
      if (result.m_length.has_value()) {
         *result.m_length -= size;
      }
      return result;
   }

   /// Return a new BinaryStreamRef with the last \p N elements removed.  If
   /// this BinaryStreamRef is length-tracking and \p N is greater than 0, then
   /// this BinaryStreamRef will no longer length-track.
   RefType dropBack(uint32_t size) const
   {
      if (!m_borrowedImpl) {
         return RefType();
      }
      RefType result(static_cast<const RefType &>(*this));
      size = std::min(size, getLength());
      if (size == 0) {
         return result;
      }
      // Since we're dropping non-zero bytes from the end, stop length-tracking
      // by setting the length of the resulting StreamRef to an explicit value.
      if (!result.m_length.has_value()) {
         result.m_length = getLength();
      }
      *result.m_length -= size;
      return result;
   }

   /// Return a new BinaryStreamRef with only the first \p N elements remaining.
   RefType keepFront(uint32_t size) const
   {
      assert(size <= getLength());
      return dropBack(getLength() - size);
   }

   /// Return a new BinaryStreamRef with only the last \p N elements remaining.
   RefType keepBack(uint32_t size) const
   {
      assert(size <= getLength());
      return dropFront(getLength() - size);
   }

   /// Return a new BinaryStreamRef with the first and last \p N elements
   /// removed.
   RefType dropSymmetric(uint32_t size) const
   {
      return dropFront(size).dropBack(size);
   }

   /// Return a new BinaryStreamRef with the first \p offset elements removed,
   /// and retaining exactly \p Len elements.
   RefType slice(uint32_t offset, uint32_t length) const
   {
      return dropFront(offset).keepFront(length);
   }

   bool valid() const
   {
      return m_borrowedImpl != nullptr;
   }

   bool operator==(const RefType &other) const
   {
      if (m_borrowedImpl != other.m_borrowedImpl) {
         return false;
      }
      if (m_viewOffset != other.m_viewOffset) {
         return false;
      }
      if (m_length != other.m_length) {
         return false;
      }
      return true;
   }

protected:
   Error checkOffsetForRead(uint32_t offset, uint32_t dataSize) const
   {
      if (offset > getLength()) {
         return make_error<BinaryStreamError>(StreamErrorCode::invalid_offset);
      }
      if (getLength() < dataSize + offset) {
         return make_error<BinaryStreamError>(StreamErrorCode::stream_too_short);
      }
      return Error::getSuccess();
   }

   std::shared_ptr<StreamType> m_sharedImpl;
   StreamType *m_borrowedImpl = nullptr;
   uint32_t m_viewOffset = 0;
   std::optional<uint32_t> m_length;
};

/// \brief BinaryStreamRef is to BinaryStream what ArrayRef is to an Array.  It
/// provides copy-semantics and read only access to a "window" of the underlying
/// BinaryStream. Note that BinaryStreamRef is *not* a BinaryStream.  That is to
/// say, it does not inherit and override the methods of BinaryStream.  In
/// general, you should not pass around pointers or references to BinaryStreams
/// and use inheritance to achieve polymorphism.  Instead, you should pass
/// around BinaryStreamRefs by value and achieve polymorphism that way.
class BinaryStreamRef
      : public BinaryStreamRefBase<BinaryStreamRef, BinaryStream>
{
   friend class BinaryStreamRefBase<BinaryStreamRef, BinaryStream>;
   friend class WritableBinaryStreamRef;
   BinaryStreamRef(std::shared_ptr<BinaryStream> impl, uint32_t m_viewOffset,
                   std::optional<uint32_t> length)
      : BinaryStreamRefBase(impl, m_viewOffset, length)
   {}

public:
   BinaryStreamRef() = default;
   BinaryStreamRef(BinaryStream &stream);
   BinaryStreamRef(BinaryStream &stream, uint32_t offset,
                   std::optional<uint32_t> length);
   explicit BinaryStreamRef(ArrayRef<uint8_t> data,
                            Endianness endian);
   explicit BinaryStreamRef(StringRef data, Endianness endian);

   BinaryStreamRef(const BinaryStreamRef &other) = default;
   BinaryStreamRef &operator=(const BinaryStreamRef &other) = default;
   BinaryStreamRef(BinaryStreamRef &&other) = default;
   BinaryStreamRef &operator=(BinaryStreamRef &&other) = default;

   // Use BinaryStreamRef.slice() instead.
   BinaryStreamRef(BinaryStreamRef &stream, uint32_t offset,
                   uint32_t length) = delete;

   /// Given an offset into this StreamRef and a Size, return a reference to a
   /// buffer owned by the stream.
   ///
   /// \returns a getSuccess error code if the entire range of data is within the
   /// bounds of this BinaryStreamRef's view and the implementation could read
   /// the data, and an appropriate error code otherwise.
   Error readBytes(uint32_t offset, uint32_t size,
                   ArrayRef<uint8_t> &buffer) const;

   /// Given an offset into this BinaryStreamRef, return a reference to the
   /// largest buffer the stream could support without necessitating a copy.
   ///
   /// \returns a getSuccess error code if implementation could read the data,
   /// and an appropriate error code otherwise.
   Error readLongestContiguousChunk(uint32_t offset,
                                    ArrayRef<uint8_t> &buffer) const;
};

struct BinarySubstreamRef
{
   uint32_t m_offset;            // offset in the parent stream
   BinaryStreamRef m_streamData; // Stream Data

   BinarySubstreamRef slice(uint32_t offset, uint32_t size) const
   {
      BinaryStreamRef subSub = m_streamData.slice(offset, m_offset);
      return {offset + offset, subSub};
   }

   BinarySubstreamRef dropFront(uint32_t size) const
   {
      return slice(size, getSize() - size);
   }
   BinarySubstreamRef keepFront(uint32_t size) const
   {
      return slice(0, size);
   }

   std::pair<BinarySubstreamRef, BinarySubstreamRef>
   split(uint32_t offset) const
   {
      return std::make_pair(keepFront(offset), dropFront(offset));
   }

   uint32_t getSize() const
   {
      return m_streamData.getLength();
   }

   bool empty() const
   {
      return getSize() == 0;
   }
};

class WritableBinaryStreamRef
      : public BinaryStreamRefBase<WritableBinaryStreamRef,
      WritableBinaryStream>
{
   friend class BinaryStreamRefBase<WritableBinaryStreamRef, WritableBinaryStream>;
   WritableBinaryStreamRef(std::shared_ptr<WritableBinaryStream> impl,
                           uint32_t m_viewOffset, std::optional<uint32_t> length)
      : BinaryStreamRefBase(impl, m_viewOffset, length)
   {}

   Error checkOffsetForWrite(uint32_t offset, uint32_t dataSize) const
   {
      if (!(m_borrowedImpl->getFlags() & BSF_Append)) {
         return checkOffsetForRead(offset, dataSize);
      }
      if (offset > getLength()) {
         return make_error<BinaryStreamError>(StreamErrorCode::invalid_offset);
      }
      return Error::getSuccess();
   }

public:
   WritableBinaryStreamRef() = default;
   WritableBinaryStreamRef(WritableBinaryStream &stream);
   WritableBinaryStreamRef(WritableBinaryStream &stream, uint32_t offset,
                           std::optional<uint32_t> length);
   explicit WritableBinaryStreamRef(MutableArrayRef<uint8_t> data,
                                    Endianness endian);
   WritableBinaryStreamRef(const WritableBinaryStreamRef &other) = default;
   WritableBinaryStreamRef &
   operator=(const WritableBinaryStreamRef &other) = default;

   WritableBinaryStreamRef(WritableBinaryStreamRef &&other) = default;
   WritableBinaryStreamRef &operator=(WritableBinaryStreamRef &&other) = default;

   // Use WritableBinaryStreamRef.slice() instead.
   WritableBinaryStreamRef(WritableBinaryStreamRef &stream, uint32_t offset,
                           uint32_t length) = delete;

   /// Given an offset into this WritableBinaryStreamRef and some input data,
   /// writes the data to the underlying stream.
   ///
   /// \returns a getSuccess error code if the data could fit within the underlying
   /// stream at the specified location and the implementation could write the
   /// data, and an appropriate error code otherwise.
   Error writeBytes(uint32_t offset, ArrayRef<uint8_t> data) const;

   /// Conver this WritableBinaryStreamRef to a read-only BinaryStreamRef.
   operator BinaryStreamRef() const;

   /// \brief For buffered streams, commits changes to the backing store.
   Error commit();
};

} // utils
} // polar

#endif // POLARPHP_UTILS_BINARY_STREAM_REF_H
