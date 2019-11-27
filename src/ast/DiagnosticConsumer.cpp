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
#include "polarphp/basic/Range.h"

#include "polarphp/ast/DiagnosticConsumer.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/basic/SourceMgr.h"

namespace polar::ast {

using llvm::StringSet;

DiagnosticConsumer::~DiagnosticConsumer() = default;

llvm::SMLoc DiagnosticConsumer::getRawLoc(SourceLoc loc)
{
   return loc.m_loc;
}

LLVM_ATTRIBUTE_UNUSED
static bool has_duplicate_file_names(
      ArrayRef<FileSpecificDiagnosticConsumer::Subconsumer> subconsumers)
{
   llvm::StringSet<> seenFiles;
   for (const auto &subconsumer : subconsumers) {
      if (subconsumer.getInputFileName().empty()) {
         // We can handle multiple subconsumers that aren't associated with any
         // file, because they only collect diagnostics that aren't in any of the
         // special files. This isn't an important use case to support, but also
         // SmallSet doesn't handle empty strings anyway!
         continue;
      }

      bool isUnique = seenFiles.insert(subconsumer.getInputFileName()).second;
      if (!isUnique)
         return true;
   }
   return false;
}

std::unique_ptr<DiagnosticConsumer>
FileSpecificDiagnosticConsumer::consolidateSubconsumers(
      SmallVectorImpl<Subconsumer> &subconsumers)
{
   if (subconsumers.empty()) {
      return nullptr;
   }
   if (subconsumers.size() == 1) {
      return std::move(subconsumers.front()).m_consumer;
   }

   // Cannot use return
   // llvm::make_unique<FileSpecificDiagnosticConsumer>(subconsumers); because
   // the constructor is private.
   return std::unique_ptr<DiagnosticConsumer>(
            new FileSpecificDiagnosticConsumer(subconsumers));
}

FileSpecificDiagnosticConsumer::FileSpecificDiagnosticConsumer(
      SmallVectorImpl<Subconsumer> &subconsumers)
   : m_subconsumers(std::move(subconsumers))
{
   assert(!m_subconsumers.empty() &&
          "don't waste time handling diagnostics that will never get emitted");
   assert(!has_duplicate_file_names(m_subconsumers) &&
          "having multiple subconsumers for the same file is not implemented");
}

void FileSpecificDiagnosticConsumer::computeConsumersOrderedByRange(
      SourceManager &sm)
{
   // Look up each file's source range and add it to the "map" (to be sorted).
   for (const unsigned subconsumerIndex: polar::basic::indices(m_subconsumers)) {
      const Subconsumer &subconsumer = m_subconsumers[subconsumerIndex];
      if (subconsumer.getInputFileName().empty())
         continue;

      std::optional<unsigned> bufferID =
            sm.getIDForBufferIdentifier(subconsumer.getInputFileName());
      assert(bufferID.has_value() && "consumer registered for unknown file");
      CharSourceRange range = sm.getRangeForBuffer(bufferID.value());
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

Optional<FileSpecificDiagnosticConsumer::Subconsumer *>
FileSpecificDiagnosticConsumer::subconsumerForLocation(SourceManager &sm,
                                                       SourceLoc loc)
{
   // Diagnostics with invalid locations always go to every consumer.
   if (loc.isInvalid()) {
      return None;
   }

   // What if a there's a FileSpecificDiagnosticConsumer but there are no
   // subconsumers in it? (This situation occurs for the fix-its
   // FileSpecificDiagnosticConsumer.) In such a case, bail out now.
   if (m_subconsumers.empty()) {
      return None;
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
      if (!sm.getIDForBufferIdentifier(m_subconsumers.begin()->getInputFileName())
          .has_value()) {
         assert(llvm::none_of(m_subconsumers, [&](const Subconsumer &subconsumer) {
            return sm.getIDForBufferIdentifier(subconsumer.getInputFileName())
                  .has_value();
         }));
         return None;
      }
      auto *mutableThis = const_cast<FileSpecificDiagnosticConsumer*>(this);
      mutableThis->computeConsumersOrderedByRange(sm);
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

   return None;
}

void FileSpecificDiagnosticConsumer::handleDiagnostic(
      SourceManager &sm, SourceLoc Loc, DiagnosticKind kind,
      StringRef formatString, ArrayRef<DiagnosticArgument> formatArgs,
      const DiagnosticInfo &info,
      const SourceLoc bufferIndirectlyCausingDiagnostic)
{
   m_hasAnErrorBeenConsumed |= kind == DiagnosticKind::Error;
   auto subconsumer =
         findSubconsumer(sm, Loc, kind, bufferIndirectlyCausingDiagnostic);
   if (subconsumer) {
      subconsumer.getValue()->handleDiagnostic(sm, Loc, kind, formatString,
                                               formatArgs, info,
                                               bufferIndirectlyCausingDiagnostic);
      return;
   }
   // Last resort: spray it everywhere
   for (auto &subconsumer : m_subconsumers) {
      subconsumer.handleDiagnostic(sm, Loc, kind, formatString, formatArgs, info,
                                   bufferIndirectlyCausingDiagnostic);
   }
}

Optional<FileSpecificDiagnosticConsumer::Subconsumer *>
FileSpecificDiagnosticConsumer::findSubconsumer(
      SourceManager &sm, SourceLoc loc, DiagnosticKind kind,
      SourceLoc bufferIndirectlyCausingDiagnostic)
{
   // Ensure that a note goes to the same place as the preceeding non-note.
   switch (kind) {
   case DiagnosticKind::Error:
   case DiagnosticKind::Warning:
   case DiagnosticKind::Remark: {
      auto subconsumer =
            findSubconsumerForNonNote(sm, loc, bufferIndirectlyCausingDiagnostic);
      m_subconsumerForSubsequentNotes = subconsumer;
      return subconsumer;
   }
   case DiagnosticKind::Note:
      return m_subconsumerForSubsequentNotes;
   }
}

Optional<FileSpecificDiagnosticConsumer::Subconsumer *>
FileSpecificDiagnosticConsumer::findSubconsumerForNonNote(
      SourceManager &sm, const SourceLoc loc,
      const SourceLoc bufferIndirectlyCausingDiagnostic)
{
   const auto subconsumer = subconsumerForLocation(sm, loc);
   if (!subconsumer) {
      return None; // No place to put it; might be in an imported module
   }
   if ((*subconsumer)->getConsumer()) {
      return subconsumer; // A primary file with a .dia file
   }
   // Try to put it in the responsible primary input
   if (bufferIndirectlyCausingDiagnostic.isInvalid()) {
      return None;
   }
   const auto currentPrimarySubconsumer =
         subconsumerForLocation(sm, bufferIndirectlyCausingDiagnostic);
   assert(!currentPrimarySubconsumer ||
          (*currentPrimarySubconsumer)->getConsumer() &&
          "current primary must have a .dia file");
   return currentPrimarySubconsumer;
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
      SourceManager &sm, SourceLoc Loc, DiagnosticKind kind,
      StringRef formatString, ArrayRef<DiagnosticArgument> formatArgs,
      const DiagnosticInfo &info, const SourceLoc) {
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
      SourceManager &sm, SourceLoc Loc, DiagnosticKind kind,
      StringRef formatString, ArrayRef<DiagnosticArgument> formatArgs,
      const DiagnosticInfo &info,
      const SourceLoc bufferIndirectlyCausingDiagnostic)
{
   LLVM_DEBUG({
                 llvm::dbgs() << "ForwardingDiagnosticConsumer received diagnostic: ";
                 DiagnosticEngine::formatDiagnosticText(llvm::dbgs(), formatString,
                 formatArgs);
                 llvm::dbgs() << "\n";
              });
   for (auto *consumer : m_targetEngine.getConsumers()) {
      consumer->handleDiagnostic(sm, Loc, kind, formatString, formatArgs, info,
                                 bufferIndirectlyCausingDiagnostic);
   }
}

} // polar::ast
