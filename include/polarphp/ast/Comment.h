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

#ifndef POLARPHP_AST_COMMENT_H
#define POLARPHP_AST_COMMENT_H

#include "polarphp/markup/Markup.h"
#include <optional>

namespace polar::ast {

class Decl;
class DocComment;
struct RawComment;

using polar::markup::Document;
using polar::markup::CommentParts;
using polar::markup::MarkupAstNode;
using polar::markup::MarkupContext;
using polar::basic::ArrayRef;
using polar::basic::StringRef;

class DocComment
{
   const Decl *m_decl;
   const Document *m_doc = nullptr;
   const CommentParts m_parts;

public:
   DocComment(const Decl *decl, Document *doc,
              CommentParts parts)
      : m_decl(decl),
        m_doc(doc),
        m_parts(parts)
   {}

   const Decl *getDecl() const
   {
      return m_decl;
   }

   const Document *getDocument() const
   {
      return m_doc;
   }

   CommentParts getParts() const
   {
      return m_parts;
   }

   ArrayRef<StringRef> getTags() const
   {
      return polar::basic::make_array_ref(m_parts.Tags.begin(), m_parts.Tags.end());
   }

//   std::optional<const Paragraph *> getBrief() const
//   {
//      return m_parts.brief;
//   }

//   std::optional<const ReturnsField * >getReturnsField() const
//   {
//      return m_parts.returnsField;
//   }

//   std::optional<const ThrowsField*> getThrowsField() const
//   {
//      return m_parts.throwsField;
//   }

//   ArrayRef<const ParamField *> getParamFields() const
//   {
//      return m_parts.paramFields;
//   }

   ArrayRef<const MarkupAstNode *> getBodyNodes() const
   {
      return m_parts.bodyNodes;
   }

   std::optional<const markup::LocalizationKeyField *>
   getLocalizationKeyField() const
   {
      return m_parts.localizationKeyField;
   }

   bool isEmpty() const
   {
      return m_parts.isEmpty();
   }

   // Only allow allocation using the allocator in MarkupContext or by
   // placement new.
   void *operator new(size_t bytes, MarkupContext &mcontext,
                      unsigned alignment = alignof(DocComment));
   void *operator new(size_t bytes, void *mem)
   {
      assert(mem);
      return mem;
   }

   // Make vanilla new/delete illegal.
   void *operator new(size_t bytes) = delete;
   void operator delete(void *data) = delete;
};

/// Get a parsed documentation comment for the declaration, if there is one.
std::optional<DocComment *> get_single_doc_comment(MarkupContext &context,
                                                   const Decl *decl);

/// Attempt to get a doc comment from the declaration, or other inherited
/// sources, like from base classes or protocols.
std::optional<DocComment *> get_cascading_doc_comment(MarkupContext &mcontext,
                                                      const Decl *decl);

/// Extract comments parts from the given Markup node.
CommentParts extract_comment_parts(MarkupContext &mcontext,
                                   MarkupAstNode *node);

} // polar::ast

#endif // POLARPHP_AST_COMMENT_H
