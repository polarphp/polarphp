//===--- DiagnosticEngine.h - Diagnostic Display m_engine ---------*- C++ -*-===//
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
//
//  This file declares the DiagnosticEngine class, which manages any diagnostics
//  emitted by polarphp.
//

#ifndef POLARPHP_AST_DIAGNOSTIC_ENGINE_H
#define POLARPHP_AST_DIAGNOSTIC_ENGINE_H

#include "polarphp/ast/DiagnosticConsumer.h"
#include "llvm/Support/VersionTuple.h"
#include "llvm/ADT/DenseMap.h"

/// forward declare with namespace
namespace polar::parser {
class SourceManager;
}

namespace polar::ast {

class ReferenceOwnership;
class DiagnosticEngine;

using llvm::DenseMap;
using polar::parser::SourceManager;
using polar::parser::SourceRange;
using llvm::VersionTuple;
using llvm::raw_ostream;

enum class StaticSpellingKind : uint8_t;
enum class DescriptiveDeclKind : uint8_t;
enum DeclAttrKind : unsigned;

/// Enumeration describing all of possible diagnostics.
///
/// Each of the diagnostics described in Diagnostics.def has an entry in
/// this enumeration type that uniquely identifies it.
enum class DiagID : uint32_t;

/// Describes a diagnostic along with its argument types.
///
/// The diagnostics header introduces instances of this type for each
/// diagnostic, which provide both the set of argument types (used to
/// check/convert the arguments at each call site) and the diagnostic id
/// (for other information about the diagnostic).
template<typename ...ArgTypes>
struct Diag
{
   /// The diagnostic id corresponding to this diagnostic.
   DiagID id;
};

namespace internal {
/// Describes how to pass a diagnostic argument of the given type.
///
/// By default, diagnostic arguments are passed by value, because they
/// tend to be small. Larger diagnostic arguments
/// need to specialize this class template to pass by reference.
template<typename T>
struct PassArgument
{
   typedef T type;
};
} // internal

/// Describes the kind of diagnostic argument we're storing.
///
enum class DiagnosticArgumentKind
{
   String,
   Integer,
   Unsigned,
   Identifier,
   ValueDecl,
   Type,
   TypeRepr,
   StaticSpellingKind,
   ReferenceOwnership,
   DescriptiveDeclKind,
   DeclAttribute,
   VersionTuple,
};

namespace diag {
enum class RequirementKind : uint8_t;
} // diag

/// Variant type that holds a single diagnostic argument of a known
/// type.
///
/// All diagnostic arguments are converted to an instance of this class.
class DiagnosticArgument
{
   DiagnosticArgumentKind m_kind;
   union {
      int m_integerVal;
      unsigned m_unsignedVal;
      StringRef m_stringVal;
      StaticSpellingKind m_staticSpellingKindVal;
      DescriptiveDeclKind m_descriptiveDeclKindVal;
      VersionTuple m_versionVal;
   };

public:
   DiagnosticArgument(StringRef str)
      : m_kind(DiagnosticArgumentKind::String),
        m_stringVal(str)
   {}

   DiagnosticArgument(int intVal)
      : m_kind(DiagnosticArgumentKind::Integer),
        m_integerVal(intVal) {
   }

   DiagnosticArgument(unsigned uintVal)
      : m_kind(DiagnosticArgumentKind::Unsigned),
        m_unsignedVal(uintVal)
   {}

   DiagnosticArgument(StaticSpellingKind ssk)
      : m_kind(DiagnosticArgumentKind::StaticSpellingKind),
        m_staticSpellingKindVal(ssk)
   {}

   DiagnosticArgument(DescriptiveDeclKind ddk)
      : m_kind(DiagnosticArgumentKind::DescriptiveDeclKind),
        m_descriptiveDeclKindVal(ddk)
   {}

   DiagnosticArgument(VersionTuple version)
      : m_kind(DiagnosticArgumentKind::VersionTuple),
        m_versionVal(version)
   {}

   /// Initializes a diagnostic argument using the underlying type of the
   /// given enum.
   template<
         typename EnumType,
         typename std::enable_if<std::is_enum<EnumType>::value>::type* = nullptr>
   DiagnosticArgument(EnumType value)
      : DiagnosticArgument(
           static_cast<typename std::underlying_type<EnumType>::type>(value))
   {}

   DiagnosticArgumentKind getKind() const
   {
      return m_kind;
   }

   StringRef getAsString() const
   {
      assert(m_kind == DiagnosticArgumentKind::String);
      return m_stringVal;
   }

   int getAsInteger() const
   {
      assert(m_kind == DiagnosticArgumentKind::Integer);
      return m_integerVal;
   }

   unsigned getAsUnsigned() const
   {
      assert(m_kind == DiagnosticArgumentKind::Unsigned);
      return m_unsignedVal;
   }

   StaticSpellingKind getAsStaticSpellingKind() const
   {
      assert(m_kind == DiagnosticArgumentKind::StaticSpellingKind);
      return m_staticSpellingKindVal;
   }

   DescriptiveDeclKind getAsDescriptiveDeclKind() const
   {
      assert(m_kind == DiagnosticArgumentKind::DescriptiveDeclKind);
      return m_descriptiveDeclKindVal;
   }

   VersionTuple getAsVersionTuple() const
   {
      assert(m_kind == DiagnosticArgumentKind::VersionTuple);
      return m_versionVal;
   }
};

struct DiagnosticFormatOptions
{
   const std::string openingQuotationMark;
   const std::string closingQuotationMark;
   const std::string akaFormatString;

   DiagnosticFormatOptions(std::string openingQuotationMark,
                           std::string closingQuotationMark,
                           std::string akaFormatString)
      : openingQuotationMark(openingQuotationMark),
        closingQuotationMark(closingQuotationMark),
        akaFormatString(akaFormatString)
   {}

   DiagnosticFormatOptions()
      : openingQuotationMark("'"),
        closingQuotationMark("'"),
        akaFormatString("'%s' (aka '%s')")
   {}
};

/// Diagnostic - This is a specific instance of a diagnostic along with all of
/// the DiagnosticArguments that it requires.
class Diagnostic
{
public:
   typedef DiagnosticInfo::FixIt FixIt;

public:
   // All constructors are intentionally implicit.
   template<typename ...ArgTypes>
   Diagnostic(Diag<ArgTypes...> id,
              typename internal::PassArgument<ArgTypes>::type... vArgs)
      : m_id(id.id)
   {
      DiagnosticArgument diagArgs[] = {
         DiagnosticArgument(0), std::move(vArgs)...
      };
      m_args.append(diagArgs + 1, diagArgs + 1 + sizeof...(vArgs));
   }

   /*implicit*/Diagnostic(DiagID id, ArrayRef<DiagnosticArgument> args)
      : m_id(id),
        m_args(args.begin(), args.end())
   {}

   // Accessors.
   DiagID getID() const
   {
      return m_id;
   }

   ArrayRef<DiagnosticArgument> getArgs() const
   {
      return m_args;
   }

   ArrayRef<CharSourceRange> getRanges() const
   {
      return m_ranges;
   }

   ArrayRef<FixIt> getFixIts() const
   {
      return m_fixIts;
   }

   SourceLoc getLoc() const
   {
      return m_loc;
   }

   void setLoc(SourceLoc loc)
   {
      m_loc = loc;
   }

   /// Returns true if this object represents a particular diagnostic.
   ///
   /// \code
   /// someDiag.is(diag::invalid_diagnostic)
   /// \endcode
   template<typename ...OtherArgTypes>
   bool is(Diag<OtherArgTypes...> other) const
   {
      return m_id == other.m_id;
   }

   void addRange(CharSourceRange range)
   {
      m_ranges.push_back(range);
   }

   // Avoid copying the fix-it text more than necessary.
   void addFixIt(FixIt &&fixIt)
   {
      m_fixIts.push_back(std::move(fixIt));
   }

private:
   DiagID m_id;
   SmallVector<DiagnosticArgument, 3> m_args;
   SmallVector<CharSourceRange, 2> m_ranges;
   SmallVector<FixIt, 2> m_fixIts;
   SourceLoc m_loc;
};

/// Describes an in-flight diagnostic, which is currently active
/// within the diagnostic engine and can be augmented within additional
/// information (source ranges, Fix-Its, etc.).
///
/// Only a single in-flight diagnostic can be active at one time, and all
/// additional information must be emitted through the active in-flight
/// diagnostic.
class InFlightDiagnostic
{
   friend class DiagnosticEngine;

   DiagnosticEngine *m_engine;
   bool m_isActive;

   /// Create a new in-flight diagnostic.
   ///
   /// This constructor is only available to the DiagnosticEngine.
   InFlightDiagnostic(DiagnosticEngine &engine)
      : m_engine(&engine),
        m_isActive(true)
   {}

   InFlightDiagnostic(const InFlightDiagnostic &) = delete;
   InFlightDiagnostic &operator=(const InFlightDiagnostic &) = delete;
   InFlightDiagnostic &operator=(InFlightDiagnostic &&) = delete;

public:
   /// Create an active but unattached in-flight diagnostic.
   ///
   /// The resulting diagnostic can be used as a dummy, accepting the
   /// syntax to add additional information to a diagnostic without
   /// actually emitting a diagnostic.
   InFlightDiagnostic()
      : m_engine(0),
        m_isActive(true)
   {}

   /// Transfer an in-flight diagnostic to a new object, which is
   /// typically used when returning in-flight diagnostics.
   InFlightDiagnostic(InFlightDiagnostic &&other)
      : m_engine(other.m_engine),
        m_isActive(other.m_isActive)
   {
      other.m_isActive = false;
   }

   ~InFlightDiagnostic()
   {
      if (m_isActive) {
         flush();
      }
   }

   /// Flush the active diagnostic to the diagnostic output engine.
   void flush();

   /// Add a token-based range to the currently-active diagnostic.
   InFlightDiagnostic &highlight(SourceRange range);

   /// Add a character-based range to the currently-active diagnostic.
   InFlightDiagnostic &highlightChars(SourceLoc start, SourceLoc end);

   /// Add a token-based replacement fix-it to the currently-active
   /// diagnostic.
   InFlightDiagnostic &fixItReplace(SourceRange range, StringRef str);

   /// Add a character-based replacement fix-it to the currently-active
   /// diagnostic.
   InFlightDiagnostic &fixItReplaceChars(SourceLoc start, SourceLoc end,
                                         StringRef str);

   /// Add an insertion fix-it to the currently-active diagnostic.
   InFlightDiagnostic &fixItInsert(SourceLoc loc, StringRef str)
   {
      return fixItReplaceChars(loc, loc, str);
   }

   /// Add an insertion fix-it to the currently-active diagnostic.  The
   /// text is inserted immediately *after* the token specified.
   ///
   InFlightDiagnostic &fixItInsertAfter(SourceLoc loc, StringRef);

   /// Add a token-based removal fix-it to the currently-active
   /// diagnostic.
   InFlightDiagnostic &fixItRemove(SourceRange range);

   /// Add a character-based removal fix-it to the currently-active
   /// diagnostic.
   InFlightDiagnostic &fixItRemoveChars(SourceLoc start, SourceLoc end)
   {
      return fixItReplaceChars(start, end, {});
   }

   /// Add two replacement fix-it exchanging source ranges to the
   /// currently-active diagnostic.
   InFlightDiagnostic &fixItExchange(SourceRange range1, SourceRange range2);
};

/// Class to track, map, and remap diagnostic severity and fatality
///
class DiagnosticState
{
public:
   /// Describes the current behavior to take with a diagnostic
   enum class Behavior : uint8_t
   {
      Unspecified,
      Ignore,
      Note,
      Remark,
      Warning,
      Error,
      Fatal,
   };

private:
   /// Whether we should continue to emit diagnostics, even after a
   /// fatal error
   bool m_showDiagnosticsAfterFatalError = false;

   /// Don't emit any warnings
   bool m_suppressWarnings = false;

   /// Emit all warnings as errors
   bool m_warningsAsErrors = false;

   /// Whether a fatal error has occurred
   bool m_fatalErrorOccurred = false;

   /// Whether any error diagnostics have been emitted.
   bool m_anyErrorOccurred = false;

   /// Track the previous emitted Behavior, useful for notes
   Behavior m_previousBehavior = Behavior::Unspecified;

   /// Track settable, per-diagnostic state that we store
   std::vector<Behavior> m_perDiagnosticBehavior;

public:
   DiagnosticState();

   /// Figure out the Behavior for the given diagnostic, taking current
   /// state such as fatality into account.
   Behavior determineBehavior(DiagID id);

   bool hadAnyError() const
   {
      return m_anyErrorOccurred;
   }

   bool hasFatalErrorOccurred() const
   {
      return m_fatalErrorOccurred;
   }

   void setShowDiagnosticsAfterFatalError(bool val = true)
   {
      m_showDiagnosticsAfterFatalError = val;
   }

   bool getShowDiagnosticsAfterFatalError()
   {
      return m_showDiagnosticsAfterFatalError;
   }

   /// Whether to skip emitting warnings
   void setSuppressWarnings(bool val)
   {
      m_suppressWarnings = val;
   }

   bool getSuppressWarnings() const
   {
      return m_suppressWarnings;
   }

   /// Whether to treat warnings as errors
   void setWarningsAsErrors(bool val)
   {
      m_warningsAsErrors = val;
   }

   bool getWarningsAsErrors() const
   {
      return m_warningsAsErrors;
   }

   void resetHadAnyError()
   {
      m_anyErrorOccurred = false;
      m_fatalErrorOccurred = false;
   }

   /// Set per-diagnostic behavior
   void setDiagnosticBehavior(DiagID id, Behavior behavior)
   {
      m_perDiagnosticBehavior[(unsigned)id] = behavior;
   }

private:
   // Make the state movable only
   DiagnosticState(const DiagnosticState &) = delete;
   const DiagnosticState &operator=(const DiagnosticState &) = delete;

   DiagnosticState(DiagnosticState &&) = default;
   DiagnosticState &operator=(DiagnosticState &&) = default;
};

/// Class responsible for formatting diagnostics and presenting them
/// to the user.
class DiagnosticEngine
{
public:
   explicit DiagnosticEngine(SourceManager &sourceMgr)
      : m_sourceMgr(sourceMgr),
        m_activeDiagnostic()
   {}

   /// hadAnyError - return true if any *error* diagnostics have been emitted.
   bool hadAnyError() const
   {
      return m_state.hadAnyError();
   }

   bool hasFatalErrorOccurred() const
   {
      return m_state.hasFatalErrorOccurred();
   }

   void setShowDiagnosticsAfterFatalError(bool val = true)
   {
      m_state.setShowDiagnosticsAfterFatalError(val);
   }

   bool getShowDiagnosticsAfterFatalError()
   {
      return m_state.getShowDiagnosticsAfterFatalError();
   }

   /// Whether to skip emitting warnings
   void setSuppressWarnings(bool val)
   {
      m_state.setSuppressWarnings(val);
   }

   bool getSuppressWarnings() const
   {
      return m_state.getSuppressWarnings();
   }

   /// Whether to treat warnings as errors
   void setWarningsAsErrors(bool val)
   {
      m_state.setWarningsAsErrors(val);
   }

   bool getWarningsAsErrors() const
   {
      return m_state.getWarningsAsErrors();
   }

   void ignoreDiagnostic(DiagID id)
   {
      m_state.setDiagnosticBehavior(id, DiagnosticState::Behavior::Ignore);
   }

   void resetHadAnyError()
   {
      m_state.resetHadAnyError();
   }

   /// Add an additional DiagnosticConsumer to receive diagnostics.
   void addConsumer(DiagnosticConsumer &consumer)
   {
      m_consumers.push_back(&consumer);
   }

   /// Remove a specific DiagnosticConsumer.
   void removeConsumer(DiagnosticConsumer &consumer)
   {
      m_consumers.erase(
               std::remove(m_consumers.begin(), m_consumers.end(), &consumer));
   }

   /// Remove and return all \c DiagnosticConsumers.
   std::vector<DiagnosticConsumer *> takeConsumers()
   {
      auto result = std::vector<DiagnosticConsumer*>(m_consumers.begin(),
                                                     m_consumers.end());
      m_consumers.clear();
      return result;
   }

   /// Return all \c DiagnosticConsumers.
   ArrayRef<DiagnosticConsumer *> getConsumers()
   {
      return m_consumers;
   }

   /// Emit a diagnostic using a preformatted array of diagnostic
   /// arguments.
   ///
   /// \param loc The location to which the diagnostic refers in the source
   /// code.
   ///
   /// \param id The diagnostic id.
   ///
   /// \param args The preformatted set of diagnostic arguments. The caller
   /// must ensure that the diagnostic arguments have the appropriate type.
   ///
   /// \returns An in-flight diagnostic, to which additional information can
   /// be attached.
   InFlightDiagnostic diagnose(SourceLoc loc, DiagID id,
                               ArrayRef<DiagnosticArgument> args)
   {
      return diagnose(loc, Diagnostic(id, args));
   }

   /// Emit an already-constructed diagnostic at the given location.
   ///
   /// \param loc The location to which the diagnostic refers in the source
   /// code.
   ///
   /// \param D The diagnostic.
   ///
   /// \returns An in-flight diagnostic, to which additional information can
   /// be attached.
   InFlightDiagnostic diagnose(SourceLoc loc, const Diagnostic &diagnostic)
   {
      assert(!m_activeDiagnostic && "Already have an active diagnostic");
      m_activeDiagnostic = diagnostic;
      m_activeDiagnostic->setLoc(loc);
      return InFlightDiagnostic(*this);
   }

   /// Emit a diagnostic with the given set of diagnostic arguments.
   ///
   /// \param loc The location to which the diagnostic refers in the source
   /// code.
   ///
   /// \param id The diagnostic to be emitted.
   ///
   /// \param args The diagnostic arguments, which will be converted to
   /// the types expected by the diagnostic \p id.
   template<typename ...ArgTypes>
   InFlightDiagnostic
   diagnose(SourceLoc loc, Diag<ArgTypes...> id,
            typename internal::PassArgument<ArgTypes>::type... args)
   {
      return diagnose(loc, Diagnostic(id, std::move(args)...));
   }

   /// Delete an API that may lead clients to avoid specifying source location.
   template<typename ...ArgTypes>
   InFlightDiagnostic
   diagnose(Diag<ArgTypes...> id,
            typename internal::PassArgument<ArgTypes>::type... args) = delete;

   /// \returns true if diagnostic is marked with PointsToFirstBadToken
   /// option.
   bool isDiagnosticPointsToFirstBadToken(DiagID id) const;

   /// \returns true if any diagnostic consumer gave an error while invoking
   //// \c finishProcessing.
   bool finishProcessing();

   /// Format the given diagnostic text and place the result in the given
   /// buffer.
   static void formatDiagnosticText(
         raw_ostream &outStream, StringRef inText,
         ArrayRef<DiagnosticArgument> formatArgs,
         DiagnosticFormatOptions formatOpts = DiagnosticFormatOptions());

public:
   static const char *diagnosticStringFor(const DiagID id);

private:
   /// Flush the active diagnostic.
   void flushActiveDiagnostic();

   /// Retrieve the active diagnostic.
   Diagnostic &getActiveDiagnostic()
   {
      return *m_activeDiagnostic;
   }

   /// Send \c diag to all diagnostic consumers.
   void emitDiagnostic(const Diagnostic &diag);

   /// Send all tentative diagnostics to all diagnostic consumers and
   /// delete them.
   void emitTentativeDiagnostics();

private:
   /// The source manager used to interpret source locations and
   /// display diagnostics.
   SourceManager &m_sourceMgr;

   /// The diagnostic consumer(s) that will be responsible for actually
   /// emitting diagnostics.
   SmallVector<DiagnosticConsumer *, 2> m_consumers;

   /// Tracks diagnostic behaviors and state
   DiagnosticState m_state;

   /// The currently active diagnostic, if there is one.
   std::optional<Diagnostic> m_activeDiagnostic;

   /// All diagnostics that have are no longer active but have not yet
   /// been emitted due to an open transaction.
   SmallVector<Diagnostic, 4> m_tentativeDiagnostics;

   /// The number of open diagnostic transactions. Diagnostics are only
   /// emitted once all transactions have closed.
   unsigned m_transactionCount = 0;

   friend class InFlightDiagnostic;
   friend class DiagnosticTransaction;
};

/// Represents a diagnostic transaction. While a transaction is
/// open, all recorded diagnostics are saved until the transaction commits,
/// at which point they are emitted. If the transaction is instead aborted,
/// the diagnostics are erased. Transactions may be nested but must be closed
/// in LIFO order. An open transaction is implicitly committed upon
/// destruction.
class DiagnosticTransaction
{
public:
   explicit DiagnosticTransaction(DiagnosticEngine &engine)
      : m_engine(engine),
        m_prevDiagnostics(m_engine.m_tentativeDiagnostics.size()),
        m_depth(m_engine.m_transactionCount),
        m_isOpen(true)
   {
      assert(!m_engine.m_activeDiagnostic);
      m_engine.m_transactionCount++;
   }

   ~DiagnosticTransaction()
   {
      if (m_isOpen) {
         commit();
      }
   }

   /// Abort and close this transaction and erase all diagnostics
   /// record while it was open.
   void abort()
   {
      close();
      m_engine.m_tentativeDiagnostics.erase(
               m_engine.m_tentativeDiagnostics.begin() + m_prevDiagnostics,
               m_engine.m_tentativeDiagnostics.end());
   }

   /// Commit and close this transaction. If this is the top-level
   /// transaction, emit any diagnostics that were recorded while it was open.
   void commit() {
      close();
      if (m_depth == 0) {
         assert(m_prevDiagnostics == 0);
         m_engine.emitTentativeDiagnostics();
      }
   }

private:
   void close()
   {
      assert(m_isOpen && "only open transactions may be closed");
      m_isOpen = false;
      m_engine.m_transactionCount--;
      assert(m_depth == m_engine.m_transactionCount &&
             "transactions must be closed LIFO");
   }

private:
   DiagnosticEngine &m_engine;

   /// How many tentative diagnostics there were when the transaction
   /// was opened.
   unsigned m_prevDiagnostics;

   /// How many other transactions were open when this transaction was
   /// opened.
   unsigned m_depth;

   /// Whether this transaction is currently open.
   bool m_isOpen = true;
};

} // polar::ast

#endif // POLARPHP_AST_DIAGNOSTIC_ENGINE_H
