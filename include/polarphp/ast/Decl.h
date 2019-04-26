//===--- Decl.h - Swift Language Declaration ASTs ---------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
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
// This file defines the Decl class and subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DECL_H
#define POLARPHP_AST_DECL_H

#include "polarphp/basic/InlineBitfield.h"
#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/ast/DeclContext.h"
#include "polarphp/ast/Attr.h"
#include "polarphp/ast/DefaultArgumentKind.h"
#include <type_traits>
#include <cstdint>

/// forward declared class with namespace
namespace polar {
namespace utils {
class RawOutStream;
} // utils
} // polar

namespace polar::ast {

using polar::utils::RawOutStream;
using polar::basic::PointerUnion;
using polar::basic::StringRef;

enum class AccessSemantics : unsigned char;
class AccessorDecl;
class ApplyExpr;
class GenericEnvironment;
class ArchetypeType;
class AstContext;
struct AstNode;
class AstPrinter;
class AstWalker;
class ConstructorDecl;
class DestructorDecl;
class DiagnosticEngine;
class DynamicSelfType;
class Type;
class Expr;
class DeclRefExpr;
class ForeignErrorConvention;
class LiteralExpr;
class BraceStmt;
class DeclAttributes;
class GenericContext;
class GenericSignature;
class GenericTypeParamDecl;
class GenericTypeParamType;
class LazyResolver;
class ModuleDecl;
class EnumCaseDecl;
class EnumElementDecl;
class ParameterList;
class ParameterTypeFlags;
class Pattern;
struct PrintOptions;
class ProtocolDecl;
class ProtocolType;
struct RawComment;
enum class ResilienceExpansion : unsigned;
class TypeAliasDecl;
class Stmt;
class SubscriptDecl;
class UnboundGenericType;
class ValueDecl;
class VarDecl;

enum class DeclKind : uint8_t
{
#define DECL(Id, Parent) Id,
#define LAST_DECL(Id) Last_Decl = Id,
#define DECL_RANGE(Id, FirstId, LastId) \
   First_##Id##Decl = FirstId, Last_##Id##Decl = LastId,
#include "polarphp/ast/DeclNodesDefs.h"
};

enum : unsigned
{
   NumDeclKindBits =
   polar::basic::count_bits_used(static_cast<unsigned>(DeclKind::Last_Decl))
};

/// Fine-grained declaration kind that provides a description of the
/// kind of entity a declaration represents, as it would be used in
/// diagnostics.
///
/// For example, \c FuncDecl is a single declaration class, but it has
/// several descriptive entries depending on whether it is an
/// operator, global function, local function, method, (observing)
/// accessor, etc.
enum class DescriptiveDeclKind : uint8_t
{
   Import,
   Extension,
   EnumCase,
   TopLevelCode,
   IfConfig,
   PoundDiagnostic,
   PatternBinding,
   Var,
   Param,
   Let,
   Property,
   StaticProperty,
   ClassProperty,
   InfixOperator,
   PrefixOperator,
   PostfixOperator,
   PrecedenceGroup,
   TypeAlias,
   GenericTypeParam,
   AssociatedType,
   Type,
   Enum,
   Struct,
   Class,
   Protocol,
   GenericEnum,
   GenericStruct,
   GenericClass,
   GenericType,
   Subscript,
   Constructor,
   Destructor,
   LocalFunction,
   GlobalFunction,
   OperatorFunction,
   Method,
   StaticMethod,
   ClassMethod,
   Getter,
   Setter,
   Addressor,
   MutableAddressor,
   ReadAccessor,
   ModifyAccessor,
   WillSet,
   DidSet,
   EnumElement,
   Module,
   MissingMember,
   Requirement,
};

/// Keeps track of stage of circularity checking for the given protocol.
enum class CircularityCheck
{
   /// Circularity has not yet been checked.
   Unchecked,
   /// We're currently checking circularity.
   Checking,
   /// Circularity has already been checked.
   Checked
};

/// Describes which spelling was used in the source for the 'static' or 'class'
/// keyword.
enum class StaticSpellingKind : uint8_t
{
   None,
   KeywordStatic,
   KeywordClass,
};


/// Keeps track of whether an enum has cases that have associated values.
enum class AssociatedValueCheck
{
   /// We have not yet checked.
   Unchecked,
   /// The enum contains no cases or all cases contain no associated values.
   NoAssociatedValues,
   /// The enum contains at least one case with associated values.
   HasAssociatedValues,
};

/// Diagnostic printing of \c StaticSpellingKind.
RawOutStream &operator<<(RawOutStream &outStream, StaticSpellingKind ssKind);

/// Decl - Base class for all declarations in Swift.
class alignas(1 << DeclAlignInBits) Decl
{
   public:
   enum class ValidationState {
      Unchecked,
      Checking,
      CheckingWithValidSignature,
      Checked,
   };

   protected:
   union {
      uint64_t opaqueBits;
      POLAR_INLINE_BITFIELD_BASE(
               Decl, polar::basic::bitmax(NumDeclKindBits,8)+1+1+1+1+1+1+1,
               Kind : polar::basic::bitmax(NumDeclKindBits,8),
               /// Whether this declaration is invalid.
               Invalid : 1,

               /// Whether this declaration was implicitly created, e.g.,
               /// an implicit constructor in a struct.
               Implicit : 1,

               /// Whether this declaration was mapped directly from a Clang AST.
               ///
               /// Use getClangNode() to retrieve the corresponding Clang AST.
               FromClang : 1,

               /// Whether we've already performed early attribute validation.
               /// FIXME: This is ugly.
               EarlyAttrValidation : 1,

               /// The validation state of this declaration.
               ValidationState : 2,

               /// Whether this declaration was added to the surrounding
               /// DeclContext of an active #if config clause.
               EscapedFromIfConfig : 1);

      POLAR_INLINE_BITFIELD_FULL(
               PatternBindingDecl, Decl, 1+2+16,
               /// Whether this pattern binding declares static variables.
               IsStatic : 1,

               /// Whether 'static' or 'class' was used.
               StaticSpelling : 2,

               : NumPadBits,

               /// The number of pattern binding declarations.
               NumPatternEntries : 16
               );

      POLAR_INLINE_BITFIELD_FULL(
               EnumCaseDecl, Decl, 32,
               : NumPadBits,

               /// The number of tail-allocated element pointers.
               NumElements : 32
               );

      POLAR_INLINE_BITFIELD(
               ValueDecl, Decl, 1+1+1,
               AlreadyInLookupTable : 1,

               /// Whether we have already checked whether this declaration is a
               /// redeclaration.
               CheckedRedeclaration : 1,

               /// Whether the decl can be accessed by swift users; for instance,
               /// a.storage for lazy var a is a decl that cannot be accessed.
               IsUserAccessible : 1
               );

      POLAR_INLINE_BITFIELD(
               AbstractStorageDecl, ValueDecl, 1+1+1+1+2+1+1,
               /// Whether the getter is mutating.
               IsGetterMutating : 1,

               /// Whether the setter is mutating.
               IsSetterMutating : 1,

               /// Whether this represents physical storage.
               HasStorage : 1,

               /// Whether this storage supports semantic mutation in some way.
               SupportsMutation : 1,

               /// Whether an opaque read of this storage produces an owned value.
               OpaqueReadOwnership : 2,

               /// Whether a keypath component can directly reference this storage,
               /// or if it must use the overridden declaration instead.
               HasComputedValidKeyPathComponent : 1,
               ValidKeyPathComponent : 1
               );

      POLAR_INLINE_BITFIELD(
               VarDecl, AbstractStorageDecl, 1+4+1+1+1+1,
               /// Whether this property is a type property (currently unfortunately
               /// called 'static').
               IsStatic : 1,

               /// The specifier associated with this variable or parameter.  This
               /// determines the storage semantics of the value e.g. mutability.
               Specifier : 4,

               /// Whether this declaration was an element of a capture list.
               IsCaptureList : 1,

               /// Whether this vardecl has an initial value bound to it in a way
               /// that isn't represented in the AST with an initializer in the pattern
               /// binding.  This happens in cases like "for i in ...", switch cases, etc.
               HasNonPatternBindingInit : 1,

               /// Whether this is a property used in expressions in the debugger.
               /// It is up to the debugger to instruct SIL how to access this variable.
               IsDebuggerVar : 1,

               /// Whether this is a property defined in the debugger's REPL.
               /// FIXME: Remove this once LLDB has proper support for resilience.
               IsREPLVar : 1
               );

      POLAR_INLINE_BITFIELD(
               ParamDecl, VarDecl, 1 + NumDefaultArgumentKindBits,
               /// True if the type is implicitly specified in the source, but this has an
               /// apparently valid typeRepr.  This is used in accessors, which look like:
               ///    set (value) {
               /// but need to get the typeRepr from the property as a whole so Sema can
               /// resolve the type.
               IsTypeLocImplicit : 1,

               /// Information about a symbolic default argument, like #file.
               defaultArgumentKind : NumDefaultArgumentKindBits
               );

      POLAR_INLINE_BITFIELD(
               EnumElementDecl, ValueDecl, 1,
               /// The ResilienceExpansion to use for default arguments.
               DefaultArgumentResilienceExpansion : 1
               );

      POLAR_INLINE_BITFIELD(
               AbstractFunctionDecl, ValueDecl, 3+8+1+1+1+1+1+1+1,
               /// \see AbstractFunctionDecl::BodyKind
               BodyKind : 3,

               /// Import as member status.
               IAMStatus : 8,

               /// Whether the function has an implicit 'self' parameter.
               HasImplicitSelfDecl : 1,

               /// Whether we are overridden later.
               Overridden : 1,

               /// Whether the function body throws.
               Throws : 1,

               /// Whether this function requires a new vtable entry.
               NeedsNewVTableEntry : 1,

               /// Whether NeedsNewVTableEntry is valid.
               HasComputedNeedsNewVTableEntry : 1,

               /// The ResilienceExpansion to use for default arguments.
               DefaultArgumentResilienceExpansion : 1,

               /// Whether this member was synthesized as part of a derived
               /// protocol conformance.
               Synthesized : 1
               );

      POLAR_INLINE_BITFIELD(
               FuncDecl, AbstractFunctionDecl, 1+2+1+1+2,
               /// Whether this function is a 'static' method.
               IsStatic : 1,

               /// Whether 'static' or 'class' was used.
               StaticSpelling : 2,

               /// Whether we are statically dispatched even if overridable
               ForcedStaticDispatch : 1,

               /// Whether this function has a dynamic Self return type.
               HasDynamicSelf : 1,

               /// Backing bits for 'self' access kind.
               SelfAccess : 2
               );

      POLAR_INLINE_BITFIELD(
               AccessorDecl, FuncDecl, 4,
               /// The kind of accessor this is.
               AccessorKind : 4
               );

      POLAR_INLINE_BITFIELD(
               ConstructorDecl, AbstractFunctionDecl, 3+2+2+1,
               /// The body initialization kind (+1), or zero if not yet computed.
               ///
               /// This value is cached but is not serialized, because it is a property
               /// of the definition of the constructor that is useful only to semantic
               /// analysis and SIL generation.
               ComputedBodyInitKind : 3,

               /// The kind of initializer we have.
               InitKind : 2,

               /// The failability of this initializer, which is an OptionalTypeKind.
               Failability : 2,

               /// Whether this initializer is a stub placed into a subclass to
               /// catch invalid delegations to a designated initializer not
               /// overridden by the subclass. A stub will always trap at runtime.
               ///
               /// Initializer stubs can be invoked from Objective-C or through
               /// the Objective-C runtime; there is no way to directly express
               /// an object construction that will invoke a stub.
               HasStubImplementation : 1
               );

      POLAR_INLINE_BITFIELD_EMPTY(
               TypeDecl, ValueDecl);
      POLAR_INLINE_BITFIELD_EMPTY(
               AbstractTypeParamDecl, TypeDecl);

      POLAR_INLINE_BITFIELD_FULL(
               GenericTypeParamDecl, AbstractTypeParamDecl, 16+16,
               : NumPadBits,

               Depth : 16,
               Index : 16
               );

      POLAR_INLINE_BITFIELD_EMPTY(
               GenericTypeDecl, TypeDecl
               );

      POLAR_INLINE_BITFIELD(
               TypeAliasDecl, GenericTypeDecl, 1+1,
               /// Whether the typealias forwards perfectly to its underlying type.
               IsCompatibilityAlias : 1,
               /// Whether this was a global typealias synthesized by the debugger.
               IsDebuggerAlias : 1
               );

      POLAR_INLINE_BITFIELD(
               NominalTypeDecl, GenericTypeDecl, 1+1,
               /// Whether we have already added implicitly-defined initializers
               /// to this declaration.
               AddedImplicitInitializers : 1,

               /// Whether there is are lazily-loaded conformances for this nominal type.
               HasLazyConformances : 1
               );

      POLAR_INLINE_BITFIELD_FULL(
               ProtocolDecl, NominalTypeDecl, 1+1+1+1+1+1+1+2+1+8+16,
               /// Whether the \c RequiresClass bit is valid.
               RequiresClassValid : 1,

               /// Whether this is a class-bounded protocol.
               RequiresClass : 1,

               /// Whether the \c ExistentialConformsToSelf bit is valid.
               ExistentialConformsToSelfValid : 1,

               /// Whether the existential of this protocol conforms to itself.
               ExistentialConformsToSelf : 1,

               /// Whether the \c ExistentialTypeSupported bit is valid.
               ExistentialTypeSupportedValid : 1,

               /// Whether the existential of this protocol can be represented.
               ExistentialTypeSupported : 1,

               /// True if the protocol has requirements that cannot be satisfied (e.g.
               /// because they could not be imported from Objective-C).
               HasMissingRequirements : 1,

               /// The stage of the circularity check for this protocol.
               Circularity : 2,

               /// Whether we've computed the inherited protocols list yet.
               InheritedProtocolsValid : 1,

               : NumPadBits,

               /// If this is a compiler-known protocol, this will be a KnownProtocolKind
               /// value, plus one. Otherwise, it will be 0.
               KnownProtocol : 8, // '8' for speed. This only needs 6.

               /// The number of requirements in the requirement signature.
               NumRequirementsInSignature : 16
               );

      POLAR_INLINE_BITFIELD(
               ClassDecl, NominalTypeDecl, 1+2+1+2+1+3+1+1+1+1,
               /// Whether this class requires all of its instance variables to
               /// have in-class initializers.
               RequiresStoredPropertyInits : 1,

               /// The stage of the inheritance circularity check for this class.
               Circularity : 2,

               /// Whether this class inherits its superclass's convenience initializers.
               InheritsSuperclassInits : 1,

               /// \see ClassDecl::ForeignKind
               RawForeignKind : 2,

               /// Whether this class contains a destructor decl.
               ///
               /// A fully type-checked class always contains a destructor member, even if
               /// it is implicit. This bit is used during parsing and type-checking to
               /// control inserting the implicit destructor.
               HasDestructorDecl : 1,

               /// Whether the class has @objc ancestry.
               ObjCKind : 3,

               /// Whether this class has @objc members.
               HasObjCMembersComputed : 1,
               HasObjCMembers : 1,

               HasMissingDesignatedInitializers : 1,
               HasMissingVTableEntries : 1
               );

      POLAR_INLINE_BITFIELD(
               StructDecl, NominalTypeDecl, 1,
               /// True if this struct has storage for fields that aren't accessible in
               /// Swift.
               HasUnreferenceableStorage : 1
               );

      POLAR_INLINE_BITFIELD(
               EnumDecl, NominalTypeDecl, 2+2+1,
               /// The stage of the raw type circularity check for this class.
               Circularity : 2,

               /// True if the enum has cases and at least one case has associated values.
               HasAssociatedValues : 2,
               /// True if the enum has at least one case that has some availability
               /// attribute.  A single bit because it's lazily computed along with the
               /// HasAssociatedValues bit.
               HasAnyUnavailableValues : 1
               );

      POLAR_INLINE_BITFIELD(
               ModuleDecl, TypeDecl, 1+1+1+1+1+1,
               /// If the module was or is being compiled with `-enable-testing`.
               TestingEnabled : 1,

               /// If the module failed to load
               FailedToLoad : 1,

               /// Whether the module is resilient.
               ///
               /// \sa ResilienceStrategy
               RawResilienceStrategy : 1,

               /// Whether all imports have been resolved. Used to detect circular imports.
               HasResolvedImports : 1,

               // If the module was or is being compiled with `-enable-private-imports`.
               PrivateImportsEnabled : 1,

               // If the module is compiled with `-enable-implicit-dynamic`.
               ImplicitDynamicEnabled : 1
               );

      POLAR_INLINE_BITFIELD(
               PrecedenceGroupDecl, Decl, 1+2,
               /// Is this an assignment operator?
               IsAssignment : 1,

               /// The group's associativity.  A value of the Associativity enum.
               Associativity : 2
               );

      POLAR_INLINE_BITFIELD(
               ImportDecl, Decl, 3+8,
               ImportKind : 3,

               /// The number of elements in this path.
               NumPathElements : 8
               );

      POLAR_INLINE_BITFIELD(
               ExtensionDecl, Decl, 3+1,
               /// An encoding of the default and maximum access level for this extension.
               ///
               /// This is encoded as (1 << (maxAccess-1)) | (1 << (defaultAccess-1)),
               /// which works because the maximum is always greater than or equal to the
               /// default, and 'private' is never used. 0 represents an uncomputed value.
               DefaultAndMaxAccessLevel : 3,

               /// Whether there is are lazily-loaded conformances for this extension.
               HasLazyConformances : 1
               );

      POLAR_INLINE_BITFIELD(
               IfConfigDecl, Decl, 1,
               /// Whether this decl is missing its closing '#endif'.
               HadMissingEnd : 1
               );

      POLAR_INLINE_BITFIELD(
               PoundDiagnosticDecl, Decl, 1+1,
               /// `true` if the diagnostic is an error, `false` if it's a warning.
               IsError : 1,

               /// Whether this diagnostic has already been emitted.
               HasBeenEmitted : 1
               );

      POLAR_INLINE_BITFIELD(
               MissingMemberDecl, Decl, 1+2,
               NumberOfFieldOffsetVectorEntries : 1,
               NumberOfVTableEntries : 2
               );

   } bits;

   // Storage for the declaration attributes.
   DeclAttributes m_attrs;

   /// The next declaration in the list of declarations within this
   /// member context.
   Decl *nextDecl = nullptr;

   friend class DeclIterator;
   friend class IterableDeclContext;
   friend class MemberLookupTable;

   private:
   PointerUnion<DeclContext *, AstContext *> m_context;

   Decl(const Decl&) = delete;
   void operator=(const Decl&) = delete;

   protected:

   Decl(DeclKind kind, PointerUnion<DeclContext *, AstContext *> context)
      : m_context(context)
   {
      bits.opaqueBits = 0;
      bits.Decl.Kind = unsigned(kind);
      bits.Decl.Invalid = false;
      bits.Decl.Implicit = false;
      bits.Decl.FromClang = false;
      bits.Decl.EarlyAttrValidation = false;
      bits.Decl.ValidationState = unsigned(ValidationState::Unchecked);
      bits.Decl.EscapedFromIfConfig = false;
   }

   DeclContext *getDeclContextForModule() const;

   public:
   DeclKind getKind() const
   {
      return DeclKind(bits.Decl.Kind);
   }

   /// Retrieve the name of the given declaration kind.
   ///
   /// This name should only be used for debugging dumps and other
   /// developer aids, and should never be part of a diagnostic or exposed
   /// to the user of the compiler in any way.
   static StringRef getKindName(DeclKind K);

   /// Retrieve the descriptive kind for this declaration.
   DescriptiveDeclKind getDescriptiveKind() const;

   /// Produce a name for the given descriptive declaration kind, which
   /// is suitable for use in diagnostics.
   static StringRef getDescriptiveKindName(DescriptiveDeclKind K);

   /// Whether swift users should be able to access this decl. For instance,
   /// var a.storage for lazy var a is an inaccessible decl. An inaccessible decl
   /// has to be implicit; but an implicit decl does not have to be inaccessible,
   /// for instance, self.
   bool isUserAccessible() const;

   /// Determine if the decl can have a comment.  If false, a comment will
   /// not be serialized.
   bool canHaveComment() const;

   POLAR_READONLY DeclContext *getDeclContext() const
   {
      if (auto dc = m_context.dynamicCast<DeclContext *>()) {
         return dc;
      }
      return getDeclContextForModule();
   }

   void setDeclContext(DeclContext *DC);

   /// Retrieve the innermost declaration context corresponding to this
   /// declaration, which will either be the declaration itself (if it's
   /// also a declaration context) or its declaration context.
   DeclContext *getInnermostDeclContext() const;

   /// Retrieve the module in which this declaration resides.
   POLAR_READONLY ModuleDecl *getModuleContext() const;

   /// getAstContext - Return the AstContext that this decl lives in.
   POLAR_READONLY AstContext &getAstContext() const
   {
      if (auto dc = m_context.dynamicCast<DeclContext *>()) {
         return dc->getAstContext();
      }
      return *m_context.get<AstContext *>();
   }

   const DeclAttributes &getAttrs() const
   {
      return m_attrs;
   }

   DeclAttributes &getAttrs()
   {
      return m_attrs;
   }

   /// Returns the starting location of the entire declaration.
   SourceLoc getStartLoc() const
   {
      return getSourceRange().m_start;
   }

   /// Returns the end location of the entire declaration.
   SourceLoc getEndLoc() const
   {
      return getSourceRange().m_end;
   }

   /// Returns the preferred location when referring to declarations
   /// in diagnostics.
   SourceLoc getLoc() const;

   /// Returns the source range of the entire declaration.
   SourceRange getSourceRange() const;

   /// Returns the source range of the declaration including its attributes.
   SourceRange getSourceRangeIncludingm_attrs() const;

   SourceLoc trailingSemiLoc;

   POLAR_ATTRIBUTE_DEPRECATED(
            void dump() const POLAR_ATTRIBUTE_USED,
            "only for use within the debugger");
   POLAR_ATTRIBUTE_DEPRECATED(
            void dump(const char *filename) const POLAR_ATTRIBUTE_USED,
            "only for use within the debugger");
   void dump(RawOutStream &outStream, unsigned ident = 0) const;

   /// Pretty-print the given declaration.
   ///
   /// \param outStream Output stream to which the declaration will be printed.
   void print(RawOutStream &outStream) const;
   void print(RawOutStream &outStream, const PrintOptions &opts) const;

   /// Pretty-print the given declaration.
   ///
   /// \param printer AstPrinter object.
   ///
   /// \param opts Options to control how pretty-printing is performed.
   ///
   /// \returns true if the declaration was printed or false if the print options
   /// required the declaration to be skipped from printing.
   bool print(AstPrinter &printer, const PrintOptions &opts) const;

   /// Determine whether this declaration should be printed when
   /// encountered in its declaration context's list of members.
   bool shouldPrintInContext(const PrintOptions &PO) const;

   bool walk(AstWalker &walker);

   /// Return whether this declaration has been determined invalid.
   bool isInvalid() const
   {
      return bits.Decl.Invalid;
   }

   /// Mark this declaration invalid.
   void setInvalid(bool isInvalid = true)
   {
      bits.Decl.Invalid = isInvalid;
   }

   /// Determine whether this declaration was implicitly generated by the
   /// compiler (rather than explicitly written in source code).
   bool isImplicit() const
   {
      return bits.Decl.Implicit;
   }

   /// Mark this declaration as implicit.
   void setImplicit(bool implicit = true)
   {
      bits.Decl.Implicit = implicit;
   }

   /// Whether we have already done early attribute validation.
   bool didEarlyAttrValidation() const
   {
      return bits.Decl.EarlyAttrValidation;
   }

   /// Set whether we've performed early attribute validation.
   void setEarlyAttrValidation(bool validated = true)
   {
      bits.Decl.EarlyAttrValidation = validated;
   }

   /// Get the validation state.
   ValidationState getValidationState() const
   {
      return ValidationState(bits.Decl.ValidationState);
   }

   private:
   friend class DeclValidationRAII;

   /// Set the validation state.
   void setValidationState(ValidationState VS)
   {
      assert(VS > getValidationState() && "Validation is unidirectional");
      bits.Decl.ValidationState = unsigned(VS);
   }

   public:
   /// Whether the declaration is in the middle of validation or not.
   bool isBeingValidated() const
   {
      switch (getValidationState()) {
      case ValidationState::Unchecked:
      case ValidationState::Checked:
         return false;
      case ValidationState::Checking:
      case ValidationState::CheckingWithValidSignature:
         return true;
      }
      polar_unreachable("Unknown ValidationState");
   }

   /// Update the validation state for the declaration to allow access to the
   /// generic signature.
   void setSignatureIsValidated()
   {
      assert(getValidationState() == ValidationState::Checking);
      setValidationState(ValidationState::CheckingWithValidSignature);
   }

   bool hasValidationStarted() const
   {
      return getValidationState() > ValidationState::Unchecked;
   }

   /// Manually indicate that validation is complete for the declaration. For
   /// example: during importing, code synthesis, or derived conformances.
   ///
   /// For normal code validation, please use DeclValidationRAII instead.
   ///
   /// FIXME -- Everything should use DeclValidationRAII instead of this.
   void setValidationToChecked()
   {
      if (!isBeingValidated())
         bits.Decl.ValidationState = unsigned(ValidationState::Checked);
   }

   bool escapedFromIfConfig() const
   {
      return bits.Decl.EscapedFromIfConfig;
   }

   void setEscapedFromIfConfig(bool Escaped)
   {
      bits.Decl.EscapedFromIfConfig = Escaped;
   }

   /// \returns the unparsed comment attached to this declaration.
   RawComment getRawComment() const;

   std::optional<StringRef> getGroupName() const;

   std::optional<StringRef> getSourceFileName() const;

   std::optional<unsigned> getSourceOrder() const;

   /// \returns the brief comment attached to this declaration.
   StringRef getBriefComment() const;

   /// Return the GenericContext if the Decl has one.
   POLAR_READONLY const GenericContext *getAsGenericContext() const;

   bool isPrivateStdlibDecl(bool treatNonBuiltinProtocolsAsPublic = true) const;

   /// Whether this declaration is weak-imported.
   bool isWeakImported(ModuleDecl *fromModule) const;

   /// Returns true if the nature of this declaration allows overrides.
   /// Note that this does not consider whether it is final or whether
   /// the class it's on is final.
   ///
   /// If this returns true, the decl can be safely casted to ValueDecl.
   bool isPotentiallyOverridable() const;

   /// Emit a diagnostic tied to this declaration.
   //   template<typename ...ArgTypes>
   //   InFlightDiagnostic diagnose(
   //            Diag<ArgTypes...> ID,
   //            typename internal::PassArgument<ArgTypes>::type... Args) const
   //   {
   //      return getDiags().diagnose(this, ID, std::move(Args)...);
   //   }

   /// Retrieve the diagnostic engine for diagnostics emission.
   POLAR_READONLY DiagnosticEngine &getDiags() const;

   // Make vanilla new/delete illegal for Decls.
   void *operator new(size_t bytes) = delete;
   void operator delete(void *Data) = delete;

   // Only allow allocation of Decls using the allocator in AstContext
   // or by doing a placement new.
   void *operator new(size_t bytes, const AstContext &context,
                      unsigned alignment = alignof(Decl));
   void *operator new(size_t bytes, void *mem)
   {
      assert(mem);
      return mem;
   }
};

/// Use RAII to track Decl validation progress and non-reentrancy.
class DeclValidationRAII
{
   Decl *D;

public:
   DeclValidationRAII(const DeclValidationRAII &) = delete;
   DeclValidationRAII(DeclValidationRAII &&) = delete;
   void operator =(const DeclValidationRAII &) = delete;
   void operator =(DeclValidationRAII &&) = delete;

   DeclValidationRAII(Decl *decl) : D(decl)
   {
      D->setValidationState(Decl::ValidationState::Checking);
   }

   ~DeclValidationRAII()
   {
      D->setValidationState(Decl::ValidationState::Checked);
   }
};

// A private class for forcing exact field layout.
class alignas(8) _GenericContext
{
   // Not really public. See GenericContext.
   public:
   GenericParamList *genericParams = nullptr;

   /// The trailing where clause.
   ///
   /// Note that this is not currently serialized, because semantic analysis
   /// moves the trailing where clause into the generic parameter list.
   TrailingWhereClause *trailingWhere = nullptr;

   /// The generic signature or environment of this declaration.
   ///
   /// When this declaration stores only a signature, the generic
   /// environment will be lazily loaded.
   mutable PointerUnion<GenericSignature *, GenericEnvironment *>
         genericSigOrEnv;
};

class GenericContext : private _GenericContext, public DeclContext
{
   /// Lazily populate the generic environment.
   GenericEnvironment *getLazyGenericEnvironmentSlow() const;

protected:
   GenericContext(DeclContextKind kind, DeclContext *parent)
      : _GenericContext(),
        DeclContext(kind, parent)
   {}

public:
   /// Retrieve the set of parameters to a generic context, or null if
   /// this context is not generic.
   GenericParamList *getGenericParams() const
   {
      return genericParams;
   }

   void setGenericParams(GenericParamList *GenericParams);

   /// Determine whether this context has generic parameters
   /// of its own.
   bool isGeneric() const
   {
      return genericParams != nullptr;
   }

   /// Retrieve the trailing where clause for this extension, if any.
   TrailingWhereClause *getTrailingWhereClause() const
   {
      return trailingWhere;
   }

   /// Set the trailing where clause for this extension.
   void setTrailingWhereClause(TrailingWhereClause *trailingWhereClause)
   {
      trailingWhere = trailingWhereClause;
   }

   /// Retrieve the generic signature for this context.
   GenericSignature *getGenericSignature() const;

   /// Retrieve the generic context for this context.
   GenericEnvironment *getGenericEnvironment() const;

   /// Retrieve the innermost generic parameter types.
   TypeArrayView<GenericTypeParamType> getInnermostGenericParamTypes() const;

   /// Retrieve the generic requirements.
   ArrayRef<Requirement> getGenericRequirements() const;

   /// Set a lazy generic environment.
   void setLazyGenericEnvironment(LazyMemberLoader *lazyLoader,
                                  GenericSignature *genericSig,
                                  uint64_t genericEnvData);

   /// Whether this generic context has a lazily-created generic environment
   /// that has not yet been constructed.
   bool hasLazyGenericEnvironment() const;

   /// Set the generic context of this context.
   void setGenericEnvironment(GenericEnvironment *genericEnv);

   /// Retrieve the position of any where clause for this context's
   /// generic parameters.
   SourceRange getGenericTrailingWhereClauseSourceRange() const;
};

static_assert(sizeof(_GenericContext) + sizeof(DeclContext) ==
              sizeof(GenericContext), "Please add fields to _GenericContext");

} // polar::ast

#endif // POLARPHP_AST_DECL_H
