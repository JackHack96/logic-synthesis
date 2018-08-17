
#include "sis.h"
#include "node_int.h"


typedef struct div_cube_struct div_cube_t;
struct div_cube_struct {
    pset       original;
    pset       cube;
    int        is_divisor_cube;
    div_cube_t *next;
};


#define SORT    divide_sort_helper
#define SORT1    divide_sort
#define    TYPE    div_cube_t

#include "lsort.h"

#ifdef DIVIDE_DEBUG

static void
dump_set(fp, setp)
FILE *fp;
pset setp;
{
    int i, n;

    n = (BPI*LOOP(setp)) / 2;
    for(i = 0; i < n; i++) {
    putc("?01-"[GETINPUT(setp, i)], fp);
    }
}


static void
dump_list(list)
div_cube_t *list;
{
    div_cube_t *p;

    for(p = list; p != 0; p = p->next) {
    (void) fprintf(misout, "original=");
    dump_set(misout, p->original);
    (void) fprintf(misout, "  cube=");
    dump_set(misout, p->cube);
    (void) fprintf(misout, "  is_divisor=%d\n", p->is_divisor_cube);
    }
}


static void
dump_fanin(fanin, nfanin)
node_t **fanin;
int nfanin;
{
    int i;

    for(i = 0; i < nfanin; i++) {
    (void) fprintf(misout, "fanin %d: (%08X) %s\n",
        i, fanin[i], node_name(fanin[i]));
    }
}

#endif

static div_cube_t *
divide_setup_divisor(sf)
        pset_family sf;
{
    register pset       last, p;
    register div_cube_t *div, *head;

    head = 0;
    foreach_set(sf, last, p)
    {
        div = ALLOC(div_cube_t, 1);
        div->original        = p;
        div->is_divisor_cube = 1;
        div->cube            = p;
        div->next            = head;
        head = div;
    }
    return head;
}


static div_cube_t *
divide_setup_dividend(head, sf, divisor_support)
        register div_cube_t *head;
        pset_family sf;
        register pset divisor_support;
{
    register pset       last, p;
    register div_cube_t *div;

    foreach_set(sf, last, p)
    {
        div = ALLOC(div_cube_t, 1);
        div->original        = p;
        div->is_divisor_cube = 0;
        div->cube            = set_or(set_save(p), p, divisor_support);
        div->next            = head;
        head = div;
    }
    return head;
}

static div_cube_t *
divide_find_classes(list, fullset)
        div_cube_t **list;
        register pset fullset;
{
    register div_cube_t **prev, *p, *quotient;
    pset                divisor;

    quotient  = 0;
    for (prev = list; (p = *prev) != 0;) {
        if (p->is_divisor_cube) {
            /* unlink divisor cube from this list, and free it */
            *prev = p->next;
            divisor = p->cube;
            FREE(p);

            /* now find all cubes 'equal' (under a mask) to this divisor cube */
            while ((p = *prev) != 0 && lex_order(&divisor, &(p->cube)) == 0) {
                /* unlink from this list, add to quotient list */
                *prev = p->next;
                p->next = quotient;
                quotient = p;

                /* divide by this divisor cube */
                INLINEset_ndiff(p->cube, p->original, divisor, fullset);
            }

            /* ignore any cubes which are not divisor cubes */
        } else {
            prev = &(p->next);
        }
    }
    return quotient;
}

void
divide_finale(list, quotient, divisor_ncube, q_sf, r_sf)
        div_cube_t *list, *quotient;
        int divisor_ncube;
        pset_family q_sf, r_sf;
{
    register div_cube_t *p, *p1, *p2;
    register int        run;
    register pset       pdest;

    for (p = quotient; p != 0; p = p1) {
        run = 1;
        p1  = p->next;
        while (p1 != 0 && lex_order(&(p->cube), &(p1->cube)) == 0) {
            p1 = p1->next;
            run++;
        }
        if (run == divisor_ncube) {
            /* add single cube to quotient */
            pdest = GETSET(q_sf, q_sf->count++);
            INLINEset_copy(pdest, p->cube);
        } else {
            /* add all of these cubes to remainder */
            if (r_sf != 0) {
                for (p2 = p; p2 != p1; p2 = p2->next) {
                    pdest = GETSET(r_sf, r_sf->count++);
                    INLINEset_copy(pdest, p2->original);
                }
            }
        }
    }

    /* discard the quotient */
    for (p = quotient; p != 0; p = p1) {
        p1 = p->next;
        set_free(p->cube);
        FREE(p);
    }

    /* copy the remainder list to the remainder and discard it */
    for (p = list; p != 0; p = p1) {
        p1 = p->next;
        /* add all of these cubes to remainder */
        if (r_sf != 0) {
            pdest = GETSET(r_sf, r_sf->count++);
            INLINEset_copy(pdest, p->original);
        }
        set_free(p->cube);
        FREE(p);
    }
}

static int
list_compare(p, q)
        div_cube_t *p, *q;
{
    int x;

    x = lex_order(&(p->cube), &(q->cube));
    if (x == 0) {
        return q->is_divisor_cube - p->is_divisor_cube;
    } else {
        return x;
    }
}

/*
 *  divide f/g giving quotient and (optional) remainder.  Assumes that
 *  g is already known to be a single cube (but not a single literal).
 */

static node_t *
divide_single_cube(f, g, r_node)
        node_t *f, *g;
        node_t **r_node;
{
    pset_family   newf, newg, q_sf, r_sf;
    register pset last, p, fullset, pdest, g_cube;
    node_t        **new_fanin, *q_node;
    int           new_nin;

    /* express f and g in a common base */
    make_common_base(f, g, &new_fanin, &new_nin, &newf, &newg);

    /* make a full set of the right size */
    fullset = set_fill(set_new(new_nin * 2), new_nin * 2);

    q_sf   = sf_new(newf->count, new_nin * 2);
    r_sf   = r_node ? sf_new(newf->count, new_nin * 2) : 0;
    g_cube = GETSET(newg, 0);
    foreach_set(newf, last, p)
    {
        if (setp_implies(p, g_cube)) {
            pdest = GETSET(q_sf, q_sf->count++);
            *pdest = 0;        /* ### crock for saber */
            INLINEset_ndiff(pdest, p, g_cube, fullset);
        } else {
            if (r_node != 0) {
                pdest = GETSET(r_sf, r_sf->count++);
                INLINEset_copy(pdest, p);
            }
        }
    }

    if (r_node != 0) {
        *r_node = node_create(r_sf, nodevec_dup(new_fanin, new_nin), new_nin);
        (*r_node)->is_dup_free    = 1;    /* make_common_base insures this */
        (*r_node)->is_scc_minimal = f->is_scc_minimal;
        node_minimum_base(*r_node);
    }

    q_node = node_create(q_sf, new_fanin, new_nin);
    q_node->is_dup_free    = 1;        /* make_common_base insures this */
    q_node->is_scc_minimal = f->is_scc_minimal;
    node_minimum_base(q_node);

    sf_free(newf);
    sf_free(newg);
    set_free(fullset);
    return q_node;
}

/*
 *  divide f/g giving quotient and (optional) remainder.  Assumes that
 *  g is already known to be a single literal.
 */

static node_t *
divide_single_literal(f, g, r_node)
        node_t *f, *g;
        node_t **r_node;
{
    node_t        *q_node;
    pset_family   q_sf, r_sf;
    register pset last, p, pdest;
    register int  index, index1;
    int           g_lit;

    index = node_get_fanin_index(f, g->fanin[0]);
    if (index == -1) {
        if (r_node != 0) *r_node = node_dup(f);
        return node_constant(0);
    }

    q_sf  = sf_new(f->F->count, f->nin * 2);
    r_sf  = r_node ? sf_new(f->F->count, f->nin * 2) : 0;
    g_lit = GETINPUT(GETSET(g->F, 0), 0);
    assert(g_lit != TWO);        /* if so, g is not minimum base */
    index1 = g_lit == ZERO ? 2 * index + 1 : 2 * index;
    foreach_set(f->F, last, p)
    {
        if (GETINPUT(p, index) == g_lit) {
            pdest = GETSET(q_sf, q_sf->count++);
            INLINEset_copy(pdest, p);
            set_insert(pdest, index1);
        } else {
            if (r_node != 0) {
                pdest = GETSET(r_sf, r_sf->count++);
                INLINEset_copy(pdest, p);
            }
        }
    }

    if (r_node != 0) {
        *r_node = node_create(r_sf, nodevec_dup(f->fanin, f->nin), f->nin);
        (*r_node)->is_dup_free    = 1;    /* make_common_base insures this */
        (*r_node)->is_scc_minimal = f->is_scc_minimal;
        node_minimum_base(*r_node);
    }

    q_node = node_create(q_sf, nodevec_dup(f->fanin, f->nin), f->nin);
    q_node->is_dup_free    = 1;        /* make_common_base insures this */
    q_node->is_scc_minimal = f->is_scc_minimal;
    node_minimum_base(q_node);
    return q_node;
}

/*
 * node_div -- compute f/g returning quotient in r and remainder in q 
 *
 *  This looks complicated, but its really not.  
 *
 *  The idea is to find all cubes of f which are equal to a cube of g
 *  when restricted to the support of g.  If we collect the cube
 *  quotients h(i) = f(i)/g(i) for each of these cases, then a cube
 *  h(i) belongs to the final quotient iff it appears m times in the
 *  collection of cubes h(i).  (m is the number of cubes of g).
 *
 *  Hence, a single sort (restricted to the support of g) identifies
 *  all cubes f(i) which are equal to some cube g(i); then, we form the
 *  cubes h(i) for each of these cases, and sort this list to find the
 *  duplicity of each h(i).
 *
 *  Some simple filters speed things up:
 * 	1. support(g) must be contained in the support of f; if not,
 *	   q = 0, r = f
 *	2. number of cubes of f must be greater than (or equal) to the
 *	   number of cubes of g; if not, q = 0, r = f.
 *	3. divide by a single literal, or divide by a single term is
 *	   much easier (avoid sort, guaranteed o(n)); these are handled
 *	   as special cases
 */

node_t *
node_div(f, g, r_node)
        node_t *f, *g;
        node_t **r_node;
{
    pset        fullset, f_support, g_support;
    pset_family newf, newg, q_sf, r_sf;
    node_t      **new_fanin, *q_node;
    int         new_nin;
    div_cube_t  *list, *quotient;

    if (f->F == 0 || g->F == 0) {
        fail("node_div: node does not have a function");
    }

    /* filter 1: trivial answer if f has too few cubes */
    if (f->F->count < g->F->count) {
        if (r_node != 0) *r_node = node_dup(f);
        return node_constant(0);
    }

    /* filter 2: check for a single cube or a single literal */
    if (g->F->count == 1) {
        if (g->nin == 1) {
            return divide_single_literal(f, g, r_node);
        } else {
            return divide_single_cube(f, g, r_node);
        }
    }

    /* express f and g in a common base */
    make_common_base(f, g, &new_fanin, &new_nin, &newf, &newg);
    f_support = sf_and(newf);
    g_support = sf_and(newg);

    /* filter 3: trivial answer if support(g) not contained in support(f) */
    if (!setp_implies(f_support, g_support)) {
        sf_free(newf);
        sf_free(newg);
        set_free(f_support);
        set_free(g_support);
        FREE(new_fanin);
        if (r_node != 0) *r_node = node_dup(f);
        return node_constant(0);
    }

    /* make a full set of the right size */
    fullset = set_fill(set_new(new_nin * 2), new_nin * 2);

    /* build the list for the cubes of f and g */
    list = divide_setup_divisor(newg);
    list = divide_setup_dividend(list, newf, g_support);

    /* sort the list */
    list = divide_sort(list, list_compare);

    /* form the quotient list */
    quotient = divide_find_classes(&list, fullset);

    /* sort the quotient list */
    quotient = divide_sort(quotient, list_compare);

    /* finally extract the quotient and remainder */
    q_sf = sf_new(newf->count, new_nin * 2);
    r_sf = r_node ? sf_new(newf->count, new_nin * 2) : 0;
    divide_finale(list, quotient, newg->count, q_sf, r_sf);

    if (r_node != 0) {
        *r_node = node_create(r_sf, nodevec_dup(new_fanin, new_nin), new_nin);
        (*r_node)->is_dup_free    = 1;    /* make_common_base insures this */
        (*r_node)->is_scc_minimal = f->is_scc_minimal;
        node_minimum_base(*r_node);
    }

    q_node = node_create(q_sf, new_fanin, new_nin);
    q_node->is_dup_free    = 1;        /* make_common_base insures this */
    q_node->is_scc_minimal = f->is_scc_minimal;
    node_minimum_base(q_node);

    sf_free(newf);
    sf_free(newg);
    set_free(fullset);
    set_free(f_support);
    set_free(g_support);
    return q_node;
}
