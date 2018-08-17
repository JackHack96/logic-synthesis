
/* SCCSID %W% */
#include "sis/util/util.h"
#include "struct.h"
#include "global.h"
#include "stack.h"
#include "merge.h"

STACK               *xstack;
static unsigned int _size;
static int          *_local[STACKLIMIT];
static int          **top_of_stack;
static int          **stack_ptr;

/* Emulation of the stack operation */
/* This routine guarantees compact use of memory */
/* But is must pay for it in terms of time */

alloc_stack(size)
                   {
                           if ((xstack = ALLOC(STACK, size)) == NIL(STACK))
                           panic("ALLOC in init_stack");
                   }

free_stack_head() {
    if (FREE(xstack))
        panic("lint");
}

init_stack(id)
                  {
                          _size = 0;
                          top_of_stack = _local;
                          stack_ptr = _local;
                          xstack[id].status = CONDITIONAL_COMPATIBLE;
                  }

open_stack(id)
                  {
                          stack_ptr = xstack[id].ptr;
                          top_of_stack = (int **)*stack_ptr;
                          stack_ptr = (int **)*(stack_ptr + 1);
                  }

close_stack(id, status
)
{
xstack[id].
ptr = NIL(
int *);
xstack[id].
status = status;
}

free_stack(id, status
)
{
if (
FREE(xstack[id]
.ptr))
panic("lint");
xstack[id].
ptr = NIL(
int *);
xstack[id].
status = status;
}

pack_stack(id)
                  {
                          int **real_alloc;

                          if ((real_alloc = ALLOC(int *, _size+2)) == NIL(int *))
                          panic("ALLOC in stack");
                          xstack[id].ptr = real_alloc;
                          xstack[id].status = CONDITIONAL_COMPATIBLE;
                          *real_alloc = (int *)(real_alloc + 2);
                          real_alloc++;
                          *real_alloc = (int *)(real_alloc + _size + 1);
                  MEMCPY(++real_alloc, _local, sizeof(char *) * _size);
                  }

static _push(STACK
*);

push(state2, state1
)
{
#ifdef DEBUG
(void) printf("Push %s %s \n",states[state1]->state_name,states[state2]->state_name);
#endif
if (state2 >  state1)
_push(&xstack[(num_st-state1-2)*(num_st-state1-1)/2-state2+num_st-1]);
else
_push(&xstack[(num_st-state2-2)*(num_st-state2-1)/2-state1+num_st-1]);
}

static
_push(operand)
STACK *operand;
{
*stack_ptr++ = (int *)
operand;
if (_size++ > STACKLIMIT)
panic("Stack Overflow");
}

/*
STACK *
_pop(arg2)
STACK **arg2;
{
	if ((*arg2 = pop()) == NIL(STACK))
		return NIL(STACK);
	else
		return(pop());
}
*/

STACK *
pop() {
    if (stack_ptr == top_of_stack)
        return EMPTY;
    else
        return (STACK *) *--stack_ptr;
}

#ifdef DEBUG
dump_stack(id)
{
    int **stack;
    register i;


    stack = xstack[id].ptr;
    (void) printf("STACK SIZE %d\n",_size);
    for (i=0; i<_size+2; i++) {
        (void) printf("ADDRESS: %x VALUE %x\n",stack,*stack);
        stack++;
    }
}
#endif
