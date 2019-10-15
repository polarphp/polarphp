//===--- SourceManager.h - Manager for Source Buffers -----------*- C++ -*-===//
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

#ifndef POLARPHP_PARSER_SOURCE_MGR_H
#define POLARPHP_PARSER_SOURCE_MGR_H

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/VirtualFileSystem.h"
#include "polarphp/utils/SourceMgr.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/parser/SourceLoc.h"
#include <map>

namespace polar::parser {

using BasicSourceMgr = polar::utils::SourceMgr;
using polar::basic::IntrusiveRefCountPtr;
using polar::basic::DenseMap;
using polar::basic::ArrayRef;
using polar::basic::Twine;
using polar::utils::MemoryBuffer;
using polar::utils::SMDiagnostic;
using polar::utils::SMFixIt;
using polar::utils::SMRange;

/// This class manages and owns source buffers.
class SourceManager
{
   // \c #sourceLocation directive handling.
   struct VirtualFile
   {
      CharSourceRange range;
      std::string name;
      int lineOffset;
   };
   std::map<const char *, VirtualFile> m_virtualFiles;
   mutable std::pair<const char *, const VirtualFile*> m_cachedVFile = {nullptr, nullptr};

public:
   SourceManager(IntrusiveRefCountPtr<polar::vfs::FileSystem> fs =
         polar::vfs::get_real_file_system())
      : m_filesystem(fs)
   {}

   BasicSourceMgr &getBasicSourceMgr()
   {
      return m_sourceMgr;
   }

   const BasicSourceMgr &getBasicSourceMgr() const
   {
      return m_sourceMgr;
   }

   void setFileSystem(IntrusiveRefCountPtr<polar::vfs::FileSystem> fs)
   {
      m_filesystem = fs;
   }

   IntrusiveRefCountPtr<polar::vfs::FileSystem> getFileSystem()
   {
      return m_filesystem;
   }

   void setCodeCompletionPoint(unsigned bufferID, unsigned offset)
   {
      assert(bufferID != 0U && "Buffer should be valid");
      m_codeCompletionBufferID = bufferID;
      m_codeCompletionOffset = offset;
   }

   unsigned getCodeCompletionBufferID() const
   {
      return m_codeCompletionBufferID;
   }

   unsigned getCodeCompletionOffset() const
   {
      return m_codeCompletionOffset;
   }

   SourceLoc getCodeCompletionLoc() const;

   /// Returns true if \c LHS is before \c RHS in the source buffer.
   bool isBeforeInBuffer(SourceLoc lhs, SourceLoc rhs) const
   {
      return lhs.m_loc.getPointer() < rhs.m_loc.getPointer();
   }

   /// Returns true if range \c R contains the location \c loc.  The location
   /// \c loc should point at the beginning of the token.
   bool rangeContainsTokenLoc(SourceRange range, SourceLoc loc) const
   {
      return loc == range.m_start || loc == range.m_end ||
            (isBeforeInBuffer(range.m_start, loc) && isBeforeInBuffer(loc, range.m_end));
   }

   /// Returns true if range \c enclosing contains the range \c inner.
   bool rangeContains(SourceRange enclosing, SourceRange inner) const
   {
      return rangeContainsTokenLoc(enclosing, inner.m_start) &&
            rangeContainsTokenLoc(enclosing, inner.m_end);
   }

   /// Returns the buffer ID for the specified *valid* location.
   ///
   /// Because a valid source location always corresponds to a source buffer,
   /// this routine always returns a valid buffer ID.
   unsigned findBufferContainingLoc(SourceLoc loc) const;

   /// Adds a memory buffer to the SourceManager, taking ownership of it.
   unsigned addNewSourceBuffer(std::unique_ptr<MemoryBuffer> buffer);

   /// Add a \c #sourceLocation-defined virtual file region.
   ///
   /// By default, this region continues to the end of the buffer.
   ///
   /// \returns True if the new file was added, false if the file already exists.
   /// The name and line offset must match exactly in that case.
   ///
   /// \sa closeVirtualFile
   bool openVirtualFile(SourceLoc loc, StringRef name, int lineOffset);

   /// Close a \c #sourceLocation-defined virtual file region.
   void closeVirtualFile(SourceLoc end);

   /// Creates a copy of a \c MemoryBuffer and adds it to the \c SourceManager,
   /// taking ownership of the copy.
   unsigned addMemBufferCopy(MemoryBuffer *buffer);

   /// Creates and adds a memory buffer to the \c SourceManager, taking
   /// ownership of the newly created copy.
   ///
   /// \p inputData and \p bufIdentifier are copied, so that this memory can go
   /// away as soon as this function returns.
   unsigned addMemBufferCopy(StringRef inputData, StringRef bufIdentifier = "");

   /// Returns a buffer ID for a previously added buffer with the given
   /// buffer identifier, or None if there is no such buffer.
   std::optional<unsigned> getIDForBufferIdentifier(StringRef bufIdentifier);

   /// Returns the identifier for the buffer with the given ID.
   ///
   /// \p bufferID must be a valid buffer ID.
   ///
   /// This should not be used for displaying information about the \e contents
   /// of a buffer, since lines within the buffer may be marked as coming from
   /// other files using \c #sourceLocation. Use #getDisplayNameForLoc instead
   /// in that case.
   StringRef getIdentifierForBuffer(unsigned bufferID) const;

   /// Returns a SourceRange covering the entire specified buffer.
   ///
   /// Note that the start location might not point at the first token: it
   /// might point at whitespace or a comment.
   CharSourceRange getRangeForBuffer(unsigned bufferID) const;

   /// Returns the SourceLoc for the beginning of the specified buffer
   /// (at offset zero).
   ///
   /// Note that the resulting location might not point at the first token: it
   /// might point at whitespace or a comment.
   SourceLoc getLocForBufferStart(unsigned bufferID) const
   {
      return getRangeForBuffer(bufferID).getStart();
   }

   /// Returns the offset in bytes for the given valid source location.
   unsigned getLocOffsetInBuffer(SourceLoc loc, unsigned bufferID) const;

   /// Returns the distance in bytes between the given valid source
   /// locations.
   unsigned getByteDistance(SourceLoc start, SourceLoc end) const;

   /// Returns the SourceLoc for the byte offset in the specified buffer.
   SourceLoc getLocForOffset(unsigned bufferID, unsigned offset) const
   {
      return getLocForBufferStart(bufferID).getAdvancedLoc(offset);
   }

   /// Returns a buffer identifier suitable for display to the user containing
   /// the given source location.
   ///
   /// This respects \c #sourceLocation directives and the 'use-external-names'
   /// directive in VFS overlay files. If you need an on-disk file name, use
   /// #getIdentifierForBuffer instead.
   StringRef getDisplayNameForLoc(SourceLoc loc) const;

   /// Returns the line and column represented by the given source location.
   ///
   /// If \p bufferID is provided, \p loc must come from that source buffer.
   ///
   /// This respects \c #sourceLocation directives.
   std::pair<unsigned, unsigned>
   getLineAndColumn(SourceLoc loc, unsigned bufferID = 0) const
   {
      assert(loc.isValid());
      int lineOffset = getLineOffset(loc);
      int l, c;
      std::tie(l, c) = m_sourceMgr.getLineAndColumn(loc.m_loc, bufferID);
      assert(lineOffset+l > 0 && "bogus line offset");
      return { lineOffset + l, c };
   }

   /// Returns the real line number for a source location.
   ///
   /// If \p bufferID is provided, \p loc must come from that source buffer.
   ///
   /// This does not respect \c #sourceLocation directives.
   unsigned getLineNumber(SourceLoc loc, unsigned bufferID = 0) const
   {
      assert(loc.isValid());
      return m_sourceMgr.findLineNumber(loc.m_loc, bufferID);
   }

   StringRef getEntireTextForBuffer(unsigned bufferID) const;

   StringRef extractText(CharSourceRange range,
                         std::optional<unsigned> bufferID = std::nullopt) const;

   SMDiagnostic getMessage(SourceLoc loc, BasicSourceMgr::DiagKind kind,
                           const Twine &Msg,
                           ArrayRef<SMRange> ranges,
                           ArrayRef<SMFixIt> fixIts) const;

   /// Verifies that all buffers are still valid.
   void verifyAllBuffers() const;

   /// Translate line and column pair to the offset.
   std::optional<unsigned> resolveFromLineCol(unsigned bufferId, unsigned line,
                                              unsigned col) const;

   SourceLoc getLocForLineCol(unsigned bufferId, unsigned line, unsigned col) const
   {
      auto offset = resolveFromLineCol(bufferId, line, col);
      return offset.has_value() ? getLocForOffset(bufferId, offset.value()) :
                                 SourceLoc();
   }

private:
   const VirtualFile *getVirtualFile(SourceLoc loc) const;

   int getLineOffset(SourceLoc loc) const
   {
      if (auto vfile = getVirtualFile(loc)) {
         return vfile->lineOffset;
      } else {
         return 0;
      }
   }

private:
   BasicSourceMgr m_sourceMgr;
   IntrusiveRefCountPtr<polar::vfs::FileSystem> m_filesystem;
   unsigned m_codeCompletionBufferID = 0U;
   unsigned m_codeCompletionOffset;

   /// Associates buffer identifiers to buffer IDs.
   DenseMap<StringRef, unsigned> m_bufIdentIDMap;

   /// A cache mapping buffer identifiers to vfs Status entries.
   ///
   /// This is as much a hack to prolong the lifetime of status objects as it is
   /// to speed up stats.
   mutable DenseMap<StringRef, polar::vfs::Status> m_statusCache;
};

} // polar::parser

#endif // POLARPHP_PARSER_SOURCE_MGR_H
