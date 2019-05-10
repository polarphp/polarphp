//===--- AvailabilitySpec.h - Swift Availability Query ASTs -----*- context++ -*-===//
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
//
// This file defines the availability specification AST classes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AVAILABILITY_SPEC_H
#define POLARPHP_AST_AVAILABILITY_SPEC_H

#include "polarphp/ast/Identifier.h"
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/ast/PlatformKind.h"
#include "polarphp/utils/VersionTuple.h"

namespace polar::ast {

using polar::parser::SourceLoc;
using polar::parser::SourceRange;
using polar::utils::VersionTuple;

class AstContext;

enum class VersionComparison
{
   GreaterThanEqual
};

enum class AvailabilitySpecKind
{
   /// A platform-version constraint of the form "PlatformName X.Y.Z"
   PlatformVersionConstraint,

   /// A wildcard constraint, spelled '*', that is equivalent
   /// to CurrentPlatformName >= MinimumDeploymentTargetVersion
   OtherPlatform,

   /// A language-version constraint of the form "swift X.Y.Z"
   LanguageVersionConstraint,

   /// A PackageDescription version constraint of the form "_PackageDescription X.Y.Z"
   PackageDescriptionVersionConstraint,
};

/// The root class for specifications of API availability in availability
/// queries.
class AvailabilitySpec
{
   AvailabilitySpecKind m_kind;

public:
   AvailabilitySpec(AvailabilitySpecKind kind)
      : m_kind(kind)
   {}

   AvailabilitySpecKind getKind() const
   {
      return m_kind;
   }

   SourceRange getSourceRange() const;

   void *operator new(size_t bytes, AstContext &context,
                      unsigned alignment = alignof(AvailabilitySpec));
   void *operator new(size_t bytes) throw() = delete;
   void operator delete(void *data) throw() = delete;
};

/// An availability specification that guards execution based on the
/// run-time platform and version, e.g., outStream X >= 10.10.
class PlatformVersionConstraintAvailabilitySpec : public AvailabilitySpec
{
public:
   PlatformVersionConstraintAvailabilitySpec(PlatformKind platform,
                                             SourceLoc platformLoc,
                                             VersionTuple version,
                                             SourceRange versionSrcRange)
      : AvailabilitySpec(AvailabilitySpecKind::PlatformVersionConstraint),
        m_platform(platform),
        m_platformLoc(platformLoc),
        m_version(version),
        m_versionSrcRange(versionSrcRange)
   {}

   /// The required platform.
   PlatformKind getPlatform() const
   {
      return m_platform;
   }

   SourceLoc getPlatformLoc() const
   {
      return m_platformLoc;
   }

   // The platform version to compare against.
   VersionTuple getVersion() const
   {
      return m_version;
   }

   SourceRange getVersionSrcRange() const
   {
      return m_versionSrcRange;
   }

   SourceRange getSourceRange() const;

   void print(RawOutStream &outStream, unsigned indent) const;

   static bool classof(const AvailabilitySpec *spec)
   {
      return spec->getKind() == AvailabilitySpecKind::PlatformVersionConstraint;
   }

   void *
   operator new(size_t bytes, AstContext &context,
                unsigned alignment = alignof(PlatformVersionConstraintAvailabilitySpec))
   {
      return AvailabilitySpec::operator new(bytes, context, alignment);
   }

private:
   PlatformKind m_platform;
   SourceLoc m_platformLoc;
   VersionTuple m_version;
   SourceRange m_versionSrcRange;
};

/// An availability specification that guards execution based on the
/// compile-time platform agnostic version, e.g., swift >= 3.0.1,
/// package-description >= 4.0.
class PlatformAgnosticVersionConstraintAvailabilitySpec : public AvailabilitySpec
{
public:
   PlatformAgnosticVersionConstraintAvailabilitySpec(
         AvailabilitySpecKind AvailabilitySpecKind,
         SourceLoc m_platformAgnosticNameLoc, VersionTuple m_version,
         SourceRange m_versionSrcRange)
      : AvailabilitySpec(AvailabilitySpecKind),
        m_platformAgnosticNameLoc(m_platformAgnosticNameLoc),
        m_version(m_version),
        m_versionSrcRange(m_versionSrcRange)
   {
      assert(AvailabilitySpecKind == AvailabilitySpecKind::LanguageVersionConstraint ||
             AvailabilitySpecKind == AvailabilitySpecKind::PackageDescriptionVersionConstraint);
   }

   SourceLoc getPlatformAgnosticNameLoc() const
   {
      return m_platformAgnosticNameLoc;
   }

   // The platform version to compare against.
   VersionTuple getVersion() const
   {
      return m_version;
   }

   SourceRange getVersionSrcRange() const
   {
      return m_versionSrcRange;
   }

   SourceRange getSourceRange() const;

   bool isLanguageVersionSpecific() const
   {
      return getKind() == AvailabilitySpecKind::LanguageVersionConstraint;
   }

   void print(RawOutStream &outStream, unsigned indent) const;

   static bool classof(const AvailabilitySpec *spec)
   {
      return spec->getKind() == AvailabilitySpecKind::LanguageVersionConstraint ||
            spec->getKind() == AvailabilitySpecKind::PackageDescriptionVersionConstraint;
   }

   void *operator new(size_t bytes, AstContext &context,
                      unsigned alignment = alignof(PlatformAgnosticVersionConstraintAvailabilitySpec))
   {
      return AvailabilitySpec::operator new(bytes, context, alignment);
   }

private:
   SourceLoc m_platformAgnosticNameLoc;
   VersionTuple m_version;
   SourceRange m_versionSrcRange;
};

/// A wildcard availability specification that guards execution
/// by checking that the run-time version is greater than the minimum
/// deployment target. This specification is designed to ease porting
/// to new platforms. Because new platforms typically branch from
/// existing platforms, the wildcard allows an #available() check to do the
/// "right" thing (executing the guarded branch) on the new platform without
/// requiring a modification to every availability guard in the program. Note
/// that we still do compile-time availability checking with '*', so the
/// compiler will still catch references to potentially unavailable symbols.
class OtherPlatformAvailabilitySpec : public AvailabilitySpec
{
   SourceLoc m_starLoc;

public:
   OtherPlatformAvailabilitySpec(SourceLoc starLoc)
      : AvailabilitySpec(AvailabilitySpecKind::OtherPlatform),
        m_starLoc(starLoc)
   {}

   SourceRange getSourceRange() const
   {
      return SourceRange(m_starLoc, m_starLoc);
   }

   void print(RawOutStream &outStream, unsigned indent) const;

   static bool classof(const AvailabilitySpec *spec)
   {
      return spec->getKind() == AvailabilitySpecKind::OtherPlatform;
   }

   void *operator new(size_t bytes, AstContext &context,
                      unsigned alignment = alignof(OtherPlatformAvailabilitySpec))
   {
      return AvailabilitySpec::operator new(bytes, context, alignment);
   }
};

} // polar::ast

#endif // POLARPHP_AST_AVAILABILITY_SPEC_H
