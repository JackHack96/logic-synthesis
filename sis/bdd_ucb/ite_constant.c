/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/ite_constant.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:02 $
 *
 */
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/ite_constant.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:02 $
 * $Log: ite_constant.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:02  pchong
 * imported
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.2  1992/09/19  01:48:31  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_.
 *
 * Revision 1.1  1992/07/29  00:27:08  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:40  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:44  shiple
 * Initial revision
 * 
 *
 */

static bdd_constant_status grock_constantness();
static bdd_constant_status grock_status();

/*
 *    bdd__ITE_constant - the ITE operation special-cased for constants
 *
 *    ITE Identities
 *        ITE(1, G, H) => G
 *        ITE(0, G, H) => H
 *        ITE(F, 1, 0) || ITE(F, 0, 1) => non_const
 *
 *    return {bdd_constant_one, bdd_constant_zero, bdd_nonconstant}
 */
bdd_constant_status
bdd__ITE_constant(manager, f, g, h)
bdd_manager *manager;
bdd_node *f;
bdd_node *g;
bdd_node *h;
{
    bdd_node *ret;
    bdd_node *reg_f, *f_v, *f_nv;
    bdd_node *reg_g, *g_v, *g_nv;
    bdd_node *reg_h, *h_v, *h_nv;
    bdd_variableId varId;
    boolean complement;
    bdd_constant_status ret_status, ite_T_status, ite_E_status;

    /* WATCHOUT - no safe frame is declared here (b/c its not needed) */

    manager->heap.stats.ITE_constant_ops.calls++;

    if (f == BDD_ONE(manager)) {
	/*
	 *    ITE(1, G, H) => G
	 */
	manager->heap.stats.ITE_constant_ops.returns.trivial++;
#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
	return (grock_constantness(manager, g, FALSE));
#else				/* } else { */
	if (g == BDD_ZERO(manager)) {
	    ret_status = bdd_constant_zero;
	} else if (g == BDD_ONE(manager)) {
	    ret_status = bdd_constant_one;
	} else {
	    ret_status = bdd_nonconstant;
	}

	return (ret_status);
#endif				/* } */
    }

    if (f == BDD_ZERO(manager)) {
	/*
	 *    ITE(0, G, H) => H
	 */
	manager->heap.stats.ITE_constant_ops.returns.trivial++;
#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
	return (grock_constantness(manager, h, FALSE));
#else				/* } else { */
	if (h == BDD_ZERO(manager)) {
	    ret_status = bdd_constant_zero;
	} else if (h == BDD_ONE(manager)) {
	    ret_status = bdd_constant_one;
	} else {
	    ret_status = bdd_nonconstant;
	}

	return (ret_status);
#endif				/* }  */
    }

    /*
     *    Because f is known not to be either the zero
     *    or the one, it is known to be not a constant.
     */
#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    (void) bdd_ite_var_to_const(manager, f, &g, &h);
#else				/* } else { */
    BDD_ASSERT_NOTNIL(f);

    if (f == g) {
	/*
	 *    ITE(F, F, H) = ITE(F, 1, H) = F + H
	 */
	g = BDD_ONE(manager);
    } else if (f == BDD_NOT(g)) {
	/*
	 *    ITE(F, !F, H) = ITE(F, 0, H) = !F * H
	 */
	g = BDD_ZERO(manager);
    }

    if (f == h) {
	/*
	 *    ITE(F, G, F) = ITE(F, G, 0) = F * G
	 */
	h = BDD_ZERO(manager);
    } else if (f == BDD_NOT(h)) {
	/*
	 *    ITE(F, G, !F) = ITE(F, G, 1) = !F + G
	 */
	h = BDD_ONE(manager);
    }
#endif				/* } */

    /* g and/or h may have been converted to constants */

    if (g == h) {
	/*
	 *    ITE(F, G, G) => G
	 */
	manager->heap.stats.ITE_constant_ops.returns.trivial++;

#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
	return (grock_constantness(manager, g, FALSE));
#else				/* } else { */
	if (g == BDD_ZERO(manager)) {
	    ret_status = bdd_constant_zero;
	} else if (g == BDD_ONE(manager)) {
	    ret_status = bdd_constant_one;
	} else {
	    ret_status = bdd_nonconstant;
	}

	return (ret_status);
#endif				/* }  */
    }
    if (BDD_IS_CONSTANT(manager, g) && BDD_IS_CONSTANT(manager, h)) {
	/*
	 *    ITE(F, 1, 0) || ITE(F, 0, 1) => non_const
	 */
	manager->heap.stats.ITE_constant_ops.returns.trivial++;
	return (bdd_nonconstant);
    }

#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    (void) bdd_ite_canonicalize_ite_inputs(manager, &f, &g, &h, &complement);
#else				/* } else { */
    BDD_ASSERT_NOTNIL(f);
    BDD_ASSERT_NOTNIL(g);
    BDD_ASSERT_NOTNIL(h);

    if (BDD_IS_CONSTANT(manager, g)) {
	/*
	 *    ITE(F, c, H)
	 *    where c is either zero or one
	 */
	if (BDD_REGULAR(f)->id > BDD_REGULAR(h)->id ||
	    (BDD_REGULAR(f)->id == BDD_REGULAR(h)->id && (int) f > (int) h)) {
	    /*
	     *    ``F > H'' implies a rewrite by the rule
	     */
	    if (g == BDD_ONE(manager)) {
		/*
		 *    ITE(F, 1, H) = ITE(H, 1, F)
		 */
		{ bdd_node *tmp;
		  tmp = f;
		  f = h;
		  h = tmp; }
	    } else {		/* g == BDD_ZERO(manager) */
		/*
		 *    ITE(F, 0, H) = ITE(!H, 0, !F)
		 */
		{ bdd_node *tmp;
		  tmp = h;
		  h = f;
		  f = tmp; }
		f = BDD_NOT(f);
		h = BDD_NOT(h);
	    }
	}
    } else if (BDD_IS_CONSTANT(manager, h)) {
	/*
	 *    ITE(F, G, c)
	 *    where c is either zero or one
	 */
	if (BDD_REGULAR(f)->id > BDD_REGULAR(g)->id ||
	    (BDD_REGULAR(f)->id == BDD_REGULAR(g)->id && (int) f > (int) g)) {
	    /*
	     *    ``F > G'' implies a rewrite by the rule
	     */
	    if (h == BDD_ONE(manager)) {
		/*
		 *    ITE(F, G, 1) = ITE(!G, !F, 1)
		 */
		{ bdd_node *tmp;
		  tmp = g;
		  g = f;
		  f = tmp; }
		f = BDD_NOT(f);
		g = BDD_NOT(g);
	    } else {		/* h == BDD_ONE(manager) */
		/*
		 *    ITE(F, G, 0) = ITE(G, F, 0)
		 */
		{ bdd_node *tmp;
		  tmp = g;
		  g = f;
		  f = tmp; }
	    }
	}
    } else if (g == BDD_NOT(h)) {
	/*
	 *    ITE(F, G, !G)
	 */
	if (BDD_REGULAR(f)->id > BDD_REGULAR(g)->id ||
	    (BDD_REGULAR(f)->id == BDD_REGULAR(g)->id && (int) f > (int) g)) {
	    /*
	     *    ``F > G'' implies a rewrite by the rule
	     *
	     *    ITE(F, G, !G) = ITE(G, F, !F)
	     */
	    { bdd_node *tmp;
	      tmp = f;
	      f = g;
	      g = tmp; }
	    h = BDD_NOT(g);
	}
    }

    /*
     *    Adjust pointers so that the first 2 arguments to ITE are regular.
     *    This canonicalizes the ordering of the ITE inputs some more.
     */
    if (BDD_IS_COMPLEMENT(f)) {
	/*
	 *    ITE(!F, G, H) = ITE(F, H, G)
	 */
	f = BDD_NOT(f);
	{ bdd_node *tmp;
	  tmp = g;
	  g = h;
	  h = tmp; }
    }

    if ( ! BDD_IS_COMPLEMENT(g) ) {
	/*
	 *    ITE(F, G, H)
	 *    where H may be complemented or not
	 */
	complement = FALSE;

	BDD_ASSERT_REGNODE(f);
	BDD_ASSERT_REGNODE(g);
    } else {			/* BDD_IS_COMPLEMENT(g) */
	/*
	 *    ITE(F, !G, H) = ! ITE(F, G, !H)
	 */
	complement = TRUE;

	g = BDD_NOT(g);
	h = BDD_NOT(h);
    }

    BDD_ASSERT_REGNODE(f);
    BDD_ASSERT_REGNODE(g);
#endif /* } */
	
	reg_f = BDD_REGULAR(f);
	reg_g = BDD_REGULAR(g);
	reg_h = BDD_REGULAR(h);

        /*
	 *    Perform a cache lookup; may be the value
	 *    has already been computed.
	 */
	if (bdd_constcache_lookup(manager, f, g, h, &ret_status)) {
	    /*
	     *    Fine, a hit, just return what we know about it.
	     */
	    manager->heap.stats.ITE_constant_ops.returns.cached++;
#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN)  /* { */
	    return (grock_status(ret_status, complement));
#else /* } else { */
	if (complement) {
	    switch (ret_status) {
	    case bdd_constant_zero:
		ret_status = bdd_constant_one;
		break;
	    case bdd_constant_one:
		ret_status = bdd_constant_zero;
		break;
	    case bdd_nonconstant:
		/* leave it alone */
		break;
	    }
	}

	return (ret_status);
#endif /* } */
	}

	if (bdd_hashcache_lookup(manager, f, g, h, &ret)) {
	    /*
	     *    Fine, a hit, just return what we know about it.
	     */
	    manager->heap.stats.ITE_constant_ops.returns.cached++;
#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN)  /* { */
	    return (grock_constantness(manager, ret, complement));
#else /* } else { */
	if (ret == BDD_ZERO(manager)) {
	    ret_status = complement ? bdd_constant_one: bdd_constant_zero;
	} else if (ret == BDD_ONE(manager)) {
	    ret_status = complement ? bdd_constant_zero: bdd_constant_one;
	} else {
	    ret_status = bdd_nonconstant;
	}

	    return (ret_status);
#endif /* } */
	}

	/*
	 *    Caching didn't work out for us so we 
	 *    need to go and do the recursive step.
	 */

	/* v = top_varId(reg_f, reg_g, reg_h) */
	varId = MIN3(reg_f->id, reg_g->id, reg_h->id);

	/*
	 *    The pattern:
	 *        f_v	- f cofactored by v
	 *        f_nv	- f cofactored by v bar
	 */
#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN)  /* { */
	(void) bdd_ite_quick_cofactor(manager, f, varId, &f_v, &f_nv);
	(void) bdd_ite_quick_cofactor(manager, g, varId, &g_v, &g_nv);
	(void) bdd_ite_quick_cofactor(manager, h, varId, &h_v, &h_nv);
#else /* } else { */
    {
	bdd_node *reg_f;

	BDD_ASSERT_NOTNIL(f);

	reg_f = BDD_REGULAR(f);

	if (reg_f->id == varId) {
	    /*
	     *    If f's variable is varId, then do a bit
	     *    of work to cofactor the bdd at f
	     */
	    BDD_ASSERT( ! BDD_IS_CONSTANT(manager, f) );	/* can't do this on zero or one */

	    if (BDD_IS_COMPLEMENT(f)) {
		f_v = BDD_NOT(reg_f->T);
		f_nv = BDD_NOT(reg_f->E);
	    } else {
		f_v = reg_f->T;
		f_nv = reg_f->E;
	    }
	} else {
	    /*
	     *    If f's variable is not varId, then no work need be done.
	     */
	    f_v = f;
	    f_nv = f;
	}

	BDD_ASSERT_NOTNIL(f_v);
	BDD_ASSERT_NOTNIL(f_nv);
}
    {
	bdd_node *reg_g;

	BDD_ASSERT_NOTNIL(g);

	reg_g = BDD_REGULAR(g);

	if (reg_g->id == varId) {
	    /*
	     *    If g's variable is varId, then do a bit
	     *    of work to cofactor the bdd at g
	     */
	    BDD_ASSERT( ! BDD_IS_CONSTANT(manager, g) );	/* can't do this on zero or one */

	    if (BDD_IS_COMPLEMENT(g)) {
		g_v = BDD_NOT(reg_g->T);
		g_nv = BDD_NOT(reg_g->E);
	    } else {
		g_v = reg_g->T;
		g_nv = reg_g->E;
	    }
	} else {
	    /*
	     *    If g's variable is not varId, then no work need be done.
	     */
	    g_v = g;
	    g_nv = g;
	}

	BDD_ASSERT_NOTNIL(g_v);
	BDD_ASSERT_NOTNIL(g_nv);
}
    {
	bdd_node *reg_h;

	BDD_ASSERT_NOTNIL(h);

	reg_h = BDD_REGULAR(h);

	if (reg_h->id == varId) {
	    /*
	     *    If h's variable is varId, then do a bit
	     *    of work to cofactor the bdd at h
	     */
	    BDD_ASSERT( ! BDD_IS_CONSTANT(manager, h) );	/* can't do this on zero or one */

	    if (BDD_IS_COMPLEMENT(h)) {
		h_v = BDD_NOT(reg_h->T);
		h_nv = BDD_NOT(reg_h->E);
	    } else {
		h_v = reg_h->T;
		h_nv = reg_h->E;
	    }
	} else {
	    /*
	     *    If h's variable is not varId, then no work need be done.
	     */
	    h_v = h;
	    h_nv = h;
	}

	BDD_ASSERT_NOTNIL(h_v);
	BDD_ASSERT_NOTNIL(h_nv);
}
#endif /* } */
	
	ite_T_status = bdd__ITE_constant(manager, f_v, g_v, h_v);
	if (ite_T_status == bdd_nonconstant) {
	    /*
	     *    Its a nonconstant
	     */
	    manager->heap.stats.ITE_constant_ops.returns.full++;
	    (void) bdd_constcache_insert(manager, f, g, h, bdd_nonconstant);
	    return (bdd_nonconstant);
	}
	  
	ite_E_status = bdd__ITE_constant(manager, f_nv, g_nv, h_nv);
	if (ite_E_status == bdd_nonconstant || ite_T_status != ite_E_status) {
	    /*
	     *    Its a nonconstant again ...
	     */
	    manager->heap.stats.ITE_constant_ops.returns.full++;
	    (void) bdd_constcache_insert(manager, f, g, h, bdd_nonconstant);
	    return (bdd_nonconstant);
	}

	ret_status = ite_T_status;	/* or equivalently ite_E_status */

	(void) bdd_constcache_insert(manager, f, g, h, ret_status);

	manager->heap.stats.ITE_constant_ops.returns.full++;

#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN)  /* { */
	    return (grock_status(ret_status, complement));
#else /* } else { */
	if (complement) {
	    switch (ret_status) {
	    case bdd_constant_zero:
		ret_status = bdd_constant_one;
		break;
	    case bdd_constant_one:
		ret_status = bdd_constant_zero;
		break;
	    case bdd_nonconstant:
		/* leave it alone */
		break;
	    }
	}

	return (ret_status);
#endif /* } */
}

static bdd_constant_status
grock_constantness(manager, f, complement)
bdd_manager *manager;
bdd_node *f;
boolean complement;
{
	bdd_constant_status status;

	if (f == BDD_ZERO(manager)) {
	    status = bdd_constant_zero;
	} else if (f == BDD_ONE(manager)) {
	    status = bdd_constant_one;
	} else {
	    status = bdd_nonconstant;
	}

	return (grock_status(status, complement));
}

static bdd_constant_status
grock_status(status, complement)
bdd_constant_status status;
boolean complement;
{
	if (complement) {
	    switch (status) {
	    case bdd_constant_zero:
		status = bdd_constant_one;
		break;
	    case bdd_constant_one:
		status = bdd_constant_zero;
		break;
	    case bdd_nonconstant:
		/* leave it alone */
		break;
	    }
	}

	return (status);
}
