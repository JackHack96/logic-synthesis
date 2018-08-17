
#include "sis.h"
#include "pld_int.h"

static void join();

/* Initialize the ACT*/
act_t *
my_init_act() {
    act_t *vertex;

    vertex = ALLOC(act_t, 1);
    vertex->high       = NIL(ACT_VERTEX);
    vertex->low        = NIL(ACT_VERTEX);
    vertex->index      = 0;
    vertex->value      = 0;
    vertex->id         = 0;
    vertex->mark       = 0;
    vertex->index_size = 0;
    vertex->node       = NIL(node_t);
    vertex->name       = NIL(
    char);
    vertex->multiple_fo = 0;
    vertex->cost        = 0;
    vertex->mapped      = 0;
    /* vertex->parent = NULL; */
    return vertex;
}


/* Construct bdd for a cube ie a bdd which is the and of its variables*/
act_t *
act_F(F)
        sm_matrix *F;
{
    act_t      *vertex, *t_vertex, *my_init_act(), *my_act_and();
    sm_row     *row;
    sm_element *p;
    int        skip = -1, size;
    char       *name;

/* Avoid stupidity*/
    if (F->nrows > 1) {
        (void) fprintf(sisout, "error in act_f - more than one cube\n");
        return 0;
    }
    row    = F->first_row;
    vertex = my_init_act();
/* vertex->value = 3 is an indication of a dummy vertex*/
    vertex->value = 3;

/* Preprocess so that variable at end of act is not in negative phase (if
 * possible) */
    size = 0;
    sm_foreach_row_element(row, p)
    {
        if ((p->user_word == (char *) 1) && (skip == -1)) {
            name     = sm_get_col(F, p->col_num)->user_word;
            t_vertex = my_act_and(p, vertex, name);
            vertex   = t_vertex;
            skip     = p->col_num;
        }
        if (p->user_word != (char *) 2) size++;
    }

    sm_foreach_row_element(row, p)
    {
        if ((p->user_word != (char *) 2) && (p->col_num != skip)) {
            name     = sm_get_col(F, p->col_num)->user_word;
            t_vertex = my_act_and(p, vertex, name);
            vertex   = t_vertex;
        }
    }
    vertex->index_size = size;
    return vertex;
}

/* And variables in a cube*/
act_t *
my_act_and(p, vertex, name)
        sm_element *p;
        act_t *vertex;
        char *name;
{
    int   is_first = 0;
    act_t *p_vertex, *h_vertex, *l_vertex;
    char  *dummy;
/* Weird initialization for the first vertex, its value is 3*/
    if (vertex->value == 3) {
        my_free_act(vertex);
        is_first = 1;
    }
    p_vertex       = my_init_act();
    p_vertex->value = 4;
    p_vertex->index = p->col_num;
    p_vertex->name  = name;
    (void) st_lookup(end_table, (char *) 1, &dummy);
    h_vertex = (act_t *) dummy;
    (void) st_lookup(end_table, (char *) 0, &dummy);
    l_vertex = (act_t *) dummy;

    if (is_first) {
        p_vertex->index_size = 1;
        switch ((int) p->user_word) {
            case 0: p_vertex->high = l_vertex;
                p_vertex->low      = h_vertex;
                break;
            case 1: p_vertex->high = h_vertex;
                p_vertex->low      = l_vertex;
                break;
            case 2: (void) fprintf(sisout, "error in my_and_act,\n");
                break;
        }
    } else {
        switch ((int) p->user_word) {
            case 0: p_vertex->low    = vertex;
                p_vertex->high       = l_vertex;
                p_vertex->index_size = vertex->index_size + 1;
                break;
            case 1: p_vertex->high   = vertex;
                p_vertex->low        = l_vertex;
                p_vertex->index_size = vertex->index_size + 1;
                break;
            case 2: (void) fprintf(sisout, "error in my_and act, p->u_w == 2\n");
                break;

        }
    }
    return p_vertex;
}

/* Or 2 ACT's*/
act_t *
my_or_act_F(array_b, cover, array)
        array_t *array_b;
        array_t *array;
        sm_row *cover;
{
    static int compare();
    int        i;
    act_t      *up_vertex, *down_vertex, *vertex;
    sm_element *p;
    act_t      *act;
    char       *name;

/* Append the appropriate variables at top of acts*/
    i = 0;
    sm_foreach_row_element(cover, p)
    {
        name   = sm_get_col(array_fetch(sm_matrix * , array, i), p->col_num)
                ->user_word;
        vertex = array_fetch(act_t * , array_b, i);
        act    = my_act_and(p, vertex, name);
        array_insert(act_t * , array_b, i, act);
        i++;
    }
/* Sort the cubes so that the smaller act s are at the top so that less fanout 
 * problem also we could use the heuristic merging done earlier*/
    array_sort(array_b, compare);

    up_vertex = array_fetch(act_t * , array_b, 0);
    act       = up_vertex;
    for (i    = 1; i < array_n(array_b); i++) {
        down_vertex = array_fetch(act_t * , array_b, i);
        join(up_vertex, up_vertex, down_vertex);
        up_vertex = down_vertex;
    }
    return act;
}

/* Recursively Join ACT's constructed*/
static void
join(parent_vertex, vertex, down_vertex)
        act_t *vertex, *down_vertex, *parent_vertex;
{


    if ((vertex->low == NIL(act_t)) && (vertex->high == NIL(act_t))) {
        if (parent_vertex->low->value == 0) {
            parent_vertex->low = down_vertex;
        } else if (parent_vertex->high->value == 0) {
            parent_vertex->high = down_vertex;
        }
    }
    if ((vertex->low != NIL(act_t)) && (vertex->low != down_vertex)) {
        join(vertex, vertex->low, down_vertex);
        if ((vertex->high != NIL(act_t)) && (vertex->high != down_vertex)) {
            join(vertex, vertex->high, down_vertex);
        }
    }
}



/* ACT construction where act_l and act_r are known and we have a binate
varible above the two eg f = a (..) + a'(..)*/
act_t *
my_and_act(act_l, act_r, b_col, name)
        act_t *act_l, *act_r;
        int b_col;
        char *name;
{
    act_t *act;

    act = my_init_act();
    act->value = 4;
    act->index = b_col;
    act->name  = name;


    act->index_size = act_l->index_size + act_r->index_size + 1;
    act->low        = act_l;
    act->high       = act_r;
    return act;
}


trace_act(vertex)
act_t *vertex;
{
if ((vertex->low !=
NIL (act_t)
) && (vertex->high !=
NIL (act_t)
)) {
(void)
fprintf(sisout,
"current variable - %s\n", vertex->name);
}

if (vertex->low !=
NIL (act_t)
) {
trace_act(vertex
->low);
if (vertex->high !=
NIL (act_t)
) {
trace_act(vertex
->high);
}
}

if ((vertex->low ==
NIL (act_t)
) && (vertex->high ==
NIL (act_t)
)) {
(void)
fprintf(sisout,
"END- %d\n", vertex->value);
}
}

static int
compare(obj1, obj2)
        char *obj1, *obj2;
{
    act_t * act1 = *(act_t **) obj1;
    act_t * act2 = *(act_t **) obj2;
    int value;

    if ((act1->index_size == 0) || (act2->index_size == 0)) {
        (void) fprintf(sisout, "Hey u DOLT act of size \n");
    }
    if (act1->index_size == act2->index_size) value = 0;
    if (act1->index_size > act2->index_size) value  = 1;
    if (act1->index_size < act2->index_size) value  = -1;
    return value;
}

