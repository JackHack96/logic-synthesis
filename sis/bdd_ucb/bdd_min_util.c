/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_min_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:02 $
 * $Log: bdd_min_util.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:02  pchong
 * imported
 *
 * Revision 1.1  1993/07/27  20:14:50  sis
 * Initial revision
 *
 * Revision 1.1  1993/07/27  19:31:15  shiple
 * Initial revision
 *
 *
 */
#include "util.h"
#include "array.h"
#include "st.h"
#include "bdd.h"
#include "bdd_int.h"

/*
 * determine if [f1,c1] matches [f2,c2] under the specified match type
 * in {BDD_MIN_TSM, BDD_MIN_OSM, BDD_MIN_OSDM}.
 * NOTE WELL: We are not calling any routines inside this function which 
 * would use the adhoc cache; doing so would interfere with the current use
 * of the adhoc cache.
 */
boolean
bdd_min_is_match(manager, match_type, compl, f1, c1, f2, c2)
bdd_manager *manager;
bdd_min_match_type_t match_type;
boolean compl;
bdd_node *f1;
bdd_node *c1;
bdd_node *f2;
bdd_node *c2;
{
    boolean ret, temp1, temp2;
    bdd_safeframe frame;
    bdd_safenode safe_f1, safe_c1, safe_f2, safe_c2, diff, rhs;

    /*
     * Set up the safeframe.
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_f1, f1);
    bdd_safenode_link(manager, safe_c1, c1);
    bdd_safenode_link(manager, safe_f2, f2);
    bdd_safenode_link(manager, safe_c2, c2);
    bdd_safenode_declare(manager, diff);
    bdd_safenode_declare(manager, rhs);

    /*
     * Perform the appropriate test, depending on the match_type and complement flag.
     */
    if (match_type == BDD_MIN_OSDM) {
	ret = (c1 == BDD_ZERO(manager));
    } else {
	if (compl == FALSE) {
	    diff.node = bdd__ITE_(manager, f1, BDD_NOT(f2), f2);  /* f1 XOR f2 */
	} else { /* compl == TRUE i.e. try to match f1 to complement of f2 */
	    diff.node = bdd__ITE_(manager, f1, f2, BDD_NOT(f2));  /* f1 XNOR f2 */
	}

	if (match_type == BDD_MIN_OSM) {
	    rhs.node = BDD_NOT(c1);
	    temp1 = (bdd__ITE_constant(manager, diff.node, rhs.node, BDD_ONE(manager)) == bdd_constant_one) 
		? TRUE : FALSE;  /* diff => rhs */
	    temp2 = (bdd__ITE_constant(manager, c1, c2, BDD_ONE(manager)) == bdd_constant_one) 
		? TRUE : FALSE;  /* c1 => c2, which is equiv to !c1 <= !c2 */
	    ret = temp1 && temp2;
        } else if (match_type == BDD_MIN_TSM) {
	    rhs.node = bdd__ITE_(manager, c1, BDD_NOT(c2), BDD_ONE(manager));  /* !c1 + !c2 */
	    ret = (bdd__ITE_constant(manager, diff.node, rhs.node, BDD_ONE(manager)) == bdd_constant_one) 
		? TRUE : FALSE;  /* diff => rhs */
        } else {
	    fail("HEUR_FAIL, bdd_min_is_match: illegal match type");
	}
    }

    /*
     * Kill the safeframe and return.
     */
    bdd_safeframe_end(manager);
    return (ret);
}


/*
 * return the result of matching [f1,c1] to [f2,c2] using match_type in 
 * {BDD_MIN_TSM, BDD_MIN_OSM, BDD_MIN_OSDM}.
 */
void
bdd_min_match_result(manager, match_type, compl, f1, c1, f2, c2, new_f, new_c)
bdd_manager *manager;
bdd_min_match_type_t match_type;
boolean compl;
bdd_node *f1;
bdd_node *c1;
bdd_node *f2;
bdd_node *c2;
bdd_node **new_f; /* return */
bdd_node **new_c; /* return */
{
    bdd_safeframe frame;
    bdd_safenode safe_f1, safe_c1, safe_f2, safe_c2, temp1, temp2;

    /*
     * Set up the safeframe.
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_link(manager, safe_f1, f1);
    bdd_safenode_link(manager, safe_c1, c1);
    bdd_safenode_link(manager, safe_f2, f2);
    bdd_safenode_link(manager, safe_c2, c2);
    bdd_safenode_declare(manager, temp1);
    bdd_safenode_declare(manager, temp2);

    /* 
     * Based on the matching type and complement flag, compute the new f and c.
     */
    if ((match_type == BDD_MIN_OSM) || (match_type == BDD_MIN_OSDM)) {
	*new_f = f2;
	*new_c = c2;
    } else if (match_type == BDD_MIN_TSM) {
	if (compl == FALSE) {
	    temp1.node = bdd__ITE_(manager, f1, c1, BDD_ZERO(manager));  /* f1 * c1 */
	} else { /* compl == TRUE i.e. match f1 to complement of f2 */
	    temp1.node = bdd__ITE_(manager, BDD_NOT(f1), c1, BDD_ZERO(manager));  /* !f1 * c1 */
	}
	
	temp2.node = bdd__ITE_(manager, f2, c2, BDD_ZERO(manager));  /* f2 * c2 */
	*new_f = bdd__ITE_(manager, temp1.node, BDD_ONE(manager), temp2.node);  /* temp1 + temp2 */
	*new_c = bdd__ITE_(manager, c1, BDD_ONE(manager), c2);  /* c1 + c2 */
    } else {
	fail("HEUR_FAIL, bdd_min_match_result: illegal match type");
    }

    /*
     * Kill the safeframe and return.
     */
    bdd_safeframe_end(manager);
    return;
}

/*
 * Recursively determine if f is a cube. f is a cube if there is a single
 * path to the constant one.
 */
static boolean
is_cube(manager, f)
bdd_manager *manager;
bdd_node *f;
{
    bdd_node *f0, *f1;

    BDD_ASSERT(f != BDD_ZERO(manager));

    if (f == BDD_ONE(manager)) {
	return TRUE;
    }

    bdd_get_branches(f, &f1, &f0);

    /*
     * Exactly one branch of f must point to ZERO to be a cube.
     */
    if (f1 == BDD_ZERO(manager)) {
	return (is_cube(manager, f0));
    } else if (f0 == BDD_ZERO(manager)) {
	return (is_cube(manager, f1));
    } else { /* not a cube, because neither branch is zero */
	return FALSE;
    }
}

/* 
 * Return TRUE if f is a cube, else return FALSE.
 */
boolean
bdd_is_cube(f)
bdd_t *f;
{
    bdd_manager *manager;

    if (f == NIL(bdd_t)) {
	fail("bdd_is_cube: invalid BDD");
    }
    BDD_ASSERT( ! f->free );

    manager = f->bdd;
    return (is_cube(manager, f->node));
}

