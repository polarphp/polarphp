//===--- AstPrinter.h - Class for printing the AST --------------*- C++ -*-===//
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
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PRINTER_H
#define POLARPHP_AST_PRINTER_H

#include "polarphp/basic/Uuid.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/DenseSet.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/PrintOptions.h"

// forward decare class with namespace
namespace polar::parser {
class SourceLoc;
}

namespace polar::ast {

class Decl;
class DeclContext;
class DynamicSelfType;
class ModuleEntity;
class TypeDecl;
class EnumElementDecl;
class Type;
struct typeLoc;
class NominalTypeDecl;
class ValueDecl;
enum class Token;

using polar::parser::SourceLoc;
using polar::basic::UUID;
using polar::basic::SmallString;
using polar::basic::DenseSet;
using polar::basic::FunctionRef;

/// Describes the context in which a name is being printed, which
/// affects the keywords that need to be escaped.
enum class PrintNameContext
{
   /// Normal context
   Normal,
   /// Keyword context, where no keywords are escaped.
   Keyword,
   /// Generic parameter context, where 'Self' is not escaped.
   GenericParameter,
   /// Class method return type, where 'Self' is not escaped.
   ClassDynamicSelf,
   /// Function parameter context, where keywords other than let/var/inout are
   /// not escaped.
   FunctionParameterExternal,
   FunctionParameterLocal,
   /// Tuple element context, similar to \c FunctionParameterExternal.
   TupleElement,
   /// Attributes, which are escaped as 'Normal', but differentiated for
   /// the purposes of printName* callbacks.
   Attribute,
};

/// Describes the kind of structured entity being printed.
///
/// This includes printables with sub-structure that cannot be completely
/// handled by the printDeclPre/printDeclPost callbacks.
/// E.g.
/// \code
///   func foo(<FunctionParameter>x: Int = 2</FunctionParameter>, ...)
/// \endcode
enum class PrintStructureKind
{
   GenericParameter,
   GenericRequirement,
   FunctionParameter,
   FunctionType,
   FunctionReturnType,
   BuiltinAttribute,
   TupleType,
   TupleElement,
   NumberLiteral,
   StringLiteral,
};

/// An abstract class used to print an AST.
class AstPrinter
{
public:
   virtual ~AstPrinter()
   {}

   virtual void printText(StringRef text) = 0;

   // MARK: Callback interface.

   /// Called after the printer decides not to print decl.
   ///
   /// Callers should use callAvoidPrintDeclPost().
   virtual void avoidPrintDeclPost(const Decl *decl)
   {}

   /// Called before printing of a declaration.
   ///
   /// Callers should use callPrintDeclPre().
   virtual void printDeclPre(const Decl *decl, std::optional<BracketOptions> bracket)
   {}

   /// Called before printing at the point which would be considered the location
   /// of the declaration (normally the name of the declaration).
   ///
   /// Callers should use callPrintDeclLoc().
   virtual void printDeclLoc(const Decl *decl)
   {}

   /// Called after printing the name of the declaration.
   virtual void printDeclNameEndLoc(const Decl *decl)
   {}

   /// Called after printing the name of a declaration, or in the case of
   /// functions its signature.
   virtual void printDeclNameOrSignatureEndLoc(const Decl *decl)
   {}

   /// Called after finishing printing of a declaration.
   ///
   /// Callers should use callPrintDeclPost().
   virtual void printDeclPost(const Decl *decl, std::optional<BracketOptions> bracket)
   {}

   /// Called before printing a type.
   virtual void printTypePre(const typeLoc &typeLoc)
   {}

   /// Called after printing a type.
   virtual void printTypePost(const typeLoc &typeLoc)
   {}

   /// Called when printing the referenced name of a type declaration, possibly
   /// from deep inside another type.
   ///
   /// \param T the original \c Type being referenced. May be null.
   /// \param refTo the \c TypeDecl this is considered a reference to.
   /// \param name the name to be printed.
   virtual void printTypeRef(Type type, const TypeDecl *refTo, Identifier name);


   //   /// Called before printing a synthesized extension.
   //   virtual void printSynthesizedExtensionPre(const ExtensionDecl *extensionDecl,
   //                                             TypeOrExtensionDecl NTD,
   //                                             std::optional<BracketOptions> bracket)
   //   {}

   //   /// Called after printing a synthesized extension.
   //   virtual void printSynthesizedExtensionPost(const ExtensionDecl *extensionDecl,
   //                                              TypeOrExtensionDecl targetDecl,
   //                                              std::optional<BracketOptions> bracket)
   //   {}

   /// Called before printing a structured entity.
   ///
   /// Callers should use callPrintStructurePre().
   virtual void printStructurePre(PrintStructureKind kind,
                                  const Decl *decl = nullptr)
   {}

   /// Called after printing a structured entity.
   virtual void printStructurePost(PrintStructureKind kind,
                                   const Decl *decl = nullptr)
   {}

   /// Called before printing a name in the given context.
   virtual void printNamePre(PrintNameContext context)
   {}

   /// Called after printing a name in the given context.
   virtual void printNamePost(PrintNameContext context)
   {}

   // Helper functions.

   void printSeparator(bool &first, StringRef separator)
   {
      if (first) {
         first = false;
      } else {
         printTextImpl(separator);
      }
   }

   AstPrinter &operator<<(StringRef text)
   {
      printTextImpl(text);
      return *this;
   }

   AstPrinter &operator<<(unsigned long long N);
   AstPrinter &operator<<(UUID uuid);

   AstPrinter &operator<<(DeclName name);

   // Special case for 'char', but not arbitrary things that convert to 'char'.
   template <typename T>
   typename std::enable_if<std::is_same<T, char>::value, AstPrinter &>::type
   operator<<(T c)
   {
      return *this << StringRef(&c, 1);
   }

   void printKeyword(StringRef name, PrintOptions opts, StringRef suffix = "")
   {
      if (opts.skipUnderscoredKeywords && name.startsWith("_")) {
         return;
      }
      callPrintNamePre(PrintNameContext::Keyword);
      *this << name;
      printNamePost(PrintNameContext::Keyword);
      *this << suffix;
   }

   void printAttrName(StringRef name, bool needAt = false)
   {
      callPrintNamePre(PrintNameContext::Attribute);
      if (needAt) {
         *this << "@";
      }
      *this << name;
      printNamePost(PrintNameContext::Attribute);
   }

   AstPrinter &printSimpleAttr(StringRef name, bool needAt = false)
   {
      callPrintStructurePre(PrintStructureKind::BuiltinAttribute);
      printAttrName(name, needAt);
      printStructurePost(PrintStructureKind::BuiltinAttribute);
      return *this;
   }

   void printEscapedStringLiteral(StringRef str);

   void printName(Identifier name,
                  PrintNameContext context = PrintNameContext::Normal);

   void setIndent(unsigned numSpaces)
   {
      m_currentIndentation = numSpaces;
   }

   //   void setSynthesizedTarget(TypeOrExtensionDecl target)
   //   {
   //      assert((!SynthesizeTarget || !target || target == SynthesizeTarget) &&
   //             "unexpected change of setSynthesizedTarget");
   //      // FIXME: this can overwrite the original target with nullptr.
   //      SynthesizeTarget = target;
   //   }

   void printNewline()
   {
      m_pendingNewlines++;
   }

   void forceNewlines()
   {
      if (m_pendingNewlines > 0) {
         SmallString<16> str;
         for (unsigned i = 0; i != m_pendingNewlines; ++i) {
            str += '\n';
         }
         m_pendingNewlines = 0;
         printText(str);
         printIndent();
      }
   }

   virtual void printIndent();

   // MARK: Callback interface wrappers that perform AstPrinter bookkeeping.

   /// Make a callback to printDeclPre(), performing any necessary bookeeping.
   void callPrintDeclPre(const Decl *decl, std::optional<BracketOptions> bracket);

   /// Make a callback to printDeclPost(), performing any necessary bookeeping.
   void callPrintDeclPost(const Decl *decl, std::optional<BracketOptions> bracket)
   {
      printDeclPost(decl, bracket);
   }

   /// Make a callback to avoidPrintDeclPost(), performing any necessary
   /// bookkeeping.
   void callAvoidPrintDeclPost(const Decl *decl)
   {
      avoidPrintDeclPost(decl);
   }

   /// Make a callback to printDeclLoc(), performing any necessary bookeeping.
   void callPrintDeclLoc(const Decl *decl)
   {
      forceNewlines();
      printDeclLoc(decl);
   }

   /// Make a callback to printNamePre(), performing any necessary bookeeping.
   void callPrintNamePre(PrintNameContext context)
   {
      forceNewlines();
      printNamePre(context);
   }

   /// Make a callback to printStructurePre(), performing any necessary
   /// bookkeeping.
   void callPrintStructurePre(PrintStructureKind kind, const Decl *decl = nullptr)
   {
      forceNewlines();
      printStructurePre(kind, decl);
   }

   /// To sanitize a malformed utf8 string to a well-formed one.
   static std::string sanitizeUtf8(StringRef text);
   static ValueDecl* findConformancesWithDocComment(ValueDecl *VD);

private:
   void printTextImpl(StringRef text);
   virtual void anchor();

private:
   unsigned m_currentIndentation = 0;
   unsigned m_pendingNewlines = 0;
};

/// An AST printer for a RawOutStream.
class StreamPrinter : public AstPrinter
{
protected:
   RawOutStream &m_outStream;

public:
   explicit StreamPrinter(RawOutStream &outStream)
      : m_outStream(outStream)
   {}

   void printText(StringRef text) override;
};

/// AST stream printer that adds extra indentation to each line.
class ExtraIndentStreamPrinter : public StreamPrinter
{
   StringRef m_extraIndent;

public:
   ExtraIndentStreamPrinter(RawOutStream &out, StringRef extraIndent)
      : StreamPrinter(out),
        m_extraIndent(extraIndent)
   {}

   virtual void printIndent()
   {
      printText(m_extraIndent);
      StreamPrinter::printIndent();
   }
};

void print_context(RawOutStream &outStream, DeclContext *dc);
bool print_requirement_stub(ValueDecl *requirement, DeclContext *adopter,
                            Type adopterType, SourceLoc typeLoc, RawOutStream &outStream);

/// Print a keyword or punctuator directly by its kind.
RawOutStream &operator<<(RawOutStream &outStream, Token keyword);

/// Get the length of a keyword or punctuator by its kind.
uint8_t get_keyword_len(Token keyword);

/// Get <#code#>;
StringRef get_code_placeholder();

/// Given an array of enum element decls, print them as case statements with
/// placeholders as contents.
void print_enum_elements_as_cases(
      DenseSet<EnumElementDecl *> &unhandledElements, RawOutStream &outStream);

void get_inherited_for_printing(const Decl *decl,
                                FunctionRef<bool(const Decl*)> shouldPrint,
                                SmallVectorImpl<typeLoc> &results);

} // polar::ast

#endif // POLARPHP_AST_PRINTER_H
