/* A Bison parser, made by GNU Bison 3.1.  */

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

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int EQN_yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum EQN_yytokentype
  {
    OPR_OR = 258,
    OPR_AND = 259,
    CONST_ZERO = 260,
    CONST_ONE = 261,
    IDENTIFIER = 262,
    LPAREN = 263,
    OPR_XOR = 264,
    OPR_XNOR = 265,
    OPR_NOT = 266,
    OPR_NOT_POST = 267,
    NAME = 268,
    INORDER = 269,
    OUTORDER = 270,
    PASS = 271,
    ASSIGN = 272,
    SEMI = 273,
    RPAREN = 274,
    END = 275
  };
#endif
/* Tokens.  */
#define OPR_OR 258
#define OPR_AND 259
#define CONST_ZERO 260
#define CONST_ONE 261
#define IDENTIFIER 262
#define LPAREN 263
#define OPR_XOR 264
#define OPR_XNOR 265
#define OPR_NOT 266
#define OPR_NOT_POST 267
#define NAME 268
#define INORDER 269
#define OUTORDER 270
#define PASS 271
#define ASSIGN 272
#define SEMI 273
#define RPAREN 274
#define END 275

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 69 "read_eqn.y" /* yacc.c:1913  */

    char *strval;
    node_t *node;
    array_t *array;

#line 100 "y.tab.h" /* yacc.c:1913  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE EQN_yylval;

int EQN_yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
