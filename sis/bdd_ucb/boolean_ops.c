
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



/*
 *    bdd_and - returns the and of two bdd's
 *
 *    bdd_ITE Identity: F * G  =  ITE(F, G, 0)
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_and(f, g, f_phase, g_phase)
bdd_t *f;
bdd_t *g;
boolean f_phase;		/* if ! f_phase then negate f */
boolean g_phase;		/* if ! g_phase then negate g */
{
    bdd_safeframe frame;
    bdd_safenode real_f, real_g, ret;
    bdd_manager *manager;
    bdd_t *h;

    if (f == NIL(bdd_t) || g == NIL(bdd_t))
	fail("bdd_and: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );

    if (f->bdd != g->bdd)
	fail("bdd_and: different bdd managers");

    manager = f->bdd;		/* either this or g->bdd will do */

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, real_f);
    bdd_safenode_declare(manager, real_g);
    bdd_safenode_declare(manager, ret);

    real_f.node = f_phase ? f->node: BDD_NOT(f->node);
    real_g.node = g_phase ? g->node: BDD_NOT(g->node);

    ret.node = bdd__ITE_(manager, real_f.node, real_g.node, BDD_ZERO(manager));

    /*
     *    End the safe frame and return the result
     */
    bdd_safeframe_end(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_and");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_and(%d, %d)\n",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g));
#endif /* } */

    return (h);
}

/*
 *    bdd_not - returns the not of the given bdd.
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_not(f)
bdd_t *f;
{
    bdd_manager *manager;
    bdd_t *g;

    if (f == NIL(bdd_t))
	fail("bdd_not: invalid BDD");

    BDD_ASSERT( ! f->free );

    manager = f->bdd;

    /* WATCHOUT - no safe frame is declared here (b/c its not needed) */

    g = bdd_make_external_pointer(f->bdd, BDD_NOT(f->node), "bdd_not");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_not(%d)\n",
	    bdd_index_of_external_pointer(g),
	    bdd_index_of_external_pointer(f));
#endif /* } */

    return (g);
}

/*
 *    bdd_or - returns the or of two bdd's
 *
 *    bdd_ITE Identity:  F + G  =  ITE(F, 1, G)
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_or(f, g, f_phase, g_phase)
bdd_t *f;
bdd_t *g;
boolean f_phase;		/* if ! f_phase then negate f */
boolean g_phase;		/* if ! g_phase then negate g */
{
    bdd_safeframe frame;
    bdd_safenode real_f, real_g, ret;
    bdd_manager *manager;
    bdd_t *h;
	
    if (f == NIL(bdd_t) || g == NIL(bdd_t))
	fail("bdd_or: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );

    if (f->bdd != g->bdd)
	fail("bdd_or: different bdd managers");

    manager = f->bdd;		/* either this or g->bdd will do */

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, real_f);
    bdd_safenode_declare(manager, real_g);
    bdd_safenode_declare(manager, ret);

    real_f.node = f_phase ? f->node: BDD_NOT(f->node);
    real_g.node = g_phase ? g->node: BDD_NOT(g->node);

    ret.node = bdd__ITE_(manager, real_f.node, BDD_ONE(manager), real_g.node);

    /*
     *    End the safe frame and return the result
     */
    bdd_safeframe_end(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_or");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_or(%d, %d)\n",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g));
#endif /* } */

    return (h);
}

/*
 *    bdd_xor - returns the xor of two bdd's
 *
 *    bdd_ITE Identity: F xor G  =  ITE(F, !G, G)
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_xor(f, g)		/* F xor G  =  ITE(F, !G, G) */
bdd_t *f;
bdd_t *g;
{
    bdd_safeframe frame;
    bdd_safenode ret;
    bdd_manager *manager;
    bdd_t *h;

    if (f == NIL(bdd_t) || g == NIL(bdd_t))
	fail("bdd_xor: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );

    if (f->bdd != g->bdd)
	fail("bdd_xor: different bdd managers");

    manager = f->bdd;		/* either this or g->bdd will do */

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, ret);

    ret.node = bdd__ITE_(manager, f->node, BDD_NOT(g->node), g->node);

    /*
     *    End the safe frame and return the result
     */
    bdd_safeframe_end(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_xor");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_xor(%d, %d)\n",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g));
#endif /* } */

    return (h);
}

/* 
 *    bdd_xnor - returns the xnor of two bdd's
 *
 *    bdd_ITE Identity: !(F xor G)  =  ITE(F, G, !G)
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_xnor(f, g)		/* !(F xor G)  =  ITE(F, G, !G) */
bdd_t *f;
bdd_t *g;
{
	bdd_safeframe frame;
	bdd_safenode ret;
	bdd_manager *manager;
	bdd_t *h;

	if (f == NIL(bdd_t) || g == NIL(bdd_t))
	    fail("bdd_xnor: invalid BDD");

	if (f->bdd != g->bdd)
	    fail("bdd_xnor: different bdd managers");

	manager = f->bdd;	/* either this or g->bdd will do */

	/*
	 *    After the input is checked for correctness, start the safe frame
	 *    f and g are already external pointers so they need not be protected
	 */
	bdd_safeframe_start(manager, frame);
	bdd_safenode_declare(manager, ret);

	ret.node = bdd__ITE_(manager, f->node, g->node, BDD_NOT(g->node));

	/*
	 *    End the safe frame and return the result
	 */
	bdd_safeframe_end(manager);
	h = bdd_make_external_pointer(manager, ret.node, "bdd_xnor");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_xnor(%d, %d)\n",
	    bdd_index_of_external_pointer(h),
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g));
#endif /* } */

	return (h);
}

/*
 *    bdd_one - return a new copy of the 'one' constant
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_one(manager)
bdd_manager *manager;
{
	bdd_t *one;

	if (manager == NIL(bdd_manager))
	    fail("bdd_one: bad bdd manager");

	/* WATCHOUT - no safe frame is declared here (b/c its not needed) */

	one = bdd_make_external_pointer(manager, BDD_ONE(manager), "bdd_one");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_one(manager)\n",
	    bdd_index_of_external_pointer(one));
#endif /* } */

	return (one);
}

/*
 *    bdd_zero - return a new copy of the 'zero' constant
 *
 *    return the new manager (external pointer)
 */
bdd_t *
bdd_zero(manager)
bdd_manager *manager;
{
	bdd_t *zero;

	if (manager == NIL(bdd_manager))
	    fail("bdd_zero: bad bdd manager");

	/* WATCHOUT - no safe frame is declared here (b/c its not needed) */

	zero = bdd_make_external_pointer(manager, BDD_ZERO(manager), "bdd_zero");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_zero(manager)\n",
	    bdd_index_of_external_pointer(zero));
#endif /* } */

	return (zero);
}

/*
 *    bdd_is_tautology - is the bdd a tautology?
 *
 *    Just see if the bdd is equal to a constant (one or zero)
 *
 *    return {TRUE, FALSE} on {is, is not}.
 */
boolean
bdd_is_tautology(f, phase)
bdd_t *f;
boolean phase;
{
    bdd_manager *manager;
    bdd_node *constant;
    boolean ret;

    if (f == NIL(bdd_t))
	fail("bdd_is_tautology: invalid BDD");

    BDD_ASSERT( ! f->free );

    manager = f->bdd;

    /* WATCHOUT - no safe frame is declared here (b/c its not needed) */

    constant = phase ? BDD_ONE(f->bdd): BDD_ZERO(f->bdd);
    ret = f->node == constant ? TRUE: FALSE;

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "bdd_is_tautology(%d, %d)\n",
	    bdd_index_of_external_pointer(f), phase);
#endif /* } */

    return (ret);
}

/* 
 *    bdd_equal - whether the two bdds are the same
 *
 *    return {TRUE, FALSE} on {is, is not}.
 */
boolean
bdd_equal(f, g)
bdd_t *f;
bdd_t *g;
{
    bdd_manager *manager;
    boolean ret;

    if (f == NIL(bdd_t) || g == NIL(bdd_t))
	fail("bdd_equal: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );

    if (f->bdd != g->bdd)
	fail("bdd_equal: different bdd managers");

    manager = f->bdd;	/* or g->bdd */

    /* WATCHOUT - no safe frame is declared here (b/c its not needed) */

    ret = f->node == g->node ? TRUE: FALSE;

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "bdd_equal(%d, %d)\n",
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g));
#endif /* } */

    return (ret);
}

/* 
 *    bdd_leq - returns the leq of two bdd's
 *
 *    ITE Identity: f implies g  <=>  ITE_constant(F, G, 1) = 1
 *
 *    return {TRUE, FALSE} on {is, is not}.
 */
boolean
bdd_leq(f, g, f_phase, g_phase)
bdd_t *f;
bdd_t *g;
boolean f_phase;		/* if ! f_phase then negate f */
boolean g_phase;		/* if ! g_phase then negate g */
{
    bdd_safeframe frame;
    bdd_safenode real_f, real_g;
    bdd_manager *manager;
    boolean ret;

    if (f == NIL(bdd_t) || g == NIL(bdd_t))
	fail("bdd_leq: invalid BDD");

    BDD_ASSERT( ! f->free );
    BDD_ASSERT( ! g->free );

    if (f->bdd != g->bdd)
	fail("bdd_leq: different bdd managers");

    manager = f->bdd;		/* either this or g->bdd will do */

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, real_f);
    bdd_safenode_declare(manager, real_g);

    real_f.node = f_phase ? f->node: BDD_NOT(f->node);
    real_g.node = g_phase ? g->node: BDD_NOT(g->node);

    ret = bdd__ITE_constant(manager, real_f.node, real_g.node, BDD_ONE(manager)) == bdd_constant_one ? TRUE: FALSE;

    bdd_safeframe_end(manager);

#if defined(BDD_FLIGHT_RECORDER)	/* { */
    (void) fprintf(manager->debug.flight_recorder.log, "bdd_leq(%d, %d, %d, %d)\n",
	    bdd_index_of_external_pointer(f),
	    bdd_index_of_external_pointer(g),
	    f_phase, g_phase);
#endif				/* } */

    return (ret);
}
