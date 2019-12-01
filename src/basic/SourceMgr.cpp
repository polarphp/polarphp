//===--- SourceMgr.cpp - SourceManager implementations --------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file defines types used to reason about source locations and ranges.
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.

#include "polarphp/basic/SourceLoc.h"
#include "polarphp/basic/SourceMgr.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/MemoryBuffer.h"

#include <optional>

namespace polar::basic {

using llvm::PrettyStackTraceString;
using llvm::SMLoc;

void SourceManager::verifyAllBuffers() const
{
   llvm::PrettyStackTraceString backtrace{
      "Checking that all source buffers are still valid"
   };

   // FIXME: This depends on the buffer IDs chosen by llvm::SourceMgr.
   LLVM_ATTRIBUTE_USED static char arbitraryTotal = 0;
   for (unsigned i = 1, e = LLVMSourceMgr.getNumBuffers(); i <= e; ++i) {
      auto *buffer = LLVMSourceMgr.getMemoryBuffer(i);
      if (buffer->getBufferSize() == 0)
         continue;
      arbitraryTotal += buffer->getBufferStart()[0];
      arbitraryTotal += buffer->getBufferEnd()[-1];
   }
}

SourceLoc SourceManager::getCodeCompletionLoc() const
{
   return getLocForBufferStart(m_codeCompletionBufferId)
         .getAdvancedLoc(m_codeCompletionOffset);
}

StringRef SourceManager::getDisplayNameForLoc(SourceLoc loc) const
{
   // Respect #line first
   if (auto vfile = getVirtualFile(loc)) {
      return vfile->name;
   }

   // Next, try the stat cache
   auto ident = getIdentifierForBuffer(findBufferContainingLoc(loc));
   auto found = m_statusCache.find(ident);
   if (found != m_statusCache.end()) {
      return found->second.getName();
   }

   // Populate the cache with a (virtual) stat.
   if (auto status = m_filesystem->status(ident)) {
      return (m_statusCache[ident] = status.get()).getName();
   }

   // Finally, fall back to the buffer identifier.
   return ident;
}

unsigned
SourceManager::addNewSourceBuffer(std::unique_ptr<llvm::MemoryBuffer> buffer)
{
   assert(buffer);
   StringRef bufIdentifier = buffer->getBufferIdentifier();
   auto id = LLVMSourceMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
   m_bufIdentIdMap[bufIdentifier] = id;
   return id;
}

unsigned SourceManager::addMemBufferCopy(llvm::MemoryBuffer *buffer)
{
   return addMemBufferCopy(buffer->getBuffer(), buffer->getBufferIdentifier());
}

unsigned SourceManager::addMemBufferCopy(StringRef inputData,
                                         StringRef bufIdentifier)
{
   auto buffer = std::unique_ptr<llvm::MemoryBuffer>(
            llvm::MemoryBuffer::getMemBufferCopy(inputData, bufIdentifier));
   return addNewSourceBuffer(std::move(buffer));
}

bool SourceManager::openVirtualFile(SourceLoc loc, StringRef name,
                                    int lineOffset)
{
   CharSourceRange fullRange = getRangeForBuffer(findBufferContainingLoc(loc));
   SourceLoc end;
   auto nextRangeIter = m_virtualFiles.upper_bound(loc.m_value.getPointer());
   if (nextRangeIter != m_virtualFiles.end() &&
       fullRange.contains(nextRangeIter->second.range.getStart())) {
      const VirtualFile &existingFile = nextRangeIter->second;
      if (existingFile.range.getStart() == loc) {
         assert(existingFile.name == name);
         assert(existingFile.lineOffset == lineOffset);
         return false;
      }
      assert(!existingFile.range.contains(loc) &&
             "must close current open file first");
      end = nextRangeIter->second.range.getStart();
   } else {
      end = fullRange.getEnd();
   }

   CharSourceRange range = CharSourceRange(*this, loc, end);
   m_virtualFiles[end.m_value.getPointer()] = { range, name, lineOffset };
   m_cachedVFile = {nullptr, nullptr};
   return true;
}

void SourceManager::closeVirtualFile(SourceLoc end)
{
   auto *virtualFile = const_cast<VirtualFile *>(getVirtualFile(end));
   if (!virtualFile) {
#ifndef NDEBUG
      unsigned bufferId = findBufferContainingLoc(end);
      CharSourceRange fullRange = getRangeForBuffer(bufferId);
      assert((fullRange.getByteLength() == 0 ||
              getVirtualFile(end.getAdvancedLoc(-1))) &&
             "no open virtual file for this location");
      assert(fullRange.getEnd() == end);
#endif
      return;
   }
   m_cachedVFile = {nullptr, nullptr};

   CharSourceRange oldRange = virtualFile->range;
   virtualFile->range = CharSourceRange(*this, virtualFile->range.getStart(),
                                        end);
   m_virtualFiles[end.m_value.getPointer()] = std::move(*virtualFile);
   bool existed = m_virtualFiles.erase(oldRange.getEnd().m_value.getPointer());
   assert(existed);
   (void)existed;
}

const SourceManager::VirtualFile *
SourceManager::getVirtualFile(SourceLoc loc) const
{
   const char *p = loc.m_value.getPointer();
   if (m_cachedVFile.first == p) {
      return m_cachedVFile.second;
   }
   // Returns the first element that is >p.
   auto vfileIter = m_virtualFiles.upper_bound(p);
   if (vfileIter != m_virtualFiles.end() && vfileIter->second.range.contains(loc)) {
      m_cachedVFile = { p, &vfileIter->second };
      return m_cachedVFile.second;
   }
   return nullptr;
}


Optional<unsigned> SourceManager::getIDForBufferIdentifier(
      StringRef bufIdentifier)
{
   auto iter = m_bufIdentIdMap.find(bufIdentifier);
   if (iter == m_bufIdentIdMap.end()) {
      return None;
   }
   return iter->second;
}

StringRef SourceManager::getIdentifierForBuffer(unsigned bufferId) const
{
   auto *buffer = LLVMSourceMgr.getMemoryBuffer(bufferId);
   assert(buffer && "invalid buffer id");
   return buffer->getBufferIdentifier();
}

CharSourceRange SourceManager::getRangeForBuffer(unsigned bufferId) const
{
   auto *buffer = LLVMSourceMgr.getMemoryBuffer(bufferId);
   SourceLoc start{llvm::SMLoc::getFromPointer(buffer->getBufferStart())};
   return CharSourceRange(start, buffer->getBufferSize());
}

unsigned SourceManager::getLocOffsetInBuffer(SourceLoc loc,
                                             unsigned bufferId) const
{
   assert(loc.isValid() && "location should be valid");
   auto *buffer = LLVMSourceMgr.getMemoryBuffer(bufferId);
   assert(loc.m_value.getPointer() >= buffer->getBuffer().begin() &&
          loc.m_value.getPointer() <= buffer->getBuffer().end() &&
          "Location is not from the specified buffer");
   return loc.m_value.getPointer() - buffer->getBuffer().begin();
}

unsigned SourceManager::getByteDistance(SourceLoc start, SourceLoc end) const
{
   assert(start.isValid() && "start location should be valid");
   assert(end.isValid() && "end location should be valid");
#ifndef NDEBUG
   unsigned bufferId = findBufferContainingLoc(start);
   auto *buffer = LLVMSourceMgr.getMemoryBuffer(bufferId);
   assert(end.m_value.getPointer() >= buffer->getBuffer().begin() &&
          end.m_value.getPointer() <= buffer->getBuffer().end() &&
          "end location is not from the same buffer");
#endif
   // When we have a rope buffer, could be implemented in terms of
   // getLocOffsetInBuffer().
   return end.m_value.getPointer() - start.m_value.getPointer();
}

StringRef SourceManager::getEntireTextForBuffer(unsigned bufferId) const
{
   return LLVMSourceMgr.getMemoryBuffer(bufferId)->getBuffer();
}

StringRef SourceManager::extractText(CharSourceRange range,
                                     Optional<unsigned> bufferId) const
{
   assert(range.isValid() && "range should be valid");
   if (!bufferId) {
      bufferId = findBufferContainingLoc(range.getStart());
   }
   StringRef buffer = LLVMSourceMgr.getMemoryBuffer(*bufferId)->getBuffer();
   return buffer.substr(getLocOffsetInBuffer(range.getStart(), *bufferId),
                        range.getByteLength());
}

unsigned SourceManager::findBufferContainingLoc(SourceLoc loc) const
{
   assert(loc.isValid());
   // Search the buffers back-to front, so later alias buffers are
   // visited first.
   auto less_equal = std::less_equal<const char *>();
   for (unsigned i = LLVMSourceMgr.getNumBuffers(), e = 1; i >= e; --i) {
      auto buffer = LLVMSourceMgr.getMemoryBuffer(i);
      if (less_equal(buffer->getBufferStart(), loc.m_value.getPointer()) &&
          // Use <= here so that a pointer to the null at the end of the buffer
          // is included as part of the buffer.
          less_equal(loc.m_value.getPointer(), buffer->getBufferEnd()))
         return i;
   }
   llvm_unreachable("no buffer containing location found");
}

llvm::Optional<unsigned>
SourceManager::resolveOffsetForEndOfLine(unsigned bufferId,
                                         unsigned line) const
{
   return resolveFromLineCol(bufferId, line, ~0u);
}

llvm::Optional<unsigned> SourceManager::resolveFromLineCol(unsigned bufferId,
                                                           unsigned line,
                                                           unsigned col) const
{
   if (line == 0 || col == 0) {
      return None;
   }
   const bool lineEnd = col == ~0u;
   auto inputBuf = getLLVMSourceMgr().getMemoryBuffer(bufferId);
   const char *ptr = inputBuf->getBufferStart();
   const char *end = inputBuf->getBufferEnd();
   const char *lineStart = ptr;
   --line;
   for (; line && (ptr < end); ++ptr) {
      if (*ptr == '\n') {
         --line;
         lineStart = ptr+1;
      }
   }
   if (line != 0) {
      return None;
   }
   ptr = lineStart;
   // The <= here is to allow for non-inclusive range end positions at EOF
   for (; ; ++ptr) {
      --col;
      if (col == 0) {
         return ptr - inputBuf->getBufferStart();
      }
      if (*ptr == '\n' || ptr == end) {
         if (lineEnd) {
            return ptr - inputBuf->getBufferStart();
         } else {
            break;
         }
      }
   }
   return None;
}

} // polar::basic
