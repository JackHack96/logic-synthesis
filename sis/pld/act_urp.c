
/* Created by Narendra */
#include "sis.h"
#include "pld_int.h"

st_table *end_table;

static sm_matrix *build_F();

static act_t *urp_F();

static int good_bin_var();

static int find_var();

static void split_F();

static void update_F();

/* We need this to initialise the 0 and 1 act s and store them 
 * in a table so they can be easily accessed*/

make_act(node)
node_t *node;
{
sm_matrix *F;
act_t *act, *my_init_act();
ACT_ENTRY *act_entry;
ACT       *act_f;

/* Init a global lookup table for terminal vertices*/
end_table = st_init_table(st_ptrcmp, st_ptrhash);
act       = my_init_act();
act->
value = 0;
(void)
st_insert(end_table,
(char *)0, (char *)act);
act = my_init_act();
act->
value = 1;
(void)
st_insert(end_table,
(char *)1, (char *)act);
/* initialize Matrix for node*/
F              = build_F(node);
if (ACT_DEBUG)
my_sm_print(F);
/* construct act for node*/
           act = urp_F(F);
/* Free the st_table now*/
st_free_table(end_table);
act_entry = ALLOC(ACT_ENTRY, 1);
act_entry->
order_style = 0;
ACT_SET(node)->LOCAL_ACT =
act_entry;
ACT_SET(node)->GLOBAL_ACT =
NIL (ACT_ENTRY);
    act_f   = ALLOC(ACT, 1);
act_f->
root = act;
act_f->
node_list = NIL(array_t);
act_f->
node_name = util_strsav(node_name(node));
ACT_SET(node)->LOCAL_ACT->
act = act_f;
if (ACT_DEBUG) {
trace_act(act);
}
/* Free up the space*/
sm_free_space(F);
}



/* Constructs the cover of the node. We use the sm package because we 
   need to solve a column covering problem at the unate leaf*/
static
sm_matrix *
build_F(node)
        node_t *node;
{
    int            i, j;
    node_cube_t    cube;
    node_literal_t literal;
    sm_matrix      *F;
    node_t         *fanin;
    sm_col         *p_col;

    F = sm_alloc();

    for (i = node_num_cube(node) - 1; i >= 0; i--) {
        cube = node_get_cube(node, i);
        foreach_fanin(node, j, fanin)
        {
            literal = node_get_literal(cube, j);
            switch (literal) {
                case ONE: sm_insert(F, i, j)->user_word = (char *) 1;
                    break;
                case ZERO: sm_insert(F, i, j)->user_word = (char *) 0;
                    break;
                case TWO: sm_insert(F, i, j)->user_word = (char *) 2;
                    break;
                default: (void) fprintf(sisout, "error in F construction \n");
                    break;
            }
        }
    }
/* STuff the fanin names in the user word for each column*/
    foreach_fanin(node, j, fanin)
    {
        p_col = sm_get_col(F, j);
        p_col->user_word = node_long_name(fanin);
    }
    return F;
}



/* Recurse !! using Unate recursive paradigm(modified)*/
static
act_t *
urp_F(F)
        sm_matrix *F;
{
    int        b_col, ex;
    act_t      *act_l, *act_r, *my_and_act(), *act, *unate_act();
    sm_matrix  *Fl, *Fr;
    sm_element *p;
    char       *dummy;
    char       *b_name;
    int        value;

    ex    = 0;
    b_col = good_bin_var(F, &ex);
    if (ACT_DEBUG) {
        (void) fprintf(sisout, "The most binate col is %d\n", b_col);
    }
    if (b_col != -1) {
        if (!ex) {
            b_name = sm_get_col(F, b_col)->user_word;
            split_F(F, b_col, &Fl, &Fr);
            if (ACT_DEBUG) {
                (void) fprintf(sisout, "factored wrt %d'\n", b_col);
                my_sm_print(Fl);
            }
            act_l = urp_F(Fl);
            if (ACT_DEBUG) {
                (void) fprintf(sisout, "factored wrt %d \n", b_col);
                my_sm_print(Fr);
            }
            act_r = urp_F(Fr);
            sm_free_space(Fl);
            sm_free_space(Fr);
            act = my_and_act(act_l, act_r, b_col, b_name);
        } else {
/* Common variable to all cubes*/
            b_name = sm_get_col(F, b_col)->user_word;
            p      = sm_find(F, 0, b_col);
            value  = (int) p->user_word;
            if ((value != 1) &&
                (value != 0)) {
                (void) fprintf(sisout, "ERROR in Common Variable\n");
            }
            if (value == 1) {
                (void) st_lookup(end_table, (char *) 0, &dummy);
                act_l = (act_t *) dummy;
                update_F(F, b_col);
                if (ACT_DEBUG) {
                    my_sm_print(F);
                }
                act_r = urp_F(F);
            }
            if (value == 0) {
                (void) st_lookup(end_table, (char *) 0, &dummy);
                act_r = (act_t *) dummy;
                update_F(F, b_col);
                if (ACT_DEBUG) {
                    my_sm_print(F);
                }
                act_l = urp_F(F);
            }
            act    = my_and_act(act_l, act_r, b_col, b_name);
        }
    } else {
        act = unate_act(F);
    }
    return act;

}


/* Choose good variable for co-factoring*/
static int
good_bin_var(A, ex)
        sm_matrix *A;
        int *ex;

{
    int        *o, *z, i, var;
    sm_col     *pcol;
    sm_element *p;

    o = ALLOC(
    int, A->ncols);
    z = ALLOC(
    int, A->ncols);
    for (i = 0; i < A->ncols; i++) {
        o[i] = 0;
        z[i] = 0;
    }

    i = 0;

    sm_foreach_col(A, pcol)
    {
        sm_foreach_col_element(pcol, p)
        {
            switch ((int) p->user_word) {
                case 1: o[i]++;
                    break;
                case 2: break;
                case 0: z[i]++;
                    break;
                default: (void) fprintf(sisout, "Err in sm->user_word\n");
                    break;
            }
        }
        if (o[i] == A->nrows) {
            *ex = 1;
            FREE(o);
            FREE(z);
            return i;
        }
        if (z[i] == A->nrows) {
            *ex = 1;
            FREE(o);
            FREE(z);
            return i;
        }
        i++;
    }
    var = find_var(o, z, A->ncols);
    FREE(o);
    FREE(z);
    return var;
}



static int
find_var(o, z, size)
        int *o, *z, size;
{
    int i, b_b = 0, b_d = 100, var;

    var    = -1;
    for (i = 0; i < size; i++) {
        if ((o[i] > 0) && (z[i] > 0)) {
            if (b_b < o[i] + z[i]) {
                b_b = o[i] + z[i];
                b_d = abs(o[i] - z[i]);
                var = i;
            } else if (b_b == o[i] + z[i]) {
                if (abs(o[i] - z[i]) < b_d) {
                    b_d = abs(o[i] - z[i]);
                    var = i;
                }
            }
        }
    }
    return var;
}


/* Divide matrix into the 2 co-factored matrices*/
static void
split_F(F, b_col, Fl, Fr)
        sm_matrix *F, **Fl, **Fr;
        int b_col;
{
    sm_matrix  *Fhi, *Flo;
    sm_row     *F_row;
    sm_col     *pcol;
    sm_element *p;

    Fhi  = sm_alloc();
    Flo  = sm_alloc();
    pcol = sm_get_col(F, b_col);
    sm_foreach_col_element(pcol, p)
    {
        F_row = sm_get_row(F, p->row_num);
        switch ((int) p->user_word) {
            case 0: my_sm_copy_row(F_row, Flo, b_col, F);
                break;
            case 1: my_sm_copy_row(F_row, Fhi, b_col, F);
                break;
            case 2: my_sm_copy_row(F_row, Flo, b_col, F);
                my_sm_copy_row(F_row, Fhi, b_col, F);
                break;
            default: (void) fprintf(sisout, "error in duplicating row\n");
                break;
        }
    }
    *Fl = Flo;
    *Fr = Fhi;
}


my_sm_copy_row(F_row, F, col, Fp
)
sm_row    *F_row;
sm_matrix *F, *Fp;
int       col;
{
int j;
int row;

row = F->nrows;
for (
j = 0;
j< F_row->
length;
j++){
if (j != col) {
sm_insert(F, row, j
)->
user_word =
sm_row_find(F_row, j)->user_word;
} else if (j == col) {
sm_insert(F, row, j
)->
user_word = (char *) 2;
}
sm_get_col(F, j
)->
user_word = sm_get_col(Fp, j)->user_word;
}
}

/* For debug*/
my_sm_print(F)
sm_matrix *F;
{
sm_row     *row;
sm_element *p;

(void)
fprintf(sisout,
"Printing F matrix\n");
sm_foreach_row(F, row
) {
sm_foreach_row_element(row, p
){
(void)
fprintf(sisout,
"%d", (int *)p->user_word);
}
(void)
fprintf(sisout,
"\n");
}
}


/* Set co-factored variable to 2*/
static void
update_F(F, b_col)
        sm_matrix *F;
        int b_col;
{
    sm_col     *p_col;
    sm_element *p;

    p_col = sm_get_col(F, b_col);
    sm_foreach_col_element(p_col, p)
    {
        p->user_word = (char *) 2;
    }
}
