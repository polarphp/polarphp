//===--- AST.h - Markup AST nodes -------------------------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/27.

#ifndef POLARPHP_MARKUP_AST_H
#define POLARPHP_MARKUP_AST_H

#include "polarphp/markup/LineList.h"
#include "llvm/ADTSetVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TrailingObjects.h"

#include <optional>

namespace polar::markup {

class MarkupContext;
class MarkupAstNode;
class Paragraph;
class ParamField;
class ReturnsField;
class TagField;
class ThrowsField;
class LocalizationKeyField;

using polar::basic::SmallSetVector;
using polar::basic::ArrayRef;
using polar::utils::TrailingObjects;
using polar::utils::raw_ostream;

/// The basic structure of a doc comment attached to a Swift
/// declaration.
struct CommentParts
{
   std::optional<const Paragraph *> brief;
   ArrayRef<const MarkupAstNode *> bodyNodes;
   ArrayRef<ParamField *> paramFields;
   std::optional<const ReturnsField *> returnsField;
   std::optional<const ThrowsField *> throwsField;
   SmallSetVector<StringRef, 8> Tags;
   std::optional<const LocalizationKeyField *> localizationKeyField;

   bool isEmpty() const
   {
      return !brief.has_value() &&
            !returnsField.has_value() &&
            !throwsField.has_value() &&
            bodyNodes.empty() &&
            paramFields.empty();
   }

   bool hasFunctionDocumentation() const
   {
      return !paramFields.empty() ||
            returnsField.has_value() ||
            throwsField.has_value();
   }
};

#define MARKUP_AST_NODE(Id, Parent) class Id;
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent) class Id;
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"

enum class AstNodeKind : uint8_t
{
#define MARKUP_AST_NODE(Id, Parent) Id,
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId) \
   First_##Id = FirstId, Last_##Id = LastId,
#include "polarphp/markup/AstNodesDefs.h"
};

class alignas(void *) MarkupAstNode
{
   protected:
   AstNodeKind m_kind;

   public:

   MarkupAstNode(AstNodeKind kind)
      : m_kind(kind)
   {}

   AstNodeKind getKind() const
   {
      return m_kind;
   }

   void *operator new(size_t bytes, MarkupContext &mcontext,
                      unsigned alignment = alignof(MarkupAstNode));
   void *operator new(size_t bytes, void *mem)
   {
      assert(mem);
      return mem;
   }

   ArrayRef<MarkupAstNode *> getChildren();
   ArrayRef<const MarkupAstNode *> getChildren() const;
   void *operator new(size_t bytes) = delete;
   void operator delete(void *Data) = delete;

   private:
   MarkupAstNode(const MarkupAstNode &) = delete;
   void operator=(const MarkupAstNode &) = delete;
};

#pragma mark Markdown Nodes

class Document final : public MarkupAstNode,
      private TrailingObjects<Document, MarkupAstNode *>
{
public:
   static Document *create(MarkupContext &mcontext,
                           ArrayRef<MarkupAstNode *> children);

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Document;
   }

private:
   friend class TrailingObjects;
   size_t m_numChildren;
   Document(ArrayRef<MarkupAstNode*> children);
};

class BlockQuote final : public MarkupAstNode,
      private TrailingObjects<BlockQuote, MarkupAstNode *>
{
public:
   static BlockQuote *create(MarkupContext &mcontext, ArrayRef<MarkupAstNode *> children);
   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::BlockQuote;
   }

private:
   friend class TrailingObjects;
   size_t m_numChildren;
   BlockQuote(ArrayRef<MarkupAstNode *> children);
};

class List final : public MarkupAstNode,
      private TrailingObjects<List, MarkupAstNode *>
{
public:
   static List *create(MarkupContext &mcontext, ArrayRef<MarkupAstNode *> items,
                       bool isOrdered);

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   void setChildren(ArrayRef<MarkupAstNode *> newChildren)
   {
      assert(newChildren.size() <= m_numChildren);
      std::copy(newChildren.begin(), newChildren.end(),
                getTrailingObjects<MarkupAstNode *>());
      m_numChildren = newChildren.size();
   }

   bool isOrdered() const
   {
      return m_ordered;
   }

   static bool classOf(const MarkupAstNode *node) {
      return node->getKind() == AstNodeKind::List;
   }

private:
   friend class TrailingObjects;
   size_t m_numChildren;
   bool m_ordered;
   List(ArrayRef<MarkupAstNode *> children, bool isOrdered);
};

class Item final : public MarkupAstNode,
      private TrailingObjects<Item, MarkupAstNode *>
{
   friend class TrailingObjects;

   size_t m_numChildren;

   Item(ArrayRef<MarkupAstNode *> children);

public:
   static Item *create(MarkupContext &mcontext, ArrayRef<MarkupAstNode *> children);

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Item;
   }
};

class CodeBlock final : public MarkupAstNode
{
public:
   static CodeBlock *create(MarkupContext &mcontext, StringRef m_literalContent,
                            StringRef m_language);

   StringRef getLiteralContent() const
   {
      return m_literalContent;
   }

   StringRef getLanguage() const
   {
      return m_language;
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::CodeBlock;
   }

private:
   StringRef m_literalContent;
   StringRef m_language;
   CodeBlock(StringRef literalContent, StringRef language)
      : MarkupAstNode(AstNodeKind::CodeBlock),
        m_literalContent(literalContent),
        m_language(language)
   {}
};

class HTML final : public MarkupAstNode
{
   StringRef m_literalContent;
   HTML(StringRef literalContent)
      : MarkupAstNode(AstNodeKind::HTML),
        m_literalContent(literalContent)
   {}
public:
   static HTML *create(MarkupContext &mcontext, StringRef literalContent);
   StringRef getLiteralContent() const
   {
      return m_literalContent;
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::HTML;
   }
};

class Paragraph final : public MarkupAstNode,
      private TrailingObjects<Paragraph, MarkupAstNode *>
{
public:
   static Paragraph *create(MarkupContext &mcontext,
                            ArrayRef<MarkupAstNode *> children);

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Paragraph;
   }

private:
   friend class TrailingObjects;
   size_t m_numChildren;
   Paragraph(ArrayRef<MarkupAstNode *> children);
};

class Header final : public MarkupAstNode,
      private TrailingObjects<Header, MarkupAstNode *>
{
public:
   static Header *create(MarkupContext &mcontext, unsigned m_level,
                         ArrayRef<MarkupAstNode *> children);

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   unsigned getLevel() const
   {
      return m_level;
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Header;
   }
private:
   friend class TrailingObjects;
   size_t m_numChildren;
   unsigned m_level;
   Header(unsigned level, ArrayRef<MarkupAstNode *> children);
};

class HRule final : public MarkupAstNode
{
   HRule() : MarkupAstNode(AstNodeKind::HRule)
   {}
public:
   static HRule *create(MarkupContext &mcontext);

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::HRule;
   }
};

class InlineContent : public MarkupAstNode
{
public:
   InlineContent(AstNodeKind kind) : MarkupAstNode(kind)
   {}

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() >= AstNodeKind::First_Inline &&
            node->getKind() <= AstNodeKind::Last_Inline;
   }
};

class Text final : public InlineContent
{
public:
   static Text *create(MarkupContext &mcontext, StringRef iteralContent);
   StringRef getLiteralContent() const
   {
      return m_literalContent;
   }

   void setLiteralContent(StringRef content)
   {
      m_literalContent = content;
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   StringRef str() const
   {
      return m_literalContent;
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Text;
   }

private:
   StringRef m_literalContent;
   Text(StringRef m_literalContent)
      : InlineContent(AstNodeKind::Text),
        m_literalContent(m_literalContent)
   {}
};

class SoftBreak final : public InlineContent
{
   SoftBreak() : InlineContent(AstNodeKind::SoftBreak)
   {}
public:
   static SoftBreak *create(MarkupContext &mcontext);
   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   StringRef str() const
   {
      return "\n";
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::SoftBreak;
   }
};

class LineBreak final : public InlineContent
{
   LineBreak()
      : InlineContent(AstNodeKind::LineBreak)
   {}
public:
   static LineBreak *create(MarkupContext &mcontext);

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   StringRef str() const
   {
      return "\n";
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::LineBreak;
   }
};

class Code final : public InlineContent
{
public:
   static Code *create(MarkupContext &mcontext, StringRef literalContent);

   StringRef getLiteralContent() const
   {
      return m_literalContent;
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   StringRef str() const
   {
      return m_literalContent;
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Code;
   }
private:
   StringRef m_literalContent;
   Code(StringRef literalContent)
      : InlineContent(AstNodeKind::Code),
        m_literalContent(literalContent)
   {}
};

class InlineHTML final : public InlineContent
{
public:
   static InlineHTML *create(MarkupContext &mcontext, StringRef m_literalContent);

   StringRef getLiteralContent() const
   {
      return m_literalContent;
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   StringRef str() const
   {
      return m_literalContent;
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::InlineHTML;
   }
private:
   StringRef m_literalContent;
   InlineHTML(StringRef literalContent)
      : InlineContent(AstNodeKind::InlineHTML),
        m_literalContent(literalContent)
   {}
};

class Emphasis final : public InlineContent,
      private TrailingObjects<Emphasis, MarkupAstNode *>
{
public:
   static Emphasis *create(MarkupContext &mcontext,
                           ArrayRef<MarkupAstNode *> children);

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Emphasis;
   }
private:
   friend class TrailingObjects;
   size_t m_numChildren;
   Emphasis(ArrayRef<MarkupAstNode *> children);
};

class Strong final : public InlineContent,
      private TrailingObjects<Strong, MarkupAstNode *>
{
public:
   static Strong *create(MarkupContext &mcontext,
                         ArrayRef<MarkupAstNode *> children);

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Strong;
   }
private:
   friend class TrailingObjects;
   size_t m_numChildren;
   Strong(ArrayRef<MarkupAstNode *> children);
};

class Link final : public InlineContent,
      private TrailingObjects<Link, MarkupAstNode *>
{
public:
   static Link *create(MarkupContext &mcontext,
                       StringRef destination,
                       ArrayRef<MarkupAstNode *> children);

   StringRef getDestination() const
   {
      return m_destination;
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Link;
   }
private:
   friend class TrailingObjects;
   size_t m_numChildren;
   StringRef m_destination;
   Link(StringRef destination, ArrayRef<MarkupAstNode *> children);
};

class Image final : public InlineContent,
      private TrailingObjects<Image, MarkupAstNode *>
{
public:
   static Image *create(MarkupContext &mcontext,
                        StringRef destination,
                        std::optional<StringRef> title,
                        ArrayRef<MarkupAstNode *> children);

   StringRef getDestination() const
   {
      return m_destination;
   }

   bool hasTitle() const
   {
      return m_title.has_value();
   }

   StringRef getTitle() const
   {
      return StringRef(m_title.value());
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::Image;
   }
private:
   friend class TrailingObjects;
   size_t m_numChildren;
   // FIXME: Hyperlink destinations can't be wrapped - use a Line
   StringRef m_destination;
   std::optional<StringRef> m_title;

   Image(StringRef m_destination, std::optional<StringRef> title,
         ArrayRef<MarkupAstNode *> children);
};

#pragma mark Private Extensions

class PrivateExtension : public MarkupAstNode
{
protected:
   PrivateExtension(AstNodeKind kind)
      : MarkupAstNode(kind) {}
public:

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {};
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() >= AstNodeKind::First_Private &&
            node->getKind() <= AstNodeKind::Last_Private;
   }
};

class ParamField final : public PrivateExtension,
      private TrailingObjects<ParamField, MarkupAstNode *>
{
public:
   static ParamField *create(MarkupContext &mcontext, StringRef name,
                             ArrayRef<MarkupAstNode *> children);

   StringRef getName() const
   {
      return m_name;
   }

   std::optional<CommentParts> getParts() const
   {
      return m_parts;
   }

   void setParts(CommentParts parts)
   {
      m_parts = parts;
   }

   bool isClosureParameter() const
   {
      if (!m_parts.has_value()) {
         return false;
      }
      return m_parts.value().hasFunctionDocumentation();
   }

   ArrayRef<MarkupAstNode *> getChildren()
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   ArrayRef<const MarkupAstNode *> getChildren() const
   {
      return {getTrailingObjects<MarkupAstNode *>(), m_numChildren};
   }

   static bool classOf(const MarkupAstNode *node)
   {
      return node->getKind() == AstNodeKind::ParamField;
   }
private:
   friend class TrailingObjects;
   size_t m_numChildren;
   StringRef m_name;
   // Parameter fields can contain a substructure describing a
   // function or closure parameter.
   std::optional<CommentParts> m_parts;
   ParamField(StringRef name, ArrayRef<MarkupAstNode *> children);
};

#define MARKUP_SIMPLE_FIELD(Id, Keyword, XMLKind) \
   class Id final : public PrivateExtension, \
   private TrailingObjects<Id, MarkupAstNode *> { \
   friend class TrailingObjects; \
   \
   size_t m_numChildren; \
   \
   Id(ArrayRef<MarkupAstNode *> children);\
   \
   public: \
   static Id *create(MarkupContext &mcontext, ArrayRef<MarkupAstNode *> children); \
   \
   ArrayRef<MarkupAstNode *> getChildren() { \
   return {getTrailingObjects<MarkupAstNode *>(), m_numChildren}; \
} \
   \
   ArrayRef<const MarkupAstNode *> getChildren() const { \
   return {getTrailingObjects<MarkupAstNode *>(), m_numChildren}; \
} \
   \
   static bool classOf(const MarkupAstNode *node) { \
   return node->getKind() == AstNodeKind::Id; \
} \
};
#include "polarphp/markup/SimpleFieldsDefs.h"

class MarkupAstWalker
{
public:
   void walk(const MarkupAstNode *node)
   {
      enter(node);

      switch(node->getKind()) {
#define MARKUP_AST_NODE(Id, Parent)                                         \
      case AstNodeKind::Id:                                                   \
   visit##Id(static_cast<const Id*>(node));                              \
   break;
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"
      }

      if (shouldVisitChildrenOf(node)) {
         for (auto child : node->getChildren()) {
            walk(child);
         }

      }
      exit(node);
   }

   virtual bool shouldVisitChildrenOf(const MarkupAstNode *node)
   {
      return true;
   }

   virtual void enter(const MarkupAstNode *node)
   {}

   virtual void exit(const MarkupAstNode *node)
   {}

#define MARKUP_AST_NODE(Id, Parent) \
   virtual void visit##Id(const Id *node) {}
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"

   virtual ~MarkupAstWalker() = default;
};

MarkupAstNode *create_simple_field(MarkupContext &mcontext, StringRef tag,
                                   ArrayRef<MarkupAstNode *> children);

bool is_a_field_tag(StringRef Tag);

void dump(const MarkupAstNode *node, raw_ostream &OS, unsigned indent = 0);
void print_inlines_under(const MarkupAstNode *node, raw_ostream &OS,
                         bool printDecorators = false);


template <typename ImplClass, typename RetTy = void, typename... Args>
class MarkupAstVisitor
{
public:
   RetTy visit(const MarkupAstNode *node, Args... args)
   {
//      switch (node->getKind()) {
//#define MARKUP_AST_NODE(Id, Parent) \
//      case AstNodeKind::Id: \
//   return static_cast<ImplClass*>(this) \
//   ->visit##Id(cast<const Id>(node), \
//   ::std::forward<Args>(args)...);
//#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
//#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
//#include "polarphp/markup/AstNodesDefs.h"
//      }
   }
   virtual ~MarkupAstVisitor() {}
};


} // polar::markup

#endif // POLARPHP_MARKUP_AST_H
