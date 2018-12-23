// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/21.

//===----------------------------------------------------------------------===//
//
// This file declares the polar::sys::Memory class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_MEMORY_H
#define POLARPHP_UTILS_MEMORY_H

#include "polarphp/global/DataTypes.h"
#include <string>
#include <system_error>

namespace polar {
namespace sys {

/// This class encapsulates the notion of a memory block which has an m_address
/// and a size. It is used by the Memory class (a friend) as the result of
/// various memory allocation operations.
/// @see Memory
/// @brief Memory block abstraction.
class MemoryBlock
{
public:
   MemoryBlock() : m_address(nullptr),
      m_size(0)
   {}

   MemoryBlock(void *addr, size_t size)
      : m_address(addr),
        m_size(size)
   {}

   void *getBase() const
   {
      return m_address;
   }

   size_t getSize() const
   {
      return m_size;
   }

private:
   void *m_address;    ///< m_address of first byte of memory area
   size_t m_size;      ///< Size, in bytes of the memory area
   friend class Memory;
};

/// This class provides various memory handling functions that manipulate
/// MemoryBlock instances.
/// @since 1.4
/// @brief An abstraction for memory operations.
class Memory
{
public:
   enum ProtectionFlags
   {
      MF_READ  = 0x1000000,
      MF_WRITE = 0x2000000,
      MF_EXEC  = 0x4000000
   };

   /// This method allocates a block of memory that is suitable for loading
   /// dynamically generated code (e.g. JIT). An attempt to allocate
   /// \p NumBytes bytes of virtual memory is made.
   /// \p NearBlock may point to an existing allocation in which case
   /// an attempt is made to allocate more memory near the existing block.
   /// The actual allocated m_address is not guaranteed to be near the requested
   /// m_address.
   /// \p Flags is used to set the initial protection flags for the block
   /// of the memory.
   /// \p EC [out] returns an object describing any error that occurs.
   ///
   /// This method may allocate more than the number of bytes requested.  The
   /// actual number of bytes allocated is indicated in the returned
   /// MemoryBlock.
   ///
   /// The start of the allocated block must be aligned with the
   /// system allocation granularity (64K on Windows, page size on Linux).
   /// If the m_address following \p NearBlock is not so aligned, it will be
   /// rounded up to the next allocation granularity boundary.
   ///
   /// \r a non-null MemoryBlock if the function was successful,
   /// otherwise a null MemoryBlock is with \p EC describing the error.
   ///
   /// @brief Allocate mapped memory.
   static MemoryBlock allocateMappedMemory(size_t numBytes,
                                           const MemoryBlock *const nearBlock,
                                           unsigned flags,
                                           std::error_code &errorCode);

   /// This method releases a block of memory that was allocated with the
   /// allocateMappedMemory method. It should not be used to release any
   /// memory block allocated any other way.
   /// \p Block describes the memory to be released.
   ///
   /// \r error_success if the function was successful, or an error_code
   /// describing the failure if an error occurred.
   ///
   /// @brief Release mapped memory.
   static std::error_code releaseMappedMemory(MemoryBlock &block);

   /// This method sets the protection flags for a block of memory to the
   /// state specified by /p Flags.  The behavior is not specified if the
   /// memory was not allocated using the allocateMappedMemory method.
   /// \p Block describes the memory block to be protected.
   /// \p Flags specifies the new protection state to be assigned to the block.
   /// \p ErrMsg [out] returns a string describing any error that occurred.
   ///
   /// If \p Flags is MF_WRITE, the actual behavior varies
   /// with the operating system (i.e. MF_READ | MF_WRITE on Windows) and the
   /// target architecture (i.e. MF_WRITE -> MF_READ | MF_WRITE on i386).
   ///
   /// \r error_success if the function was successful, or an error_code
   /// describing the failure if an error occurred.
   ///
   /// @brief Set memory protection state.
   static std::error_code protectMappedMemory(const MemoryBlock &block,
                                              unsigned flags);

   /// invalidateInstructionCache - Before the JIT can run a block of code
   /// that has been emitted it must invalidate the instruction cache on some
   /// platforms.
   static void invalidateInstructionCache(const void *addr, size_t len);
};

/// Owning version of MemoryBlock.
class OwningMemoryBlock
{
public:
   OwningMemoryBlock() = default;
   explicit OwningMemoryBlock(MemoryBlock block)
      : m_memoryBlock(block)
   {}

   OwningMemoryBlock(OwningMemoryBlock &&other)
   {
      m_memoryBlock = other.m_memoryBlock;
      other.m_memoryBlock = MemoryBlock();
   }

   OwningMemoryBlock& operator=(OwningMemoryBlock &&other)
   {
      m_memoryBlock = other.m_memoryBlock;
      other.m_memoryBlock = MemoryBlock();
      return *this;
   }
   ~OwningMemoryBlock()
   {
      Memory::releaseMappedMemory(m_memoryBlock);
   }
   void *getBase() const
   {
      return m_memoryBlock.getBase();
   }

   size_t getSize() const
   {
      return m_memoryBlock.getSize();
   }

   MemoryBlock getMemoryBlock() const
   {
      return m_memoryBlock;
   }
private:
   MemoryBlock m_memoryBlock;
};

} // sys
} // polar

#endif // POLARPHP_UTILS_MEMORY_H
