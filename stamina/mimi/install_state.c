
/* SCCSID%W% */

/************************************************************
 * install_state() --- install a state if it has not been   *
 *		       isntalled yet and return a pointer   *
 *                     to it.                               *
 *                     If it has been installed, just return*
 *                     a pointer to it.                     *
 ************************************************************/
/*  Modified by June Rho Dec. 1989.			    */

#include "sis/util/util.h"
#include "struct.h"
#include "global.h"

static n_states = 0;

access_n_state() {
    return n_states;
}

STATE *
install_state(state_name, hashtab, hash_size)
        char *state_name;
        NLIST *hashtab[];
        int hash_size;
{
    STATE      *state_ptr;
    static int count = 0;            /* state index counting */
    NLIST *lookup();
    NLIST *install();
    NLIST      *np;

    int id;            /* reference index of existing state */

    if ((np = lookup(state_name, hashtab, hash_size)) != NIL(NLIST)) {
        id        = np->order_index;
        state_ptr = states[id];
        return (state_ptr);        /* the state has been installed. */
    }
    n_states++;

    /*
     * Allocate memory for a new state.
      */

    if ((state_ptr = ALLOC(STATE, 1)) == NIL(STATE)) {
        panic("Memory allocation error");
    }
    state_ptr->edge = NIL(EDGE);

    np = install(state_name, count, (char *) 0, hashtab, hash_size);
    if (np == NIL(NLIST)) {
        panic("Failed to install a new state!");
    }

    states[count] = state_ptr;    /* save the pointer to a state */
    state_ptr->state_index = count; /* save the state_index        */
    state_ptr->state_name  = np->name;   /* save the state_name     */
/*	state_ptr->pedge = NIL(P_EDGE); */
/*	state_ptr->code = NIL(char);	/* assign a null pointer to char */
    state_ptr->assigned    = 0;    /* initial the flag to be zero */
    count++;


    return (state_ptr);

}
