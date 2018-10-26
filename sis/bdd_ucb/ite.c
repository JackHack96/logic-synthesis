
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



/*
 *    bdd__ITE_ - perform the ITE recursion
 *
 *    WATCHOUT - do NOT use ``return (r)'' in here unless bdd_safeframe_end()
 *    has been used to deallocate the safeframe.   Unfortunately we are not
 *    using C++ and so we cannot have the compiler do this for us.  This routine
 *    must be coded carefully to avoid not deallocating frames and the like.
 *    The whole goal here is to safeguard ourselves in case the garbage collector
 *    is called during the (long) recursive call to bdd__ITE_()
 *
 *    ITE Identities
 *        ITE(1, G, H) = G
 *        ITE(0, G, H) = H
 *        ITE(F, G, G) = G
 *        ITE(F, 1, 0) = F
 *        ... and others in canonicalize_ite_inputs ...
 *
 *    return the canonical bdd_node* held by the manager
 */
bdd_node *
bdd__ITE_(manager, f, g, h)
bdd_manager *manager;
bdd_node *f;
bdd_node *g;
bdd_node *h;
{
    bdd_safeframe frame;
    bdd_safenode safe_f, safe_g, safe_h;
    bdd_safenode ret;
    bdd_safenode reg_f, f_v, f_nv;
    bdd_safenode reg_g, g_v, g_nv;
    bdd_safenode reg_h, h_v, h_nv;
    bdd_safenode ite_T, ite_E;
    bdd_variableId varId;
    boolean complement;

    BDD_ASSERT_NOTNIL(f);
    BDD_ASSERT_NOTNIL(g);
    BDD_ASSERT_NOTNIL(h);

    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_f, f);
    bdd_safenode_link(manager, safe_g, g);
    bdd_safenode_link(manager, safe_h, h);
    bdd_safenode_declare(manager, ret);
    bdd_safenode_declare(manager, reg_f);
    bdd_safenode_declare(manager, f_v);
    bdd_safenode_declare(manager, f_nv);
    bdd_safenode_declare(manager, reg_g);
    bdd_safenode_declare(manager, g_v);
    bdd_safenode_declare(manager, g_nv);
    bdd_safenode_declare(manager, reg_h);
    bdd_safenode_declare(manager, h_v);
    bdd_safenode_declare(manager, h_nv);
    bdd_safenode_declare(manager, ite_T);
    bdd_safenode_declare(manager, ite_E);

    manager->heap.stats.ITE_ops.calls++;

    /* 
     * Handle the trivial cases when f is a constant.
     */
    if (f == BDD_ONE(manager)) {
	/*
	 *    ITE(1, G, H) = G
	 */
	manager->heap.stats.ITE_ops.returns.trivial++;
	bdd_safeframe_end(manager);
	return (g);
    }
    if (f == BDD_ZERO(manager)) {
	/*
	 *    ITE(0, G, H) = H
	 */
	manager->heap.stats.ITE_ops.returns.trivial++;
	bdd_safeframe_end(manager);
	return (h);
    }

#if ! defined(BDD_INLINE_ITE) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    (void) bdd_ite_var_to_const(manager, f, &g, &h);
#else				/* } else { */
    BDD_ASSERT_NOTNIL(f);

    /*
     *    Equivalent to call to bdd_ite_var_to_const(manager, f, &g, &h).
     *    See bdd_ite_var_to_const for documentation.
     */
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
	 *    ITE(F, G, G) = G
	 */
	manager->heap.stats.ITE_ops.returns.trivial++;
	bdd_safeframe_end(manager);
	return (g);
    }
    if (g == BDD_ONE(manager) && h == BDD_ZERO(manager)) {
	/*
	 *    ITE(F, 1, 0) = F
	 */
	manager->heap.stats.ITE_ops.returns.trivial++;
	bdd_safeframe_end(manager);
	return (f);
    }
    if (g == BDD_ZERO(manager) && h == BDD_ONE(manager)) {
	/*
	 *    ITE(F, 0, 1) = !F
	 */
	manager->heap.stats.ITE_ops.returns.trivial++;
	bdd_safeframe_end(manager);
	return (BDD_NOT(f));
    }

    /*
     *    It's not the case that both g and h are are constants; thus,
     *    at most one is a constant (either g or h).
     */

#if ! defined(BDD_INLINE_ITE) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    (void) bdd_ite_canonicalize_ite_inputs(manager, &f, &g, &h, &complement);
#else				/* } else { */
    BDD_ASSERT_NOTNIL(f);
    BDD_ASSERT_NOTNIL(g);
    BDD_ASSERT_NOTNIL(h);

    /*
     *    Equivalent to call to bdd_ite_canonicalize_ite_inputs(manager, &f, &g, &h, &complement).
     *    See bdd_ite_canonicalize_ite_inputs for documentation.  No function calls are made here
     *    so that the code is as fast as possible.
     */
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
		  tmp = h;
		  h = f;
		  f = tmp; }
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
#endif				/* } */

    BDD_ASSERT( ! BDD_IS_COMPLEMENT(f) );
    BDD_ASSERT( ! BDD_IS_COMPLEMENT(g) );
    /* ASSERT: BDD_IS_COMPLEMENT(h) || ! BDD_IS_COMPLEMENT(h) */

    reg_f.node = BDD_REGULAR(f);
    reg_g.node = BDD_REGULAR(g);
    reg_h.node = BDD_REGULAR(h);

    /*
     *    A shortcut can be noted here:
     *
     *    ITE(F, G, H) = (v, G, H), if F = (v, 1, 0) and  v < top(G, H).
     */
    varId = MIN(reg_g.node->id, reg_h.node->id);

    if (reg_f.node->id < varId &&
	reg_f.node->T == BDD_ONE(manager) &&
	reg_f.node->E == BDD_ZERO(manager)) {
	/*
	 *     f is the trivial case: ITE(v, 1, 0)
	 */
	manager->heap.stats.ITE_ops.returns.trivial++;
	ret.node = bdd_find_or_add(manager, reg_f.node->id, g, h);
	bdd_safeframe_end(manager);
	return ( ! complement ? ret.node: BDD_NOT(ret.node));
    }

    /*
     *    Perform a cache lookup; if it was a hit, then return straightaway.
     */
    if (bdd_hashcache_lookup(manager, f, g, h, &ret.node)) {
	/*
	 *    A hit, so blow off the frame, and return (complementing or not).
	 */
	manager->heap.stats.ITE_ops.returns.cached++;
	bdd_safeframe_end(manager);
	return ( ! complement ? ret.node: BDD_NOT(ret.node));
    }

    /*
     *    Caching didn't work out for us so we need to go and do the recursive step.
     */

    varId = MIN3(reg_f.node->id, reg_g.node->id, reg_h.node->id);

    /*
     *    The pattern:
     *        f_v	- f cofactored by v
     *        f_nv	- f cofactored by v bar
     */
#if ! defined(BDD_INLINE_ITE_CONSTANT) || defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    (void) bdd_ite_quick_cofactor(manager, f, varId, &(f_v.node), &(f_nv.node));
    (void) bdd_ite_quick_cofactor(manager, g, varId, &(g_v.node), &(g_nv.node));
    (void) bdd_ite_quick_cofactor(manager, h, varId, &(h_v.node), &(h_nv.node));
#else				/* } else { */
    
    /*
     *    Equivalent to bdd_ite_quick_cofactor(manager, f, varId, &f_v, &f_nv);
     */
    {
	bdd_node *reg_f;

	BDD_ASSERT_NOTNIL(f);

	reg_f = BDD_REGULAR(f);

	if (reg_f->id == varId) {
	    /*
	     *    If f's variable is varId, then do a bit
	     *    of work to cofactor the bdd at f
	     */
	    BDD_ASSERT( ! BDD_IS_CONSTANT(manager, f) ); /* can't do this on zero or one */

	    if (BDD_IS_COMPLEMENT(f)) {
		f_v.node = BDD_NOT(reg_f->T);
		f_nv.node = BDD_NOT(reg_f->E);
	    } else {
		f_v.node = reg_f->T;
		f_nv.node = reg_f->E;
	    }
	} else {
	    /*
	     *    If f's variable is not varId, then no work need be done.
	     */
	    f_v.node = f;
	    f_nv.node = f;
	}

	BDD_ASSERT_NOTNIL(f_v.node);
	BDD_ASSERT_NOTNIL(f_nv.node);
    }
    
    /*
     *    Equivalent to bdd_ite_quick_cofactor(manager, g, varId, &g_v, &g_nv);
     */
    {
	bdd_node *reg_g;

	BDD_ASSERT_NOTNIL(g);

	reg_g = BDD_REGULAR(g);

	if (reg_g->id == varId) {
	    /*
	     *    If g's variable is varId, then do a bit
	     *    of work to cofactor the bdd at g
	     */
	    BDD_ASSERT( ! BDD_IS_CONSTANT(manager, g) ); /* can't do this on zero or one */

	    if (BDD_IS_COMPLEMENT(g)) {
		g_v.node = BDD_NOT(reg_g->T);
		g_nv.node = BDD_NOT(reg_g->E);
	    } else {
		g_v.node = reg_g->T;
		g_nv.node = reg_g->E;
	    }
	} else {
	    /*
	     *    If g's variable is not varId, then no work need be done.
	     */
	    g_v.node = g;
	    g_nv.node = g;
	}

	BDD_ASSERT_NOTNIL(g_v.node);
	BDD_ASSERT_NOTNIL(g_nv.node);
    }
    
    /*
     *    Equivalent to bdd_ite_quick_cofactor(manager, h, varId, &h_v, &h_nv);
     */
    {
	bdd_node *reg_h;

	BDD_ASSERT_NOTNIL(h);

	reg_h = BDD_REGULAR(h);

	if (reg_h->id == varId) {
	    /*
	     *    If h's variable is varId, then do a bit
	     *    of work to cofactor the bdd at h
	     */
	    BDD_ASSERT( ! BDD_IS_CONSTANT(manager, h) ); /* can't do this on zero or one */

	    if (BDD_IS_COMPLEMENT(h)) {
		h_v.node = BDD_NOT(reg_h->T);
		h_nv.node = BDD_NOT(reg_h->E);
	    } else {
		h_v.node = reg_h->T;
		h_nv.node = reg_h->E;
	    }
	} else {
	    /*
	     *    If h's variable is not varId, then no work need be done.
	     */
	    h_v.node = h;
	    h_nv.node = h;
	}

	BDD_ASSERT_NOTNIL(h_v.node);
	BDD_ASSERT_NOTNIL(h_nv.node);
    }
#endif				/* } */
	
    ite_T.node = bdd__ITE_(manager, f_v.node, g_v.node, h_v.node);
    ite_E.node = bdd__ITE_(manager, f_nv.node, g_nv.node, h_nv.node);

    if (ite_T.node == ite_E.node) {
	ret.node = ite_T.node;	/* either ite_T or ite_E */
    } else {
	ret.node = bdd_find_or_add(manager, varId, ite_T.node, ite_E.node);
    }

    (void) bdd_hashcache_insert(manager, f, g, h, ret.node);

    BDD_ASSERT_NOTNIL(ret.node);

    /*
     *    complement refers to how the user should refer to this bdd;
     *    if complement is set then the caller will refer to this bdd
     *    by its complemented name, otherwise by the uncomplemented name
     */
    if (complement) {
	ret.node = BDD_NOT(ret.node);
    }

    manager->heap.stats.ITE_ops.returns.full++;
    bdd_safeframe_end(manager);
    return (ret.node);
}
