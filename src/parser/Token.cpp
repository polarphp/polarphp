// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/07/09.

#include "polarphp/parser/Token.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/syntax/TokenKinds.h"

namespace polar::parser {

using namespace polar::syntax;

void Token::dump() const
{
   dump(llvm::errs());
   llvm::errs() << "\n";
}

/// Dump this piece of syntax recursively.
void Token::dump(raw_ostream &outStream) const
{
   outStream << "============================" << "\n";
   outStream << "name: ";
   dump_token_kind(outStream, m_kind);
   outStream << "\n";
   if (is_keyword_token(m_kind) || is_punctuator_token(m_kind)) {
      outStream << "text: " << getLexicalText() << "\n";
   }
   if (!isInvalidLexValue()) {
      if (m_kind == TokenKindType::T_VARIABLE ||
          m_kind == TokenKindType::T_IDENTIFIER_STRING ||
          m_kind == TokenKindType::T_STRING_VARNAME) {
         outStream << "value: ";
         if (m_kind == TokenKindType::T_VARIABLE){
            outStream << '$';
         }
         outStream << getValue<std::string>() << "\n";
      } else if (m_kind == TokenKindType::T_LNUMBER) {
         outStream << "value: ";
         outStream << getValue<std::int64_t>() << "\n";
      } else if (m_kind == TokenKindType::T_DNUMBER) {
         outStream << "value: ";
         outStream << getValue<double>() << "\n";
      } else if (m_kind == TokenKindType::T_CONSTANT_ENCAPSED_STRING ||
                 m_kind == TokenKindType::T_ENCAPSED_AND_WHITESPACE) {
         const std::string &text = getValue<std::string>();
         outStream << "length: " << text.size() << "\n";
         outStream << "value: " << text << "\n";
      } else if (m_kind == TokenKindType::T_ERROR && hasValue()) {
         outStream << "error: ";
         outStream << getValue<std::string>() << "\n";
      }
   } else {
      outStream << "value: invalid lex value" << "\n";
   }
}

} // polar::parser
