// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/19.

#include "polarphp/parser/ParsedAbstractSyntaxNodeRecorder.h"

namespace polar::parser {

bool AbstractSyntaxNodeRecorder::formExactLayoutFor(syntax::SyntaxKind kind, ArrayRef<ParsedRawSyntaxNode> elements,
                                                    FunctionRef<void (syntax::SyntaxKind, ArrayRef<ParsedRawSyntaxNode>)> receiver)
{
   return false;
}

} // polar::parser
