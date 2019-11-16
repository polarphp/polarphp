//===--- Version.h - Swift Version Number -----------------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
///
/// \file
/// Defines version macros and version-related utility functions
/// for polarphp.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_KERNEL_VERSION_H
#define POLARPHP_KERNEL_VERSION_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/VersionTuple.h"

#include <optional>
#include <string>

/// forward declare class with namespace
namespace polar {
namespace ast {
class DiagnosticEngine;
} // ast
namespace parser {
class SourceLoc;
} // parser
} // polar
namespace llvm {
class raw_ostream;
} // llvm

namespace polar::version {

using polar::ast::DiagnosticEngine;
using polar::parser::SourceLoc;
using llvm::SmallVector;
using llvm::StringRef;
using llvm::ArrayRef;
using llvm::VersionTuple;
using llvm::raw_ostream;

/// Represents an internal compiler version, represented as a tuple of
/// integers, or "version components".
///
/// For comparison, if a `CompilerVersion` contains more than one
/// version component, the second one is ignored for comparison,
/// as it represents a compiler variant with no defined ordering.
///
/// A CompilerVersion must have no more than five components and must fit in a
/// 64-bit unsigned integer representation.
///
/// Assuming a maximal version component layout of X.Y.Z.a.b,
/// X, Y, Z, a, b are integers with the following (inclusive) ranges:
/// X: [0 - 9223371]
/// Y: [0 - 999]
/// Z: [0 - 999]
/// a: [0 - 999]
/// b: [0 - 999]
class Version
{
   SmallVector<unsigned, 5> m_components;
public:
   /// Create the empty compiler version - this always compares greater
   /// or equal to any other CompilerVersion, as in the case of building Swift
   /// from latest sources outside of a build/integration/release context.
   Version() = default;

   /// Create a literal version from a list of components.
   Version(std::initializer_list<unsigned> values)
      : m_components(values)
   {}

   /// Create a version from a string in source code.
   ///
   /// Must include only groups of digits separated by a dot.
   Version(StringRef versionString, SourceLoc loc, DiagnosticEngine *diags);

   /// Return a string to be used as an internal preprocessor define.
   ///
   /// The components of the version are multiplied element-wise by
   /// \p componentWeights, then added together (a dot product operation).
   /// If either array is longer than the other, the missing elements are
   /// treated as zero.
   ///
   /// The resulting string will have the form "-DMACRO_NAME=XYYZZ".
   /// The combined value must fit in a uint64_t.
   std::string preprocessorDefinition(StringRef macroName,
                                      ArrayRef<uint64_t> componentWeights) const;

   /// Return the ith version component.
   unsigned operator[](size_t i) const
   {
      return m_components[i];
   }

   /// Return the number of version components.
   size_t size() const
   {
      return m_components.size();
   }

   bool empty() const
   {
      return m_components.empty();
   }

   /// Convert to a (maximum-4-element) llvm::VersionTuple, truncating
   /// away any 5th component that might be in this version.
   operator VersionTuple() const;

   /// Returns the concrete version to use when \e this version is provided as
   /// an argument to -swift-version.
   ///
   /// This is not the same as the set of Swift versions that have ever existed,
   /// just those that we are attempting to maintain backward-compatibility
   /// support for. It's also common for valid versions to produce a different
   /// result; for example "-swift-version 3" at one point instructed the
   /// compiler to act as if it is version 3.1.
   std::optional<Version> getEffectiveLanguageVersion() const;

   /// Whether this version is greater than or equal to the given major version
   /// number.
   bool isVersionAtLeast(unsigned major, unsigned minor = 0) const
   {
      switch (size()) {
      case 0:
         return false;
      case 1:
         return ((m_components[0] == major && 0 == minor) ||
               (m_components[0] > major));
      default:
         return ((m_components[0] == major && m_components[1] >= minor) ||
               (m_components[0] > major));
      }
   }

   /// Return this Version struct with minor and sub-minor components stripped
   Version asMajorVersion() const;

   /// Return this Version struct as the appropriate version string for APINotes.
   std::string asAPINotesVersionString() const;

   /// Parse a version in the form used by the _compiler_version \#if condition.
   static std::optional<Version> parseCompilerVersionString(StringRef versionString,
                                                            SourceLoc loc,
                                                            DiagnosticEngine *diags);

   /// Parse a generic version string of the format [0-9]+(.[0-9]+)*
   ///
   /// Version components can be any unsigned 64-bit number.
   static std::optional<Version> parseVersionString(StringRef versionString,
                                                    SourceLoc loc,
                                                    DiagnosticEngine *diags);

   /// Returns a version from the currently defined SWIFT_COMPILER_VERSION.
   ///
   /// If SWIFT_COMPILER_VERSION is undefined, this will return the empty
   /// compiler version.
   static Version getCurrentCompilerVersion();

   /// Returns a version from the currently defined SWIFT_VERSION_MAJOR and
   /// SWIFT_VERSION_MINOR.
   static Version getCurrentLanguageVersion();

   // List of backward-compatibility versions that we permit passing as
   // -polarphp-version <vers>
   static std::array<StringRef, 3> getValidEffectiveVersions()
   {
      return {{"4", "4.2", "5"}};
   }
};

bool operator>=(const Version &lhs, const Version &rhs);
bool operator<(const Version &lhs, const Version &rhs);
bool operator==(const Version &lhs, const Version &rhs);
inline bool operator!=(const Version &lhs, const Version &rhs)
{
   return !(lhs == rhs);
}

raw_ostream &operator<<(raw_ostream &outStream, const Version &version);

/// Retrieves the numeric {major, minor} Swift version.
///
/// Note that this is the underlying version of the language, ignoring any
/// -swift-version flags that may have been used in a particular invocation of
/// the compiler.
std::pair<unsigned, unsigned> retrieve_polarphp_numeric_version();

/// Retrieves a string representing the complete Swift version, which includes
/// the Swift supported and effective version numbers, the repository version,
/// and the vendor tag.
std::string retrieve_polarphp_full_version(Version effectiveLanguageVersion =
      Version::getCurrentLanguageVersion());

/// Retrieves the repository revision number (or identifier) from which
/// this polarphp was built.
std::string retrieve_polarphp_revision();

} // polar::version

#endif // POLARPHP_KERNEL_VERSION_H
