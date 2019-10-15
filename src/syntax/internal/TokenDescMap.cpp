// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#include "polarphp/syntax/TokenKinds.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar::syntax {
namespace {

static const std::map<TokenKindType, TokenDescItemType> scg_tokenDescTable{
   {TokenKindType::END, {"END", "end of file", TokenCategory::Internal}},
      {TokenKindType::T_LINE, {"T_LINE", "__LINE__", TokenCategory::Keyword}},
      {TokenKindType::T_FILE, {"T_FILE", "__FILE__", TokenCategory::Keyword}},
      {TokenKindType::T_DIR, {"T_DIR", "__DIR__", TokenCategory::Keyword}},
      {TokenKindType::T_CLASS_CONST, {"T_CLASS_CONST", "__CLASS__", TokenCategory::Keyword}},
      {TokenKindType::T_TRAIT_CONST, {"T_TRAIT_CONST", "__TRAIT__", TokenCategory::Keyword}},
      {TokenKindType::T_METHOD_CONST, {"T_METHOD_CONST", "__METHOD__", TokenCategory::Keyword}},
      {TokenKindType::T_FUNC_CONST, {"T_FUNC_CONST", "__FUNCTION__", TokenCategory::Keyword}},
      {TokenKindType::T_NS_CONST, {"T_NS_CONST", "__NAMESPACE__", TokenCategory::Keyword}},
      {TokenKindType::T_NAMESPACE, {"T_NAMESPACE", "namespace", TokenCategory::DeclKeyword}},
      {TokenKindType::T_CLASS, {"T_CLASS", "class", TokenCategory::DeclKeyword}},
      {TokenKindType::T_TRAIT, {"T_TRAIT", "trait", TokenCategory::DeclKeyword}},
      {TokenKindType::T_INTERFACE, {"T_INTERFACE", "interface", TokenCategory::DeclKeyword}},
      {TokenKindType::T_EXTENDS, {"T_EXTENDS", "extends", TokenCategory::DeclKeyword}},
      {TokenKindType::T_IMPLEMENTS, {"T_IMPLEMENTS", "implements", TokenCategory::DeclKeyword}},
      {TokenKindType::T_FUNCTION, {"T_FUNCTION", "function", TokenCategory::DeclKeyword}},
      {TokenKindType::T_FN, {"T_FN", "fn", TokenCategory::DeclKeyword}},
      {TokenKindType::T_CONST, {"T_CONST", "const", TokenCategory::DeclKeyword}},
      {TokenKindType::T_VAR, {"T_VAR", "var", TokenCategory::DeclKeyword}},
      {TokenKindType::T_USE, {"T_USE", "use", TokenCategory::DeclKeyword}},
      {TokenKindType::T_INSTEADOF, {"T_INSTEADOF", "insteadof", TokenCategory::DeclKeyword}},
      {TokenKindType::T_AS, {"T_AS", "as", TokenCategory::DeclKeyword}},
      {TokenKindType::T_GLOBAL, {"T_GLOBAL", "global", TokenCategory::DeclKeyword}},
      {TokenKindType::T_STATIC, {"T_STATIC", "static", TokenCategory::DeclKeyword}},
      {TokenKindType::T_ABSTRACT, {"T_ABSTRACT", "abstract", TokenCategory::DeclKeyword}},
      {TokenKindType::T_FINAL, {"T_FINAL", "final", TokenCategory::DeclKeyword}},
      {TokenKindType::T_PRIVATE, {"T_PRIVATE", "private", TokenCategory::DeclKeyword}},
      {TokenKindType::T_PROTECTED, {"T_PROTECTED", "protected", TokenCategory::DeclKeyword}},
      {TokenKindType::T_PUBLIC, {"T_PUBLIC", "public", TokenCategory::DeclKeyword}},
      {TokenKindType::T_LIST, {"T_LIST", "list", TokenCategory::DeclKeyword}},
      {TokenKindType::T_ARRAY, {"T_ARRAY", "array", TokenCategory::DeclKeyword}},
      {TokenKindType::T_CALLABLE, {"T_CALLABLE", "callable", TokenCategory::DeclKeyword}},
      {TokenKindType::T_THREAD_LOCAL, {"T_THREAD_LOCAL", "thread_local", TokenCategory::DeclKeyword}},
      {TokenKindType::T_MODULE, {"T_MODULE", "module", TokenCategory::DeclKeyword}},
      {TokenKindType::T_PACKAGE, {"T_PACKAGE", "package", TokenCategory::DeclKeyword}},
      {TokenKindType::T_ASYNC, {"T_ASYNC", "async", TokenCategory::DeclKeyword}},
      {TokenKindType::T_EXPORT, {"T_EXPORT", "export", TokenCategory::DeclKeyword}},
      {TokenKindType::T_DEFER, {"T_DEFER", "defer", TokenCategory::StmtKeyword}},
      {TokenKindType::T_IF, {"T_IF", "if", TokenCategory::StmtKeyword}},
      {TokenKindType::T_ELSEIF, {"T_ELSEIF", "elseif", TokenCategory::StmtKeyword}},
      {TokenKindType::T_ELSE, {"T_ELSE", "else", TokenCategory::StmtKeyword}},
      {TokenKindType::T_ECHO, {"T_ECHO", "echo", TokenCategory::StmtKeyword}},
      {TokenKindType::T_DO, {"T_DO", "do", TokenCategory::StmtKeyword}},
      {TokenKindType::T_WHILE, {"T_WHILE", "while", TokenCategory::StmtKeyword}},
      {TokenKindType::T_FOR, {"T_FOR", "for", TokenCategory::StmtKeyword}},
      {TokenKindType::T_FOREACH, {"T_FOREACH", "foreach", TokenCategory::StmtKeyword}},
      {TokenKindType::T_SWITCH, {"T_SWITCH", "switch", TokenCategory::StmtKeyword}},
      {TokenKindType::T_CASE, {"T_CASE", "case", TokenCategory::StmtKeyword}},
      {TokenKindType::T_DEFAULT, {"T_DEFAULT", "default", TokenCategory::StmtKeyword}},
      {TokenKindType::T_BREAK, {"T_BREAK", "break", TokenCategory::StmtKeyword}},
      {TokenKindType::T_CONTINUE, {"T_CONTINUE", "continue", TokenCategory::StmtKeyword}},
      {TokenKindType::T_FALLTHROUGH, {"T_FALLTHROUGH", "fallthrough", TokenCategory::StmtKeyword}},
      {TokenKindType::T_GOTO, {"T_GOTO", "goto", TokenCategory::StmtKeyword}},
      {TokenKindType::T_RETURN, {"T_RETURN", "return", TokenCategory::StmtKeyword}},
      {TokenKindType::T_TRY, {"T_TRY", "try", TokenCategory::StmtKeyword}},
      {TokenKindType::T_CATCH, {"T_CATCH", "catch", TokenCategory::StmtKeyword}},
      {TokenKindType::T_FINALLY, {"T_FINALLY", "finally", TokenCategory::StmtKeyword}},
      {TokenKindType::T_THROW, {"T_THROW", "throw", TokenCategory::StmtKeyword}},
      {TokenKindType::T_UNSET, {"T_UNSET", "unset", TokenCategory::ExprKeyword}},
      {TokenKindType::T_ISSET, {"T_ISSET", "isset", TokenCategory::ExprKeyword}},
      {TokenKindType::T_EMPTY, {"T_EMPTY", "empty", TokenCategory::ExprKeyword}},
      {TokenKindType::T_HALT_COMPILER, {"T_HALT_COMPILER", "__halt_compiler", TokenCategory::ExprKeyword}},
      {TokenKindType::T_EVAL, {"T_EVAL", "eval", TokenCategory::ExprKeyword}},
      {TokenKindType::T_INCLUDE, {"T_INCLUDE", "include", TokenCategory::ExprKeyword}},
      {TokenKindType::T_INCLUDE_ONCE, {"T_INCLUDE_ONCE", "include_once", TokenCategory::ExprKeyword}},
      {TokenKindType::T_REQUIRE, {"T_REQUIRE", "require", TokenCategory::ExprKeyword}},
      {TokenKindType::T_REQUIRE_ONCE, {"T_REQUIRE_ONCE", "require_once", TokenCategory::ExprKeyword}},
      {TokenKindType::T_LOGICAL_OR, {"T_LOGICAL_OR", "or", TokenCategory::ExprKeyword}},
      {TokenKindType::T_LOGICAL_XOR, {"T_LOGICAL_XOR", "xor", TokenCategory::ExprKeyword}},
      {TokenKindType::T_LOGICAL_AND, {"T_LOGICAL_AND", "and", TokenCategory::ExprKeyword}},
      {TokenKindType::T_PRINT, {"T_PRINT", "print", TokenCategory::ExprKeyword}},
      {TokenKindType::T_YIELD, {"T_YIELD", "yield", TokenCategory::ExprKeyword}},
      {TokenKindType::T_YIELD_FROM, {"T_YIELD_FROM", "yield from", TokenCategory::ExprKeyword}},
      {TokenKindType::T_INSTANCEOF, {"T_INSTANCEOF", "instanceof", TokenCategory::ExprKeyword}},
      {TokenKindType::T_INT_CAST, {"T_INT_CAST", "", TokenCategory::ExprKeyword}},
      {TokenKindType::T_DOUBLE_CAST, {"T_DOUBLE_CAST", "", TokenCategory::ExprKeyword}},
      {TokenKindType::T_STRING_CAST, {"T_STRING_CAST", "", TokenCategory::ExprKeyword}},
      {TokenKindType::T_ARRAY_CAST, {"T_ARRAY_CAST", "", TokenCategory::ExprKeyword}},
      {TokenKindType::T_OBJECT_CAST, {"T_OBJECT_CAST", "", TokenCategory::ExprKeyword}},
      {TokenKindType::T_BOOL_CAST, {"T_BOOL_CAST", "", TokenCategory::ExprKeyword}},
      {TokenKindType::T_UNSET_CAST, {"T_UNSET_CAST", "", TokenCategory::ExprKeyword}},
      {TokenKindType::T_NEW, {"T_NEW", "new", TokenCategory::ExprKeyword}},
      {TokenKindType::T_CLONE, {"T_CLONE", "clone", TokenCategory::ExprKeyword}},
      {TokenKindType::T_EXIT, {"T_EXIT", "exit", TokenCategory::ExprKeyword}},
      {TokenKindType::T_DECLARE, {"T_DECLARE", "declare", TokenCategory::ExprKeyword}},
      {TokenKindType::T_CLASS_REF_STATIC, {"T_CLASS_REF_STATIC", "static", TokenCategory::ExprKeyword}},
      {TokenKindType::T_CLASS_REF_SELF, {"T_CLASS_REF_SELF", "self", TokenCategory::ExprKeyword}},
      {TokenKindType::T_CLASS_REF_PARENT, {"T_CLASS_REF_PARENT", "parent", TokenCategory::ExprKeyword}},
      {TokenKindType::T_OBJ_REF, {"T_OBJ_REF", "this", TokenCategory::ExprKeyword}},
      {TokenKindType::T_TRUE, {"T_TRUE", "true", TokenCategory::ExprKeyword}},
      {TokenKindType::T_FALSE, {"T_FALSE", "false", TokenCategory::ExprKeyword}},
      {TokenKindType::T_NULL, {"T_NULL", "null", TokenCategory::ExprKeyword}},
      {TokenKindType::T_AWAIT, {"T_AWAIT", "await", TokenCategory::ExprKeyword}},
      {TokenKindType::T_PLUS_SIGN, {"T_PLUS_SIGN", "+", TokenCategory::Punctuator}},
      {TokenKindType::T_MINUS_SIGN, {"T_MINUS_SIGN", "-", TokenCategory::Punctuator}},
      {TokenKindType::T_MUL_SIGN, {"T_MUL_SIGN", "*", TokenCategory::Punctuator}},
      {TokenKindType::T_DIV_SIGN, {"T_DIV_SIGN", "/", TokenCategory::Punctuator}},
      {TokenKindType::T_MOD_SIGN, {"T_MOD_SIGN", "%", TokenCategory::Punctuator}},
      {TokenKindType::T_EQUAL, {"T_EQUAL", "=", TokenCategory::Punctuator}},
      {TokenKindType::T_STR_CONCAT, {"T_STR_CONCAT", ".", TokenCategory::Punctuator}},
      {TokenKindType::T_PLUS_EQUAL, {"T_PLUS_EQUAL", "+=", TokenCategory::Punctuator}},
      {TokenKindType::T_MINUS_EQUAL, {"T_MINUS_EQUAL", "-=", TokenCategory::Punctuator}},
      {TokenKindType::T_MUL_EQUAL, {"T_MUL_EQUAL", "*=", TokenCategory::Punctuator}},
      {TokenKindType::T_DIV_EQUAL, {"T_DIV_EQUAL", "/=", TokenCategory::Punctuator}},
      {TokenKindType::T_STR_CONCAT_EQUAL, {"T_STR_CONCAT_EQUAL", ".=", TokenCategory::Punctuator}},
      {TokenKindType::T_MOD_EQUAL, {"T_MOD_EQUAL", "%=", TokenCategory::Punctuator}},
      {TokenKindType::T_AND_EQUAL, {"T_AND_EQUAL", "&=", TokenCategory::Punctuator}},
      {TokenKindType::T_OR_EQUAL, {"T_OR_EQUAL", "|=", TokenCategory::Punctuator}},
      {TokenKindType::T_XOR_EQUAL, {"T_XOR_EQUAL", "^=", TokenCategory::Punctuator}},
      {TokenKindType::T_SL_EQUAL, {"T_SL_EQUAL", "<<=", TokenCategory::Punctuator}},
      {TokenKindType::T_SR_EQUAL, {"T_SR_EQUAL", ">>=", TokenCategory::Punctuator}},
      {TokenKindType::T_COALESCE_EQUAL, {"T_COALESCE_EQUAL", "\?\?=", TokenCategory::Punctuator}},
      {TokenKindType::T_BOOLEAN_OR, {"T_BOOLEAN_OR", "||", TokenCategory::Punctuator}},
      {TokenKindType::T_BOOLEAN_AND, {"T_BOOLEAN_AND", "&&", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_EQUAL, {"T_IS_EQUAL", "==", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_NOT_EQUAL, {"T_IS_NOT_EQUAL", "!=", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_IDENTICAL, {"T_IS_IDENTICAL", "===", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_NOT_IDENTICAL, {"T_IS_NOT_IDENTICAL", "!==", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_SMALLER, {"T_IS_SMALLER", "<", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_SMALLER_OR_EQUAL, {"T_IS_SMALLER_OR_EQUAL", "<=", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_GREATER_OR_EQUAL, {"T_IS_GREATER_OR_EQUAL", ">=", TokenCategory::Punctuator}},
      {TokenKindType::T_IS_GREATER, {"T_IS_GREATER", ">", TokenCategory::Punctuator}},
      {TokenKindType::T_SPACESHIP, {"T_SPACESHIP", "", TokenCategory::Punctuator}},
      {TokenKindType::T_SL, {"T_SL", "<<", TokenCategory::Punctuator}},
      {TokenKindType::T_SR, {"T_SR", ">>", TokenCategory::Punctuator}},
      {TokenKindType::T_INC, {"T_INC", "++", TokenCategory::Punctuator}},
      {TokenKindType::T_DEC, {"T_DEC", "--", TokenCategory::Punctuator}},
      {TokenKindType::T_NS_SEPARATOR, {"T_NS_SEPARATOR", "\\", TokenCategory::Punctuator}},
      {TokenKindType::T_ELLIPSIS, {"T_ELLIPSIS", "...", TokenCategory::Punctuator}},
      {TokenKindType::T_COALESCE, {"T_COALESCE", "??", TokenCategory::Punctuator}},
      {TokenKindType::T_POW, {"T_POW", "**", TokenCategory::Punctuator}},
      {TokenKindType::T_POW_EQUAL, {"T_POW_EQUAL", "**=", TokenCategory::Punctuator}},
      {TokenKindType::T_OBJECT_OPERATOR, {"T_OBJECT_OPERATOR", "->", TokenCategory::Punctuator}},
      {TokenKindType::T_DOUBLE_ARROW, {"T_DOUBLE_ARROW", "=>", TokenCategory::Punctuator}},
      {TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, {"T_DOLLAR_OPEN_CURLY_BRACES", "${", TokenCategory::Punctuator}},
      {TokenKindType::T_CURLY_OPEN, {"T_CURLY_OPEN", "{$", TokenCategory::Punctuator}},
      {TokenKindType::T_PAAMAYIM_NEKUDOTAYIM, {"T_PAAMAYIM_NEKUDOTAYIM", "::", TokenCategory::Punctuator}},
      {TokenKindType::T_LEFT_PAREN, {"T_LEFT_PAREN", "", TokenCategory::Punctuator}},
      {TokenKindType::T_RIGHT_PAREN, {"T_RIGHT_PAREN", ")", TokenCategory::Punctuator}},
      {TokenKindType::T_LEFT_BRACE, {"T_LEFT_BRACE", "{", TokenCategory::Punctuator}},
      {TokenKindType::T_RIGHT_BRACE, {"T_RIGHT_BRACE", "}", TokenCategory::Punctuator}},
      {TokenKindType::T_LEFT_SQUARE_BRACKET, {"T_LEFT_SQUARE_BRACKET", "[", TokenCategory::Punctuator}},
      {TokenKindType::T_RIGHT_SQUARE_BRACKET, {"T_RIGHT_SQUARE_BRACKET", "]", TokenCategory::Punctuator}},
      {TokenKindType::T_LEFT_ANGLE, {"T_LEFT_ANGLE", "<", TokenCategory::Punctuator}},
      {TokenKindType::T_RIGHT_ANGLE, {"T_RIGHT_ANGLE", ">", TokenCategory::Punctuator}},
      {TokenKindType::T_COMMA, {"T_COMMA", ",", TokenCategory::Punctuator}},
      {TokenKindType::T_COLON, {"T_COLON", ":", TokenCategory::Punctuator}},
      {TokenKindType::T_SEMICOLON, {"T_SEMICOLON", ";", TokenCategory::Punctuator}},
      {TokenKindType::T_BACKTICK, {"T_BACKTICK", "`", TokenCategory::Punctuator}},
      {TokenKindType::T_SINGLE_QUOTE, {"T_SINGLE_QUOTE", "'", TokenCategory::Punctuator}},
      {TokenKindType::T_DOUBLE_QUOTE, {"T_DOUBLE_QUOTE", "\"", TokenCategory::Punctuator}},
      {TokenKindType::T_VBAR, {"T_VBAR", "|", TokenCategory::Punctuator}},
      {TokenKindType::T_CARET, {"T_CARET", "^", TokenCategory::Punctuator}},
      {TokenKindType::T_EXCLAMATION_MARK, {"T_EXCLAMATION_MARK", "!", TokenCategory::Punctuator}},
      {TokenKindType::T_TILDE, {"T_TILDE", "~", TokenCategory::Punctuator}},
      {TokenKindType::T_DOLLAR_SIGN, {"T_DOLLAR_SIGN", "$", TokenCategory::Punctuator}},
      {TokenKindType::T_QUESTION_MARK, {"T_QUESTION_MARK", "?", TokenCategory::Punctuator}},
      {TokenKindType::T_ERROR_SUPPRESS_SIGN, {"T_ERROR_SUPPRESS_SIGN", "@", TokenCategory::Punctuator}},
      {TokenKindType::T_AMPERSAND, {"T_AMPERSAND", "&", TokenCategory::Punctuator}},
      {TokenKindType::T_LNUMBER, {"T_LNUMBER", "integer number", TokenCategory::Misc}},
      {TokenKindType::T_DNUMBER, {"T_DNUMBER", "floating-point number", TokenCategory::Misc}},
      {TokenKindType::T_IDENTIFIER_STRING, {"T_IDENTIFIER_STRING", "identifier", TokenCategory::Misc}},
      {TokenKindType::T_VARIABLE, {"T_VARIABLE", "variable", TokenCategory::Misc}},
      {TokenKindType::T_ENCAPSED_AND_WHITESPACE, {"T_ENCAPSED_AND_WHITESPACE", "quoted-string and whitespace", TokenCategory::Misc}},
      {TokenKindType::T_CONSTANT_ENCAPSED_STRING, {"T_CONSTANT_ENCAPSED_STRING", "quoted-string", TokenCategory::Misc}},
      {TokenKindType::T_STRING_VARNAME, {"T_STRING_VARNAME", "variable name", TokenCategory::Misc}},
      {TokenKindType::T_NUM_STRING, {"T_NUM_STRING", "number", TokenCategory::Misc}},
      {TokenKindType::T_WHITESPACE, {"T_WHITESPACE", "whitespace", TokenCategory::Misc}},
      {TokenKindType::T_PREFIX_OPERATOR, {"T_PREFIX_OPERATOR", "prefix operator", TokenCategory::Misc}},
      {TokenKindType::T_POSTFIX_OPERATOR, {"T_POSTFIX_OPERATOR", "postfix operator", TokenCategory::Misc}},
      {TokenKindType::T_BINARY_OPERATOR, {"T_BINARY_OPERATOR", "binary operator", TokenCategory::Misc}},
      {TokenKindType::T_COMMENT, {"T_COMMENT", "comment", TokenCategory::Misc}},
      {TokenKindType::T_DOC_COMMENT, {"T_DOC_COMMENT", "doc comment", TokenCategory::Misc}},
      {TokenKindType::T_OPEN_TAG, {"T_OPEN_TAG", "open tag", TokenCategory::Misc}},
      {TokenKindType::T_OPEN_TAG_WITH_ECHO, {"T_OPEN_TAG_WITH_ECHO", "open tag with echo", TokenCategory::Misc}},
      {TokenKindType::T_CLOSE_TAG, {"T_CLOSE_TAG", "close tag", TokenCategory::Misc}},
      {TokenKindType::T_START_HEREDOC, {"T_START_HEREDOC", "heredoc start", TokenCategory::Misc}},
      {TokenKindType::T_END_HEREDOC, {"T_END_HEREDOC", "heredoc end", TokenCategory::Misc}},
      {TokenKindType::T_ERROR, {"T_ERROR", "error", TokenCategory::Misc}},
      {TokenKindType::T_UNKNOWN_MARK, {"T_UNKNOWN_MARK", "unknown token", TokenCategory::Misc}},
};
} // anonymous namespace

TokenDescItemType retrieve_token_desc_entry(TokenKindType kind)
{
   if (scg_tokenDescTable.find(kind) == scg_tokenDescTable.end()) {
      kind = TokenKindType::T_UNKNOWN_MARK;
   }
   return scg_tokenDescTable.at(kind);
}

TokenDescMap::const_iterator find_token_desc_entry(TokenKindType kind)
{
   return scg_tokenDescTable.find(kind);
}

TokenDescMap::const_iterator token_desc_map_end()
{
   return scg_tokenDescTable.end();
}

TokenCategory get_token_category(TokenKindType kind)
{
   TokenDescMap::const_iterator iter = scg_tokenDescTable.find(kind);
   assert(iter != scg_tokenDescTable.end());
   return std::get<2>(iter->second);
}

bool is_internal_token(TokenKindType kind)
{
   TokenCategory category = get_token_category(kind);
   return category == TokenCategory::Internal;
}

bool is_keyword_token(TokenKindType kind)
{
   TokenCategory category = get_token_category(kind);
   return category == TokenCategory::Keyword ||
          category == TokenCategory::DeclKeyword ||
          category == TokenCategory::StmtKeyword ||
          category == TokenCategory::ExprKeyword;
}

bool is_decl_keyword_token(TokenKindType kind)
{
   TokenCategory category = get_token_category(kind);
   return category == TokenCategory::DeclKeyword;
}
bool is_stmt_keyword_token(TokenKindType kind)
{
   TokenCategory category = get_token_category(kind);
   return category == TokenCategory::StmtKeyword;
}

bool is_expr_keyword_token(TokenKindType kind)
{
   TokenCategory category = get_token_category(kind);
   return category == TokenCategory::ExprKeyword;
}

bool is_punctuator_token(TokenKindType kind)
{
   TokenCategory category = get_token_category(kind);
   return category == TokenCategory::Punctuator;
}

bool is_misc_token(TokenKindType kind)
{
   TokenCategory category = get_token_category(kind);
   return category == TokenCategory::Misc;
}

} // polar::syntax
