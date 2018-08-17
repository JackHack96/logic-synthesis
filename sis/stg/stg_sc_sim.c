
#ifdef SIS
#include "sis.h"
#include "stg_int.h"

/*
 * This routine depends on the network consisting of solely AND gates.  It was
 * modified from the "explict/simulate" code.
 */
static void
sc_evaluate(n,cid)
ndata *n;
int cid;
{
    int covered,literal,val,k;
    node_t *fi;
    long cube;
    node_t *node;
    char *value;

    node = n->node;
    cube = n->cube;
    covered = 1;

    foreach_fanin (node,k,fi) {
        literal = cube & 1;
    cube >>= 1;
    val = nptr(fi)->value[cid];
    if (val == 2) {
        covered = 2;
    }
    else if (literal + val == 1) {
        covered = 0;
        break;
    }
    }
    value = n->value;
    if (value[cid] != covered) {
        value[cid] = covered;
    n->jflag[cid] |= CHANGED;
    }
}

void
stg_sc_sim(cid)
int cid;
{
    node_t *node,*fo;
    lsGen gen,gen2;
    char *jflag;
    int i;
    ndata *n;

    foreach_primary_input (copy,gen,node) {
    n = nptr(node);
        jflag = n->jflag;
        if (jflag[cid] & CHANGED) {
        jflag[cid] &= ~CHANGED;
        foreach_fanout (node,gen2,fo) {
            if (node_type(fo) != PRIMARY_OUTPUT) {
            nptr(fo)->jflag[cid] |= SCHEDULED;
        }
        }
    }
    }
    for (i = npi; i < n_varying_nodes; i++) {
        n = varying_node[i];
        jflag = n->jflag;
    if (jflag[cid] & SCHEDULED) {
        jflag[cid] &= ~SCHEDULED;
        sc_evaluate(n,cid);
        if (jflag[cid] & CHANGED) {
            jflag[cid] &= ~CHANGED;
        node = n->node;
        foreach_fanout (node,gen2,fo) {
            if (node_type(fo) != PRIMARY_OUTPUT) {
                nptr(fo)->jflag[cid] |= SCHEDULED;
            }
        }
        }
    }
    }
}
#endif /* SIS */
