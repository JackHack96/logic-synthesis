/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/stg/stg.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 *
 */
#ifdef SIS
#include "sis.h"
#include "stg_int.h"

graph_t *
stg_alloc()
{
    graph_t *new;

    new = g_alloc_static(NUM_G_SLOTS, NUM_S_SLOTS, NUM_T_SLOTS);
    g_set_g_slot_static(new, STG_INPUT_NAMES, (gGeneric)0);
    g_set_g_slot_static(new, STG_OUTPUT_NAMES, (gGeneric)0);
    g_set_g_slot_static(new, CLOCK_DATA, (gGeneric)0);
    return(new);
}


/*
 * Free the arrays associated with the input and output names
 */
static void
stg_free_g(thing)
gGeneric thing;
{
    array_t *x;
    char *name;
    int i, j, param;
    stg_clock_t *ptr;

    for ( j = 0; j < 2; j++){
	param = (j == 0 ? STG_INPUT_NAMES : STG_OUTPUT_NAMES);
	x = (array_t *)(((gGeneric *) thing)[param]);
	if (x != NIL(array_t)){
	    for( i = array_n(x); i-- > 0; ){
	       name = array_fetch(char *, x, i);
	       FREE(name);
	    }
	    array_free(x);
	}
    }
    ptr = (stg_clock_t *)(((gGeneric *) thing)[CLOCK_DATA]);
    if (ptr != NIL(stg_clock_t)) {
	FREE(ptr->name);
	FREE(ptr);
    }
    return;
}

static void
stg_free_v(thing)
gGeneric thing;
{
    gGeneric x;

    x = ((gGeneric *) thing)[STATE_STRING];
    FREE(x);
    x = ((gGeneric *) thing)[ENCODING_STRING];
    FREE(x);
    return;
}

static void
stg_free_e(thing)
gGeneric thing;
{
    gGeneric x;

    x = ((gGeneric *) thing)[INPUT_STRING];
    FREE(x);
    x = ((gGeneric *) thing)[OUTPUT_STRING];
    FREE(x);
}


void
stg_free(stg)
graph_t *stg;
{
    g_free_static(stg, stg_free_g, stg_free_v, stg_free_e);
    return;
}

    

static gGeneric
stg_copy_v(old)
gGeneric old;
{
    gGeneric *new = ALLOC(gGeneric,NUM_S_SLOTS);
    gGeneric x;

    new[STATE_STRING] = util_strsav(((gGeneric *) old)[STATE_STRING]);
    x = ((gGeneric *) old)[ENCODING_STRING];
    if (x) {
	new[ENCODING_STRING] = util_strsav(((gGeneric *) old)[ENCODING_STRING]);
    } else {
	new[ENCODING_STRING] = (gGeneric) 0;
    }
    return((gGeneric) new);
}


static gGeneric
stg_copy_e(old)
gGeneric old;
{
    gGeneric *new = ALLOC(gGeneric,NUM_T_SLOTS);

    new[INPUT_STRING] = util_strsav(((gGeneric *) old)[INPUT_STRING]);
    new[OUTPUT_STRING] = util_strsav(((gGeneric *) old)[OUTPUT_STRING]);
    return((gGeneric) new);
}

void
stg_copy_names(old, new)
graph_t *old, *new;
{
    array_t *x, *newx;
    char *name;
    int i, j;

    for ( j = 0; j < 2; j++){	/* for inputs (j = 0) and for outputs (j = 1) */
	x = stg_get_names(old, j);
	newx = x;
	if (x != NIL(array_t)){
	    newx = array_alloc(char *, array_n(x));
	    for (i = array_n(x); i-- > 0; ){
		name = array_fetch(char *, x, i);
		array_insert(char *, newx, i, util_strsav(name));
	    }
	}
	stg_set_names(new, newx, j);
    }
}

/*
 *Routine to copy the clock and edge_index fields 
 * Must be called to preserve the clocking info stored
 */
void
stg_copy_clock_data(old, new)
graph_t *old, *new;
{
    stg_clock_t *clock, *new_clock;

    clock = stg_get_clock_data(old);
    if (clock != NIL(stg_clock_t)){
	/* Copy the clocking data */
	new_clock = ALLOC(stg_clock_t, 1);
	*new_clock = *clock;
	new_clock->name = util_strsav(clock->name);
	stg_set_clock_data(new, new_clock);
    } else {
	stg_set_clock_data(new, NIL(stg_clock_t));
    }
    stg_set_edge_index(new, stg_get_edge_index(old));
}
    
graph_t *
stg_dup(stg)
graph_t *stg;
{
    graph_t *new;
    vertex_t *start, *start2;
    vertex_t *current, *current2;
    

    if (stg == NIL(graph_t)) {
        return(NIL(graph_t));
    }
    new = g_dup_static(stg, (gGeneric (*)()) 0, stg_copy_v, stg_copy_e);

    /* fix the pointers to the start state and the current state */

    start = stg_get_start(stg);
    start2 = stg_get_state_by_name(new, stg_get_state_name(start));
    stg_set_start(new, start2);
    current = stg_get_current(stg);
    current2 = stg_get_state_by_name(new, stg_get_state_name(current));
    stg_set_current(new, current2);
    stg_copy_names(stg, new);
    stg_copy_clock_data(stg, new);
    return(new);
}


static int
equivtrans(x,y)
char *x,*y;
{
    char c;

    while ((c = *x++) != '\0') {
        if (c != '-' && *y != '-' && c != *y) {
	    return(FALSE);
	}
	y++;
    }
    return(TRUE);
}

    
int
stg_check(stg)
graph_t *stg;
{
    lsGen gen, gen2;
    vertex_t *vert, *start, *any_state;
    lsList edge_list, any_edge_list;
    edge_t *edge;
    char **input_array;
    char **output_array;
    char **ns_array;
    array_t *name_array;
    int i,j,len;
    int return_code = 1;
    int code_length = 0;
    int state_len;

    if (stg == NIL(graph_t)) {
	return 1;
    }
    g_check(stg);

    start = (vertex_t *) g_get_g_slot_static(stg, START);
    if (!start) {
	(void) fprintf(siserr,
		       "Fatal error in stg_check: no start state specified");
	return 0;
    }
    if (stg_get_state_encoding(start) != NIL(char)) {
        code_length = (int) strlen(stg_get_state_encoding(start));
    }
    if (!g_get_g_slot_static(stg,CURRENT)) {
        (void) fprintf(siserr,
		"Warning in stg_check: no current state specified\n");
	g_set_g_slot_static(stg,CURRENT,(gGeneric) start);
    }
    name_array = stg_get_names(stg, 1);
    if (name_array != NIL(array_t)  &&
	     array_n(name_array) != stg_get_num_inputs(stg)){
	(void) fprintf(siserr,
		"Warning in stg_check: incorrect number of input names.\n");
    }
    name_array = stg_get_names(stg, 0);
    if (name_array != NIL(array_t)  &&
	     array_n(name_array) != stg_get_num_outputs(stg)){
	(void) fprintf(siserr,
		"Warning in stg_check: incorrect number of output names.\n");
    }
    any_state = stg_get_state_by_name(stg, "ANY");
    if (any_state == NIL(vertex_t)) {
	any_state = stg_get_state_by_name(stg, "*");
    }
    if (any_state != NIL(vertex_t)) {
        any_edge_list = g_get_out_edges(any_state);
    }
    foreach_vertex (stg,gen,vert) {
  	state_len = 0;
	if (stg_get_state_encoding(vert) != NIL(char)) {
	    state_len = (int) strlen(stg_get_state_encoding(vert));
        }	
	if (state_len != code_length) {
	    (void) fprintf(siserr, "Fatal error in stg_check:  state %s has an encoding with the wrong number of bits\n", stg_get_state_name(vert));
	    return_code = 0;
	    goto bad_exit2;
	}
        if (vert != start && lsLength(g_get_in_edges(vert)) == 0) {
	    if (vert != any_state) {
	        (void) fprintf(siserr,
                               "Warning in stg_check: unreachable vertex, %s\n",
                               stg_get_state_name(vert));
	    }
	}
	edge_list = g_get_out_edges(vert);
	len = lsLength(edge_list);
	if (any_state != NIL(vertex_t)) {
	    len += lsLength(any_edge_list);
	}
	switch(len) {
	    case 0:
	        (void) fprintf(siserr,
			"Warning in stg_check: vertex does not fanout, %s\n",
			     stg_get_state_name(vert));
	    case 1:
	        break;
	    default:
	        input_array = ALLOC(char *,len);
	        output_array = ALLOC(char *,len);
	        ns_array = ALLOC(char *,len);
		i = 0;
		foreach_out_edge (vert,gen2,edge) {
		    input_array[i] = g_get_e_slot_static(edge,INPUT_STRING);
                    output_array[i] = g_get_e_slot_static(edge,OUTPUT_STRING);
		    ns_array[i] = stg_get_state_name(g_e_dest(edge));
		    for (j = i - 1; j >= 0; j--) {
		       if (equivtrans(input_array[i], input_array[j])) {
			   if (strcmp(output_array[i], output_array[j]) ||
                               strcmp(ns_array[i], ns_array[j])) {
			       (void) fprintf(siserr, "Fatal error in stg_check: machine is not deterministic (see state %s)\n", stg_get_state_name(vert));
			       return_code = 0;
			       goto bad_exit1;
			   }
		       }
		    }
		    i++;
		}

		/* All the out-edges of state "ANY" are also out-edges of
                   every other state - so check those also */
		if (any_state != NIL(vertex_t)) {
		    foreach_out_edge(any_state, gen2, edge) {
		        input_array[i] = g_get_e_slot_static(edge,INPUT_STRING);
                        output_array[i] = g_get_e_slot_static(edge,OUTPUT_STRING);
		        ns_array[i] = stg_get_state_name(g_e_dest(edge));
		        for (j = i - 1; j >= 0; j--) {
		            if (equivtrans(input_array[i], input_array[j])) {
			        if (strcmp(output_array[i], output_array[j]) ||
                                    strcmp(ns_array[i], ns_array[j])) {
			            (void) fprintf(siserr, "Fatal error in stg_check: machine is not deterministic (see state %s)\n", stg_get_state_name(vert));
			            return_code = 0;
			            goto bad_exit1;
			        }
			    }
		        }
		        i++;
		    }
		}
		FREE(input_array);
		FREE(output_array);
		FREE(ns_array);
	}
    }
    return return_code;
bad_exit1:
    FREE(input_array);
    FREE(output_array);
    FREE(ns_array);
bad_exit2:
    lsFinish(gen);
    return return_code;
}


void
stg_reset(stg)
graph_t *stg;
{
    if (!stg) {
        return;
    }
    g_set_g_slot_static(stg,CURRENT,g_get_g_slot_static(stg,START));
}


void
stg_sim(stg,vector)
graph_t *stg;
char *vector;
{
    vertex_t *current, *next;
    edge_t *edge;
    char *input;
    lsGen gen;
    
    current = (vertex_t *) g_get_g_slot_static(stg, CURRENT);

    foreach_out_edge(current, gen, edge) {
        input = (char *) g_get_e_slot_static(edge, INPUT_STRING);
	if (equivtrans(input, vector)) {
	    next = g_e_dest(edge);
	    (void) fprintf(sisout,"%s %s %s %s\n",input,
	    	    g_get_v_slot_static(current,STATE_STRING),
		    g_get_v_slot_static(next,STATE_STRING),
		    g_get_e_slot_static(edge,OUTPUT_STRING));
	    g_set_g_slot_static(stg,CURRENT,(gGeneric) next);
	    (void) lsFinish(gen);
	    return;
	}
    }
    (void) fprintf(siserr,"in stg_sim: next state is undeterminable\n");
    return;
}

void
stg_set_names(stg, name_array, flag)
graph_t *stg;
array_t *name_array;	/* array of names of the inputs or outputs */
int flag;	/* == 1 => inputs, 0 => outputs */
{
    int param;
    param = (flag ? STG_INPUT_NAMES : STG_OUTPUT_NAMES);
    g_set_g_slot_static(stg, param, (gGeneric)name_array);
    return;
}

array_t *
stg_get_names(stg, flag)
graph_t *stg;
int flag;	/* == 1 => inputs, 0 => outputs */
{
    int param;
    array_t *name_array;

    param = (flag ? STG_INPUT_NAMES : STG_OUTPUT_NAMES);
    name_array = (array_t *)g_get_g_slot_static(stg, param);
    return name_array;
}

void
stg_set_start(stg, v)
graph_t *stg;
vertex_t *v;
{
    if (g_vertex_graph(v) != stg) {
	fail("State is not part of the give stg");
    }
    g_set_g_slot_static(stg, START, (gGeneric) v);
    return;
}

void
stg_set_current(stg, v)
graph_t *stg;
vertex_t *v;
{
    if (g_vertex_graph(v) != stg) {
	fail("State is not part of the given stg");
    }
    g_set_g_slot_static(stg, CURRENT, (gGeneric) v);
    return;
}


vertex_t *
stg_create_state(stg, name, encoding)
graph_t *stg;
char *name, *encoding;
{
    vertex_t *v;

    v = g_add_vertex_static(stg);
    if (name == NIL(char)) {
	g_set_v_slot_static(v, STATE_STRING, (gGeneric) 0);
    } else {
	g_set_v_slot_static(v, STATE_STRING, (gGeneric) util_strsav(name));
    }
    if (encoding == NIL(char)) {
	g_set_v_slot_static(v, ENCODING_STRING, (gGeneric) 0);
    } else {
	g_set_v_slot_static(v, ENCODING_STRING, (gGeneric) util_strsav(encoding));
    }
    g_set_g_slot_static(stg, NUM_STATES,
    	    (gGeneric) ((int) g_get_g_slot_static(stg, NUM_STATES) + 1));
    return v;
}


edge_t *
stg_create_transition(from, to, in, out)
vertex_t *from, *to;
char *in, *out;
{
    edge_t *edge;
    lsGen gen;
    char *input;
    graph_t *stg;

    stg = g_vertex_graph(from);
    if (stg != g_vertex_graph(to)) {
	fail("Vertices to and from belong to different stgs");
    }
    if (strlen(in) != (int) g_get_g_slot_static(stg, NUM_INPUTS)) {
        fail("In stg_create_transition: Invalid number of inputs specified");
    }
    if (strlen(out) != (int) g_get_g_slot_static(stg, NUM_OUTPUTS)) {
        fail("In stg_create_transition: Invalid number of outputs specified");
    }
    foreach_out_edge (from, gen, edge) {
      if (g_e_dest(edge) != to) {
         input = (char *) g_get_e_slot_static(edge, INPUT_STRING);
	 if (equivtrans(input, in)) {
	    fail("stg_create_transition: Same transition to different states");
	 }
      }
    }        
    edge = g_add_edge_static(from, to);
    g_set_e_slot_static(edge, INPUT_STRING, (gGeneric) util_strsav(in));
    g_set_e_slot_static(edge, OUTPUT_STRING, (gGeneric) util_strsav(out));
    g_set_g_slot_static(stg,NUM_PRODUCTS,
    	    (gGeneric) ((int) g_get_g_slot_static(stg, NUM_PRODUCTS) + 1));
    return(edge);
}


vertex_t *
stg_get_state_by_name(stg, name)
graph_t *stg;
char *name;
{
    lsGen gen;
    vertex_t *s;

    stg_foreach_state(stg, gen, s) {
	if (!strcmp(stg_get_state_name(s), name)) {
	    (void) lsFinish(gen);
	    return s;
	}
    }
    return NIL(vertex_t);
}


vertex_t *
stg_get_state_by_encoding(stg, encoding)
graph_t *stg;
char *encoding;
{
    lsGen gen;
    vertex_t *s;

    stg_foreach_state(stg, gen, s) {
	if (!strcmp(stg_get_state_encoding(s), encoding)) {
	    (void) lsFinish(gen);
	    return s;
	}
    }
    return NIL(vertex_t);
}


void
stg_set_state_name(v, n)
vertex_t *v;
char *n;
{
    char *oldname;

    oldname = stg_get_state_name(v);
    if (oldname != NIL(char)) {
	FREE(oldname);
    }
    g_set_v_slot_static((v), STATE_STRING, (gGeneric) (util_strsav(n)));
    return;
}


void
stg_set_state_encoding(v, n)
vertex_t *v;
char *n;
{
    char *oldname;

    oldname = stg_get_state_encoding(v);
    if (oldname != NIL(char)) {
	FREE(oldname);
    }
    g_set_v_slot_static((v), ENCODING_STRING, (gGeneric) (util_strsav(n)));
    return;
}
#endif /* SIS */
