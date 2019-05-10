//===--- UnknownSyntax.h - Swift Unknown Syntax Interface -------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/07.

#ifndef POLARPHP_SYNTAX_UNKNOWNSYNTAX_H
#define POLARPHP_SYNTAX_UNKNOWNSYNTAX_H

#include "polarphp/syntax/SyntaxData.h"
#include "polarphp/syntax/Syntax.h"

namespace polar::syntax {

#pragma mark unknown-syntax API

/// A chunk of "unknown" syntax.
///
/// Effectively wraps a tree of RawSyntax.
///
/// This should not be vended by SyntaxFactory.
class UnknownSyntax : public Syntax
{
   void validate() const;
public:
   UnknownSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool classof(const Syntax *syntax)
   {
      return syntax->isUnknown();
   }
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_UNKNOWNSYNTAX_H
