
#include <math.h>
#include "util.h"
#include "array.h"
#include "st.h"
#include "var_set.h"
#include "bdd.h"
#include "bdd_int.h"

typedef struct {
    double count;    /* result */
    int    index;       /* index in vars array */
} count_cache_t;

static void extract_bdd_support();

/* returns the support (in terms of primary inputs) of a BDD */
/* the result is returned as a bit array */
/* bdd read-only so no need for protection */

var_set_t *bdd_get_support(fn)
        bdd_t *fn;
{
    bdd_manager *manager = fn->bdd;
    st_table    *visited = st_init_table(st_ptrcmp, st_ptrhash);
    var_set_t   *result  = var_set_new((int) manager->bdd.nvariables);

    (void) extract_bdd_support(result, fn->node, fn->bdd, visited);
    (void) st_free_table(visited);
    return result;
}

static void extract_bdd_support(set, fn, manager, visited)
        var_set_t *set;
        bdd_node *fn;
        bdd_manager *manager;
        st_table *visited;
{
    bdd_node     *zero = BDD_ZERO(manager);
    bdd_node     *one  = BDD_ONE(manager);
    bdd_node     *f0, *f1;
    unsigned int topf;

    if (fn == zero || fn == one) return;
    if (st_lookup(visited, (char *) fn, NIL(char *))) return;
    topf = BDD_REGULAR(fn)->id;
    (void) var_set_set_elt(set, (int) topf);
    (void) bdd_get_branches(fn, &f1, &f0);
    (void) extract_bdd_support(set, f1, manager, visited);
    (void) extract_bdd_support(set, f0, manager, visited);
    (void) st_insert(visited, (char *) fn, NIL(char));
}


static double bdd_do_count_onset();

/* counts the number of minterms in the onset of a bdd */
/* bdd read-only so no need for protection */
/* var_array is an array of variables (as bdd_t *'s) we care about */

double bdd_count_onset(fn, var_array)
        bdd_t *fn;
        array_t *var_array;
{
    bdd_node     *node;
    bdd_manager  *bdd;
    double       result;
    st_generator *gen;
    char         *key, *value;
    array_t      *vars;
    st_table     *visited = st_init_table(st_ptrcmp, st_ptrhash);

    assert(fn != NIL(bdd_t));
    assert(var_array != NIL(array_t));
    bdd  = fn->bdd;
    node = fn->node;
    vars = bdd_get_sorted_varids(var_array);
    if (array_n(vars) == 0) {
        result = 0.0;
    } else {
        result = bdd_do_count_onset(node, bdd, 0, vars, visited);
    }
    st_foreach_item(visited, gen, (char **) &key, (char **) &value)
    {
        FREE(value);
    }
    (void) st_free_table(visited);
    (void) array_free(vars);
    return result;
}


/* we put in the cache only the entries where top_var == topf */
/* vars is an array listing the variables that have to be considered */
/* it is a fatal error if a variable in f does not appear in vars */
/* the count is relative to 2^array_n(vars) */

static double bdd_do_count_onset(f, bdd, top_index, vars, visited)
        bdd_node *f;
        bdd_manager *bdd;
        int top_index;
        array_t *vars;
        st_table *visited;
{
    int           topf, top_var;
    bdd_node      *one  = BDD_ONE(bdd);
    bdd_node      *zero = BDD_ZERO(bdd);
    bdd_node      *f1, *f0;
    count_cache_t *value;
    double        result;

    if (st_lookup(visited, (char *) f, (char **) &value)) {
        assert(f != one && f != zero);
        assert(top_index <= value->index);
        return value->count * pow((double) 2.0, (double) value->index - top_index);
    }
    if (f == zero) {
        return 0.0;
    } else if (f == one) {
        return pow((double) 2, (double) array_n(vars) - top_index);
    } else {
        (void) bdd_get_branches(f, &f1, &f0);
        topf    = BDD_REGULAR(f)->id;
        top_var = array_fetch(
        int, vars, top_index);
        assert(top_var <= topf);
        if (topf == top_var) {
            result = bdd_do_count_onset(f1, bdd, top_index + 1, vars, visited)
                     + bdd_do_count_onset(f0, bdd, top_index + 1, vars, visited);
            value  = ALLOC(count_cache_t, 1);
            value->count = result;
            value->index = top_index;
            (void) st_insert(visited, (char *) f, (char *) value);
            return result;
        } else {
            return 2 * bdd_do_count_onset(f, bdd, top_index + 1, vars, visited);
        }
    }
}
