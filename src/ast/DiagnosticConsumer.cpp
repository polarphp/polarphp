//===--- DiagnosticConsumer.cpp - Diagnostic Consumer Impl ----------------===//
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
//  This file implements the DiagnosticConsumer class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "polarphp-ast"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/basic/Defer.h"

#include "polarphp/ast/DiagnosticConsumer.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/parser/SourceMgr.h"

namespace polar::ast {

using llvm::StringSet;

DiagnosticConsumer::~DiagnosticConsumer()
{}

SMLoc DiagnosticConsumer::getRawLoc(SourceLoc loc)
{
   return loc.m_loc;
}

LLVM_ATTRIBUTE_UNUSED
static bool has_duplicate_filenames(
      ArrayRef<FileSpecificDiagnosticConsumer::Subconsumer> m_subconsumers)
{
   StringSet<> seenFiles;
   for (const auto &subconsumer : m_subconsumers) {
      if (subconsumer.getInputFileName().empty()) {
         // We can handle multiple m_subconsumers that aren't associated with any
         // file, because they only collect diagnostics that aren't in any of the
         // special files. This isn't an important use case to support, but also
         // SmallSet doesn't handle empty strings anyway!
         continue;
      }

      bool isUnique = seenFiles.insert(subconsumer.getInputFileName()).second;
      if (!isUnique) {
         return true;
      }
   }
   return false;
}

std::unique_ptr<DiagnosticConsumer>
FileSpecificDiagnosticConsumer::consolidateSubconsumers(
      SmallVectorImpl<Subconsumer> &m_subconsumers)
{
   if (m_subconsumers.empty()) {
      return nullptr;
   }
   if (m_subconsumers.size() == 1) {
      return std::move(m_subconsumers.front()).m_consumer;
   }
   // Cannot use return
   // llvm::make_unique<FileSpecificDiagnosticConsumer>(m_subconsumers); because
   // the constructor is private.
   return std::unique_ptr<DiagnosticConsumer>(
            new FileSpecificDiagnosticConsumer(m_subconsumers));
}

FileSpecificDiagnosticConsumer::FileSpecificDiagnosticConsumer(
      SmallVectorImpl<Subconsumer> &m_subconsumers)
   : m_subconsumers(std::move(m_subconsumers))
{
   assert(!m_subconsumers.empty() &&
          "don't waste time handling diagnostics that will never get emitted");
   assert(!has_duplicate_filenames(m_subconsumers) &&
          "having multiple m_subconsumers for the same file is not implemented");
}

void FileSpecificDiagnosticConsumer::computeConsumersOrderedByRange(
      SourceManager &sourceMgr)
{
   // Look up each file's source range and add it to the "map" (to be sorted).
   for (unsigned subconsumerIndex = 0; subconsumerIndex < m_subconsumers.size(); ++subconsumerIndex) {
      const Subconsumer &subconsumer = m_subconsumers[subconsumerIndex];
      if (subconsumer.getInputFileName().empty()) {
         continue;
      }

      std::optional<unsigned> bufferID =
            sourceMgr.getIDForBufferIdentifier(subconsumer.getInputFileName());
      assert(bufferID.has_value() && "consumer registered for unknown file");
      CharSourceRange range = sourceMgr.getRangeForBuffer(bufferID.value());
      m_consumersOrderedByRange.emplace_back(
               ConsumerAndRange(range, subconsumerIndex));
   }

   // Sort the "map" by buffer /end/ location, for use with std::lower_bound
   // later. (Sorting by start location would produce the same sort, since the
   // ranges must not be overlapping, but since we need to check end locations
   // later it's consistent to sort by that here.)
   std::sort(m_consumersOrderedByRange.begin(), m_consumersOrderedByRange.end());

   // Check that the ranges are non-overlapping. If the files really are all
   // distinct, this should be trivially true, but if it's ever not we might end
   // up mis-filing diagnostics.
   assert(m_consumersOrderedByRange.end() ==
          std::adjacent_find(m_consumersOrderedByRange.begin(),
                             m_consumersOrderedByRange.end(),
                             [](const ConsumerAndRange &left,
                             const ConsumerAndRange &right) {
      return left.overlaps(right);
   }) &&
          "overlapping ranges despite having distinct files");
}

std::optional<FileSpecificDiagnosticConsumer::Subconsumer *>
FileSpecificDiagnosticConsumer::subconsumerForLocation(SourceManager &sourceMgr,
                                                       SourceLoc loc)
{
   // Diagnostics with invalid locations always go to every consumer.
   if (loc.isInvalid()) {
      return std::nullopt;
   }
   // This map is generated on first use and cached, to allow the
   // FileSpecificDiagnosticConsumer to be set up before the source files are
   // actually loaded.
   if (m_consumersOrderedByRange.empty()) {

      // It's possible to get here while a bridging header PCH is being
      // attached-to, if there's some sort of AST-reader warning or error, which
      // happens before CompilerInstance::setUpInputs(), at which point _no_
      // source buffers are loaded in yet. In that case we return None, rather
      // than trying to build a nonsensical map (and actually crashing since we
      // can't find buffers for the inputs).
      assert(!m_subconsumers.empty());
      if (!sourceMgr.getIDForBufferIdentifier(m_subconsumers.begin()->getInputFileName())
          .has_value()) {
         assert(llvm::none_of(m_subconsumers, [&sourceMgr](const Subconsumer &subconsumer) {
            return sourceMgr.getIDForBufferIdentifier(subconsumer.getInputFileName())
                  .has_value();
         }));
         return std::nullopt;
      }
      auto *mutableThis = const_cast<FileSpecificDiagnosticConsumer*>(this);
      mutableThis->computeConsumersOrderedByRange(sourceMgr);
   }

   // This std::lower_bound call is doing a binary search for the first range
   // that /might/ contain 'loc'. Specifically, since the ranges are sorted
   // by end location, it's looking for the first range where the end location
   // is greater than or equal to 'loc'.
   const ConsumerAndRange *possiblyContainingRangeIter = std::lower_bound(
            m_consumersOrderedByRange.begin(), m_consumersOrderedByRange.end(), loc,
            [](const ConsumerAndRange &entry, SourceLoc loc) -> bool {
      return entry.endsAfter(loc);
   });

   if (possiblyContainingRangeIter != m_consumersOrderedByRange.end() &&
       possiblyContainingRangeIter->contains(loc)) {
      auto *consumerAndRangeForLocation =
            const_cast<ConsumerAndRange *>(possiblyContainingRangeIter);
      return &(*this)[*consumerAndRangeForLocation];
   }

   return std::nullopt;
}

void FileSpecificDiagnosticConsumer::handleDiagnostic(
      SourceManager &sourceMgr, SourceLoc loc, DiagnosticKind kind,
      StringRef formatString, ArrayRef<DiagnosticArgument> formatArgs,
      const DiagnosticInfo &info)
{

   m_hasAnErrorBeenConsumed |= kind == DiagnosticKind::Error;

   std::optional<FileSpecificDiagnosticConsumer::Subconsumer *> subconsumer;
   switch (kind) {
   case DiagnosticKind::Error:
   case DiagnosticKind::Warning:
   case DiagnosticKind::Remark:
      subconsumer = subconsumerForLocation(sourceMgr, loc);
      m_subconsumerForSubsequentNotes = subconsumer;
      break;
   case DiagnosticKind::Note:
      subconsumer = m_subconsumerForSubsequentNotes;
      break;
   }
   if (subconsumer.has_value()) {
      subconsumer.value()->handleDiagnostic(sourceMgr, loc, kind, formatString,
                                            formatArgs, info);
      return;
   }
   for (auto &subconsumer : m_subconsumers) {
      subconsumer.handleDiagnostic(sourceMgr, loc, kind, formatString, formatArgs, info);
   }
}

bool FileSpecificDiagnosticConsumer::finishProcessing()
{
   tellSubconsumersToInformDriverOfIncompleteBatchModeCompilation();

   // Deliberately don't use std::any_of here because we don't want early-exit
   // behavior.

   bool hadError = false;
   for (auto &subconsumer : m_subconsumers) {
      hadError |= subconsumer.getConsumer() &&
            subconsumer.getConsumer()->finishProcessing();
   }
   return hadError;
}

void FileSpecificDiagnosticConsumer::
tellSubconsumersToInformDriverOfIncompleteBatchModeCompilation()
{
   if (!m_hasAnErrorBeenConsumed) {
      return;
   }
   for (auto &info : m_consumersOrderedByRange) {
      (*this)[info].informDriverOfIncompleteBatchModeCompilation();
   }
}

void NullDiagnosticConsumer::handleDiagnostic(
      SourceManager &sourceMgr, SourceLoc loc, DiagnosticKind kind,
      StringRef formatString, ArrayRef<DiagnosticArgument> formatArgs,
      const DiagnosticInfo &info)
{
   LLVM_DEBUG({
                 llvm::dbgs() << "NullDiagnosticConsumer received diagnostic: ";
                 DiagnosticEngine::formatDiagnosticText(llvm::dbgs(), formatString,
                 formatArgs);
                 llvm::dbgs() << "\n";
              });
}

ForwardingDiagnosticConsumer::ForwardingDiagnosticConsumer(DiagnosticEngine &target)
   : m_targetEngine(target)
{}

void ForwardingDiagnosticConsumer::handleDiagnostic(
      SourceManager &sourceMgr, SourceLoc loc, DiagnosticKind kind,
      StringRef formatString, ArrayRef<DiagnosticArgument> formatArgs,
      const DiagnosticInfo &info)
{
   LLVM_DEBUG({
                 llvm::dbgs() << "ForwardingDiagnosticConsumer received diagnostic: ";
                 DiagnosticEngine::formatDiagnosticText(llvm::dbgs(), formatString,
                 formatArgs);
                 llvm::dbgs() << "\n";
              });
   for (auto *C : m_targetEngine.getConsumers()) {
      C->handleDiagnostic(sourceMgr, loc, kind, formatString, formatArgs, info);
   }
}

} // polar::ast
