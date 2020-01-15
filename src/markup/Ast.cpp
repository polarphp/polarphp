//===--- AST.cpp - Extraction of raw comments -----------------------------===//
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
///
/// \file
/// This file implements polarphp markup AST nodes.
///
//===----------------------------------------------------------------------===//

#include "polarphp/markup/Markup.h"
#include "polarphp/markup/Ast.h"
#include "llvm/ADT/Optional.h"

using namespace polar;
using namespace markup;
using namespace llvm;

Document::Document(ArrayRef<MarkupAstNode*> Children)
   : MarkupAstNode(AstNodeKind::Document), NumChildren(Children.size()) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Document *Document::create(MarkupContext &MC,
                           ArrayRef<polar::markup::MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Document));
   return new (Mem) Document(Children);
}

BlockQuote::BlockQuote(ArrayRef<MarkupAstNode*> Children)
   : MarkupAstNode(AstNodeKind::BlockQuote), NumChildren(Children.size()) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

BlockQuote *BlockQuote::create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(BlockQuote));
   return new (Mem) BlockQuote(Children);
}

HTML *HTML::create(MarkupContext &MC, StringRef LiteralContent) {
   void *Mem = MC.allocate(sizeof(HTML), alignof(HTML));
   return new (Mem) HTML(LiteralContent);
}

InlineHTML *InlineHTML::create(MarkupContext &MC, StringRef LiteralContent) {
   void *Mem = MC.allocate(sizeof(InlineHTML), alignof(InlineHTML));
   return new (Mem) InlineHTML(LiteralContent);
}

Code *Code::create(MarkupContext &MC, StringRef LiteralContent) {
   void *Mem = MC.allocate(sizeof(Code), alignof(Code));
   return new (Mem) Code(LiteralContent);
}

CodeBlock *CodeBlock::create(MarkupContext &MC, StringRef LiteralContent,
                             StringRef Language) {
   void *Mem = MC.allocate(sizeof(CodeBlock), alignof(CodeBlock));
   return new (Mem) CodeBlock(LiteralContent, Language);
}

List::List(ArrayRef<MarkupAstNode *> Children, bool IsOrdered)
   : MarkupAstNode(AstNodeKind::List), NumChildren(Children.size()),
     Ordered(IsOrdered) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

List *List::create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Children,
                   bool IsOrdered) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(List));
   return new (Mem) List(Children, IsOrdered);
}

Item::Item(ArrayRef<MarkupAstNode*> Children)
   : MarkupAstNode(AstNodeKind::Item), NumChildren(Children.size()) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Item *Item::create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Item));
   return new (Mem) Item(Children);
}

Link::Link(StringRef Destination, ArrayRef<MarkupAstNode *> Children)
   : InlineContent(AstNodeKind::Link), NumChildren(Children.size()),
     Destination(Destination) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Link *Link::create(MarkupContext &MC, StringRef Destination,
                   ArrayRef<MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Link));
   StringRef DestinationCopy = MC.allocateCopy(Destination);
   return new (Mem) Link(DestinationCopy, Children);
}

Image::Image(StringRef Destination, Optional<StringRef> Title,
             ArrayRef<MarkupAstNode *> Children)
   : InlineContent(AstNodeKind::Image), NumChildren(Children.size()),
     Destination(Destination), Title(Title) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Image *Image::create(MarkupContext &MC, StringRef Destination,
                     Optional<StringRef> Title,
                     ArrayRef<MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Image));
   StringRef DestinationCopy = MC.allocateCopy(Destination);
   Optional<StringRef> TitleCopy;
   if (Title)
      TitleCopy = MC.allocateCopy(*Title);
   return new (Mem) Image(DestinationCopy, TitleCopy, Children);
}

Header::Header(unsigned Level, ArrayRef<MarkupAstNode *> Children)
   : MarkupAstNode(AstNodeKind::Header), NumChildren(Children.size()),
     Level(Level) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Header *Header::create(MarkupContext &MC, unsigned Level,
                       ArrayRef<MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Header));
   return new (Mem) Header(Level, Children);
}

Paragraph::Paragraph(ArrayRef<MarkupAstNode *> Children)
   : MarkupAstNode(AstNodeKind::Paragraph),
     NumChildren(Children.size()) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Paragraph *Paragraph::create(MarkupContext &MC,
                             ArrayRef<polar::markup::MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Paragraph));
   return new (Mem) Paragraph(Children);
}

HRule *HRule::create(MarkupContext &MC) {
   void *Mem = MC.allocate(sizeof(HRule), alignof(HRule));
   return new (Mem) HRule();
}

Text *Text::create(MarkupContext &MC, StringRef LiteralContent) {
   void *Mem = MC.allocate(sizeof(Text), alignof(Text));
   return new (Mem) Text(LiteralContent);
}

SoftBreak *SoftBreak::create(MarkupContext &MC) {
   void *Mem = MC.allocate(sizeof(SoftBreak), alignof(SoftBreak));
   return new (Mem) SoftBreak();
}

LineBreak *LineBreak::create(MarkupContext &MC) {
   void *Mem = MC.allocate(sizeof(LineBreak), alignof(LineBreak));
   return new (Mem) LineBreak();
}

Emphasis::Emphasis(ArrayRef<MarkupAstNode *> Children)
   : InlineContent(AstNodeKind::Emphasis), NumChildren(Children.size()) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Emphasis *Emphasis::create(MarkupContext &MC,
                           ArrayRef<polar::markup::MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Emphasis));
   return new (Mem) Emphasis(Children);
}

Strong::Strong(ArrayRef<MarkupAstNode *> Children)
   : InlineContent(AstNodeKind::Strong), NumChildren(Children.size()) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

Strong *Strong::create(MarkupContext &MC,
                       ArrayRef<polar::markup::MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(Strong));
   return new (Mem) Strong(Children);
}

ParamField::ParamField(StringRef Name, ArrayRef<MarkupAstNode *> Children)
   : PrivateExtension(AstNodeKind::ParamField), NumChildren(Children.size()),
     Name(Name),
     Parts(None) {
   std::uninitialized_copy(Children.begin(), Children.end(),
                           getTrailingObjects<MarkupAstNode *>());
}

ParamField *ParamField::create(MarkupContext &MC, StringRef Name,
                               ArrayRef<MarkupAstNode *> Children) {
   void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()),
                           alignof(ParamField));
   return new (Mem) ParamField(Name, Children);
}

#define MARKUP_SIMPLE_FIELD(Id, Keyword, XMLKind) \
Id *Id::create(MarkupContext &MC, ArrayRef<MarkupAstNode *> Children) { \
  void *Mem = MC.allocate(totalSizeToAlloc<MarkupAstNode *>(Children.size()), \
                          alignof(Id)); \
  return new (Mem) Id(Children); \
} \
\
Id::Id(ArrayRef<MarkupAstNode *> Children) \
    : PrivateExtension(AstNodeKind::Id), NumChildren(Children.size()) { \
  std::uninitialized_copy(Children.begin(), Children.end(), \
                          getTrailingObjects<MarkupAstNode *>()); \
}
#include "polarphp/markup/SimpleFieldsDefs.h"

ArrayRef<MarkupAstNode *> MarkupAstNode::getChildren() {
   switch (Kind) {
#define MARKUP_AST_NODE(Id, Parent) \
  case AstNodeKind::Id: \
      return cast<Id>(this)->getChildren();
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent) class Id;
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"
   }

   llvm_unreachable("Unhandled AstNodeKind in switch.");
}

ArrayRef<const MarkupAstNode *> MarkupAstNode::getChildren() const {
   switch (Kind) {
#define MARKUP_AST_NODE(Id, Parent) \
case AstNodeKind::Id: \
return cast<Id>(this)->getChildren();
#define ABSTRACT_MARKUP_AST_NODE(Id, Parent) class Id;
#define MARKUP_AST_NODE_RANGE(Id, FirstId, LastId)
#include "polarphp/markup/AstNodesDefs.h"
   }

   llvm_unreachable("Unhandled AstNodeKind in switch.");
}

void polar::markup::printInlinesUnder(const MarkupAstNode *Node,
                                      llvm::raw_ostream &OS,
                                      bool PrintDecorators) {
   auto printChildren = [](const ArrayRef<const MarkupAstNode *> Children,
                           llvm::raw_ostream &OS) {
      for (auto Child = Children.begin(); Child != Children.end(); Child++)
         polar::markup::printInlinesUnder(*Child, OS);
   };

   switch (Node->getKind()) {
      case polar::markup::AstNodeKind::HTML: {
         auto H = cast<HTML>(Node);
         OS << H->getLiteralContent();
         break;
      }
      case polar::markup::AstNodeKind::InlineHTML: {
         auto IH = cast<InlineHTML>(Node);
         OS << IH->getLiteralContent();
         break;
      }
      case polar::markup::AstNodeKind::HRule:
         OS << '\n';
         break;
      case polar::markup::AstNodeKind::Text: {
         auto T = cast<Text>(Node);
         OS << T->getLiteralContent();
         break;
      }
      case polar::markup::AstNodeKind::SoftBreak:
         OS << ' ';
         break;
      case polar::markup::AstNodeKind::LineBreak:
         OS << '\n';
         break;
      case polar::markup::AstNodeKind::Code: {
         auto C = cast<Code>(Node);
         if (PrintDecorators)
            OS << '`';

         OS << C->getLiteralContent();

         if (PrintDecorators)
            OS << '`';

         break;
      }
      case polar::markup::AstNodeKind::CodeBlock: {
         auto CB = cast<CodeBlock>(Node);
         if (PrintDecorators) OS << "``";

         OS << CB->getLiteralContent();

         if (PrintDecorators) OS << "``";

         break;
      }
      case polar::markup::AstNodeKind::Emphasis: {
         auto E = cast<Emphasis>(Node);
         if (PrintDecorators) OS << '*';
         printChildren(E->getChildren(), OS);
         if (PrintDecorators) OS << '*';
         break;
      }
      case polar::markup::AstNodeKind::Strong: {
         auto S = cast<Strong>(Node);
         if (PrintDecorators) OS << "**";
         printChildren(S->getChildren(), OS);
         if (PrintDecorators) OS << "**";
         break;
      }
      default:
         printChildren(Node->getChildren(), OS);
   }
   OS.flush();
}

polar::markup::MarkupAstNode *polar::markup::createSimpleField(
   MarkupContext &MC,
   StringRef Tag,
   ArrayRef<polar::markup::MarkupAstNode *> Children) {
   if (false) {

   }
#define MARKUP_SIMPLE_FIELD(Id, Keyword, XMLKind) \
  else if (Tag.compare_lower(#Keyword) == 0) { \
    return Id::create(MC, Children); \
  }
#include "polarphp/markup/SimpleFieldsDefs.h"
   llvm_unreachable("Given tag not for any simple markup field");
}

bool polar::markup::isAFieldTag(StringRef Tag) {
   if (false) {

   }
#define MARKUP_SIMPLE_FIELD(Id, Keyword, XMLKind) \
  else if (Tag.compare_lower(#Keyword) == 0) { \
    return true; \
  }
#include "polarphp/markup/SimpleFieldsDefs.h"
   return false;
}

void polar::markup::dump(const MarkupAstNode *Node, llvm::raw_ostream &OS,
                         unsigned indent) {
   auto dumpChildren = [](const ArrayRef<const MarkupAstNode *> Children,
                          llvm::raw_ostream &OS, unsigned indent) {
      OS << '\n';
      for (auto Child = Children.begin(); Child != Children.end(); Child++) {
         polar::markup::dump(*Child, OS, indent + 1);
         if (Child != Children.end() - 1)
            OS << '\n';
      }
   };

   auto simpleEscapingPrint = [](StringRef LiteralContent,
                                 llvm::raw_ostream &OS) {
      OS << "\"";
      for (auto C : LiteralContent) {
         switch (C) {
            case '\n':
               OS << "\\n";
               break;
            case '\r':
               OS << "\\r";
               break;
            case '\t':
               OS << "\\t";
               break;
            case '"':
               OS << "\\\"";
               break;
            default:
               OS << C;
         }
      }
      OS << "\"";
   };

   for (unsigned i = 0; i < indent; ++i) {
      OS << ' ';
   }

   OS << '(';
   switch (Node->getKind()) {
      case polar::markup::AstNodeKind::Document: {
         OS << "Document: Children=" << Node->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::BlockQuote: {
         OS << "BlockQuote: Children=" << Node->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::List: {
         auto L = cast<List>(Node);
         OS << "List: " << (L->isOrdered() ? "Ordered " : "Unordered ");
         OS << "Items=" << Node->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::Item: {
         OS << "Item: Children=" << Node->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::HTML: {
         auto H = cast<HTML>(Node);
         OS << "HTML: Content=";
         simpleEscapingPrint(H->getLiteralContent(), OS);
         break;
      }
      case polar::markup::AstNodeKind::InlineHTML: {
         auto IH = cast<InlineHTML>(Node);
         OS << "InlineHTML: Content=";
         simpleEscapingPrint(IH->getLiteralContent(), OS);
         break;
      }
      case polar::markup::AstNodeKind::Paragraph: {
         OS << "Paragraph: Children=" << Node->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::Header: {
         auto H = cast<Header>(Node);
         OS << "Header: Level=" << H->getLevel();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::HRule: {
         OS << "HRule";
         break;
      }
      case polar::markup::AstNodeKind::Text: {
         auto T = cast<Text>(Node);
         OS << "Text: Content=";
         simpleEscapingPrint(T->getLiteralContent(), OS);
         break;
      }
      case polar::markup::AstNodeKind::SoftBreak: {
         OS << "SoftBreak";
         break;
      }
      case polar::markup::AstNodeKind::LineBreak: {
         OS << "LineBreak";
         break;
      }
      case polar::markup::AstNodeKind::CodeBlock: {
         auto CB = cast<CodeBlock>(Node);
         OS << "CodeBlock: ";
         OS << "Language=";
         simpleEscapingPrint(CB->getLanguage(), OS);
         OS << " Content=";
         simpleEscapingPrint(CB->getLiteralContent(), OS);
         break;
      }
      case polar::markup::AstNodeKind::Code: {
         auto C = cast<Code>(Node);
         OS << "Code: Content=\"";
         simpleEscapingPrint(C->getLiteralContent(), OS);
         OS << "\"";
         break;
      }
      case polar::markup::AstNodeKind::Strong: {
         OS << "Strong: Children=" << Node->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::Emphasis: {
         OS << "Emphasis: Children=" << Node->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::Link: {
         auto L = cast<Link>(Node);
         OS << "Link: Destination=";
         simpleEscapingPrint(L->getDestination(), OS);
         OS << ' ' << "Children=" << L->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::Image: {
         auto I = cast<Image>(Node);
         OS << "Image: Destination=";
         simpleEscapingPrint(I->getDestination(), OS);
         OS << ' ' << "Children=" << I->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }
      case polar::markup::AstNodeKind::ParamField: {
         auto PF = cast<ParamField>(Node);
         OS << "ParamField: Name=";
         simpleEscapingPrint(PF->getName(), OS);
         OS << " Children=" << PF->getChildren().size();
         dumpChildren(Node->getChildren(), OS, indent + 1);
         break;
      }

#define MARKUP_SIMPLE_FIELD(Id, Keyword, XMLKind) \
  case polar::markup::AstNodeKind::Id: { \
    auto Field = cast<Id>(Node); \
    OS << #Id << ": Children=" << Field->getChildren().size(); \
    dumpChildren(Node->getChildren(), OS, indent + 1); \
    break; \
  }
#include "polarphp/markup/SimpleFieldsDefs.h"

      default:
         llvm_unreachable("Can't dump Markup AST Node: unknown node kind");
   }
   OS << ')';
}
