//===------ InlinableText.h - Extract inlinable source text -----*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_INTERNAL_INLINABLETEXT_H
#define POLARPHP_AST_INTERNAL_INLINABLETEXT_H

#include "polarphp/ast/AstNode.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"

namespace polar::basic {
class SourceManager;
} // polar::basic

namespace polar::ast {

using polar::basic::SourceManager;

/// Extracts the text of this ASTNode from the source buffer, ignoring
/// all #if declarations and clauses except the elements that are active.
StringRef extractInlinableText(SourceManager &sourceMgr, AstNode node,
                               SmallVectorImpl<char> &scratch);

} // end namespace polar::ast

#endif // POLARPHP_AST_INTERNAL_INLINABLETEXT_H
