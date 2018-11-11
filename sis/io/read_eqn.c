/* A Bison parser, made by GNU Bison 3.1.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with EQN_yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 2 "read_eqn.y" /* yacc.c:339  */

#include "sis.h"
#include "io_int.h"
#include <setjmp.h>
#include "config.h"

#if YYTEXT_POINTER
extern char *EQN_yytext;
#else
extern char EQN_yytext[];
#endif

static network_t *global_network;
static lsList po_list;

int EQN_yyerror();
#if 0 /* #ifndef FLEX_SCANNER */
#undef EQN_yywrap
static int input();
static int unput();
static int EQN_yywrap();
#endif

extern int equation_setup_string(char *);
extern int equation_setup_file(FILE *);

static void
do_assign(name, expr)
char *name;
node_t *expr;
{
    char errmsg[1024];
    node_t *node;

    node = read_find_or_create_node(global_network, name);
    if (node_function(node) != NODE_UNDEFINED) {
	(void) sprintf(errmsg, "Attempt to redefine '%s'\n", name);
	EQN_yyerror(errmsg);   /* never returns */
    }
    FREE(name);
    node_replace(node, expr);
}

static node_t *
do_sexpr_list(list, func)
array_t *list;
node_t *(*func)();
{
    int i;
    node_t *node, *node1, *node2;

    node1 = array_fetch(node_t *, list, 0);
    node = node_dup(node1);
    node_free(node1);
    for(i = 1; i < array_n(list); i++) {
	node1 = node;
	node2 = array_fetch(node_t *, list, i);
	node = (*func)(node1, node2);
	node_free(node1);
	node_free(node2);
    }
    array_free(list);
    return node;
}


#line 133 "y.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
#line 69 "read_eqn.y" /* yacc.c:355  */

    char *strval;
    node_t *node;
    array_t *array;

#line 219 "y.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE EQN_yylval;

int EQN_yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 236 "y.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 EQN_yytype_uint8;
#else
typedef unsigned char EQN_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 EQN_yytype_int8;
#else
typedef signed char EQN_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 EQN_yytype_uint16;
#else
typedef unsigned short EQN_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 EQN_yytype_int16;
#else
typedef short EQN_yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about EQN_yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined EQN_yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined EQN_yyoverflow || YYERROR_VERBOSE */


#if (! defined EQN_yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union EQN_yyalloc
{
  EQN_yytype_int16 EQN_yyss_alloc;
  YYSTYPE EQN_yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union EQN_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (EQN_yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T EQN_yynewbytes;                                            \
        YYCOPY (&EQN_yyptr->Stack_alloc, Stack, EQN_yysize);                    \
        Stack = &EQN_yyptr->Stack_alloc;                                    \
        EQN_yynewbytes = EQN_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        EQN_yyptr += EQN_yynewbytes / sizeof (*EQN_yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T EQN_yyi;                         \
          for (EQN_yyi = 0; EQN_yyi < (Count); EQN_yyi++)   \
            (Dst)[EQN_yyi] = (Src)[EQN_yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  22
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   132

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  21
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  10
/* YYNRULES -- Number of rules.  */
#define YYNRULES  39
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  70

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by EQN_yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? EQN_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by EQN_yylex, without out-of-bounds checking.  */
static const EQN_yytype_uint8 EQN_yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const EQN_yytype_uint8 EQN_yyrline[] =
{
       0,    86,    86,    87,    88,    89,    93,    94,    97,    98,
     104,   106,   108,   111,   116,   119,   122,   125,   128,   131,
     134,   137,   140,   143,   146,   156,   159,   162,   165,   168,
     171,   174,   177,   187,   192,   198,   203,   204,   221,   222
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const EQN_yytname[] =
{
  "$end", "error", "$undefined", "OPR_OR", "OPR_AND", "CONST_ZERO",
  "CONST_ONE", "IDENTIFIER", "LPAREN", "OPR_XOR", "OPR_XNOR", "OPR_NOT",
  "OPR_NOT_POST", "NAME", "INORDER", "OUTORDER", "PASS", "ASSIGN", "SEMI",
  "RPAREN", "END", "$accept", "program", "prog", "stat", "expr", "sexpr",
  "sexpr_list", "identifier", "input_list", "output_list", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const EQN_yytype_uint16 EQN_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275
};
# endif

#define YYPACT_NINF -10

#define EQN_yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-10)))

#define YYTABLE_NINF -9

#define EQN_yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const EQN_yytype_int8 EQN_yypact[] =
{
      64,   -10,   -10,   -10,    24,    -9,    -7,    -6,    12,    73,
       2,   -10,    -1,   119,   119,     4,     6,   119,    14,    14,
     -10,   -10,   -10,     7,    16,    37,    -1,   -10,   112,   110,
     -10,    -4,   -10,    57,   -10,   -10,    13,   119,   -10,    14,
      14,   -10,   -10,   -10,   -10,   112,   112,    91,   -10,   -10,
     -10,   -10,   -10,    41,   -10,   -10,    42,   -10,   112,   112,
     112,   112,   -10,   120,   -10,   -10,   100,   120,    -5,    -5
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const EQN_yytype_uint8 EQN_yydefact[] =
{
       2,    28,    29,    35,     0,     0,     0,     0,     0,     3,
       0,     5,    32,     0,     0,     0,     0,     0,     0,     0,
      36,    38,     1,     0,     0,     0,     0,     6,     0,     0,
      33,     0,    32,     0,    30,    31,     0,     0,     9,    10,
      11,     4,     7,    21,    22,     0,     0,    12,    24,    26,
      34,    25,    27,     0,    37,    39,     0,    20,     0,     0,
       0,     0,    19,    16,    13,    23,    14,    15,    17,    18
};

  /* YYPGOTO[NTERM-NUM].  */
static const EQN_yytype_int8 EQN_yypgoto[] =
{
     -10,   -10,   -10,    58,    -2,     5,    52,     0,   -10,   -10
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const EQN_yytype_int8 EQN_yydefgoto[] =
{
      -1,     8,     9,    10,    63,    30,    31,    48,    39,    40
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const EQN_yytype_int8 EQN_yytable[] =
{
      12,     1,     2,     3,    29,    11,    46,    62,    19,    26,
      20,    21,    22,    32,    32,    49,    28,    32,    37,    38,
      27,     3,    36,    34,    18,    35,    47,    13,    14,    15,
      16,    32,    52,    32,    41,    17,    50,    32,    50,    54,
      55,    18,    53,    56,    57,    58,    59,    43,    44,     3,
      45,    60,    61,    46,    62,    42,    66,    67,    68,    69,
      64,    65,     1,     2,     3,    29,    33,    25,     0,     1,
       2,     3,     4,     0,     0,     0,    51,     5,     6,     7,
       3,    23,    -8,     0,     0,     0,     5,     6,     7,     0,
       0,    -8,     0,    24,    58,    59,    43,    44,     3,    45,
      60,    61,    46,    62,    59,    43,    44,     3,    45,    60,
      61,    46,    62,    13,    14,    15,    16,    43,    44,     3,
      45,    17,     0,    46,     1,     2,     3,    29,     0,    60,
      61,    46,    62
};

static const EQN_yytype_int8 EQN_yycheck[] =
{
       0,     5,     6,     7,     8,     0,    11,    12,    17,     9,
      17,    17,     0,    13,    14,    19,    17,    17,    18,    19,
      18,     7,    17,    19,    17,    19,    28,     3,     4,     5,
       6,    31,    19,    33,    18,    11,    31,    37,    33,    39,
      40,    17,    37,    45,    46,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    18,    58,    59,    60,    61,
      19,    19,     5,     6,     7,     8,    14,     9,    -1,     5,
       6,     7,     8,    -1,    -1,    -1,    19,    13,    14,    15,
       7,     8,    18,    -1,    -1,    -1,    13,    14,    15,    -1,
      -1,    18,    -1,    20,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,     4,     5,     6,     7,     8,     9,
      10,    11,    12,     3,     4,     5,     6,     5,     6,     7,
       8,    11,    -1,    11,     5,     6,     7,     8,    -1,     9,
      10,    11,    12
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const EQN_yytype_uint8 EQN_yystos[] =
{
       0,     5,     6,     7,     8,    13,    14,    15,    22,    23,
      24,    26,    28,     3,     4,     5,     6,    11,    17,    17,
      17,    17,     0,     8,    20,    24,    28,    18,    17,     8,
      26,    27,    28,    27,    19,    19,    26,    28,    28,    29,
      30,    18,    18,     5,     6,     8,    11,    25,    28,    19,
      26,    19,    19,    26,    28,    28,    25,    25,     3,     4,
       9,    10,    12,    25,    19,    19,    25,    25,    25,    25
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const EQN_yytype_uint8 EQN_yyr1[] =
{
       0,    21,    22,    22,    22,    22,    23,    23,    24,    24,
      24,    24,    24,    24,    25,    25,    25,    25,    25,    25,
      25,    25,    25,    25,    25,    26,    26,    26,    26,    26,
      26,    26,    26,    27,    27,    28,    29,    29,    30,    30
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const EQN_yytype_uint8 EQN_yyr2[] =
{
       0,     2,     0,     1,     3,     1,     2,     3,     0,     3,
       3,     3,     3,     5,     3,     3,     2,     3,     3,     2,
       2,     1,     1,     3,     1,     4,     4,     4,     1,     1,
       3,     3,     1,     1,     2,     1,     0,     2,     0,     2
};


#define EQN_yyerrok         (EQN_yyerrstatus = 0)
#define EQN_yyclearin       (EQN_yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto EQN_yyacceptlab
#define YYABORT         goto EQN_yyabortlab
#define YYERROR         goto EQN_yyerrorlab


#define YYRECOVERING()  (!!EQN_yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (EQN_yychar == YYEMPTY)                                        \
    {                                                           \
      EQN_yychar = (Token);                                         \
      EQN_yylval = (Value);                                         \
      YYPOPSTACK (EQN_yylen);                                       \
      EQN_yystate = *EQN_yyssp;                                         \
      goto EQN_yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      EQN_yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (EQN_yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (EQN_yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      EQN_yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
EQN_yy_symbol_value_print (FILE *EQN_yyoutput, int EQN_yytype, YYSTYPE const * const EQN_yyvaluep)
{
  FILE *EQN_yyo = EQN_yyoutput;
  YYUSE (EQN_yyo);
  if (!EQN_yyvaluep)
    return;
# ifdef YYPRINT
  if (EQN_yytype < YYNTOKENS)
    YYPRINT (EQN_yyoutput, EQN_yytoknum[EQN_yytype], *EQN_yyvaluep);
# endif
  YYUSE (EQN_yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
EQN_yy_symbol_print (FILE *EQN_yyoutput, int EQN_yytype, YYSTYPE const * const EQN_yyvaluep)
{
  YYFPRINTF (EQN_yyoutput, "%s %s (",
             EQN_yytype < YYNTOKENS ? "token" : "nterm", EQN_yytname[EQN_yytype]);

  EQN_yy_symbol_value_print (EQN_yyoutput, EQN_yytype, EQN_yyvaluep);
  YYFPRINTF (EQN_yyoutput, ")");
}

/*------------------------------------------------------------------.
| EQN_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
EQN_yy_stack_print (EQN_yytype_int16 *EQN_yybottom, EQN_yytype_int16 *EQN_yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; EQN_yybottom <= EQN_yytop; EQN_yybottom++)
    {
      int EQN_yybot = *EQN_yybottom;
      YYFPRINTF (stderr, " %d", EQN_yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (EQN_yydebug)                                                  \
    EQN_yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
EQN_yy_reduce_print (EQN_yytype_int16 *EQN_yyssp, YYSTYPE *EQN_yyvsp, int EQN_yyrule)
{
  unsigned long EQN_yylno = EQN_yyrline[EQN_yyrule];
  int EQN_yynrhs = EQN_yyr2[EQN_yyrule];
  int EQN_yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             EQN_yyrule - 1, EQN_yylno);
  /* The symbols being reduced.  */
  for (EQN_yyi = 0; EQN_yyi < EQN_yynrhs; EQN_yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", EQN_yyi + 1);
      EQN_yy_symbol_print (stderr,
                       EQN_yystos[EQN_yyssp[EQN_yyi + 1 - EQN_yynrhs]],
                       &(EQN_yyvsp[(EQN_yyi + 1) - (EQN_yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (EQN_yydebug)                          \
    EQN_yy_reduce_print (EQN_yyssp, EQN_yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int EQN_yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef EQN_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define EQN_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
EQN_yystrlen (const char *EQN_yystr)
{
  YYSIZE_T EQN_yylen;
  for (EQN_yylen = 0; EQN_yystr[EQN_yylen]; EQN_yylen++)
    continue;
  return EQN_yylen;
}
#  endif
# endif

# ifndef EQN_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define EQN_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
EQN_yystpcpy (char *EQN_yydest, const char *EQN_yysrc)
{
  char *EQN_yyd = EQN_yydest;
  const char *EQN_yys = EQN_yysrc;

  while ((*EQN_yyd++ = *EQN_yys++) != '\0')
    continue;

  return EQN_yyd - 1;
}
#  endif
# endif

# ifndef EQN_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for EQN_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from EQN_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
EQN_yytnamerr (char *EQN_yyres, const char *EQN_yystr)
{
  if (*EQN_yystr == '"')
    {
      YYSIZE_T EQN_yyn = 0;
      char const *EQN_yyp = EQN_yystr;

      for (;;)
        switch (*++EQN_yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++EQN_yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (EQN_yyres)
              EQN_yyres[EQN_yyn] = *EQN_yyp;
            EQN_yyn++;
            break;

          case '"':
            if (EQN_yyres)
              EQN_yyres[EQN_yyn] = '\0';
            return EQN_yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! EQN_yyres)
    return EQN_yystrlen (EQN_yystr);

  return EQN_yystpcpy (EQN_yyres, EQN_yystr) - EQN_yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
EQN_yysyntax_error (YYSIZE_T *EQN_yymsg_alloc, char **EQN_yymsg,
                EQN_yytype_int16 *EQN_yyssp, int EQN_yytoken)
{
  YYSIZE_T EQN_yysize0 = EQN_yytnamerr (YY_NULLPTR, EQN_yytname[EQN_yytoken]);
  YYSIZE_T EQN_yysize = EQN_yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *EQN_yyformat = YY_NULLPTR;
  /* Arguments of EQN_yyformat. */
  char const *EQN_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int EQN_yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in EQN_yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated EQN_yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (EQN_yytoken != YYEMPTY)
    {
      int EQN_yyn = EQN_yypact[*EQN_yyssp];
      EQN_yyarg[EQN_yycount++] = EQN_yytname[EQN_yytoken];
      if (!EQN_yypact_value_is_default (EQN_yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int EQN_yyxbegin = EQN_yyn < 0 ? -EQN_yyn : 0;
          /* Stay within bounds of both EQN_yycheck and EQN_yytname.  */
          int EQN_yychecklim = YYLAST - EQN_yyn + 1;
          int EQN_yyxend = EQN_yychecklim < YYNTOKENS ? EQN_yychecklim : YYNTOKENS;
          int EQN_yyx;

          for (EQN_yyx = EQN_yyxbegin; EQN_yyx < EQN_yyxend; ++EQN_yyx)
            if (EQN_yycheck[EQN_yyx + EQN_yyn] == EQN_yyx && EQN_yyx != YYTERROR
                && !EQN_yytable_value_is_error (EQN_yytable[EQN_yyx + EQN_yyn]))
              {
                if (EQN_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    EQN_yycount = 1;
                    EQN_yysize = EQN_yysize0;
                    break;
                  }
                EQN_yyarg[EQN_yycount++] = EQN_yytname[EQN_yyx];
                {
                  YYSIZE_T EQN_yysize1 = EQN_yysize + EQN_yytnamerr (YY_NULLPTR, EQN_yytname[EQN_yyx]);
                  if (! (EQN_yysize <= EQN_yysize1
                         && EQN_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  EQN_yysize = EQN_yysize1;
                }
              }
        }
    }

  switch (EQN_yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        EQN_yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T EQN_yysize1 = EQN_yysize + EQN_yystrlen (EQN_yyformat);
    if (! (EQN_yysize <= EQN_yysize1 && EQN_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    EQN_yysize = EQN_yysize1;
  }

  if (*EQN_yymsg_alloc < EQN_yysize)
    {
      *EQN_yymsg_alloc = 2 * EQN_yysize;
      if (! (EQN_yysize <= *EQN_yymsg_alloc
             && *EQN_yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *EQN_yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *EQN_yyp = *EQN_yymsg;
    int EQN_yyi = 0;
    while ((*EQN_yyp = *EQN_yyformat) != '\0')
      if (*EQN_yyp == '%' && EQN_yyformat[1] == 's' && EQN_yyi < EQN_yycount)
        {
          EQN_yyp += EQN_yytnamerr (EQN_yyp, EQN_yyarg[EQN_yyi++]);
          EQN_yyformat += 2;
        }
      else
        {
          EQN_yyp++;
          EQN_yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
EQN_yydestruct (const char *EQN_yymsg, int EQN_yytype, YYSTYPE *EQN_yyvaluep)
{
  YYUSE (EQN_yyvaluep);
  if (!EQN_yymsg)
    EQN_yymsg = "Deleting";
  YY_SYMBOL_PRINT (EQN_yymsg, EQN_yytype, EQN_yyvaluep, EQN_yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (EQN_yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int EQN_yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE EQN_yylval;
/* Number of syntax errors so far.  */
int EQN_yynerrs;


/*----------.
| EQN_yyparse.  |
`----------*/

int
EQN_yyparse (void)
{
    int EQN_yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int EQN_yyerrstatus;

    /* The stacks and their tools:
       'EQN_yyss': related to states.
       'EQN_yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow EQN_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    EQN_yytype_int16 EQN_yyssa[YYINITDEPTH];
    EQN_yytype_int16 *EQN_yyss;
    EQN_yytype_int16 *EQN_yyssp;

    /* The semantic value stack.  */
    YYSTYPE EQN_yyvsa[YYINITDEPTH];
    YYSTYPE *EQN_yyvs;
    YYSTYPE *EQN_yyvsp;

    YYSIZE_T EQN_yystacksize;

  int EQN_yyn;
  int EQN_yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int EQN_yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE EQN_yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char EQN_yymsgbuf[128];
  char *EQN_yymsg = EQN_yymsgbuf;
  YYSIZE_T EQN_yymsg_alloc = sizeof EQN_yymsgbuf;
#endif

#define YYPOPSTACK(N)   (EQN_yyvsp -= (N), EQN_yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int EQN_yylen = 0;

  EQN_yyssp = EQN_yyss = EQN_yyssa;
  EQN_yyvsp = EQN_yyvs = EQN_yyvsa;
  EQN_yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  EQN_yystate = 0;
  EQN_yyerrstatus = 0;
  EQN_yynerrs = 0;
  EQN_yychar = YYEMPTY; /* Cause a token to be read.  */
  goto EQN_yysetstate;

/*------------------------------------------------------------.
| EQN_yynewstate -- Push a new state, which is found in EQN_yystate.  |
`------------------------------------------------------------*/
 EQN_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  EQN_yyssp++;

 EQN_yysetstate:
  *EQN_yyssp = EQN_yystate;

  if (EQN_yyss + EQN_yystacksize - 1 <= EQN_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T EQN_yysize = EQN_yyssp - EQN_yyss + 1;

#ifdef EQN_yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *EQN_yyvs1 = EQN_yyvs;
        EQN_yytype_int16 *EQN_yyss1 = EQN_yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if EQN_yyoverflow is a macro.  */
        EQN_yyoverflow (YY_("memory exhausted"),
                    &EQN_yyss1, EQN_yysize * sizeof (*EQN_yyssp),
                    &EQN_yyvs1, EQN_yysize * sizeof (*EQN_yyvsp),
                    &EQN_yystacksize);

        EQN_yyss = EQN_yyss1;
        EQN_yyvs = EQN_yyvs1;
      }
#else /* no EQN_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto EQN_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= EQN_yystacksize)
        goto EQN_yyexhaustedlab;
      EQN_yystacksize *= 2;
      if (YYMAXDEPTH < EQN_yystacksize)
        EQN_yystacksize = YYMAXDEPTH;

      {
        EQN_yytype_int16 *EQN_yyss1 = EQN_yyss;
        union EQN_yyalloc *EQN_yyptr =
          (union EQN_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (EQN_yystacksize));
        if (! EQN_yyptr)
          goto EQN_yyexhaustedlab;
        YYSTACK_RELOCATE (EQN_yyss_alloc, EQN_yyss);
        YYSTACK_RELOCATE (EQN_yyvs_alloc, EQN_yyvs);
#  undef YYSTACK_RELOCATE
        if (EQN_yyss1 != EQN_yyssa)
          YYSTACK_FREE (EQN_yyss1);
      }
# endif
#endif /* no EQN_yyoverflow */

      EQN_yyssp = EQN_yyss + EQN_yysize - 1;
      EQN_yyvsp = EQN_yyvs + EQN_yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) EQN_yystacksize));

      if (EQN_yyss + EQN_yystacksize - 1 <= EQN_yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", EQN_yystate));

  if (EQN_yystate == YYFINAL)
    YYACCEPT;

  goto EQN_yybackup;

/*-----------.
| EQN_yybackup.  |
`-----------*/
EQN_yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  EQN_yyn = EQN_yypact[EQN_yystate];
  if (EQN_yypact_value_is_default (EQN_yyn))
    goto EQN_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (EQN_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      EQN_yychar = EQN_yylex ();
    }

  if (EQN_yychar <= YYEOF)
    {
      EQN_yychar = EQN_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      EQN_yytoken = YYTRANSLATE (EQN_yychar);
      YY_SYMBOL_PRINT ("Next token is", EQN_yytoken, &EQN_yylval, &EQN_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  EQN_yyn += EQN_yytoken;
  if (EQN_yyn < 0 || YYLAST < EQN_yyn || EQN_yycheck[EQN_yyn] != EQN_yytoken)
    goto EQN_yydefault;
  EQN_yyn = EQN_yytable[EQN_yyn];
  if (EQN_yyn <= 0)
    {
      if (EQN_yytable_value_is_error (EQN_yyn))
        goto EQN_yyerrlab;
      EQN_yyn = -EQN_yyn;
      goto EQN_yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (EQN_yyerrstatus)
    EQN_yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", EQN_yytoken, &EQN_yylval, &EQN_yylloc);

  /* Discard the shifted token.  */
  EQN_yychar = YYEMPTY;

  EQN_yystate = EQN_yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++EQN_yyvsp = EQN_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto EQN_yynewstate;


/*-----------------------------------------------------------.
| EQN_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
EQN_yydefault:
  EQN_yyn = EQN_yydefact[EQN_yystate];
  if (EQN_yyn == 0)
    goto EQN_yyerrlab;
  goto EQN_yyreduce;


/*-----------------------------.
| EQN_yyreduce -- Do a reduction.  |
`-----------------------------*/
EQN_yyreduce:
  /* EQN_yyn is the number of a rule to reduce with.  */
  EQN_yylen = EQN_yyr2[EQN_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  EQN_yyval = EQN_yyvsp[1-EQN_yylen];


  YY_REDUCE_PRINT (EQN_yyn);
  switch (EQN_yyn)
    {
        case 5:
#line 90 "read_eqn.y" /* yacc.c:1651  */
    { do_assign(util_strsav("SILLY"), (EQN_yyvsp[0].node)); }
#line 1362 "y.tab.c" /* yacc.c:1651  */
    break;

  case 9:
#line 99 "read_eqn.y" /* yacc.c:1651  */
    {
			network_set_name(global_network, (EQN_yyvsp[0].strval));
			FREE((EQN_yyvsp[0].strval));
		    }
#line 1371 "y.tab.c" /* yacc.c:1651  */
    break;

  case 12:
#line 109 "read_eqn.y" /* yacc.c:1651  */
    { do_assign((EQN_yyvsp[-2].strval), (EQN_yyvsp[0].node)); }
#line 1377 "y.tab.c" /* yacc.c:1651  */
    break;

  case 13:
#line 112 "read_eqn.y" /* yacc.c:1651  */
    { do_assign((EQN_yyvsp[-2].strval), (EQN_yyvsp[-1].node)); }
#line 1383 "y.tab.c" /* yacc.c:1651  */
    break;

  case 14:
#line 117 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_or((EQN_yyvsp[-2].node), (EQN_yyvsp[0].node)); node_free((EQN_yyvsp[-2].node)); node_free((EQN_yyvsp[0].node));}
#line 1389 "y.tab.c" /* yacc.c:1651  */
    break;

  case 15:
#line 120 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_and((EQN_yyvsp[-2].node), (EQN_yyvsp[0].node)); node_free((EQN_yyvsp[-2].node)); node_free((EQN_yyvsp[0].node));}
#line 1395 "y.tab.c" /* yacc.c:1651  */
    break;

  case 16:
#line 123 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_and((EQN_yyvsp[-1].node), (EQN_yyvsp[0].node)); node_free((EQN_yyvsp[-1].node)); node_free((EQN_yyvsp[0].node));}
#line 1401 "y.tab.c" /* yacc.c:1651  */
    break;

  case 17:
#line 126 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_xor((EQN_yyvsp[-2].node), (EQN_yyvsp[0].node)); node_free((EQN_yyvsp[-2].node)); node_free((EQN_yyvsp[0].node));}
#line 1407 "y.tab.c" /* yacc.c:1651  */
    break;

  case 18:
#line 129 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_xnor((EQN_yyvsp[-2].node), (EQN_yyvsp[0].node)); node_free((EQN_yyvsp[-2].node)); node_free((EQN_yyvsp[0].node));}
#line 1413 "y.tab.c" /* yacc.c:1651  */
    break;

  case 19:
#line 132 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_not((EQN_yyvsp[-1].node)); node_free((EQN_yyvsp[-1].node)); }
#line 1419 "y.tab.c" /* yacc.c:1651  */
    break;

  case 20:
#line 135 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_not((EQN_yyvsp[0].node)); node_free((EQN_yyvsp[0].node)); }
#line 1425 "y.tab.c" /* yacc.c:1651  */
    break;

  case 21:
#line 138 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_constant(0); }
#line 1431 "y.tab.c" /* yacc.c:1651  */
    break;

  case 22:
#line 141 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_constant(1); }
#line 1437 "y.tab.c" /* yacc.c:1651  */
    break;

  case 23:
#line 144 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = (EQN_yyvsp[-1].node); }
#line 1443 "y.tab.c" /* yacc.c:1651  */
    break;

  case 24:
#line 147 "read_eqn.y" /* yacc.c:1651  */
    {
			node_t *node;
			node = read_find_or_create_node(global_network, (EQN_yyvsp[0].strval));
			(EQN_yyval.node) = node_literal(node, 1);
			FREE((EQN_yyvsp[0].strval));
		    }
#line 1454 "y.tab.c" /* yacc.c:1651  */
    break;

  case 25:
#line 157 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = do_sexpr_list((EQN_yyvsp[-1].array), node_and); }
#line 1460 "y.tab.c" /* yacc.c:1651  */
    break;

  case 26:
#line 160 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = do_sexpr_list((EQN_yyvsp[-1].array), node_or); }
#line 1466 "y.tab.c" /* yacc.c:1651  */
    break;

  case 27:
#line 163 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_not((EQN_yyvsp[-1].node)); node_free((EQN_yyvsp[-1].node)); }
#line 1472 "y.tab.c" /* yacc.c:1651  */
    break;

  case 28:
#line 166 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_constant(0); }
#line 1478 "y.tab.c" /* yacc.c:1651  */
    break;

  case 29:
#line 169 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_constant(1); }
#line 1484 "y.tab.c" /* yacc.c:1651  */
    break;

  case 30:
#line 172 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_constant(0); }
#line 1490 "y.tab.c" /* yacc.c:1651  */
    break;

  case 31:
#line 175 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.node) = node_constant(1); }
#line 1496 "y.tab.c" /* yacc.c:1651  */
    break;

  case 32:
#line 178 "read_eqn.y" /* yacc.c:1651  */
    {
			node_t *node;
			node = read_find_or_create_node(global_network, (EQN_yyvsp[0].strval));
			(EQN_yyval.node) = node_literal(node, 1);
			FREE((EQN_yyvsp[0].strval));
		    }
#line 1507 "y.tab.c" /* yacc.c:1651  */
    break;

  case 33:
#line 188 "read_eqn.y" /* yacc.c:1651  */
    {
			(EQN_yyval.array) = array_alloc(node_t *, 10);
			array_insert_last(node_t *, (EQN_yyval.array), (EQN_yyvsp[0].node));
		    }
#line 1516 "y.tab.c" /* yacc.c:1651  */
    break;

  case 34:
#line 193 "read_eqn.y" /* yacc.c:1651  */
    {
			array_insert_last(node_t *, (EQN_yyvsp[-1].array), (EQN_yyvsp[0].node));
		    }
#line 1524 "y.tab.c" /* yacc.c:1651  */
    break;

  case 35:
#line 199 "read_eqn.y" /* yacc.c:1651  */
    { (EQN_yyval.strval) = util_strsav(EQN_yytext); }
#line 1530 "y.tab.c" /* yacc.c:1651  */
    break;

  case 37:
#line 205 "read_eqn.y" /* yacc.c:1651  */
    {
			node_t *node;
			char errmsg[1024];
			node = read_find_or_create_node(global_network, (EQN_yyvsp[0].strval));
			if (node_function(node) != NODE_UNDEFINED) {
			    (void) sprintf(errmsg, 
				"Attempt to redefine '%s'\n", (EQN_yyvsp[0].strval));
			    EQN_yyerror(errmsg);   /* never returns */
			}
			network_change_node_type(global_network, 
							node, PRIMARY_INPUT);
			FREE((EQN_yyvsp[0].strval));
		    }
#line 1548 "y.tab.c" /* yacc.c:1651  */
    break;

  case 39:
#line 223 "read_eqn.y" /* yacc.c:1651  */
    {
			node_t *node;
			node = read_find_or_create_node(global_network, (EQN_yyvsp[0].strval));
			LS_ASSERT(lsNewEnd(po_list, (lsGeneric) node, LS_NH));
			FREE((EQN_yyvsp[0].strval));
		    }
#line 1559 "y.tab.c" /* yacc.c:1651  */
    break;


#line 1563 "y.tab.c" /* yacc.c:1651  */
      default: break;
    }
  /* User semantic actions sometimes alter EQN_yychar, and that requires
     that EQN_yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of EQN_yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering EQN_yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", EQN_yyr1[EQN_yyn], &EQN_yyval, &EQN_yyloc);

  YYPOPSTACK (EQN_yylen);
  EQN_yylen = 0;
  YY_STACK_PRINT (EQN_yyss, EQN_yyssp);

  *++EQN_yyvsp = EQN_yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  EQN_yyn = EQN_yyr1[EQN_yyn];

  EQN_yystate = EQN_yypgoto[EQN_yyn - YYNTOKENS] + *EQN_yyssp;
  if (0 <= EQN_yystate && EQN_yystate <= YYLAST && EQN_yycheck[EQN_yystate] == *EQN_yyssp)
    EQN_yystate = EQN_yytable[EQN_yystate];
  else
    EQN_yystate = EQN_yydefgoto[EQN_yyn - YYNTOKENS];

  goto EQN_yynewstate;


/*--------------------------------------.
| EQN_yyerrlab -- here on detecting error.  |
`--------------------------------------*/
EQN_yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  EQN_yytoken = EQN_yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (EQN_yychar);

  /* If not already recovering from an error, report this error.  */
  if (!EQN_yyerrstatus)
    {
      ++EQN_yynerrs;
#if ! YYERROR_VERBOSE
      EQN_yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR EQN_yysyntax_error (&EQN_yymsg_alloc, &EQN_yymsg, \
                                        EQN_yyssp, EQN_yytoken)
      {
        char const *EQN_yymsgp = YY_("syntax error");
        int EQN_yysyntax_error_status;
        EQN_yysyntax_error_status = YYSYNTAX_ERROR;
        if (EQN_yysyntax_error_status == 0)
          EQN_yymsgp = EQN_yymsg;
        else if (EQN_yysyntax_error_status == 1)
          {
            if (EQN_yymsg != EQN_yymsgbuf)
              YYSTACK_FREE (EQN_yymsg);
            EQN_yymsg = (char *) YYSTACK_ALLOC (EQN_yymsg_alloc);
            if (!EQN_yymsg)
              {
                EQN_yymsg = EQN_yymsgbuf;
                EQN_yymsg_alloc = sizeof EQN_yymsgbuf;
                EQN_yysyntax_error_status = 2;
              }
            else
              {
                EQN_yysyntax_error_status = YYSYNTAX_ERROR;
                EQN_yymsgp = EQN_yymsg;
              }
          }
        EQN_yyerror (EQN_yymsgp);
        if (EQN_yysyntax_error_status == 2)
          goto EQN_yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (EQN_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (EQN_yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (EQN_yychar == YYEOF)
            YYABORT;
        }
      else
        {
          EQN_yydestruct ("Error: discarding",
                      EQN_yytoken, &EQN_yylval);
          EQN_yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto EQN_yyerrlab1;


/*---------------------------------------------------.
| EQN_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
EQN_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label EQN_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto EQN_yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (EQN_yylen);
  EQN_yylen = 0;
  YY_STACK_PRINT (EQN_yyss, EQN_yyssp);
  EQN_yystate = *EQN_yyssp;
  goto EQN_yyerrlab1;


/*-------------------------------------------------------------.
| EQN_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
EQN_yyerrlab1:
  EQN_yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      EQN_yyn = EQN_yypact[EQN_yystate];
      if (!EQN_yypact_value_is_default (EQN_yyn))
        {
          EQN_yyn += YYTERROR;
          if (0 <= EQN_yyn && EQN_yyn <= YYLAST && EQN_yycheck[EQN_yyn] == YYTERROR)
            {
              EQN_yyn = EQN_yytable[EQN_yyn];
              if (0 < EQN_yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (EQN_yyssp == EQN_yyss)
        YYABORT;


      EQN_yydestruct ("Error: popping",
                  EQN_yystos[EQN_yystate], EQN_yyvsp);
      YYPOPSTACK (1);
      EQN_yystate = *EQN_yyssp;
      YY_STACK_PRINT (EQN_yyss, EQN_yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++EQN_yyvsp = EQN_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", EQN_yystos[EQN_yyn], EQN_yyvsp, EQN_yylsp);

  EQN_yystate = EQN_yyn;
  goto EQN_yynewstate;


/*-------------------------------------.
| EQN_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
EQN_yyacceptlab:
  EQN_yyresult = 0;
  goto EQN_yyreturn;

/*-----------------------------------.
| EQN_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
EQN_yyabortlab:
  EQN_yyresult = 1;
  goto EQN_yyreturn;

#if !defined EQN_yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| EQN_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
EQN_yyexhaustedlab:
  EQN_yyerror (YY_("memory exhausted"));
  EQN_yyresult = 2;
  /* Fall through.  */
#endif

EQN_yyreturn:
  if (EQN_yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      EQN_yytoken = YYTRANSLATE (EQN_yychar);
      EQN_yydestruct ("Cleanup: discarding lookahead",
                  EQN_yytoken, &EQN_yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (EQN_yylen);
  YY_STACK_PRINT (EQN_yyss, EQN_yyssp);
  while (EQN_yyssp != EQN_yyss)
    {
      EQN_yydestruct ("Cleanup: popping",
                  EQN_yystos[*EQN_yyssp], EQN_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef EQN_yyoverflow
  if (EQN_yyss != EQN_yyssa)
    YYSTACK_FREE (EQN_yyss);
#endif
#if YYERROR_VERBOSE
  if (EQN_yymsg != EQN_yymsgbuf)
    YYSTACK_FREE (EQN_yymsg);
#endif
  return EQN_yyresult;
}
#line 231 "read_eqn.y" /* yacc.c:1910  */


static jmp_buf jmpbuf;

int
EQN_yyerror(errmsg)
char *errmsg;
{
    read_error(errmsg);
    longjmp(jmpbuf, 1);
}


network_t *
read_eqn(fp)
FILE *fp;
{
    error_init();

    if (setjmp(jmpbuf)) {
	/* syntax error -- return from EQN_yyerror() */
	LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));
	network_free(global_network);
	return 0;

    } else {
	global_network = network_alloc();
	read_filename_to_netname(global_network, read_filename);
	po_list = lsCreate();
	equation_setup_file(fp);
	(void) EQN_yyparse();

	if (!read_check_io_list(global_network, po_list)) {
	    network_free(global_network);
	    return 0;
	}
	read_hack_outputs(global_network, po_list);
	LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));

	if (! network_is_acyclic(global_network)) {
	    network_free(global_network);
	    return 0;
	}

	return global_network;
    }
}

network_t *
read_eqn_string(s)
char *s;
{
    error_init();

    if (setjmp(jmpbuf)) {
	/* syntax error -- return from EQN_yyerror() */
	LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));
	network_free(global_network);
	return 0;

    } else {
	global_network = network_alloc();
	po_list = lsCreate();
	equation_setup_string(s);
	(void) EQN_yyparse();

	if (!read_check_io_list(global_network, po_list)) {
	    network_free(global_network);
	    return 0;
	}
	read_hack_outputs(global_network, po_list);
	LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));

	if (! network_is_acyclic(global_network)) {
	    network_free(global_network);
	    return 0;
	}

	return global_network;
    }
}
