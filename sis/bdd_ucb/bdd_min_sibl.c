
#include <stdio.h>    /* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"
#include "bdd.h"
#include "bdd_int.h"


static bdd_node *min_sibling();

/*
 * bdd_minimize_with_params - perform minimization on the incompletely specified 
 * function [f,c] using match_type in {BDD_MIN_TSM, BDD_MIN_OSM, BDD_MIN_OSDM}.
 * The compl flag indicates whether the complement of one child should be matched
 * to the other child.  When the no_new_vars flag is TRUE, new variables will not 
 * be introduced into f.
 *
 * return the new bdd (external pointer)
 */
bdd_t *
bdd_minimize_with_params(f, c, match_type, compl, no_new_vars, return_min)
        bdd_t *f;
        bdd_t *c;
        bdd_min_match_type_t match_type;
        boolean compl;
        boolean no_new_vars;
        boolean return_min;
{
    bdd_safeframe frame;
    bdd_safenode  ret;
    bdd_manager   *manager;
    bdd_t         *h;
    int           size_h, size_f;

    if (f == NIL(bdd_t) || c == NIL(bdd_t)) {
        fail("bdd_minimize_with_params: invalid BDD");
    }

    BDD_ASSERT(!f->free);
    BDD_ASSERT(!c->free);

    if (f->bdd != c->bdd) {
        fail("bdd_minimize_with_params: different bdd managers");
    }

    manager = f->bdd;        /* either this or c->bdd will do */

    /*
     * Start the safe frame now that we know the input is correct.
     * f and c are external pointers so they need not be protected.
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, ret);

    bdd_adhoccache_init(manager);

    ret.node = min_sibling(manager, f->node, c->node, match_type, compl, no_new_vars);

    /*
     * Free the cache, end the safe frame and return the (safe) result
     */
    bdd_adhoccache_uninit(manager);
    h = bdd_make_external_pointer(manager, ret.node, "bdd_minimize_with_params");
    bdd_safeframe_end(manager);

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_minimize_with_params(%d, %d)\n",
        bdd_index_of_external_pointer(f),
        bdd_index_of_external_pointer(c));
#endif /* } */

    /*
     * If return_min is TRUE and if h is larger than f, then free h, and return a copy of f.
     */
    if (return_min == TRUE) {
        size_h = bdd_size(h);
        size_f = bdd_size(f);
        if (size_h > size_f) {
            bdd_free(h);
            h = bdd_dup(f);
        }
    }

    return (h);
}

/*
 * bdd_minimize - call bdd_minimize_with_params with default parameters.
 *
 * return the new bdd (external pointer)
 */
bdd_t *
bdd_minimize(f, c)
        bdd_t *f;
        bdd_t *c;
{
    return (bdd_minimize_with_params(f, c, BDD_MIN_OSM, TRUE, TRUE, TRUE));
}

/*
 * bdd_between - return a small BDD between f_min and f_max.
 *
 * return the new bdd (external pointer)
 */
bdd_t *
bdd_between(f_min, f_max)
        bdd_t *f_min;
        bdd_t *f_max;
{
    bdd_t *temp, *ret;

    temp = bdd_or(f_min, f_max, 1, 0);
    ret  = bdd_minimize(f_min, temp);
    bdd_free(temp);
    return ret;
}

/*
 *    min_sibling - recursively perform the min_siblinging
 *
 *    return the result of the reorganization
 */
static bdd_node *
min_sibling(mgr, f, c, mtype, compl, no_new_vars)
        bdd_manager *mgr;
        bdd_node *f;
        bdd_node *c;
        bdd_min_match_type_t mtype;
        boolean compl;
        boolean no_new_vars;
{
    bdd_safeframe  frame;
    bdd_safenode   safe_f, safe_c,
                   ret,
                   f_T, f_E,
                   c_T, c_E,
                   var, co_T, co_E,
                   new_f, new_c, sum;
    bdd_variableId fId, cId, topId;
    boolean        match_made;

    mgr->heap.stats.adhoc_ops.calls++;

    if (BDD_IS_CONSTANT(mgr, f)) {
        /*
         *  f is either zero or one, so just return f.
         */
        mgr->heap.stats.adhoc_ops.returns.trivial++;
        return (f);
    }

    if (c == BDD_ONE(mgr)) {
        /*
         * The care function is the tautology, so just return f.
         */
        mgr->heap.stats.adhoc_ops.returns.trivial++;
        return (f);
    }

    assert(c != BDD_ZERO(mgr));  /* operation is undefined for don't care everywhere */

    /*
     * Check if we have already computed the result for f,c.
     */
    if (bdd_adhoccache_lookup(mgr, f, c, /* v */ 0, &ret.node)) {
        /*
         *    The answer was already in the cache, so just return it.
         */
        mgr->heap.stats.adhoc_ops.returns.cached++;
        return (ret.node);
    }

    /*
     * Start the safeframe.
     */
    bdd_safeframe_start(mgr, frame);
    bdd_safenode_link(mgr, safe_f, f);
    bdd_safenode_link(mgr, safe_c, c);
    bdd_safenode_declare(mgr, ret);
    bdd_safenode_declare(mgr, f_T);
    bdd_safenode_declare(mgr, f_E);
    bdd_safenode_declare(mgr, c_T);
    bdd_safenode_declare(mgr, c_E);
    bdd_safenode_declare(mgr, var);
    bdd_safenode_declare(mgr, co_T);
    bdd_safenode_declare(mgr, co_E);
    bdd_safenode_declare(mgr, new_f);
    bdd_safenode_declare(mgr, new_c);
    bdd_safenode_declare(mgr, sum);

    /*
     * Dereference the id of the top node, and the then and else branches, for each of f and c.
     */
    fId = BDD_REGULAR(f)->id;
    cId = BDD_REGULAR(c)->id;
    bdd_get_branches(f, &f_T.node, &f_E.node);
    bdd_get_branches(c, &c_T.node, &c_E.node);

    /*
     * Based on topId, determine the then and else branches to use for the recursion, for f and c.
     */
    topId = MIN(fId, cId);
    if (topId < cId) {
        /* c is independent of fId; thus, the two cofactors of c by fId are the same */
        c_T.node = c;
        c_E.node = c;
    }
    if (topId < fId) {
        /* f is independent of cId; thus, the two cofactors of f by cId are the same */
        f_T.node = f;
        f_E.node = f;
    }

    /*
     * Try to match functions in a predetermined order.  If a match is made, then skip the
     * rest of the match attempts.  Using multiple IFs and the match_made flag allows for easier
     * to understand code.
     */
    match_made = FALSE;
    if ((fId > cId) && (no_new_vars == TRUE)) {
        /*
         *  f is independent of cId; keep it that way
         */
        match_made = TRUE;
        sum.node = bdd__ITE_(mgr, c_T.node, BDD_ONE(mgr), c_E.node);
        ret.node = min_sibling(mgr, f, sum.node, mtype, compl, no_new_vars);
    }

    if (match_made == FALSE) {
        if (bdd_min_is_match(mgr, mtype, FALSE, f_T.node, c_T.node, f_E.node, c_E.node)) {
            /*
             * Match T to E.
             */
            match_made = TRUE;
            bdd_min_match_result(mgr, mtype, FALSE, f_T.node, c_T.node, f_E.node, c_E.node, &new_f.node, &new_c.node);
            ret.node = min_sibling(mgr, new_f.node, new_c.node, mtype, compl, no_new_vars);
        }
    }

    if ((match_made == FALSE) && (mtype != BDD_MIN_TSM)) {  /* BDD_MIN_TSM is symmetric */
        if (bdd_min_is_match(mgr, mtype, FALSE, f_E.node, c_E.node, f_T.node, c_T.node)) {
            /*
             * Match E to T.
             */
            match_made = TRUE;
            bdd_min_match_result(mgr, mtype, FALSE, f_E.node, c_E.node, f_T.node, c_T.node, &new_f.node, &new_c.node);
            ret.node = min_sibling(mgr, new_f.node, new_c.node, mtype, compl, no_new_vars);
        }
    }

    if ((match_made == FALSE) && (compl == TRUE)) {
        if (bdd_min_is_match(mgr, mtype, TRUE, f_T.node, c_T.node, f_E.node, c_E.node)) {
            /*
             * Match T to complement of E.
             */
            match_made = TRUE;
            bdd_min_match_result(mgr, mtype, TRUE, f_T.node, c_T.node, f_E.node, c_E.node, &new_f.node, &new_c.node);
            co_E.node = min_sibling(mgr, new_f.node, new_c.node, mtype, compl, no_new_vars);
            var.node  = bdd_find_or_add(mgr, topId, BDD_ONE(mgr), BDD_ZERO(mgr));
            ret.node  = bdd__ITE_(mgr, var.node, BDD_NOT(co_E.node), co_E.node);
        }
    }

    if ((match_made == FALSE) && (compl == TRUE) && (mtype != BDD_MIN_TSM)) {
        if (bdd_min_is_match(mgr, mtype, TRUE, f_E.node, c_E.node, f_T.node, c_T.node)) {
            /*
             * Match E to complement of T.
             */
            match_made = TRUE;
            bdd_min_match_result(mgr, mtype, TRUE, f_E.node, c_E.node, f_T.node, c_T.node, &new_f.node, &new_c.node);
            co_T.node = min_sibling(mgr, new_f.node, new_c.node, mtype, compl, no_new_vars);
            var.node  = bdd_find_or_add(mgr, topId, BDD_ONE(mgr), BDD_ZERO(mgr));
            ret.node  = bdd__ITE_(mgr, var.node, co_T.node, BDD_NOT(co_T.node));
        }
    }

    if (match_made == FALSE) {
        /*
         * No match can be made.  Compute ITE(topId, min_sibling(f_T, c_T), min_sibling(f_E, c_E)).
         */
        co_T.node = min_sibling(mgr, f_T.node, c_T.node, mtype, compl, no_new_vars);
        co_E.node = min_sibling(mgr, f_E.node, c_E.node, mtype, compl, no_new_vars);
        var.node  = bdd_find_or_add(mgr, topId, BDD_ONE(mgr), BDD_ZERO(mgr));
        ret.node  = bdd__ITE_(mgr, var.node, co_T.node, co_E.node);
    }

    /*
     * Insert the result, kill the safeframe, and return.
     */
    bdd_adhoccache_insert(mgr, f, c, /* v */ 0, ret.node);
    mgr->heap.stats.adhoc_ops.returns.full++;

    bdd_safeframe_end(mgr);
    return (ret.node);
}
