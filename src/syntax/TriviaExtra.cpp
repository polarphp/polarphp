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

#include "polarphp/syntax/Trivia.h"

namespace polar::syntax {

namespace {
static const std::map<TriviaKind, std::string> scg_triviaKindTable {
};
} // anonymous namespace

StringRef retrieve_trivia_kind_name(TriviaKind kind)
{
   auto iter = scg_triviaKindTable.find(kind);
   if (iter == scg_triviaKindTable.end()) {
      return StringRef();
   }
   return iter->second;
}

} // polar::syntax
