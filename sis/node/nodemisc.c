
#include "sis.h"
#include "../include/node_int.h"

static int merge_fanin_list();

/* static pset_family node_sf_adjust(); -- net2pla.c needs this ... */
static pset_family node_sf_adjust_by_name();

static pset_family do_sf_permute();

static int has_dup_fanin();


int
node_compare_id(p1, p2)
        char **p1, **p2;
{
    node_t *q1 = *(node_t **) p1, *q2 = *(node_t **) p2;

    return q1->sis_id - q2->sis_id;
}


static int
compare_node_by_name(p1, p2)
        char **p1, **p2;
{
    node_t *q1 = *(node_t **) p1, *q2 = *(node_t **) p2;

    if (q1->name == 0) node_assign_name(q1);    /* hack */
    if (q2->name == 0) node_assign_name(q2);    /* hack */
    return strcmp(q1->name, q2->name);
}

/*
 *  take two nodes, and express them over a common base
 *	- works even if either node has 'duplicate' fanin
 */

void
make_common_base(f, g, new_fanin, new_nin, newf, newg)
        node_t *f, *g;
        node_t ***new_fanin;
        int *new_nin;
        pset_family *newf, *newg;
{
    node_t *func_list[2];

    func_list[0] = f;
    func_list[1] = g;
    *new_nin = merge_fanin_list(func_list, 2, new_fanin, node_compare_id);

    *newf = node_sf_adjust(f, *new_fanin, *new_nin);
    *newg = node_sf_adjust(g, *new_fanin, *new_nin);
}

/*
 *  merge the fanin list from multiple nodes into a single, uniq list
 */

static int
merge_fanin_list(func_list, nfunc, new_list, compare)
        node_t **func_list;
        int nfunc;
        node_t ***new_list;
        int (*compare)();
{
    register int    nlist, i, j, count;
    register node_t **list, *func;

    count  = 0;
    for (i = 0; i < nfunc; i++) {
        count += func_list[i]->nin;
    }

    list   = ALLOC(node_t * , count);
    nlist  = 0;
    for (i = nfunc - 1; i >= 0; i--) {
        func   = func_list[i];
        for (j = func->nin - 1; j >= 0; j--) {
            list[nlist++] = func->fanin[j];
        }
    }

    if (nlist >= 2) {
        /* Sort the elements */
        qsort((char *) list, nlist, sizeof(node_t * ), compare);

        /* Make the list unique */
        count  = 1;
        for (i = 1; i < nlist; i++) {
            if ((*compare)((char **) &(list[i - 1]), (char **) &(list[i])) != 0) {
                list[count++] = list[i];
            }
        }
        nlist = count;
    }

    *new_list = list;
    return nlist;
}

/*
 *  adjust a node to place it over a different base
 *	- watch carefully for duplicated fanin in the node
 */

pset_family
node_sf_adjust(node, new_fanin, new_nin)
        node_t *node;
        register node_t **new_fanin;
        int new_nin;
{
    register int    i, j, *permute;
    register node_t **old_fanin;
    int             has_dup, old_nin;
    pset_family     new_sf;

    if (node->is_dup_free) {
        has_dup = 0;
    } else {
        has_dup = has_dup_fanin(node);
        node->is_dup_free = !has_dup;
    }
    old_nin   = node->nin;
    old_fanin = node->fanin;

    /* check for 0, or 1 function to be returned ... */
    if (new_nin == 0) {
        assert(!has_dup);        /* does not make sense */
        new_sf = sf_new(1, 0);
        new_sf->count = (node->F->count > 0);
        return new_sf;
    }

    /* check for identity permutation */
    if (old_nin == new_nin) {
        for (i = new_nin - 1; i >= 0; i--) {
            if (old_fanin[i] != new_fanin[i]) {
                break;
            }
        }
        if (i < 0) {
            return sf_save(node->F);
        }
    }

    /* setup permute(old column index) = new column index (or -1) */
    permute = ALLOC(
    int, old_nin);
    for (i = old_nin - 1; i >= 0; i--) {
        permute[i] = -1;
        for (j = new_nin - 1; j >= 0; j--) {
            if (old_fanin[i] == new_fanin[j]) {
                permute[i] = j;
                break;
            }
        }
    }

    new_sf = do_sf_permute(node->F, permute, new_nin, has_dup);
    FREE(permute);
    return new_sf;
}

static pset_family
do_sf_permute(old_sf, permute, new_nin, has_dup)
        pset_family old_sf;
        int *permute;
        int new_nin;
        int has_dup;
{
    pset_family   new_sf;
    register pset last, p, pdest;
    register int  i, new_col, t;
    int           old_nin;

    if (has_dup) {
        /* make sure that cdist0 and cube.fullset are defined ! */
        define_cube_size(new_nin);
    }

    new_sf  = sf_new(old_sf->count, new_nin * 2);
    old_nin = old_sf->sf_size / 2;
    foreach_set(old_sf, last, p)
    {
        pdest = GETSET(new_sf, new_sf->count++);
        (void) set_fill(pdest, new_nin * 2);

        for (i = old_nin - 1; i >= 0; i--) {
            new_col = permute[i];
            if (new_col != -1) {
                switch (GETINPUT(p, i)) {
                    case ZERO: t = 2 * new_col + 1;
                        set_remove(pdest, t);
                        break;
                    case ONE: t = 2 * new_col;
                        set_remove(pdest, t);
                        break;
                    case TWO: break;
                    default: fail("do_sf_permute: improper set encountered\n");
                }
            }
        }

        /* don't save the set if it has a 00 coordinate */
        if (has_dup && !cdist0(pdest, cube.fullset)) {
            new_sf->count--;
        }
    }
    return new_sf;
}

/*
 *  put 'node' over a minimum base
 *	- removes duplicated fanin if present
 */

void
node_minimum_base(node)
        node_t *node;
{
    pset   floor;
    pcover F1;
    node_t **new_fanin;
    int    var, new_nin;

    if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) {
        return;
    }
    if (node->F == 0) {
        fail("node_minimum_base: node does not have a function");
    }

    /* make sure that no fanin is duplicated */
    if (!node->is_dup_free) {
        node_remove_dup_fanin(node);
    }

    /* bail out for 0, 1 functions ... */
    if (node->nin == 0) {
        node->F->count = (node->F->count > 0);    /* make it 0 or 1 */
        return;
    }

    /* make sure it is scc-minimal */
    if (!node->is_scc_minimal) {
        node->F = sf_contain(node->F);
    }

    /* AND together all of the cubes; should form 00 for most positions */
    floor = sf_and(node->F);

    /* see if there are variables we don't depend on */
    for (var = 0; var < node->nin; var++) {
        if (GETINPUT(floor, var) == TWO) {
            break;
        }
    }

    if (var != node->nin) {
        /* create new fanin list (with minimum number of variables) */
        new_fanin = ALLOC(node_t * , node->nin);
        new_nin   = 0;
        for (var  = 0; var < node->nin; var++) {
            if (GETINPUT(floor, var) != TWO) {
                new_fanin[new_nin++] = node->fanin[var];
            }
        }

        F1 = node_sf_adjust(node, new_fanin, new_nin);
        node_replace_internal(node, new_fanin, new_nin, F1);
    }
    set_free(floor);
    node->is_dup_free    = 1;
    node->is_scc_minimal = 1;
}

/* intended for internal use only ... */
node_t *
node_create(func, fanin, nin)
        pset_family func;
        node_t **fanin;
        int nin;
{
    node_t *node;

    node = node_alloc();
    node->nin   = nin;
    node->fanin = fanin;
    node->F     = func;
    return node;
}


/* intended for internal use only ... */
void
node_replace_internal(f, fanin, nin, F)
        node_t *f;
        node_t **fanin;
        int nin;
        pset_family F;
{
    /* hack -- allow assignment of a logic function to a PI */
    if (f->type == PRIMARY_INPUT) {
        if (f->network != 0) {
            network_change_node_type(f->network, f, INTERNAL);
        } else {
            f->type = UNASSIGNED;    /* not in network !?! */
        }
    }

    /* patch fanout lists if 'f' is in a network */
    if (f->network != 0) {
        fanin_remove_fanout(f);
    }

    /* replace the values at the node */
    if (f->fanin != NIL(node_t * )) FREE(f->fanin);
    if (f->F != NIL(set_family_t)) sf_free(f->F);
    f->nin   = nin;
    f->fanin = fanin;
    f->F     = F;

    /* patch fanout list for new fanin list if 'f' is in a network */
    if (f->network != 0) {
        fanin_add_fanout(f);
    }

    node_invalid(f);
    f->fanin_changed = 1;
}


void
node_replace(f, r)
        node_t *f, *r;
{
    if (r->F == 0) {
        fail("node_replace: node does not have a function");
    }

    node_replace_internal(f, r->fanin, r->nin, r->F);
    f->is_dup_free    = r->is_dup_free;
    f->is_scc_minimal = r->is_scc_minimal;

    r->F     = 0;
    r->fanin = 0;
    r->nin   = 0;
    node_free(r);
}

int
node_base_contain(f, g)
        node_t *f, *g;
{
    register int    i, j;
    register node_t **listf, **listg;

    if (f->nin < g->nin) {
        return 0;        /* base of g CANNOT contain base of f */
    }

    /* could avoid sort if already known to be sorted ... */

    listf = nodevec_dup(f->fanin, f->nin);
    qsort((char *) listf, f->nin, sizeof(node_t * ), (int (*)()) node_compare_id);

    listg = nodevec_dup(g->fanin, g->nin);
    qsort((char *) listg, g->nin, sizeof(node_t * ), (int (*)()) node_compare_id);

    /* see if each element of g has a mate in f */
    j      = f->nin - 1;
    for (i = g->nin - 1; i >= 0; i--) {
        while (j >= 0 && listf[j]->sis_id > listg[i]->sis_id) {
            j--;
        }
        if (j < 0 || listf[j]->sis_id < listg[i]->sis_id) {
            FREE(listf);
            FREE(listg);
            return 0;        /* no mate for listg[i] */
        }
        j--;
    }

    FREE(listf);
    FREE(listg);
    return 1;
}

static int
has_dup_fanin(node)
        node_t *node;
{
    int    new_nin;
    node_t *func_list[1], **list;

    func_list[0] = node;
    new_nin = merge_fanin_list(func_list, 1, &list, node_compare_id);
    FREE(list);

    return new_nin != node->nin;
}


void
node_remove_dup_fanin(node)
        node_t *node;
{
    int         new_nin;
    node_t      *func_list[1], **list;
    pset_family newf;

    func_list[0] = node;
    new_nin = merge_fanin_list(func_list, 1, &list, node_compare_id);

    /* if some input was duplicated, re-adjust the set family */
    if (new_nin != node->nin) {
        newf = node_sf_adjust(node, list, new_nin);
        node_replace_internal(node, list, new_nin, newf);
    } else {
        FREE(list);
    }
    node->is_dup_free = 1;
}

/*
 *  adjust a node to place it over a different base
 *	- watch carefully for duplicated fanin in the node
 */

static pset_family
node_sf_adjust_by_name(node, new_fanin, new_nin)
        node_t *node;
        register node_t **new_fanin;
        int new_nin;
{
    register int    i, j, *permute;
    register node_t **old_fanin;
    int             has_dup, old_nin;
    pset_family     new_sf;
    char            *p1, *p2;

    if (node->is_dup_free) {
        has_dup = 0;
    } else {
        has_dup = has_dup_fanin(node);
        node->is_dup_free = !has_dup;
    }
    old_nin   = node->nin;
    old_fanin = node->fanin;

    /* check for identity permutation */
    if (old_nin == new_nin) {
        for (i = new_nin - 1; i >= 0; i--) {
            p1 = (char *) old_fanin[i];
            p2 = (char *) new_fanin[i];
            if (compare_node_by_name(&p1, &p2) != 0) {
                break;
            }
        }
        if (i < 0) {
            return sf_save(node->F);
        }
    }

    /* setup permute(old column index) = new column index (or -1) */
    permute = ALLOC(
    int, old_nin);
    for (i = old_nin - 1; i >= 0; i--) {
        permute[i] = -1;
        for (j = new_nin - 1; j >= 0; j--) {
            p1 = (char *) old_fanin[i];
            p2 = (char *) new_fanin[j];
            if (compare_node_by_name(&p1, &p2) == 0) {
                permute[i] = j;
                break;
            }
        }
    }

    new_sf = do_sf_permute(node->F, permute, new_nin, has_dup);
    FREE(permute);
    return new_sf;
}

void
make_common_base_by_name(f, g, new_fanin, new_nin, newf, newg)
        node_t *f, *g;
        node_t ***new_fanin;
        int *new_nin;
        pset_family *newf, *newg;
{
    node_t *func_list[2];

    func_list[0] = f;
    func_list[1] = g;
    *new_nin = merge_fanin_list(func_list, 2, new_fanin, compare_node_by_name);

    *newf = node_sf_adjust_by_name(f, *new_fanin, *new_nin);
    *newg = node_sf_adjust_by_name(g, *new_fanin, *new_nin);
}
