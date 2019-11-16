//===--- Version.cpp - Swift Version Number -------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
//===----------------------------------------------------------------------===//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines several version-related utility functions for polarphp.
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/CharInfo.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/kernel/Version.h"
#include "polarphp/ast/DiagnosticsParse.h"

#include <vector>

#define TOSTR2(X) #X
#define TOSTR(X) TOSTR2(X)

#ifdef POLAR_VERSION_PATCHLEVEL
/// Helper macro for POLAR_VERSION_STRING.
#define POLAR_MAKE_VERSION_STRING(X, Y, Z) TOSTR(X) "." TOSTR(Y) "." TOSTR(Z)

/// A string that describes the Swift version number, e.g., "1.0".
#define POLAR_VERSION_STRING                                                   \
   POLAR_MAKE_VERSION_STRING(POLAR_VERSION_MAJOR, POLAR_VERSION_MINOR,          \
   POLAR_VERSION_PATCHLEVEL)
#else
/// Helper macro for POLAR_VERSION_STRING.
#define POLAR_MAKE_VERSION_STRING(X, Y) TOSTR(X) "." TOSTR(Y)

/// A string that describes the Swift version number, e.g., "1.0".
#define POLAR_VERSION_STRING                                                   \
   POLAR_MAKE_VERSION_STRING(POLAR_VERSION_MAJOR, POLAR_VERSION_MINOR)
#endif

namespace polar::version {


using polar::parser::SourceLoc;
using polar::parser::SourceRange;

using llvm::raw_ostream;
using llvm::raw_string_ostream;
using llvm::raw_svector_ostream;
using llvm::SmallVectorImpl;
using llvm::SmallString;

/// Print a string of the form "LLVM xxxxx, Clang yyyyy, Swift zzzzz",
/// where each placeholder is the revision for the associated repository.
//static void print_full_revision_string(raw_ostream &out)
//{
//}

static void split_version_components(
      SmallVectorImpl<std::pair<StringRef, SourceRange>> &splitComponents,
      StringRef &versionString, SourceLoc loc,
      bool skipQuote = false)
{
   SourceLoc start = (loc.isValid() && skipQuote) ? loc.getAdvancedLoc(1) : loc;
   SourceLoc end = start;

   // Split the version string into tokens separated by the '.' character.
   while (!versionString.empty()) {
      StringRef SplitComponent, Rest;
      std::tie(SplitComponent, Rest) = versionString.split('.');

      if (loc.isValid()) {
         end = end.getAdvancedLoc(SplitComponent.size());
      }
      auto range = loc.isValid() ? SourceRange(start, end) : SourceRange();
      if (loc.isValid()) {
         end = end.getAdvancedLoc(1);
      }

      start = end;
      splitComponents.push_back({SplitComponent, range});
      versionString = Rest;
   }
}

std::optional<Version> Version::parseCompilerVersionString(
      StringRef versionString, SourceLoc loc, DiagnosticEngine *diags)
{

   Version CV;
   SmallString<16> digits;
   raw_svector_ostream OS(digits);
   SmallVector<std::pair<StringRef, SourceRange>, 5> splitComponents;

   split_version_components(splitComponents, versionString, loc,
                          /*skipQuote=*/true);

//   uint64_t componentNumber;
   bool isValidVersion = true;
//   auto checkVersionComponent = [&](unsigned Component, SourceRange range) {
//      unsigned limit = CV.m_components.empty() ? 9223371 : 999;

//      if (Component > limit) {
//         if (diags)
//            diags->diagnose(range.start,
//                            polar::ast::diag::compiler_version_component_out_of_range, limit);
//         isValidVersion = false;
//      }
//   };

//   for (size_t i = 0; i < splitComponents.size(); ++i) {
//      StringRef SplitComponent;
//      SourceRange range;
//      std::tie(SplitComponent, range) = splitComponents[i];

//      // Version components can't be empty.
//      if (SplitComponent.empty()) {
//         if (diags)
//            diags->diagnose(range.start, diag::empty_version_component);
//         isValidVersion = false;
//         continue;
//      }

//      // The second version component isn't used for comparison.
//      if (i == 1) {
//         if (!SplitComponent.equals("*")) {
//            if (diags)
//               diags->diagnose(range.start, diag::unused_compiler_version_component)
//                     .fixItReplaceChars(range.start, range.end, "*");
//         }

//         CV.m_components.push_back(0);
//         continue;
//      }

//      // All other version components must be numbers.
//      if (!SplitComponent.getAsInteger(10, componentNumber)) {
//         checkVersionComponent(componentNumber, range);
//         CV.m_components.push_back(componentNumber);
//         continue;
//      } else {
//         if (diags)
//            diags->diagnose(range.start, diag::version_component_not_number);
//         isValidVersion = false;
//      }
//   }

//   if (CV.m_components.size() > 5) {
//      if (diags)
//         diags->diagnose(loc, diag::compiler_version_too_many_components);
//      isValidVersion = false;
//   }
   return isValidVersion ? std::optional<Version>(CV) : std::nullopt;
}

std::optional<Version> Version::parseVersionString(StringRef versionString,
                                              SourceLoc loc,
                                              DiagnosticEngine *diags)
{
   Version TheVersion;
   SmallString<16> digits;
   raw_svector_ostream OS(digits);
   SmallVector<std::pair<StringRef, SourceRange>, 5> splitComponents;
   // Skip over quote character in string literal.

//   if (versionString.empty()) {
//      if (diags)
//         diags->diagnose(loc, diag::empty_version_string);
//      return None;
//   }

   split_version_components(splitComponents, versionString, loc, diags);

//   uint64_t componentNumber;
   bool isValidVersion = true;

//   for (size_t i = 0; i < splitComponents.size(); ++i) {
//      StringRef SplitComponent;
//      SourceRange range;
//      std::tie(SplitComponent, range) = splitComponents[i];

//      // Version components can't be empty.
//      if (SplitComponent.empty()) {
//         if (diags)
//            diags->diagnose(range.start, diag::empty_version_component);

//         isValidVersion = false;
//         continue;
//      }

//      // All other version components must be numbers.
//      if (!SplitComponent.getAsInteger(10, componentNumber)) {
//         TheVersion.m_components.push_back(componentNumber);
//         continue;
//      } else {
//         if (diags)
//            diags->diagnose(range.start,
//                            diag::version_component_not_number);
//         isValidVersion = false;
//      }
//   }

   return isValidVersion ? std::optional<Version>(TheVersion) : std::nullopt;
}

Version::Version(StringRef versionString,
                 SourceLoc loc,
                 DiagnosticEngine *diags)
   : Version(*parseVersionString(versionString, loc, diags))
{}

Version Version::getCurrentCompilerVersion()
{
//#ifdef POLARPHP_VERSION
//   auto currentVersion = Version::parseVersionString(
//            POLARPHP_VERSION, SourceLoc(), nullptr);
//   assert(currentVersion.hasValue() &&
//          "Embedded polarphp language version couldn't be parsed: '"
//          POLARPHP_VERSION
//          "'");
//   return currentVersion.getValue();
//#else
//   return Version();
//#endif
   return Version();
}

Version Version::getCurrentLanguageVersion()
{
#if POLAR_VERSION_PATCHLEVEL
   return {0, 1, 1};
#else
   return {0, 1};
#endif
}

raw_ostream &operator<<(raw_ostream &outStream, const Version &version)
{
   if (version.empty()) {
       return outStream;
   }
   outStream << version[0];
   for (size_t i = 1, e = version.size(); i != e; ++i) {
      outStream << '.' << version[i];
   }
   return outStream;
}

std::string
Version::preprocessorDefinition(StringRef macroName,
                                ArrayRef<uint64_t> componentWeights) const
{
   uint64_t versionConstant = 0;
   for (size_t i = 0, e = std::min(componentWeights.size(), m_components.size());
        i < e; ++i) {
      versionConstant += componentWeights[i] * m_components[i];
   }

   std::string define("-D");
   raw_string_ostream(define) << macroName << '=' << versionConstant;
   // This isn't using stream.str() so that we get move semantics.
   return define;
}

Version::operator VersionTuple() const
{
   switch (m_components.size()) {
   case 0:
      return VersionTuple();
   case 1:
      return VersionTuple((unsigned)m_components[0]);
   case 2:
      return VersionTuple((unsigned)m_components[0],
            (unsigned)m_components[1]);
   case 3:
      return VersionTuple((unsigned)m_components[0],
            (unsigned)m_components[1],
            (unsigned)m_components[2]);
   case 4:
   case 5:
      return VersionTuple((unsigned)m_components[0],
            (unsigned)m_components[1],
            (unsigned)m_components[2],
            (unsigned)m_components[3]);
   default:
      llvm_unreachable("polar::version::Version with 6 or more components");
   }
}

std::optional<Version> Version::getEffectiveLanguageVersion() const
{
   switch (size()) {
   case 0:
      return std::nullopt;
   case 1:
      break;
   case 2:
      // The only valid explicit language version with a minor
      // component is 4.2.
      if (m_components[0] == 4 && m_components[1] == 2) {
         break;
      }
      return std::nullopt;
   default:
      // We do not want to permit users requesting more precise effective language
      // versions since accepting such an argument promises more than we're able
      // to deliver.
      return std::nullopt;
   }

   // FIXME: When we switch to Swift 5 by default, the "4" case should return
   // a version newer than any released 4.x compiler, and the
   // "5" case should start returning getCurrentLanguageVersion. We should
   // also check for the presence of SWIFT_VERSION_PATCHLEVEL, and if that's
   // set apply it to the "3" case, so that Swift 4.0.1 will automatically
   // have a compatibility mode of 3.2.1.
   switch (m_components[0]) {
   case 4:
      // Version '4' on its own implies '4.1.50'.
      if (size() == 1) {
         return Version{4, 1, 50};
      }
      // This should be true because of the check up above.
      assert(size() == 2 && m_components[0] == 4 && m_components[1] == 2);
      return Version{4, 2};
   case 5:
//      static_assert(POLAR_VERSION_MAJOR == 5,
//                    "getCurrentLanguageVersion is no longer correct here");
      return Version::getCurrentLanguageVersion();
   default:
      return std::nullopt;
   }
}

Version Version::asMajorVersion() const
{
   if (empty()) {
      return {};
   }
   Version res;
   res.m_components.push_back(m_components[0]);
   return res;
}

std::string Version::asAPINotesVersionString() const
{
   // Other than for "4.2.x", map the Swift major version into
   // the API notes version for Swift. This has the effect of allowing
   // API notes to effect changes only on Swift major versions,
   // not minor versions.
   if (size() >= 2 && m_components[0] == 4 && m_components[1] == 2) {
      return "4.2";
   }
   return llvm::itostr(m_components[0]);
}

bool operator>=(const class Version &lhs,
                const class Version &rhs) {

   // The empty compiler version represents the latest possible version,
   // usually built from the source repository.
   if (lhs.empty())
      return true;

   auto n = std::max(lhs.size(), rhs.size());

   for (size_t i = 0; i < n; ++i) {
      auto lv = i < lhs.size() ? lhs[i] : 0;
      auto rv = i < rhs.size() ? rhs[i] : 0;
      if (lv < rv)
         return false;
      else if (lv > rv)
         return true;
   }
   // Equality
   return true;
}

bool operator<(const class Version &lhs, const class Version &rhs) {

   return !(lhs >= rhs);
}

bool operator==(const class Version &lhs,
                const class Version &rhs) {
   auto n = std::max(lhs.size(), rhs.size());
   for (size_t i = 0; i < n; ++i) {
      auto lv = i < lhs.size() ? lhs[i] : 0;
      auto rv = i < rhs.size() ? rhs[i] : 0;
      if (lv != rv) {
         return false;
      }
   }
   return true;
}

std::pair<unsigned, unsigned> retrieve_polarphp_numeric_version()
{
   return { 0, 1 };
}

std::string retrieve_polarphp_full_version(Version effectiveVersion)
{
   return "";
}

std::string retrieve_polarphp_revision()
{
   return "";
}

} // polar::version
