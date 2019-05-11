// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/11.

#include "polarphp/syntax/SyntaxKind.h"

namespace polar::syntax {

static const std::map<SyntaxKind, std::tuple<std::string, int>> scg_syntaxKindTable {
};

StringRef retrieve_syntax_kind_text(SyntaxKind kind)
{
   auto iter = scg_syntaxKindTable.find(kind);
   if (iter == scg_syntaxKindTable.end()) {
      return StringRef();
   }
   return std::get<0>(iter->second);
}

int retrieve_syntax_kind_serialization_code(SyntaxKind kind)
{
   auto iter = scg_syntaxKindTable.find(kind);
   if (iter == scg_syntaxKindTable.end()) {
      return -1;
   }
   return std::get<1>(iter->second);
}

} // polar::syntax
