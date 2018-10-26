

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
