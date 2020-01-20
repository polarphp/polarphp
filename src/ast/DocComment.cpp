////===--- DocComment.cpp - Extraction of doc comments ----------------------===//
////
//// This source file is part of the Swift.org open source project
////
//// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
//// Licensed under Apache License v2.0 with Runtime Library Exception
////
//// See https://swift.org/LICENSE.txt for license information
//// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
////
////===----------------------------------------------------------------------===//
/////
///// \file
///// This file implements extraction of documentation comments from a Swift
///// Markup AST tree.
/////
////===----------------------------------------------------------------------===//
//
//#include "polarphp/ast/AstContext.h"
//#include "polarphp/ast/Comment.h"
//#include "polarphp/ast/Decl.h"
//#include "polarphp/ast/Types.h"
//#include "polarphp/ast/PrettyStackTrace.h"
//#include "polarphp/ast/RawComment.h"
//#include "polarphp/markup/Markup.h"
//#include <queue>
//
//namespace polar {
//
//void *DocComment::operator new(size_t Bytes, polar::markup::MarkupContext &MC,
//                               unsigned Alignment) {
//  return MC.allocate(Bytes, Alignment);
//}
//
//namespace {
//Optional<polar::markup::ParamField *> extractParamOutlineItem(
//    polar::markup::MarkupContext &MC,
//    polar::markup::MarkupAstNode *Node) {
//
//  auto Item = dyn_cast<polar::markup::Item>(Node);
//  if (!Item)
//    return None;
//
//  auto Children = Item->getChildren();
//  if (Children.empty())
//    return None;
//
//  auto FirstChild = Children.front();
//  auto FirstParagraph = dyn_cast<polar::markup::Paragraph>(FirstChild);
//  if (!FirstParagraph)
//    return None;
//
//  auto FirstParagraphChildren = FirstParagraph->getChildren();
//  if (FirstParagraphChildren.empty())
//    return None;
//
//  auto ParagraphText =
//      dyn_cast<polar::markup::Text>(FirstParagraphChildren.front());
//  if (!ParagraphText)
//    return None;
//
//  StringRef Name;
//  StringRef Remainder;
//  std::tie(Name, Remainder) = ParagraphText->getLiteralContent().split(':');
//  Name = Name.rtrim();
//
//  if (Name.empty())
//    return None;
//
//  ParagraphText->setLiteralContent(Remainder.ltrim());
//
//  return polar::markup::ParamField::create(MC, Name, Children);
//}
//
//bool extractParameterOutline(
//    polar::markup::MarkupContext &MC, polar::markup::List *L,
//    SmallVectorImpl<polar::markup::ParamField *> &ParamFields) {
//  SmallVector<polar::markup::MarkupAstNode *, 8> NormalItems;
//  auto Children = L->getChildren();
//  if (Children.empty())
//    return false;
//
//  for (auto Child : Children) {
//    auto I = dyn_cast<polar::markup::Item>(Child);
//    if (!I) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto ItemChildren = I->getChildren();
//    if (ItemChildren.empty()) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto FirstChild = ItemChildren.front();
//    auto FirstParagraph = dyn_cast<polar::markup::Paragraph>(FirstChild);
//    if (!FirstParagraph) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto FirstParagraphChildren = FirstParagraph->getChildren();
//    if (FirstParagraphChildren.empty()) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto HeadingText
//        = dyn_cast<polar::markup::Text>(FirstParagraphChildren.front());
//    if (!HeadingText) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto HeadingContent = HeadingText->getLiteralContent();
//    if (!HeadingContent.rtrim().equals_lower("parameters:")) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto Rest = ArrayRef<polar::markup::MarkupAstNode *>(
//        ItemChildren.begin() + 1, ItemChildren.end());
//    if (Rest.empty()) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    for (auto Child : Rest) {
//      auto SubList = dyn_cast<polar::markup::List>(Child);
//      if (!SubList)
//        continue;
//
//      for (auto SubListChild : SubList->getChildren()) {
//        auto Param = extractParamOutlineItem(MC, SubListChild);
//        if (Param.hasValue()) {
//          ParamFields.push_back(Param.getValue());
//        }
//      }
//    }
//  }
//
//  if (NormalItems.size() != Children.size()) {
//    L->setChildren(NormalItems);
//  }
//
//  return NormalItems.empty();
//}
//
//bool extractSeparatedParams(
//    polar::markup::MarkupContext &MC, polar::markup::List *L,
//    SmallVectorImpl<polar::markup::ParamField *> &ParamFields) {
//  SmallVector<polar::markup::MarkupAstNode *, 8> NormalItems;
//  auto Children = L->getChildren();
//
//  for (auto Child : Children) {
//    auto I = dyn_cast<polar::markup::Item>(Child);
//    if (!I) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto ItemChildren = I->getChildren();
//    if (ItemChildren.empty()) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto FirstChild = ItemChildren.front();
//    auto FirstParagraph = dyn_cast<polar::markup::Paragraph>(FirstChild);
//    if (!FirstParagraph) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto FirstParagraphChildren = FirstParagraph->getChildren();
//    if (FirstParagraphChildren.empty()) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto ParagraphText
//        = dyn_cast<polar::markup::Text>(FirstParagraphChildren.front());
//    if (!ParagraphText) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    StringRef ParameterPrefix("parameter ");
//    auto ParagraphContent = ParagraphText->getLiteralContent();
//    auto PotentialMatch = ParagraphContent.substr(0, ParameterPrefix.size());
//
//    if (!PotentialMatch.startswith_lower(ParameterPrefix)) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    ParagraphContent = ParagraphContent.substr(ParameterPrefix.size());
//    ParagraphText->setLiteralContent(ParagraphContent.ltrim());
//
//    auto ParamField = extractParamOutlineItem(MC, I);
//    if (ParamField.hasValue())
//      ParamFields.push_back(ParamField.getValue());
//    else
//      NormalItems.push_back(Child);
//  }
//
//  if (NormalItems.size() != Children.size())
//    L->setChildren(NormalItems);
//
//  return NormalItems.empty();
//}
//
//bool extractSimpleField(
//    polar::markup::MarkupContext &MC, polar::markup::List *L,
//    polar::markup::CommentParts &Parts,
//    SmallVectorImpl<const polar::markup::MarkupAstNode *> &BodyNodes) {
//  auto Children = L->getChildren();
//  SmallVector<polar::markup::MarkupAstNode *, 8> NormalItems;
//  for (auto Child : Children) {
//    auto I = dyn_cast<polar::markup::Item>(Child);
//    if (!I) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto ItemChildren = I->getChildren();
//    if (ItemChildren.empty()) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto FirstParagraph
//        = dyn_cast<polar::markup::Paragraph>(ItemChildren.front());
//    if (!FirstParagraph) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto ParagraphChildren = FirstParagraph->getChildren();
//    if (ParagraphChildren.empty()) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    auto ParagraphText
//        = dyn_cast<polar::markup::Text>(ParagraphChildren.front());
//    if (!ParagraphText) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    StringRef Tag;
//    StringRef Remainder;
//    std::tie(Tag, Remainder) = ParagraphText->getLiteralContent().split(':');
//    Tag = Tag.ltrim().rtrim();
//    Remainder = Remainder.ltrim();
//
//    if (!polar::markup::isAFieldTag(Tag)) {
//      NormalItems.push_back(Child);
//      continue;
//    }
//
//    ParagraphText->setLiteralContent(Remainder);
//    auto Field = polar::markup::createSimpleField(MC, Tag, ItemChildren);
//
//    if (auto RF = dyn_cast<polar::markup::ReturnsField>(Field)) {
//      Parts.returnsField = RF;
//    } else if (auto TF = dyn_cast<polar::markup::ThrowsField>(Field)) {
//      Parts.throwsField = TF;
//    } else if (auto TF = dyn_cast<polar::markup::TagField>(Field)) {
//      llvm::SmallString<64> Scratch;
//      llvm::raw_svector_ostream OS(Scratch);
//      printInlinesUnder(TF, OS);
//      Parts.tags.insert(MC.allocateCopy(OS.str()));
//    } else if (auto LKF = dyn_cast<markup::LocalizationKeyField>(Field)) {
//      Parts.localizationKeyField = LKF;
//    } else {
//      BodyNodes.push_back(Field);
//    }
//  }
//
//  if (NormalItems.size() != Children.size())
//    L->setChildren(NormalItems);
//
//  return NormalItems.empty();
//}
//} // namespace
//
//void printBriefComment(RawComment RC, llvm::raw_ostream &OS) {
//  markup::MarkupContext MC;
//  markup::LineList LL = MC.getLineList(RC);
//  auto *markupDoc = markup::parseDocument(MC, LL);
//
//  auto children = markupDoc->getChildren();
//  if (children.empty())
//    return;
//  auto FirstParagraph = dyn_cast<polar::markup::Paragraph>(children.front());
//  if (!FirstParagraph)
//    return;
//  polar::markup::printInlinesUnder(FirstParagraph, OS);
//}
//
//polar::markup::CommentParts extractCommentParts(polar::markup::MarkupContext &MC,
//                    polar::markup::MarkupAstNode *Node) {
//
//  polar::markup::CommentParts Parts;
//  auto Children = Node->getChildren();
//  if (Children.empty())
//    return Parts;
//
//  auto FirstParagraph
//      = dyn_cast<polar::markup::Paragraph>(Node->getChildren().front());
//  if (FirstParagraph)
//    Parts.brief = FirstParagraph;
//
//  SmallVector<const polar::markup::MarkupAstNode *, 4> BodyNodes;
//  SmallVector<polar::markup::ParamField *, 8> ParamFields;
//
//  // Look for special top-level lists
//  size_t StartOffset = FirstParagraph == nullptr ? 0 : 1;
//  for (auto C = Children.begin() + StartOffset; C != Children.end(); ++C) {
//    if (auto L = dyn_cast<polar::markup::List>(*C)) {
//      // Could be one of the following:
//      // 1. A parameter outline:
//      //    - Parameters:
//      //      - x: ...
//      //      - y: ...
//      // 2. An exploded parameter list:
//      //    - parameter x: ...
//      //    - parameter y: ...
//      // 3. Some other simple field, including "returns" (see SimpleFields.def)
//      auto ListNowEmpty = extractParameterOutline(MC, L, ParamFields);
//      ListNowEmpty |= extractSeparatedParams(MC, L, ParamFields);
//      ListNowEmpty |= extractSimpleField(MC, L, Parts, BodyNodes);
//      if (ListNowEmpty)
//        continue; // This drops the empty list from the doc comment body.
//    }
//    BodyNodes.push_back(*C);
//  }
//
//  // Copy BodyNodes and ParamFields into the MarkupContext.
//  Parts.bodyNodes = MC.allocateCopy(llvm::makeArrayRef(BodyNodes));
//  Parts.paramFields = MC.allocateCopy(llvm::makeArrayRef(ParamFields));
//
//  for (auto Param : Parts.paramFields) {
//    auto ParamParts = extractCommentParts(MC, Param);
//    Param->setParts(ParamParts);
//  }
//
//  return Parts;
//}
//
//DocComment *DocComment::create(const Decl *D, markup::MarkupContext &MC,
//                               RawComment RC) {
//  assert(!RC.isEmpty());
//  polar::markup::LineList LL = MC.getLineList(RC);
//  auto *Doc = polar::markup::parseDocument(MC, LL);
//  auto Parts = extractCommentParts(MC, Doc);
//  return new (MC) DocComment(D, Doc, Parts);
//}
//
//void DocComment::addInheritanceNote(polar::markup::MarkupContext &MC,
//                                    TypeDecl *base) {
//  auto text = MC.allocateCopy("This documentation comment was inherited from ");
//  auto name = MC.allocateCopy(base->getNameStr());
//  auto period = MC.allocateCopy(".");
//  auto paragraph = markup::Paragraph::create(MC, {
//    markup::Text::create(MC, text),
//    markup::Code::create(MC, name),
//    markup::Text::create(MC, period)});
//
//  auto note = markup::NoteField::create(MC, {paragraph});
//
//  SmallVector<const markup::MarkupAstNode *, 8> BodyNodes{
//    Parts.bodyNodes.begin(), Parts.bodyNodes.end()};
//  BodyNodes.push_back(note);
//  Parts.bodyNodes = MC.allocateCopy(llvm::makeArrayRef(BodyNodes));
//}
//
//DocComment *getSingleDocComment(polar::markup::MarkupContext &MC,
//                                       const Decl *D) {
//  PrettyStackTraceDecl StackTrace("parsing comment for", D);
//
//  auto RC = D->getRawComment();
//  if (RC.isEmpty())
//    return nullptr;
//  return DocComment::create(D, MC, RC);
//}
//
//namespace {
//const ValueDecl *findOverriddenDeclWithDocComment(const ValueDecl *VD) {
//  // Only applies to class member.
//  if (!VD->getDeclContext()->getSelfClassDecl())
//    return nullptr;
//
//  while (auto *baseDecl = VD->getOverriddenDecl()) {
//    if (!baseDecl->getRawComment().isEmpty())
//      return baseDecl;
//    VD = baseDecl;
//  }
//
//  return nullptr;
//}
//
//const ValueDecl *findDefaultProvidedDeclWithDocComment(const ValueDecl *VD) {
//  auto protocol = VD->getDeclContext()->getExtendedInterfaceDecl();
//  // Only applies to protocol extension member.
//  if (!protocol)
//    return nullptr;
//
//  ValueDecl *requirement = nullptr;
//
//  SmallVector<ValueDecl *, 2> members;
//  protocol->lookupQualified(const_cast<InterfaceDecl *>(protocol),
//                            VD->getFullName(), NLOptions::NL_InterfaceMembers,
//                            members);
//
//  for (auto *member : members) {
//    if (!isa<InterfaceDecl>(member->getDeclContext()) ||
//        !member->isInterfaceRequirement() ||
//        member->getRawComment().isEmpty())
//      continue;
//    if (requirement)
//      // Found two or more decls with doc-comment.
//      return nullptr;
//
//    requirement = member;
//  }
//  return requirement;
//}
//
//const ValueDecl *findRequirementDeclWithDocComment(const ValueDecl *VD) {
//  std::queue<const ValueDecl *> requirements;
//  while (true) {
//    for (auto *req : VD->getSatisfiedInterfaceRequirements()) {
//      if (!req->getRawComment().isEmpty())
//        return req;
//      else
//        requirements.push(req);
//    }
//    if (requirements.empty())
//      return nullptr;
//    VD = requirements.front();
//    requirements.pop();
//  }
//}
//} // namespace
//
//const Decl *getDocCommentProvidingDecl(const Decl *D) {
//  if (!D->canHaveComment())
//    return nullptr;
//
//  if (!D->getRawComment().isEmpty())
//    return D;
//
//  auto *VD = dyn_cast<ValueDecl>(D);
//  if (!VD)
//    return nullptr;
//
//  if (auto *overriden = findOverriddenDeclWithDocComment(VD))
//    return overriden;
//
//  if (auto *requirement = findDefaultProvidedDeclWithDocComment(VD))
//    return requirement;
//
//  if (auto *requirement = findRequirementDeclWithDocComment(VD))
//    return requirement;
//
//  return nullptr;
//}
//
//DocComment *getCascadingDocComment(polar::markup::MarkupContext &MC, const Decl *D) {
//  auto *docD = getDocCommentProvidingDecl(D);
//  if (!docD)
//    return nullptr;
//
//  auto *doc = getSingleDocComment(MC, docD);
//  assert(doc && "getDocCommentProvidingDecl() returned decl with no comment");
//
//  // If the doc-comment is inherited from other decl, add a note about it.
//  if (docD != D) {
//    doc->setDecl(D);
//    if (auto baseD = docD->getDeclContext()->getSelfNominalTypeDecl()) {
//      doc->addInheritanceNote(MC, baseD);
//
//      // If the doc is inherited from protocol requirement, associate the
//      // requirement with the doc-comment.
//      // FIXME: This is to keep the old behavior.
//      if (isa<InterfaceDecl>(baseD))
//        doc->setDecl(docD);
//    }
//  }
//
//  return doc;
//}
//
//StringRef Decl::getBriefComment() const {
//  if (!this->canHaveComment())
//    return StringRef();
//
//  // Ensure the serialized doc is populated to AstContext.
//  auto RC = getRawComment();
//
//  // Check the cache in AstContext.
//  auto &Context = getAstContext();
//  if (Optional<StringRef> Comment = Context.getBriefComment(this))
//    return Comment.getValue();
//
//  StringRef Result;
//  if (RC.isEmpty())
//    if (auto *docD = getDocCommentProvidingDecl(this))
//      RC = docD->getRawComment();
//  if (!RC.isEmpty()) {
//    SmallString<256> BriefStr;
//    llvm::raw_svector_ostream OS(BriefStr);
//    printBriefComment(RC, OS);
//    Result = Context.AllocateCopy(BriefStr.str());
//  }
//
//  // Cache it.
//  Context.setBriefComment(this, Result);
//  return Result;
//}
//
//} // polar