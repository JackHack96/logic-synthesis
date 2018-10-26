
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



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
    bdd_manager *manager;
    bdd_gen *gen;
    int i;

    if (fn == NIL(bdd_t))
	fail("bdd_first_cube: invalid BDD");

    BDD_ASSERT( ! fn->free );

    manager = fn->bdd;

    /*
     *    Allocate a new generator structure and fill it in; the stack and the 
     *    cube will be used, but the visited table and the node will not be used.
     */
    gen = ALLOC(bdd_gen, 1);
    if (gen == NIL(bdd_gen))
	(void) bdd_memfail(manager, "bdd_first_cube");

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

    gen->gen.cubes.cube = array_alloc(bdd_literal, manager->bdd.nvariables);
    if (gen->gen.cubes.cube == NIL(array_t))
	(void) bdd_memfail(manager, "bdd_first_cube");
    
    /*
     * Initialize each literal to 2 (don't care).
     */
    for (i = manager->bdd.nvariables; --i >= 0; ) {
        (void) array_insert(bdd_literal, gen->gen.cubes.cube, i, 2);
    }

    /*
     * The stack size will never exceed the number of variables in the BDD, since
     * the longest possible path from root to constant 1 is the number of variables 
     * in the BDD.
     */
    gen->stack.sp = 0;
    gen->stack.stack = ALLOC(bdd_node *, manager->bdd.nvariables);
    if (gen->stack.stack == NIL(bdd_node *))
	(void) bdd_memfail(fn->bdd, "bdd_first_cube");

    if (fn->node == BDD_ZERO(gen->manager)) {
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
	 */
	gen->status = bdd_NONEMPTY;
	(void) push_cube_stack(fn->node, gen);
    }

    /*
     *    There is one more generator open on the manager now.
     */
    gen->manager->heap.gc.status.open_generators++;

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

    (void) pop_cube_stack(gen);
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
    bdd_manager *manager;
    bdd_gen *gen;

    if (fn == NIL(bdd_t))
	fail("bdd_first_node: invalid BDD");

    BDD_ASSERT(! fn->free );

    manager = fn->bdd;

    /*
     *    Allocate a new generator structure and fill it in; the
     *    visited table will be used, as will the stack, but the
     *    cube array will not be used.
     */
    gen = ALLOC(bdd_gen, 1);
    if (gen == NIL(bdd_gen))
	(void) bdd_memfail(manager, "bdd_first_node");

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
    if (gen->gen.nodes.visited == NIL(st_table))
	(void) bdd_memfail(manager, "bdd_first_node");

    /*
     * The stack size will never exceed the number of variables in the BDD, since
     * the longest possible path from root to constant 1 is the number of variables 
     * in the BDD.
     */
    gen->stack.sp = 0;
    gen->stack.stack = ALLOC(bdd_node *, manager->bdd.nvariables);
    if (gen->stack.stack == NIL(bdd_node *))
	(void) bdd_memfail(manager, "bdd_first_node");

    /*
     * Get the first node.
     */
    (void) push_node_stack(fn->node, gen);
    gen->status = bdd_NONEMPTY;

    /*
     *    There is one more generator open on the manager now.
     */
    gen->manager->heap.gc.status.open_generators++;

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
    (void) pop_node_stack(gen);
    if (gen->status == bdd_EMPTY)
	return (FALSE);

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
    gen->manager->heap.gc.status.open_generators--;

    switch (gen->type) {
    case bdd_gen_cubes:
	(void) array_free(gen->gen.cubes.cube);
	gen->gen.cubes.cube = NIL(array_t);
	break;
    case bdd_gen_nodes:
	(void) st_free_table(gen->gen.nodes.visited);
	gen->gen.nodes.visited = NIL(st_table);
	break;
    }

    /*
     *    These are always allocated.
     */
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
    bdd_variableId topf;
    bdd_node *f0, *f1;

    if (f == BDD_ONE(gen->manager))
	return;

    topf = BDD_REGULAR(f)->id;

    /* 
     * Note that bdd_get_branches automatically takes care of inverted pointers.  
     * For example, if f is a complemented pointer, then bdd_get_branches(f, &f1, &f0) 
     * will automatically set f1 to be NOT(BDD_REGULAR(f)->T) and f0 to be 
     * NOT(BDD_REGULAR(f)->E).
     */
    (void) bdd_get_branches(f, &f1, &f0);
    if (f1 == BDD_ZERO(gen->manager)) {
	/*
	 *    No choice: take the 0 branch.  Since there is only one branch to 
         *    explore from f, there is no need to push f onto the stack, because
         *    after exploring this branch we are done with f.  A consequence of 
         *    this is that there will be no f to pop either.  Same goes for the
         *    next case.
	 */
	(void) array_insert(bdd_literal, gen->gen.cubes.cube, topf, 0);
	(void) push_cube_stack(f0, gen);
    } else if (f0 == BDD_ZERO(gen->manager)) {
	/*
	 *    No choice: take the 1 branch
	 */
	(void) array_insert(bdd_literal, gen->gen.cubes.cube, topf, 1);
	(void) push_cube_stack(f1, gen);
    } else {
        /*
         * In this case, we must explore both branches of f.  We always choose
         * to explore the 0 branch first.  We must push f on the stack, so that
         * we can later pop it and explore its 1 branch.
         */
	gen->stack.stack[gen->stack.sp++] = f;
	(void) array_insert(bdd_literal, gen->gen.cubes.cube, topf, 0);
	(void) push_cube_stack(f0, gen);
    }
}

static void
pop_cube_stack(gen)
bdd_gen *gen;
{
    bdd_variableId topf;
    bdd_node *branch_f;
    bdd_node *f0, *f1;
    int i;

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
         * permanently pop the top node, since there are no more edges left to 
         * explore. 
         */
	branch_f = gen->stack.stack[--gen->stack.sp];
	topf = BDD_REGULAR(branch_f)->id;
	(void) bdd_get_branches(branch_f, &f1, &f0);
	(void) array_insert(bdd_literal, gen->gen.cubes.cube, topf, 1);

        /* 
         * We must set the variables with variables ids greater than topf, 
         * back to 2 (don't care).  This is because these variables are not
         * on the current path, and thus there values are don't care.
         */
	for (i=topf+1; i<array_n(gen->gen.cubes.cube); i++) {
	    array_insert(bdd_literal, gen->gen.cubes.cube, i, 2);
	}
	(void) push_cube_stack(f1, gen);
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

    if (st_lookup(gen->gen.nodes.visited, (refany) BDD_REGULAR(f), NIL(refany))) {
        /* 
         * Already been visited.
         */
	return;
    }

    if (f == BDD_ONE(gen->manager) || f == BDD_ZERO(gen->manager)) {
        /*
         * If f is the constant node and it has not been visited yet, then put it in the visited table
         * and set the gen->node pointer.  There is no need to put it in the stack because
         * the constant node does not have any branches.  
         */
	(void) st_insert(gen->gen.nodes.visited, (refany) BDD_REGULAR(f), NIL(any));
	gen->node = BDD_REGULAR(f);
    } else {
        /*
         * f has not been marked as visited.  We don't know yet if any of its branches remain to be explored.
         * First get its branches.  Note that bdd_get_branches properly handles inverted pointers.
         */
	(void) bdd_get_branches(f, &f1, &f0);
	if (! st_lookup(gen->gen.nodes.visited, (refany) BDD_REGULAR(f0), NIL(refany))) {
            /* 
             * The 0 child has not been visited, so explore the 0 branch.  First push f on 
             * the stack.
             */
	    gen->stack.stack[gen->stack.sp++] = f;
            (void) push_node_stack(f0, gen);
	} else if (! st_lookup(gen->gen.nodes.visited, (refany) BDD_REGULAR(f1), NIL(refany))) {
            /* 
             * The 0 child has been visited, but the 1 child has not been visited, so 
             * explore the 1 branch.  First push f on the stack.
             */
	    gen->stack.stack[gen->stack.sp++] = f;
            (void) push_node_stack(f1, gen);
	} else {
            /*
             * Both the 0 and 1 children have been visited. Thus we are done exploring from f.  
             * Mark f as visited (put it in the visited table), and set the gen->node pointer.
             */
            (void) st_insert(gen->gen.nodes.visited, (refany) BDD_REGULAR(f), NIL(any));
	    gen->node = BDD_REGULAR(f);
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
	branch_f = gen->stack.stack[--gen->stack.sp];
	(void) push_node_stack(branch_f, gen);
    }
}







