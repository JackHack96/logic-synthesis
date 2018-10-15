/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_print.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
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
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_print.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: bdd_print.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.3  1992/09/21  23:30:31  sis
 * Updates from Tom Shiple - this is BDD package release 2.4.
 *
 * Revision 1.2  1992/09/19  02:41:07  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_. Added typecast to void to some function calls.
 * Fixed bug in function id_name.  For case where BDD_DEBUG_UID was true, node
 * pointer was not being regularized before it was dereferenced.
 *
 * Revision 1.1  1992/07/29  00:26:51  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:28  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:33  shiple
 * Initial revision
 * 
 *
 */

static void print();
static string id_name();

/*
 *    bdd_print - print a bdd
 *
 *    return nothing, just do it.
 */
void
bdd_print(f)
bdd_t *f;
{
    bdd_manager *manager;
    st_table *table;		/* used to ensure that we don't print anything twice */
    int j;
	
    if (f == NIL(bdd_t))
	fail("bdd_print: invalid BDD");

    BDD_ASSERT( ! f->free );

    /* WATCHOUT - no safe frame is declared here (b/c its not needed) */

    manager = f->bdd;

    for (j=0; j<manager->bdd.nvariables; j++) {
	char variable_name[50];	/* actually only for mis-independent operation */

	(void) sprintf(variable_name, "v#%d", j);
	(void) fprintf(stdout, "\tindex %d is %s\n", j, variable_name);
    }

    table = st_init_table(st_ptrcmp, st_ptrhash);
    (void) print(f->node, manager, table);
    (void) st_free_table(table);

    (void) fputc('\n', stdout);
}

    
/* 
 *    print - bdd print recursively
 *
 *    return nothing, just do it.
 */
static void
print(f, manager, table)
bdd_node *f;		/* the node to print */
bdd_manager *manager;
st_table *table;	/* table recording what has been printed so far */
{
	bdd_node *f_reg;
	boolean T, E;

	/*
	 *    This should never occur
	 */
	if (f == NIL(bdd_node))
	    return;

	/*
	 *    If we've reached a constant, then we're done
	 */
	f_reg = BDD_REGULAR(f);
	if (f_reg == BDD_ONE(manager)) {
	    (void) fprintf(stdout, "ID =  %d\n", f == BDD_ONE(manager) ? 1: 0);
	    return;
	}

	/*
	 *    If it was already printed, don't continue any further
	 */
	if (st_is_member(table, (refany) f_reg))
	    return;

	(void) st_insert(table, (refany) f_reg, NIL(any));

	(void) fprintf(stdout, "ID = %s\tindex = %d\t", id_name(f), f_reg->id);

	if (BDD_IS_CONSTANT(manager, f_reg->T)) {
	    (void) fprintf(stdout, "T =  %d\t\t", f_reg->T == BDD_ONE(manager) ? 1: 0);
	    T = TRUE;
	} else {
	    (void) fprintf(stdout, "T =  %s\t", id_name(f_reg->T));
	    T = FALSE;
	}

	if (BDD_IS_CONSTANT(manager, f_reg->E)) {
	    (void) fprintf(stdout, "E =  %d\n", f_reg->E == BDD_ONE(manager) ? 1: 0);
	    E = TRUE;
	} else {
	    (void) fprintf(stdout, "E = %s\n", id_name(f_reg->E));
	    E = FALSE;
	}

	/*
	 *    Recur if appropriate
	 */
	if ( ! E )
	    (void) print(BDD_REGULAR(f_reg->E), manager, table);
	if ( ! T )
	    (void) print(BDD_REGULAR(f_reg->T), manager, table);
}

static string
id_name(f)
bdd_node *f;
{
    static char buf[100];
    int uid;
    bdd_node *f_reg;

    f_reg = BDD_REGULAR(f);

#if defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    uid = f_reg->uniqueId;
#else /* } else { */
    uid = (int) f_reg / sizeof(bdd_node);
#endif /* } */

    sprintf(buf, "%c%#x", BDD_IS_COMPLEMENT(f) ? '!' : ' ', uid);

    return (buf);
}
