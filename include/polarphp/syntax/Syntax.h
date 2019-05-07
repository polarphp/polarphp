//===--- Syntax.h - Swift Syntax Interface ----------------------*- C++ -*-===//
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
// This file defines the Syntax type, the main public-facing classes and
// subclasses for dealing with polarphp Syntax.
//
// Syntax types contain a strong reference to the root of the tree to keep
// the subtree above alive, and a weak reference to the data representing
// the syntax node (weak to prevent retain cycles). All significant public API
// are contained in Syntax and its subclasses.
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
// Created by polarboy on 2019/05/07.

#ifndef POLARPHP_SYNTAX_SYNTAX_H
#define POLARPHP_SYNTAX_SYNTAX_H

#include "polarphp/syntax/SyntaxData.h"
#include "polarphp/syntax/References.h"
#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "polarphp/utils/RawOutStream.h"

#include <optional>

namespace polar::syntax {

class SyntaxAstMap;
struct SyntaxVisitor;
class SourceFileSyntax;

template <typename SyntaxNode>
SyntaxNode make(RefCountPtr<RawSyntax> raw)
{
   auto data = SyntaxData::make(raw);
   return { data, data.get() };
}

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_H
