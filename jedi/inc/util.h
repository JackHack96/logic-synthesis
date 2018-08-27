
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

/*
 * useful types
 */
typedef int Boolean;

/*
 * useful macros
 */

#define TRUE 1
#define FALSE 0

#define pow_of_2(x) (int)pow((double)2, (double)x)

/*
 * token holders for parse_line
 */
char *targv[5000]; /* pointer to tokens */
int targc;         /* number of tokens */

/*
 * forward declarations
 */
char *int_to_binary(); /* converts decimal to binary */
FILE *my_fopen();      /* opens a file */
