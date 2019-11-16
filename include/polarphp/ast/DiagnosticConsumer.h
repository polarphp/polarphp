//===--- DiagnosticConsumer.h - Diagnostic Consumer Interface ---*- C++ -*-===//
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
//
//===----------------------------------------------------------------------===//
//
//  This file declares the DiagnosticConsumer class, which receives callbacks
//  whenever the front end emits a diagnostic and is responsible for presenting
//  or storing that diagnostic (whatever is appropriate).
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DIAGNOSTIC_CONSUMER_H
#define POLARPHP_AST_DIAGNOSTIC_CONSUMER_H

#include "polarphp/parser/SourceLoc.h"
#include "llvm/Support/SourceMgr.h"

/// forward declare class with namespace
namespace polar::parser {
class SourceManager;
}

namespace polar::ast {

using llvm::ArrayRef;
using llvm::StringRef;
using llvm::SmallVectorImpl;
using llvm::SmallVector;
using polar::parser::CharSourceRange;
using polar::parser::SourceLoc;
using polar::parser::SourceManager;
using llvm::SMLoc;
using llvm::SMFixIt;
using llvm::SMRange;

/// forward declare class
class DiagnosticArgument;
class DiagnosticEngine;

enum class DiagID : uint32_t;

/// Describes the kind of diagnostic.
///
enum class DiagnosticKind : uint8_t
{
   Error,
   Warning,
   Remark,
   Note
};

/// Extra information carried along with a diagnostic, which may or
/// may not be of interest to a given diagnostic consumer.
struct DiagnosticInfo
{
   DiagID id = DiagID(0);
   /// Represents a fix-it, a replacement of one range of text with another.
   class FixIt
   {
   public:
      FixIt(CharSourceRange range, StringRef str)
         : m_range(range),
           m_text(str)
      {}

      CharSourceRange getRange() const
      {
         return m_range;
      }

      StringRef getText() const
      {
         return m_text;
      }
   private:
      CharSourceRange m_range;
      std::string m_text;
   };

   /// Extra source ranges that are attached to the diagnostic.
   ArrayRef<CharSourceRange> ranges;
   /// Extra source ranges that are attached to the diagnostic.
   ArrayRef<FixIt> fixIts;
};

/// Abstract interface for classes that present diagnostics to the user.
class DiagnosticConsumer
{
protected:
   static SMLoc getRawLoc(SourceLoc loc);

   static SMRange getRawRange(SourceManager &, CharSourceRange range)
   {
      return SMRange(getRawLoc(range.getStart()), getRawLoc(range.getEnd()));
   }

   static SMFixIt getRawFixIt(SourceManager &sourceMgr, DiagnosticInfo::FixIt fixIt)
   {
      // FIXME: It's unfortunate that we have to copy the replacement text.
      return SMFixIt(getRawRange(sourceMgr, fixIt.getRange()), fixIt.getText());
   }

public:
   virtual ~DiagnosticConsumer();

   /// Invoked whenever the frontend emits a diagnostic.
   ///
   /// \param sourceMgr The source manager associated with the source locations in
   /// this diagnostic.
   ///
   /// \param loc The source location associated with this diagnostic. This
   /// location may be invalid, if the diagnostic is not directly related to
   /// the source (e.g., if it comes from command-line parsing).
   ///
   /// \param kind The severity of the diagnostic (error, warning, note).
   ///
   /// \param formatArgs The diagnostic format string arguments.
   ///
   /// \param info Extra information associated with the diagnostic.
   virtual void handleDiagnostic(SourceManager &sourceMgr, SourceLoc loc,
                                 DiagnosticKind kind,
                                 StringRef formatString,
                                 ArrayRef<DiagnosticArgument> formatArgs,
                                 const DiagnosticInfo &info) = 0;

   /// \returns true if an error occurred while finishing-up.
   virtual bool finishProcessing()
   {
      return false;
   }

   /// In batch mode, any error causes failure for all primary files, but
   /// anyone consulting .dia files will only see an error for a particular
   /// primary in that primary's serialized diagnostics file. For other
   /// primaries' serialized diagnostics files, do something to signal the driver
   /// what happened. This is only meaningful for SerializedDiagnosticConsumers,
   /// so here's a placeholder.

   virtual void informDriverOfIncompleteBatchModeCompilation()
   {}
};

/// DiagnosticConsumer that discards all diagnostics.
class NullDiagnosticConsumer : public DiagnosticConsumer
{
public:
   void handleDiagnostic(SourceManager &sourceMgr, SourceLoc loc,
                         DiagnosticKind kind,
                         StringRef formatString,
                         ArrayRef<DiagnosticArgument> formatArgs,
                         const DiagnosticInfo &info) override;
};

/// DiagnosticConsumer that forwards diagnostics to the consumers of
// another DiagnosticEngine.
class ForwardingDiagnosticConsumer : public DiagnosticConsumer
{
public:
   ForwardingDiagnosticConsumer(DiagnosticEngine &Target);
   void handleDiagnostic(SourceManager &sourceMgr, SourceLoc loc,
                         DiagnosticKind kind,
                         StringRef formatString,
                         ArrayRef<DiagnosticArgument> formatArgs,
                         const DiagnosticInfo &info) override;
private:
   DiagnosticEngine &m_targetEngine;
};

/// DiagnosticConsumer that funnels diagnostics in certain files to
/// particular sub-consumers.
///
/// The intended use case for such a consumer is "batch mode" compilations,
/// where we want to record diagnostics for each file as if they were compiled
/// separately. This is important for incremental builds, so that if a file has
/// warnings but doesn't get recompiled in the next build, the warnings persist.
///
/// Diagnostics that are not in one of the special files are emitted into every
/// sub-consumer. This is necessary to deal with, for example, diagnostics in a
/// bridging header imported from Objective-C, which isn't really about the
/// current file.
class FileSpecificDiagnosticConsumer : public DiagnosticConsumer
{
public:
   class Subconsumer;

   /// Given a vector of subconsumers, return the most specific
   /// DiagnosticConsumer for that vector. That will be a
   /// FileSpecificDiagnosticConsumer if the vector has > 1 subconsumer, the
   /// subconsumer itself if the vector has just one, or a null pointer if there
   /// are no subconsumers. Takes ownership of the DiagnosticConsumers specified
   /// in \p subconsumers.
   static std::unique_ptr<DiagnosticConsumer>
   consolidateSubconsumers(SmallVectorImpl<Subconsumer> &subconsumers);

   /// A diagnostic consumer, along with the name of the buffer that it should
   /// be associated with.
   class Subconsumer
   {
   public:
      std::string getInputFileName() const
      {
         return m_inputFileName;
      }

      DiagnosticConsumer *getConsumer() const
      {
         return m_consumer.get();
      }

      Subconsumer(std::string inputFileName,
                  std::unique_ptr<DiagnosticConsumer> consumer)
         : m_inputFileName(inputFileName),
           m_consumer(std::move(consumer))
      {}

      void handleDiagnostic(SourceManager &sourceMgr, SourceLoc loc,
                            DiagnosticKind kind,
                            StringRef formatString,
                            ArrayRef<DiagnosticArgument> formatArgs,
                            const DiagnosticInfo &info)
      {
         if (!getConsumer()) {
            return;
         }
         m_hasAnErrorBeenConsumed |= kind == DiagnosticKind::Error;
         getConsumer()->handleDiagnostic(sourceMgr, loc, kind, formatString, formatArgs,
                                         info);
      }

      void informDriverOfIncompleteBatchModeCompilation()
      {
         if (!m_hasAnErrorBeenConsumed && getConsumer()) {
            getConsumer()->informDriverOfIncompleteBatchModeCompilation();
         }
      }

   private:
      friend std::unique_ptr<DiagnosticConsumer>
      FileSpecificDiagnosticConsumer::consolidateSubconsumers(
            SmallVectorImpl<Subconsumer> &subconsumers);

   private:
      /// The name of the input file that a consumer and diagnostics should
      /// be associated with. An empty string means that a consumer is not
      /// associated with any particular buffer, and should only receive
      /// diagnostics that are not in any of the other consumers' files.
      std::string m_inputFileName;

      /// The consumer (if any) for diagnostics associated with the inputFileName.
      /// A null pointer for the DiagnosticConsumer means that diagnostics for
      /// this file should not be emitted.
      std::unique_ptr<DiagnosticConsumer> m_consumer;

      // Has this subconsumer ever handled a diagnostic that is an error?
      bool m_hasAnErrorBeenConsumed = false;
   };

   class ConsumerAndRange
   {
   private:
      /// The range of SourceLoc's for which diagnostics should be directed to
      /// this subconsumer.
      /// Should be const but then the sort won't compile.
      /*const*/ CharSourceRange m_range;

      /// Index into m_subconsumers vector for this subconsumer.
      /// Should be const but then the sort won't compile.
      /*const*/ unsigned m_subconsumerIndex;

   public:
      unsigned getSubconsumerIndex() const
      {
         return m_subconsumerIndex;
      }

      ConsumerAndRange(const CharSourceRange range, unsigned subconsumerIndex)
         : m_range(range),
           m_subconsumerIndex(subconsumerIndex)
      {}

      /// Compare according to range:
      bool operator<(const ConsumerAndRange &right) const
      {
         auto compare = std::less<const char *>();
         return compare(getRawLoc(m_range.getEnd()).getPointer(),
                        getRawLoc(right.m_range.getEnd()).getPointer());
      }

      /// Overlaps by range:
      bool overlaps(const ConsumerAndRange &other) const
      {
         return m_range.overlaps(other.m_range);
      }

      /// Does my range end after \p loc?
      bool endsAfter(const SourceLoc loc) const
      {
         auto compare = std::less<const char *>();
         return compare(getRawLoc(m_range.getEnd()).getPointer(),
                        getRawLoc(loc).getPointer());
      }

      bool contains(const SourceLoc loc) const
      {
         return m_range.contains(loc);
      }
   };

public:
   void handleDiagnostic(SourceManager &sourceMgr, SourceLoc loc,
                         DiagnosticKind kind,
                         StringRef formatString,
                         ArrayRef<DiagnosticArgument> formatArgs,
                         const DiagnosticInfo &info) override;

   bool finishProcessing() override;

private:
   /// Takes ownership of the DiagnosticConsumers specified in \p consumers.
   ///
   /// There must not be two consumers for the same file (i.e., having the same
   /// buffer name).
   explicit FileSpecificDiagnosticConsumer(
         SmallVectorImpl<Subconsumer> &consumers);

   /// In batch mode, any error causes failure for all primary files, but
   /// Xcode will only see an error for a particular primary in that primary's
   /// serialized diagnostics file. So, tell the subconsumers to inform the
   /// driver of incomplete batch mode compilation.
   void tellSubconsumersToInformDriverOfIncompleteBatchModeCompilation();

   void computeConsumersOrderedByRange(SourceManager &sourceMgr);

   /// Returns nullptr if diagnostic is to be suppressed,
   /// None if diagnostic is to be distributed to every consumer,
   /// a particular consumer if diagnostic goes there.
   std::optional<FileSpecificDiagnosticConsumer::Subconsumer *>
   subconsumerForLocation(SourceManager &sourceMgr, SourceLoc loc);

   Subconsumer &operator[](const ConsumerAndRange &consumerAndRange)
   {
      return m_subconsumers[consumerAndRange.getSubconsumerIndex()];
   }

private:

   /// All consumers owned by this FileSpecificDiagnosticConsumer.
   SmallVector<Subconsumer, 4> m_subconsumers;

   /// The consumers owned by this FileSpecificDiagnosticConsumer, sorted by
   /// the end locations of each file so that a lookup by position can be done
   /// using binary search.
   ///
   /// Generated and cached when the first diagnostic with a location is emitted.
   /// This allows diagnostics to be emitted before files are actually opened,
   /// as long as they don't have source locations.
   ///
   /// \see #subconsumerForLocation
   SmallVector<ConsumerAndRange, 4> m_consumersOrderedByRange;

   /// Indicates which consumer to send Note diagnostics too.
   ///
   /// Notes are always considered attached to the error, warning, or remark
   /// that was most recently emitted.
   ///
   /// If None, Note diagnostics are sent to every consumer.
   /// If null, diagnostics are suppressed.
   std::optional<Subconsumer *> m_subconsumerForSubsequentNotes = std::nullopt;

   bool m_hasAnErrorBeenConsumed = false;
};

} // polar::ast

#endif // POLARPHP_AST_DIAGNOSTIC_CONSUMER_H
