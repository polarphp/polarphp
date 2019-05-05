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
#include "polarphp/basic/adt/TinyPtrVector.h"
#include "polarphp/basic/adt/OptionalEnum.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/ast/DeclContext.h"
#include "polarphp/ast/Attr.h"
#include "polarphp/ast/AccessScope.h"
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
using polar::basic::TinyPtrVector;
using polar::basic::OptionalEnum;

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
#define DECL(Id, parent) Id,
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
               kind : polar::basic::bitmax(NumDeclKindBits,8),
               /// Whether this declaration is invalid.
               invalid : 1,

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

               /// Backing m_bits for 'self' access kind.
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

   } m_bits;

   // Storage for the declaration attributes.
   DeclAttributes m_attrs;

   /// The next declaration in the list of declarations within this
   /// member context.
   Decl *m_nextDecl = nullptr;

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
      m_bits.opaqueBits = 0;
      m_bits.Decl.kind = unsigned(kind);
      m_bits.Decl.invalid = false;
      m_bits.Decl.Implicit = false;
      m_bits.Decl.FromClang = false;
      m_bits.Decl.EarlyAttrValidation = false;
      m_bits.Decl.ValidationState = unsigned(ValidationState::Unchecked);
      m_bits.Decl.EscapedFromIfConfig = false;
   }

   DeclContext *getDeclContextForModule() const;

   public:
   DeclKind getKind() const
   {
      return DeclKind(m_bits.Decl.kind);
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
//      if (auto dc = m_context.dynamicCast<DeclContext *>()) {
//         return dc;
//      }
//      return getDeclContextForModule();
      return nullptr;
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
      return m_bits.Decl.invalid;
   }

   /// Mark this declaration invalid.
   void setInvalid(bool isInvalid = true)
   {
      m_bits.Decl.invalid = isInvalid;
   }

   /// Determine whether this declaration was implicitly generated by the
   /// compiler (rather than explicitly written in source code).
   bool isImplicit() const
   {
      return m_bits.Decl.Implicit;
   }

   /// Mark this declaration as implicit.
   void setImplicit(bool implicit = true)
   {
      m_bits.Decl.Implicit = implicit;
   }

   /// Whether we have already done early attribute validation.
   bool didEarlyAttrValidation() const
   {
      return m_bits.Decl.EarlyAttrValidation;
   }

   /// Set whether we've performed early attribute validation.
   void setEarlyAttrValidation(bool validated = true)
   {
      m_bits.Decl.EarlyAttrValidation = validated;
   }

   /// Get the validation state.
   ValidationState getValidationState() const
   {
      return ValidationState(m_bits.Decl.ValidationState);
   }

   private:
   friend class DeclValidationRAII;

   /// Set the validation state.
   void setValidationState(ValidationState VS)
   {
      assert(VS > getValidationState() && "Validation is unidirectional");
      m_bits.Decl.ValidationState = unsigned(VS);
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
         m_bits.Decl.ValidationState = unsigned(ValidationState::Checked);
   }

   bool escapedFromIfConfig() const
   {
      return m_bits.Decl.EscapedFromIfConfig;
   }

   void setEscapedFromIfConfig(bool Escaped)
   {
      m_bits.Decl.EscapedFromIfConfig = Escaped;
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

   // Make vanilla new/delete illegal for m_decls.
   void *operator new(size_t bytes) = delete;
   void operator delete(void *Data) = delete;

   // Only allow allocation of m_decls using the allocator in AstContext
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

/// Allocates memory for a Decl with the given \p baseSize. If necessary,
/// it includes additional space immediately preceding the Decl for a ClangNode.
/// \note \p baseSize does not need to include space for a ClangNode if
/// requested -- the necessary space will be added automatically.
template <typename DeclTy, typename AllocatorTy>
void *allocate_memory_for_decl(AllocatorTy &allocator, size_t baseSize,
                               bool includeSpaceForClangNode)
{
   static_assert(alignof(DeclTy) >= sizeof(void *),
                 "A pointer must fit in the alignment of the DeclTy!");

   size_t size = baseSize;
   if (includeSpaceForClangNode) {
      size += alignof(DeclTy);
   }
   void *mem = allocator.allocate(size, alignof(DeclTy));
   if (includeSpaceForClangNode) {
      mem = reinterpret_cast<char *>(mem) + alignof(DeclTy);
   }
   return mem;
}

enum class RequirementReprKind : unsigned
{
   /// A type bound T : P, where T is a type that depends on a generic
   /// parameter and P is some type that should bound T, either as a concrete
   /// supertype or a protocol to which T must conform.
   TypeConstraint,

   /// A same-type requirement T == U, where T and U are types that shall be
   /// equivalent.
   SameType,

   /// A layout bound T : L, where T is a type that depends on a generic
   /// parameter and L is some layout specification that should bound T.
   LayoutConstraint,

   // Note: there is code that packs this enum in a 2-bit bitfield.  Audit users
   // when adding enumerators.
};

/// A single requirement in a 'where' clause, which places additional
/// restrictions on the generic parameters or associated types of a generic
/// function, type, or protocol.
///
/// This always represents a requirement spelled in the source code.  It is
/// never generated implicitly.
///
/// \c GenericParamList assumes these are POD-like.
class RequirementRepr
{
private:
   RequirementRepr(SourceLoc separatorLoc, RequirementReprKind kind,
                   TypeLoc firstType, TypeLoc secondType)
      : m_separatorLoc(separatorLoc),
        m_kind(kind),
        m_invalid(false),
        m_firstType(firstType),
        m_secondType(secondType)
   {}

   void printImpl(AstPrinter &outStream, bool asWritten) const;

public:
   /// Construct a new type-constraint requirement.
   ///
   /// \param subject The type that must conform to the given protocol or
   /// composition, or be a subclass of the given class type.
   /// \param colonLoc The location of the ':', or an invalid location if
   /// this requirement was implied.
   /// \param constraint The protocol or protocol composition to which the
   /// subject must conform, or superclass from which the subject must inherit.
   static RequirementRepr getTypeConstraint(TypeLoc subject,
                                            SourceLoc colonLoc,
                                            TypeLoc constraint)
   {
      return { colonLoc, RequirementReprKind::TypeConstraint, subject, constraint };
   }

   /// Construct a new same-type requirement.
   ///
   /// \param m_firstType The first type.
   /// \param EqualLoc The location of the '==' in the same-type constraint, or
   /// an invalid location if this requirement was implied.
   /// \param m_secondType The second type.
   static RequirementRepr getSameType(TypeLoc firstType,
                                      SourceLoc equalLoc,
                                      TypeLoc secondType)
   {
      return { equalLoc, RequirementReprKind::SameType, firstType, secondType };
   }

   /// Determine the kind of requirement
   RequirementReprKind getKind() const
   {
      return m_kind;
   }

   /// Determine whether this requirement is invalid.
   bool isInvalid() const
   {
      return m_invalid;
   }

   /// Mark this requirement invalid.
   void setInvalid()
   {
      m_invalid = true;
   }

   /// For a type-bound requirement, return the subject of the
   /// conformance relationship.
   Type getSubject() const
   {
      assert(getKind() == RequirementReprKind::TypeConstraint ||
             getKind() == RequirementReprKind::LayoutConstraint);
      return m_firstType.getType();
   }

   TypeRepr *getSubjectRepr() const
   {
      assert(getKind() == RequirementReprKind::TypeConstraint ||
             getKind() == RequirementReprKind::LayoutConstraint);
      return m_firstType.getTypeRepr();
   }

   TypeLoc &getSubjectLoc()
   {
      assert(getKind() == RequirementReprKind::TypeConstraint ||
             getKind() == RequirementReprKind::LayoutConstraint);
      return m_firstType;
   }

   const TypeLoc &getSubjectLoc() const
   {
      assert(getKind() == RequirementReprKind::TypeConstraint ||
             getKind() == RequirementReprKind::LayoutConstraint);
      return m_firstType;
   }

   /// For a type-bound requirement, return the protocol or to which
   /// the subject conforms or superclass it inherits.
   Type getConstraint() const
   {
      assert(getKind() == RequirementReprKind::TypeConstraint);
      return m_secondType.getType();
   }

   TypeRepr *getConstraintRepr() const
   {
      assert(getKind() == RequirementReprKind::TypeConstraint);
      return m_secondType.getTypeRepr();
   }

   TypeLoc &getConstraintLoc()
   {
      assert(getKind() == RequirementReprKind::TypeConstraint);
      return m_secondType;
   }

   const TypeLoc &getConstraintLoc() const
   {
      assert(getKind() == RequirementReprKind::TypeConstraint);
      return m_secondType;
   }

   /// Retrieve the first type of a same-type requirement.
   Type getFirstType() const
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_firstType.getType();
   }

   TypeRepr *getFirstTypeRepr() const
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_firstType.getTypeRepr();
   }

   TypeLoc &getFirstTypeLoc()
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_firstType;
   }

   const TypeLoc &getFirstTypeLoc() const
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_firstType;
   }

   /// Retrieve the second type of a same-type requirement.
   Type getSecondType() const
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_secondType.getType();
   }

   TypeRepr *getSecondTypeRepr() const
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_secondType.getTypeRepr();
   }

   TypeLoc &getSecondTypeLoc()
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_secondType;
   }

   const TypeLoc &getSecondTypeLoc() const
   {
      assert(getKind() == RequirementReprKind::SameType);
      return m_secondType;
   }

   /// Retrieve the location of the ':' or '==' in an explicitly-written
   /// conformance or same-type requirement respectively.
   SourceLoc getSeparatorLoc() const
   {
      return m_separatorLoc;
   }

   SourceRange getSourceRange() const
   {
      //      if (getKind() == RequirementReprKind::LayoutConstraint)
      //         return SourceRange(m_firstType.getSourceRange().Start,
      //                            SecondLayout.getSourceRange().End);
      return SourceRange(m_firstType.getSourceRange().m_start,
                         m_secondType.getSourceRange().m_end);
   }

   /// Retrieve the first or subject type representation from the \c repr,
   /// or \c nullptr if \c repr is null.
   static TypeRepr *getFirstTypeRepr(const RequirementRepr *repr)
   {
      if (!repr) {
         return nullptr;
      }
      return repr->m_firstType.getTypeRepr();
   }

   /// Retrieve the second or constraint type representation from the \c repr,
   /// or \c nullptr if \c repr is null.
   static TypeRepr *getSecondTypeRepr(const RequirementRepr *repr)
   {
      if (!repr) {
         return nullptr;
      }
      assert(repr->getKind() == RequirementReprKind::TypeConstraint ||
             repr->getKind() == RequirementReprKind::SameType);
      return repr->m_secondType.getTypeRepr();
   }

   POLAR_ATTRIBUTE_DEPRECATED(
         void dump() const POLAR_ATTRIBUTE_USED,
         "only for use within the debugger");
   void print(RawOutStream &outStream) const;
   void print(AstPrinter &printer) const;
private:
   SourceLoc m_separatorLoc;
   RequirementReprKind m_kind : 2;
   bool m_invalid : 1;
   TypeLoc m_firstType;

   /// The second element represents the right-hand side of the constraint.
   /// It can be e.g. a type or a layout constraint.
   union
   {
      TypeLoc m_secondType;
   };

   /// Set during deserialization; used to print out the requirements accurately
   /// for the generated interface.
   StringRef m_asWrittenString;
};


/// GenericParamList - A list of generic parameters that is part of a generic
/// function or type, along with extra requirements placed on those generic
/// parameters and types derived from them.
class GenericParamList final :
      private TrailingObjects<GenericParamList, GenericTypeParamDecl *>
{
   friend class TrailingObjects;
   SourceRange m_brackets;
   unsigned m_numParams;
   SourceLoc m_whereLoc;
   MutableArrayRef<RequirementRepr> m_requirements;
   GenericParamList *m_outerParameters;
   SourceLoc m_trailingWhereLoc;
   unsigned m_firstTrailingWhereArg;

   GenericParamList(SourceLoc lAngleLoc,
                    ArrayRef<GenericTypeParamDecl *> params,
                    SourceLoc whereLoc,
                    MutableArrayRef<RequirementRepr> requirements,
                    SourceLoc rAngleLoc);

   // Don't copy.
   GenericParamList(const GenericParamList &) = delete;
   GenericParamList &operator=(const GenericParamList &) = delete;

public:
   /// create - Create a new generic parameter list within the given AST context.
   ///
   /// \param context The AstContext in which the generic parameter list will
   /// be allocated.
   /// \param lAngleLoc The location of the opening angle bracket ('<')
   /// \param params The list of generic parameters, which will be copied into
   /// AstContext-allocated memory.
   /// \param rAngleLoc The location of the closing angle bracket ('>')
   static GenericParamList *create(AstContext &context,
                                   SourceLoc lAngleLoc,
                                   ArrayRef<GenericTypeParamDecl *> params,
                                   SourceLoc rAngleLoc);

   /// create - Create a new generic parameter list and "where" clause within
   /// the given AST context.
   ///
   /// \param context The AstContext in which the generic parameter list will
   /// be allocated.
   /// \param lAngleLoc The location of the opening angle bracket ('<')
   /// \param params The list of generic parameters, which will be copied into
   /// AstContext-allocated memory.
   /// \param m_whereLoc The location of the 'where' keyword, if any.
   /// \param m_requirements The list of requirements, which will be copied into
   /// AstContext-allocated memory.
   /// \param rAngleLoc The location of the closing angle bracket ('>')
   static GenericParamList *create(const AstContext &context,
                                   SourceLoc lAngleLoc,
                                   ArrayRef<GenericTypeParamDecl *> params,
                                   SourceLoc whereLoc,
                                   ArrayRef<RequirementRepr> requirements,
                                   SourceLoc rAngleLoc);

   MutableArrayRef<GenericTypeParamDecl *> getParams()
   {
      return {getTrailingObjects<GenericTypeParamDecl *>(), m_numParams};
   }

   ArrayRef<GenericTypeParamDecl *> getParams() const
   {
      return {getTrailingObjects<GenericTypeParamDecl *>(), m_numParams};
   }

   using iterator = GenericTypeParamDecl **;
   using const_iterator = const GenericTypeParamDecl * const *;

   unsigned size() const
   {
      return m_numParams;
   }

   iterator begin()
   {
      return getParams().begin();
   }

   iterator end()
   {
      return getParams().end();
   }

   const_iterator begin() const
   {
      return getParams().begin();
   }

   const_iterator end() const
   {
      return getParams().end();
   }

   /// Retrieve the location of the 'where' keyword, or an invalid
   /// location if 'where' was not present.
   SourceLoc getWhereLoc() const
   {
      return m_whereLoc;
   }

   /// Retrieve the set of additional requirements placed on these
   /// generic parameters and types derived from them.
   ///
   /// This list may contain both explicitly-written requirements as well as
   /// implicitly-generated requirements, and may be non-empty even if no
   /// 'where' keyword is present.
   MutableArrayRef<RequirementRepr> getRequirements()
   {
      return m_requirements;
   }

   /// Retrieve the set of additional requirements placed on these
   /// generic parameters and types derived from them.
   ///
   /// This list may contain both explicitly-written requirements as well as
   /// implicitly-generated requirements, and may be non-empty even if no
   /// 'where' keyword is present.
   ArrayRef<RequirementRepr> getRequirements() const
   {
      return m_requirements;
   }

   /// Retrieve only those requirements that are written within the brackets,
   /// which does not include any requirements written in a trailing where
   /// clause.
   ArrayRef<RequirementRepr> getNonTrailingRequirements() const
   {
      return m_requirements.slice(0, m_firstTrailingWhereArg);
   }

   /// Retrieve only those requirements written in a trailing where
   /// clause.
   ArrayRef<RequirementRepr> getTrailingRequirements() const
   {
      return m_requirements.slice(m_firstTrailingWhereArg);
   }

   /// Determine whether the generic parameters have a trailing where clause.
   bool hasTrailingWhereClause() const
   {
      return m_firstTrailingWhereArg < m_requirements.size();
   }

   /// Add a trailing 'where' clause to the list of requirements.
   ///
   /// Trailing where clauses are written outside the angle brackets, after the
   /// main part of a declaration's signature.
   void addTrailingWhereClause(AstContext &ctx, SourceLoc trailingWhereLoc,
                               ArrayRef<RequirementRepr> trailingRequirements);

   /// Retrieve the outer generic parameter list.
   ///
   /// This is used for extensions of nested types, and in SIL mode, where a
   /// single lexical context can have multiple logical generic parameter
   /// lists.
   GenericParamList *getOuterParameters() const
   {
      return m_outerParameters;
   }

   /// Set the outer generic parameter list. See \c getOuterParameters
   /// for more information.
   void setOuterParameters(GenericParamList *outer)
   {
      m_outerParameters = outer;
   }

   SourceLoc getLAngleLoc() const
   {
      return m_brackets.m_start;
   }

   SourceLoc getRAngleLoc() const
   {
      return m_brackets.m_end;
   }

   SourceRange getSourceRange() const
   {
      return m_brackets;
   }

   /// Retrieve the source range covering the where clause.
   SourceRange getWhereClauseSourceRange() const
   {
      if (m_whereLoc.isInvalid()) {
         return SourceRange();
      }
      auto endLoc = m_requirements[m_firstTrailingWhereArg-1].getSourceRange().m_end;
      return SourceRange(m_whereLoc, endLoc);
   }

   /// Retrieve the source range covering the trailing where clause.
   SourceRange getTrailingWhereClauseSourceRange() const
   {
      if (!hasTrailingWhereClause()) {
         return SourceRange();
      }
      return SourceRange(m_trailingWhereLoc,
                         m_requirements.back().getSourceRange().m_end);
   }

   /// Configure the depth of the generic parameters in this list.
   void setDepth(unsigned depth);

   /// Create a copy of the generic parameter list and all of its generic
   /// parameter declarations. The copied generic parameters are re-parented
   /// to the given DeclContext.
   GenericParamList *clone(DeclContext *dc) const;
   void print(RawOutStream &outStream);
   void dump();
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

/// Describes what kind of name is being imported.
///
/// If the enumerators here are changed, make sure to update all diagnostics
/// using ImportKind as a select index.
enum class ImportKind : uint8_t
{
   Module = 0,
   Type,
   Struct,
   Class,
   Enum,
   Protocol,
   Var,
   Func
};

/// ImportDecl - This represents a single import declaration, e.g.:
///   import Swift
///   import typealias Swift.Int
class ImportDecl final : public Decl,
      private TrailingObjects<ImportDecl, std::pair<Identifier,SourceLoc>>
{
   friend class TrailingObjects;

public:
   typedef std::pair<Identifier, SourceLoc> AccessPathElement;

private:
   SourceLoc m_importLoc;
   SourceLoc m_kindLoc;

   /// The resolved module.
   ModuleDecl *m_mod = nullptr;
   /// The resolved m_decls if this is a decl import.
   ArrayRef<ValueDecl *> m_decls;

   ImportDecl(DeclContext *declContext, SourceLoc importLoc, ImportKind kind,
              SourceLoc kindLoc, ArrayRef<AccessPathElement> path);

public:
   static ImportDecl *create(AstContext &context, DeclContext *declContext,
                             SourceLoc importLoc, ImportKind kind,
                             SourceLoc kindLoc,
                             ArrayRef<AccessPathElement> path);

   /// Returns the import kind that is most appropriate for \p VD.
   ///
   /// Note that this will never return \c Type; an imported typealias will use
   /// the more specific kind from its underlying type.
   static ImportKind getBestImportKind(const ValueDecl *valueDecl);

   /// Returns the most appropriate import kind for the given list of m_decls.
   ///
   /// If the list is non-homogeneous, or if there is more than one decl that
   /// cannot be overloaded, returns None.
   static std::optional<ImportKind> findBestImportKind(ArrayRef<ValueDecl *> decls);

   ArrayRef<AccessPathElement> getFullAccessPath() const
   {
      return {getTrailingObjects<AccessPathElement>(),
               static_cast<size_t>(m_bits.ImportDecl.NumPathElements)};
   }

   ArrayRef<AccessPathElement> getModulePath() const
   {
      auto result = getFullAccessPath();
      if (getImportKind() != ImportKind::Module) {
         result = result.slice(0, result.size()-1);
      }
      return result;
   }

   ArrayRef<AccessPathElement> getDeclPath() const
   {
      if (getImportKind() == ImportKind::Module) {
         return {};
      }
      return getFullAccessPath().back();
   }

   ImportKind getImportKind() const
   {
      return static_cast<ImportKind>(m_bits.ImportDecl.ImportKind);
   }

   bool isExported() const
   {
      //      return getAttrs().hasAttribute<ExportedAttr>();
   }

   ModuleDecl *getModule() const
   {
      return m_mod;
   }

   void setModule(ModuleDecl *module)
   {
      m_mod = module;
   }

   ArrayRef<ValueDecl *> getDecls() const
   {
      return m_decls;
   }

   void setDecls(ArrayRef<ValueDecl *> decl)
   {
      m_decls = decl;
   }

   SourceLoc getStartLoc() const
   {
      return m_importLoc;
   }

   SourceLoc getLoc() const
   {
      return getFullAccessPath().front().second;
   }

   SourceRange getSourceRange() const
   {
      return SourceRange(m_importLoc, getFullAccessPath().back().second);
   }

   SourceLoc getKindLoc() const
   {
      return m_kindLoc;
   }

   static bool classof(const Decl *decl)
   {
      return decl->getKind() == DeclKind::Import;
   }
};

/// TopLevelCodeDecl - This decl is used as a container for top-level
/// expressions and statements in the main module.  It is always a direct
/// child of a SourceFile.  The primary reason for building these is to give
/// top-level statements a DeclContext which is distinct from the file itself.
/// This, among other things, makes it easier to distinguish between local
/// top-level variables (which are not live past the end of the statement) and
/// global variables.
class TopLevelCodeDecl : public DeclContext, public Decl
{
public:
   TopLevelCodeDecl(DeclContext *parent, BraceStmt *body = nullptr)
      : DeclContext(DeclContextKind::TopLevelCodeDecl, parent),
        Decl(DeclKind::TopLevelCode, parent),
        m_body(body)
   {}

   BraceStmt *getBody() const
   {
      return m_body;
   }

   void setBody(BraceStmt *stmt)
   {
      m_body = stmt;
   }

   SourceLoc getStartLoc() const;
   SourceLoc getLoc() const
   {
      return getStartLoc();
   }

   SourceRange getSourceRange() const;

   static bool classof(const Decl *decl)
   {
      return decl->getKind() == DeclKind::TopLevelCode;
   }

   static bool classof(const DeclContext *context)
   {
      if (auto decl = context->getAsDecl()) {
         return classof(decl);
      }
      return false;
   }

   using DeclContext::operator new;
private:
   BraceStmt *m_body;
};

/// SerializedTopLevelCodeDeclContext - This represents what was originally a
/// TopLevelCodeDecl during serialization. It is preserved only to maintain the
/// correct AST structure and remangling after deserialization.
class SerializedTopLevelCodeDeclContext : public SerializedLocalDeclContext
{
public:
   SerializedTopLevelCodeDeclContext(DeclContext *parent)
      : SerializedLocalDeclContext(LocalDeclContextKind::TopLevelCodeDecl,
                                   parent)
   {}

   static bool classof(const DeclContext *declContext)
   {
      //      if (auto ldc = dyn_cast<SerializedLocalDeclContext>(declContext)) {
      //         return ldc->getLocalDeclContextKind() ==
      //               LocalDeclContextKind::TopLevelCodeDecl;
      //      }
      return false;
   }
};


/// ValueDecl - All named decls that are values in the language.  These can
/// have a type, etc.
class ValueDecl : public Decl
{
private:
   DeclName m_name;
   SourceLoc m_nameLoc;
   PointerIntPair<Type, 3, OptionalEnum<AccessLevel>> m_typeAndAccess;
   unsigned m_localDiscriminator = 0;

   struct
   {
      /// Whether the "overridden" declarations have been computed already.
      unsigned hasOverriddenComputed : 1;

      /// Whether there are any "overridden" declarations. The actual overridden
      /// declarations are kept in a side table in the AstContext.
      unsigned hasOverridden : 1;

      /// Whether the "isDynamic" bit has been computed yet.
      unsigned isDynamicComputed : 1;

      /// Whether this declaration is 'dynamic', meaning that all uses of
      /// the declaration will go through an extra level of indirection that
      /// allows the entity to be replaced at runtime.
      unsigned isDynamic : 1;
   } m_lazySemanticInfo;

   friend class OverriddenDeclsRequest;
   friend class IsDynamicRequest;
protected:
   ValueDecl(DeclKind kind,
             PointerUnion<DeclContext *, AstContext *> context,
             DeclName name, SourceLoc nameLoc)
      : Decl(kind, context),
        m_name(name),
        m_nameLoc(nameLoc)
   {
      m_bits.ValueDecl.AlreadyInLookupTable = false;
      m_bits.ValueDecl.CheckedRedeclaration = false;
      m_bits.ValueDecl.IsUserAccessible = true;
      m_lazySemanticInfo.hasOverriddenComputed = false;
      m_lazySemanticInfo.hasOverridden = false;
      m_lazySemanticInfo.isDynamicComputed = false;
      m_lazySemanticInfo.isDynamic = false;
   }

   // MemberLookupTable borrows a bit from this type
   friend class MemberLookupTable;
   bool isAlreadyInLookupTable()
   {
      return m_bits.ValueDecl.AlreadyInLookupTable;
   }

   void setAlreadyInLookupTable(bool value = true)
   {
      m_bits.ValueDecl.AlreadyInLookupTable = value;
   }

public:
   /// Return true if this protocol member is a protocol requirement.
   ///
   /// Asserts if this is not a member of a protocol.
   bool isProtocolRequirement() const;

   /// Determine whether we have already checked whether this
   /// declaration is a redeclaration.
   bool alreadyCheckedRedeclaration() const
   {
      return m_bits.ValueDecl.CheckedRedeclaration;
   }

   /// Set whether we have already checked this declaration as a
   /// redeclaration.
   void setCheckedRedeclaration(bool checked)
   {
      m_bits.ValueDecl.CheckedRedeclaration = checked;
   }

   void setUserAccessible(bool accessible)
   {
      m_bits.ValueDecl.IsUserAccessible = accessible;
   }

   bool isUserAccessible() const
   {
      return m_bits.ValueDecl.IsUserAccessible;
   }

   bool hasName() const
   {
      return bool(m_name);
   }

   bool isOperator() const
   {
      return m_name.isOperator();
   }

   /// Retrieve the full name of the declaration.
   /// TODO: Rename to getName?
   DeclName getFullName() const
   {
      return m_name;
   }

   void setName(DeclName name)
   {
      m_name = name;
   }

   /// Retrieve the base name of the declaration, ignoring any argument
   /// names.
   DeclBaseName getBaseName() const
   {
      return m_name.getBaseName();
   }

   SourceLoc getNameLoc() const
   {
      return m_nameLoc;
   }

   SourceLoc getLoc() const
   {
      return m_nameLoc;
   }

   bool isUsableFromInline() const;

   /// Returns \c true if this declaration is *not* intended to be used directly
   /// by application developers despite the visibility.
   bool shouldHideFromEditor() const;

   bool hasAccess() const
   {
      return m_typeAndAccess.getInt().hasValue();
   }

   /// Access control is done by Requests.
   friend class AccessLevelRequest;

   /// Returns the access level specified explicitly by the user, or provided by
   /// default according to language rules.
   ///
   /// Most of the time this is not the interesting value to check; access is
   /// limited by enclosing scopes per SE-0025. Use #getFormalAccessScope to
   /// check if access control is being used consistently, and to take features
   /// such as \c \@testable and \c \@usableFromInline into account.
   ///
   /// \sa getFormalAccessScope
   /// \sa hasOpenAccess
   AccessLevel getFormalAccess() const;

   /// Determine whether this Decl has either Private or FilePrivate access,
   /// and its DeclContext does not.
   bool isOutermostPrivateOrFilePrivateScope() const;

   /// Returns the outermost DeclContext from which this declaration can be
   /// accessed, or null if the declaration is public.
   ///
   /// This is used when calculating if access control is being used
   /// consistently. If \p useDC is provided (the location where the value is
   /// being used), features that affect formal access such as \c \@testable are
   /// taken into account.
   ///
   /// \invariant
   /// <code>value.isAccessibleFrom(
   ///     value.getFormalAccessScope().getDeclContext())</code>
   ///
   /// If \p treatUsableFromInlineAsPublic is true, declarations marked with the
   /// \c @usableFromInline attribute are treated as public. This is normally
   /// false for name lookup and other source language concerns, but true when
   /// computing the linkage of generated functions.
   ///
   /// \sa getFormalAccess
   /// \sa isAccessibleFrom
   /// \sa hasOpenAccess
   AccessScope
   getFormalAccessScope(const DeclContext *useDC = nullptr,
                        bool treatUsableFromInlineAsPublic = false) const;


   /// Copy the formal access level and @usableFromInline attribute from
   /// \p source.
   ///
   /// If \p sourceIsParentContext is true, an access level of \c private will
   /// be copied as \c fileprivate, to ensure that this declaration will be
   /// available everywhere \p source is.
   void copyFormalAccessFrom(const ValueDecl *source,
                             bool sourceIsParentContext = false);

   /// Returns the access level that actually controls how a declaration should
   /// be emitted and may be used.
   ///
   /// This is the access used when making optimization and code generation
   /// decisions. It should not be used at the AST or semantic level.
   AccessLevel getEffectiveAccess() const;

   void setAccess(AccessLevel access)
   {
      assert(!hasAccess() && "access already set");
      overwriteAccess(access);
   }

   /// Overwrite the access of this declaration.
   ///
   /// This is needed in the LLDB REPL.
   void overwriteAccess(AccessLevel access)
   {
      m_typeAndAccess.setInt(access);
   }

   /// Returns true if this declaration is accessible from the given context.
   ///
   /// A private declaration is accessible from any DeclContext within the same
   /// source file.
   ///
   /// An internal declaration is accessible from any DeclContext within the same
   /// module.
   ///
   /// A public declaration is accessible everywhere.
   ///
   /// If \p DC is null, returns true only if this declaration is public.
   ///
   /// If \p forConformance is true, we ignore the visibility of the protocol
   /// when evaluating protocol extension members. This language rule allows a
   /// protocol extension of a private protocol to provide default
   /// implementations for the requirements of a public protocol, even when
   /// the default implementations are not visible to name lookup.
   bool isAccessibleFrom(const DeclContext *declContext,
                         bool forConformance = false) const;

   /// Returns whether this declaration should be treated as \c open from
   /// \p useDC. This is very similar to #getFormalAccess, but takes
   /// \c \@testable into account.
   ///
   /// This is mostly only useful when considering requirements on an override:
   /// if the base declaration is \c open, the override might have to be too.
   bool hasOpenAccess(const DeclContext *useDC) const;

   /// Retrieve the "interface" type of this value, which uses
   /// GenericTypeParamType if the declaration is generic. For a generic
   /// function, this will have a GenericFunctionType with a
   /// GenericSignature inside the type.
   Type getInterfaceType() const;
   bool hasInterfaceType() const;

   /// Set the interface type for the given value.
   void setInterfaceType(Type type);

   bool hasValidSignature() const;

   /// isSettable - Determine whether references to this decl may appear
   /// on the left-hand side of an assignment or as the operand of a
   /// `&` or 'inout' operator.
   bool isSettable(const DeclContext *useDC,
                   const DeclRefExpr *base = nullptr) const;

   /// isInstanceMember - Determine whether this value is an instance member
   /// of an enum or protocol.
   bool isInstanceMember() const;

   /// Retrieve the context discriminator for this local value, which
   /// is the index of this declaration in the sequence of
   /// discriminated declarations with the same name in the current
   /// context.  Only local functions and variables with getters and
   /// setters have discriminators.
   unsigned getLocalDiscriminator() const;
   void setLocalDiscriminator(unsigned index);

   /// Retrieve the declaration that this declaration overrides, if any.
   ValueDecl *getOverriddenDecl() const;

   /// Retrieve the declarations that this declaration overrides, if any.
   TinyPtrVector<ValueDecl *> getOverriddenDecls() const;

   /// Set the declaration that this declaration overrides.
   void setOverriddenDecl(ValueDecl *overridden)
   {
      setOverriddenDecls(overridden);
   }

   /// Set the declarations that this declaration overrides.
   void setOverriddenDecls(ArrayRef<ValueDecl *> overridden);

   /// Whether the overridden declarations have already been computed.
   bool overriddenDeclsComputed() const;

   /// Is this declaration marked with 'final'?
   bool isFinal() const
   {
      return getAttrs().hasAttribute<FinalAttr>();
   }

   /// Is this declaration marked with 'dynamic'?
   bool isDynamic() const;

   /// Set whether this type is 'dynamic' or not.
   void setIsDynamic(bool value);

   /// Whether the 'dynamic' bit has been computed already.
   bool isDynamicComputed() const
   {
      return m_lazySemanticInfo.isDynamicComputed;
   }

   /// Returns true if this decl can be found by id-style dynamic lookup.
   bool canBeAccessedByDynamicLookup() const;

   /// Returns the protocol requirements that this decl conforms to.
   ArrayRef<ValueDecl *>
   getSatisfiedProtocolRequirements(bool sorted = false) const;

   /// Determines the kind of access that should be performed by a
   /// DeclRefExpr or MemberRefExpr use of this value in the specified
   /// context.
   ///
   /// \param DC The declaration context.
   ///
   /// \param isAccessOnSelf Whether this is a member access on the implicit
   ///        'self' declaration of the declaration context.
   AccessSemantics getAccessSemanticsFromContext(const DeclContext *declContext,
                                                 bool isAccessOnSelf) const;

   /// Print a reference to the given declaration.
   std::string printRef() const;

   /// Dump a reference to the given declaration.
   void dumpRef(RawOutStream &outStream) const;

   /// Dump a reference to the given declaration.
   void dumpRef() const;

   /// Returns true if the declaration is a static member of a type.
   ///
   /// This is not necessarily the opposite of "isInstanceMember()". Both
   /// predicates will be false for declarations that either categorically
   /// can't be "static" or are in a context where "static" doesn't make sense.
   bool isStatic() const;

   /// Retrieve the location at which we should insert a new attribute or
   /// modifier.
   SourceLoc getAttributeInsertionLoc(bool forModifier) const;

   static bool classof(const Decl *decl)
   {
      return decl->getKind() >= DeclKind::First_ValueDecl &&
            decl->getKind() <= DeclKind::Last_ValueDecl;
   }

   /// True if this is a C function that was imported as a member of a type in
   /// Swift.
   bool isImportAsMember() const;
};

/// AbstractStorageDecl - This is the common superclass for VarDecl and
/// SubscriptDecl, representing potentially settable memory locations.
class AbstractStorageDecl : public ValueDecl
{
   friend class SetterAccessLevelRequest;
public:
   static const size_t MaxNumAccessors = 255;
private:
   /// A record of the accessors for the declaration.
   //   class alignas(1 << 3) AccessorRecord final :
   class AccessorRecord final :
         private TrailingObjects<AccessorRecord, AccessorDecl*>
   {
      friend class TrailingObjects;

      using AccessorIndex = uint8_t;
      static const AccessorIndex InvalidIndex = 0;

      /// The range of the braces around the accessor clause.
      SourceRange m_braces;

      /// The implementation info for the accessors.  If there's no
      /// AccessorRecord for a storage decl, the decl is just stored.
//      StorageImplInfo m_implInfo;

      /// The number of accessors currently stored in this record.
      AccessorIndex m_numAccessors;

      /// The storage capacity of this record for accessors.  Always includes
      /// enough space for adding opaque accessors to the record, which are the
      /// only accessors that should ever be added retroactively; hence this
      /// field is only here for the purposes of safety checks.
      AccessorIndex m_accessorsCapacity;

      /// Either 0, meaning there is no registered accessor of the given kind,
      /// or the index+1 of the accessor in the accessors array.
//      AccessorIndex m_accessorIndices[NumAccessorKinds];

//      AccessorRecord(SourceRange braces, StorageImplInfo implInfo,
//                     ArrayRef<AccessorDecl*> accessors,
//                     AccessorIndex accessorsCapacity);
//   public:
//      static AccessorRecord *create(AstContext &ctx, SourceRange braces,
//                                    StorageImplInfo implInfo,
//                                    ArrayRef<AccessorDecl*> accessors);

//      SourceRange getBracesRange() const
//      {
//         return m_braces;
//      }

//      const StorageImplInfo &getImplInfo() const
//      {
//         return m_implInfo;
//      }

//      void overwriteImplInfo(StorageImplInfo newInfo)
//      {
//         m_implInfo = newInfo;
//      }

//      inline AccessorDecl *getAccessor(AccessorKind kind) const;

      ArrayRef<AccessorDecl *> getAllAccessors() const
      {
         return { getTrailingObjects<AccessorDecl*>(), m_numAccessors };
      }

      void addOpaqueAccessor(AccessorDecl *accessor);

   private:
      MutableArrayRef<AccessorDecl *> getAccessorsBuffer()
      {
         return { getTrailingObjects<AccessorDecl*>(), m_numAccessors };
      }

      bool registerAccessor(AccessorDecl *accessor, AccessorIndex index);
   };

   PointerIntPair<AccessorRecord*, 3, OptionalEnum<AccessLevel>> m_accessors;

//   void setFieldsFromImplInfo(StorageImplInfo implInfo)
//   {
//      m_bits.AbstractStorageDecl.HasStorage = implInfo.hasStorage();
//      m_bits.AbstractStorageDecl.SupportsMutation = implInfo.supportsMutation();
//   }

protected:
   AbstractStorageDecl(DeclKind kind, DeclContext *DC, DeclName Name,
                       SourceLoc nameLoc)
      : ValueDecl(kind, DC, Name, nameLoc)
   {
      m_bits.AbstractStorageDecl.HasStorage = true;
//      m_bits.AbstractStorageDecl.SupportsMutation = supportsMutation;
      m_bits.AbstractStorageDecl.IsGetterMutating = false;
      m_bits.AbstractStorageDecl.IsSetterMutating = true;
//      m_bits.AbstractStorageDecl.OpaqueReadOwnership =
//            unsigned(OpaqueReadOwnership::Owned);
   }

   void computeIsValidKeyPathComponent();

public:

   /// Should this declaration be treated as if annotated with transparent
   /// attribute.
   bool isTransparent() const;

   /// Determine whether this storage is a static member, if it
   /// is a member.  Currently only variables can be static.
   inline bool isStatic() const; // defined in this header

   /// Return the interface type of the stored value.
   Type getValueInterfaceType() const;

//   /// Determine how this storage is implemented.
//   StorageImplInfo getImplInfo() const
//   {
//      if (auto ptr = m_accessors.getPointer())
//         return ptr->getImplInfo();
//      return StorageImplInfo::getSimpleStored(supportsMutation());
//   }

//   ReadImplKind getReadImpl() const
//   {
//      return getImplInfo().getReadImpl();
//   }

//   WriteImplKind getWriteImpl() const
//   {
//      return getImplInfo().getWriteImpl();
//   }

//   ReadWriteImplKind getReadWriteImpl() const
//   {
//      return getImplInfo().getReadWriteImpl();
//   }

//   /// Overwrite the registered implementation-info.  This should be
//   /// used carefully.
//   void overwriteImplInfo(StorageImplInfo implInfo);

//   /// Return true if this is a VarDecl that has storage associated with
//   /// it.
//   bool hasStorage() const {
//      return m_bits.AbstractStorageDecl.HasStorage;
//   }

   /// Return true if this storage has the basic accessors/capability
   /// to be mutated.  This is generally constant after the accessors are
   /// installed by the parser/importer/whatever.
   ///
   /// Note that this is different from the mutability of the declaration
   /// in the user language: sometimes we can assign to declarations that
   /// don't support mutation (e.g. to initialize them), and sometimes we
   /// can't mutate things that do support mutation (e.g. because their
   /// setter is private).
//   StorageIsMutable_t supportsMutation() const {
//      return StorageIsMutable_t(m_bits.AbstractStorageDecl.SupportsMutation);
//   }

//   /// Are there any accessors for this declaration, including implicit ones?
//   bool hasAnyAccessors() const {
//      return !getAllAccessors().empty();
//   }

//   /// Return the ownership of values opaquely read from this storage.
//   OpaqueReadOwnership getOpaqueReadOwnership() const {
//      return OpaqueReadOwnership(m_bits.AbstractStorageDecl.OpaqueReadOwnership);
//   }
//   void setOpaqueReadOwnership(OpaqueReadOwnership ownership) {
//      m_bits.AbstractStorageDecl.OpaqueReadOwnership = unsigned(ownership);
//   }

   /// Return true if reading this storage requires the ability to
   /// modify the base value.
   bool isGetterMutating() const {
      return m_bits.AbstractStorageDecl.IsGetterMutating;
   }
   void setIsGetterMutating(bool isMutating) {
      m_bits.AbstractStorageDecl.IsGetterMutating = isMutating;
   }

   /// Return true if modifying this storage requires the ability to
   /// modify the base value.
   bool isSetterMutating() const {
      return m_bits.AbstractStorageDecl.IsSetterMutating;
   }
   void setIsSetterMutating(bool isMutating) {
      m_bits.AbstractStorageDecl.IsSetterMutating = isMutating;
   }

//   AccessorDecl *getAccessor(AccessorKind kind) const {
//      if (auto info = m_accessors.getPointer())
//         return info->getAccessor(kind);
//      return nullptr;
//   }

//   ArrayRef<AccessorDecl*> getAllAccessors() const {
//      if (const auto *info = m_accessors.getPointer())
//         return info->getAllAccessors();
//      return {};
//   }

   /// Visit all the opaque accessors that this storage is expected to have.
//   void visitExpectedOpaqueAccessors(
//         llvm::function_ref<void (AccessorKind)>) const;

//   /// Visit all the opaque accessors of this storage declaration.
//   void visitOpaqueAccessors(llvm::function_ref<void (AccessorDecl*)>) const;

//   void setAccessors(StorageImplInfo storageImpl,
//                     SourceLoc lbraceLoc, ArrayRef<AccessorDecl*> accessors,
//                     SourceLoc rbraceLoc);

   /// Add a setter to an existing Computed var.
   ///
   /// This should only be used by the ClangImporter.
   void setComputedSetter(AccessorDecl *Set);

   /// Add a synthesized getter.
   void setSynthesizedGetter(AccessorDecl *getter);

   /// Add a synthesized setter.
   void setSynthesizedSetter(AccessorDecl *setter);

   /// Add a synthesized read coroutine.
   void setSynthesizedReadCoroutine(AccessorDecl *read);

   /// Add a synthesized modify coroutine.
   void setSynthesizedModifyCoroutine(AccessorDecl *modify);

   /// Does this storage require an opaque accessor of the given kind?
//   bool requiresOpaqueAccessor(AccessorKind kind) const;

//   /// Does this storage require a 'get' accessor in its opaque-accessors set?
//   bool requiresOpaqueGetter() const {
//      return getOpaqueReadOwnership() != OpaqueReadOwnership::Borrowed;
//   }

//   /// Does this storage require a 'read' accessor in its opaque-accessors set?
//   bool requiresOpaqueReadCoroutine() const {
//      return getOpaqueReadOwnership() != OpaqueReadOwnership::Owned;
//   }

//   /// Does this storage require a 'set' accessor in its opaque-accessors set?
//   bool requiresOpaqueSetter() const { return supportsMutation(); }

//   /// Does this storage require a 'modify' accessor in its opaque-accessors set?
//   bool requiresOpaqueModifyCoroutine() const;

//   SourceRange getBracesRange() const {
//      if (auto info = m_accessors.getPointer())
//         return info->getBracesRange();
//      return SourceRange();
//   }

//   /// Retrieve the getter used to access the value of this variable.
//   AccessorDecl *getGetter() const {
//      return getAccessor(AccessorKind::Get);
//   }

//   /// Retrieve the setter used to mutate the value of this variable.
//   AccessorDecl *getSetter() const {
//      return getAccessor(AccessorKind::Set);
//   }

//   AccessLevel getSetterFormalAccess() const;

//   void setSetterAccess(AccessLevel accessLevel) {
//      assert(!m_accessors.getInt().hasValue());
//      overwriteSetterAccess(accessLevel);
//   }

//   void overwriteSetterAccess(AccessLevel accessLevel);

//   /// Return the decl for the immutable addressor if it exists.
//   AccessorDecl *getAddressor() const {
//      return getAccessor(AccessorKind::Address);
//   }

//   /// Return the decl for the mutable accessor if it exists.
//   AccessorDecl *getMutableAddressor() const {
//      return getAccessor(AccessorKind::MutableAddress);
//   }

//   /// Return the appropriate addressor for the given access kind.
//   AccessorDecl *getAddressorForAccess(AccessKind accessKind) const {
//      if (accessKind == AccessKind::Read)
//         return getAddressor();
//      return getMutableAddressor();
//   }

//   /// Return the decl for the 'read' coroutine accessor if it exists.
//   AccessorDecl *getReadCoroutine() const {
//      return getAccessor(AccessorKind::Read);
//   }

//   /// Return the decl for the 'modify' coroutine accessor if it exists.
//   AccessorDecl *getModifyCoroutine() const {
//      return getAccessor(AccessorKind::Modify);
//   }

//   /// Return the decl for the willSet specifier if it exists, this is
//   /// only valid on a declaration with Observing storage.
//   AccessorDecl *getWillSetFunc() const {
//      return getAccessor(AccessorKind::WillSet);
//   }

//   /// Return the decl for the didSet specifier if it exists, this is
//   /// only valid on a declaration with Observing storage.
//   AccessorDecl *getDidSetFunc() const {
//      return getAccessor(AccessorKind::DidSet);
//   }

//   /// Given that this is an Objective-C property or subscript declaration,
//   /// produce its getter selector.
//   ObjCSelector
//   getObjCGetterSelector(Identifier preferredName = Identifier()) const;

//   /// Given that this is an Objective-C property or subscript declaration,
//   /// produce its setter selector.
//   ObjCSelector
//   getObjCSetterSelector(Identifier preferredName = Identifier()) const;

//   AbstractStorageDecl *getOverriddenDecl() const {
//      return cast_or_null<AbstractStorageDecl>(ValueDecl::getOverriddenDecl());
//   }

//   /// Returns the location of 'override' keyword, if any.
//   SourceLoc getOverrideLoc() const;

//   /// Returns true if this declaration has a setter accessible from the given
//   /// context.
//   ///
//   /// If \p DC is null, returns true only if the setter is public.
//   ///
//   /// See \c isAccessibleFrom for a discussion of the \p forConformance
//   /// parameter.
//   bool isSetterAccessibleFrom(const DeclContext *DC,
//                               bool forConformance=false) const;

//   /// Determine how this storage declaration should actually be accessed.
//   AccessStrategy getAccessStrategy(AccessSemantics semantics,
//                                    AccessKind accessKind,
//                                    ModuleDecl *module,
//                                    ResilienceExpansion expansion) const;

//   /// Should this declaration behave as if it must be accessed
//   /// resiliently, even when we're building a non-resilient module?
//   ///
//   /// This is used for diagnostics, because we do not want a behavior
//   /// change between builds with resilience enabled and disabled.
//   bool isFormallyResilient() const;

//   /// Do we need to use resilient access patterns outside of this
//   /// property's resilience domain?
//   bool isResilient() const;

//   /// Do we need to use resilient access patterns when accessing this
//   /// property from the given module?
//   bool isResilient(ModuleDecl *M, ResilienceExpansion expansion) const;

//   void setIsValidKeyPathComponent(bool value) {
//      m_bits.AbstractStorageDecl.HasComputedValidKeyPathComponent = true;
//      m_bits.AbstractStorageDecl.ValidKeyPathComponent = value;
//   }

//   /// True if the storage can be referenced by a keypath directly.
//   /// Otherwise, its override must be referenced.
//   bool isValidKeyPathComponent() const {
//      if (!m_bits.AbstractStorageDecl.HasComputedValidKeyPathComponent)
//         const_cast<AbstractStorageDecl *>(this)->computeIsValidKeyPathComponent();
//      return m_bits.AbstractStorageDecl.ValidKeyPathComponent;
//   }

//   /// True if the storage exports a property descriptor for key paths in
//   /// other modules.
//   bool exportsPropertyDescriptor() const;

//   // Implement isa/cast/dyncast/etc.
//   static bool classof(const Decl *D) {
//      return D->getKind() >= DeclKind::First_AbstractStorageDecl &&
//            D->getKind() <= DeclKind::Last_AbstractStorageDecl;
//   }
};

/// VarDecl - 'var' and 'let' declarations.
class VarDecl : public AbstractStorageDecl
{
public:
   enum class Specifier : uint8_t {
      // For Var Decls

      Let = 0,
      Var = 1,

      // For Param Decls

      Default = Let,
      InOut = 2,
      Shared = 3,
      Owned = 4,
   };

protected:

   VarDecl(DeclKind kind, bool IsStatic, Specifier Sp, bool IsCaptureList,
           SourceLoc nameLoc, Identifier Name, DeclContext *DC)
      : AbstractStorageDecl(kind, DC, Name, nameLoc)
   {
      m_bits.VarDecl.IsStatic = IsStatic;
      m_bits.VarDecl.Specifier = static_cast<unsigned>(Sp);
      m_bits.VarDecl.IsCaptureList = IsCaptureList;
      m_bits.VarDecl.IsDebuggerVar = false;
      m_bits.VarDecl.IsREPLVar = false;
      m_bits.VarDecl.HasNonPatternBindingInit = false;
   }

   /// This is the type specified, including location information.
   TypeLoc typeLoc;

   Type typeInContext;

public:
   VarDecl(bool IsStatic, Specifier Sp, bool IsCaptureList, SourceLoc nameLoc,
           Identifier Name, DeclContext *DC)
      : VarDecl(DeclKind::Var, IsStatic, Sp, IsCaptureList, nameLoc, Name, DC) {}

   SourceRange getSourceRange() const;

   Identifier getName() const { return getFullName().getBaseIdentifier(); }

   /// Returns the string for the base name, or "_" if this is unnamed.
   StringRef getNameStr() const {
      assert(!getFullName().isSpecial() && "Cannot get string for special names");
      return hasName() ? getBaseName().getIdentifier().str() : "_";
   }

   TypeLoc &getTypeLoc() { return typeLoc; }
   TypeLoc getTypeLoc() const { return typeLoc; }

   bool hasType() const {
      // We have a type if either the type has been computed already or if
      // this is a deserialized declaration with an interface type.
      return !typeInContext.isNull();
   }

   /// Get the type of the variable within its context. If the context is generic,
   /// this will use archetypes.
   Type getType() const;

   /// Set the type of the variable within its context.
   void setType(Type t);

   void markInvalid();

   /// Retrieve the source range of the variable type, or an invalid range if the
   /// variable's type is not explicitly written in the source.
   ///
   /// Only for use in diagnostics.  It is not always possible to always
   /// precisely point to the variable type because of type aliases.
   SourceRange getTypeSourceRangeForDiagnostics() const;

   /// Returns whether the var is settable in the specified context: this
   /// is either because it is a stored var, because it has a custom setter, or
   /// is a let member in an initializer.
   ///
   /// Pass a null context and null base to check if it's always settable.
   bool isSettable(const DeclContext *UseDC,
                   const DeclRefExpr *base = nullptr) const;

   /// Return the parent pattern binding that may provide an initializer for this
   /// VarDecl.  This returns null if there is none associated with the VarDecl.
//   PatternBindingDecl *getParentPatternBinding() const {
//      return ParentPattern.dyn_cast<PatternBindingDecl *>();
//   }
//   void setParentPatternBinding(PatternBindingDecl *PBD) {
//      ParentPattern = PBD;
//   }

   /// Return the Pattern involved in initializing this VarDecl.  However, recall
   /// that the Pattern may be involved in initializing more than just this one
   /// vardecl.  For example, if this is a VarDecl for "x", the pattern may be
   /// "(x, y)" and the initializer on the PatternBindingDecl may be "(1,2)" or
   /// "foo()".
   ///
   /// If this has no parent pattern binding decl or statement associated, it
   /// returns null.
   ///
//   Pattern *getParentPattern() const;

   /// Return the statement that owns the pattern associated with this VarDecl,
   /// if one exists.
//   Stmt *getParentPatternStmt() const {
//      return ParentPattern.dyn_cast<Stmt*>();
//   }
//   void setParentPatternStmt(Stmt *S) {
//      ParentPattern = S;
//   }

   /// True if the global stored property requires lazy initialization.
   bool isLazilyInitializedGlobal() const;

   /// Return the initializer involved in this VarDecl.  Recall that the
   /// initializer may be involved in initializing more than just this one
   /// vardecl though.  For example, if this is a VarDecl for "x", the pattern
   /// may be "(x, y)" and the initializer on the PatternBindingDecl may be
   /// "(1,2)" or "foo()".
   ///
   /// If this has no parent pattern binding decl associated, or if that pattern
   /// binding has no initial value, this returns null.
   ///
//   Expr *getParentInitializer() const {
//      if (auto *PBD = getParentPatternBinding())
//         return PBD->getPatternEntryForVarDecl(this).getInit();
//      return nullptr;
//   }

   // Return whether this VarDecl has an initial value, either by checking
   // if it has an initializer in its parent pattern binding or if it has
   // the @_hasInitialValue attribute.
//   bool hasInitialValue() const {
//      return getAttrs().hasAttribute<HasInitialValueAttr>() ||
//            getParentInitializer();
//   }

//   VarDecl *getOverriddenDecl() const {
//      return cast_or_null<VarDecl>(AbstractStorageDecl::getOverriddenDecl());
//   }

   /// Determine whether this declaration is an anonymous closure parameter.
   bool isAnonClosureParam() const;

   /// Return the raw specifier value for this property or parameter.
   Specifier getSpecifier() const {
      return static_cast<Specifier>(m_bits.VarDecl.Specifier);
   }
   void setSpecifier(Specifier Spec);

   /// Is the type of this parameter 'inout'?
   ///
   /// FIXME(Remove InOut): This is only valid on ParamDecls but multiple parts
   /// of the compiler check ParamDecls and VarDecls along the same paths.
   bool isInOut() const {
      // FIXME: Re-enable this assertion and fix callers.
      //    assert((getKind() == DeclKind::Param) && "querying 'inout' on var decl?");
      return getSpecifier() == Specifier::InOut;
   }


   /// Is this a type ('static') variable?
   bool isStatic() const { return m_bits.VarDecl.IsStatic; }
   void setStatic(bool IsStatic) { m_bits.VarDecl.IsStatic = IsStatic; }

   /// \returns the way 'static'/'class' should be spelled for this declaration.
   StaticSpellingKind getCorrectStaticSpelling() const;

//   bool isImmutable() const
//   {
//      return isImmutableSpecifier(getSpecifier());
//   }
//   static bool isImmutableSpecifier(Specifier sp) {
//      switch (sp) {
//      case Specifier::Let:
//      case Specifier::Shared:
//      case Specifier::Owned:
//         return true;
//      case Specifier::Var:
//      case Specifier::InOut:
//         return false;
//      }
//      llvm_unreachable("unhandled specifier");
//   }
   /// Is this an immutable 'let' property?
   bool isLet() const { return getSpecifier() == Specifier::Let; }
   /// Is this an immutable 'shared' property?
   bool isShared() const { return getSpecifier() == Specifier::Shared; }
   /// Is this an immutable 'owned' property?
   bool isOwned() const { return getSpecifier() == Specifier::Owned; }

//   ValueOwnership getValueOwnership() const {
//      return getValueOwnershipForSpecifier(getSpecifier());
//   }

//   static ValueOwnership getValueOwnershipForSpecifier(Specifier specifier) {
//      switch (specifier) {
//      case Specifier::Let:
//         return ValueOwnership::Default;
//      case Specifier::Var:
//         return ValueOwnership::Default;
//      case Specifier::InOut:
//         return ValueOwnership::InOut;
//      case Specifier::Shared:
//         return ValueOwnership::Shared;
//      case Specifier::Owned:
//         return ValueOwnership::Owned;
//      }
//      llvm_unreachable("unhandled specifier");
//   }

//   static Specifier
//   getParameterSpecifierForValueOwnership(ValueOwnership ownership) {
//      switch (ownership) {
//      case ValueOwnership::Default:
//         return Specifier::Let;
//      case ValueOwnership::Shared:
//         return Specifier::Shared;
//      case ValueOwnership::InOut:
//         return Specifier::InOut;
//      case ValueOwnership::Owned:
//         return Specifier::Owned;
//      }
//      llvm_unreachable("unhandled ownership");
//   }

   /// Is this an element in a capture list?
   bool isCaptureList() const { return m_bits.VarDecl.IsCaptureList; }

   /// Return true if this vardecl has an initial value bound to it in a way
   /// that isn't represented in the AST with an initializer in the pattern
   /// binding.  This happens in cases like "for i in ...", switch cases, etc.
   bool hasNonPatternBindingInit() const {
      return m_bits.VarDecl.HasNonPatternBindingInit;
   }
   void setHasNonPatternBindingInit(bool V = true) {
      m_bits.VarDecl.HasNonPatternBindingInit = V;
   }

   /// Determines if this var has an initializer expression that should be
   /// exposed to clients.
   /// There's a very narrow case when we would: if the decl is an instance
   /// member with an initializer expression and the parent type is
   /// @_fixed_layout and resides in a resilient module.
   bool isInitExposedToClients() const;

   /// Is this a special debugger variable?
   bool isDebuggerVar() const { return m_bits.VarDecl.IsDebuggerVar; }
   void setDebuggerVar(bool IsDebuggerVar) {
      m_bits.VarDecl.IsDebuggerVar = IsDebuggerVar;
   }

   /// Return the Objective-C runtime name for this property.
//   Identifier getObjCPropertyName() const;

//   /// Retrieve the default Objective-C selector for the getter of a
//   /// property of the given name.
//   static ObjCSelector getDefaultObjCGetterSelector(AstContext &ctx,
//                                                    Identifier propertyName);

//   /// Retrieve the default Objective-C selector for the setter of a
//   /// property of the given name.
//   static ObjCSelector getDefaultObjCSetterSelector(AstContext &ctx,
//                                                    Identifier propertyName);

   /// If this is a simple 'let' constant, emit a note with a fixit indicating
   /// that it can be rewritten to a 'var'.  This is used in situations where the
   /// compiler detects obvious attempts to mutate a constant.
   void emitLetToVarNoteIfSimple(DeclContext *UseDC) const;

   /// Returns true if the name is the self identifier and is implicit.
   bool isSelfParameter() const;

   // Implement isa/cast/dyncast/etc.
   static bool classof(const Decl *D)
   {
      return D->getKind() == DeclKind::Var || D->getKind() == DeclKind::Param;
   }
};

/// A function parameter declaration.
class ParamDecl : public VarDecl
{
   Identifier m_argumentName;
   SourceLoc m_argumentNameLoc;
   SourceLoc m_specifierLoc;

   struct StoredDefaultArgument
   {
      Expr *defaultArg = nullptr;
      Initializer *initContext = nullptr;
      StringRef stringRepresentation;
   };

   enum class Flags : uint8_t
   {
      /// Whether or not this parameter is vargs.
      IsVariadic = 1 << 0,

      /// Whether or not this parameter is `@autoclosure`.
      IsAutoClosure = 1 << 1,
   };

   /// The default value, if any, along with flags.
   PointerIntPair<StoredDefaultArgument *, 2, OptionSet<Flags>>
   m_defaultValueAndFlags;

public:
   ParamDecl(VarDecl::Specifier specifier,
             SourceLoc specifierLoc, SourceLoc argumentNameLoc,
             Identifier argumentName, SourceLoc parameterNameLoc,
             Identifier parameterName, DeclContext *dc);

   /// Clone constructor, allocates a new ParamDecl identical to the first.
   /// Intentionally not defined as a typical copy constructor to avoid
   /// accidental copies.
   ParamDecl(ParamDecl *PD, bool withTypes);

   /// Retrieve the argument (API) name for this function parameter.
   Identifier getArgumentName() const { return m_argumentName; }

   /// Retrieve the parameter (local) name for this function parameter.
   Identifier getParameterName() const { return getName(); }

   /// Retrieve the source location of the argument (API) name.
   ///
   /// The resulting source location will be valid if the argument name
   /// was specified separately from the parameter name.
   SourceLoc getArgumentNameLoc() const { return m_argumentNameLoc; }

   SourceLoc getSpecifierLoc() const { return m_specifierLoc; }

   bool isTypeLocImplicit() const { return m_bits.ParamDecl.IsTypeLocImplicit; }
   void setIsTypeLocImplicit(bool val) { m_bits.ParamDecl.IsTypeLocImplicit = val; }

   DefaultArgumentKind getDefaultArgumentKind() const {
      return static_cast<DefaultArgumentKind>(m_bits.ParamDecl.defaultArgumentKind);
   }
   bool isDefaultArgument() const {
      return getDefaultArgumentKind() != DefaultArgumentKind::None;
   }
   void setDefaultArgumentKind(DefaultArgumentKind K) {
      m_bits.ParamDecl.defaultArgumentKind = static_cast<unsigned>(K);
   }

   Expr *getDefaultValue() const {
      if (auto stored = m_defaultValueAndFlags.getPointer())
         return stored->defaultArg;
      return nullptr;
   }

   void setDefaultValue(Expr *E);

   Initializer *getDefaultArgumentInitContext() const {
      if (auto stored = m_defaultValueAndFlags.getPointer())
         return stored->initContext;
      return nullptr;
   }

   void setDefaultArgumentInitContext(Initializer *initContext);

   /// Extracts the text of the default argument attached to the provided
   /// ParamDecl, removing all inactive #if clauses and providing only the
   /// text of active #if clauses.
   ///
   /// For example, the default argument:
   /// ```
   /// {
   ///   #if false
   ///   print("false")
   ///   #else
   ///   print("true")
   ///   #endif
   /// }
   /// ```
   /// will return
   /// ```
   /// {
   ///   print("true")
   /// }
   /// ```
   /// \sa getDefaultValue
   StringRef getDefaultValueStringRepresentation(
         SmallVectorImpl<char> &scratch) const;

   void setDefaultValueStringRepresentation(StringRef stringRepresentation);

   /// Whether or not this parameter is varargs.
   bool isVariadic() const {
      return m_defaultValueAndFlags.getInt().contains(Flags::IsVariadic);
   }
   void setVariadic(bool value = true) {
      auto flags = m_defaultValueAndFlags.getInt();
      m_defaultValueAndFlags.setInt(value ? flags | Flags::IsVariadic
                                          : flags - Flags::IsVariadic);
   }

   /// Whether or not this parameter is marked with `@autoclosure`.
   bool isAutoClosure() const {
      return m_defaultValueAndFlags.getInt().contains(Flags::IsAutoClosure);
   }
   void setAutoClosure(bool value = true) {
      auto flags = m_defaultValueAndFlags.getInt();
      m_defaultValueAndFlags.setInt(value ? flags | Flags::IsAutoClosure
                                          : flags - Flags::IsAutoClosure);
   }

   /// Remove the type of this varargs element designator, without the array
   /// type wrapping it.  A parameter like "Int..." will have formal parameter
   /// type of "[Int]" and this returns "Int".
   static Type getVarargBaseTy(Type VarArgT);

   /// Remove the type of this varargs element designator, without the array
   /// type wrapping it.
   Type getVarargBaseTy() const {
      assert(isVariadic());
      return getVarargBaseTy(getInterfaceType());
   }

   SourceRange getSourceRange() const;

   // Implement isa/cast/dyncast/etc.
   static bool classof(const Decl *D) {
      return D->getKind() == DeclKind::Param;
   }
};

/// Describes the kind of subscripting used in Objective-C.
enum class ObjCSubscriptKind
{
   /// Objective-C indexed subscripting, which is based on an integral
   /// index.
   Indexed,
   /// Objective-C keyed subscripting, which is based on an object
   /// argument or metatype thereof.
   Keyed
};


} // polar::ast

#endif // POLARPHP_AST_DECL_H
