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

#ifndef POLARPHP_PARSER_PARSED_ABSTRACT_SYNTAX_NODE_RECORDER_H
#define POLARPHP_PARSER_PARSED_ABSTRACT_SYNTAX_NODE_RECORDER_H

#include "polarphp/syntax/SyntaxKind.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/parser/ParsedRawSyntaxNode.h"

namespace polar::parser {

using polar::syntax::SyntaxKind;
using polar::basic::ArrayRef;
using polar::basic::FunctionRef;

class AbstractSyntaxNodeRecorder
{
protected:

   /// The provided \c elements are in the appropriate order for the syntax
   /// \c kind's layout but optional elements are not be included.
   /// This function will form the exact layout based on the provided elements,
   /// substituting missing parts with a null ParsedRawSyntaxNode object.
   ///
   /// \returns true if the layout could be formed, false otherwise.
   static bool formExactLayoutFor(SyntaxKind kind,
                                  ArrayRef<ParsedRawSyntaxNode> elements,
                                  FunctionRef<void(SyntaxKind, ArrayRef<ParsedRawSyntaxNode>)> receiver);
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_ABSTRACT_SYNTAX_NODE_RECORDER_H
