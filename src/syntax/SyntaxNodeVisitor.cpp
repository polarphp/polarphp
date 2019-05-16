//// Do Not Edit Directly!
//===------------- SyntaxVisitor.cpp - Syntax Visitor definitions ---------===//
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
// Created by polarboy on 2019/05/12.

#include "polarphp/syntax/SyntaxVisitor.h"
#include "polarphp/basic/Defer.h"

namespace polar::syntax {

void SyntaxVisitor::visit(Syntax node)
{
  visitPre(node);
  POLAR_DEFER { visitPost(node); };
  switch (node.getKind()) {
  case SyntaxKind::Token:
    visit(node.castTo<TokenSyntax>());
    return;
  default:
    visitChildren(node);
    return;
  }
}

void Syntax::accept(SyntaxVisitor &visitor)
{
   visitor.visit(*this);
}

} // polar::syntax
