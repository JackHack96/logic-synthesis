/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/jedi/jedi.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:08 $
 *
 */
/*
 * Symbolic encoding program for compiling a symbolic
 * description into a binary representation.  Target
 * is multi-level logic synthesis
 *
 * History:
 *
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#include "copyright.h"
#include "port.h"
#include "utility.h"

#include "util.h"
#include "jedi_int.h"

/*
 * type declarations
 */
typedef struct Enumtype {
    char *name;			/* name of the enumerated type */
    int ns;			/* number of symbols */
    int nb;			/* number of bits for encoding */
    int nc;			/* number of possible codes */
    Boolean input_flag;		/* indicate input weights computed */
    Boolean output_flag;	/* indicate output weights computed */
    char *dont_care;		/* don't care bit-vector */
    struct Symbol *symbols;	/* array of admissible values */
    struct Code *codes;		/* array of possible codes */
    struct Link **links;	/* connectivity matrix */
    int **distances;		/* code distance matrix */
} Enumtype;

typedef struct Symbol {
    char *token;		/* mnemonic string */
    int code_ptr;		/* pointer to current assigned code */
} Symbol;

typedef struct Code {
    Boolean assigned;		/* assigned flag */
    char *bit_vector;		/* binary bit vector equivalent of decimal */
    int symbol_ptr;		/* pointer to current assigned symbol */
} Code;

typedef struct Link {
    int weight;			/* weight of this link */
} Link;

typedef struct Variable {
    char *name;			/* name of the variable */
    Boolean boolean_flag;	/* indicates a boolean type */
    int enumtype_ptr;		/* pointer to enumtype type */
    struct Entry *entries;	/* array of table entries */
} Variable;

typedef struct Entry {
    char *token;		/* mnemonic string */
    int enumtype_ptr;		/* pointer to enumtype type */
    int symbol_ptr;		/* pointer to symbol in enumtype */
} Entry;

/*
 * global variables
 */
int ni;				/* number of inputs */
int no;				/* number of outputs */
int np;				/* number of symbolic product terms */
int ne;				/* number of enumtype types */
int tni;			/* total number of binary inputs */
int tno;			/* total number of binary outputs */
struct Enumtype *enumtypes;	/* array of enumtypes */
struct Variable *inputs;	/* array of inputs */
struct Variable *outputs;	/* array of outputs */
