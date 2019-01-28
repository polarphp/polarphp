// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/17.

#ifndef POLARPHP_UTILS_SMALL_VECTOR_MEMORY_BUFFER_H
#define POLARPHP_UTILS_SMALL_VECTOR_MEMORY_BUFFER_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

/// SmallVector-backed MemoryBuffer instance.
///
/// This class enables efficient construction of MemoryBuffers from SmallVector
/// instances. This is useful for MCJIT and Orc, where object files are streamed
/// into SmallVectors, then inspected using ObjectFile (which takes a
/// MemoryBuffer).
class SmallVectorMemoryBuffer : public MemoryBuffer
{
public:
   /// Construct an SmallVectorMemoryBuffer from the given SmallVector
   /// r-value.
   ///
   /// FIXME: It'd be nice for this to be a non-templated constructor taking a
   /// SmallVectorImpl here instead of a templated one taking a SmallVector<N>,
   /// but SmallVector's move-construction/assignment currently only take
   /// SmallVectors. If/when that is fixed we can simplify this constructor and
   /// the following one.
   SmallVectorMemoryBuffer(SmallVectorImpl<char> &&vector)
      : m_vector(std::move(vector)), m_bufferName("<in-memory object>")
   {
      init(this->m_vector.begin(), this->m_vector.end(), false);
   }

   /// Construct a named SmallVectorMemoryBuffer from the given
   /// SmallVector r-value and StringRef.
   SmallVectorMemoryBuffer(SmallVectorImpl<char> &&vector, StringRef name)
      : m_vector(std::move(vector)), m_bufferName(name)
   {
      init(this->m_vector.begin(), this->m_vector.end(), false);
   }

   // Key function.
   ~SmallVectorMemoryBuffer() override;

   StringRef getBufferIdentifier() const override
   {
      return m_bufferName;
   }

   BufferKind getBufferKind() const override
   {
      return BufferKind::MemoryBuffer_Malloc;
   }

private:
   SmallVector<char, 0> m_vector;
   std::string m_bufferName;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_SMALL_VECTOR_MEMORY_BUFFER_H
