/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/mimi/stack.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:15 $
 *
 */

/* SCCSID %W% */
typedef struct stack STACK;

struct stack {
	int status;
	int **ptr;
};

#define STACKLIMIT 1000
#define EMPTY NIL(STACK)

STACK *pop();
extern STACK *xstack;
