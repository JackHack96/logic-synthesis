
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



/*
 *    bdd_ite_quick_cofactor - perform a quick cofactoring of f
 *
 *    return the f_v and f_nv cofactors
 */
void
bdd_ite_quick_cofactor(manager, f, varId, pf_v, pf_nv)
bdd_manager *manager;
bdd_node *f;
bdd_variableId varId;
bdd_node **pf_v;	/* return - f cofactored by varId */
bdd_node **pf_nv;	/* return - f cofactored by varId bar */
{
	bdd_node *reg_f;

	/* WATCHOUT - no safe frame is declared here (b/c its not needed) */

	BDD_ASSERT_NOTNIL(f);

	reg_f = BDD_REGULAR(f);

	if (reg_f->id == varId) {
	    /*
	     *    If f's variable is varId, then do a bit
	     *    of work to cofactor the bdd at f
	     */
	    BDD_ASSERT( ! BDD_IS_CONSTANT(manager, f) );	/* can't do this on zero or one */

	    if (BDD_IS_COMPLEMENT(f)) {
		*pf_v = BDD_NOT(reg_f->T);
		*pf_nv = BDD_NOT(reg_f->E);
	    } else {
		*pf_v = reg_f->T;
		*pf_nv = reg_f->E;
	    }
	} else {
	    /*
	     *    If f's variable is not varId, then no work need be done.
	     */
	    *pf_v = f;
	    *pf_nv = f;
	}

	BDD_ASSERT_NOTNIL(*pf_v);
	BDD_ASSERT_NOTNIL(*pf_nv);
}

/*
 *    bdd_ite_var_to_const - see if it is possible to replace variables by constants
 *
 *    Replace variables with constants if possible (part of canonical form).
 *
 *    ITE Identities
 *        ITE(F, G, F) = ITE(F, G, 0) = F * G
 *        ITE(F, G, !F) = ITE(F, G, 1) = !F + G
 *        ITE(F, F, H) = ITE(F, 1, H) = F + H
 *        ITE(F, !F, H) = ITE(F, 0, H) = !F * H
 *
 *    return nothing, just do it.
 */
void
bdd_ite_var_to_const(manager, f, pg, ph)
bdd_manager *manager;
bdd_node *f;		/* value */
bdd_node **pg;		/* value/return */
bdd_node **ph;		/* value/return */
{
	/* WATCHOUT - no safe frame is declared here (b/c its not needed) */

	BDD_ASSERT_NOTNIL(f);

	if (f == *pg) {
	    /*
	     *    ITE(F, F, H) = ITE(F, 1, H) = F + H
	     */
	    *pg = BDD_ONE(manager);
	} else if (f == BDD_NOT(*pg)) {
	    /*
	     *    ITE(F, !F, H) = ITE(F, 0, H) = !F * H
	     */
	    *pg = BDD_ZERO(manager);
	}

	if (f == *ph) {
	    /*
	     *    ITE(F, G, F) = ITE(F, G, 0) = F * G
	     */
	    *ph = BDD_ZERO(manager);
	} else if (f == BDD_NOT(*ph)) {
	    /*
	     *    ITE(F, G, !F) = ITE(F, G, 1) = !F + G
	     */
	    *ph = BDD_ONE(manager);
	}
}

static boolean greaterthan_equalp();
static void swap();

/*
 *    bdd_ite_canonicalize_ite_inputs - pick a unique member from among equivalent expressions 
 *
 *    This reduces 2 variable expressions to canonical form (p. 16, 17)
 *
 *    The purpose of this routine (as is described on page 16) is to canonicalize
 *    the inputs to the ITE function so that there can be as many cache hits as
 *    possible.  This canonicalization is a performance improvement only; all that
 *    is required of this routine is that it does not rewrite any ITE input into
 *    an incorrect form.
 *
 *    The code below is organized into what is believed to be the most efficient
 *    manner possible, given the listing of the ITE rewrite rules: a tree of if/else pairs.
 *
 *    return nothing, just do it.
 */
void
bdd_ite_canonicalize_ite_inputs(manager, pf, pg, ph, pcomplement)
bdd_manager *manager;
bdd_node **pf;			/* value/return */
bdd_node **pg;			/* value/return */
bdd_node **ph;			/* value/return */
boolean *pcomplement;		/* return */
{
	/* WATCHOUT - no safe frame is declared here (b/c its not needed) */

	BDD_ASSERT(pf != NIL(bdd_node *));
	BDD_ASSERT_NOTNIL(*pf);

	BDD_ASSERT(pg != NIL(bdd_node *));
	BDD_ASSERT_NOTNIL(*pg);

	BDD_ASSERT(ph != NIL(bdd_node *));
	BDD_ASSERT_NOTNIL(*ph);

	if (BDD_IS_CONSTANT(manager, *pg)) {
	    /*
	     *    ITE(F, c, H)
	     *    where c is either zero or one
	     */
	    if (greaterthan_equalp(*pf, *ph)) {
		/*
		 *    ``F > H'' implies a rewrite by the rule
		 */
		if (*pg == BDD_ONE(manager)) {
		    /*
		     *    ITE(F, 1, H) = ITE(H, 1, F)
		     */
		    swap(ph, pf);
		} else {	/* *pg == BDD_ZERO(manager) */
		    /*
		     *    ITE(F, 0, H) = ITE(!H, 0, !F)
		     */
		    swap(ph, pf);
		    *pf = BDD_NOT(*pf);
		    *ph = BDD_NOT(*ph);
		}
	    }
	} else if (BDD_IS_CONSTANT(manager, *ph)) {
	    /*
	     *    ITE(F, G, c)
	     *    where c is either zero or one
	     */
	    if (greaterthan_equalp(*pf, *pg)) {
		/*
		 *    ``F > G'' implies a rewrite by the rule
		 */
		if (*ph == BDD_ONE(manager)) {
		    /*
		     *    ITE(F, G, 1) = ITE(!G, !F, 1)
		     */
		    swap(pg, pf);
		    *pf = BDD_NOT(*pf);
		    *pg = BDD_NOT(*pg);
		} else {	/* *ph == BDD_ONE(manager) */
		    /*
		     *    ITE(F, G, 0) = ITE(G, F, 0)
		     */
		    swap(pg, pf);
		}
	    }
	} else if (*pg == BDD_NOT(*ph)) {
	    /*
	     *    ITE(F, G, !G)
	     */
	    if (greaterthan_equalp(*pf, *pg)) {
		/*
		 *    ``F > G'' implies a rewrite by the rule
		 *
		 *    ITE(F, G, !G) = ITE(G, F, !F)
		 */
		swap(pf, pg);
		*ph = BDD_NOT(*pg);
	    }
	}

	/*
	 *    Adjust pointers so that the first 2 arguments to ITE are regular.
	 *    This canonicalizes the ordering of the ITE inputs some more.
	 */
	if (BDD_IS_COMPLEMENT(*pf)) {
	    /*
	     *    ITE(!F, G, H) = ITE(F, H, G)
	     */
	    *pf = BDD_NOT(*pf);
	    swap(pg, ph);
	}

	if ( ! BDD_IS_COMPLEMENT(*pg) ) {
	    /*
	     *    ITE(F, G, H)
	     *    where H may be complemented or not
	     */
	    *pcomplement = FALSE;

	    BDD_ASSERT_REGNODE(*pf);
	    BDD_ASSERT_REGNODE(*pg);
	} else {	/* BDD_IS_COMPLEMENT(*pg) */
	    /*
	     *    ITE(F, !G, H) = ! ITE(F, G, !H)
	     */
	    *pcomplement = TRUE;

	    *pg = BDD_NOT(*pg);
	    *ph = BDD_NOT(*ph);
	}

	BDD_ASSERT_REGNODE(*pf);
	BDD_ASSERT_REGNODE(*pg);
}

/*
 *    greaterthan_equalp - induce a partial ordering on nodes
 *
 *    Compare first by the integer value of the variableId
 *    If the variableId's are the same, then use the complementation
 *    indication on the original function (f, or g).
 *    Complementation is ``greater than'' uncomplementation.
 *
 *    return {TRUE, FALSE} on {is, is not}.
 */
static boolean
greaterthan_equalp(f, g)
bdd_node *f;
bdd_node *g;
{
	bdd_node *reg_f, *reg_g;

	BDD_ASSERT_NOTNIL(f);
	BDD_ASSERT_NOTNIL(g);

	reg_f = BDD_REGULAR(f);
	reg_g = BDD_REGULAR(g);

	if (reg_f->id > reg_g->id) {
	    /*
	     *    If the nodes are different,
	     *    then f is clearly greater than g.
	     */
	    return (TRUE);
	}
	    
	if (reg_f->id == reg_g->id) {
	    /*
	     *    The variableId's are equal (they refer to the same variable),
	     *    so pick an arbitrary ordering for the two.  This is based
	     *    on some unique (yet arbitrary) property of the node, such
	     *    as its uniqueId or memory location (another uniqueId).
	     */
	    boolean arbitraryp;

#if defined(BDD_DEBUG_UID) && ! defined(BDD_DEBUG_LIFESPAN) /* { */
	    arbitraryp = reg_f->uniqueId > reg_g->uniqueId;
#else /* } else { */
	    arbitraryp = (int) f > (int) g;
#endif /* } */

	    if (arbitraryp);
		return (TRUE);
	}

	return (FALSE);
}

/*
 *    swap - very simple, just flip f and g
 *
 *    return nothing, just do it.
 */
static void
swap(pf, pg)
bdd_node **pf;	/* value/return */
bdd_node **pg;	/* value/return */
{
	bdd_node *tmp;	/* swap f and h */

	tmp = *pg;
	*pg = *pf;
	*pf = tmp;
}
