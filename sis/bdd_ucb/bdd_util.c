/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_util.c,v $
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
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: bdd_util.c,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.7  1993/06/30  00:14:37  shiple
 * Added definition of new function bdd_dynamic_reordering.
 *
 * Revision 1.6  1993/06/28  22:34:05  shiple
 * Added definition of new function bdd_num_vars.
 *
 * Revision 1.5  1993/06/22  00:42:45  shiple
 * Made it illegal to free a bdd_t more than once.
 *
 * Revision 1.4  1993/06/04  15:42:51  shiple
 * Added functions bdd_get_manager, bdd_get_external_hooks, bdd_top_var_id, and bdd_get_node.
 *
 * Revision 1.3  1993/01/11  23:48:44  shiple
 * Incorrect comment in function bdd_size removed.
 *
 * Revision 1.2  1992/09/19  02:50:47  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_. Added typecast to void to some function calls.
 * Subtracted one from return value of bdd_size because it was wrong.  Added get_mem_stats function.
 * In bdd_get_branches, return NIL for then and else pointers if argument is the constant 0.
 * Added bdd_will_exceed_mem_limit function.
 *
 * Revision 1.1  1992/07/29  00:26:55  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:34  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:36  shiple
 * Initial revision
 * 
 *
 */

#define BDD_ONE_MEGABYTE 1048576

static void get_mem_stats();

/*
 *    bdd_create_variable - create a new variable and add it to the bdd
 *
 *    A bdd formula of the form (v, 1, 0) is returned.
 *    The 'v' is a unique new variable.
 *
 *    return a bdd_t*
 */
bdd_t *
bdd_create_variable(manager)
bdd_manager *manager;
{
    bdd_t *var;
    
    var = bdd_get_variable(manager, manager->bdd.nvariables++);

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_create_variable(manager)\n",
	    bdd_index_of_external_pointer(var));
#endif /* } */

    return (var);
}

/*
 *    bdd_dup - return a copy of the given bdd
 *
 *    Actually what this does is allocates a new external
 *    pointer for the user.  It is the user's responsibility
 *    then to manage the allocation and deallocation of
 *    these external pointers.
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_dup(f)
bdd_t *f;
{
    bdd_manager *manager;
    bdd_t *f1;

    if (f == NIL(bdd_t))
	return (NIL(bdd_t));	/* robustness */

    BDD_ASSERT( ! f->free );

    manager = f->bdd;

    /* WATCHOUT - no safe frame is declared here (b/c its not needed) */

    f1 = bdd_make_external_pointer(f->bdd, f->node, "bdd_dup");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_dup(%d)\n",
	    bdd_index_of_external_pointer(f1),
	    bdd_index_of_external_pointer(f));
#endif /* } */

    return (f1);
}

/*
 *    bdd_free - free the given bdd_t
 *
 *    WATCHOUT - don't call with BDDs assigned to a node -- use bdd_f_noderee.
 *    WATCHOUT - there is no way to assertion-check the above condition either
 *               the only possible error message is core-dumped (later)
 *
 *    return nothing, just do it.
 */
void
bdd_free(f)
bdd_t *f;
{
    bdd_manager *manager;
    
    if (f == NIL(bdd_t)) {
	fail("bdd_free: trying to free a NIL bdd_t");			
    }

    if (f->free == TRUE) {
	fail("bdd_free: trying to free a freed bdd_t");			
    }	

    manager = f->bdd;

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "bdd_free(%d)\n",
	    bdd_index_of_external_pointer(f));
#endif /* } */

    (void) bdd_destroy_external_pointer(f);
}

/*
 *    bdd_else - return the else-side of a bdd
 *
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_else(f)
bdd_t *f;
{
    bdd_safeframe frame;
    bdd_safenode reg_f, ret;
    bdd_manager *manager;
    bdd_t *e;

    if (f == NIL(bdd_t))
	fail("bdd_else: invalid BDD");

    BDD_ASSERT( ! f->free );

    if (BDD_IS_CONSTANT(f->bdd, f->node))
	fail("bdd_else: constant BDD");

    manager = f->bdd;

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f is already an external pointer so it need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, reg_f);
    bdd_safenode_declare(manager, ret);

    reg_f.node = BDD_REGULAR(f->node);
    ret.node = BDD_IS_COMPLEMENT(f->node) ? BDD_NOT(reg_f.node->E) : reg_f.node->E;

    /*
     *    End the safe frame and return the result
     */
    bdd_safeframe_end(manager);
    e = bdd_make_external_pointer(manager, ret.node, "bdd_else");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_else(%d)\n",
	    bdd_index_of_external_pointer(e),
	    bdd_index_of_external_pointer(f));
#endif /* } */

    return (e);
}

/* 
 *    bdd_then - return the then-side of a bdd
 *    
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_then(f)
bdd_t *f;
{
    bdd_safeframe frame;
    bdd_safenode reg_f, ret;
    bdd_manager *manager;
    bdd_t *t;

    if (f == NIL(bdd_t))
	fail("bdd_then: invalid BDD");

    BDD_ASSERT( ! f->free );

    if (BDD_IS_CONSTANT(f->bdd, f->node))
	fail("bdd_then: constant BDD");
	
    manager = f->bdd;

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f is already an external pointer so it need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, reg_f);
    bdd_safenode_declare(manager, ret);

    reg_f.node = BDD_REGULAR(f->node);
    ret.node = BDD_IS_COMPLEMENT(f->node) ? BDD_NOT(reg_f.node->T) : reg_f.node->T;

    /*
     *    End the safe frame and return the result
     */
    bdd_safeframe_end(manager);
    t = bdd_make_external_pointer(manager, ret.node, "bdd_then");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_then(%d)\n",
	    bdd_index_of_external_pointer(t),
	    bdd_index_of_external_pointer(f));
#endif /* } */

    return (t);
}

/*
 *    bdd_get_branches - get the branches of a node
 *
 *    return nothing, just do it.
 */
void
bdd_get_branches(f, f_T, f_E)
bdd_node *f;
bdd_node **f_T;	/* return */
bdd_node **f_E;	/* return */
{
	bdd_node *reg_f;
        int is_constant;

	reg_f = BDD_REGULAR(f);

        /* 
         * If f is the constant 0, we just want to return NIL for the then 
         * and else branches.  Since we can't access the manager from this
         * routine, we simply check the id of the node.
         */
        is_constant = (reg_f->id == BDD_ONE_ID) ? TRUE : FALSE;

	if (BDD_IS_COMPLEMENT(f) && !(is_constant)) {
	    *f_T = BDD_NOT(reg_f->T);
	    *f_E = BDD_NOT(reg_f->E);
	} else {
	    *f_T = reg_f->T;
	    *f_E = reg_f->E;
	}
}

/* 
 *    bdd_top_var - return the top variable of the bdd
 *    
 *    return the new bdd (external pointer)
 */
bdd_t *
bdd_top_var(f)
bdd_t *f;
{
    bdd_safeframe frame;
    bdd_safenode reg_f, ret;
    bdd_t *v;
    bdd_manager *manager;

    if (f == NIL(bdd_t))
	fail("bdd_top_var: invalid BDD");

    BDD_ASSERT( ! f->free );

    if (f->node == BDD_ONE(f->bdd))
	return (bdd_one(f->bdd));

    if (f->node == BDD_ZERO(f->bdd))
	return (bdd_zero(f->bdd));

    manager = f->bdd;

    /*
     *    After the input is checked for correctness, start the safe frame
     *    f and g are already external pointers so they need not be protected
     */
    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, reg_f);
    bdd_safenode_declare(manager, ret);

    reg_f.node = BDD_REGULAR(f->node);
    ret.node = bdd_find_or_add(manager, reg_f.node->id, BDD_ONE(manager), BDD_ZERO(manager));

    /*
     *    End the safe frame and return the result
     */
    bdd_safeframe_end(manager);
    v = bdd_make_external_pointer(manager, ret.node, "bdd_top_var");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_top_var(%d)\n",
	    bdd_index_of_external_pointer(v),
	    bdd_index_of_external_pointer(f));
#endif /* } */

    return (v);
}

/* 
 *    bdd_top_var_id - return the id of the top variable of the bdd
 */
bdd_variableId
bdd_top_var_id(f)
bdd_t *f;
{

    if (f == NIL(bdd_t))
	fail("bdd_top_var_id: invalid BDD");

    BDD_ASSERT( ! f->free );

    return (BDD_REGULAR(f->node)->id);
}

/*
 *    bdd_get_variable - get or create a new variable and add it to the bdd
 *
 *    The variable had better be in the range 0 .. nvariables else a failure occurs.
 *    A bdd of the form (v, 1, 0) is returned.   This is the only acceptable way of
 *    getting or creating variables in the manager.   It should be noted that the
 *    value of bdd.nvariables was established in bdd_start at manager initialization
 *    time and is not changed over the life of the manager.  This is for reasons of
 *    strong checking; ensure that the user doesn't wildly create variables.
 *
 *    return a bdd_t*
 */
bdd_t *
bdd_get_variable(manager, variable_i)
bdd_manager *manager;
bdd_variableId variable_i;
{
    bdd_safeframe frame;
    bdd_safenode v;
    bdd_t *var;

    bdd_safeframe_start(manager, frame);
    bdd_safenode_declare(manager, v);

    BDD_ASSERT(variable_i < manager->bdd.nvariables);

    v.node = bdd_find_or_add(manager, variable_i, BDD_ONE(manager), BDD_ZERO(manager));

    bdd_safeframe_end(manager);

    var = bdd_make_external_pointer(manager, v.node, "bdd_get_variable");

#if defined(BDD_FLIGHT_RECORDER) /* { */
    (void) fprintf(manager->debug.flight_recorder.log, "%d <- bdd_get_variable(manager, %d)\n",
	    bdd_index_of_external_pointer(var), variable_i);
#endif /* } */

    return (var);
}

/*
 *    bdd_get_stats - get the statistics from the bdd package
 *
 *    return nothing, just do it.
 */
void
bdd_get_stats(manager, stats)
bdd_manager *manager;
bdd_stats *stats;	/* return */
{
    BDD_ASSERT(manager != NIL(bdd_manager));
    
    (void) get_mem_stats(manager);
    *stats = manager->heap.stats;
}

/*
 *    bdd_size - return the size of the bdd (in nodes)
 *
 *    Compute the number of nodes in the bdd by traversal (e.g counting)
 *
 */
int
bdd_size(f)
bdd_t *f;
{
    int count;
    bdd_gen *gen;
    bdd_node *node;

    if (f == NIL(bdd_t))
	fail("bdd_size: illegal BDD");

    BDD_ASSERT( ! f->free );

    if (f->node == NIL(bdd_node)) {
	static already_warned = 0;
	if (! already_warned) {
	    already_warned = 1;
	    (void) printf("WARNING: size of nil bdd computed????\n");
	}
	return (0);
    }

    count = 0;
    foreach_bdd_node(f, gen, node) {
	count++;
    }

    return (count);
}

/*
 *    bdd__fail - because the fail() macro is inside a block
 *
 *    And I need a fail function that can be inside an expression
 *
 *    don't return, just do it
 */
int		/* must return an int so it can be in an expression */
bdd__fail(__file__, __line__, message, failed)
string __file__;
int __line__;
string message;
string failed;		/* may be NIL(char) indicating don't print it */
{
	(void) fprintf(stderr, "\
Fatal error in file %s, line %d\n\
\tmessage:   %s\n\
", __file__, __line__, message);

	if (failed != NIL(char)) {
	    (void) fprintf(stderr, "\
\tassertion: %s\n\
", failed);
	}

	(void) fflush(stdout);	/* not stderr - it is unbuffered */
	abort();
}

/*
 *    bdd_memfail - declare a memory allocation failure
 *
 *    Print out the statistics, so the guy knows why he died!
 *
 *    don't return, just do it
 */
void
bdd_memfail(manager, function)
bdd_manager *manager;	/* may be NIL(bdd_manager) in case of bdd_start() */
string function;
{
	bdd_stats stats;

	/*
	 *    WATCHOUT - assume NOTHING about the manager except that you can
	 *    go in and recover the statistics from it.  During garbage_collection
	 *    the manager is in a state if semi-inconsistence, so it is not
	 *    possible to do many operations on it.
	 */

	/*
	 *    If there was a bdd_manager, then get some stats before death
	 */
	if (manager != NIL(bdd_manager)) {
	    (void) fprintf(stderr, "\nBDD Manager Death Statistics\n\n");
	    (void) bdd_get_stats(manager, &stats);
	    (void) bdd_print_stats(stats, stderr);
	}

	(void) fprintf(stderr, "Fatal error: memory allocation failure in %s\n", function);
	(void) fflush(stdout);
	exit(1);
}

/*
 * get_mem_stats  
 *
 * Compute the memory used by the BDD manager and fill in the appropriate 
 * fields of the stats.memory data structure.
 */
static void 
get_mem_stats(manager)
bdd_manager *manager;
{
    bdd_stats *stats;
    
    /*
     * Get the current stats for the manager.
     */
    stats = &manager->heap.stats;

    /*
     * Update last_sbrk.
     */
    stats->memory.last_sbrk = (int) sbrk(0);

    /*
     * Memory for the manager itself.
     */
    stats->memory.manager = sizeof(bdd_manager);

    /*
     * Memory used by bdd_nodes.  Multiply by 2 since there are two halfspaces.
     */
    stats->memory.nodes = (stats->blocks.total * 2) * sizeof(bdd_nodeBlock);

    /*
     * Memory used by the unique table.
     */
    stats->memory.hashtable = manager->heap.hashtable.nbuckets * sizeof(bdd_node **);

    /*
     * Memory used by external pointers.
     */
    stats->memory.ext_ptrs = stats->extptrs.blocks * sizeof(bdd_bddBlock);

    /*
     * Memory used by ITE table entries and ITE table buckets.
     */
    stats->memory.ITE_cache = (manager->heap.cache.itetable.nbuckets * sizeof(bdd_hashcache_entry *))
        + (manager->heap.cache.itetable.nentries * sizeof(bdd_hashcache_entry));

    /*
     * Memory used by consttable entries and consttable buckets.
     */
    stats->memory.ITE_const_cache = (manager->heap.cache.consttable.nbuckets * sizeof(bdd_constcache_entry *))
        + (manager->heap.cache.consttable.nentries * sizeof(bdd_constcache_entry));

    /*
     * Memory used by the adhoc table: table + buckets + entries.
     */
    if (manager->heap.cache.adhoc.table == NIL(st_table)) {
        stats->memory.adhoc_cache = 0;
    } else {
        stats->memory.adhoc_cache = (sizeof(st_table) 
            + (manager->heap.cache.adhoc.table->num_bins * sizeof(st_table_entry *))
            + (st_count(manager->heap.cache.adhoc.table) * (sizeof(st_table_entry) + sizeof(bdd_adhoccache_key))) );
    }

    /*
     * Total memory used.
     */
    stats->memory.total = stats->memory.manager + stats->memory.nodes
        + stats->memory.hashtable + stats->memory.ext_ptrs + stats->memory.ITE_cache
        + stats->memory.ITE_const_cache + stats->memory.adhoc_cache;

}


/*
 * bdd_will_exceed_mem_limit  
 *
 * Test to see if allocating alloc_size will cause the manager's memory limit to
 * be exceeded.  Return TRUE if limit will be exceeded, else FALSE. If memory limit
 * will be exceeded and if call_daemon is TRUE, then don't return; instead, call the daemon.
 */
boolean
bdd_will_exceed_mem_limit(manager, alloc_size, call_daemon)
bdd_manager *manager;
int alloc_size;
boolean call_daemon;
{

    /*
     * If there is no memory limit, then return FALSE.
     */
    if (manager->memory.limit == BDD_NO_LIMIT) { 
        return FALSE;
    } 

    /* 
     * Compute the memory statistics for the manager. TODO: if get_mem_stats is taking too much time,
     * then we should update the memory stats incrementally each time we do an allocation.
     */
    (void) get_mem_stats(manager);

    /*
     * If allocating alloc_size bytes will not cause the limit to be exceeded, then return FALSE.
     */
    if ((manager->heap.stats.memory.total + alloc_size) < (manager->memory.limit * BDD_ONE_MEGABYTE)) { 
        return FALSE;
    } else {
        /*
         * Allocating alloc_size bytes will cause the memory limit to be exceeded.  If call_daemon is
         * FALSE, then just return TRUE.  Else, clean up, and then call the daemon.
         */
        if (call_daemon == FALSE) {
            return TRUE;
	} else {
            /*
             * Reset the safeframe pointer of the manager.  All of the data structures associated with the
             * safeframe mechanism are automatic (e.g. stack) variables which are on the stack somewhere.
             * Thus, we simply need to tell the manager that there are no safeframes.
             */
            manager->heap.internal_refs.frames = NIL(bdd_safeframe);

            /*
             * Free the adhoc table if it is in use.
             */
            if (manager->heap.cache.adhoc.table != NIL(st_table)) {
                (void) bdd_adhoccache_uninit(manager);
            }

            /*
             * Call the garbage collector to get rid of the current computation, and to try
             * to free up additional space.
             */
	    (void) bdd_garbage_collect(manager);

            /*
             * Call the daemon specified by the application.
             */
            if (manager->memory.daemon != NIL(void)) {   
                (*(manager->memory.daemon)) (manager); 
            } else {
                fail("bdd_will_exceed_mem_limit: memory limit set, but no daemon registered");
            }
	}
    }
}


/*
 * Get the manager of the bdd_t.
 */    
bdd_manager *
bdd_get_manager(f)
bdd_t *f;
{
    if (f == NIL(bdd_t)) {
	fail("bdd_get_manager: invalid BDD");
    }

    BDD_ASSERT( ! f->free );

    return f->bdd;
}

/*
 * Return a pointer to the external_hooks data structure of the manager.
 */
bdd_external_hooks *
bdd_get_external_hooks(manager)
bdd_manager *manager;
{
    return &(manager->hooks);
}


/*
 * Return the regularized bdd_node which the bdd_t points to.  Also, 
 * return whether or not the node pointer was complemented.
 */
bdd_node *
bdd_get_node(f, is_complemented)
bdd_t *f;
boolean *is_complemented; /* return */
{
    if (f == NIL(bdd_t))
	fail("bdd_equal: invalid BDD");

    BDD_ASSERT( ! f->free );

    *is_complemented = BDD_IS_COMPLEMENT(f->node);
    return (BDD_REGULAR(f->node));
}

/*
 * Return the number of variables that the bdd_manager knows of.
 */
unsigned int
bdd_num_vars(manager)
bdd_manager *manager;
{
    return (manager->bdd.nvariables);
}

/*
 * Shell for dynamic variable reordering.
 */
void 
bdd_dynamic_reordering(manager, algorithm_type)
bdd_manager *manager;
bdd_reorder_type_t algorithm_type;
{
    (void) printf("WARNING: Dynamic variable reordering not implemented in the Berkeley BDD package.\n");
}
