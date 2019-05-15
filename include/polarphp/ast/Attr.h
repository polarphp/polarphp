//===--- attr.h - Swift Language Attribute ASTs -----------------*- C++ -*-===//
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
// This file defines classes related to declaration attributes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ATTR_H
#define POLARPHP_AST_ATTR_H

#include "polarphp/basic/Uuid.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/VersionTuple.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/TrailingObjects.h"
#include "polarphp/utils/VersionTuple.h"
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/PlatformKind.h"
#include "polarphp/ast/TypeLoc.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/DeclNameLoc.h"
#include "polarphp/ast/OptimizationMode.h"
#include "polarphp/kernel/Version.h"

#include <optional>

namespace polar::ast {

using polar::basic::UUID;
using polar::basic::SmallVectorImpl;
using polar::basic::StringRef;
using polar::basic::OptionalTransformRange;
using polar::parser::SourceLoc;
using polar::parser::SourceRange;
using polar::parser::CharSourceRange;
using polar::basic::bitmax;
using polar::basic::count_bits_used;
using polar::utils::VersionTuple;

class AstPrinter;
class AstContext;
struct PrintOptions;
class Decl;
class AbstractFunctionDecl;
class FuncDecl;
class ClassDecl;
class GenericFunctionType;
class LazyConformanceLoader;
class TrailingWhereClause;

/// TypeAttributes - These are attributes that may be applied to types.
class TypeAttributes
{
   // Get a SourceLoc for every possible attribute that can be parsed in source.
   // the presence of the attribute is indicated by its location being set.
   SourceLoc m_attrLocs[TAK_Count];
public:
   /// m_atLoc - This is the location of the first '@' in the attribute specifier.
   /// If this is an empty attribute specifier, then this will be an invalid loc.
   SourceLoc m_atLoc;
   std::optional<StringRef> m_convention = std::nullopt;
   std::optional<StringRef> m_conventionWitnessMethodProtocol = std::nullopt;

   // For an opened existential type, the known ID.
   std::optional<UUID> m_openedID;

   TypeAttributes()
   {}

   bool isValid() const
   {
      return m_atLoc.isValid();
   }

   void clearAttribute(TypeAttrKind attr)
   {
      m_attrLocs[attr] = SourceLoc();
   }

   bool has(TypeAttrKind attr) const
   {
      return getLoc(attr).isValid();
   }

   SourceLoc getLoc(TypeAttrKind attr) const
   {
      return m_attrLocs[attr];
   }

   void setAttr(TypeAttrKind attr, SourceLoc loc)
   {
      assert(!loc.isInvalid() && "Cannot clear attribute with this method");
      m_attrLocs[attr] = loc;
   }

   void getAttrRanges(SmallVectorImpl<SourceRange> &ranges) const
   {
      for (auto loc : m_attrLocs) {
         if (loc.isValid()) {
            ranges.push_back(loc);
         }
      }
   }

   // This attribute list is empty if no attributes are specified.  Note that
   // the presence of the leading @ is not enough to tell, because we want
   // clients to be able to remove attributes they process until they get to
   // an empty list.
   bool empty() const
   {
      for (SourceLoc elt : m_attrLocs) {
         if (elt.isValid()) {
            return false;
         }
      }
      return true;
   }

   bool hasConvention() const
   {
      return m_convention.has_value();
   }

   StringRef getConvention() const
   {
      return *m_convention;
   }

   bool hasOpenedID() const
   {
      return m_openedID.has_value();
   }

   UUID getOpenedID() const
   {
      return *m_openedID;
   }

   /// Given a name like "autoclosure", return the type attribute ID that
   /// corresponds to it.  This returns TAK_Count on failure.
   ///
   static TypeAttrKind getAttrKindFromString(StringRef str);

   /// Return the name (like "autoclosure") for an attribute ID.
   static const char *getAttrName(TypeAttrKind kind);
};

class AttributeBase
{
public:
   /// The location of the '@'.
   const SourceLoc m_atLoc;

   /// The source range of the attribute.
   const SourceRange m_range;

   /// The location of the attribute.
   SourceLoc getLocation() const
   {
      return m_range.m_start;
   }

   /// Return the source range of the attribute.
   SourceRange getRange() const
   {
      return m_range;
   }

   SourceRange getRangeWithAt() const
   {
      if (m_atLoc.isValid()) {
         return {m_atLoc, m_range.m_end};
      }
      return m_range;
   }

   // Only allow allocation of attributes using the allocator in AstContext
   // or by doing a placement new.
   void *operator new(size_t bytes, AstContext &ctx,
                      unsigned alignment = alignof(AttributeBase));

   void operator delete(void *data) noexcept
   {}

   void *operator new(size_t bytes, void *men) noexcept
   {
      return men;
   }

   // Make vanilla new/delete illegal for attributes.
   void *operator new(size_t bytes) noexcept = delete;

   AttributeBase(const AttributeBase &) = delete;

protected:
   AttributeBase(SourceLoc atLoc, SourceRange range)
      : m_atLoc(atLoc),
        m_range(range)
   {}
};

class DeclAttributes;
enum class DeclKind : uint8_t;

/// Represents one declaration attribute.
class DeclAttribute : public AttributeBase
{
   friend class DeclAttributes;

protected:
   union {
      uint64_t OpaqueBits;

      POLAR_INLINE_BITFIELD_BASE(
            DeclAttribute, bitmax(NumDeclAttrKindBits,8)+1+1,
            Kind : bitmax(NumDeclAttrKindBits,8),
            // Whether this attribute was implicitly added.
            implicit : 1,

            Invalid : 1
            );

      POLAR_INLINE_BITFIELD(
            DynamicReplacementAttr, DeclAttribute, 1,
            /// Whether this attribute has location information that trails the main
            /// record, which contains the locations of the parentheses and any names.
            HasTrailingLocationInfo : 1
            );

      POLAR_INLINE_BITFIELD(
            AbstractAccessControlAttr, DeclAttribute, 3,
            AccessLevel : 3
            );

      POLAR_INLINE_BITFIELD_FULL(
            AlignmentAttr, DeclAttribute, 32,
            : NumPadBits,
            // The alignment value.
            Value : 32
            );

      POLAR_INLINE_BITFIELD(
            EffectsAttr, DeclAttribute, NumEffectsKindBits,
            kind : NumEffectsKindBits
            );

      POLAR_INLINE_BITFIELD(
            InlineAttr, DeclAttribute, NumInlineKindBits,
            kind : NumInlineKindBits
            );

      POLAR_INLINE_BITFIELD(
            OptimizeAttr, DeclAttribute, NumOptimizationModeBits,
            mode : NumOptimizationModeBits
            );

      POLAR_INLINE_BITFIELD_FULL(
            SpecializeAttr, DeclAttribute, 1+1+32,
            exported : 1,
            kind : 1,
            : NumPadBits,
            numRequirements : 32
            );
   } m_bits;

   DeclAttribute *m_next = nullptr;

   DeclAttribute(DeclAttrKind declAttrKind, SourceLoc atLoc, SourceRange range,
                 bool implicit) : AttributeBase(atLoc, range)
   {
      m_bits.OpaqueBits = 0;
      m_bits.DeclAttribute.Kind = static_cast<unsigned>(declAttrKind);
      m_bits.DeclAttribute.implicit = implicit;
      m_bits.DeclAttribute.Invalid = false;
   }

private:
   // NOTE: We cannot use DeclKind due to layering. Even if we could, there is no
   // guarantee that the first DeclKind starts at zero. This is only used to
   // build "OnXYZ" flags.
   enum class DeclKindIndex : unsigned
   {
#define DECL(Name, _) Name,
#define LAST_DECL(Name) Last_Decl = Name
#include "polarphp/ast/DeclNodesDefs.h"
   };

public:
   enum DeclAttrOptions : uint64_t
   {
      // There is one entry for each DeclKind, and some higher level buckets
      // below. These are used in attr.def to control which kinds of declarations
      // an attribute can be attached to.
#define DECL(Name, _) On##Name = 1ull << unsigned(DeclKindIndex::Name),
#include "polarphp/ast/DeclNodesDefs.h"

      // Abstract class aggregations for use in attr.def.
      OnValue = 0
#define DECL(Name, _)
#define VALUE_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDefs.h"
      ,

      OnNominalType = 0
#define DECL(Name, _)
#define NOMINAL_TYPE_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDefs.h"
      ,
      OnConcreteNominalType = OnNominalType & ~OnProtocol,
      OnGenericType = OnNominalType | OnTypeAlias,

      OnAbstractFunction = 0
#define DECL(Name, _)
#define ABSTRACT_FUNCTION_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDefs.h"
      ,

      OnOperator = 0
#define DECL(Name, _)
#define OPERATOR_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDefs.h"
      ,

      OnAnyDecl = 0
#define DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDefs.h"
      ,

      /// True if multiple instances of this attribute are allowed on a single
      /// declaration.
      AllowMultipleAttributes = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 1),

      /// True if this is a decl modifier - i.e., that it should not be spelled
      /// with an @.
      DeclModifier = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 2),

      /// True if this is a long attribute that should be printed on its own line.
      ///
      /// Currently has no effect on DeclModifier attributes.
      LongAttribute = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 3),

      /// True if this shouldn't be serialized.
      NotSerialized = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 4),

      /// True if this attribute is only valid when parsing a .sil file.
      SILOnly = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 5),

      /// The attribute should be reported by parser as unknown.
      RejectByParser = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 6),

      /// Whether client code cannot use the attribute.
      UserInaccessible = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 7),
   };

   POLAR_READNONE
   static uint64_t getOptions(DeclAttrKind kind);

   uint64_t getOptions() const
   {
      return getOptions(getKind());
   }

   /// Prints this attribute (if applicable), returning `true` if anything was
   /// printed.
   bool printImpl(AstPrinter &printer, const PrintOptions &options,
                  const Decl *decl = nullptr) const;

public:
   DeclAttrKind getKind() const
   {
      return static_cast<DeclAttrKind>(m_bits.DeclAttribute.Kind);
   }

   /// Whether this attribute was implicitly added.
   bool isImplicit() const
   {
      return m_bits.DeclAttribute.implicit;
   }

   /// Set whether this attribute was implicitly added.
   void setImplicit(bool implicit = true)
   {
      m_bits.DeclAttribute.implicit = implicit;
   }

   /// Returns true if this attribute was find to be invalid in some way by
   /// semantic analysis.  In that case, the attribute should not be considered,
   /// the attribute node should be only used to retrieve source information.
   bool isInvalid() const
   {
      return m_bits.DeclAttribute.Invalid;
   }

   void setInvalid()
   {
      m_bits.DeclAttribute.Invalid = true;
   }

   bool isValid() const
   {
      return !isInvalid();
   }

   /// Returns the address of the next pointer field.
   /// Used for object deserialization.
   DeclAttribute **getMutableNext()
   {
      return &m_next;
   }

   /// Print the attribute to the provided AstPrinter.
   void print(AstPrinter &printer, const PrintOptions &options,
              const Decl *decl = nullptr) const;

   /// Print the attribute to the provided stream.
   void print(RawOutStream &outStream, const Decl *decl = nullptr) const;

   /// Returns true if this attribute can appear on the specified decl.  This is
   /// controlled by the flags in attr.def.
   bool canAppearOnDecl(const Decl *decl) const
   {
      return canAttributeAppearOnDecl(getKind(), decl);
   }

   POLAR_READONLY
   static bool canAttributeAppearOnDecl(DeclAttrKind declAttrKind, const Decl *decl);

   /// Returns true if multiple instances of an attribute kind
   /// can appear on a declaration.
   static bool allowMultipleAttributes(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & AllowMultipleAttributes;
   }

   bool isLongAttribute() const
   {
      return isLongAttribute(getKind());
   }

   static bool isLongAttribute(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & LongAttribute;
   }

   static bool shouldBeRejectedByParser(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & RejectByParser;
   }

   static bool isSilOnly(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & SILOnly;
   }

   static bool isUserInaccessible(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & UserInaccessible;
   }

   bool isDeclModifier() const
   {
      return isDeclModifier(getKind());
   }
   static bool isDeclModifier(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & DeclModifier;
   }

   static bool isOnParam(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & OnParam;
   }

   static bool isOnFunc(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & OnFunc;
   }

   static bool isOnClass(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & OnClass;
   }

   static bool isNotSerialized(DeclAttrKind declAttrKind)
   {
      return getOptions(declAttrKind) & NotSerialized;
   }

   bool isNotSerialized() const
   {
      return isNotSerialized(getKind());
   }

   POLAR_READNONE
   static bool canAttributeAppearOnDeclKind(DeclAttrKind declAttrKind, DeclKind declKind);

   /// Returns the source name of the attribute, without the @ or any arguments.
   StringRef getAttrName() const;

   /// Given a name like "inline", return the decl attribute ID that corresponds
   /// to it.  Note that this is a many-to-one mapping, and that the identifier
   /// passed in may only be the first portion of the attribute (e.g. in the case
   /// of the 'unowned(unsafe)' attribute, the string passed in is 'unowned'.
   ///
   /// Also note that this recognizes both attributes like '@inline' (with no @)
   /// and decl modifiers like 'final'.  This returns DAK_Count on failure.
   ///
   static DeclAttrKind getAttrKindFromString(StringRef str);
};

/// Describes a "simple" declaration attribute that carries no data.
template<DeclAttrKind kind>
class SimpleDeclAttr : public DeclAttribute
{
public:
   SimpleDeclAttr(bool isImplicit)
      : DeclAttribute(kind, SourceLoc(), SourceLoc(), isImplicit)
   {}

   SimpleDeclAttr(SourceLoc atLoc, SourceLoc nameLoc)
      : DeclAttribute(kind, atLoc,
                      SourceRange(atLoc.isValid() ? atLoc : nameLoc, nameLoc),
                      /*implicit=*/false)
   {}

   SimpleDeclAttr(SourceLoc nameLoc)
      : DeclAttribute(kind, SourceLoc(), SourceRange(nameLoc, nameLoc),
                      /*implicit=*/false)
   {}

   static bool classOf(const DeclAttribute *deeclAttribute)
   {
      return deeclAttribute->getKind() == kind;
   }
};

// Declare typedefs for all of the simple declaration attributes.
#define SIMPLE_DECL_ATTR(_, CLASS, ...) \
 typedef SimpleDeclAttr<DAK_##CLASS> CLASS##Attr;
#include "polarphp/ast/AttrDefs.h"

/// Determine the result of comparing an availability attribute to a specific
/// platform or language version.
enum class AvailableVersionComparison
{
   /// The entity is guaranteed to be available.
   Available,

   /// The entity is never available.
   Unavailable,

   /// The entity might be unavailable at runtime, because it was introduced
   /// after the requested minimum platform version.
   PotentiallyUnavailable,

   /// The entity has been obsoleted.
   obsoleted,
};

/// Describes the platform-agnostic availability of a declaration.
enum class PlatformAgnosticAvailabilityKind
{
   /// The associated availability attribute is not platform-agnostic.
   None,
   /// The declaration is deprecated, but can still be used.
   deprecated,
   /// The declaration is unavailable in Swift, specifically
   UnavailableInSwift,
   /// The declaration is available in some but not all versions
   /// of Swift, as specified by the VersionTuple members.
   SwiftVersionSpecific,
   /// The declaration is available in some but not all versions
   /// of SwiftPM's PackageDescription library, as specified by
   /// the VersionTuple members.
   PackageDescriptionVersionSpecific,
   /// The declaration is unavailable for other reasons.
   Unavailable,
};

/// Defines the @available attribute.
class AvailableAttr : public DeclAttribute
{
public:
#define INIT_VER_TUPLE(X)\
   X(X.empty() ? std::optional<VersionTuple>() : X)

   AvailableAttr(SourceLoc AtLoc, SourceRange range,
                 PlatformKind platform,
                 StringRef message, StringRef rename,
                 const VersionTuple &introduced,
                 SourceRange introducedRange,
                 const VersionTuple &deprecated,
                 SourceRange deprecatedRange,
                 const VersionTuple &obsoleted,
                 SourceRange obsoletedRange,
                 PlatformAgnosticAvailabilityKind platformAgnostic,
                 bool implicit)
      : DeclAttribute(DAK_Available, AtLoc, range, implicit),
        message(message), rename(rename),
        INIT_VER_TUPLE(introduced), introducedRange(introducedRange),
        INIT_VER_TUPLE(deprecated), deprecatedRange(deprecatedRange),
        INIT_VER_TUPLE(obsoleted), obsoletedRange(obsoletedRange),
        platformAgnostic(platformAgnostic),
        platform(platform)
   {}

#undef INIT_VER_TUPLE

   /// The optional message.
   const StringRef message;

   /// An optional replacement string to emit in a fixit.  This allows simple
   /// declaration renames to be applied by Xcode.
   ///
   /// This should take the form of an operator, identifier, or full function
   /// name, optionally with a prefixed type, similar to the syntax used for
   /// the `NS_SWIFT_NAME` annotation in Objective-C.
   const StringRef rename;

   /// Indicates when the symbol was introduced.
   const std::optional<VersionTuple> introduced;

   /// Indicates where the introduced version was specified.
   const SourceRange introducedRange;

   /// Indicates when the symbol was deprecated.
   const std::optional<VersionTuple> deprecated;

   /// Indicates where the deprecated version was specified.
   const SourceRange deprecatedRange;

   /// Indicates when the symbol was obsoleted.
   const std::optional<VersionTuple> obsoleted;

   /// Indicates where the obsoleted version was specified.
   const SourceRange obsoletedRange;

   /// Indicates if the declaration has platform-agnostic availability.
   const PlatformAgnosticAvailabilityKind platformAgnostic;

   /// The platform of the availability.
   const PlatformKind platform;

   /// Whether this is a language-version-specific entity.
   bool isLanguageVersionSpecific() const;

   /// Whether this is a PackageDescription version specific entity.
   bool isPackageDescriptionVersionSpecific() const;

   /// Whether this is an unconditionally unavailable entity.
   bool isUnconditionallyUnavailable() const;

   /// Whether this is an unconditionally deprecated entity.
   bool isUnconditionallyDeprecated() const;

   /// Returns the platform-agnostic availability.
   PlatformAgnosticAvailabilityKind getPlatformAgnosticAvailability() const
   {
      return platformAgnostic;
   }

   /// Determine if a given declaration should be considered unavailable given
   /// the current settings.
   ///
   /// \returns The attribute responsible for making the declaration unavailable.
   static const AvailableAttr *isUnavailable(const Decl *decl);

   /// Returns true if the availability applies to a specific
   /// platform.
   bool hasPlatform() const
   {
      return platform != PlatformKind::none;
   }

   /// Returns the string for the platform of the attribute.
   StringRef platformString() const
   {
      return platform_string(platform);
   }

   /// Returns the human-readable string for the platform of the attribute.
   StringRef prettyPlatformString() const
   {
      return pretty_platform_string(platform);
   }

   /// Returns true if this attribute is active given the current platform.
   bool isActivePlatform(const AstContext &ctx) const;

   /// Returns the active version from the AST context corresponding to
   /// the available kind. For example, this will return the effective language
   /// version for swift version-specific availability kind, PackageDescription
   /// version for PackageDescription version-specific availability.
   VersionTuple getActiveVersion(const AstContext &ctx) const;

   /// Compare this attribute's version information against the platform or
   /// language version (assuming the this attribute pertains to the active
   /// platform).
   AvailableVersionComparison getVersionAvailability(const AstContext &ctx) const;

   /// Create an AvailableAttr that indicates specific availability
   /// for all platforms.
   static AvailableAttr *
   createPlatformAgnostic(AstContext &C, StringRef message, StringRef rename = "",
                          PlatformAgnosticAvailabilityKind reason
                          = PlatformAgnosticAvailabilityKind::Unavailable,
                          VersionTuple obsoleted
                          = VersionTuple());

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_Available;
   }
};


/// Represents any sort of access control modifier.
class AbstractAccessControlAttr : public DeclAttribute
{
protected:
   AbstractAccessControlAttr(DeclAttrKind DK, SourceLoc atLoc, SourceRange range,
                             AccessLevel access, bool implicit)
      : DeclAttribute(DK, atLoc, range, implicit)
   {
      m_bits.AbstractAccessControlAttr.AccessLevel = static_cast<unsigned>(access);
      assert(getAccess() == access && "not enough bits for access control");
   }

public:
   AccessLevel getAccess() const
   {
      return static_cast<AccessLevel>(m_bits.AbstractAccessControlAttr.AccessLevel);
   }

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_AccessControl ||
            declAttr->getKind() == DAK_SetterAccess;
   }
};

/// Represents a 'private', 'internal', or 'public' marker on a declaration.
class AccessControlAttr : public AbstractAccessControlAttr
{
public:
   AccessControlAttr(SourceLoc atLoc, SourceRange range, AccessLevel access,
                     bool implicit = false)
      : AbstractAccessControlAttr(DAK_AccessControl, atLoc, range, access,
                                  implicit)
   {}

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_AccessControl;
   }
};

/// Represents a 'private', 'internal', or 'public' marker for a setter on a
/// declaration.
class SetterAccessAttr : public AbstractAccessControlAttr
{
public:
   SetterAccessAttr(SourceLoc atLoc, SourceRange range,
                    AccessLevel access, bool implicit = false)
      : AbstractAccessControlAttr(DAK_SetterAccess, atLoc, range, access,
                                  implicit)
   {}

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_SetterAccess;
   }
};

/// Represents an inline attribute.
class InlineAttr : public DeclAttribute
{
public:
   InlineAttr(SourceLoc atLoc, SourceRange range, InlineKind kind)
      : DeclAttribute(DAK_Inline, atLoc, range, /*implicit=*/false)
   {
      m_bits.InlineAttr.kind = unsigned(kind);
   }

   InlineAttr(InlineKind kind)
      : InlineAttr(SourceLoc(), SourceRange(), kind)
   {}

   InlineKind getKind() const
   {
      return InlineKind(m_bits.InlineAttr.kind);
   }

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_Inline;
   }
};

/// Represents the optimize attribute.
class OptimizeAttr : public DeclAttribute
{
public:
   OptimizeAttr(SourceLoc atLoc, SourceRange range, OptimizationMode mode)
      : DeclAttribute(DAK_Optimize, atLoc, range, /*implicit=*/false)
   {
      m_bits.OptimizeAttr.mode = unsigned(mode);
   }

   OptimizeAttr(OptimizationMode mode)
      : OptimizeAttr(SourceLoc(), SourceRange(), mode)
   {}

   OptimizationMode getMode() const
   {
      return OptimizationMode(m_bits.OptimizeAttr.mode);
   }

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_Optimize;
   }
};

/// Represents the side effects attribute.
class EffectsAttr : public DeclAttribute
{
public:
   EffectsAttr(SourceLoc atLoc, SourceRange range, EffectsKind kind)
      : DeclAttribute(DAK_Effects, atLoc, range, /*implicit=*/false)
   {
      m_bits.EffectsAttr.kind = unsigned(kind);
   }

   EffectsAttr(EffectsKind kind)
      : EffectsAttr(SourceLoc(), SourceRange(), kind)
   {}

   EffectsKind getKind() const
   {
      return EffectsKind(m_bits.EffectsAttr.kind);
   }

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_Effects;
   }
};

/// Defines the attribute that we use to model documentation comments.
class RawDocCommentAttr : public DeclAttribute
{
   /// Source range of the attached comment.  This comment is located before
   /// the declaration.
   CharSourceRange CommentRange;

public:
   RawDocCommentAttr(CharSourceRange CommentRange)
      : DeclAttribute(DAK_RawDocComment, SourceLoc(), SourceRange(),
                      /*implicit=*/false),
        CommentRange(CommentRange) {}

   CharSourceRange getCommentRange() const { return CommentRange; }

   static bool classOf(const DeclAttribute *DA) {
      return DA->getKind() == DAK_RawDocComment;
   }
};

/// The @_implements attribute, which treats a decl as the implementation for
/// some named protocol requirement (but otherwise not-visible by that name).
class ImplementsAttr : public DeclAttribute
{

   TypeLoc m_protocolType;
   DeclName m_memberName;
   DeclNameLoc m_memberNameLoc;

public:
   ImplementsAttr(SourceLoc atLoc, SourceRange range,
                  TypeLoc protocolType,
                  DeclName memberName,
                  DeclNameLoc memberNameLoc);

   static ImplementsAttr *create(AstContext &ctx, SourceLoc atLoc,
                                 SourceRange range,
                                 TypeLoc protocolType,
                                 DeclName memberName,
                                 DeclNameLoc memberNameLoc);

   TypeLoc getProtocolType() const;
   TypeLoc &getProtocolType();
   DeclName getMemberName() const
   {
      return m_memberName;
   }

   DeclNameLoc getMemberNameLoc() const
   {
      return m_memberNameLoc;
   }

   static bool classOf(const DeclAttribute *declAttr)
   {
      return declAttr->getKind() == DAK_Implements;
   }
};

/// Attributes that may be applied to declarations.
class DeclAttributes
{
   /// Linked list of declaration attributes.
   DeclAttribute *m_declAttrs;

public:
   DeclAttributes()
      : m_declAttrs(nullptr)
   {}

   bool isEmpty() const
   {
      return m_declAttrs == nullptr;
   }

   void getAttrRanges(SmallVectorImpl<SourceRange> &ranges) const
   {
      for (auto attr : *this) {
         auto range = attr->getRangeWithAt();
         if (range.isValid()) {
            ranges.push_back(range);
         }
      }
   }

   /// If this attribute set has a prefix/postfix attribute on it, return this.
   //   UnaryOperatorKind getUnaryOperatorKind() const
   //   {
   //      if (hasAttribute<PrefixAttr>()) {
   //         return UnaryOperatorKind::Prefix;
   //      }
   //      if (hasAttribute<PostfixAttr>()) {
   //         return UnaryOperatorKind::Postfix;
   //      }
   //      return UnaryOperatorKind::None;
   //   }

   bool isUnavailable(const AstContext &ctx) const
   {
      return getUnavailable(ctx) != nullptr;
   }

   /// Determine whether there is a swiftVersionSpecific attribute that's
   /// unavailable relative to the provided language version.
   bool
   isUnavailableInPolarVersion(const polar::version::Version &effectiveVersion) const;

   /// Returns the first @available attribute that indicates
   /// a declaration is unavailable, or null otherwise.
   const AvailableAttr *getUnavailable(const AstContext &ctx) const;

   /// Returns the first @available attribute that indicates
   /// a declaration is deprecated on all deployment targets, or null otherwise.
   const AvailableAttr *getDeprecated(const AstContext &ctx) const;

   void dump(const Decl *decl = nullptr) const;
   void print(AstPrinter &printer, const PrintOptions &options,
              const Decl *decl = nullptr) const;

   template <typename T, typename DERIVED>
   class IteratorBase : public std::iterator<std::forward_iterator_tag, T *>
   {
      T *m_impl;
   public:
      explicit IteratorBase(T *impl)
         : m_impl(impl)
      {}

      DERIVED &operator++()
      {
         m_impl = m_impl->m_next;
         return (DERIVED&)*this;
      }

      bool operator==(const IteratorBase &other) const
      {
         return other.m_impl == m_impl;
      }

      bool operator!=(const IteratorBase &other) const
      {
         return other.m_impl != m_impl;
      }

      T *operator*() const
      {
         return m_impl;
      }

      T &operator->() const
      {
         return *m_impl;
      }
   };

   /// Add a constructed DeclAttribute to this list.
   void add(DeclAttribute *attr)
   {
      attr->m_next = m_declAttrs;
      m_declAttrs = attr;
   }

   // Iterator interface over DeclAttribute objects.
   class Iterator : public IteratorBase<DeclAttribute, Iterator>
   {
   public:
      explicit Iterator(DeclAttribute *impl) : IteratorBase(impl)
      {}
   };

   class ConstIterator : public IteratorBase<const DeclAttribute,
         ConstIterator>
   {
   public:
      explicit ConstIterator(const DeclAttribute *impl)
         : IteratorBase(impl)
      {}
   };

   Iterator begin()
   {
      return Iterator(m_declAttrs);
   }

   Iterator end()
   {
      return Iterator(nullptr);
   }

   ConstIterator begin() const
   {
      return ConstIterator(m_declAttrs);
   }

   ConstIterator end() const
   {
      return ConstIterator(nullptr);
   }

   /// Retrieve the first attribute of the given attribute class.
   template <typename ATTR>
   const ATTR *getAttribute(bool allowInvalid = false) const
   {
      return const_cast<DeclAttributes *>(this)->getAttribute<ATTR>(allowInvalid);
   }

   template <typename ATTR>
   ATTR *getAttribute(bool allowInvalid = false)
   {
//      for (auto attr : *this) {
//         if (auto *specificAttr = dyn_cast<ATTR>(attr)) {
//            if (specificAttr->isValid() || allowInvalid) {
//               return specificAttr;
//            }
//         }
//      }
      return nullptr;
   }

   /// Determine whether there is an attribute with the given attribute class.
   template <typename ATTR>
   bool hasAttribute(bool allowInvalid = false) const {
      return getAttribute<ATTR>(allowInvalid) != nullptr;
   }

   /// Retrieve the first attribute with the given kind.
   const DeclAttribute *getAttribute(DeclAttrKind DK,
                                     bool allowInvalid = false) const
   {
      for (auto attr : *this)
         if (attr->getKind() == DK && (attr->isValid() || allowInvalid))
            return attr;
      return nullptr;
   }

private:
   /// Predicate used to filter MatchingAttributeRange.
   template <typename ATTR, bool allowInvalid>
   struct ToAttributeKind
   {
      ToAttributeKind() {}

      std::optional<const ATTR *>
      operator()(const DeclAttribute *attr) const
      {
         if (isa<ATTR>(attr) && (attr->isValid() || allowInvalid)) {
            return cast<ATTR>(attr);
         }
         return std::nullopt;
      }
   };

public:
   template <typename ATTR, bool allowInvalid>
   using AttributeKindRange =
   OptionalTransformRange<polar::basic::IteratorRange<ConstIterator>,
   ToAttributeKind<ATTR, allowInvalid>,
   ConstIterator>;

   /// Return a range with all attributes in DeclAttributes with AttrKind
   /// ATTR.
   template <typename ATTR, bool allowInvalid = false>
   AttributeKindRange<ATTR, allowInvalid> getAttributes() const
   {
      return AttributeKindRange<ATTR, allowInvalid>(
               polar::basic::make_range(begin(), end()), ToAttributeKind<ATTR, allowInvalid>());
   }

   // Remove the given attribute from the list of attributes. Used when
   // the attribute was semantically invalid.
   void removeAttribute(const DeclAttribute *attr)
   {
      // If it's the first attribute, remove it.
      if (m_declAttrs == attr) {
         m_declAttrs = attr->m_next;
         return;
      }

      // Otherwise, find it in the list. This is inefficient, but rare.
      for (auto **prev = &m_declAttrs; *prev; prev = &(*prev)->m_next) {
         if ((*prev)->m_next == attr) {
            (*prev)->m_next = attr->m_next;
            return;
         }
      }
      polar_unreachable("Attribute not found for removal");
   }

   /// Set the raw chain of attributes.  Used for deserialization.
   void setRawAttributeChain(DeclAttribute *Chain)
   {
      m_declAttrs = Chain;
   }

   SourceLoc getStartLoc(bool forModifiers = false) const;
};

} // polar::ast

#endif // POLARPHP_AST_ATTR_H
