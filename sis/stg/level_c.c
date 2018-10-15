/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/stg/level_c.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 *
 */
#ifdef SIS
/**********************************************************************/
/*           This is the levelling routine for senum                  */
/*                                                                    */
/*           Author: Tony Hi-Keung Ma                                 */
/*           last update : 05/16/1987                                 */
/**********************************************************************/

#include "sis.h"
#include "stg_int.h"

void
level_circuit()
{
    int i,x;
    lsGen gen,gen2;
    node_t *node,*fi,*fo,*latch_end;
    ndata *n,*nfirst,*nlast,*nprelast,*ncurrent,*wfirst,*w,*temp;
    int schedule,level,val;
    long literal;
    node_cube_t cube;

    level = 0;
    nfirst = nlast = wfirst  = NIL(ndata);
    foreach_node (copy,gen,node) {
        n = SENUM_ALLOC(ndata,1);
	setnptr(node,n);
	n->node = node;
	n->next = n->wnext = NIL(ndata);
	if (node_num_cube(node)) {
	    cube = node_get_cube(node,0);
	    literal = 0;
	    for (i = node_num_fanin(node) - 1; i >= 0; i--) {
	        literal <<= 1;
		literal += (node_get_literal(cube,i) == ONE);
	    }
	    n->cube = literal;
	}
	else {
	    n->cube = 1;
	}
    }
    x = 0;
    foreach_primary_input (copy,gen,node) {
        n = nptr(node);
        latch_end = network_latch_end(node);
	if (latch_end) {
	    stg_pstate[x] = nptr(node);
	    stg_estate[x] = latch_get_initial_value(latch_from_node(latch_end));
	    stg_nstate[x] = nptr(node_get_fanin(latch_end,0));
	    x++;
	}
	else {
	    varying_node[n_varying_nodes++] = n;
	}
	n->level = level;
	n->jflag[0] = MARKED;
	foreach_fanout (node,gen2,fo) {
	    n = nptr(fo);
	    if (n->next == NIL(ndata) && n != nlast) {
	        n->next = nfirst;
		nfirst = n;
	    }
	    if (nlast == NIL(ndata)) {
	        nlast = nfirst;
	    }
	}
    }
    x = 0;
    foreach_primary_output (copy,gen,node) {
        fi = node_get_fanin(node,0);
	if (node_type(fi) == INTERNAL && node_num_fanin(fi) == 0) {
	    n = nptr(fi);
	    n->level = level;
	    n->jflag[0] = MARKED;
	    val = (node_function(fi) == NODE_1);
	    i = 0;
	    do {
	        n->value[i] = val;
	    } while (++i < MAX_ELENGTH);
	}
	if (network_latch_end(node) == NIL(node_t)) {
	    real_po[x++] = nptr(fi);
	}
    }

    nprelast = nlast;

    while (nfirst) {
        ncurrent = nfirst;
        nfirst = nfirst->next;
	ncurrent->next = NIL(ndata);
        schedule = TRUE;
	foreach_fanin (ncurrent->node,i,fi) {
	    if ((nptr(fi)->jflag[0] & MARKED) == 0) {
	        schedule = FALSE;
		break;
	    }
	}
        if (schedule) {
	    if (node_type(ncurrent->node) == INTERNAL) {
	        varying_node[n_varying_nodes++] = ncurrent;
	    }
	    ncurrent->wnext = wfirst;
	    wfirst = ncurrent;
	    foreach_fanout (ncurrent->node,gen,fo) {
	        n = nptr(fo);
	        if (n->next == NIL(ndata) && n != nlast) {
		    nlast->next = n;
		    nlast = n;
		    if (nfirst == NIL(ndata)) {
		        nfirst = nlast;
		    }
		}
	    }
	}
	else {
	    if (ncurrent->next == NIL(ndata) && ncurrent != nlast) {
	        nlast->next = ncurrent;
		nlast = ncurrent;
	    }
	    if (nfirst == NIL(ndata)) {
	        nfirst = nlast;
	    }
	}
        if (ncurrent == nprelast) {
            level++;
            if (wfirst == NIL(ndata)) {
                (void) fprintf(stderr,"There are errors in the circuit\n!");
                (void) fprintf(stderr,"levelling stop at level %d\n", level);
                exit(-1);
            }
            for (w = wfirst; w; w = temp) {
	        temp = w->wnext;
		w->wnext = NIL(ndata);
		w->jflag[0] = MARKED;
		w->level = level;
            }
            wfirst = NIL(ndata);
            nprelast = nlast;
        }
    }
    /* unmark each wire */
    foreach_node (copy,gen,node) {
        nptr(node)->jflag[0] = 0;
    }
}/* end of level */


void
rearrange_gate_inputs()
{
    int j,k,nin,stop,litj,litk;
    node_t **fanin,*node,*temp;
    lsGen gen;
    ndata *n,*nj,*nk;
    extern int barray[];

    foreach_node (copy,gen,node) {
        nin = node_num_fanin(node);
        if (node_type(node) == INTERNAL && nin > 1) {
	    stop = nin - 1;
	    fanin = node->fanin;
	    n = nptr(node);
	    for (j = 0; j < stop; j++) {
	        nj = nptr(fanin[j]);
	        for (k = j + 1; k < nin; k++) {
		    nk = nptr(fanin[k]);
		    if (nj->level > nk->level) {
		        temp = fanin[j];
			fanin[j] = fanin[k];
			fanin[k] = temp;
			litj = n->cube & barray[j];
			litk = n->cube & barray[k];
			if (litk == 0) {
			    n->cube &= ~litj;
			}
			else {
			    n->cube |= barray[j];
			}
			if (litj == 0) {
			    n->cube &= ~litk;
			}
			else {
			    n->cube |= barray[k];
			}			
		    }
		}
	    }
	}
    }
}
#endif /* SIS */
