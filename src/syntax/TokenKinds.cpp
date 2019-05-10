// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/10.

#include "polarphp/syntax/TokenKinds.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar::syntax {

bool is_token_text_determined(TokenKindType kind)
{
   auto entry = find_token_desc_entry(kind);
   if (entry == token_desc_map_end()) {
      return false;
   }
   return true;
}

StringRef get_token_text(TokenKindType kind)
{
   auto entry = find_token_desc_entry(kind);
   assert(entry != token_desc_map_end() && "token kind cannot be determined");
   return std::get<1>(entry->second);
}

void dump_token_kind(RawOutStream &outStream, TokenKindType kind)
{
   auto entry = find_token_desc_entry(kind);
   assert(entry != token_desc_map_end() && "token kind cannot be determined");
   outStream << std::get<0>(entry->second);
}

} // polar::syntax
