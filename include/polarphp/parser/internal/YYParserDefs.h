/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_POLAR_VOLUMES_DATA_PROJECTS_POLARPHP_INCLUDE_POLARPHP_PARSER_INTERNAL_YYPARSERDEFS_H_INCLUDED
# define YY_POLAR_VOLUMES_DATA_PROJECTS_POLARPHP_INCLUDE_POLARPHP_PARSER_INTERNAL_YYPARSERDEFS_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int polar_debug;
#endif
/* "%code requires" blocks.  */
#line 24 "/Volumes/data/projects/polarphp/include/polarphp/parser/LangGrammer.y" /* yacc.c:1916  */


#line 47 "/Volumes/data/projects/polarphp/include/polarphp/parser/internal/YYParserDefs.h" /* yacc.c:1916  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    END = 0,
    T_INCLUDE = 258,
    T_INCLUDE_ONCE = 259,
    T_REQUIRE = 260,
    T_REQUIRE_ONCE = 261,
    T_LOGICAL_OR = 262,
    T_LOGICAL_XOR = 263,
    T_LOGICAL_AND = 264,
    T_PRINT = 265,
    T_YIELD = 266,
    T_DOUBLE_ARROW = 267,
    T_YIELD_FROM = 268,
    T_PLUS_EQUAL = 269,
    T_MINUS_EQUAL = 270,
    T_MUL_EQUAL = 271,
    T_DIV_EQUAL = 272,
    T_CONCAT_EQUAL = 273,
    T_MOD_EQUAL = 274,
    T_AND_EQUAL = 275,
    T_OR_EQUAL = 276,
    T_XOR_EQUAL = 277,
    T_SL_EQUAL = 278,
    T_SR_EQUAL = 279,
    T_POW_EQUAL = 280,
    T_COALESCE_EQUAL = 281,
    T_COALESCE = 282,
    T_BOOLEAN_OR = 283,
    T_BOOLEAN_AND = 284,
    T_IS_EQUAL = 285,
    T_IS_NOT_EQUAL = 286,
    T_IS_IDENTICAL = 287,
    T_IS_NOT_IDENTICAL = 288,
    T_SPACESHIP = 289,
    T_IS_SMALLER_OR_EQUAL = 290,
    T_IS_GREATER_OR_EQUAL = 291,
    T_SL = 292,
    T_SR = 293,
    T_INSTANCEOF = 294,
    T_INC = 295,
    T_DEC = 296,
    T_INT_CAST = 297,
    T_DOUBLE_CAST = 298,
    T_STRING_CAST = 299,
    T_ARRAY_CAST = 300,
    T_OBJECT_CAST = 301,
    T_BOOL_CAST = 302,
    T_UNSET_CAST = 303,
    T_POW = 304,
    T_NEW = 305,
    T_CLONE = 306,
    T_NOELSE = 307,
    T_ELSEIF = 308,
    T_ELSE = 309,
    T_LNUMBER = 310,
    T_DNUMBER = 311,
    T_STRING = 312,
    T_VARIABLE = 313,
    T_INLINE_HTML = 314,
    T_ENCAPSED_AND_WHITESPACE = 315,
    T_CONSTANT_ENCAPSED_STRING = 316,
    T_STRING_VARNAME = 317,
    T_NUM_STRING = 318,
    T_EVAL = 319,
    T_EXIT = 320,
    T_IF = 321,
    T_ENDIF = 322,
    T_ECHO = 323,
    T_DO = 324,
    T_WHILE = 325,
    T_ENDWHILE = 326,
    T_FOR = 327,
    T_ENDFOR = 328,
    T_FOREACH = 329,
    T_ENDFOREACH = 330,
    T_DECLARE = 331,
    T_ENDDECLARE = 332,
    T_AS = 333,
    T_SWITCH = 334,
    T_ENDSWITCH = 335,
    T_CASE = 336,
    T_DEFAULT = 337,
    T_BREAK = 338,
    T_CONTINUE = 339,
    T_GOTO = 340,
    T_FUNCTION = 341,
    T_CONST = 342,
    T_RETURN = 343,
    T_TRY = 344,
    T_CATCH = 345,
    T_FINALLY = 346,
    T_THROW = 347,
    T_USE = 348,
    T_INSTEADOF = 349,
    T_GLOBAL = 350,
    T_STATIC = 351,
    T_ABSTRACT = 352,
    T_FINAL = 353,
    T_PRIVATE = 354,
    T_PROTECTED = 355,
    T_PUBLIC = 356,
    T_VAR = 357,
    T_UNSET = 358,
    T_ISSET = 359,
    T_EMPTY = 360,
    T_HALT_COMPILER = 361,
    T_CLASS = 362,
    T_TRAIT = 363,
    T_INTERFACE = 364,
    T_EXTENDS = 365,
    T_IMPLEMENTS = 366,
    T_OBJECT_OPERATOR = 367,
    T_LIST = 368,
    T_ARRAY = 369,
    T_CALLABLE = 370,
    T_LINE = 371,
    T_FILE = 372,
    T_DIR = 373,
    T_CLASS_C = 374,
    T_TRAIT_C = 375,
    T_METHOD_C = 376,
    T_FUNC_C = 377,
    T_COMMENT = 378,
    T_DOC_COMMENT = 379,
    T_OPEN_TAG = 380,
    T_OPEN_TAG_WITH_ECHO = 381,
    T_CLOSE_TAG = 382,
    T_WHITESPACE = 383,
    T_START_HEREDOC = 384,
    T_END_HEREDOC = 385,
    T_DOLLAR_OPEN_CURLY_BRACES = 386,
    T_CURLY_OPEN = 387,
    T_PAAMAYIM_NEKUDOTAYIM = 388,
    T_NAMESPACE = 389,
    T_NS_C = 390,
    T_NS_SEPARATOR = 391,
    T_ELLIPSIS = 392,
    T_ERROR = 393
  };
#endif

/* Value type.  */



int polar_parse (void);

#endif /* !YY_POLAR_VOLUMES_DATA_PROJECTS_POLARPHP_INCLUDE_POLARPHP_PARSER_INTERNAL_YYPARSERDEFS_H_INCLUDED  */
