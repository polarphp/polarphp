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

#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>

namespace polar::parser {

using llvm::PrettyStackTraceString;
using llvm::SMLoc;

void SourceManager::verifyAllBuffers() const
{
   PrettyStackTraceString backtrace{
      "Checking that all source buffers are still valid"
   };

   // FIXME: This depends on the buffer IDs chosen by llvm::SourceMgr.
   LLVM_ATTRIBUTE_USED static char arbitraryTotal = 0;
   for (unsigned i = 1, e = m_sourceMgr.getNumBuffers(); i <= e; ++i) {
      auto *buffer = m_sourceMgr.getMemoryBuffer(i);
      if (buffer->getBufferSize() == 0) {
         continue;
      }
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
SourceManager::addNewSourceBuffer(std::unique_ptr<MemoryBuffer> buffer)
{
   assert(buffer);
   StringRef bufIdentifier = buffer->getBufferIdentifier();
   auto id = m_sourceMgr.AddNewSourceBuffer(std::move(buffer), SMLoc());
   m_bufIdentIdMap[bufIdentifier] = id;
   return id;
}

unsigned SourceManager::addMemBufferCopy(MemoryBuffer *buffer)
{
   return addMemBufferCopy(buffer->getBuffer(), buffer->getBufferIdentifier());
}

unsigned SourceManager::addMemBufferCopy(StringRef inputData,
                                         StringRef bufIdentifier)
{
   auto buffer = std::unique_ptr<MemoryBuffer>(
            MemoryBuffer::getMemBufferCopy(inputData, bufIdentifier));
   return addNewSourceBuffer(std::move(buffer));
}

bool SourceManager::openVirtualFile(SourceLoc loc, StringRef name,
                                    int lineOffset)
{
   CharSourceRange fullRange = getRangeForBuffer(findBufferContainingLoc(loc));
   SourceLoc end;

   auto nextRangeIter = m_virtualFiles.upper_bound(loc.m_loc.getPointer());
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
   m_virtualFiles[end.m_loc.getPointer()] = { range, name, lineOffset };
   m_cachedVFile = {nullptr, nullptr};
   return true;
}

void SourceManager::closeVirtualFile(SourceLoc end)
{
   auto *virtualFile = const_cast<VirtualFile *>(getVirtualFile(end));
   if (!virtualFile) {
#ifndef NDEBUG
      unsigned bufferID = findBufferContainingLoc(end);
      CharSourceRange fullRange = getRangeForBuffer(bufferID);
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
   m_virtualFiles[end.m_loc.getPointer()] = std::move(*virtualFile);
   bool existed = m_virtualFiles.erase(oldRange.getEnd().m_loc.getPointer());
   assert(existed);
   (void)existed;
}

const SourceManager::VirtualFile *
SourceManager::getVirtualFile(SourceLoc loc) const
{
   const char *p = loc.m_loc.getPointer();
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


std::optional<unsigned> SourceManager::getIDForBufferIdentifier(
      StringRef bufIdentifier)
{
   auto iter = m_bufIdentIdMap.find(bufIdentifier);
   if (iter == m_bufIdentIdMap.end()) {
      return std::nullopt;
   }
   return iter->second;
}

StringRef SourceManager::getIdentifierForBuffer(unsigned bufferID) const
{
   auto *buffer = m_sourceMgr.getMemoryBuffer(bufferID);
   assert(buffer && "invalid buffer id");
   return buffer->getBufferIdentifier();
}

CharSourceRange SourceManager::getRangeForBuffer(unsigned bufferID) const
{
   auto *buffer = m_sourceMgr.getMemoryBuffer(bufferID);
   SourceLoc start{SMLoc::getFromPointer(buffer->getBufferStart())};
   return CharSourceRange(start, buffer->getBufferSize());
}

unsigned SourceManager::getLocOffsetInBuffer(SourceLoc loc,
                                             unsigned bufferID) const
{
   assert(loc.isValid() && "location should be valid");
   auto *buffer = m_sourceMgr.getMemoryBuffer(bufferID);
   assert(loc.m_loc.getPointer() >= buffer->getBuffer().begin() &&
          loc.m_loc.getPointer() <= buffer->getBuffer().end() &&
          "Location is not from the specified buffer");
   return loc.m_loc.getPointer() - buffer->getBuffer().begin();
}

unsigned SourceManager::getByteDistance(SourceLoc start, SourceLoc end) const
{
   assert(start.isValid() && "start location should be valid");
   assert(end.isValid() && "end location should be valid");
#ifndef NDEBUG
   unsigned bufferID = findBufferContainingLoc(start);
   auto *buffer = m_sourceMgr.getMemoryBuffer(bufferID);
   assert(end.m_loc.getPointer() >= buffer->getBuffer().begin() &&
          end.m_loc.getPointer() <= buffer->getBuffer().end() &&
          "end location is not from the same buffer");
#endif
   // When we have a rope buffer, could be implemented in terms of
   // getLocOffsetInBuffer().
   return end.m_loc.getPointer() - start.m_loc.getPointer();
}

StringRef SourceManager::getEntireTextForBuffer(unsigned bufferID) const
{
   return m_sourceMgr.getMemoryBuffer(bufferID)->getBuffer();
}

StringRef SourceManager::extractText(CharSourceRange range,
                                     std::optional<unsigned> bufferID) const
{
   assert(range.isValid() && "range should be valid");

   if (!bufferID) {
      bufferID = findBufferContainingLoc(range.getStart());
   }
   StringRef buffer = m_sourceMgr.getMemoryBuffer(*bufferID)->getBuffer();
   return buffer.substr(getLocOffsetInBuffer(range.getStart(), *bufferID),
                        range.getByteLength());
}

unsigned SourceManager::findBufferContainingLoc(SourceLoc loc) const
{
   assert(loc.isValid());
   // Search the buffers back-to front, so later alias buffers are
   // visited first.
   auto less_equal = std::less_equal<const char *>();
   for (unsigned i = m_sourceMgr.getNumBuffers(), e = 1; i >= e; --i) {
      auto buffer = m_sourceMgr.getMemoryBuffer(i);
      if (less_equal(buffer->getBufferStart(), loc.m_loc.getPointer()) &&
          // Use <= here so that a pointer to the null at the end of the buffer
          // is included as part of the buffer.
          less_equal(loc.m_loc.getPointer(), buffer->getBufferEnd()))
         return i;
   }
   llvm_unreachable("no buffer containing location found");
}

std::optional<unsigned> SourceManager::resolveFromLineCol(unsigned bufferId,
                                                           unsigned line,
                                                           unsigned col) const
{
   if (line == 0 || col == 0) {
      return std::nullopt;
   }
   auto inputBuf = getBasicSourceMgr().getMemoryBuffer(bufferId);
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
      return std::nullopt;
   }
   ptr = lineStart;

   // The <= here is to allow for non-inclusive range end positions at EOF
   for (; ptr <= end; ++ptr) {
      --col;
      if (col == 0) {
         return ptr - inputBuf->getBufferStart();
      }
      if (*ptr == '\n') {
         break;
      }
   }
   return std::nullopt;
}

} // polar::parser
