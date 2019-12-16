//===--- Comment.h - Swift-specific comment parsing -------------*- C++ -*-===//
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

#ifndef POLARPHP_AST_COMMENT_H
#define POLARPHP_AST_COMMENT_H

#include "polarphp/markup/Markup.h"
#include "llvm/ADT/Optional.h"

namespace polar {
class Decl;
class TypeDecl;
struct RawComment;
using llvm::StringRef;
using llvm::ArrayRef;
using llvm::Optional;

class DocComment {
  const Decl *D;
  polar::markup::Document *Doc = nullptr;
   polar::markup::CommentParts Parts;

  DocComment(const Decl *D, polar::markup::Document *Doc,
             polar::markup::CommentParts Parts)
      : D(D), Doc(Doc), Parts(Parts) {}

public:
  static DocComment *create(const Decl *D, polar::markup::MarkupContext &MC,
                            RawComment RC);

  void addInheritanceNote(polar::markup::MarkupContext &MC, TypeDecl *base);

  const Decl *getDecl() const { return D; }
  void setDecl(const Decl *D) { this->D = D; }

  const polar::markup::Document *getDocument() const { return Doc; }

  polar::markup::CommentParts getParts() const {
    return Parts;
  }

  ArrayRef<StringRef> getTags() const {
    return llvm::makeArrayRef(Parts.tags.begin(), Parts.tags.end());
  }

  Optional<const polar::markup::Paragraph *> getBrief() const {
    return Parts.brief;
  }

  Optional<const polar::markup::ReturnsField * >getReturnsField() const {
    return Parts.returnsField;
  }

  Optional<const polar::markup::ThrowsField*> getThrowsField() const {
    return Parts.throwsField;
  }

  ArrayRef<const polar::markup::ParamField *> getParamFields() const {
    return Parts.paramFields;
  }

  ArrayRef<const polar::markup::MarkupAstNode *> getBodyNodes() const {
    return Parts.bodyNodes;
  }

  Optional<const markup::LocalizationKeyField *>
  getLocalizationKeyField() const {
    return Parts.localizationKeyField;
  }

  bool isEmpty() const {
    return Parts.isEmpty();
  }

  // Only allow allocation using the allocator in MarkupContext or by
  // placement new.
  void *operator new(size_t Bytes, polar::markup::MarkupContext &MC,
                     unsigned Alignment = alignof(DocComment));
  void *operator new(size_t Bytes, void *Mem) {
    assert(Mem);
    return Mem;
  }

  // Make vanilla new/delete illegal.
  void *operator new(size_t Bytes) = delete;
  void operator delete(void *Data) = delete;
};

/// Get a parsed documentation comment for the declaration, if there is one.
DocComment *getSingleDocComment(polar::markup::MarkupContext &Context,
                                const Decl *D);

const Decl *getDocCommentProvidingDecl(const Decl *D);

/// Attempt to get a doc comment from the declaration, or other inherited
/// sources, like from base classes or protocols.
DocComment *getCascadingDocComment(polar::markup::MarkupContext &MC,
                                   const Decl *D);

/// Extract comments parts from the given Markup node.
polar::markup::CommentParts
extractCommentParts(polar::markup::MarkupContext &MC,
                    polar::markup::MarkupAstNode *Node);

/// Extract brief comment from \p RC, and print it to \p OS .
void printBriefComment(RawComment RC, llvm::raw_ostream &OS);

} // namespace polar

#endif // POLARPHP_AST_COMMENT_H
