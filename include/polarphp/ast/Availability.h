//===--- Availability.h - Swift Availability Structures ---------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines data structures for API availability.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AVAILABILITY_H
#define POLARPHP_AST_AVAILABILITY_H

#include "polarphp/ast/Type.h"
#include "polarphp/utils/VersionTuple.h"
#include <optional>

namespace polar::ast {

using polar::utils::VersionTuple;

class AstContext;
class Decl;

/// A lattice of version ranges of the form [x.y.z, +Inf).
class VersionRange
{
   // The lattice ordering is linear:
   // Empty <= ... <= [10.10.0,+Inf) <= ... [10.1.0,+Inf) <=  ... <= All
   // and corresponds to set inclusion.

   // The concretization of lattice elements is:
   //    Empty: empty
   //    All: all versions
   //    x.y.x: all versions greater than or equal to x.y.z

   enum class ExtremalRange { Empty, All };

   // A version range is either an extremal value (Empty, All) or
   // a single version tuple value representing the lower end point x.y.z of a
   // range [x.y.z, +Inf).
   union {
      VersionTuple m_lowerEndpoint;
      ExtremalRange m_extremalValue;
   };

   unsigned m_hasLowerEndpoint : 1;

public:
   /// Returns true if the range of versions is empty, or false otherwise.
   bool isEmpty() const
   {
      return !m_hasLowerEndpoint && m_extremalValue == ExtremalRange::Empty;
   }

   /// Returns true if the range includes all versions, or false otherwise.
   bool isAll() const
   {
      return !m_hasLowerEndpoint && m_extremalValue == ExtremalRange::All;
   }

   /// Returns true if the range has a lower end point; that is, if it is of
   /// the form [X, +Inf).
   bool hasLowerEndpoint() const
   {
      return m_hasLowerEndpoint;
   }

   /// Returns the range's lower endpoint.
   const VersionTuple &getLowerEndpoint() const
   {
      assert(m_hasLowerEndpoint);
      return m_lowerEndpoint;
   }

   /// Returns a representation of this range as a string for debugging purposes.
   std::string getAsString() const
   {
      if (isEmpty()) {
         return "empty";
      } else if (isAll()) {
         return "all";
      } else {
         return "[" + getLowerEndpoint().getAsString() + ",+Inf)";
      }
   }

   /// Returns true if all versions in this range are also in the other range.
   bool isContainedIn(const VersionRange &other) const
   {
      if (isEmpty() || other.isAll()) {
         return true;
      }

      if (isAll() || other.isEmpty()) {
         return false;
      }
      // [v1, +Inf) is contained in [v2, +Inf) if v1 >= v2
      return getLowerEndpoint() >= other.getLowerEndpoint();
   }

   /// Mutates this range to be a best-effort underapproximation of
   /// the intersection of itself and other. This is the
   /// meet operation (greatest lower bound) in the version range lattice.
   void intersectWith(const VersionRange &other)
   {
      // With the existing lattice this operation is precise. If the lattice
      // is ever extended it is important that this operation be an
      // underapproximation of intersection.

      if (isEmpty() || other.isAll())
         return;

      if (isAll() || other.isEmpty()) {
         *this = other;
         return;
      }

      // The g.l.b of [v1, +Inf), [v2, +Inf) is [max(v1,v2), +Inf)
      const VersionTuple maxVersion =
            std::max(this->getLowerEndpoint(), other.getLowerEndpoint());

      setLowerEndpoint(maxVersion);
   }

   /// Mutates this range to be the union of itself and other. This is the
   /// join operator (least upper bound) in the version range lattice.
   void unionWith(const VersionRange &other)
   {
      // With the existing lattice this operation is precise. If the lattice
      // is ever extended it is important that this operation be an
      // overapproximation of union.
      if (isAll() || other.isEmpty()) {
         return;
      }

      if (isEmpty() || other.isAll()) {
         *this = other;
         return;
      }

      // The l.u.b of [v1, +Inf), [v2, +Inf) is [min(v1,v2), +Inf)
      const VersionTuple minVersion =
            std::min(this->getLowerEndpoint(), other.getLowerEndpoint());
      setLowerEndpoint(minVersion);
   }

   /// Mutates this range to be a best effort over-approximation of the
   /// intersection of the concretizations of this version range and other.
   void constrainWith(const VersionRange &other)
   {
      // We can use intersection for this because the lattice is multiplicative
      // with respect to concretization--that is, the concretization
      // of Range1 meet Range2 is equal to the intersection of the
      // concretization of Range1 and the concretization of Range2.
      // This will change if we add (-Inf, v) to our version range lattice.
      intersectWith(other);
   }

   /// Returns a version range representing all versions.
   static VersionRange all()
   {
      return VersionRange(ExtremalRange::All);
   }

   /// Returns a version range representing no versions.
   static VersionRange empty()
   {
      return VersionRange(ExtremalRange::Empty);
   }

   /// Returns a version range representing all versions greater than or equal
   /// to the passed-in version.
   static VersionRange allGTE(const VersionTuple &EndPoint)
   {
      return VersionRange(EndPoint);
   }

private:
   VersionRange(const VersionTuple &lowerEndpoint)
   {
      setLowerEndpoint(lowerEndpoint);
   }

   VersionRange(ExtremalRange extremalValue)
   {
      setExtremalRange(extremalValue);
   }

   void setExtremalRange(ExtremalRange version)
   {
      m_hasLowerEndpoint = 0;
      m_extremalValue = version;
   }

   void setLowerEndpoint(const VersionTuple &version)
   {
      m_hasLowerEndpoint = 1;
      m_lowerEndpoint = version;
   }
};

/// Records the reason a declaration is potentially unavailable.
class UnavailabilityReason
{
public:
   enum class Kind {
      /// The declaration is potentially unavailable because it requires an OS
      /// version range that is not guaranteed by the minimum deployment
      /// target.
      RequiresOSVersionRange,

      /// The declaration is potentially unavailable because it is explicitly
      /// weakly linked.
      ExplicitlyWeakLinked
   };

private:
   // A value of None indicates the declaration is potentially unavailable
   // because it is explicitly weak linked.
   std::optional<VersionRange> m_requiredDeploymentRange;

   UnavailabilityReason(const std::optional<VersionRange> &requiredDeploymentRange)
      : m_requiredDeploymentRange(requiredDeploymentRange)
   {}

public:
   static UnavailabilityReason explicitlyWeaklyLinked()
   {
      return UnavailabilityReason(std::nullopt);
   }

   static UnavailabilityReason requiresVersionRange(const VersionRange Range) {
      return UnavailabilityReason(Range);
   }

   Kind getReasonKind() const
   {
      if (m_requiredDeploymentRange.has_value()) {
         return Kind::RequiresOSVersionRange;
      } else {
         return Kind::ExplicitlyWeakLinked;
      }
   }

   const VersionRange &getRequiredOSVersionRange() const
   {
      assert(getReasonKind() == Kind::RequiresOSVersionRange);
      return m_requiredDeploymentRange.value();
   }
};

/// Represents everything that a particular chunk of code may assume about its
/// runtime environment.
///
/// The AvailabilityContext structure forms a [lattice][], which allows it to
/// have meaningful union and intersection operations ("join" and "meet"),
/// which use conservative approximations to prevent availability violations.
/// See #unionWith, #intersectWith, and #constrainWith.
///
/// [lattice]: http://mathworld.wolfram.com/Lattice.html
class AvailabilityContext
{
   VersionRange m_osVersion;
public:
   /// Creates a context that requires certain versions of the target OS.
   explicit AvailabilityContext(VersionRange osVersion)
      : m_osVersion(osVersion)
   {}

   /// Creates a context that imposes no constraints.
   ///
   /// \see isAlwaysAvailable
   static AvailabilityContext alwaysAvailable()
   {
      return AvailabilityContext(VersionRange::all());
   }

   /// Creates a context that can never actually occur.
   ///
   /// \see isKnownUnreachable
   static AvailabilityContext neverAvailable()
   {
      return AvailabilityContext(VersionRange::empty());
   }

   /// Returns the range of possible OS versions required by this context.
   VersionRange getOSVersion() const
   {
      return m_osVersion;
   }

   /// Returns true if \p other makes stronger guarantees than this context.
   ///
   /// That is, `a.isContainedIn(b)` implies `a.union(b) == b`.
   bool isContainedIn(AvailabilityContext other) const
   {
      return m_osVersion.isContainedIn(other.m_osVersion);
   }

   /// Returns true if this context has constraints that make it impossible to
   /// actually occur.
   ///
   /// For example, the else branch of a `#available` check for iOS 8.0 when the
   /// containing function already requires iOS 9.
   bool isKnownUnreachable() const
   {
      return m_osVersion.isEmpty();
   }

   /// Returns true if there are no constraints on this context; that is,
   /// nothing can be assumed.
   bool isAlwaysAvailable() const
   {
      return m_osVersion.isAll();
   }

   /// Produces an under-approximation of the intersection of the two
   /// availability contexts.
   ///
   /// That is, if the intersection can't be represented exactly, prefer
   /// treating some valid deployment environments as unavailable. This is the
   /// "meet" operation of the lattice.
   ///
   /// As an example, this is used when figuring out the required availability
   /// for a type that references multiple nominal decls.
   void intersectWith(AvailabilityContext other)
   {
      m_osVersion.intersectWith(other.getOSVersion());
   }

   /// Produces an over-approximation of the intersection of the two
   /// availability contexts.
   ///
   /// That is, if the intersection can't be represented exactly, prefer
   /// treating some invalid deployment environments as available.
   ///
   /// As an example, this is used for the true branch of `#available`.
   void constrainWith(AvailabilityContext other)
   {
      m_osVersion.constrainWith(other.getOSVersion());
   }

   /// Produces an over-approximation of the union of two availability contexts.
   ///
   /// That is, if the union can't be represented exactly, prefer treating
   /// some invalid deployment environments as available. This is the "join"
   /// operation of the lattice.
   ///
   /// As an example, this is used for the else branch of a conditional with
   /// multiple `#available` checks.
   void unionWith(AvailabilityContext other)
   {
      m_osVersion.unionWith(other.getOSVersion());
   }
};

class AvailabilityInference
{
public:
   /// Infers the common availability required to access an array of
   /// declarations and adds attributes reflecting that availability
   /// to toDecl.
   static void
   applyInferredAvailableAttrs(Decl *toDecl,
                               ArrayRef<const Decl *> inferredFromDecls,
                               AstContext &context);

   static AvailabilityContext inferForType(Type type);

   /// Returns the context where a declaration is available
   ///  We assume a declaration without an annotation is always available.
   static AvailabilityContext availableRange(const Decl *decl, AstContext &context);

   /// Returns the context for which the declaration
   /// is annotated as available, or None if the declaration
   /// has no availability annotation.
   static std::optional<AvailabilityContext> annotatedAvailableRange(const Decl *decl,
                                                                AstContext &context);

};

} // polar::ast

#endif // POLARPHP_AST_AVAILABILITY_H
