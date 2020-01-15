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
//
#ifndef POLARPHP_MARKUP_AST_H
#define POLARPHP_MARKUP_AST_H

#include "polarphp/markup/LineList.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TrailingObjects.h"

namespace polar::markup {

using llvm::Optional;
using llvm::ArrayRef;
using llvm::StringRef;
using llvm::cast;

class MarkupContext;
class MarkupAstNode;
class Paragraph;
class ParamField;
class ReturnsField;
class TagField;
class ThrowsField;
class LocalizationKeyField;

/// The basic structure of a doc comment attached to a Swift
/// declaration.
struct CommentParts {
  Optional<const Paragraph *> brief;
  ArrayRef<const MarkupAstNode *> bodyNodes;
  ArrayRef<ParamField *> paramFields;
  Optional<const ReturnsField *> returnsField;
  Optional<const ThrowsField *> throwsField;
  llvm::SmallSetVector<StringRef, 8> tags;
  Optional<const LocalizationKeyField *> localizationKeyField;

  bool isEmpty() const {
    return !brief.hasValue() &&
           !returnsField.hasValue() &&
           !throwsField.hasValue() &&
           bodyNodes.empty() &&
           paramFields.empty();
  }

  bool hasFunctionDocumentation() const {
    return !paramFields.empty() ||
             returnsField.hasValue() ||
             throwsField.hasValue();
  }
};

#define MARKUP_AST_NODE(Id, Parent) class Id;
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent) class Id;
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"

enum class AstNodeKind : uint8_t {
#define MARKUP_AST_NODE(Id, Parent) Id,
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId) \
  First_##Id = FirstId, Last_##Id = LastId,
#include "polarphp/markup/AstNodesDefs.h"
};

class alignas(void *) MarkupAstNode {
  MarkupAstNode(const MarkupAstNode &) = delete;
  void operator=(const MarkupAstNode &) = delete;

protected:
  AstNodeKind Kind;

public:

  MarkupAstNode(AstNodeKind Kind) : Kind(Kind) {}

  AstNodeKind getKind() const { return Kind; }

  void *operator new(size_t Bytes, MarkupContext &MC,
                     unsigned alignment = alignof(MarkupAstNode));
  void *operator new(size_t Bytes, void *Mem) {
    assert(Mem);
    return Mem;
  }

  ArrayRef<MarkupAstNode *> getChildren();
  ArrayRef<const MarkupAstNode *> getChildren() const;
  void *operator new(size_t Bytes) = delete;
  void operator delete(void *Data) = delete;
};

#pragma mark Markdown Nodes

class Document final : public MarkupAstNode,
    private llvm::TrailingObjects<Document, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  Document(ArrayRef<MarkupAstNode*> Children);

public:
  static Document *create(MarkupContext &MC,
                          ArrayRef<MarkupAstNode *> Children);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Document;
  }
};

class BlockQuote final : public MarkupAstNode,
    private llvm::TrailingObjects<BlockQuote, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  BlockQuote(ArrayRef<MarkupAstNode *> Children);

public:
  static BlockQuote *create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Children);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::BlockQuote;
  }
};

class List final : public MarkupAstNode,
    private llvm::TrailingObjects<List, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;
  bool Ordered;

  List(ArrayRef<MarkupAstNode *> Children, bool IsOrdered);

public:
  static List *create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Items,
                      bool IsOrdered);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  void setChildren(ArrayRef<MarkupAstNode *> NewChildren) {
    assert(NewChildren.size() <= NumChildren);
    std::copy(NewChildren.begin(), NewChildren.end(),
              getTrailingObjects<MarkupAstNode *>());
    NumChildren = NewChildren.size();
  }

  bool isOrdered() const {
    return Ordered;
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::List;
  }
};

class Item final : public MarkupAstNode,
    private llvm::TrailingObjects<Item, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  Item(ArrayRef<MarkupAstNode *> Children);

public:
  static Item *create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Children);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Item;
  }
};

class CodeBlock final : public MarkupAstNode {
  StringRef LiteralContent;
  StringRef Language;

  CodeBlock(StringRef LiteralContent, StringRef Language)
      : MarkupAstNode(AstNodeKind::CodeBlock),
        LiteralContent(LiteralContent),
        Language(Language) {}

public:
  static CodeBlock *create(MarkupContext &MC, StringRef LiteralContent,
                           StringRef Language);

  StringRef getLiteralContent() const { return LiteralContent; };
  StringRef getLanguage() const { return Language; };

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::CodeBlock;
  }
};

class HTML final : public MarkupAstNode {
  StringRef LiteralContent;
  HTML(StringRef LiteralContent)
      : MarkupAstNode(AstNodeKind::HTML),
        LiteralContent(LiteralContent) {}
public:
  static HTML *create(MarkupContext &MC, StringRef LiteralContent);
  StringRef getLiteralContent() const { return LiteralContent; };

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::HTML;
  }
};

class Paragraph final : public MarkupAstNode,
    private llvm::TrailingObjects<Paragraph, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  Paragraph(ArrayRef<MarkupAstNode *> Children);

public:
  static Paragraph *create(MarkupContext &MC,
                           ArrayRef<MarkupAstNode *> Children);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Paragraph;
  }
};

class Header final : public MarkupAstNode,
    private llvm::TrailingObjects<Header, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;
  unsigned Level;

  Header(unsigned Level, ArrayRef<MarkupAstNode *> Children);

public:
  static Header *create(MarkupContext &MC, unsigned Level,
                        ArrayRef<MarkupAstNode *> Children);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  unsigned getLevel() const {
    return Level;
  }
  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Header;
  }
};

class HRule final : public MarkupAstNode {
  HRule() : MarkupAstNode(AstNodeKind::HRule) {}
public:
  static HRule *create(MarkupContext &MC);

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::HRule;
  }
};

class InlineContent : public MarkupAstNode {
public:
  InlineContent(AstNodeKind Kind) : MarkupAstNode(Kind) {}

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() >= AstNodeKind::First_Inline &&
           N->getKind() <= AstNodeKind::Last_Inline;
  }
};

class Text final : public InlineContent {
  StringRef LiteralContent;
  Text(StringRef LiteralContent)
    : InlineContent(AstNodeKind::Text),
      LiteralContent(LiteralContent) {}
public:
  static Text *create(MarkupContext &MC, StringRef LiteralContent);
  StringRef getLiteralContent() const { return LiteralContent; };
  void setLiteralContent(StringRef LC) {
    LiteralContent = LC;
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  StringRef str() const {
    return LiteralContent;
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Text;
  }
};

class SoftBreak final : public InlineContent {
  SoftBreak() : InlineContent(AstNodeKind::SoftBreak) {}
public:
  static SoftBreak *create(MarkupContext &MC);

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  StringRef str() const {
    return "\n";
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::SoftBreak;
  }
};

class LineBreak final : public InlineContent {
  LineBreak() : InlineContent(AstNodeKind::LineBreak) {}
public:
  static LineBreak *create(MarkupContext &MC);

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  StringRef str() const {
    return "\n";
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::LineBreak;
  }
};

class Code final : public InlineContent {
  StringRef LiteralContent;

  Code(StringRef LiteralContent)
      : InlineContent(AstNodeKind::Code),
        LiteralContent(LiteralContent) {}
public:
  static Code *create(MarkupContext &MC, StringRef LiteralContent);

  StringRef getLiteralContent() const { return LiteralContent; };

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  StringRef str() const {
    return LiteralContent;
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Code;
  }
};

class InlineHTML final : public InlineContent {
  StringRef LiteralContent;
  InlineHTML(StringRef LiteralContent)
    : InlineContent(AstNodeKind::InlineHTML),
      LiteralContent(LiteralContent) {}
public:
  static InlineHTML *create(MarkupContext &MC, StringRef LiteralContent);

  StringRef getLiteralContent() const { return LiteralContent; };

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  StringRef str() const {
    return LiteralContent;
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::InlineHTML;
  }
};

class Emphasis final : public InlineContent,
    private llvm::TrailingObjects<Emphasis, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  Emphasis(ArrayRef<MarkupAstNode *> Children);
public:
  static Emphasis *create(MarkupContext &MC,
                          ArrayRef<MarkupAstNode *> Children);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Emphasis;
  }
};

class Strong final : public InlineContent,
    private llvm::TrailingObjects<Strong, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  Strong(ArrayRef<MarkupAstNode *> Children);
public:
  static Strong *create(MarkupContext &MC,
                        ArrayRef<MarkupAstNode *> Children);

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Strong;
  }
};

class Link final : public InlineContent,
    private llvm::TrailingObjects<Link, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  StringRef Destination;

  Link(StringRef Destination, ArrayRef<MarkupAstNode *> Children);

public:
  static Link *create(MarkupContext &MC,
                      StringRef Destination,
                      ArrayRef<MarkupAstNode *> Children);

  StringRef getDestination() const { return Destination; }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Link;
  }
};

class Image final : public InlineContent,
    private llvm::TrailingObjects<Image, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  // FIXME: Hyperlink destinations can't be wrapped - use a Line
  StringRef Destination;
  Optional<StringRef> Title;

  Image(StringRef Destination, Optional<StringRef> Title,
        ArrayRef<MarkupAstNode *> Children);

public:
  static Image *create(MarkupContext &MC,
                      StringRef Destination,
                      Optional<StringRef> Title,
                      ArrayRef<MarkupAstNode *> Children);

  StringRef getDestination() const { return Destination; }

  bool hasTitle() const {
    return Title.hasValue();
  }

  StringRef getTitle() const {
    return StringRef(Title.getValue());
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::Image;
  }
};

#pragma mark Private Extensions

class PrivateExtension : public MarkupAstNode {
protected:
  PrivateExtension(AstNodeKind Kind)
      : MarkupAstNode(Kind) {}
public:

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {};
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() >= AstNodeKind::First_Private &&
      N->getKind() <= AstNodeKind::Last_Private;
  }
};

class ParamField final : public PrivateExtension,
    private llvm::TrailingObjects<ParamField, MarkupAstNode *> {
  friend TrailingObjects;

  size_t NumChildren;

  StringRef Name;

  // Parameter fields can contain a substructure describing a
  // function or closure parameter.
  llvm::Optional<CommentParts> Parts;

  ParamField(StringRef Name, ArrayRef<MarkupAstNode *> Children);

public:

  static ParamField *create(MarkupContext &MC, StringRef Name,
                            ArrayRef<MarkupAstNode *> Children);

  StringRef getName() const {
    return Name;
  }

  llvm::Optional<CommentParts> getParts() const {
    return Parts;
  }

  void setParts(CommentParts P) {
    Parts = P;
  }

  bool isClosureParameter() const {
    if (!Parts.hasValue())
      return false;

    return Parts.getValue().hasFunctionDocumentation();
  }

  ArrayRef<MarkupAstNode *> getChildren() {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  ArrayRef<const MarkupAstNode *> getChildren() const {
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren};
  }

  static bool classof(const MarkupAstNode *N) {
    return N->getKind() == AstNodeKind::ParamField;
  }
};

#define MARKUP_SIMPLE_FIELD(Id, Keyword, XMLKind) \
class Id final : public PrivateExtension, \
    private llvm::TrailingObjects<Id, MarkupAstNode *> { \
  friend TrailingObjects; \
\
  size_t NumChildren; \
\
  Id(ArrayRef<MarkupAstNode *> Children);\
\
public: \
  static Id *create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Children); \
\
  ArrayRef<MarkupAstNode *> getChildren() { \
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren}; \
  } \
\
  ArrayRef<const MarkupAstNode *> getChildren() const { \
    return {getTrailingObjects<MarkupAstNode *>(), NumChildren}; \
  } \
\
  static bool classof(const MarkupAstNode *N) { \
    return N->getKind() == AstNodeKind::Id; \
  } \
};
#include "polarphp/markup/SimpleFieldsDefs.h"

class MarkupAstWalker {
public:
  void walk(const MarkupAstNode *Node) {
    enter(Node);

    switch(Node->getKind()) {
#define MARKUP_AST_NODE(Id, Parent)                                         \
    case AstNodeKind::Id:                                                   \
      visit##Id(static_cast<const Id*>(Node));                              \
      break;
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"
    }

    if (shouldVisitChildrenOf(Node))
      for (auto Child : Node->getChildren())
        walk(Child);

    exit(Node);
  }

  virtual bool shouldVisitChildrenOf(const MarkupAstNode *Node) {
    return true;
  }

  virtual void enter(const MarkupAstNode *Node) {}
  virtual void exit(const MarkupAstNode *Node) {}

#define MARKUP_AST_NODE(Id, Parent) \
  virtual void visit##Id(const Id *Node) {}
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"

  virtual ~MarkupAstWalker() = default;
};

MarkupAstNode *createSimpleField(MarkupContext &MC, StringRef Tag,
                                 ArrayRef<MarkupAstNode *> Children);

bool isAFieldTag(StringRef Tag);

void dump(const MarkupAstNode *Node, llvm::raw_ostream &OS, unsigned indent = 0);
void printInlinesUnder(const MarkupAstNode *Node, llvm::raw_ostream &OS,
                       bool PrintDecorators = false);


template <typename ImplClass, typename RetTy = void, typename... Args>
class MarkupAstVisitor {
public:
  RetTy visit(const MarkupAstNode *Node, Args... args) {
    switch (Node->getKind()) {
#define MARKUP_AST_NODE(Id, Parent) \
    case AstNodeKind::Id: \
      return static_cast<ImplClass*>(this) \
        ->visit##Id(cast<const Id>(Node), \
                    ::std::forward<Args>(args)...);
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent)
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"
    }
  }

  virtual ~MarkupAstVisitor() {}
};

} // namespace polar::markup

#endif // POLARPHP_MARKUP_AST_H
