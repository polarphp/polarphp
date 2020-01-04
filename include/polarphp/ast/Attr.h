//===--- Attr.h - Swift Language Attribute ASTs -----------------*- C++ -*-===//
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
//
// This file defines classes related to declaration attributes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ATTR_H
#define POLARPHP_AST_ATTR_H

#include "polarphp/basic/Debug.h"
#include "polarphp/basic/InlineBitfield.h"
#include "polarphp/basic/SourceLoc.h"
#include "polarphp/basic/Uuid.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/basic/Range.h"
#include "polarphp/basic/OptimizationMode.h"
#include "polarphp/basic/Version.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/AutoDiff.h"
#include "polarphp/ast/ConcreteDeclRef.h"
#include "polarphp/ast/DeclNameLoc.h"
#include "polarphp/ast/KnownInterfaces.h"
#include "polarphp/ast/Ownership.h"
#include "polarphp/ast/PlatformKind.h"
#include "polarphp/ast/Requirement.h"
#include "polarphp/ast/TrailingCallArguments.h"
#include "polarphp/ast/TypeLoc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TrailingObjects.h"
#include "llvm/Support/VersionTuple.h"

namespace polar {

class AstPrinter;
class AstContext;
struct PrintOptions;
class Decl;
class AbstractFunctionDecl;
class FuncDecl;
class ClassDecl;
class GenericFunctionType;
class LazyConformanceLoader;
class LazyMemberLoader;
class PatternBindingInitializer;
class TrailingWhereClause;

using llvm::dyn_cast;
using polar::UUID;
using polar::bitmax;
using polar::NumOptimizationModeBits;
using polar::CharSourceRange;
using polar::OptimizationMode;
using polar::OptionalTransformRange;

/// TypeAttributes - These are attributes that may be applied to types.
class TypeAttributes {
   // Get a SourceLoc for every possible attribute that can be parsed in source.
   // the presence of the attribute is indicated by its location being set.
   SourceLoc AttrLocs[TAK_Count];
public:
   /// AtLoc - This is the location of the first '@' in the attribute specifier.
   /// If this is an empty attribute specifier, then this will be an invalid loc.
   SourceLoc AtLoc;
   Optional<StringRef> convention = None;
   Optional<StringRef> conventionWitnessMethodInterface = None;

   // Indicates whether the type's '@differentiable' attribute has a 'linear'
   // argument.
   bool linear = false;

   // For an opened existential type, the known ID.
   Optional<UUID> OpenedID;

   // For a reference to an opaque return type, the mangled name and argument
   // index into the generic signature.
   struct OpaqueReturnTypeRef {
      StringRef mangledName;
      unsigned index;
   };
   Optional<OpaqueReturnTypeRef> OpaqueReturnTypeOf;

   TypeAttributes() {}

   bool isValid() const { return AtLoc.isValid(); }

   bool isLinear() const {
      assert(
               (!linear ||
                (linear && has(TAK_differentiable))) &&
               "Linear shouldn't have been true if there's no `@differentiable`");
      return linear;
   }

   void clearAttribute(TypeAttrKind A) {
      AttrLocs[A] = SourceLoc();
   }

   bool has(TypeAttrKind A) const {
      return getLoc(A).isValid();
   }

   SourceLoc getLoc(TypeAttrKind A) const {
      return AttrLocs[A];
   }

   void setOpaqueReturnTypeOf(StringRef mangling, unsigned index) {
      OpaqueReturnTypeOf = OpaqueReturnTypeRef{mangling, index};
   }

   void setAttr(TypeAttrKind A, SourceLoc L) {
      assert(!L.isInvalid() && "Cannot clear attribute with this method");
      AttrLocs[A] = L;
   }

   void getAttrLocs(SmallVectorImpl<SourceLoc> &Locs) const {
      for (auto Loc : AttrLocs) {
         if (Loc.isValid())
            Locs.push_back(Loc);
      }
   }

   // This attribute list is empty if no attributes are specified.  Note that
   // the presence of the leading @ is not enough to tell, because we want
   // clients to be able to remove attributes they process until they get to
   // an empty list.
   bool empty() const {
      for (SourceLoc elt : AttrLocs)
         if (elt.isValid())
            return false;

      return true;
   }

   bool hasConvention() const { return convention.hasValue(); }
   StringRef getConvention() const { return *convention; }

   bool hasOwnership() const {
      return getOwnership() != ReferenceOwnership::Strong;
   }
   ReferenceOwnership getOwnership() const {
#define REF_STORAGE(Name, name, ...) \
   if (has(TAK_pil_##name)) return ReferenceOwnership::Name;
#include "polarphp/ast/ReferenceStorageDef.h"
      return ReferenceOwnership::Strong;
   }

   void clearOwnership() {
#define REF_STORAGE(Name, name, ...) \
   clearAttribute(TAK_pil_##name);
#include "polarphp/ast/ReferenceStorageDef.h"
   }

   bool hasOpenedID() const { return OpenedID.hasValue(); }
   UUID getOpenedID() const { return *OpenedID; }

   /// Given a name like "autoclosure", return the type attribute ID that
   /// corresponds to it.  This returns TAK_Count on failure.
   ///
   static TypeAttrKind getAttrKindFromString(StringRef Str);

   /// Return the name (like "autoclosure") for an attribute ID.
   static const char *getAttrName(TypeAttrKind kind);
};

class alignas(1 << AttrAlignInBits) AttributeBase {
   public:
   /// The location of the '@'.
   const SourceLoc AtLoc;

   /// The source range of the attribute.
   const SourceRange Range;

   /// The location of the attribute.
   SourceLoc getLocation() const { return Range.start; }

   /// Return the source range of the attribute.
   SourceRange getRange() const { return Range; }

   SourceRange getRangeWithAt() const {
      if (AtLoc.isValid())
         return {AtLoc, Range.end};
      return Range;
   }

   // Only allow allocation of attributes using the allocator in AstContext
   // or by doing a placement new.
   void *operator new(size_t Bytes, AstContext &C,
                      unsigned Alignment = alignof(AttributeBase));

   void operator delete(void *Data) noexcept { }
   void *operator new(size_t Bytes, void *Mem) noexcept { return Mem; }

   // Make vanilla new/delete illegal for attributes.
   void *operator new(size_t Bytes) noexcept = delete;

   AttributeBase(const AttributeBase &) = delete;

   protected:
   AttributeBase(SourceLoc AtLoc, SourceRange Range)
      : AtLoc(AtLoc), Range(Range) {}
};

class DeclAttributes;
enum class DeclKind : uint8_t;

/// Represents one declaration attribute.
class DeclAttribute : public AttributeBase {
   friend class DeclAttributes;

protected:
   union {
      uint64_t OpaqueBits;

      POLAR_INLINE_BITFIELD_BASE(DeclAttribute, bitmax(NumDeclAttrKindBits,8)+1+1,
                                 Kind : bitmax(NumDeclAttrKindBits,8),
                                 // Whether this attribute was implicitly added.
                                 Implicit : 1,

                                 Invalid : 1
                                 );

      POLAR_INLINE_BITFIELD(ObjCAttr, DeclAttribute, 1+1+1,
                            /// Whether this attribute has location information that trails the main
                            /// record, which contains the locations of the parentheses and any names.
                            HasTrailingLocationInfo : 1,

                            /// Whether the name is implicit, produced as the result of caching.
                            ImplicitName : 1,

                            /// Whether the @objc was inferred using Swift 3's deprecated inference
                            /// rules.
                            Swift3Inferred : 1
                            );

      POLAR_INLINE_BITFIELD(DynamicReplacementAttr, DeclAttribute, 1,
                            /// Whether this attribute has location information that trails the main
                            /// record, which contains the locations of the parentheses and any names.
                            HasTrailingLocationInfo : 1
                            );

      POLAR_INLINE_BITFIELD(AbstractAccessControlAttr, DeclAttribute, 3,
                            AccessLevel : 3
                            );

      POLAR_INLINE_BITFIELD_FULL(AlignmentAttr, DeclAttribute, 32,
                                 : NumPadBits,
                                 // The alignment value.
                                 Value : 32
                                 );

      POLAR_INLINE_BITFIELD(ClangImporterSynthesizedTypeAttr, DeclAttribute, 1,
                            kind : 1
                            );

      POLAR_INLINE_BITFIELD(EffectsAttr, DeclAttribute, NumEffectsKindBits,
                            kind : NumEffectsKindBits
                            );

      POLAR_INLINE_BITFIELD(InlineAttr, DeclAttribute, NumInlineKindBits,
                            kind : NumInlineKindBits
                            );

      POLAR_INLINE_BITFIELD(OptimizeAttr, DeclAttribute, NumOptimizationModeBits,
                            mode : NumOptimizationModeBits
                            );

      POLAR_INLINE_BITFIELD(ReferenceOwnershipAttr, DeclAttribute,
                            NumReferenceOwnershipBits,
                            ownership : NumReferenceOwnershipBits
                            );

      POLAR_INLINE_BITFIELD(SpecializeAttr, DeclAttribute, 1+1,
                            exported : 1,
                            kind : 1
                            );

      POLAR_INLINE_BITFIELD(SynthesizedInterfaceAttr, DeclAttribute,
                            NumKnownInterfaceKindBits,
                            kind : NumKnownInterfaceKindBits
                            );
   } Bits;

   DeclAttribute *Next = nullptr;

   DeclAttribute(DeclAttrKind DK, SourceLoc AtLoc, SourceRange Range,
                 bool Implicit) : AttributeBase(AtLoc, Range) {
      Bits.OpaqueBits = 0;
      Bits.DeclAttribute.Kind = static_cast<unsigned>(DK);
      Bits.DeclAttribute.Implicit = Implicit;
      Bits.DeclAttribute.Invalid = false;
   }

private:
   // NOTE: We cannot use DeclKind due to layering. Even if we could, there is no
   // guarantee that the first DeclKind starts at zero. This is only used to
   // build "OnXYZ" flags.
   enum class DeclKindIndex : unsigned {
#define DECL(Name, _) Name,
#define LAST_DECL(Name) Last_Decl = Name
#include "polarphp/ast/DeclNodesDef.h"
   };

public:
   enum DeclAttrOptions : uint64_t {
      // There is one entry for each DeclKind, and some higher level buckets
      // below. These are used in Attr.def to control which kinds of declarations
      // an attribute can be attached to.
#define DECL(Name, _) On##Name = 1ull << unsigned(DeclKindIndex::Name),
#include "polarphp/ast/DeclNodesDef.h"

      // Abstract class aggregations for use in Attr.def.
      OnValue = 0
#define DECL(Name, _)
#define VALUE_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDef.h"
      ,

      OnNominalType = 0
#define DECL(Name, _)
#define NOMINAL_TYPE_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDef.h"
      ,
      OnConcreteNominalType = OnNominalType & ~OnInterface,
      OnGenericType = OnNominalType | OnTypeAlias,

      OnAbstractFunction = 0
#define DECL(Name, _)
#define ABSTRACT_FUNCTION_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDef.h"
      ,

      OnOperator = 0
#define DECL(Name, _)
#define OPERATOR_DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDef.h"
      ,

      OnAnyDecl = 0
#define DECL(Name, _) |On##Name
#include "polarphp/ast/DeclNodesDef.h"
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

      /// True if this attribute is only valid when parsing a .pil file.
      PILOnly = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 5),

      /// The attribute should be reported by parser as unknown.
      RejectByParser = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 6),

      /// Whether client code cannot use the attribute.
      UserInaccessible = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 7),

      /// Whether adding this attribute can break API
      APIBreakingToAdd = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 8),

      /// Whether removing this attribute can break API
      APIBreakingToRemove = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 9),

      /// Whether adding this attribute can break ABI
      ABIBreakingToAdd = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 10),

      /// Whether removing this attribute can break ABI
      ABIBreakingToRemove = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 11),

      /// The opposite of APIBreakingToAdd
      APIStableToAdd = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 12),

      /// The opposite of APIBreakingToRemove
      APIStableToRemove = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 13),

      /// The opposite of ABIBreakingToAdd
      ABIStableToAdd = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 14),

      /// The opposite of ABIBreakingToRemove
      ABIStableToRemove = 1ull << (unsigned(DeclKindIndex::Last_Decl) + 15),
   };

   LLVM_READNONE
   static uint64_t getOptions(DeclAttrKind DK);

   uint64_t getOptions() const {
      return getOptions(getKind());
   }

   /// Prints this attribute (if applicable), returning `true` if anything was
   /// printed.
   bool printImpl(AstPrinter &Printer, const PrintOptions &Options,
                  const Decl *D = nullptr) const;

public:
   DeclAttrKind getKind() const {
      return static_cast<DeclAttrKind>(Bits.DeclAttribute.Kind);
   }

   /// Whether this attribute was implicitly added.
   bool isImplicit() const { return Bits.DeclAttribute.Implicit; }

   /// Set whether this attribute was implicitly added.
   void setImplicit(bool Implicit = true) {
      Bits.DeclAttribute.Implicit = Implicit;
   }

   /// Returns true if this attribute was find to be invalid in some way by
   /// semantic analysis.  In that case, the attribute should not be considered,
   /// the attribute node should be only used to retrieve source information.
   bool isInvalid() const { return Bits.DeclAttribute.Invalid; }
   void setInvalid() { Bits.DeclAttribute.Invalid = true; }

   bool isValid() const { return !isInvalid(); }

   /// Returns the address of the next pointer field.
   /// Used for object deserialization.
   DeclAttribute **getMutableNext() {
      return &Next;
   }

   /// Print the attribute to the provided AstPrinter.
   void print(AstPrinter &Printer, const PrintOptions &Options,
              const Decl *D = nullptr) const;

   /// Print the attribute to the provided stream.
   void print(llvm::raw_ostream &OS, const Decl *D = nullptr) const;

   /// Returns true if this attribute can appear on the specified decl.  This is
   /// controlled by the flags in Attr.def.
   bool canAppearOnDecl(const Decl *D) const {
      return canAttributeAppearOnDecl(getKind(), D);
   }

   LLVM_READONLY
   static bool canAttributeAppearOnDecl(DeclAttrKind DK, const Decl *D);

   /// Returns true if multiple instances of an attribute kind
   /// can appear on a declaration.
   static bool allowMultipleAttributes(DeclAttrKind DK) {
      return getOptions(DK) & AllowMultipleAttributes;
   }

   bool isLongAttribute() const {
      return isLongAttribute(getKind());
   }
   static bool isLongAttribute(DeclAttrKind DK) {
      return getOptions(DK) & LongAttribute;
   }

   static bool shouldBeRejectedByParser(DeclAttrKind DK) {
      return getOptions(DK) & RejectByParser;
   }

   static bool isSilOnly(DeclAttrKind DK) {
      return getOptions(DK) & PILOnly;
   }

   static bool isUserInaccessible(DeclAttrKind DK) {
      return getOptions(DK) & UserInaccessible;
   }

   static bool isAddingBreakingABI(DeclAttrKind DK) {
      return getOptions(DK) & ABIBreakingToAdd;
   }

#define DECL_ATTR(_, CLASS, OPTIONS, ...)                                                         \
   static constexpr bool isOptionSetFor##CLASS(DeclAttrOptions Bit) {                              \
   return (OPTIONS) & Bit;                                                                       \
}
#include "polarphp/ast/AttrDef.h"

   static bool isAddingBreakingAPI(DeclAttrKind DK) {
      return getOptions(DK) & APIBreakingToAdd;
   }

   static bool isRemovingBreakingABI(DeclAttrKind DK) {
      return getOptions(DK) & ABIBreakingToRemove;
   }
   static bool isRemovingBreakingAPI(DeclAttrKind DK) {
      return getOptions(DK) & APIBreakingToRemove;
   }

   bool isDeclModifier() const {
      return isDeclModifier(getKind());
   }
   static bool isDeclModifier(DeclAttrKind DK) {
      return getOptions(DK) & DeclModifier;
   }

   static bool isOnParam(DeclAttrKind DK) {
      return getOptions(DK) & OnParam;
   }

   static bool isOnFunc(DeclAttrKind DK) {
      return getOptions(DK) & OnFunc;
   }

   static bool isOnClass(DeclAttrKind DK) {
      return getOptions(DK) & OnClass;
   }

   static bool isNotSerialized(DeclAttrKind DK) {
      return getOptions(DK) & NotSerialized;
   }
   bool isNotSerialized() const {
      return isNotSerialized(getKind());
   }

   LLVM_READNONE
   static bool canAttributeAppearOnDeclKind(DeclAttrKind DAK, DeclKind DK);

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
   static DeclAttrKind getAttrKindFromString(StringRef Str);
};

/// Describes a "simple" declaration attribute that carries no data.
template<DeclAttrKind Kind>
class SimpleDeclAttr : public DeclAttribute {
public:
   SimpleDeclAttr(bool IsImplicit)
      : DeclAttribute(Kind, SourceLoc(), SourceLoc(), IsImplicit) {}

   SimpleDeclAttr(SourceLoc AtLoc, SourceLoc NameLoc)
      : DeclAttribute(Kind, AtLoc,
                      SourceRange(AtLoc.isValid() ? AtLoc : NameLoc, NameLoc),
                      /*Implicit=*/false) { }
   SimpleDeclAttr(SourceLoc NameLoc)
      : DeclAttribute(Kind, SourceLoc(), SourceRange(NameLoc, NameLoc),
                      /*Implicit=*/false) { }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == Kind;
   }
};

// Declare typedefs for all of the simple declaration attributes.
#define SIMPLE_DECL_ATTR(_, CLASS, ...) \
   typedef SimpleDeclAttr<DAK_##CLASS> CLASS##Attr;
#include "polarphp/ast/AttrDef.h"

/// Defines the @_pilgen_name attribute.
class PILGenNameAttr : public DeclAttribute {
public:
   PILGenNameAttr(StringRef Name, SourceLoc AtLoc, SourceRange Range, bool Implicit)
      : DeclAttribute(DAK_PILGenName, AtLoc, Range, Implicit),
        Name(Name) {}

   PILGenNameAttr(StringRef Name, bool Implicit)
      : PILGenNameAttr(Name, SourceLoc(), SourceRange(), /*Implicit=*/true) {}

   /// The symbol name.
   const StringRef Name;

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_PILGenName;
   }
};

/// Defines the @_cdecl attribute.
class CDeclAttr : public DeclAttribute {
public:
   CDeclAttr(StringRef Name, SourceLoc AtLoc, SourceRange Range, bool Implicit)
      : DeclAttribute(DAK_CDecl, AtLoc, Range, Implicit),
        Name(Name) {}

   CDeclAttr(StringRef Name, bool Implicit)
      : CDeclAttr(Name, SourceLoc(), SourceRange(), /*Implicit=*/true) {}

   /// The symbol name.
   const StringRef Name;

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_CDecl;
   }
};

/// Defines the @_semantics attribute.
class SemanticsAttr : public DeclAttribute {
public:
   SemanticsAttr(StringRef Value, SourceLoc AtLoc, SourceRange Range,
                 bool Implicit)
      : DeclAttribute(DAK_Semantics, AtLoc, Range, Implicit),
        Value(Value) {}

   SemanticsAttr(StringRef Value, bool Implicit)
      : SemanticsAttr(Value, SourceLoc(), SourceRange(), /*Implicit=*/true) {}

   /// The semantics tag value.
   const StringRef Value;

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Semantics;
   }
};

/// Defines the @_alignment attribute.
class AlignmentAttr : public DeclAttribute {
public:
   AlignmentAttr(unsigned Value, SourceLoc AtLoc, SourceRange Range,
                 bool Implicit)
      : DeclAttribute(DAK_Alignment, AtLoc, Range, Implicit) {
      Bits.AlignmentAttr.Value = Value;
   }

   unsigned getValue() const { return Bits.AlignmentAttr.Value; }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Alignment;
   }
};

/// Defines the @_swift_native_objc_runtime_base attribute.
///
/// This attribute indicates a class that should be treated semantically
/// as a native Swift root class, but which inherits a specific Objective-C
/// class at runtime. For most classes this is the runtime's "SwiftObject"
/// root class. The compiler does not need to know about the class; it's the
/// build system's responsibility to link against the ObjC code that implements
/// the root class, and the ObjC implementation's responsibility to ensure
/// instances begin with a Swift-refcounting-compatible object header and
/// override all the necessary NSObject refcounting methods.
// @todo
//class SwiftNativeObjCRuntimeBaseAttr : public DeclAttribute {
//public:
//   SwiftNativeObjCRuntimeBaseAttr(Identifier BaseClassName,
//                                  SourceLoc AtLoc, SourceRange Range,
//                                  bool Implicit)
//      : DeclAttribute(DAK_SwiftNativeObjCRuntimeBase, AtLoc, Range, Implicit),
//        BaseClassName(BaseClassName) {}
//
//   // The base class's name.
//   const Identifier BaseClassName;
//
//   static bool classof(const DeclAttribute *DA) {
//      return DA->getKind() == DAK_SwiftNativeObjCRuntimeBase;
//   }
//};

/// Determine the result of comparing an availability attribute to a specific
/// platform or language version.
enum class AvailableVersionComparison {
   /// The entity is guaranteed to be available.
   Available,

   /// The entity is never available.
   Unavailable,

   /// The entity might be unavailable at runtime, because it was introduced
   /// after the requested minimum platform version.
   PotentiallyUnavailable,

   /// The entity has been obsoleted.
   Obsoleted,
};

/// Describes the platform-agnostic availability of a declaration.
enum class PlatformAgnosticAvailabilityKind {
   /// The associated availability attribute is not platform-agnostic.
   None,
   /// The declaration is deprecated, but can still be used.
   Deprecated,
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
class AvailableAttr : public DeclAttribute {
public:
#define INIT_VER_TUPLE(X)\
   X(X.empty() ? Optional<llvm::VersionTuple>() : X)

   AvailableAttr(SourceLoc AtLoc, SourceRange Range,
                 PlatformKind Platform,
                 StringRef Message, StringRef Rename,
                 const llvm::VersionTuple &Introduced,
                 SourceRange IntroducedRange,
                 const llvm::VersionTuple &Deprecated,
                 SourceRange DeprecatedRange,
                 const llvm::VersionTuple &Obsoleted,
                 SourceRange ObsoletedRange,
                 PlatformAgnosticAvailabilityKind PlatformAgnostic,
                 bool Implicit)
      : DeclAttribute(DAK_Available, AtLoc, Range, Implicit),
        Message(Message), Rename(Rename),
        INIT_VER_TUPLE(Introduced), IntroducedRange(IntroducedRange),
        INIT_VER_TUPLE(Deprecated), DeprecatedRange(DeprecatedRange),
        INIT_VER_TUPLE(Obsoleted), ObsoletedRange(ObsoletedRange),
        PlatformAgnostic(PlatformAgnostic),
        Platform(Platform)
   {}

#undef INIT_VER_TUPLE

   /// The optional message.
   const StringRef Message;

   /// An optional replacement string to emit in a fixit.  This allows simple
   /// declaration renames to be applied by Xcode.
   ///
   /// This should take the form of an operator, identifier, or full function
   /// name, optionally with a prefixed type, similar to the syntax used for
   /// the `NS_POLAR_NAME` annotation in Objective-C.
   const StringRef Rename;

   /// Indicates when the symbol was introduced.
   const Optional<llvm::VersionTuple> Introduced;

   /// Indicates where the Introduced version was specified.
   const SourceRange IntroducedRange;

   /// Indicates when the symbol was deprecated.
   const Optional<llvm::VersionTuple> Deprecated;

   /// Indicates where the Deprecated version was specified.
   const SourceRange DeprecatedRange;

   /// Indicates when the symbol was obsoleted.
   const Optional<llvm::VersionTuple> Obsoleted;

   /// Indicates where the Obsoleted version was specified.
   const SourceRange ObsoletedRange;

   /// Indicates if the declaration has platform-agnostic availability.
   const PlatformAgnosticAvailabilityKind PlatformAgnostic;

   /// The platform of the availability.
   const PlatformKind Platform;

   /// Whether this is a language-version-specific entity.
   bool isLanguageVersionSpecific() const;

   /// Whether this is a PackageDescription version specific entity.
   bool isPackageDescriptionVersionSpecific() const;

   /// Whether this is an unconditionally unavailable entity.
   bool isUnconditionallyUnavailable() const;

   /// Whether this is an unconditionally deprecated entity.
   bool isUnconditionallyDeprecated() const;

   /// Returns the platform-agnostic availability.
   PlatformAgnosticAvailabilityKind getPlatformAgnosticAvailability() const {
      return PlatformAgnostic;
   }

   /// Determine if a given declaration should be considered unavailable given
   /// the current settings.
   ///
   /// \returns The attribute responsible for making the declaration unavailable.
   static const AvailableAttr *isUnavailable(const Decl *D);

   /// Returns true if the availability applies to a specific
   /// platform.
   bool hasPlatform() const {
      return Platform != PlatformKind::none;
   }

   /// Returns the string for the platform of the attribute.
   StringRef platformString() const {
      return polar::platformString(Platform);
   }

   /// Returns the human-readable string for the platform of the attribute.
   StringRef prettyPlatformString() const {
      return polar::prettyPlatformString(Platform);
   }

   /// Returns true if this attribute is active given the current platform.
   bool isActivePlatform(const AstContext &ctx) const;

   /// Returns the active version from the AST context corresponding to
   /// the available kind. For example, this will return the effective language
   /// version for swift version-specific availability kind, PackageDescription
   /// version for PackageDescription version-specific availability.
   llvm::VersionTuple getActiveVersion(const AstContext &ctx) const;

   /// Compare this attribute's version information against the platform or
   /// language version (assuming the this attribute pertains to the active
   /// platform).
   AvailableVersionComparison getVersionAvailability(const AstContext &ctx) const;

   /// Create an AvailableAttr that indicates specific availability
   /// for all platforms.
   static AvailableAttr *
   createPlatformAgnostic(AstContext &C, StringRef Message, StringRef Rename = "",
                          PlatformAgnosticAvailabilityKind Reason
                          = PlatformAgnosticAvailabilityKind::Unavailable,
                          llvm::VersionTuple Obsoleted
                          = llvm::VersionTuple());

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Available;
   }
};

class PrivateImportAttr final
      : public DeclAttribute {
   StringRef SourceFile;

   PrivateImportAttr(SourceLoc atLoc, SourceRange baseRange,
                     StringRef sourceFile, SourceRange parentRange);

public:
   static PrivateImportAttr *create(AstContext &Ctxt, SourceLoc AtLoc,
                                    SourceLoc PrivateLoc, SourceLoc LParenLoc,
                                    StringRef sourceFile, SourceLoc RParenLoc);

   StringRef getSourceFile() const {
      return SourceFile;
   }
   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_PrivateImport;
   }
};

/// The @_dynamicReplacement(for:) attribute.
class DynamicReplacementAttr final
      : public DeclAttribute,
      private llvm::TrailingObjects<DynamicReplacementAttr, SourceLoc> {
   friend TrailingObjects;
   friend class DynamicallyReplacedDeclRequest;

   DeclName ReplacedFunctionName;
   LazyMemberLoader *Resolver = nullptr;
   uint64_t ResolverContextData;

   /// Create an @_dynamicReplacement(for:) attribute written in the source.
   DynamicReplacementAttr(SourceLoc atLoc, SourceRange baseRange,
                          DeclName replacedFunctionName, SourceRange parenRange);

   DynamicReplacementAttr(DeclName name, AbstractFunctionDecl *f)
      : DeclAttribute(DAK_DynamicReplacement, SourceLoc(), SourceRange(),
                      /*Implicit=*/false),
        ReplacedFunctionName(name),
        Resolver(nullptr), ResolverContextData(0) {
      Bits.DynamicReplacementAttr.HasTrailingLocationInfo = false;
   }

   DynamicReplacementAttr(DeclName name,
                          LazyMemberLoader *Resolver = nullptr,
                          uint64_t Data = 0)
      : DeclAttribute(DAK_DynamicReplacement, SourceLoc(), SourceRange(),
                      /*Implicit=*/false),
        ReplacedFunctionName(name),
        Resolver(Resolver), ResolverContextData(Data) {
      Bits.DynamicReplacementAttr.HasTrailingLocationInfo = false;
   }

   /// Retrieve the trailing location information.
   MutableArrayRef<SourceLoc> getTrailingLocations() {
      assert(Bits.DynamicReplacementAttr.HasTrailingLocationInfo);
      unsigned length = 2;
      return {getTrailingObjects<SourceLoc>(), length};
   }

   /// Retrieve the trailing location information.
   ArrayRef<SourceLoc> getTrailingLocations() const {
      assert(Bits.DynamicReplacementAttr.HasTrailingLocationInfo);
      unsigned length = 2; // lParens, rParens
      return {getTrailingObjects<SourceLoc>(), length};
   }

public:
   static DynamicReplacementAttr *
   create(AstContext &Context, SourceLoc AtLoc, SourceLoc DynReplLoc,
          SourceLoc LParenLoc, DeclName replacedFunction, SourceLoc RParenLoc);

   static DynamicReplacementAttr *create(AstContext &ctx,
                                         DeclName replacedFunction,
                                         AbstractFunctionDecl *replacedFuncDecl);

   static DynamicReplacementAttr *create(AstContext &ctx,
                                         DeclName replacedFunction,
                                         LazyMemberLoader *Resolver,
                                         uint64_t Data);

   DeclName getReplacedFunctionName() const {
      return ReplacedFunctionName;
   }

   /// Retrieve the location of the opening parentheses, if there is one.
   SourceLoc getLParenLoc() const;

   /// Retrieve the location of the closing parentheses, if there is one.
   SourceLoc getRParenLoc() const;

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_DynamicReplacement;
   }
};

/// Represents any sort of access control modifier.
class AbstractAccessControlAttr : public DeclAttribute {
protected:
   AbstractAccessControlAttr(DeclAttrKind DK, SourceLoc atLoc, SourceRange range,
                             AccessLevel access, bool implicit)
      : DeclAttribute(DK, atLoc, range, implicit) {
      Bits.AbstractAccessControlAttr.AccessLevel = static_cast<unsigned>(access);
      assert(getAccess() == access && "not enough bits for access control");
   }

public:
   AccessLevel getAccess() const {
      return static_cast<AccessLevel>(Bits.AbstractAccessControlAttr.AccessLevel);
   }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_AccessControl ||
            DA->getKind() == DAK_SetterAccess;
   }
};

/// Represents a 'private', 'internal', or 'public' marker on a declaration.
class AccessControlAttr : public AbstractAccessControlAttr {
public:
   AccessControlAttr(SourceLoc atLoc, SourceRange range, AccessLevel access,
                     bool implicit = false)
      : AbstractAccessControlAttr(DAK_AccessControl, atLoc, range, access,
                                  implicit) {}

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_AccessControl;
   }
};

/// Represents a 'private', 'internal', or 'public' marker for a setter on a
/// declaration.
class SetterAccessAttr : public AbstractAccessControlAttr {
public:
   SetterAccessAttr(SourceLoc atLoc, SourceRange range,
                    AccessLevel access, bool implicit = false)
      : AbstractAccessControlAttr(DAK_SetterAccess, atLoc, range, access,
                                  implicit) {}

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_SetterAccess;
   }
};

/// Represents an inline attribute.
class InlineAttr : public DeclAttribute {
public:
   InlineAttr(SourceLoc atLoc, SourceRange range, InlineKind kind)
      : DeclAttribute(DAK_Inline, atLoc, range, /*Implicit=*/false) {
      Bits.InlineAttr.kind = unsigned(kind);
   }

   InlineAttr(InlineKind kind)
      : InlineAttr(SourceLoc(), SourceRange(), kind) {}

   InlineKind getKind() const { return InlineKind(Bits.InlineAttr.kind); }
   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Inline;
   }
};

/// Represents the optimize attribute.
class OptimizeAttr : public DeclAttribute {
public:
   OptimizeAttr(SourceLoc atLoc, SourceRange range, OptimizationMode mode)
      : DeclAttribute(DAK_Optimize, atLoc, range, /*Implicit=*/false) {
      Bits.OptimizeAttr.mode = unsigned(mode);
   }

   OptimizeAttr(OptimizationMode mode)
      : OptimizeAttr(SourceLoc(), SourceRange(), mode) {}

   OptimizationMode getMode() const {
      return OptimizationMode(Bits.OptimizeAttr.mode);
   }
   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Optimize;
   }
};

/// Represents the side effects attribute.
class EffectsAttr : public DeclAttribute {
public:
   EffectsAttr(SourceLoc atLoc, SourceRange range, EffectsKind kind)
      : DeclAttribute(DAK_Effects, atLoc, range, /*Implicit=*/false) {
      Bits.EffectsAttr.kind = unsigned(kind);
   }

   EffectsAttr(EffectsKind kind)
      : EffectsAttr(SourceLoc(), SourceRange(), kind) {}

   EffectsKind getKind() const { return EffectsKind(Bits.EffectsAttr.kind); }
   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Effects;
   }
};



/// Represents weak/unowned/unowned(unsafe) decl modifiers.
class ReferenceOwnershipAttr : public DeclAttribute {
public:
   ReferenceOwnershipAttr(SourceRange range, ReferenceOwnership kind)
      : DeclAttribute(DAK_ReferenceOwnership, range.start, range,
                      /*Implicit=*/false) {
      Bits.ReferenceOwnershipAttr.ownership = unsigned(kind);
   }

   ReferenceOwnershipAttr(ReferenceOwnership kind)
      : ReferenceOwnershipAttr(SourceRange(), kind) {}

   ReferenceOwnership get() const {
      return ReferenceOwnership(Bits.ReferenceOwnershipAttr.ownership);
   }

   /// Returns a copy of this attribute without any source information.
   ReferenceOwnershipAttr *clone(AstContext &context) const {
      return new (context) ReferenceOwnershipAttr(get());
   }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_ReferenceOwnership;
   }
};

/// Defines the attribute that we use to model documentation comments.
class RawDocCommentAttr : public DeclAttribute {
   /// Source range of the attached comment.  This comment is located before
   /// the declaration.
   CharSourceRange CommentRange;

public:
   RawDocCommentAttr(CharSourceRange CommentRange)
      : DeclAttribute(DAK_RawDocComment, SourceLoc(), SourceRange(),
                      /*Implicit=*/false),
        CommentRange(CommentRange) {}

   CharSourceRange getCommentRange() const { return CommentRange; }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_RawDocComment;
   }
};

/// An attribute applied to a CoreFoundation class that is toll-free bridged to
/// an Objective-C class.
///
/// This attribute is introduced by the Clang importer, and is therefore always
/// implicit.
// @todo
//class ObjCBridgedAttr : public DeclAttribute {
//   ClassDecl *ObjCClass;
//
//public:
//   ObjCBridgedAttr(ClassDecl *ObjCClass)
//      : DeclAttribute(DAK_ObjCBridged, SourceLoc(), SourceRange(),
//                      /*Implicit=*/true),
//        ObjCClass(ObjCClass)
//   {
//   }
//
//   /// Retrieve the Objective-C class to which this foreign class is toll-free
//   /// bridged.
//   ClassDecl *getObjCClass() const { return ObjCClass; }
//
//   static bool classof(const DeclAttribute *DA) {
//      return DA->getKind() == DAK_ObjCBridged;
//   }
//};

/// An attribute that specifies a synthesized conformance of a known
/// protocol for the declaration to which it appertains.
///
/// There is no spelling for this particular attribute in source code;
/// rather, it is introduced by the Clang importer to indicate
/// synthesized conformances.
class SynthesizedInterfaceAttr : public DeclAttribute {
   LazyConformanceLoader *Loader;

public:
   SynthesizedInterfaceAttr(KnownInterfaceKind protocolKind,
                            LazyConformanceLoader *Loader)
      : DeclAttribute(DAK_SynthesizedInterface, SourceLoc(), SourceRange(),
                      /*Implicit=*/true), Loader(Loader)
   {
      Bits.SynthesizedInterfaceAttr.kind = unsigned(protocolKind);
   }

   /// Retrieve the known protocol kind naming the protocol to be
   /// synthesized.
   KnownInterfaceKind getInterfaceKind() const {
      return KnownInterfaceKind(Bits.SynthesizedInterfaceAttr.kind);
   }

   /// Retrieve the lazy loader that will be used to populate the
   /// synthesized conformance.
   LazyConformanceLoader *getLazyLoader() const { return Loader; }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_SynthesizedInterface;
   }
};

/// The @_specialize attribute, which forces specialization on the specified
/// type list.
class SpecializeAttr : public DeclAttribute {
public:
   // NOTE: When adding new kinds, you must update the inline bitfield macro.
   enum class SpecializationKind {
      Full,
      Partial
   };

private:
   TrailingWhereClause *trailingWhereClause;
   GenericSignature specializedSignature;

   SpecializeAttr(SourceLoc atLoc, SourceRange Range,
                  TrailingWhereClause *clause, bool exported,
                  SpecializationKind kind,
                  GenericSignature specializedSignature);

public:
   static SpecializeAttr *create(AstContext &Ctx, SourceLoc atLoc,
                                 SourceRange Range, TrailingWhereClause *clause,
                                 bool exported, SpecializationKind kind,
                                 GenericSignature specializedSignature
                                 = nullptr);

   TrailingWhereClause *getTrailingWhereClause() const;

   GenericSignature getSpecializedSgnature() const {
      return specializedSignature;
   }

   void setSpecializedSignature(GenericSignature newSig) {
      specializedSignature = newSig;
   }

   bool isExported() const {
      return Bits.SpecializeAttr.exported;
   }

   SpecializationKind getSpecializationKind() const {
      return SpecializationKind(Bits.SpecializeAttr.kind);
   }

   bool isFullSpecialization() const {
      return getSpecializationKind() == SpecializationKind::Full;
   }

   bool isPartialSpecialization() const {
      return getSpecializationKind() == SpecializationKind::Partial;
   }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Specialize;
   }
};

/// The @_implements attribute, which treats a decl as the implementation for
/// some named protocol requirement (but otherwise not-visible by that name).
class ImplementsAttr : public DeclAttribute {

   TypeLoc InterfaceType;
   DeclName MemberName;
   DeclNameLoc MemberNameLoc;

public:
   ImplementsAttr(SourceLoc atLoc, SourceRange Range,
                  TypeLoc InterfaceType,
                  DeclName MemberName,
                  DeclNameLoc MemberNameLoc);

   static ImplementsAttr *create(AstContext &Ctx, SourceLoc atLoc,
                                 SourceRange Range,
                                 TypeLoc InterfaceType,
                                 DeclName MemberName,
                                 DeclNameLoc MemberNameLoc);

   TypeLoc getInterfaceType() const;
   TypeLoc &getInterfaceType();
   DeclName getMemberName() const { return MemberName; }
   DeclNameLoc getMemberNameLoc() const { return MemberNameLoc; }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Implements;
   }
};

///// Attribute that specifies a protocol conformance that has been restated
///// (i.e., is redundant) but should still be emitted in Objective-C metadata.
//class RestatedObjCConformanceAttr : public DeclAttribute {
//public:
//  explicit RestatedObjCConformanceAttr(InterfaceDecl *proto)
//    : DeclAttribute(DAK_RestatedObjCConformance, SourceLoc(), SourceRange(),
//                    /*Implicit=*/true),
//      Proto(proto) {}

//  /// The protocol to which this type conforms.
//  InterfaceDecl * const Proto;

//  static bool classof(const DeclAttribute *DA) {
//    return DA->getKind() == DAK_RestatedObjCConformance;
//  }
//};

/// Attached to type declarations synthesized by the Clang importer.
///
/// Used to control manglings.
class ClangImporterSynthesizedTypeAttr : public DeclAttribute {
public:
   // NOTE: When adding new kinds, you must update the inline bitfield macro.
   enum class Kind : char {
      /// A struct synthesized by the importer to represent an NSError with a
      /// particular domain, as specified by an enum with the \c ns_error_domain
      /// Clang attribute.
      ///
      /// This one is for enums with names.
      NSErrorWrapper,

      /// A struct synthesized by the importer to represent an NSError with a
      /// particular domain, as specified by an enum with the \c ns_error_domain
      /// Clang attribute.
      ///
      /// This one is for anonymous enums that are immediately typedef'd, giving
      /// them a unique name for linkage purposes according to the C++ standard.
      NSErrorWrapperAnon,
   };

   /// The (Clang) name of the declaration that caused this type declaration to
   /// be synthesized.
   ///
   /// Must be a valid Swift identifier as well, for mangling purposes.
   const StringRef originalTypeName;

   explicit ClangImporterSynthesizedTypeAttr(StringRef originalTypeName,
                                             Kind kind)
      : DeclAttribute(DAK_ClangImporterSynthesizedType, SourceLoc(),
                      SourceRange(), /*Implicit=*/true),
        originalTypeName(originalTypeName) {
      assert(!originalTypeName.empty());
      Bits.ClangImporterSynthesizedTypeAttr.kind = unsigned(kind);
   }

   Kind getKind() const {
      return Kind(Bits.ClangImporterSynthesizedTypeAttr.kind);
   }

   StringRef getManglingName() const {
      return manglingNameForKind(getKind());
   }

   static StringRef manglingNameForKind(Kind kind) {
      switch (kind) {
      case Kind::NSErrorWrapper:
         return "e";
      case Kind::NSErrorWrapperAnon:
         return "E";
      }
      llvm_unreachable("unhandled kind");
   }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_ClangImporterSynthesizedType;
   }
};

/// Defines a custom attribute.
class CustomAttr final : public DeclAttribute,
      public TrailingCallArguments<CustomAttr> {
   TypeLoc type;
   Expr *arg;
   PatternBindingInitializer *initContext;
   Expr *semanticInit = nullptr;

   unsigned hasArgLabelLocs : 1;
   unsigned numArgLabels : 16;

   CustomAttr(SourceLoc atLoc, SourceRange range, TypeLoc type,
              PatternBindingInitializer *initContext, Expr *arg,
              ArrayRef<Identifier> argLabels, ArrayRef<SourceLoc> argLabelLocs,
              bool implicit);

public:
   static CustomAttr *create(AstContext &ctx, SourceLoc atLoc, TypeLoc type,
                             bool implicit = false) {
      return create(ctx, atLoc, type, false, nullptr, SourceLoc(), { }, { }, { },
                    SourceLoc(), implicit);
   }

   static CustomAttr *create(AstContext &ctx, SourceLoc atLoc, TypeLoc type,
                             bool hasInitializer,
                             PatternBindingInitializer *initContext,
                             SourceLoc lParenLoc,
                             ArrayRef<Expr *> args,
                             ArrayRef<Identifier> argLabels,
                             ArrayRef<SourceLoc> argLabelLocs,
                             SourceLoc rParenLoc,
                             bool implicit = false);

   unsigned getNumArguments() const { return numArgLabels; }
   bool hasArgumentLabelLocs() const { return hasArgLabelLocs; }

   TypeLoc &getTypeLoc() { return type; }
   const TypeLoc &getTypeLoc() const { return type; }

   Expr *getArg() const { return arg; }
   void setArg(Expr *newArg) { arg = newArg; }

   Expr *getSemanticInit() const { return semanticInit; }
   void setSemanticInit(Expr *expr) { semanticInit = expr; }

   PatternBindingInitializer *getInitContext() const { return initContext; }

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Custom;
   }
};

/// Relates a property to its projection value property, as described by a property wrapper. For
/// example, given
/// \code
/// @A var foo: Int
/// \endcode
///
/// Where \c A is a property wrapper that has a \c projectedValue property, the compiler
/// synthesizes a declaration $foo an attaches the attribute
/// \c _projectedValuePropertyAttr($foo) to \c foo to record the link.
class ProjectedValuePropertyAttr : public DeclAttribute {
public:
   ProjectedValuePropertyAttr(Identifier PropertyName,
                              SourceLoc AtLoc, SourceRange Range,
                              bool Implicit)
      : DeclAttribute(DAK_ProjectedValueProperty, AtLoc, Range, Implicit),
        ProjectionPropertyName(PropertyName) {}

   // The projection property name.
   const Identifier ProjectionPropertyName;

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_ProjectedValueProperty;
   }
};

/// Describe a symbol was originally defined in another module. For example, given
/// \code
/// @_originallyDefinedIn(module: "Original", OSX 10.15) var foo: Int
/// \endcode
///
/// Where variable Foo has originally defined in another module called Original prior to OSX 10.15
class OriginallyDefinedInAttr: public DeclAttribute {
public:
   OriginallyDefinedInAttr(SourceLoc AtLoc, SourceRange Range,
                           StringRef OriginalModuleName,
                           PlatformKind Platform,
                           const llvm::VersionTuple MovedVersion,
                           bool Implicit)
      : DeclAttribute(DAK_OriginallyDefinedIn, AtLoc, Range, Implicit),
        OriginalModuleName(OriginalModuleName),
        Platform(Platform),
        MovedVersion(MovedVersion) {}

   // The original module name.
   const StringRef OriginalModuleName;

   /// The platform of the symbol.
   const PlatformKind Platform;

   /// Indicates when the symbol was moved here.
   const llvm::VersionTuple MovedVersion;

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_OriginallyDefinedIn;
   }
};

/// Attributes that may be applied to declarations.
class DeclAttributes {
   /// Linked list of declaration attributes.
   DeclAttribute *DeclAttrs;

public:
   DeclAttributes() : DeclAttrs(nullptr) {}

   bool isEmpty() const {
      return DeclAttrs == nullptr;
   }

   void getAttrRanges(SmallVectorImpl<SourceRange> &Ranges) const {
      for (auto Attr : *this) {
         auto R = Attr->getRangeWithAt();
         if (R.isValid())
            Ranges.push_back(R);
      }
   }

   /// If this attribute set has a prefix/postfix attribute on it, return this.
   UnaryOperatorKind getUnaryOperatorKind() const {
      if (hasAttribute<PrefixAttr>())
         return UnaryOperatorKind::Prefix;
      if (hasAttribute<PostfixAttr>())
         return UnaryOperatorKind::Postfix;
      return UnaryOperatorKind::None;
   }

   bool isUnavailable(const AstContext &ctx) const {
      return getUnavailable(ctx) != nullptr;
   }

   /// Determine whether there is a swiftVersionSpecific attribute that's
   /// unavailable relative to the provided language version.
   bool
   isUnavailableInSwiftVersion(const version::Version &effectiveVersion) const;

   /// Returns the first @available attribute that indicates
   /// a declaration is unavailable, or the first one that indicates it's
   /// potentially unavailable, or null otherwise.
   const AvailableAttr *getPotentiallyUnavailable(const AstContext &ctx) const;

   /// Returns the first @available attribute that indicates
   /// a declaration is unavailable, or null otherwise.
   const AvailableAttr *getUnavailable(const AstContext &ctx) const;

   /// Returns the first @available attribute that indicates
   /// a declaration is deprecated on all deployment targets, or null otherwise.
   const AvailableAttr *getDeprecated(const AstContext &ctx) const;

   POLAR_DEBUG_DUMPER(dump(const Decl *D = nullptr));
   void print(AstPrinter &Printer, const PrintOptions &Options,
              const Decl *D = nullptr) const;
   static void print(AstPrinter &Printer, const PrintOptions &Options,
                     ArrayRef<const DeclAttribute *> FlattenedAttrs,
                     const Decl *D = nullptr);

   template <typename T, typename DERIVED>
   class iterator_base : public std::iterator<std::forward_iterator_tag, T *> {
      T *Impl;
   public:
      explicit iterator_base(T *Impl) : Impl(Impl) {}
      DERIVED &operator++() { Impl = Impl->Next; return (DERIVED&)*this; }
      bool operator==(const iterator_base &X) const { return X.Impl == Impl; }
      bool operator!=(const iterator_base &X) const { return X.Impl != Impl; }
      T *operator*() const { return Impl; }
      T &operator->() const { return *Impl; }
   };

   /// Add a constructed DeclAttribute to this list.
   void add(DeclAttribute *Attr) {
      Attr->Next = DeclAttrs;
      DeclAttrs = Attr;
   }

   // Iterator interface over DeclAttribute objects.
   class iterator : public iterator_base<DeclAttribute, iterator> {
   public:
      explicit iterator(DeclAttribute *Impl) : iterator_base(Impl) {}
   };

   class const_iterator : public iterator_base<const DeclAttribute,
         const_iterator> {
   public:
      explicit const_iterator(const DeclAttribute *Impl)
         : iterator_base(Impl) {}
   };

   iterator begin() { return iterator(DeclAttrs); }
   iterator end() { return iterator(nullptr); }
   const_iterator begin() const { return const_iterator(DeclAttrs); }
   const_iterator end() const { return const_iterator(nullptr); }

   /// Retrieve the first attribute of the given attribute class.
   template <typename ATTR>
   const ATTR *getAttribute(bool AllowInvalid = false) const {
      return const_cast<DeclAttributes *>(this)->getAttribute<ATTR>(AllowInvalid);
   }

   template <typename ATTR>
   ATTR *getAttribute(bool AllowInvalid = false) {
      for (auto Attr : *this)
         if (auto *SpecificAttr = dyn_cast<ATTR>(Attr))
            if (SpecificAttr->isValid() || AllowInvalid)
               return SpecificAttr;
      return nullptr;
   }

   /// Determine whether there is an attribute with the given attribute class.
   template <typename ATTR>
   bool hasAttribute(bool AllowInvalid = false) const {
      return getAttribute<ATTR>(AllowInvalid) != nullptr;
   }

   /// Retrieve the first attribute with the given kind.
   const DeclAttribute *getAttribute(DeclAttrKind DK,
                                     bool AllowInvalid = false) const {
      for (auto Attr : *this)
         if (Attr->getKind() == DK && (Attr->isValid() || AllowInvalid))
            return Attr;
      return nullptr;
   }

private:
   /// Predicate used to filter MatchingAttributeRange.
   template <typename ATTR, bool AllowInvalid> struct ToAttributeKind {
      ToAttributeKind() {}

      Optional<const ATTR *>
      operator()(const DeclAttribute *Attr) const {
         if (isa<ATTR>(Attr) && (Attr->isValid() || AllowInvalid))
            return cast<ATTR>(Attr);
         return None;
      }
   };

public:
   template <typename ATTR, bool AllowInvalid>
   using AttributeKindRange =
   OptionalTransformRange<iterator_range<const_iterator>,
   ToAttributeKind<ATTR, AllowInvalid>,
   const_iterator>;

   /// Return a range with all attributes in DeclAttributes with AttrKind
   /// ATTR.
   template <typename ATTR, bool AllowInvalid = false>
   AttributeKindRange<ATTR, AllowInvalid> getAttributes() const {
      return AttributeKindRange<ATTR, AllowInvalid>(
               make_range(begin(), end()), ToAttributeKind<ATTR, AllowInvalid>());
   }

   // Remove the given attribute from the list of attributes. Used when
   // the attribute was semantically invalid.
   void removeAttribute(const DeclAttribute *attr) {
      // If it's the first attribute, remove it.
      if (DeclAttrs == attr) {
         DeclAttrs = attr->Next;
         return;
      }

      // Otherwise, find it in the list. This is inefficient, but rare.
      for (auto **prev = &DeclAttrs; *prev; prev = &(*prev)->Next) {
         if ((*prev)->Next == attr) {
            (*prev)->Next = attr->Next;
            return;
         }
      }
      llvm_unreachable("Attribute not found for removal");
   }

   /// Set the raw chain of attributes.  Used for deserialization.
   void setRawAttributeChain(DeclAttribute *Chain) {
      DeclAttrs = Chain;
   }

   SourceLoc getStartLoc(bool forModifiers = false) const;
};

/// A declaration name with location.
struct DeclNameWithLoc {
   DeclName Name;
   DeclNameLoc Loc;
};

/// Attribute that marks a function as differentiable and optionally specifies
/// custom associated derivative functions: 'jvp' and 'vjp'.
///
/// Examples:
///   @differentiable(jvp: jvpFoo where T : FloatingPoint)
///   @differentiable(wrt: (self, x, y), jvp: jvpFoo)
class DifferentiableAttr final
      : public DeclAttribute,
      private llvm::TrailingObjects<DifferentiableAttr,
      ParsedAutoDiffParameter> {
   friend TrailingObjects;

   /// Whether this function is linear (optional).
   bool linear;
   /// The number of parsed parameters specified in 'wrt:'.
   unsigned NumParsedParameters = 0;
   /// The JVP function.
   Optional<DeclNameWithLoc> JVP;
   /// The VJP function.
   Optional<DeclNameWithLoc> VJP;
   /// The JVP function (optional), resolved by the type checker if JVP name is
   /// specified.
   FuncDecl *JVPFunction = nullptr;
   /// The VJP function (optional), resolved by the type checker if VJP name is
   /// specified.
   FuncDecl *VJPFunction = nullptr;
   /// The differentiation parameters' indices, resolved by the type checker.
   IndexSubset *ParameterIndices = nullptr;
   /// The trailing where clause (optional).
   TrailingWhereClause *WhereClause = nullptr;
   /// The generic signature for autodiff associated functions. Resolved by the
   /// type checker based on the original function's generic signature and the
   /// attribute's where clause requirements. This is set only if the attribute
   /// has a where clause.
   GenericSignature DerivativeGenericSignature;

   explicit DifferentiableAttr(AstContext &context, bool implicit,
                               SourceLoc atLoc, SourceRange baseRange,
                               bool linear,
                               ArrayRef<ParsedAutoDiffParameter> parameters,
                               Optional<DeclNameWithLoc> jvp,
                               Optional<DeclNameWithLoc> vjp,
                               TrailingWhereClause *clause);

   explicit DifferentiableAttr(AstContext &context, bool implicit,
                               SourceLoc atLoc, SourceRange baseRange,
                               bool linear, IndexSubset *indices,
                               Optional<DeclNameWithLoc> jvp,
                               Optional<DeclNameWithLoc> vjp,
                               GenericSignature derivativeGenericSignature);

public:
   static DifferentiableAttr *create(AstContext &context, bool implicit,
                                     SourceLoc atLoc, SourceRange baseRange,
                                     bool linear,
                                     ArrayRef<ParsedAutoDiffParameter> params,
                                     Optional<DeclNameWithLoc> jvp,
                                     Optional<DeclNameWithLoc> vjp,
                                     TrailingWhereClause *clause);

   static DifferentiableAttr *create(AstContext &context, bool implicit,
                                     SourceLoc atLoc, SourceRange baseRange,
                                     bool linear, IndexSubset *indices,
                                     Optional<DeclNameWithLoc> jvp,
                                     Optional<DeclNameWithLoc> vjp,
                                     GenericSignature derivativeGenSig);

   /// Get the optional 'jvp:' function name and location.
   /// Use this instead of `getJVPFunction` to check whether the attribute has a
   /// registered JVP.
   Optional<DeclNameWithLoc> getJVP() const { return JVP; }

   /// Get the optional 'vjp:' function name and location.
   /// Use this instead of `getVJPFunction` to check whether the attribute has a
   /// registered VJP.
   Optional<DeclNameWithLoc> getVJP() const { return VJP; }

   IndexSubset *getParameterIndices() const {
      return ParameterIndices;
   }
   void setParameterIndices(IndexSubset *pi) {
      ParameterIndices = pi;
   }

   /// The parsed differentiation parameters, i.e. the list of parameters
   /// specified in 'wrt:'.
   ArrayRef<ParsedAutoDiffParameter> getParsedParameters() const {
      return {getTrailingObjects<ParsedAutoDiffParameter>(), NumParsedParameters};
   }
   MutableArrayRef<ParsedAutoDiffParameter> getParsedParameters() {
      return {getTrailingObjects<ParsedAutoDiffParameter>(), NumParsedParameters};
   }
   size_t numTrailingObjects(OverloadToken<ParsedAutoDiffParameter>) const {
      return NumParsedParameters;
   }

   bool isLinear() const { return linear; }

   TrailingWhereClause *getWhereClause() const { return WhereClause; }

   GenericSignature getDerivativeGenericSignature() const {
      return DerivativeGenericSignature;
   }
   void setDerivativeGenericSignature(AstContext &context,
                                      GenericSignature derivativeGenSig) {
      DerivativeGenericSignature = derivativeGenSig;
   }

   FuncDecl *getJVPFunction() const { return JVPFunction; }
   void setJVPFunction(FuncDecl *decl);
   FuncDecl *getVJPFunction() const { return VJPFunction; }
   void setVJPFunction(FuncDecl *decl);

   bool parametersMatch(const DifferentiableAttr &other) const {
      assert(ParameterIndices && other.ParameterIndices);
      return ParameterIndices == other.ParameterIndices;
   }

   /// Get the derivative generic environment for the given `@differentiable`
   /// attribute and original function.
   GenericEnvironment *
   getDerivativeGenericEnvironment(AbstractFunctionDecl *original) const;

   // Print the attribute to the given stream.
   // If `omitWrtClause` is true, omit printing the `wrt:` clause.
   // If `omitAssociatedFunctions` is true, omit printing associated functions.
   void print(llvm::raw_ostream &OS, const Decl *D,
              bool omitWrtClause = false,
              bool omitAssociatedFunctions = false) const;

   static bool classof(const DeclAttribute *DA) {
      return DA->getKind() == DAK_Differentiable;
   }
};


void simple_display(llvm::raw_ostream &out, const DeclAttribute *attr);

inline SourceLoc extractNearestSourceLoc(const DeclAttribute *attr) {
   return attr->getLocation();
}

} // end namespace swift

#endif
