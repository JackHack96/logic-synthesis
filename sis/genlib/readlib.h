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
extern int GENLIB_yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum GENLIB_yytokentype
  {
    OPR_OR = 258,
    OPR_AND = 259,
    CONST1 = 260,
    CONST0 = 261,
    IDENTIFIER = 262,
    LPAREN = 263,
    REAL = 264,
    OPR_NOT = 265,
    OPR_NOT_POST = 266,
    GATE = 267,
    PIN = 268,
    SEMI = 269,
    ASSIGN = 270,
    RPAREN = 271,
    LATCH = 272,
    CONTROL = 273,
    CONSTRAINT = 274,
    SEQ = 275
  };
#endif
/* Tokens.  */
#define OPR_OR 258
#define OPR_AND 259
#define CONST1 260
#define CONST0 261
#define IDENTIFIER 262
#define LPAREN 263
#define REAL 264
#define OPR_NOT 265
#define OPR_NOT_POST 266
#define GATE 267
#define PIN 268
#define SEMI 269
#define ASSIGN 270
#define RPAREN 271
#define LATCH 272
#define CONTROL 273
#define CONSTRAINT 274
#define SEQ 275

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 29 "readlib.y" /* yacc.c:1913  */

    tree_node_t *nodeval;
    char *strval;
    double realval; 
    pin_info_t *pinval;
    function_t *functionval;
    latch_info_t *latchval;
    constraint_info_t *constrval;

#line 104 "y.tab.h" /* yacc.c:1913  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE GENLIB_yylval;

int GENLIB_yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
