
/*
 * Revision 1.7  1993/07/27  20:16:10  sis
 * Added declarations for BDD minimization related routines.
 *
 * Revision 1.7  1993/07/27  19:29:53  shiple
 * Added declarations for BDD minimization related routines.
 *
 * Revision 1.6  1993/07/19  21:28:18  shiple
 * Removed external declaration of bdd_get_varids.
 *
 * Revision 1.5  1993/05/03  20:32:24  shiple
 * Made changes for ANSI C compatibility: removed illegal comma in enum
 * definition.  Added bdd_internal_quantify declaration.  Added enum for
 * quantification type.
 *
 * Revision 1.4  1993/01/11  23:43:19  shiple
 * Made changes for DEC Alpha compatibility.
 *
 * Revision 1.3  1992/09/19  03:45:08  shiple
 * Fix typo on change just made.
 *
 * Revision 1.2  1992/09/19  02:35:33  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_.   Added declaration of bdd_get_percentage.
 * Defined bdd_const_hash macro.  Before, the constcache was using the bdd_ITE_hash function, and
 * thus there was an implicit assumption that the constcache and ITE cache had the same number of
 * entries.  Renamed hashtable struct in manager to itetable. Changed ITE and ITE_const caches to
 * be arrays of pointers.  In manager, added rehash_at and max_size fields to caches.  In manager,
 * added node_ratio, init_node_blocks, and memory structure.
 *
 * Revision 1.1  1992/07/29  00:26:47  shiple
 * Initial revision
 *
 * Revision 1.2  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.1  92/01/08  17:34:27  sis
 * Initial revision
 * 
 * Revision 1.1  91/03/27  14:35:30  shiple
 * Initial revision
 * 
 *
 */

/*
 *    Internal stuff for the BDD package
 *
 *    None of this stuff is exported back to the package user.
 */

/* stuff.h - generic useful stuff */
/*
 *    Stuff to make the code more pleasant
 *
 *    These kinds of things ought to be in
 *    the global .h files for any project.
 */

#if ! defined(__STDC__) && ! defined(boolean)
/*
 * Ansi C defines this right?
 * this otta be a typedef somewhere
 */
#define boolean int	/* either 0 or 1 */
#endif

#ifndef NIL
/* this otta be a defined somewhere for us already */
#define NIL(type)	((type *) 0)
#endif

#ifndef string	/* Ansi C defines this right? */
/* this otta be a typedef somewhere */
#define string char *	/* a real live '\0' terminated string, not a refany */
#endif

#ifndef refany
/* this otta be a typedef somewhere */
#define refany char *	/* a void * or any sort of untyped pointer */
#endif

#ifndef any	/* Ansi C defines this right? */
/* this otta be a typedef somewhere */
#define any char	/* so that NIL(any) == refany */
#endif

#ifndef TRUE	/* Ansi C defines this right? */
#define TRUE 1
#endif
#ifndef FALSE	/* Ansi C defines this right? */
#define FALSE 0
#endif

#define sizeof_el(thing)                (sizeof (thing)/sizeof (thing[0]))

#ifndef MIN	/* Ansi C defines this right? */
#define MIN(a, b)       ((a) < (b) ? (a): (b))
#endif

#ifndef MAX	/* Ansi C defines this right? */
#define MAX(a, b)       ((a) > (b) ? (a): (b))
#endif

#ifndef MIN3
#define MIN3(a, b, c)   MIN(a, MIN(b, c))
#endif

#ifndef MAX3
#define MAX3(a, b, c)   MAX(a, MAX(b, c))
#endif

/* end stuff.h - end generic useful stuff */

/*
 *    This is the formal beginning of the internal bdd stuff
*/

extern int bdd__fail();		/* must return an int so it can be in an expression */
extern void bdd_memfail();

/*
 *    void
 *    BDD_FAIL(message)
 *    string message;
 */
#define BDD_FAIL(message)	bdd__fail(__FILE__, __LINE__, message, /* assertion */ NIL(char))

#if defined(BDD_DEBUG) /* { */
#define BDD_ASSERT(condition)	((condition) ? TRUE: \
				 bdd__fail(__FILE__, __LINE__, "BDD_ASSERT assertion failed", "condition"))
#else /* } else { */
#define BDD_ASSERT(condition)	(/* be happy */ TRUE)
#endif /* } */

/*
 *    The hash following function is the one that is suggested by Rudell's paper
 *
 *    unsigned int
 *    bdd_generic_hash(a, b, c, nbuckets)
 *    unsigned int a;
 *    unsigned int b;
 *    unsigned int c;
 *    unsigned int nbuckets;	- presumably a prime
 */
#define bdd_generic_hash(a, b, c, nbuckets)	\
	(((((unsigned int) (a)) << 5) + (((unsigned int) (b)) << 7) + (((unsigned int) (c)) << 11)) % ((unsigned int) (nbuckets)))

/*
 *    The id's and how some are reserved for system use
*/
#define BDD_ONE_ID		((bdd_variableId) (1<<30))
#define BDD_BROKEN_HEART_ID	((bdd_variableId) (1<<30)-1)	/* see the broken_heart implementation below */
#define BDD_USER_0_ID		((bdd_variableId) 0)		/* the 0th variableId */

/*
 *    Note the caveat above about referencing bdd nodes only
 *    via BDD_REGULAR and BDD_COMPLEMENT because any pointer.
 *    is going to have a sign bit encoded in it.
 *
 *    In addition to being a node in a bdd structure, this node
 *    is a chain element in a open-hashing chain.  The next field
 *    points to the next member of the chain.
 */
/* typedef struct bdd_node bdd_node is in bdd.h */
struct bdd_node {
    bdd_variableId id;		/* variableId is the index of the variable in the bdd */
    struct bdd_node *T;		/* then */
    struct bdd_node *E;		/* else */
    struct bdd_node *next;	/* chain pointer of the containing hashtable */
#if defined(BDD_DEBUG) /* { */
#if defined(BDD_DEBUG_AGE) || defined(BDD_DEBUG_LIFESPAN) /* { */
    unsigned int age;		/* the garbage-collect epoch of its creation */
#endif /* } */
#if defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
    unsigned int uniqueId;
#endif /* } */
#if defined(BDD_DEBUG_GC) /* { */
    int halfspace;		/* the halfspace that we're operating in now */
#endif /* } */
#endif /* } */
};

/*
 *    Here's the trick: bdd_node*'s can be either complemented or uncomplemented.
 *    This means that they are either suitable or unsuitable for use as a C pointer.
 *    To be used as a pointer, they must be ``regularized'' via BDD_REGULAR.
 *
 *    The convention is that bdd_node*'s can be either regular or unregular
 *    however some bdd_node*'s MUST be regular.  That way we can assert that things
 *    are correct by asserting that the regular nodes are in fact really regular.
 */
#if defined(BDD_DEBUG) /* { */
#define BDD_ASSERT_REGNODE(p)			\
	( ! BDD_IS_COMPLEMENT(p) ? TRUE:	\
	  bdd__fail(__FILE__, __LINE__, "BDD_ASSERT_REGNODE - a bdd_node* is complemented", "p"))
#else /* } else { */
#define BDD_ASSERT_REGNODE(p)		\
	(/* be happy */ TRUE)
#endif /* } */

#if defined(BDD_DEBUG) /* { */
#define BDD_ASSERT_NOTNIL(p)		\
	( (p) != NIL(bdd_node) ? TRUE:	\
	  bdd__fail(__FILE__, __LINE__, "BDD_ASSERT_NOTNIL - a bdd_node* is NIL", "p"))
#else /* } else { */
#define BDD_ASSERT_NOTNIL(p)		\
	(/* be happy */ TRUE)
#endif /* } */

/*
 *    Now some help for the garbage-collector:
 *
 *    Each bdd_node will need to represent forwarding pointers so that
 *    the garbage-collector can see what pointers have been forwarded
 *    from the old space to the new space.  These forwarding pointers
 *    are called ``broken hearts'' (Abelson & Sussman pg 500).
 *
 *    boolean
 *    BDD_BROKEN_HEART(node)
 *    bdd_node *node;
 *
 *    boolean
 *    BDD_FORWARD_POINTER(node)
 *    bdd_node *node;
 *
 *    boolean
 *    BDD_SET_FORWARD_POINTER(node, forward)
 *    bdd_node *node;
 *    bdd_node *forward;
 */
#define BDD__BROKEN_HEART(node)			( (node) != NIL(bdd_node) && \
						  ! BDD_IS_COMPLEMENT(node) && \
						  (node)->id == BDD_BROKEN_HEART_ID )
#define BDD__FORWARD_POINTER(node)		((node)->T)
#define BDD__SET_FORWARD_POINTER(node, forward)	{ (node)->id = BDD_BROKEN_HEART_ID; \
						  (node)->T = (forward); }

#if defined(BDD_DEBUG_GC) /* { */
#define BDD_ASSERT_BROKEN_HEART(n)		\
	( BDD_BROKEN_HEART(n) ? TRUE:	\
	  bdd__fail(__FILE__, __LINE__, "BDD_ASSERT_BROKEN_HEART - not a broken heart", "n"))
#else /* } else { */
#define BDD_ASSERT_BROKEN_HEART(n)		\
	(/* be happy */ TRUE)
#endif /* } */

#if defined(BDD_DEBUG_GC) /* { */
#define BDD_ASSERT_NOT_BROKEN_HEART(n)		\
	( ! BDD_BROKEN_HEART(n) ? TRUE:	\
	  bdd__fail(__FILE__, __LINE__, "BDD_ASSERT_NOT_BROKEN_HEART - is a broken heart", "n"))
#else /* } else { */
#define BDD_ASSERT_NOT_BROKEN_HEART(n)		\
	(/* be happy */ TRUE)
#endif /* } */

#define BDD_BROKEN_HEART(node)		\
	BDD__BROKEN_HEART(node)
#define BDD_FORWARD_POINTER(node)	\
	( BDD_ASSERT_BROKEN_HEART(node),	\
	  BDD__FORWARD_POINTER(node))
#define BDD_SET_FORWARD_POINTER(node, forward)	\
	{ BDD_ASSERT_REGNODE(node);		\
	  BDD_ASSERT_REGNODE(forward);		\
	  BDD__SET_FORWARD_POINTER(node, forward); }

/*
 *    int
 *    bdd_raw_node_hash(manager, variableId, T, E)
 *    bdd_manager *manager;
 *    bdd_variableId variableId;
 *    bdd_node *T;		- may be nil
 *    bdd_node *E;		- may be nil
 */
#if defined(BDD_DEBUG) && (defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN)) /* { */
#define bdd_raw_node_hash(manager, variableId, T, E)			\
	bdd_generic_hash(variableId,					\
		(T) == NIL(bdd_node) ? 0: BDD_REGULAR(T)->uniqueId,	\
		(E) == NIL(bdd_node) ? 0: BDD_REGULAR(E)->uniqueId,	\
		(manager)->heap.hashtable.nbuckets)
#else /* } else { */
#define bdd_raw_node_hash(manager, variableId, T, E)			\
	bdd_generic_hash(variableId, T, E, (manager)->heap.hashtable.nbuckets)
#endif /* } */

/*
 *    int
 *    bdd_node_hash(manager, node)
 *    bdd_manager *manager;
 *    bdd_node *node;		- may not be nil
 */
#define bdd_node_hash(manager, node)	\
	bdd_raw_node_hash(manager, (node)->id, BDD_REGULAR(node)->T, BDD_REGULAR(node)->E)

/*
 *    WATCHOUT A sign bit is encoded in the low-order bit of a
 *    node pointer.  This allows a bdd node to represent both the
 *    positive phase and negative phase of the function at the same
 *    time.  The phase all depends on how you reference the bdd.
 *
 *    bdd_node *
 *    BDD_NOT(node)
 *    bdd_node *node;
 *
 *        Create the negated function of the bdd
 *
 *    bdd_node *
 *    BDD_REGULAR(node)
 *    bdd_node *node;
 *
 *        Create the regular function of the bdd
 *
 *    boolean
 *    BDD_IS_COMPLEMENT(node)
 *    bdd_node *node;
 *
 *        Does this reprsent the complemented function?
 *
 *    boolean
 *    BDD_IS_CONSTANT(manager, node)
 *    bdd_manager *manager;
 *    bdd_node *node;
 *
 *        Does this reprsent a (the) constant?
 */
#define BDD_NOT(node)			((bdd_node *) ((long) (node) ^ 01L))
#define BDD_REGULAR(node)		((bdd_node *) ((long) (node) & ~01L))
#define BDD_IS_COMPLEMENT(node)		((long) (node) & 01L)
#define BDD_IS_CONSTANT(manager, node)	(BDD_REGULAR(node) == (manager)->bdd.one)

/*
 *    The constants available in the system
 *
 *    bdd_node *
 *    BDD_ONE(manager)
 *    bdd_manager *manager;
 *
 *        the constant one
 *
 *    bdd_node *
 *    BDD_ZERO(manager)
 *    bdd_manager *manager;
 *
 *        the constant zero (derived from one by complementation)
 */
#define BDD_ONE(manager)		(manager)->bdd.one
#define BDD_ZERO(manager)		BDD_NOT(BDD_ONE(manager))

/*
 *    The bdd_node heap is made up of a chain of nodeBlocks, each
 *    of which contain some number of bdd_nodes.
 *
 *    Note now magical the incantiation is for BDD_NODES_PER_NODEBLOCK.
 *    This is done to ensure that the memory allocator which underlys
 *    this package is not too smart for our own good.  Typical behaviors
 *    are that memory allocators add their internal overhead, round
 *    up to the nextclosest power of 2 and allocate that amount of space
 *    to satisfy the user's (large) space request.   We want that internal
 *    number to be exactly a power of two (or real close to one but less-than)
 *
 *    Want: BDD_NODES_PER_NODEBLOCK == 2^k - BDD_MALLOC_OVERHEAD_ESTIMATE
 *    ... for appropriate value of k (k is called BDD_NODEBLOCK_PO2 here)
 */
#define BDD_MALLOC_OVERHEAD_ESTIMATE		8	/* assume malloc adds 4 bytes of header info */

#if defined(BDD_DEBUG_GC) /* { */
#define BDD_NODEBLOCK_PO2			7	/* 2^7 = 128 - small for debugging */
#else /* } else { */
#if defined(BDD_DEBUG_LIFESPAN) /* { */
#define BDD_NODEBLOCK_PO2			12	/* 2^12 = 4K - small(er) for debuggin lifespans */
#else /* } else { */
#define BDD_NODEBLOCK_PO2			15	/* 2^15 = 32K - the magic Power 'O 2 (PO2) */
#endif /* } */
#endif /* } */

#define BDD_NODES_PER_NODEBLOCK			\
	(((1<<(BDD_NODEBLOCK_PO2-1)) -		\
	  BDD_MALLOC_OVERHEAD_ESTIMATE -	\
	  /* used */ sizeof (int) -		\
	  /* next */ sizeof (long)) / sizeof (bdd_node))

typedef struct bdd_nodeBlock {
    int used;
    struct bdd_nodeBlock *next;
    bdd_node subheap[BDD_NODES_PER_NODEBLOCK];
} bdd_nodeBlock;	/* total size plus malloc overhead should be an exact power of 2 */

/*
 *    The hashtable cache
 *
 *    Statement: For the purposes of validation of the BDD package it should
 *    be (e.g. it is and always must be) possible to turn off the cache.
 */
typedef struct bdd_hashcache_entry {
    struct {
	bdd_node *f;
	bdd_node *g;
	bdd_node *h;
    } ITE;		/* ITE(f, g, h) */
    bdd_node *data;		/* NIL(bdd_node) indicates in invalid entry */
} bdd_hashcache_entry;

extern void bdd_hashcache_insert();
extern boolean bdd_hashcache_lookup();

/*
 *    int
 *    bdd_ITE_hash(manager, f, g, h)
 *    bdd_manager *manager;
 *    bdd_node *f;		- may not be nil
 *    bdd_node *g;		- may not be nil
 *    bdd_node *h;		- may not be nil
 */
#define bdd_ITE_hash(manager, f, g, h)	\
	bdd_generic_hash(f, g, h, (manager)->heap.cache.itetable.nbuckets)

/*
 *    int
 *    bdd_hashcache_entry_hash(manager, entry, nbuckets)
 *    bdd_manager *manager;
 *    bdd_hashcache_entry *entry;	- may not be nil
 */
#define bdd_hashcache_entry_hash(manager, entry)	\
	bdd_ITE_hash(manager, (entry)->ITE.f, (entry)->ITE.g, (entry)->ITE.h)

/*
 *    The constant cache
 *
 *    Statement: For the purposes of validation of the BDD package it should
 *    be (e.g. it is and always must be) possible to turn off the cache.
 */
typedef enum bdd_constant_status {
    bdd_status_unknown,		/* the cache entry is invalid */
    bdd_constant_zero,		/* has the constant value of zero */
    bdd_constant_one,		/* has the constant value of one */
    bdd_nonconstant		/* has a nonconstant value */
} bdd_constant_status;

typedef struct bdd_constcache_entry {
    struct {
	bdd_node *f;
	bdd_node *g;
	bdd_node *h;
    } ITE;		/* ITE(f, g, h) */
    bdd_constant_status data;	/* bdd_status_unknown indicates an invalid entry */
} bdd_constcache_entry;

extern void bdd_constcache_insert();
extern boolean bdd_constcache_lookup();

/*
 *    int
 *    bdd_const_hash(manager, f, g, h)
 *    bdd_manager *manager;
 *    bdd_node *f;		- may not be nil
 *    bdd_node *g;		- may not be nil
 *    bdd_node *h;		- may not be nil
 */
#define bdd_const_hash(manager, f, g, h)	\
	bdd_generic_hash(f, g, h, (manager)->heap.cache.consttable.nbuckets)

/*
 *    int
 *    bdd_constcache_entry_hash(manager, entry)
 *    bdd_manager *manager;
 *    bdd_constcache_entry *entry;	- may not be nil
 */
#define bdd_constcache_entry_hash(manager, entry)	\
	bdd_const_hash(manager, (entry)->ITE.f, (entry)->ITE.g, (entry)->ITE.h)


/*
 *    The ad hoc cache
 */
typedef int bdd_int;		/* any integer value */

typedef struct bdd_adhoccache_key {
    bdd_node *f;		/* may be NIL(bdd_node) */
    bdd_node *g; 		/* may be NIL(bdd_node) */
    bdd_int v;			/* may be any integer value */
} bdd_adhoccache_key;

extern void bdd_adhoccache_init();
extern void bdd_adhoccache_uninit();

extern void bdd_adhoccache_insert();
extern boolean bdd_adhoccache_lookup();

/*
 *    The external user's view of a bdd is a bdd_t* (an external pointer)
 *
 *    A ``bdd'' proper is this thing which is a pointer to a (possibly
 *    complemented) node and a pointer to the bdd manager that manages
 *    that node.    The package user always references into the bdd package
 *    with a ``bdd_t*'' which provides a safe, indirect way of referencing
 *    into the garbage-collecting heap.
 */
/* typedef struct bdd_t bdd_t; already typedef'd in bdd.h */
struct bdd_t {
    boolean free;		/* TRUE if this is free, FALSE otherwise ... */
    bdd_node *node;		/* the node referenced */
    bdd_manager *bdd;		/* the manager from whence this came */
#if defined(BDD_DEBUG_EXT) /* { */
    string origin;		/* track the origin of all bdd_t*'s */
#endif /* } */
};

/*
 *    And then how these bdd_t's are managed internally (in blocks)
 */
#define BDD_BDDBLOCK_PO2		6		/* 2^6 = 64 */

#define BDD_NODES_PER_BDDBLOCK		(1<<(BDD_BDDBLOCK_PO2-1))

typedef struct bdd_bddBlock {
    struct bdd_bddBlock *next;
    bdd_t subheap[BDD_NODES_PER_BDDBLOCK];
} bdd_bddBlock;	/* total size plus malloc overhead should be an exact power of 2 */

extern bdd_t *bdd_make_external_pointer();
extern void bdd_destroy_external_pointer();

/*
 *    Safe pointers so that garbage-collection can be called with impunity
 *    in specially-written routines (e.g. inside a recursive bdd__ITE_())
 *
 *    A bdd_safenode is a safe pointer
 *    A bdd_safeframe is a list of safe pointers which are all
 *        used in the same routine
 */
typedef struct bdd_safenode {
    bdd_node *node;			/* the node referenced */
    bdd_node **arg;			/* the address of a value to clean up */
    struct bdd_safenode *next;		/* next (safe) node in the list */
} bdd_safenode;

typedef struct bdd_safeframe {
    struct bdd_safeframe *prev;
    bdd_safenode *nodes;
} bdd_safeframe;

#if defined(BDD_DEBUG_SF) /* { */
#define BDD_ASSERT_FRAMES_CORRECT(manager)	bdd_assert_frames_correct(manager)
#else /* } else { */
#define BDD_ASSERT_FRAMES_CORRECT(manager)	(/* be happy */ TRUE)
#endif /* } */

/*
 *    void
 *    bdd_safeframe__start(manager, sf)
 *    bdd_manager *manager;	- by value
 *    bdd_safeframe sf;		- by value (sortof - by reference b/c its a macro)
 *
 *    void
 *    bdd_safeframe__end(manager)
 *    bdd_manager *manager;	- by value
 *
 *    Usage: DO NOT USE THESE (they are only useful in bdd_new_node)
 *           use the versions below which do the assertion checks.
 */
#define bdd_safeframe__start(manager, sf)	\
		    { (sf).nodes = NIL(bdd_safenode); \
		      (sf).prev = (manager)->heap.internal_refs.frames; \
		      (manager)->heap.internal_refs.frames = &(sf); }
#define bdd_safeframe__end(manager)			\
		    { (manager)->heap.internal_refs.frames = \
		              (manager)->heap.internal_refs.frames->prev; }

/*
 *    void
 *    bdd_safeframe_start(manager, sf)
 *    bdd_manager *manager;	- by value
 *    bdd_safeframe sf;		- by value (sortof - by reference b/c its a macro)
 *
 *    void
 *    bdd_safeframe_end(manager)
 *    bdd_manager *manager;	- by value
 *
 *    Usage: (see below)
 */
#define bdd_safeframe_start(manager, sf)	\
		    { BDD_ASSERT_FRAMES_CORRECT(manager); \
		      bdd_safeframe__start(manager, sf); }
#define bdd_safeframe_end(manager)		\
		    { BDD_ASSERT((manager)->heap.internal_refs.frames != NIL(bdd_safeframe)); \
		      BDD_ASSERT_FRAMES_CORRECT(manager); \
		      bdd_safeframe__end(manager); }

/*
 *    void
 *    bdd_safenode__link(manager, sn, n)
 *    bdd_manager *manager;	- by value
 *    bdd_safenode sn;		- by value (sortof - by reference b/c its a macro)
 *    bdd_node *n;		- by value (sortof - by reference b/c its a macro)
 *
 *    void
 *    bdd_safenode__declare(manager, sn, n)
 *    bdd_manager *manager;	- by value
 *    bdd_safenode sn;		- by value (sortof - by reference b/c its a macro)
 *
 *    Usage: DO NOT USE THESE (they are only useful in bdd_new_node)
 *           use the versions below which do the assertion checks.
 */
#define bdd_safenode__link(manager, sn, n)	\
		    { (sn).node = NIL(bdd_node); \
		      (sn).arg = &(n); \
		      (sn).next = (manager)->heap.internal_refs.frames->nodes; \
		      (manager)->heap.internal_refs.frames->nodes = &(sn); }

#define bdd_safenode__declare(manager, sn)	\
		    { (sn).node = NIL(bdd_node); \
		      (sn).arg = NIL(bdd_node *); \
		      (sn).next = (manager)->heap.internal_refs.frames->nodes; \
		      (manager)->heap.internal_refs.frames->nodes = &(sn); }

/*
 *    void
 *    bdd_safenode_link(manager, sn, n)
 *    bdd_manager *manager;	- by value
 *    bdd_safenode sn;		- by value (sortof - by reference b/c its a macro)
 *    bdd_node *n;		- by value (sortof - by reference b/c its a macro)
 *
 *    void
 *    bdd_safenode_declare(manager, sn, n)
 *    bdd_manager *manager;	- by value
 *    bdd_safenode sn;		- by value (sortof - by reference b/c its a macro)
 *
 *    Usage: (see below)
 */
#define bdd_safenode_link(manager, sn, n)	\
		    { bdd_safenode__link(manager, sn, n); \
		      BDD_ASSERT_FRAMES_CORRECT(manager); }

#define bdd_safenode_declare(manager, sn)	\
		    { bdd_safenode__declare(manager, sn); \
		      BDD_ASSERT_FRAMES_CORRECT(manager); }

/*
 *    Usage:
 *        void
 *        some_function_or_another(manager, a)
 *        bdd_manager *manager;
 *        bdd_node *a;
 *        {
 *            bdd_safeframe frame;
 *            bdd_safenode safe_a, b, c;
 *
 *            bdd_safeframe_start(manager, frame);
 *            bdd_safenode_link(manager, safe_a, a);
 *            bdd_safenode_declare(manager, b);
 *            bdd_safenode_declare(manager, c);
 *
 *            ...
 *
 *            a = c.node;		-- a is referenced directly
 *            b.node = a;		-- b.node references a safenode's value
 *
 *            ...
 *
 *            bdd_safeframe_end(manager);
 *        }
 */

/*
 *    The bdd_manager contains all of the information necessary to manage
 *    a garbage-collecting heap of bdd_nodes.   Garbage-collection is to 
 *    happen transparently to the user, so no pointers can be exported.
 *
 *    Note that this is a BIG structure.   That is because it must hold EVERY
 *    single pointer and variable which is to be used by the bdd package.
 *    Each bdd_manager contains the _complete_ set of state variables required
 *    for the bdd package to operate on a heap of bdd's.
 */
/* typedef struct bdd_manager bdd_manager; already typedef'd in bdd.h */
struct bdd_manager {
    char *undef1;                       /* used for debugging */
    struct {
	struct {
	    unsigned int nkeys;		/* the number of keys in the hash table */
	    unsigned int nbuckets;	/* the size of the buckets array */
	    bdd_node **buckets;		/* the buckets array */
	    int rehash_at_nkeys;	/* the number of keys at which we rehash */
	} hashtable;		/* all live bdd_nodes are in this hash table */
	struct {
	    struct {
		unsigned int index;		/* the roving index in the block */
		bdd_bddBlock *block;		/* the block in which we look */
	    } pointer;			/* the roving pointer in the map */
	    unsigned int free;		/* the number which are known to be free */
	    unsigned int nmap;		/* the total number of bdd_t's in map */
	    bdd_bddBlock *map;		/* mapping bdd_t*'s to bdd_node*'s */
	} external_refs;	/* external references from the package user */
	struct {
	    bdd_safeframe *frames;	/* a list of safe frames on the stack */
	} internal_refs;	/* internal references from the bdd package */
	struct {
	    struct {
		boolean on;			/* is caching even turned on? */
		boolean invalidate_on_gc;	/* always invalidate the cache on a gc */
                unsigned int resize_at;         /* percentage at which to resize (e.g. 85% is 85) */
                unsigned int max_size;          /* don't resize if nbuckets exceeds this */
		unsigned int nbuckets;		/* buckets in the cache */
		unsigned int nentries;		/* entries in the cache */
		bdd_hashcache_entry **buckets;	/* the cache */
	    } itetable;		        /* cache of the hashtable entries */
	    struct {
		boolean on;			/* is caching even turned on? */
		boolean invalidate_on_gc;	/* always invalidate the cache on a gc */
                unsigned int resize_at;         /* percentage at which to resize (e.g. 85% is 85) */
                unsigned int max_size;          /* don't resize if nbuckets exceeds this */
		unsigned int nbuckets;		/* buckets in the cache */
		unsigned int nentries;		/* entries in the cache */
		bdd_constcache_entry **buckets;	/* the cache */
	    } consttable;		/* for the exclusive use of ITE_constant */
	    struct {
		boolean on;			/* is caching even turned on? */
                unsigned int max_size;          /* don't resize if nbuckets exceeds this */
		st_table *table;		/* may be NIL - of {bdd_adhoccache_key, bdd_node *} */
	    } adhoc;			/* cache used for various nefarious purposes (transient) */
	} cache;		/* a cache for past computations (partially voided on gc) */
	struct {
	    struct {
		bdd_nodeBlock *top;	/* the top of the in-use list (append only) */
		bdd_nodeBlock **tail;	/* either &top or &p->next of some member of the list */
	    } inuse;			/* an in-use list of nodeBlock */
	    bdd_nodeBlock *free;	/* a free list of unused blocks in this space */
	} half[2];
        unsigned int init_node_blocks;  /* number of bdd_nodeBlocks initially allocated */
	struct {
	    bdd_nodeBlock *block;
	    unsigned int index;
	} pointer;		/* a pointer to the next block/object at which to allocate */
	struct {
	    boolean on;			/* is garbage-collection even turned on? */
	    int halfspace;		/* either 0 or 1, indicating which halfspace is in use */
            float node_ratio;           /* ratio of used to unused nodes */
	    struct {
		int open_generators;	/* must be zero to perform a garbage-collect */
	    } status;
	    struct {
		struct {
		    bdd_nodeBlock *block;
		    unsigned int index;
		} start;		/* to communicate where to start gc passes */
	    } during;			/* used _during_ gc only */
	} gc;			/* garbage-collection relevancies */
	bdd_stats stats;
    } heap;
    struct {
	bdd_node *one;			/* the constant 1 (there is no constant 0) */
	unsigned int nvariables;	/* variables in the bdd (same as array_n(order) */
    } bdd;			/* stuff relevant to bdd's and algorithms on bdd's */
    bdd_external_hooks hooks;
    struct {
        void (*daemon)();               /* used for callback when memory limit exceeded */
        unsigned int limit;             /* upper bound on memory allocated by the manager; in megabytes */
    } memory;
#if defined(BDD_DEBUG) /* { */
    struct {
	struct {
#if defined(BDD_DEBUG_UID) || defined(BDD_DEBUG_LIFESPAN) /* { */
	    unsigned int uniqueId;		/* to assign each bdd_node a unique id */
#endif /* } */
#if defined(BDD_DEBUG_AGE) || defined(BDD_DEBUG_LIFESPAN) /* { */
	    unsigned int age;			/* the age is determined by number of garbage collections */
#endif /* } */
	    int dummy;	/* just in case nothing else is defined ... */
	} gc;				/* debugging of the garbage collector */
#if defined(BDD_DEBUG_LIFESPAN) /* { */
	struct {
	    FILE *trace;			/* where to write out the lifespan trace */
	} lifespan;
#endif /* } */
#if defined(BDD_FLIGHT_RECORDER) /* { */
	struct {
	    FILE *log;				/* where to write out the log */
	} flight_recorder;
#endif /* } */
    } debug;			/* all this stuff goes away when not debugging */
#endif /* } */
};

/*
 *    Various bdd_manager initialization values
 */
#define BDD_HASHTABLE_INITIAL_SIZE		113	/* the cache must be the same size, must match an entry in bdd_get_next_hash_prime */
#define BDD_CACHE_INITIAL_SIZE			113

#define BDD_CACHE_GROW_FACTOR			2.0	/* grow the caches by this factor */

#define BDD_HASHTABLE_MAXCHAINLEN		4	/* max number bdd_nodes per chain (before a rehash) */
#define BDD_HASHTABLE_GROW_FACTOR		2.0	/* grow the hashtable by this factor */

/*
 *    Node allocation and management
 *
 *    This includes cache management and external pointer management. 
 *    The internal pointer management declarations were given above
 *    with the structure declarations for bdd_safeframe and bdd_safenode.
 */
extern bdd_node *bdd_find_or_add();
extern bdd_node *bdd_new_node();
extern void bdd_garbage_collect();
extern void bdd_resize_hashtable();

/*
 *    return or die - debugging various things
 */
extern void bdd_assert_frames_correct();
extern void bdd_assert_heap_correct();

extern void bdd_dump_external_pointers();
extern void bdd_dump_node_ages();
extern void bdd_dump_manager_stats();

/*
 *    The ITE functions knowing about ITE identities
 *
 *    THESE ARE INTERNAL FUNCTIONS.  Call them from outside
 *    of the bdd package, and I will personally hunt you down.
 *
 *    They have so many underscores in them because if they didn't
 *    they would be spelled much like exported functions of the same
 *    name: bdd_ite().   We don't want people inadvertently calling
 *    internal routines just because they are named the same as an
 *    exported routine.  This is a matter of robustness rather than style.
 */
extern bdd_node *bdd__ITE_();
extern bdd_constant_status bdd__ITE_constant();

/*
 *    Generally useful
 */
extern void bdd_get_branches();
extern array_t *bdd_get_sorted_varids();
extern float bdd_get_percentage();
extern unsigned int bdd_get_next_hash_prime();
extern boolean bdd_will_exceed_mem_limit();

/*
 * Quantification function.
 */
typedef enum {BDD_EXISTS, BDD_FORALL} bdd_quantify_type_t;
extern bdd_node *bdd_internal_quantify(); /* used by bdd_cproject also */

/*
 * BDD minimization related functions.
 */
EXTERN void bdd_match_result ARGS((bdd_manager *, bdd_min_match_type_t, boolean, 
				   bdd_node *, bdd_node *, bdd_node *, bdd_node *, bdd_node **, bdd_node **));
EXTERN boolean bdd_is_match ARGS((bdd_manager *, bdd_min_match_type_t, boolean, 
				   bdd_node *, bdd_node *, bdd_node *, bdd_node *));

/*
 *    helper funtions common to bdd__ITE_() and bdd__ITE_constant()
 */
extern void bdd_ite_quick_cofactor();
extern void bdd_ite_var_to_const();
extern void bdd_ite_canonicalize_ite_inputs();



