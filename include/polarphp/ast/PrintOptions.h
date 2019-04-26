//===--- PrintOptions.h - AST printing options ------------------*- C++ -*-===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.

#ifndef POLARPHP_AST_PRINT_OPTIONS_H
#define POLARPHP_AST_PRINT_OPTIONS_H

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/Identifier.h"

#include <limits.h>
#include <vector>

namespace polar::ast {

class AstPrinter;
class GenericEnvironment;
class CanType;
class Decl;
class Pattern;
class ValueDecl;
class ExtensionDecl;
class NominalTypeDecl;
class TypeBase;
class DeclContext;
class Type;
enum DeclAttrKind : unsigned;
class SynthesizedExtensionAnalyzer;
struct PrintOptions;

using polar::basic::DenseMap;

/// Necessary information for archetype transformation during printing.
struct TypeTransformContext
{
   TypeBase *baseType;
   explicit TypeTransformContext(Type type);
   Type getBaseType() const;
   DeclContext *getDeclContext() const;
};

class BracketOptions
{
   Decl* m_target;
   bool m_openExtension;
   bool m_closeExtension;
   bool m_closeNominal;

public:
   BracketOptions(Decl *target = nullptr, bool openExtension = true,
                  bool closeExtension = true, bool closeNominal = true) :
      m_target(target),
      m_openExtension(openExtension),
      m_closeExtension(closeExtension),
      m_closeNominal(closeNominal)
   {}

   bool shouldOpenExtension(const Decl *decl)
   {
      return decl != m_target || m_openExtension;
   }

   bool shouldCloseExtension(const Decl *decl)
   {
      return decl != m_target || m_closeExtension;
   }

   bool shouldCloseNominal(const Decl *decl)
   {
      return decl != m_target || m_closeNominal;
   }
};


/// A union of DeclAttrKind and TypeAttrKind.
class AnyAttrKind
{
public:
   AnyAttrKind(TypeAttrKind kind)
      : m_kind(static_cast<unsigned>(kind)),
        m_isType(1)
   {
      static_assert(TAK_Count < UINT_MAX, "TypeAttrKind is > 31 bits");
   }

   AnyAttrKind(DeclAttrKind kind)
      : m_kind(static_cast<unsigned>(kind)),
        m_isType(0)
   {
      static_assert(DAK_Count < UINT_MAX, "DeclAttrKind is > 31 bits");
   }

   AnyAttrKind()
      : m_kind(TAK_Count),
        m_isType(1)
   {}

   AnyAttrKind(const AnyAttrKind &) = default;

   /// Returns the TypeAttrKind, or TAK_Count if this is not a type attribute.
   TypeAttrKind type() const
   {
      return m_isType ? static_cast<TypeAttrKind>(m_kind) : TAK_Count;
   }

   /// Returns the DeclAttrKind, or DAK_Count if this is not a decl attribute.
   DeclAttrKind decl() const
   {
      return m_isType ? DAK_Count : static_cast<DeclAttrKind>(m_kind);
   }

   bool operator==(AnyAttrKind other) const
   {
      return m_kind == other.m_kind && m_isType == other.m_isType;
   }

   bool operator!=(AnyAttrKind other) const
   {
      return !(*this == other);
   }

private:
   unsigned m_kind : 31;
   unsigned m_isType : 1;
};

struct ShouldPrintChecker
{
   virtual bool shouldPrint(const Decl *decl, const PrintOptions &options);
   virtual ~ShouldPrintChecker() = default;
};

/// Options for printing AST nodes.
///
/// A default-constructed PrintOptions is suitable for printing to users;
/// there are also factory methods for specific use cases.
struct PrintOptions
{
   /// The indentation width.
   unsigned indent = 2;

   /// Whether to print function definitions.
   bool functionDefinitions = false;

   /// Whether to print '{ get set }' on readwrite computed properties.
   bool printGetSetOnRWProperties = true;

   /// Whether to print *any* accessors on properties.
   bool printPropertyAccessors = true;

   /// Whether to print the accessors of a property abstractly,
   /// i.e. always as:
   /// ```
   /// var x: Int { get set }
   /// ```
   /// rather than the specific accessors actually used to implement the
   /// property.
   ///
   /// Printing function definitions takes priority over this setting.
   bool abstractAccessors = true;

   /// Whether to print a property with only a single getter using the shorthand
   /// ```
   /// var x: Int { return y }
   /// ```
   /// vs.
   /// ```
   /// var x: Int {
   ///   get { return y }
   /// }
   /// ```
   bool collapseSingleGetterProperty = true;

   /// Whether to print the bodies of accessors in protocol context.
   bool printAccessorBodiesInProtocols = false;

   /// Whether to print type definitions.
   bool typeDefinitions = false;

   /// Whether to print variable initializers.
   bool varInitializers = false;

   /// Whether to print enum raw value expressions.
   bool enumRawValues = false;

   /// Whether to prefer printing TypeReprs instead of Types,
   /// if a TypeRepr is available.  This allows us to print the original
   /// spelling of the type name.
   ///
   /// \note This should be \c true when printing AST with the intention show
   /// it to the user.
   bool preferTypeRepr = true;

   /// Whether to print fully qualified Types.
   bool fullyQualifiedTypes = false;

   /// Print fully qualified types if our heuristics say that a certain
   /// type might be ambiguous.
   bool fullyQualifiedTypesIfAmbiguous = false;

   /// Print Swift.Array and Swift.Optional with sugared syntax
   /// ([] and ?), even if there are no sugar type nodes.
   bool synthesizeSugarOnTypes = false;

   /// If true, null types in the AST will be printed as "<null>". If
   /// false, the compiler will trap.
   bool allowNullTypes = true;

   /// If true, the printer will explode a pattern like this:
   /// \code
   ///   var (a, b) = f()
   /// \endcode
   /// into multiple variable declarations.
   ///
   /// For this option to work correctly, \c VarInitializers should be
   /// \c false.
   bool explodePatternBindingDecls = false;

   /// If true, the printer will explode an enum case like this:
   /// \code
   ///   case A, B
   /// \endcode
   /// into multiple case declarations.
   bool explodeEnumCaseDecls = false;

   /// Whether to print implicit parts of the AST.
   bool skipImplicit = false;

   /// Whether to print unavailable parts of the AST.
   bool skipUnavailable = false;

   /// Whether to skip internal stdlib declarations.
   bool SkipPrivateStdlibDecls = false;

   /// Whether to skip underscored stdlib protocols.
   /// Protocols marked with @_show_in_interface are still printed.
   bool skipUnderscoredStdlibProtocols = false;

   /// Whether to skip extensions that don't add protocols or no members.
   bool skipEmptyExtensionDecls = true;

   /// Whether to print attributes.
   bool skipAttributes = false;

   /// Whether to print keywords like 'func'.
   bool skipIntroducerKeywords = false;

   /// Whether to print destructors.
   bool skipDeinit = false;

   /// Whether to skip printing 'import' declarations.
   bool skipImports = false;

   /// Whether to skip printing overrides and witnesses for
   /// protocol requirements.
   bool skipOverrides = false;

   /// Whether to skip placeholder members.
   bool skipMissingMemberPlaceholders = true;

   /// Whether to print a long attribute like '\@available' on a separate line
   /// from the declaration or other attributes.
   bool printLongAttrsOnSeparateLines = false;

   bool printImplicitAttrs = true;

   /// Whether to skip keywords with a prefix of underscore such as __consuming.
   bool skipUnderscoredKeywords = false;

   /// Whether to print decl attributes that are only used internally,
   /// such as _silgen_name, transparent, etc.
   bool printUserInaccessibleAttrs = true;

   /// List of attribute kinds that should not be printed.
   std::vector<AnyAttrKind> excludeAttrList = {DAK_Transparent, DAK_Effects,
                                               DAK_FixedLayout,
                                               DAK_ShowInInterface,
                                               DAK_ImplicitlyUnwrappedOptional};

   /// List of attribute kinds that should be printed exclusively.
   /// Empty means allow all.
   std::vector<AnyAttrKind> exclusiveAttrList;

   /// Whether to print function @convention attribute on function types.
   bool printFunctionRepresentationAttrs = true;

   /// Whether to print storage representation attributes on types, e.g.
   /// '@sil_weak', '@sil_unmanaged'.
   bool printStorageRepresentationAttrs = false;

   /// Whether to print 'override' keyword on overridden decls.
   bool printOverrideKeyword = true;

   /// Whether to print access control information on all value decls.
   bool printAccess = false;

   /// If \c PrintAccess is true, this determines whether to print
   /// 'internal' keyword.
   bool printInternalAccessKeyword = true;

   /// Print all decls that have at least this level of access.
   AccessLevel accessFilter = AccessLevel::Private;

   /// Print IfConfigDecls.
   bool printIfConfig = true;

   /// Whether to use an empty line to separate two members in a single decl.
   bool emptyLineBetweenMembers = false;

   /// Whether to print the extensions from conforming protocols.
   bool printExtensionFromConformingProtocols = false;

   std::shared_ptr<ShouldPrintChecker> currentPrintabilityChecker =
         std::make_shared<ShouldPrintChecker>();

   enum class ArgAndParamPrintingMode
   {
      ArgumentOnly,
      MatchSource,
      BothAlways,
      EnumElement,
   };

   /// Whether to print the doc-comment from the conformance if a member decl
   /// has no associated doc-comment by itself.
   bool elevateDocCommentFromConformance = false;

   /// Whether to print the content of an extension decl inside the type decl where it
   /// extends from.
   std::function<bool(const ExtensionDecl *)> printExtensionContentAsMembers =
         [] (const ExtensionDecl *) { return false; };

   /// How to print the keyword argument and parameter name in functions.
   ArgAndParamPrintingMode ArgAndParamPrinting =
         ArgAndParamPrintingMode::MatchSource;

   /// Whether to print documentation comments attached to declarations.
   /// Note that this may print documentation comments from related declarations
   /// (e.g. the overridden method in the superclass) if such comment is found.
   bool printDocumentationComments = false;

   /// When true, printing interface from a source file will print the original
   /// source text for applicable declarations, in order to preserve the
   /// formatting.
   bool printOriginalSourceText = false;

   /// When printing a type alias type, whether print the underlying type instead
   /// of the alias.
   bool printTypeAliasUnderlyingType = false;

   /// When printing an Optional<T>, rather than printing 'T?', print
   /// 'T!'. Used as a modifier only when we know we're printing
   /// something that was declared as an implicitly unwrapped optional
   /// at the top level. This is stripped out of the printing options
   /// for optionals that are nested within other optionals.
   bool printOptionalAsImplicitlyUnwrapped = false;

   /// Replaces the name of private and internal properties of types with '_'.
   bool omitNameOfInaccessibleProperties = false;

   /// Print dependent types as references into this generic environment.
   GenericEnvironment *genericEnv = nullptr;

   /// Print types with alternative names from their canonical names.
   DenseMap<CanType, Identifier> *alternativeTypeNames = nullptr;

   /// The information for converting archetypes to specialized types.
   std::optional<TypeTransformContext> transformContext;

   bool PrintAsMember = false;

   /// Whether to print parameter specifiers as 'let' and 'var'.
   bool printParameterSpecifiers = false;

   /// \see ShouldQualifyNestedDeclarations
   enum class QualifyNestedDeclarations
   {
      Never,
      TypesOnly,
      Always
   };

   /// Controls when a nested declaration's name should be printed qualified with
   /// its enclosing context, if it's being printed on its own (rather than as
   /// part of the context).
   QualifyNestedDeclarations shouldQualifyNestedDeclarations =
         QualifyNestedDeclarations::Never;

   /// If this is not \c nullptr then function bodies (including accessors
   /// and constructors) will be printed by this function.
   std::function<void(const ValueDecl *, AstPrinter &)> functionBody;

   BracketOptions bracketOptions;

   // This is explicit to guarantee that it can be called from LLDB.
   PrintOptions()
   {}

   bool excludeAttrKind(AnyAttrKind kind) const
   {
      if (std::any_of(excludeAttrList.begin(), excludeAttrList.end(),
                      [kind](AnyAttrKind other) { return other == kind; })) {
         return true;
      }
      if (!exclusiveAttrList.empty()) {
         return std::none_of(exclusiveAttrList.begin(), exclusiveAttrList.end(),
                             [kind](AnyAttrKind other) { return other == kind; });
      }
      return false;
   }

   /// Retrieve the set of options for verbose printing to users.
   static PrintOptions printVerbose()
   {
      PrintOptions result;
      result.typeDefinitions = true;
      result.varInitializers = true;
      result.printDocumentationComments = true;
      result.printLongAttrsOnSeparateLines = true;
      return result;
   }

   /// Retrieve the set of options suitable for diagnostics printing.
   static PrintOptions printForDiagnostics()
   {
      PrintOptions result = printVerbose();
      result.printAccess = true;
      result.indent = 4;
      result.fullyQualifiedTypesIfAmbiguous = true;
      result.synthesizeSugarOnTypes = true;
      result.printUserInaccessibleAttrs = false;
      result.printImplicitAttrs = false;
      result.excludeAttrList.push_back(DAK_Exported);
      result.excludeAttrList.push_back(DAK_Inline);
      result.excludeAttrList.push_back(DAK_Optimize);
      result.excludeAttrList.push_back(DAK_Rethrows);
      result.printOverrideKeyword = false;
      result.accessFilter = AccessLevel::Public;
      result.printIfConfig = false;
      result.shouldQualifyNestedDeclarations =
            QualifyNestedDeclarations::TypesOnly;
      result.printDocumentationComments = false;
      return result;
   }

   /// Retrieve the set of options suitable for interface generation.
   static PrintOptions printInterface()
   {
      PrintOptions result = printForDiagnostics();
      result.skipUnavailable = true;
      result.skipImplicit = true;
      result.skipUnderscoredStdlibProtocols = true;
      result.skipDeinit = true;
      result.excludeAttrList.push_back(DAK_DiscardableResult);
      result.emptyLineBetweenMembers = true;
      result.elevateDocCommentFromConformance = true;
      result.shouldQualifyNestedDeclarations =
            QualifyNestedDeclarations::Always;
      result.printDocumentationComments = true;
      return result;
   }

   /// Retrieve the set of options suitable for parseable module interfaces.
   ///
   /// This is a format that will be parsed again later, so the output must be
   /// consistent and well-formed.
   ///
   /// \see swift::emitParseableInterface
   static PrintOptions printParseableInterfaceFile();
   static PrintOptions printModuleInterface();
   static PrintOptions printTypeInterface(Type T);

   void setBaseType(Type type);

   void clearSynthesizedExtension();

   bool shouldPrint(const Decl* D)
   {
      return currentPrintabilityChecker->shouldPrint(D, *this);
   }

   /// Retrieve the print options that are suitable to print the testable interface.
   static PrintOptions printTestableInterface()
   {
      PrintOptions result = printInterface();
      result.accessFilter = AccessLevel::Internal;
      return result;
   }

   /// Retrieve the print options that are suitable to print interface for a
   /// swift file.
   static PrintOptions printPolarFileInterface()
   {
      PrintOptions result = printInterface();
      result.accessFilter = AccessLevel::Internal;
      result.emptyLineBetweenMembers = true;
      return result;
   }

   /// Retrieve the set of options suitable for interface generation for
   /// documentation purposes.
   static PrintOptions printDocInterface();

   /// Retrieve the set of options that prints everything.
   ///
   /// This is only intended for debug output.
   static PrintOptions printEverything()
   {
      PrintOptions result = printVerbose();
      result.excludeAttrList.clear();
      result.excludeAttrList.push_back(DAK_FixedLayout);
      result.printStorageRepresentationAttrs = true;
      result.abstractAccessors = false;
      result.printAccess = true;
      result.skipEmptyExtensionDecls = false;
      result.skipMissingMemberPlaceholders = false;
      return result;
   }

   /// Print in the style of quick help declaration.
   static PrintOptions printQuickHelpDeclaration()
   {
      PrintOptions options;
      options.enumRawValues = true;
      options.printImplicitAttrs = false;
      options.printFunctionRepresentationAttrs = false;
      options.printDocumentationComments = false;
      options.excludeAttrList.push_back(DAK_Available);
      options.explodeEnumCaseDecls = true;
      options.shouldQualifyNestedDeclarations = QualifyNestedDeclarations::TypesOnly;
      options.printParameterSpecifiers = true;
      return options;
   }
};

} // polar::ast

#endif // POLARPHP_AST_PRINT_OPTIONS_H

