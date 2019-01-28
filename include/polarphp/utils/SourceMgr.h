// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/27.

#ifndef POLARPHP_UTILS_SOURCE_MGR_H
#define POLARPHP_UTILS_SOURCE_MGR_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/SourceLocation.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace polar {
namespace utils {

class RawOutStream;
class SMDiagnostic;
class SMFixIt;

using polar::basic::PointerUnion4;
using polar::basic::StringRef;
using polar::basic::ArrayRef;
using polar::basic::SmallVector;
using polar::basic::Twine;

/// This owns the files read by a parser, handles include stacks,
/// and handles diagnostic wrangling.
class SourceMgr
{
public:
   enum DiagKind
   {
      DK_Error,
      DK_Warning,
      DK_Remark,
      DK_Note,
   };

   /// Clients that want to handle their own diagnostics in a custom way can
   /// register a function pointer+context as a diagnostic handler.
   /// It gets called each time printMessage is invoked.
   using DiagHandlerTy = void (*)(const SMDiagnostic &, void *context);

private:
   struct SrcBuffer
   {
      /// The memory buffer for the file.
      std::unique_ptr<MemoryBuffer> m_buffer;

      /// Helper type for OffsetCache below: since we're storing many offsets
      /// into relatively small files (often smaller than 2^8 or 2^16 bytes),
      /// we select the offset vector element type dynamically based on the
      /// size of Buffer.
      using VariableSizeOffsets = PointerUnion4<std::vector<uint8_t> *,
      std::vector<uint16_t> *,
      std::vector<uint32_t> *,
      std::vector<uint64_t> *>;

      /// Vector of offsets into Buffer at which there are line-endings
      /// (lazily populated). Once populated, the '\n' that marks the end of
      /// line number N from [1..] is at Buffer[OffsetCache[N-1]]. Since
      /// these offsets are in sorted (ascending) order, they can be
      /// binary-searched for the first one after any given offset (eg. an
      /// offset corresponding to a particular SMLoc).
      mutable VariableSizeOffsets m_offsetCache;

      /// Populate \c OffsetCache and look up a given \p Ptr in it, assuming
      /// it points somewhere into \c Buffer. The static type parameter \p T
      /// must be an unsigned integer type from uint{8,16,32,64}_t large
      /// enough to store offsets inside \c Buffer.
      template<typename T>
      unsigned getLineNumber(const char *ptr) const;
      /// This is the location of the parent include, or null if at the top level.
      SMLocation m_includeLoc;
      SrcBuffer() = default;
      SrcBuffer(SrcBuffer &&);
      SrcBuffer(const SrcBuffer &) = delete;
      SrcBuffer &operator=(const SrcBuffer &) = delete;
      ~SrcBuffer();
   };

   /// This is all of the buffers that we are reading from.
   std::vector<SrcBuffer> m_buffers;

   // This is the list of directories we should search for include files in.
   std::vector<std::string> m_includeDirectories;

   DiagHandlerTy m_diagHandler = nullptr;
   void *m_diagContext = nullptr;

   bool isValidBufferID(unsigned i) const
   {
      return i && i <= m_buffers.size();
   }

public:
   SourceMgr() = default;
   SourceMgr(const SourceMgr &) = delete;
   SourceMgr &operator=(const SourceMgr &) = delete;
   ~SourceMgr() = default;

   void setIncludeDirs(const std::vector<std::string> &dirs)
   {
      m_includeDirectories = dirs;
   }

   /// Specify a diagnostic handler to be invoked every time printMessage is
   /// called. \p Ctx is passed into the handler when it is invoked.
   void setDiagHandler(DiagHandlerTy handler, void *context = nullptr)
   {
      m_diagHandler = handler;
      m_diagContext = context;
   }

   DiagHandlerTy getDiagHandler() const
   {
      return m_diagHandler;
   }

   void *getDiagContext() const
   {
      return m_diagContext;
   }

   const SrcBuffer &getBufferInfo(unsigned i) const
   {
      assert(isValidBufferID(i));
      return m_buffers[i - 1];
   }

   const MemoryBuffer *getMemoryBuffer(unsigned i) const
   {
      assert(isValidBufferID(i));
      return m_buffers[i - 1].m_buffer.get();
   }

   unsigned getNumBuffers() const
   {
      return m_buffers.size();
   }

   unsigned getMainFileID() const
   {
      assert(getNumBuffers());
      return 1;
   }

   SMLocation getParentIncludeLoc(unsigned i) const
   {
      assert(isValidBufferID(i));
      return m_buffers[i - 1].m_includeLoc;
   }

   /// Add a new source buffer to this source manager. This takes ownership of
   /// the memory buffer.
   unsigned addNewSourceBuffer(std::unique_ptr<MemoryBuffer> buffer,
                               SMLocation includeLoc)
   {
      SrcBuffer sbuffer;
      sbuffer.m_buffer = std::move(buffer);
      sbuffer.m_includeLoc = includeLoc;
      m_buffers.push_back(std::move(sbuffer));
      return m_buffers.size();
   }

   /// Search for a file with the specified name in the current directory or in
   /// one of the IncludeDirs.
   ///
   /// If no file is found, this returns 0, otherwise it returns the buffer ID
   /// of the stacked file. The full path to the included file can be found in
   /// \p IncludedFile.
   unsigned addIncludeFile(const std::string &filename, SMLocation includeLoc,
                           std::string &includedFile);

   /// Return the ID of the buffer containing the specified location.
   ///
   /// 0 is returned if the buffer is not found.
   unsigned findBufferContainingLoc(SMLocation location) const;

   /// Find the line number for the specified location in the specified file.
   /// This is not a fast method.
   unsigned findLineNumber(SMLocation location, unsigned bufferID = 0) const
   {
      return getLineAndColumn(location, bufferID).first;
   }

   /// Find the line and column number for the specified location in the
   /// specified file. This is not a fast method.
   std::pair<unsigned, unsigned> getLineAndColumn(SMLocation location,
                                                  unsigned bufferID = 0) const;

   /// Emit a message about the specified location with the specified string.
   ///
   /// \param ShowColors Display colored messages if output is a terminal and
   /// the default error handler is used.
   void printMessage(RawOutStream &outStream, SMLocation location, DiagKind kind,
                     const Twine &msg,
                     ArrayRef<SMRange> ranges = std::nullopt,
                     ArrayRef<SMFixIt> fixIts = std::nullopt,
                     bool showColors = true) const;

   /// Emits a diagnostic to polar::error_stream().
   void printMessage(SMLocation location, DiagKind kind, const Twine &msg,
                     ArrayRef<SMRange> ranges = std::nullopt,
                     ArrayRef<SMFixIt> fixIts = std::nullopt,
                     bool showColors = true) const;

   /// Emits a manually-constructed diagnostic to the given output stream.
   ///
   /// \param ShowColors Display colored messages if output is a terminal and
   /// the default error handler is used.
   void printMessage(RawOutStream &outStream, const SMDiagnostic &diagnostic,
                     bool showColors = true) const;

   /// Return an SMDiagnostic at the specified location with the specified
   /// string.
   ///
   /// \param Msg If non-null, the kind of message (e.g., "error") which is
   /// prefixed to the message.
   SMDiagnostic getMessage(SMLocation location, DiagKind lind, const Twine &msg,
                           ArrayRef<SMRange> ranges = std::nullopt,
                           ArrayRef<SMFixIt> fixIts = std::nullopt) const;

   /// Prints the names of included files and the line of the file they were
   /// included from. A diagnostic handler can use this before printing its
   /// custom formatted message.
   ///
   /// \param m_includeLoc The location of the include.
   /// \param OS the RawOutStream to print on.
   void printIncludeStack(SMLocation includeLoc, RawOutStream &outStream) const;
};

/// Represents a single fixit, a replacement of one range of text with another.
class SMFixIt
{
   SMRange m_range;
   std::string m_text;

public:
   // FIXME: Twine.str() is not very efficient.
   SMFixIt(SMLocation location, const Twine &insertion)
      : m_range(location, location),
        m_text(insertion.getStr())
   {
      assert(location.isValid());
   }

   // FIXME: Twine.str() is not very efficient.
   SMFixIt(SMRange range, const Twine &replacement)
      : m_range(range),
        m_text(replacement.getStr())
   {
      assert(range.isValid());
   }

   StringRef getText() const
   {
      return m_text;
   }

   SMRange getRange() const
   {
      return m_range;
   }

   bool operator<(const SMFixIt &other) const
   {
      if (m_range.m_start.getPointer() != other.m_range.m_start.getPointer()) {
         return m_range.m_start.getPointer() < other.m_range.m_start.getPointer();
      }
      if (m_range.m_end.getPointer() != other.m_range.m_end.getPointer()) {
         return m_range.m_end.getPointer() < other.m_range.m_end.getPointer();
      }
      return m_text < other.m_text;
   }
};

/// Instances of this class encapsulate one diagnostic report, allowing
/// printing to a RawOutStream as a caret diagnostic.
class SMDiagnostic
{
   const SourceMgr *m_sourceMgr = nullptr;
   SMLocation m_location;
   std::string m_filename;
   int m_lineNo = 0;
   int m_columnNo = 0;
   SourceMgr::DiagKind m_kind = SourceMgr::DK_Error;
   std::string m_message, m_lineContents;
   std::vector<std::pair<unsigned, unsigned>> m_ranges;
   SmallVector<SMFixIt, 4> m_fixIts;

public:
   // Null diagnostic.
   SMDiagnostic() = default;
   // Diagnostic with no location (e.g. file not found, command line arg error).
   SMDiagnostic(StringRef filename, SourceMgr::DiagKind kind, StringRef msg)
      : m_filename(filename), m_lineNo(-1), m_columnNo(-1), m_kind(kind), m_message(msg)
   {}

   // Diagnostic with a location.
   SMDiagnostic(const SourceMgr &sm, SMLocation location, StringRef funcName,
                int line, int column, SourceMgr::DiagKind kind,
                StringRef msg, StringRef lineStr,
                ArrayRef<std::pair<unsigned,unsigned>> ranges,
                ArrayRef<SMFixIt> fixIts = std::nullopt);

   const SourceMgr *getSourceMgr() const
   {
      return m_sourceMgr;
   }

   SMLocation getLocation() const
   {
      return m_location;
   }

   StringRef getFilename() const
   {
      return m_filename;
   }

   int getLineNo() const
   {
      return m_lineNo;
   }

   int getColumnNo() const
   {
      return m_columnNo;
   }

   SourceMgr::DiagKind getKind() const
   {
      return m_kind;
   }

   StringRef getMessage() const
   {
      return m_message;
   }

   StringRef getLineContents() const
   {
      return m_lineContents;
   }

   ArrayRef<std::pair<unsigned, unsigned>> getRanges() const
   {
      return m_ranges;
   }

   void addFixIt(const SMFixIt &hint)
   {
      m_fixIts.push_back(hint);
   }

   ArrayRef<SMFixIt> getFixIts() const
   {
      return m_fixIts;
   }

   void print(const char *progName, RawOutStream &outStream, bool showColors = true,
              bool showKindLabel = true) const;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_SOURCE_MGR_H
