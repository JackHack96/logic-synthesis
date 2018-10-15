/*
 * Revision Control Information
 *
 * /projects/hsis/CVS/utilities/bdd_cmu/bdd_port/bdditer.c,v
 * shiple
 * 1.6
 * 1993/12/04 06:19:30
 *
 */
#include "util.h"     /* includes math.h */
#include "array.h"
#include "st.h"

#include "bdd.h"      /* UCB interface to CMU */
#include "bdduser.h"  /* CMU exported routines */
#include "bddint.h"   /* CMU internal routines; for use in bdd_get_branches() and for BDD_POINTER */

static void pop_cube_stack();
static void pop_node_stack();
static void push_cube_stack();
static void push_node_stack();

/*
 *    Defines an iterator on the onset of a BDD.  Two routines are
 *    provided: bdd_first_cube, which extracts one cube from a BDD and
 *    returns a bdd_gen structure containing the information necessary to
 *    continue the enumeration; and bdd_next_cube, which returns 1 if another cube was
 *    found, and 0 otherwise. A cube is represented
 *    as an array of bdd_literal (which are integers in {0, 1, 2}), where 0 represents
 *    negated literal, 1 for literal, and 2 for don't care.  Returns a disjoint
 *    cover.  A third routine is there to clean up. 
 */

/*
 *    bdd_first_cube - return the first cube of the function.
 *    A generator is returned that will iterate over the rest.
 *    Return the generator.
 */
bdd_gen *
bdd_first_cube(fn, cube)
bdd_t *fn;
array_t **cube;	/* of bdd_literal */
{
    struct bdd_manager_ *manager;
    bdd_gen *gen;
    int i;
    long num_vars;
    bdd_node *f;

    if (fn == NIL(bdd_t)) {
	cmu_bdd_fatal("bdd_first_cube: invalid BDD");
    }

    manager = fn->mgr;

    /*
     *    Allocate a new generator structure and fill it in; the stack and the 
     *    cube will be used, but the visited table and the node will not be used.
     */
    gen = ALLOC(bdd_gen, 1);
    if (gen == NIL(bdd_gen)) {
	cmu_bdd_fatal("bdd_first_cube: failed on memory allocation, location 1");
    }

    /*
     *    first - init all the members to a rational value for cube iteration
     */
    gen->manager = manager;
    gen->status = bdd_EMPTY;
    gen->type = bdd_gen_cubes;
    gen->gen.cubes.cube = NIL(array_t);
    gen->stack.sp = 0;
    gen->stack.stack = NIL(bdd_node *);
    gen->node = NIL(bdd_node);

    num_vars = cmu_bdd_vars(manager);
    gen->gen.cubes.cube = array_alloc(bdd_literal, num_vars);
    if (gen->gen.cubes.cube == NIL(array_t)) {
	cmu_bdd_fatal("bdd_first_cube: failed on memory allocation, location 2");
    }
    
    /*
     * Initialize each literal to 2 (don't care).
     */
    for (i = 0; i < num_vars; i++) {
        array_insert(bdd_literal, gen->gen.cubes.cube, i, 2);
    }

    /*
     * The stack size will never exceed the number of variables in the BDD, since
     * the longest possible path from root to constant 1 is the number of variables 
     * in the BDD.
     */
    gen->stack.sp = 0;
    gen->stack.stack = ALLOC(bdd_node *, num_vars);
    if (gen->stack.stack == NIL(bdd_node *)) {
	cmu_bdd_fatal("bdd_first_cube: failed on memory allocation, location 3");
    }
    /*
     * Clear out the stack so that in bdd_gen_free, we can decrement the ref count
     * of those nodes still on the stack.
     */
    for (i = 0; i < num_vars; i++) {
	gen->stack.stack[i] = NIL(bdd_node);
    }

    if (bdd_is_tautology(fn, 0)) {
	/*
	 *    All done, for this was but the zero constant ...
	 *    We are enumerating the onset, (which is vacuous).
         *    gen->status initialized to bdd_EMPTY above, so this
         *    appears to be redundant.
	 */
	gen->status = bdd_EMPTY;
    } else {
	/*
	 *    Get to work enumerating the onset.  Get the first cube.  Note that
         *    if fn is just the constant 1, push_cube_stack will properly handle this.
	 *    Get a new pointer to fn->node beforehand: this increments
	 *    the reference count of fn->node; this is necessary, because when fn->node
	 *    is popped from the stack at the very end, it's ref count is decremented.
	 */
	gen->status = bdd_NONEMPTY;
	f = cmu_bdd_identity(manager, fn->node);
	push_cube_stack(f, gen);
    }

    *cube = gen->gen.cubes.cube;
    return (gen);
}

/*
 *    bdd_next_cube - get the next cube on the generator.
 *    Returns {TRUE, FALSE} when {more, no more}.
 */
boolean
bdd_next_cube(gen, cube)
bdd_gen *gen;
array_t **cube;		/* of bdd_literal */
{

    pop_cube_stack(gen);
    if (gen->status == bdd_EMPTY) {
      return (FALSE);
    }
    *cube = gen->gen.cubes.cube;
    return (TRUE);
}

/*
 *    bdd_first_node - enumerates all bdd_node * in fn.
 *    Return the generator.
 */
bdd_gen *
bdd_first_node(fn, node)
bdd_t *fn;
bdd_node **node;	/* return */
{
    struct bdd_manager_ *manager;
    bdd_gen *gen;
    long num_vars;
    bdd_node *f;
    int i;

    if (fn == NIL(bdd_t)) {
	cmu_bdd_fatal("bdd_first_node: invalid BDD");
    }

    manager = fn->mgr;

    /*
     *    Allocate a new generator structure and fill it in; the
     *    visited table will be used, as will the stack, but the
     *    cube array will not be used.
     */
    gen = ALLOC(bdd_gen, 1);
    if (gen == NIL(bdd_gen)) {
	cmu_bdd_fatal("bdd_first_node: failed on memory allocation, location 1");
    }

    /*
     *    first - init all the members to a rational value for node iteration.
     */
    gen->manager = manager;
    gen->status = bdd_NONEMPTY;
    gen->type = bdd_gen_nodes;
    gen->gen.nodes.visited = NIL(st_table);
    gen->stack.sp = 0;
    gen->stack.stack = NIL(bdd_node *);
    gen->node = NIL(bdd_node);
  
    /* 
     * Set up the hash table for visited nodes.  Every time we visit a node,
     * we insert it into the table.
     */
    gen->gen.nodes.visited = st_init_table(st_ptrcmp, st_ptrhash);
    if (gen->gen.nodes.visited == NIL(st_table)) {
	cmu_bdd_fatal("bdd_first_node: failed on memory allocation, location 2");
    }

    /*
     * The stack size will never exceed the number of variables in the BDD, since
     * the longest possible path from root to constant 1 is the number of variables 
     * in the BDD.
     */
    gen->stack.sp = 0;
    num_vars = cmu_bdd_vars(manager);
    gen->stack.stack = ALLOC(bdd_node *, num_vars);
    if (gen->stack.stack == NIL(bdd_node *)) {
	cmu_bdd_fatal("bdd_first_node: failed on memory allocation, location 3");
    }
    /*
     * Clear out the stack so that in bdd_gen_free, we can decrement the ref count
     * of those nodes still on the stack.
     */
    for (i = 0; i < num_vars; i++) {
	gen->stack.stack[i] = NIL(bdd_node);
    }

    /*
     * Get the first node.  Get a new pointer to fn->node beforehand: this increments
     * the reference count of fn->node; this is necessary, because when fn->node
     * is popped from the stack at the very end, it's ref count is decremented.
     */
    f = cmu_bdd_identity(manager, fn->node);
    push_node_stack(f, gen);
    gen->status = bdd_NONEMPTY;

    *node = gen->node;	/* return the node */
    return (gen);	/* and the new generator */
}

/*
 *    bdd_next_node - get the next node in the BDD.
 *    Return {TRUE, FALSE} when {more, no more}.
 */
boolean
bdd_next_node(gen, node)
bdd_gen *gen;
bdd_node **node;	/* return */
{
    pop_node_stack(gen);
    if (gen->status == bdd_EMPTY) {
	return (FALSE);
    }
    *node = gen->node;
    return (TRUE);
}

/*
 *    bdd_gen_free - frees up the space used by the generator.
 *    Return an int so that it is easier to fit in a foreach macro.
 *    Return 0 (to make it easy to put in expressions).
 */
int
bdd_gen_free(gen)
bdd_gen *gen;
{
    long num_vars;
    int i;
    struct bdd_manager_ *mgr;
    bdd_node *f;
    st_table *visited_table;
    st_generator *visited_gen;

    mgr = gen->manager;

    switch (gen->type) {
    case bdd_gen_cubes:
	array_free(gen->gen.cubes.cube);
	gen->gen.cubes.cube = NIL(array_t);
	break;
    case bdd_gen_nodes:
        visited_table = gen->gen.nodes.visited;
	st_foreach_item(visited_table, visited_gen, (refany*) &f, NIL(refany)) {
	    cmu_bdd_free(mgr, f);
	}
	st_free_table(visited_table);
	visited_table = NIL(st_table);
	break;
    }

    /*
     * Free the data associated with this generator.  If there are any nodes remaining
     * on the stack, we must free them, to get their ref counts back to what they were before.
     */
    num_vars = cmu_bdd_vars(mgr);
    for (i = 0; i < num_vars; i++) {
	f = gen->stack.stack[i];
	if (f != NIL(bdd_node)) {
	    cmu_bdd_free(mgr, f);
	}
    }
    FREE(gen->stack.stack);

    FREE(gen);

    return (0);	/* make it return some sort of an int */
}

/*
 *    INTERNAL INTERFACE
 *
 *    Invariants:
 *
 *    gen->stack.stack contains nodes that remain to be explored.
 *
 *    For a cube generator,
 *        gen->gen.cubes.cube reflects the choices made to reach node at top of the stack.
 *    For a node generator,
 *        gen->gen.nodes.visited reflects the nodes already visited in the BDD dag.
 */

/*
 *    push_cube_stack - push a cube onto the stack to visit.
 *    Return nothing, just do it.  
 *
 *    The BDD is traversed using depth-first search, with the ELSE branch 
 *    searched before the THEN branch.
 *
 *    Caution: If you are creating new BDD's while iterating through the
 *    cubes, and a garbage collection happens to be performed during this
 *    process, then the BDD generator will get lost and an error will result.
 *
 */
static void
push_cube_stack(f, gen)
bdd_node *f;
bdd_gen *gen;
{
    bdd_variableId topf_id;
    bdd_node *f0, *f1;
    struct bdd_manager_ *mgr;

    mgr = gen->manager;

    if (f == cmu_bdd_one(mgr)) {
	return;
    }

    topf_id = (bdd_variableId) (cmu_bdd_if_id(mgr, f) - 1);

    /* 
     * Get the then and else branches of f. Note that cmu_bdd_then and cmu_bdd_else 
     * automatically take care of inverted pointers.  
     */
    f0 = cmu_bdd_else(mgr, f);
    f1 = cmu_bdd_then(mgr, f);

    if (f1 == cmu_bdd_zero(mgr)) {
	/*
	 *    No choice: take the 0 branch.  Since there is only one branch to 
         *    explore from f, there is no need to push f onto the stack, because
         *    after exploring this branch we are done with f.  A consequence of 
         *    this is that there will be no f to pop either.  Same goes for the
         *    next case.  Decrement the ref count of f and of the branch leading
         *    to zero, since we will no longer need to access these nodes.
	 */
	array_insert(bdd_literal, gen->gen.cubes.cube, topf_id, 0);
	push_cube_stack(f0, gen);
        cmu_bdd_free(mgr, f1);
        cmu_bdd_free(mgr, f);
    } else if (f0 == cmu_bdd_zero(mgr)) {
	/*
	 *    No choice: take the 1 branch
	 */
	array_insert(bdd_literal, gen->gen.cubes.cube, topf_id, 1);
	push_cube_stack(f1, gen);
        cmu_bdd_free(mgr, f0);
        cmu_bdd_free(mgr, f);
    } else {
        /*
         * In this case, we must explore both branches of f.  We always choose
         * to explore the 0 branch first.  We must push f on the stack, so that
         * we can later pop it and explore its 1 branch. Decrement the ref count 
	 * of f1 since we will no longer need to access this node.  Note that 
         * the parent of f1 was bdd_freed above or in pop_cube_stack.
         */
	gen->stack.stack[gen->stack.sp++] = f;
	array_insert(bdd_literal, gen->gen.cubes.cube, topf_id, 0);
	push_cube_stack(f0, gen);
        cmu_bdd_free(mgr, f1);
    }
}

static void
pop_cube_stack(gen)
bdd_gen *gen;
{
    bdd_variableId topf_id, level_i_id;
    bdd_node *branch_f;
    bdd_node *f1;
    int i;
    long topf_level;
    struct bdd_manager_ *mgr;
    struct bdd_ *var_bdd;

    mgr = gen->manager;

    if (gen->stack.sp == 0) {
        /*
         * Stack is empty.  Have already explored both the 0 and 1 branches of 
         * the root of the BDD.
         */
	gen->status = bdd_EMPTY;
    } else {
        /*
         * Explore the 1 branch of the node at the top of the stack (since it is
         * on the stack, this means we have already explored the 0 branch).  We 
         * permanently pop the top node, and bdd_free it, since there are no more edges left to 
         * explore. 
         */
	branch_f = gen->stack.stack[--gen->stack.sp];
	gen->stack.stack[gen->stack.sp] = NIL(bdd_node); /* overwrite with NIL */
        topf_id = (bdd_variableId) (cmu_bdd_if_id(mgr, branch_f) - 1);
	array_insert(bdd_literal, gen->gen.cubes.cube, topf_id, 1);

        /* 
         * We must set the variables with levels greater than the level of branch_f,
         * back to 2 (don't care).  This is because these variables are not
         * on the current path, and thus there values are don't care.
         *
         * Note the following correspondence:
         *   CMU          UCB
         *  index         level   (both start at zero)
         *  indexindex    id      (CMU has id 0 for constant, thus really start numbering at 1;
         *                                           UCB starts numbering at 0)
         */
        topf_level = cmu_bdd_if_index(mgr, branch_f);
	for (i = topf_level + 1; i < array_n(gen->gen.cubes.cube); i++) {
            var_bdd = cmu_bdd_var_with_index(mgr, i);
            level_i_id = (bdd_variableId) (cmu_bdd_if_id(mgr, var_bdd) - 1);
	    /*
             * No need to free var_bdd, since single variable BDDs are never garbage collected.
             * Note that level_i_id is just (mgr->indexindexes[i] - 1); however, wanted
             * to avoid using CMU internals.
             */
	    array_insert(bdd_literal, gen->gen.cubes.cube, level_i_id, 2);
	}
	f1 = cmu_bdd_then(mgr, branch_f);
	push_cube_stack(f1, gen);
	cmu_bdd_free(mgr, branch_f);
    }
}

/*
 *    push_node_stack - push a node onto the stack.
 *
 *    The same as push_cube_stack but for enumerating nodes instead of cubes.
 *    The BDD is traversed using depth-first search, with the ELSE branch searched 
 *    before the THEN branch, and a node returned only after its children have been
 *    returned.  Note that the returned bdd_node pointer has the complement
 *    bit zeroed out.
 *
 *    Caution: If you are creating new BDD's while iterating through the
 *    nodes, and a garbage collection happens to be performed during this
 *    process, then the BDD generator will get lost and an error will result.
 *
 *    Return nothing, just do it.
 */
static void
push_node_stack(f, gen)
bdd_node *f;
bdd_gen *gen;
{
    bdd_node *f0, *f1;
    bdd_node *reg_f, *reg_f0, *reg_f1;
    struct bdd_manager_ *mgr;

    mgr = gen->manager;

    reg_f = (bdd_node *) BDD_POINTER(f);  /* use of bddint.h */
    if (st_lookup(gen->gen.nodes.visited, (refany) reg_f, NIL(refany))) {
        /* 
         * Already been visited.
         */
	return;
    }

    if (f == cmu_bdd_one(mgr) || f == cmu_bdd_zero(mgr)) {
        /*
         * If f is the constant node and it has not been visited yet, then put it in the visited table
         * and set the gen->node pointer.  There is no need to put it in the stack because
         * the constant node does not have any branches, and there is no need to free f because 
         * constant nodes have a saturated reference count.
         */
	st_insert(gen->gen.nodes.visited, (refany) reg_f, NIL(any));
	gen->node = reg_f;
    } else {
        /*
         * f has not been marked as visited.  We don't know yet if any of its branches 
         * remain to be explored.  First get its branches.  Note that cmu_bdd_then and 
         * cmu_bdd_else automatically take care of inverted pointers.  
         */
	f0 = cmu_bdd_else(mgr, f);
	f1 = cmu_bdd_then(mgr, f);

	reg_f0 = (bdd_node *) BDD_POINTER(f0);  /* use of bddint.h */
	reg_f1 = (bdd_node *) BDD_POINTER(f1);
	if (! st_lookup(gen->gen.nodes.visited, (refany) reg_f0, NIL(refany))) {
            /* 
             * The 0 child has not been visited, so explore the 0 branch.  First push f on 
             * the stack.  Bdd_free f1 since we will not need to access this exact pointer
             * any more.
             */
	    gen->stack.stack[gen->stack.sp++] = f;
            push_node_stack(f0, gen);
	    cmu_bdd_free(mgr, f1);
	} else if (! st_lookup(gen->gen.nodes.visited, (refany) reg_f1, NIL(refany))) {
            /* 
             * The 0 child has been visited, but the 1 child has not been visited, so 
             * explore the 1 branch.  First push f on the stack. We are done with f0, 
	     * so bdd_free it.
             */
	    gen->stack.stack[gen->stack.sp++] = f;
            push_node_stack(f1, gen);
	    cmu_bdd_free(mgr, f0);
	} else {
            /*
             * Both the 0 and 1 children have been visited. Thus we are done exploring from f.  
             * Mark f as visited (put it in the visited table), and set the gen->node pointer.
	     * We will no longer need to refer to f0 and f1, so bdd_free them.  f will be
             * bdd_freed when the visited table is freed.
             */
            st_insert(gen->gen.nodes.visited, (refany) reg_f, NIL(any));
	    gen->node = reg_f;
	    cmu_bdd_free(mgr, f0);
	    cmu_bdd_free(mgr, f1);
	}
    }
}

static void
pop_node_stack(gen)
bdd_gen *gen;
{
    bdd_node *branch_f;

    if (gen->stack.sp == 0) {
	gen->status = bdd_EMPTY;
    } else {
	branch_f = gen->stack.stack[--gen->stack.sp];  /* overwrite with NIL */
	gen->stack.stack[gen->stack.sp] = NIL(bdd_node);
	push_node_stack(branch_f, gen);
    }
}

/*
 *    bdd_get_branches - get the branches of a node
 *
 *    return nothing, just do it.
 *
 * This function is not currently being used (TRS 9/29/93)
 */
static void
bdd_get_branches(f, f_T, f_E)
bdd_node *f;
bdd_node **f_T;	/* return */
bdd_node **f_E;	/* return */
{
    BDD_SETUP(f);

    /*
     * In the CMU package, the T and E pointers of the constant ONE node have
     * strange values:
     *   bddm->one->data[0]=0x5552436c;
     *   bddm->one->data[1]=0x65766572;
     *   bddm->zero=BDD_NOT(bddm->one);
     * Thus, if f points to a constant node, don't expect to get NILs back for
     * T and E, as is done in the Berkeley package.
     */

    *f_T = BDD_THEN(f);  /* using bddint.h */
    *f_E = BDD_ELSE(f);  /* using bddint.h */
}

