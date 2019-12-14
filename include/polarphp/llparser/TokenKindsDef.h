//===--- TokenKinds.def - Swift Tokenizer Metaprogramming -----------------===//
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
///
/// This file defines x-macros used for metaprogramming with lexer tokens.
///
/// TOKEN(name)
///   KEYWORD(kw)
///     POLARPHP_KEYWORD(kw)
///       DECL_KEYWORD(kw)
///       STMT_KEYWORD(kw)
///       EXPR_KEYWORD(kw)
///       PAT_KEYWORD(kw)
///     PIL_KEYWORD(kw)
///   POUND_KEYWORD(kw)
///     POUND_OBJECT_LITERAL(kw, desc, proto)
///     POUND_CONFIG(kw)
///     POUND_DIRECTIVE_KEYWORD(kw)
///       POUND_COND_DIRECTIVE_KEYWORD(kw)
///   PUNCTUATOR(name, str)
///   LITERAL(name)
///   MISC(name)
///
//===----------------------------------------------------------------------===//

/// TOKEN(name)
///   Expands by default for every token kind.
#ifndef TOKEN
#define TOKEN(name)
#endif

/// KEYWORD(kw)
///   Expands by default for every Swift keyword and every PIL keyword, such as
///   'if', 'else', 'pil_global', etc. If you only want to use Swift keywords
///   see POLARPHP_KEYWORD.
#ifndef KEYWORD
#define KEYWORD(kw) TOKEN(kw_ ## kw)
#endif

/// POLARPHP_KEYWORD(kw)
///   Expands for every Swift keyword.
#ifndef POLARPHP_KEYWORD
#define POLARPHP_KEYWORD(kw) KEYWORD(kw)
#endif

/// DECL_KEYWORD(kw)
///   Expands for every Swift keyword that can be used in a declaration.
#ifndef DECL_KEYWORD
#define DECL_KEYWORD(kw) POLARPHP_KEYWORD(kw)
#endif

/// STMT_KEYWORD(kw)
///   Expands for every Swift keyword used in statement grammar.
#ifndef STMT_KEYWORD
#define STMT_KEYWORD(kw) POLARPHP_KEYWORD(kw)
#endif

/// EXPR_KEYWORD(kw)
///   Expands for every Swift keyword used in an expression, such as 'true',
///   'false', and 'as'
#ifndef EXPR_KEYWORD
#define EXPR_KEYWORD(kw) POLARPHP_KEYWORD(kw)
#endif

/// PAT_KEYWORD(kw)
///   Expands for every Swift keyword used in a pattern, which is currently
///   limited to '_'
#ifndef PAT_KEYWORD
#define PAT_KEYWORD(kw) POLARPHP_KEYWORD(kw)
#endif

/// PIL_KEYWORD(kw)
///   Expands for every PIL keyword. These are only keywords when parsing PIL.
#ifndef PIL_KEYWORD
#define PIL_KEYWORD(kw) KEYWORD(kw)
#endif

/// POUND_KEYWORD(kw)
///   Every keyword prefixed with a '#'.
#ifndef POUND_KEYWORD
#define POUND_KEYWORD(kw) TOKEN(pound_ ## kw)
#endif

/// POUND_OBJECT_LITERAL(kw, desc, proto)
///   Every keyword prefixed with a '#' representing an object literal.
#ifndef POUND_OBJECT_LITERAL
#define POUND_OBJECT_LITERAL(kw, desc, proto) POUND_KEYWORD(kw)
#endif

/// POUND_CONFIG(kw)
///   Every keyword prefixed with a '#' representing a configuration.
#ifndef POUND_CONFIG
#define POUND_CONFIG(kw) POUND_KEYWORD(kw)
#endif

/// POUND_DIRECTIVE_KEYWORD(kw)
///   Every keyword prefixed with a '#' that is a compiler control directive.
#ifndef POUND_DIRECTIVE_KEYWORD
#define POUND_DIRECTIVE_KEYWORD(kw) POUND_KEYWORD(kw)
#endif

/// POUND_COND_DIRECTIVE_KEYWORD(kw)
///   Every keyword prefixed with a '#' that is part of conditional compilation
///   control.
#ifndef POUND_COND_DIRECTIVE_KEYWORD
#define POUND_COND_DIRECTIVE_KEYWORD(kw) POUND_DIRECTIVE_KEYWORD(kw)
#endif

/// PUNCTUATOR(name, str)
///   Expands for every polarphp punctuator.
///   \param name  The symbolic name of the punctuator, such as
///                'l_paren' or 'arrow'.
///   \param str   A string literal containing the spelling of the punctuator,
///                such as '"("' or '"->"'.
#ifndef PUNCTUATOR
#define PUNCTUATOR(name, str) TOKEN(name)
#endif

/// LITERAL(name)
///   Tokens representing literal values, e.g. 'integer_literal'.
#ifndef LITERAL
#define LITERAL(name) TOKEN(name)
#endif

/// MISC(name)
///   Miscellaneous tokens, e.g. 'eof' and 'unknown'.
#ifndef MISC
#define MISC(name) TOKEN(name)
#endif

// Keywords that start decls.
DECL_KEYWORD(associatedtype)
DECL_KEYWORD(class)
DECL_KEYWORD(deinit)
DECL_KEYWORD(enum)
DECL_KEYWORD(extension)
DECL_KEYWORD(func)
DECL_KEYWORD(import)
DECL_KEYWORD(init)
DECL_KEYWORD(inout)
DECL_KEYWORD(let)
DECL_KEYWORD(operator)
DECL_KEYWORD(precedencegroup)
DECL_KEYWORD(interface)
DECL_KEYWORD(struct)
DECL_KEYWORD(subscript)
DECL_KEYWORD(typealias)
DECL_KEYWORD(var)
DECL_KEYWORD(fileprivate)
DECL_KEYWORD(internal)
DECL_KEYWORD(private)
DECL_KEYWORD(public)
DECL_KEYWORD(static)

// Statement keywords
STMT_KEYWORD(defer)
STMT_KEYWORD(if)
STMT_KEYWORD(guard)
STMT_KEYWORD(do)
STMT_KEYWORD(repeat)
STMT_KEYWORD(else)
STMT_KEYWORD(for)
STMT_KEYWORD(in)
STMT_KEYWORD(while)
STMT_KEYWORD(return)
STMT_KEYWORD(break)
STMT_KEYWORD(continue)
STMT_KEYWORD(fallthrough)
STMT_KEYWORD(switch)
STMT_KEYWORD(case)
STMT_KEYWORD(default)
STMT_KEYWORD(where)
STMT_KEYWORD(catch)
STMT_KEYWORD(throw)

// Expression keywords
EXPR_KEYWORD(as)
EXPR_KEYWORD(Any)
EXPR_KEYWORD(false)
EXPR_KEYWORD(is)
EXPR_KEYWORD(nil)
EXPR_KEYWORD(rethrows)
EXPR_KEYWORD(super)
EXPR_KEYWORD(self)
EXPR_KEYWORD(Self)
EXPR_KEYWORD(true)
EXPR_KEYWORD(try)
EXPR_KEYWORD(throws)

KEYWORD(__FILE__)
KEYWORD(__LINE__)
KEYWORD(__COLUMN__)
KEYWORD(__FUNCTION__)
KEYWORD(__DSO_HANDLE__)

// Pattern keywords
PAT_KEYWORD(_)

// Punctuators
PUNCTUATOR(l_paren, "(")
PUNCTUATOR(r_paren, ")")
PUNCTUATOR(l_brace, "{")
PUNCTUATOR(r_brace, "}")
PUNCTUATOR(l_square, "[")
PUNCTUATOR(r_square, "]")
PUNCTUATOR(l_angle, "<")
PUNCTUATOR(r_angle, ">")
PUNCTUATOR(period, ".")
PUNCTUATOR(period_prefix, ".")
PUNCTUATOR(comma, ",")
PUNCTUATOR(ellipsis, "...")
PUNCTUATOR(colon, ":")
PUNCTUATOR(semi, "")
PUNCTUATOR(equal, "=")
PUNCTUATOR(at_sign, "@")
PUNCTUATOR(pound, "#")
PUNCTUATOR(amp_prefix, "&")
PUNCTUATOR(arrow, "->")
PUNCTUATOR(backtick, "`")
PUNCTUATOR(backslash, "\\")
PUNCTUATOR(exclaim_postfix, "!")
PUNCTUATOR(question_postfix, "?")
PUNCTUATOR(question_infix, "?")
PUNCTUATOR(string_quote, "\"")
PUNCTUATOR(single_quote, "\'")
PUNCTUATOR(multiline_string_quote, "\"\"\"")

// Keywords prefixed with a '#'.

POUND_KEYWORD(keyPath)
POUND_KEYWORD(line)
POUND_KEYWORD(selector)
POUND_KEYWORD(file)
POUND_KEYWORD(column)
POUND_KEYWORD(function)
POUND_KEYWORD(dsohandle)
POUND_KEYWORD(assert)

POUND_DIRECTIVE_KEYWORD(sourceLocation)
POUND_DIRECTIVE_KEYWORD(warning)
POUND_DIRECTIVE_KEYWORD(error)

POUND_COND_DIRECTIVE_KEYWORD(if)
POUND_COND_DIRECTIVE_KEYWORD(else)
POUND_COND_DIRECTIVE_KEYWORD(elseif)
POUND_COND_DIRECTIVE_KEYWORD(endif)

POUND_CONFIG(available)

POUND_OBJECT_LITERAL(fileLiteral, "file reference", ExpressibleByFileReferenceLiteral)
POUND_OBJECT_LITERAL(imageLiteral, "image", ExpressibleByImageLiteral)
POUND_OBJECT_LITERAL(colorLiteral, "color", ExpressibleByColorLiteral)

LITERAL(integer_literal)
LITERAL(floating_literal)
LITERAL(string_literal)

MISC(unknown)
MISC(identifier)
MISC(oper_binary_unspaced)
MISC(oper_binary_spaced)
MISC(oper_postfix)
MISC(oper_prefix)
MISC(dollarident)
MISC(contextual_keyword)
MISC(raw_string_delimiter)
MISC(string_segment)
MISC(string_interpolation_anchor)
MISC(kw_yield)

// The following tokens are irrelevant for swiftSyntax and thus only declared
// in this .def file

PIL_KEYWORD(undef)
PIL_KEYWORD(pil)
PIL_KEYWORD(pil_stage)
PIL_KEYWORD(pil_property)
PIL_KEYWORD(pil_vtable)
PIL_KEYWORD(pil_global)
PIL_KEYWORD(pil_witness_table)
PIL_KEYWORD(pil_default_witness_table)
PIL_KEYWORD(pil_coverage_map)
PIL_KEYWORD(pil_scope)

PUNCTUATOR(pil_dollar,    "$")   // Only in PIL mode.
PUNCTUATOR(pil_exclamation, "!")    // Only in PIL mode.

MISC(eof)
MISC(code_complete)
MISC(pil_local_name)       // %42 in PIL mode.
MISC(comment)


#undef TOKEN
#undef KEYWORD
#undef POLARPHP_KEYWORD
#undef DECL_KEYWORD
#undef STMT_KEYWORD
#undef EXPR_KEYWORD
#undef PAT_KEYWORD
#undef PIL_KEYWORD
#undef POUND_KEYWORD
#undef POUND_OBJECT_LITERAL
#undef POUND_CONFIG
#undef POUND_DIRECTIVE_KEYWORD
#undef POUND_COND_DIRECTIVE_KEYWORD
#undef PUNCTUATOR
#undef LITERAL
#undef MISC
