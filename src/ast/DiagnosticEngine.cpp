//===--- DiagnosticEngine.cpp - Diagnostic Display Engine -----------------===//
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
//===----------------------------------------------------------------------===//
//  This file defines the DiagnosticEngine class, which manages any diagnostics
//  emitted by Swift.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticSuppression.h"
#include "polarphp/parser/SourceMgr.h"
#include "polarphp/parser/Lexer.h" // bad dependency
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

/// A special option only for compiler writers that causes Diagnostics to assert
/// when a failure diagnostic is emitted. Intended for use in the debugger.
llvm::cl::opt<bool> sg_assertOnError("polar-diagnostics-assert-on-error",
                                     llvm::cl::init(false));

namespace polar::ast {

using polar::parser::SourceManager;
using llvm::raw_svector_ostream;

namespace {
enum class DiagnosticOptions
{
   /// No options.
   none,

   /// The location of this diagnostic points to the beginning of the first
   /// token that the parser considers invalid.  If this token is located at the
   /// beginning of the line, then the location is adjusted to point to the end
   /// of the previous token.
   ///
   /// This behavior improves experience for "expected token X" diagnostics.
   PointsToFirstBadToken,

   /// After a fatal error subsequent diagnostics are suppressed.
   Fatal,
};

struct StoredDiagnosticInfo
{
   DiagnosticKind kind : 2;
   bool pointsToFirstBadToken : 1;
   bool isFatal : 1;

   constexpr StoredDiagnosticInfo(DiagnosticKind kind, bool firstBadToken,
                                  bool fatal)
      : kind(kind),
        pointsToFirstBadToken(firstBadToken),
        isFatal(fatal)
   {}

   constexpr StoredDiagnosticInfo(DiagnosticKind kind, DiagnosticOptions opts)
      : StoredDiagnosticInfo(kind,
                             opts == DiagnosticOptions::PointsToFirstBadToken,
                             opts == DiagnosticOptions::Fatal)
   {}
};

// Reproduce the DiagIDs, as we want both the size and access to the raw ids
// themselves.
enum LocalDiagID : uint32_t
{
#define DIAG(KIND, id, Options, text, Signature) id,
#include "polarphp/ast/DiagnosticsAllDefs.h"
   NumDiags
};
} // anonymous namespace

// TODO: categorization
static const constexpr StoredDiagnosticInfo storedDiagnosticInfos[] = {
   #define ERROR(id, Options, text, Signature)                                    \
      StoredDiagnosticInfo(DiagnosticKind::Error, DiagnosticOptions::Options),
   #define WARNING(id, Options, text, Signature)                                  \
      StoredDiagnosticInfo(DiagnosticKind::Warning, DiagnosticOptions::Options),
   #define NOTE(id, Options, text, Signature)                                     \
      StoredDiagnosticInfo(DiagnosticKind::Note, DiagnosticOptions::Options),
   #define REMARK(id, Options, text, Signature)                                   \
      StoredDiagnosticInfo(DiagnosticKind::Remark, DiagnosticOptions::Options),
   #include "polarphp/ast/DiagnosticsAllDefs.h"
};

static_assert(sizeof(storedDiagnosticInfos) / sizeof(StoredDiagnosticInfo) ==
              LocalDiagID::NumDiags,
              "array size mismatch");

static constexpr const char * const diagnosticStrings[] = {
   #define ERROR(id, Options, text, Signature) text,
   #define WARNING(id, Options, text, Signature) text,
   #define NOTE(id, Options, text, Signature) text,
   #define REMARK(id, Options, text, Signature) text,
   #include "polarphp/ast/DiagnosticsAllDefs.h"
   "<not a diagnostic>",
};

DiagnosticState::DiagnosticState()
{
   // Initialize our per-diagnostic state to default
   m_perDiagnosticBehavior.resize(LocalDiagID::NumDiags, Behavior::Unspecified);
}

static CharSourceRange to_char_source_range(SourceManager &sourceMgr, SourceRange sourceRange)
{
   //   return CharSourceRange(sourceMgr, SR.Start, Lexer::getLocForEndOfToken(sourceMgr, SR.End));
}

static CharSourceRange to_char_source_range(SourceManager &sourceMgr, SourceLoc start,
                                            SourceLoc end)
{
   return CharSourceRange(sourceMgr, start, end);
}

/// Extract a character at \p loc. If \p loc is the end of the buffer,
/// return '\f'.
static char extract_char_after(SourceManager &sourceMgr, SourceLoc loc)
{
   auto chars = sourceMgr.extractText({loc, 1});
   return chars.empty() ? '\f' : chars[0];
}

/// Extract a character immediately before \p loc. If \p loc is the
/// start of the buffer, return '\f'.
static char extractCharBefore(SourceManager &sourceMgr, SourceLoc loc)
{
   // We have to be careful not to go off the front of the buffer.
   auto bufferID = sourceMgr.findBufferContainingLoc(loc);
   auto bufferRange = sourceMgr.getRangeForBuffer(bufferID);
   if (bufferRange.getStart() == loc) {
      return '\f';
   }
   auto chars = sourceMgr.extractText({loc.getAdvancedLoc(-1), 1}, bufferID);
   assert(!chars.empty() && "Couldn't extractText with valid range");
   return chars[0];
}

InFlightDiagnostic &InFlightDiagnostic::highlight(SourceRange range)
{
   assert(m_isActive && "Cannot modify an inactive diagnostic");
   if (m_engine && range.isValid())
      m_engine->getActiveDiagnostic()
            .addRange(to_char_source_range(m_engine->m_sourceMgr, range));
   return *this;
}

InFlightDiagnostic &InFlightDiagnostic::highlightChars(SourceLoc start,
                                                       SourceLoc end)
{
   assert(m_isActive && "Cannot modify an inactive diagnostic");
   if (m_engine && start.isValid())
      m_engine->getActiveDiagnostic()
            .addRange(to_char_source_range(m_engine->m_sourceMgr, start, end));
   return *this;
}

/// Add an insertion fix-it to the currently-active diagnostic.  The
/// text is inserted immediately *after* the token specified.
///
InFlightDiagnostic &InFlightDiagnostic::fixItInsertAfter(SourceLoc loc,
                                                         StringRef str)
{
   //  loc = Lexer::getLocForEndOfToken(m_engine->m_sourceMgr, loc);
   //  return fixItInsert(loc, str);
}

/// Add a token-based removal fix-it to the currently-active
/// diagnostic.
InFlightDiagnostic &InFlightDiagnostic::fixItRemove(SourceRange range)
{
   assert(m_isActive && "Cannot modify an inactive diagnostic");
   if (range.isInvalid() || !m_engine) {
      return *this;
   }

   // Convert from a token range to a CharSourceRange, which points to the end of
   // the token we want to remove.
   auto &sourceMgr = m_engine->m_sourceMgr;
   auto charRange = to_char_source_range(sourceMgr, range);

   // If we're removing something (e.g. a keyword), do a bit of extra work to
   // make sure that we leave the code in a good place, without extraneous white
   // space around its hole.  Specifically, check to see there is whitespace
   // before and after the end of range.  If so, nuke the space afterward to keep
   // things consistent.
   if (extract_char_after(sourceMgr, charRange.getEnd()) == ' ' &&
       isspace(extractCharBefore(sourceMgr, charRange.getStart()))) {
      charRange = CharSourceRange(charRange.getStart(),
                                  charRange.getByteLength()+1);
   }
   m_engine->getActiveDiagnostic().addFixIt(Diagnostic::FixIt(charRange, {}));
   return *this;
}


InFlightDiagnostic &InFlightDiagnostic::fixItReplace(SourceRange range,
                                                     StringRef str)
{
   if (str.empty()) {
      return fixItRemove(range);
   }
   assert(m_isActive && "Cannot modify an inactive diagnostic");
   if (range.isInvalid() || !m_engine) {
      return *this;
   }
   auto &sourceMgr = m_engine->m_sourceMgr;
   auto charRange = to_char_source_range(sourceMgr, range);

   // If we're replacing with something that wants spaces around it, do a bit of
   // extra work so that we don't suggest extra spaces.
   if (str.back() == ' ') {
      if (isspace(extract_char_after(sourceMgr, charRange.getEnd()))) {
         str = str.drop_back();
      }
   }
   if (!str.empty() && str.front() == ' ') {
      if (isspace(extractCharBefore(sourceMgr, charRange.getStart()))) {
         str = str.drop_front();
      }
   }

   m_engine->getActiveDiagnostic().addFixIt(Diagnostic::FixIt(charRange, str));
   return *this;
}

InFlightDiagnostic &InFlightDiagnostic::fixItReplaceChars(SourceLoc start,
                                                          SourceLoc end,
                                                          StringRef str)
{
   assert(m_isActive && "Cannot modify an inactive diagnostic");
   if (m_engine && start.isValid()) {
      m_engine->getActiveDiagnostic().addFixIt(Diagnostic::FixIt(
                                                  to_char_source_range(m_engine->m_sourceMgr, start, end), str));
   }
   return *this;
}

InFlightDiagnostic &InFlightDiagnostic::fixItExchange(SourceRange range1,
                                                      SourceRange range2)
{
   assert(m_isActive && "Cannot modify an inactive diagnostic");
   auto &sourceMgr = m_engine->m_sourceMgr;
   // Convert from a token range to a CharSourceRange
   auto charRange1 = to_char_source_range(sourceMgr, range1);
   auto charRange2 = to_char_source_range(sourceMgr, range2);
   // Extract source text.
   auto text1 = sourceMgr.extractText(charRange1);
   auto text2 = sourceMgr.extractText(charRange2);

   m_engine->getActiveDiagnostic()
         .addFixIt(Diagnostic::FixIt(charRange1, text2));
   m_engine->getActiveDiagnostic()
         .addFixIt(Diagnostic::FixIt(charRange2, text1));
   return *this;
}

void InFlightDiagnostic::flush()
{
   if (!m_isActive) {
      return;
   }
   m_isActive = false;
   if (m_engine) {
      m_engine->flushActiveDiagnostic();
   }
}

bool DiagnosticEngine::isDiagnosticPointsToFirstBadToken(DiagID id) const
{
   return storedDiagnosticInfos[(unsigned) id].pointsToFirstBadToken;
}

bool DiagnosticEngine::finishProcessing()
{
   bool hadError = false;
   for (auto &consumer : m_consumers) {
      hadError |= consumer->finishProcessing();
   }
   return hadError;
}

/// Skip forward to one of the given delimiters.
///
/// \param text The text to search through, which will be updated to point
/// just after the delimiter.
///
/// \param delim The first character delimiter to search for.
///
/// \param foundDelim On return, true if the delimiter was found, or false
/// if the end of the string was reached.
///
/// \returns The string leading up to the delimiter, or the empty string
/// if no delimiter is found.
static StringRef
skip_to_delimiter(StringRef &text, char delim, bool *foundDelim = nullptr)
{
   unsigned depth = 0;
   if (foundDelim) {
      *foundDelim = false;
   }
   unsigned index = 0;
   for (unsigned N = text.size(); index != N; ++index) {
      if (text[index] == '{') {
         ++depth;
         continue;
      }
      if (depth > 0) {
         if (text[index] == '}') {
            --depth;
         }
         continue;
      }

      if (text[index] == delim) {
         if (foundDelim) {
            *foundDelim = true;
         }
         break;
      }
   }

   assert(depth == 0 && "Unbalanced {} set in diagnostic text");
   StringRef result = text.substr(0, index);
   text = text.substr(index + 1);
   return result;
}

/// Handle the integer 'select' modifier.  This is used like this:
/// %select{foo|bar|baz}2.  This means that the integer argument "%2" has a
/// value from 0-2.  If the value is 0, the diagnostic prints 'foo'.
/// If the value is 1, it prints 'bar'.  If it has the value 2, it prints 'baz'.
/// This is very useful for certain classes of variant diagnostics.
static void formatSelectionArgument(StringRef modifierArguments,
                                    ArrayRef<DiagnosticArgument> args,
                                    unsigned selectedIndex,
                                    DiagnosticFormatOptions formatOpts,
                                    raw_ostream &out)
{
   bool foundPipe = false;
   do {
      assert((!modifierArguments.empty() || foundPipe) &&
             "Index beyond bounds in %select modifier");
      StringRef text = skip_to_delimiter(modifierArguments, '|', &foundPipe);
      if (selectedIndex == 0) {
         DiagnosticEngine::formatDiagnosticText(out, text, args, formatOpts);
         break;
      }
      --selectedIndex;
   } while (true);

}

/// Format a single diagnostic argument and write it to the given
/// stream.
static void format_diagnostic_argument(StringRef modifier,
                                       StringRef modifierArguments,
                                       ArrayRef<DiagnosticArgument> args,
                                       unsigned argIndex,
                                       DiagnosticFormatOptions formatOpts,
                                       raw_ostream &out)
{
   //   const DiagnosticArgument &arg = args[argIndex];
   //   switch (arg.getKind()) {
   //   case DiagnosticArgumentKind::Integer:
   //      if (modifier == "select") {
   //         assert(arg.getAsInteger() >= 0 && "Negative selection index");
   //         formatSelectionArgument(modifierArguments, args, arg.getAsInteger(),
   //                                 formatOpts, out);
   //      } else if (modifier == "s") {
   //         if (arg.getAsInteger() != 1)
   //            out << 's';
   //      } else {
   //         assert(modifier.empty() && "Improper modifier for integer argument");
   //         out << arg.getAsInteger();
   //      }
   //      break;

   //   case DiagnosticArgumentKind::Unsigned:
   //      if (modifier == "select") {
   //         formatSelectionArgument(modifierArguments, args, arg.getAsUnsigned(),
   //                                 formatOpts, out);
   //      } else if (modifier == "s") {
   //         if (arg.getAsUnsigned() != 1) {
   //            out << 's';
   //         }
   //      } else {
   //         assert(modifier.empty() && "Improper modifier for unsigned argument");
   //         out << arg.getAsUnsigned();
   //      }
   //      break;

   //   case DiagnosticArgumentKind::String:
   //      assert(modifier.empty() && "Improper modifier for string argument");
   //      out << arg.getAsString();
   //      break;

   //   case DiagnosticArgumentKind::Identifier:
   //      assert(modifier.empty() && "Improper modifier for identifier argument");
   //      out << formatOpts.openingQuotationMark;
   //      arg.getAsIdentifier().printPretty(out);
   //      out << formatOpts.closingQuotationMark;
   //      break;

   //   case DiagnosticArgumentKind::ValueDecl:
   //      out << formatOpts.openingQuotationMark;
   //      //      arg.getAsValueDecl()->getFullName().printPretty(out);
   //      out << formatOpts.closingQuotationMark;
   //      break;

   //   case DiagnosticArgumentKind::Type: {
   //      assert(modifier.empty() && "Improper modifier for Type argument");

   //      // Strip extraneous parentheses; they add no value.
   //      //      auto type = arg.getAsType()->getWithoutParens();
   //      //      std::string typeName = type->getString();
   ////      std::string typeName;
   ////      if (should_show_aka(type, typeName)) {
   ////         SmallString<256> akaText;
   ////         RawSvectorOutStream OutAka(akaText);
   ////         OutAka << type->getCanonicalType();
   ////         out << polar::utils::format(formatOpts.akaFormatString.c_str(), typeName.c_str(),
   ////                                     akaText.getCStr());
   ////      } else {
   ////         out << formatOpts.openingQuotationMark << typeName
   ////             << formatOpts.closingQuotationMark;
   ////      }
   //      break;
   //   }
   //   case DiagnosticArgumentKind::TypeRepr:
   //      assert(modifier.empty() && "Improper modifier for TypeRepr argument");
   //      out << formatOpts.openingQuotationMark << arg.getAsTypeRepr()
   //          << formatOpts.closingQuotationMark;
   //      break;

   //   case DiagnosticArgumentKind::ReferenceOwnership:
   //      //      if (modifier == "select") {
   //      //         formatSelectionArgument(modifierArguments, args,
   //      //                                 unsigned(arg.getAsReferenceOwnership()),
   //      //                                 formatOpts, out);
   //      //      } else {
   //      //         assert(modifier.empty() &&
   //      //                "Improper modifier for ReferenceOwnership argument");
   //      //         out << arg.getAsReferenceOwnership();
   //      //      }
   //      break;

   //   case DiagnosticArgumentKind::StaticSpellingKind:
   //      if (modifier == "select") {
   //         formatSelectionArgument(modifierArguments, args,
   //                                 unsigned(arg.getAsStaticSpellingKind()),
   //                                 formatOpts, out);
   //      } else {
   //         assert(modifier.empty() &&
   //                "Improper modifier for StaticSpellingKind argument");
   //         out << arg.getAsStaticSpellingKind();
   //      }
   //      break;

   //   case DiagnosticArgumentKind::DescriptiveDeclKind:
   //      assert(modifier.empty() &&
   //             "Improper modifier for DescriptiveDeclKind argument");
   //      out << Decl::getDescriptiveKindName(arg.getAsDescriptiveDeclKind());
   //      break;

   //   case DiagnosticArgumentKind::DeclAttribute:
   //      assert(modifier.empty() &&
   //             "Improper modifier for DeclAttribute argument");
   //      if (arg.getAsDeclAttribute()->isDeclModifier())
   //         out << formatOpts.openingQuotationMark
   //             << arg.getAsDeclAttribute()->getAttrName()
   //             << formatOpts.closingQuotationMark;
   //      else
   //         out << '@' << arg.getAsDeclAttribute()->getAttrName();
   //      break;

   //   case DiagnosticArgumentKind::VersionTuple:
   //      assert(modifier.empty() &&
   //             "Improper modifier for VersionTuple argument");
   //      out << arg.getAsVersionTuple().getAsString();
   //      break;
   //   }
}

/// Format the given diagnostic text and place the result in the given
/// buffer.
void DiagnosticEngine::formatDiagnosticText(
      raw_ostream &out, StringRef inText, ArrayRef<DiagnosticArgument> args,
      DiagnosticFormatOptions formatOpts)
{
   while (!inText.empty())
   {
      size_t percent = inText.find('%');
      if (percent == StringRef::npos) {
         // Write the rest of the string; we're done.
         out.write(inText.data(), inText.size());
         break;
      }

      // Write the string up to (but not including) the %, then drop that text
      // (including the %).
      out.write(inText.data(), percent);
      inText = inText.substr(percent + 1);

      // '%%' -> '%'.
      if (inText[0] == '%') {
         out.write('%');
         inText = inText.substr(1);
         continue;
      }

      // Parse an optional modifier.
      StringRef modifier;
      {
         size_t length = inText.find_if_not(isalpha);
         modifier = inText.substr(0, length);
         inText = inText.substr(length);
      }

      if (modifier == "error") {
         assert(false && "encountered %error in diagnostic text");
         out << StringRef("<<ERROR>>");
         break;
      }

      // Parse the optional argument list for a modifier, which is brace-enclosed.
      StringRef modifierArguments;
      if (inText[0] == '{') {
         inText = inText.substr(1);
         modifierArguments = skip_to_delimiter(inText, '}');
      }

      // Find the digit sequence, and parse it into an argument index.
      size_t length = inText.find_if_not(isdigit);
      unsigned argIndex;
      bool result = inText.substr(0, length).getAsInteger(10, argIndex);
      assert(!result && "Unparseable argument index value?");
      (void)result;
      assert(argIndex < args.size() && "out-of-range argument index");
      inText = inText.substr(length);

      // Convert the argument to a string.
      format_diagnostic_argument(modifier, modifierArguments, args, argIndex,
                                 formatOpts, out);
   }
}

static DiagnosticKind toDiagnosticKind(DiagnosticState::Behavior behavior)
{
   switch (behavior) {
   case DiagnosticState::Behavior::Unspecified:
      llvm_unreachable("unspecified behavior");
   case DiagnosticState::Behavior::Ignore:
      llvm_unreachable("trying to map an ignored diagnostic");
   case DiagnosticState::Behavior::Error:
   case DiagnosticState::Behavior::Fatal:
      return DiagnosticKind::Error;
   case DiagnosticState::Behavior::Note:
      return DiagnosticKind::Note;
   case DiagnosticState::Behavior::Warning:
      return DiagnosticKind::Warning;
   case DiagnosticState::Behavior::Remark:
      return DiagnosticKind::Remark;
   }
   llvm_unreachable("Unhandled DiagnosticKind in switch.");
}

DiagnosticState::Behavior DiagnosticState::determineBehavior(DiagID id)
{
   auto set = [this](DiagnosticState::Behavior lvl) {
      if (lvl == Behavior::Fatal) {
         m_fatalErrorOccurred = true;
         m_anyErrorOccurred = true;
      } else if (lvl == Behavior::Error) {
         m_anyErrorOccurred = true;
      }

      assert((!sg_assertOnError || !m_anyErrorOccurred) && "We emitted an error?!");
      m_previousBehavior = lvl;
      return lvl;
   };

   // We determine how to handle a diagnostic based on the following rules
   //   1) If current state dictates a certain behavior, follow that
   //   2) If the user provided a behavior for this specific diagnostic, follow
   //      that
   //   3) If the user provided a behavior for this diagnostic's kind, follow
   //      that
   //   4) Otherwise remap the diagnostic kind

   auto diagInfo = storedDiagnosticInfos[(unsigned)id];
   bool isNote = diagInfo.kind == DiagnosticKind::Note;

   //   1) If current state dictates a certain behavior, follow that

   // Notes relating to ignored diagnostics should also be ignored
   if (m_previousBehavior == Behavior::Ignore && isNote) {
      return set(Behavior::Ignore);
   }

   // Suppress diagnostics when in a fatal state, except for follow-on notes
   if (m_fatalErrorOccurred) {
      if (!m_showDiagnosticsAfterFatalError && !isNote) {
         return set(Behavior::Ignore);
      }
   }

   //   2) If the user provided a behavior for this specific diagnostic, follow
   //      that

   if (m_perDiagnosticBehavior[(unsigned)id] != Behavior::Unspecified) {
      return set(m_perDiagnosticBehavior[(unsigned)id]);
   }

   //   3) If the user provided a behavior for this diagnostic's kind, follow
   //      that
   if (diagInfo.kind == DiagnosticKind::Warning) {
      if (m_suppressWarnings) {
         return set(Behavior::Ignore);
      }

      if (m_warningsAsErrors) {
         return set(Behavior::Error);
      }
   }

   //   4) Otherwise remap the diagnostic kind
   switch (diagInfo.kind) {
   case DiagnosticKind::Note:
      return set(Behavior::Note);
   case DiagnosticKind::Error:
      return set(diagInfo.isFatal ? Behavior::Fatal : Behavior::Error);
   case DiagnosticKind::Warning:
      return set(Behavior::Warning);
   case DiagnosticKind::Remark:
      return set(Behavior::Remark);
   }

   llvm_unreachable("Unhandled DiagnosticKind in switch.");
}

void DiagnosticEngine::flushActiveDiagnostic()
{
   assert(m_activeDiagnostic && "No active diagnostic to flush");
   if (m_transactionCount == 0) {
      emitDiagnostic(*m_activeDiagnostic);
   } else {
      m_tentativeDiagnostics.emplace_back(std::move(*m_activeDiagnostic));
   }
   m_activeDiagnostic.reset();
}

void DiagnosticEngine::emitTentativeDiagnostics()
{
   for (auto &diag : m_tentativeDiagnostics) {
      emitDiagnostic(diag);
   }
   m_tentativeDiagnostics.clear();
}

void DiagnosticEngine::emitDiagnostic(const Diagnostic &diagnostic)
{
   //   auto behavior = m_state.determineBehavior(diagnostic.getID());
   //   if (behavior == DiagnosticState::Behavior::Ignore) {
   //      return;
   //   }
   //   // Figure out the source location.
   //   SourceLoc loc = diagnostic.getLoc();
   //   if (loc.isInvalid() && diagnostic.getDecl()) {
   //      const Decl *decl = diagnostic.getDecl();
   //      // If a declaration was provided instead of a location, and that declaration
   //      // has a location we can point to, use that location.
   //      loc = decl->getLoc();
   //      if (loc.isInvalid()) {
   //         // There is no location we can point to. Pretty-print the declaration
   //         // so we can point to it.
   //         SourceLoc ppLoc = m_prettyPrintedDeclarations[decl];
   //         if (ppLoc.isInvalid()) {
   //            class TrackingPrinter : public StreamPrinter
   //            {
   //               SmallVectorImpl<std::pair<const Decl *, uint64_t>> &m_entries;

   //            public:
   //               TrackingPrinter(
   //                        SmallVectorImpl<std::pair<const Decl *, uint64_t>> &entries,
   //                        raw_ostream &outStream) :
   //                  StreamPrinter(outStream),
   //                  m_entries(entries)
   //               {}

   //               void printDeclLoc(const Decl *decl) override
   //               {
   //                  m_entries.push_back({ decl, m_outStream.tell() });
   //               }
   //            };
   //            SmallVector<std::pair<const Decl *, uint64_t>, 8> entries;
   //            SmallString<128> buffer;
   //            SmallString<128> bufferName;
   //            {
   //               // Figure out which declaration to print. It's the top-most
   //               // declaration (not a module).
   //               const Decl *ppDecl = decl;
   //               auto dc = decl->getDeclContext();

   //               // FIXME: Horrible, horrible hackaround. We're not getting a
   //               // DeclContext everywhere we should.
   //               if (!dc) {
   //                  return;
   //               }

   ////               while (!dc->isModuleContext()) {
   ////                  switch (dc->getContextKind()) {
   ////                  case DeclContextKind::Module:
   ////                     llvm_unreachable("Not in a module context!");
   ////                     break;

   ////                  case DeclContextKind::FileUnit:
   ////                  case DeclContextKind::TopLevelCodeDecl:
   ////                     break;

   ////                  case DeclContextKind::ExtensionDecl:
   ////                     //ppDecl = cast<ExtensionDecl>(dc);
   ////                     break;

   ////                  case DeclContextKind::GenericTypeDecl:
   ////                     //ppDecl = cast<GenericTypeDecl>(dc);
   ////                     break;

   ////                  case DeclContextKind::SerializedLocal:
   ////                  case DeclContextKind::Initializer:
   ////                  case DeclContextKind::AbstractClosureExpr:
   ////                  case DeclContextKind::AbstractFunctionDecl:
   ////                  case DeclContextKind::SubscriptDecl:
   ////                  case DeclContextKind::EnumElementDecl:
   ////                     break;
   ////                  }
   ////                  dc = dc->getParent();
   ////               }

   //               // Build the module name path (in reverse), which we use to
   //               // build the name of the buffer.
   //               SmallVector<StringRef, 4> nameComponents;
   //               //               while (dc) {
   //               //                  nameComponents.push_back(cast<ModuleDecl>(dc)->getName().str());
   //               //                  dc = dc->getParent();
   //               //               }

   //               for (unsigned i = nameComponents.size(); i; --i) {
   //                  bufferName += nameComponents[i-1];
   //                  bufferName += '.';
   //               }

   ////               if (auto value = dyn_cast<ValueDecl>(ppDecl)) {
   ////                  bufferName += value->getBaseName().userFacingName();
   ////               } else if (auto ext = dyn_cast<ExtensionDecl>(ppDecl)) {
   ////                  bufferName += ext->getExtendedType().getString();
   ////               }

   //               // Pretty-print the declaration we've picked.
   //               RawSvectorOutStream out(buffer);
   //               TrackingPrinter printer(entries, out);
   //               ppDecl->print(printer, PrintOptions::printForDiagnostics());
   //            }

   //            // Build a buffer with the pretty-printed declaration.
   //            auto bufferID = m_sourceMgr.addMemBufferCopy(buffer, bufferName);
   //            auto memBufferStartLoc = m_sourceMgr.getLocForBufferStart(bufferID);

   //            // Go through all of the pretty-printed entries and record their
   //            // locations.
   //            for (auto entry : entries) {
   //               m_prettyPrintedDeclarations[entry.first] =
   //                     memBufferStartLoc.getAdvancedLoc(entry.second);
   //            }

   //            // Grab the pretty-printed location.
   //            ppLoc = m_prettyPrintedDeclarations[decl];
   //         }

   //         loc = ppLoc;
   //      }
   //   }

   //   // Pass the diagnostic off to the consumer.
   //   DiagnosticInfo Info;
   //   Info.id = diagnostic.getID();
   //   Info.ranges = diagnostic.getRanges();
   //   Info.fixIts = diagnostic.getFixIts();
   //   for (auto &consumer : m_consumers)
   //   {
   //      consumer->handleDiagnostic(m_sourceMgr, loc, toDiagnosticKind(behavior),
   //                                 diagnosticStringFor(Info.id),
   //                                 diagnostic.getArgs(), Info);
   //   }
}

const char *DiagnosticEngine::diagnosticStringFor(const DiagID id)
{
   return diagnosticStrings[(unsigned)id];
}

DiagnosticSuppression::DiagnosticSuppression(DiagnosticEngine &diags)
   : m_diags(diags)
{
   m_consumers = m_diags.takeConsumers();
}

DiagnosticSuppression::~DiagnosticSuppression()
{
   for (auto consumer : m_consumers) {
      m_diags.addConsumer(*consumer);
   }
}

} // polar::ast
