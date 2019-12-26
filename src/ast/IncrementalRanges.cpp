//===------------- IncrementalRanges.cpp - Generates swiftdeps files
//---------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

// These are the definitions for managing serializable source locations so that
// the frontend and the driver can implement incremental compilation based on
// source ranges.

#include "polarphp/ast/IncrementalRanges.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsCommon.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/ast/FileSystem.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/basic/SourceMgr.h"
#include "polarphp/parser/Lexer.h"
#include "llvm/Support/YAMLParser.h"

namespace polar {
using namespace incremental_ranges;

//==============================================================================
// MARK: SerializableSourceLocation
//==============================================================================

SerializableSourceLocation::SerializableSourceLocation(
   const SourceLoc loc, const SourceManager &SM) {
   auto lc = SM.getLineAndColumn(loc);
   line = lc.first;
   column = lc.second;
}

const SerializableSourceLocation SerializableSourceLocation::endOfAnyFile = {
   ~decltype(line)(0), 0};

void SerializableSourceLocation::print(raw_ostream &out) const {
   out << line << ":" << column;
}

void SerializableSourceLocation::dump() const { print(llvm::errs()); }

//==============================================================================
// MARK: SerializableSourceRange
//==============================================================================

SerializableSourceRange::SerializableSourceRange(
   SerializableSourceLocation start, SerializableSourceLocation end)
   : start(start), end(end) {
#ifndef NDEBUG
   if (!(start <= end)) {
      llvm::errs() << "*** Error: creating backwards SerializableSourceRange: [";
      start.print(llvm::errs());
      llvm::errs() << " -- ";
      end.print(llvm::errs());
      llvm::errs() << ")\n";
      assert(false && "Detected backwards SerializableSourceRange");
   }
#endif
}

SerializableSourceRange::SerializableSourceRange(const CharSourceRange r,
                                                 const SourceManager &SM)
   : start(SerializableSourceLocation(r.getStart(), SM)),
     end(SerializableSourceLocation(r.getEnd(), SM)) {}

const SerializableSourceRange SerializableSourceRange::wholeFile = {
   {0, 0}, SerializableSourceLocation::endOfAnyFile};

Ranges SerializableSourceRange::RangesForWholeFile() {
   assert(wholeFile.end.line && "Ensure initialization happens");
   return {wholeFile};
}

bool SerializableSourceRange::properlyPreceeds(
   const SerializableSourceRange &other) const {
   return end <= other.start;
}

bool SerializableSourceRange::isProperlySorted(
   ArrayRef<SerializableSourceRange> ranges) {
   Optional<SerializableSourceRange> lastRange;
   for (const auto &thisRange : ranges) {
      if (!lastRange)
         lastRange = thisRange;
      else if (!lastRange->properlyPreceeds(thisRange))
         return false;
   }
   return true;
}

bool SerializableSourceRange::isImproperSubsetOf(
   const SerializableSourceRange &superset) const {
   return superset.start <= start && end <= superset.end;
}

Optional<SerializableSourceRange> SerializableSourceRange::findOutlierIfAny(
   ArrayRef<SerializableSourceRange> subset,
   ArrayRef<SerializableSourceRange> superset) {
   // TODO: could optimize this by exploiting sortedness of the arrays:
   // i.e. could skip searching the start of the superset
   for (const auto &subsetRange : subset) {
      const bool isSubsetRangeContained = subsetRange.isImproperSubsetOfAny(superset);
      if (!isSubsetRangeContained)
         return subsetRange;
   }
   return None;
}

Ranges SerializableSourceRange::findAllOutliers(
   ArrayRef<SerializableSourceRange> subset,
   ArrayRef<SerializableSourceRange> superset) {
   // TODO:  optimize with slice of superset
   Ranges outliers;
   std::copy_if(subset.begin(), subset.end(), std::back_inserter(outliers),
                [&](const SerializableSourceRange &r) {
                   return !r.isImproperSubsetOfAny(superset);
                });
   return outliers;
}

bool SerializableSourceRange::isImproperSubsetOfAny(
   ArrayRef<SerializableSourceRange> supersetRanges) const {
   assert(isProperlySorted(supersetRanges) && "required for binary search");
   const auto firstRangeInSupersetNotBeforeSub =
      std::lower_bound(supersetRanges.begin(), supersetRanges.end(), *this,
                       [](const SerializableSourceRange &superRange,
                          const SerializableSourceRange &subsetRange) {
                          return superRange.properlyPreceeds(subsetRange);
                       });
   const bool result =
      firstRangeInSupersetNotBeforeSub != supersetRanges.end() &&
      isImproperSubsetOf(*firstRangeInSupersetNotBeforeSub);

   // slow if input is too big
   assert((supersetRanges.size() >= 5 ||
           result == isImproperSubsetOfAnySlowlyAndSimply(supersetRanges)) &&
          "Check against reference");

   return result;
}

bool SerializableSourceRange::isImproperSubsetOfAnySlowlyAndSimply(
   ArrayRef<SerializableSourceRange> supersetRanges) const {
   return llvm::any_of(supersetRanges,
                       [&](const SerializableSourceRange &superset) {
                          return isImproperSubsetOf(superset);
                       });
}

std::string SerializableSourceRange::printString() const {
   std::string result;
   llvm::raw_string_ostream out(result);
   print(out);
   return out.str();
}

void SerializableSourceRange::print(raw_ostream &out) const {
   out << "[";
   start.print(out);
   out << "--";
   end.print(out);
   out << ")";
}

void SerializableSourceRange::dump() const { print(llvm::errs()); }

//==============================================================================
// MARK: PHPRangesEmitter
//==============================================================================

bool PHPRangesEmitter::emit() const {
   const bool hadError =
      withOutputFile(diags, outputPath, [&](llvm::raw_pwrite_stream &out) {
         out << PHPRangesFileContents::header;
         emitRanges(out);
         return false;
      });
   if (!hadError)
      return false;
   diags.diagnose(SourceLoc(), diag::error_unable_to_write_php_ranges_file,
                  outputPath, "Output error");
   return true;
}

void PHPRangesEmitter::emitRanges(llvm::raw_ostream &out) const {
   PHPRangesFileContents wholeFileContents(
      collectSortedSerializedNoninlinableFunctionBodies());
   llvm::yaml::Output yamlWriter(out);
   yamlWriter << wholeFileContents;
}

Ranges
PHPRangesEmitter::collectSortedSerializedNoninlinableFunctionBodies() const {
   return serializeRanges(
      coalesceSortedRanges(sortRanges(collectNoninlinableFunctionBodies())));
}

std::vector<CharSourceRange>
PHPRangesEmitter::collectNoninlinableFunctionBodies() const {
   struct FnBodyCollector : AstWalker {
      const SourceManager &SM;

      FnBodyCollector(const SourceManager &SM) : SM(SM) {}

      std::vector<CharSourceRange> ranges;

      bool walkToDeclPre(Decl *D) override {
      /// @todo
//         if (const auto *AFD = dyn_cast<AbstractFunctionDecl>(D)) {
//            // If you change an accessor body you might change the inferred
//            // type, and the change might propagate, so exclude them from local
//            // change ranges.
//            // Also rule out implicit constructors a fortiori.
//            if (!isa<AccessorDecl>(AFD) && !AFD->isImplicit() &&
//                !AFD->getAttrs().hasAttribute<InlinableAttr>()) {
//               auto sr = AFD->getBodySourceRange();
//               if (sr.isValid())
//                  ranges.push_back(Lexer::getCharSourceRangeFromSourceRange(SM, sr));
//            }
//            return false;
//         }
         return true;
      }
   };
   FnBodyCollector collector(sourceMgr);
   primaryFile->walk(collector);
   return collector.ranges;
}

std::vector<CharSourceRange>
PHPRangesEmitter::sortRanges(std::vector<CharSourceRange> ranges) const {
   std::sort(ranges.begin(), ranges.end(),
             [&](const CharSourceRange &lhs, const CharSourceRange &rhs) {
                return sourceMgr.isBeforeInBuffer(lhs.getStart(), rhs.getStart());
             });
   return ranges;
}

std::vector<CharSourceRange> PHPRangesEmitter::coalesceSortedRanges(
   std::vector<CharSourceRange> ranges) const {
   if (ranges.empty())
      return ranges;
   auto toBeWidened = ranges.begin();
   auto candidate = toBeWidened + 1;
   while (candidate < ranges.end()) {
      if (isImmediatelyBeforeOrOverlapping(*toBeWidened, *candidate))
         toBeWidened->widen(*candidate++);
      else
         *++toBeWidened = *candidate++;
   }
   ranges.erase(toBeWidened + 1, ranges.end());
   return ranges;
}

std::vector<SerializableSourceRange>
PHPRangesEmitter::serializeRanges(std::vector<CharSourceRange> ranges) const {
   std::vector<SerializableSourceRange> result;
   for (const auto r : ranges)
      result.push_back(SerializableSourceRange(r, sourceMgr));
   return result;
}

bool PHPRangesEmitter::isImmediatelyBeforeOrOverlapping(
   CharSourceRange prev, CharSourceRange next) const {
   // TODO: investigate returning true if only white space intervenes.
   // Would be more work here, but less work downstream.
   return !sourceMgr.isBeforeInBuffer(prev.getEnd(), next.getStart());
}

//==============================================================================
// MARK: CompiledSource
//==============================================================================

bool CompiledSourceEmitter::emit() {
   auto const bufID = primaryFile->getBufferID();
   if (!bufID.hasValue()) {
      diags.diagnose(SourceLoc(),
                     diag::error_unable_to_write_compiled_source_file, outputPath,
                     "No buffer");
      return true;
   }
   const bool hadError =
      withOutputFile(diags, outputPath, [&](llvm::raw_pwrite_stream &out) {
         out << sourceMgr.getEntireTextForBuffer(bufID.getValue());
         return false;
      });
   if (!hadError)
      return false;
   diags.diagnose(SourceLoc(), diag::error_unable_to_write_compiled_source_file,
                  outputPath, "Output error");
   return true;
}

//==============================================================================
// MARK: PHPRangesFileContents
//==============================================================================

void PHPRangesFileContents::dump(const StringRef primaryInputFilename) const {
   llvm::errs() << "\n*** Swift range file contents for '"
                << primaryInputFilename << "': ***\n";
   llvm::yaml::Output dumper(llvm::errs());
   dumper << *const_cast<PHPRangesFileContents *>(this);
}

} // polar