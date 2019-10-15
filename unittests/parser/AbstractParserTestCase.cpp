// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/07/24.

#include "AbstractParserTestCase.h"
#include "polarphp/parser/Parser.h"

namespace polar::unittest {

using polar::parser::Parser;

RefCountPtr<RawSyntax> AbstractParserTestCase::parseSource(StringRef source)
{
   unsigned bufferId = m_sourceMgr.addMemBufferCopy(source);
   Parser parser(m_langOpts, bufferId, m_sourceMgr, nullptr);
   parser.parse();
   return parser.getSyntaxTree();
}

AbstractParserTestCase::~AbstractParserTestCase()
{
}

} // polar::uittest
