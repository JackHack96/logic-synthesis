
#ifdef SIS
#include "sis.h"
#include "clock.h"

#define EPS 1.0e-12    /* EPSILON for checking floating point equality */
#define clock_dup_param(new,old,p) \
	((void)clock_set_parameter(new,p,clock_get_parameter(old,p)))

/*
 * Create the clock signal entry.. It saves the string passed to it
 */
sis_clock_t *
clock_create(name)
char *name;
{
    int i, j;
    sis_clock_t *clock;

    clock = ALLOC(sis_clock_t, 1);
    clock->name = util_strsav(name);
    clock->network = NIL(network_t);

    for(i = RISE_TRANSITION; i <= FALL_TRANSITION; i++){/* RISE and FALL edge */
	clock->edges[i].clock = clock;
	clock->edges[i].transition = i;
	for(j = 0; j <= 1; j++){     /* SPECIFICATION and WORKING */
	    clock->value[i][j].nominal = CLOCK_NOT_SET;
	    clock->value[i][j].upper_range = CLOCK_NOT_SET;
	    clock->value[i][j].lower_range = CLOCK_NOT_SET;
	    clock->dependency[i][j] = lsCreate();
	}
    }

    return clock;
}

/*
 * Free the storage associated with the entry for the clock
 */
void
clock_free(clock)
sis_clock_t *clock;
{
    int i, j;

    FREE(clock->name);
    for(i = RISE_TRANSITION; i <= FALL_TRANSITION; i++){/* RISE and FALL edge */
	for(j = 0; j <= 1; j++){     /* SPECIFICATION and WORKING */
	    LS_ASSERT(lsDestroy(clock->dependency[i][j], (void(*)())0));
	}
    }
    FREE(clock);
}

/*
 * Adds the clock to the network structure
 * Returns 1 if this is a new clock signal, 0 if a clock with the
 * same name is already present. Does not overwrite the previous
 * definition
 */
int
clock_add_to_network(network, clock)
network_t *network;
sis_clock_t *clock;
{
    lsHandle handle;

    if (clock_get_by_name(network, clock->name) != NIL(sis_clock_t)){
	return 0;
    } else {
	LS_ASSERT(lsNewEnd(CLOCK(network)->clock_defn, (lsGeneric)clock, &handle));
	clock->network = network;
	clock->net_handle = handle;
	return 1;
    }
}

void
clock_set_cycletime(network, value)
network_t *network;
double value;
{
    int flag = clock_get_current_setting_index(network);
    CLOCK(network)->cycle_time[flag] = value;
}

double
clock_get_cycletime(network)
network_t *network;
{
    int flag = clock_get_current_setting_index(network);
    return  (CLOCK(network)->cycle_time[flag]);
}

/*
 * Deletes the clock from the network structure. Returns 1 if sucess
 * Frees the memory associated with the clock structure if successful.
 */
int
clock_delete_from_network(network, clock)
network_t *network;
sis_clock_t *clock;
{
    int i, j;
    lsGen gen;
    char *data;
    clock_setting_t setting;
    sis_clock_t *current_clk;
    clock_edge_t edge, new_edge;

    if (network != clock->network){
	error_append("Clock is not part of network\n");
	return 0;
    }

    edge.clock = clock;
    setting = clock_get_current_setting(network);

    clock_set_current_setting(network, SPECIFICATION);
    foreach_clock(network, gen, current_clk){
	if (current_clk != clock){
	    new_edge.clock = current_clk;
	    for (i=RISE_TRANSITION; i <= FALL_TRANSITION; i++){
		edge.transition = i;
		for (j=RISE_TRANSITION; j <= FALL_TRANSITION; j++){
		    new_edge.transition = j;
		    clock_remove_dependency(edge, new_edge);
		}
	    }
	}
    }
    clock_set_current_setting(network, WORKING);
    foreach_clock(network, gen, current_clk){
	if (current_clk != clock){
	    new_edge.clock = current_clk;
	    for (i=RISE_TRANSITION; i <= FALL_TRANSITION; i++){
		edge.transition = i;
		for (j=RISE_TRANSITION; j <= FALL_TRANSITION; j++){
		    new_edge.transition = j;
		    clock_remove_dependency(edge, new_edge);
		}
	    }
	}
    }
    clock_set_current_setting(network, setting);

    gen = lsGenHandle(clock->net_handle, &data, LS_AFTER);
    assert((sis_clock_t *) data == clock);            /* Paranoid */
    LS_ASSERT(lsDelBefore(gen, (lsGeneric *) &current_clk));
    clock_free(current_clk);
    LS_ASSERT(lsFinish(gen));

    return 1;
}

/*
 * Free all the storage associated with the clock data structure
 */
void
network_clock_free(network)
network_t *network;
{
    int i;
    lsGen gen;
    sis_clock_t *clock;
    array_t *clock_array;

    clock_array = array_alloc(sis_clock_t *, 4);
    foreach_clock(network, gen, clock){
	array_insert_last(sis_clock_t *, clock_array, clock);
    }
    for(i = 0; i < array_n(clock_array); i++){
	clock = array_fetch(sis_clock_t *, clock_array, i);
	assert(clock_delete_from_network(network, clock));
    }
    LS_ASSERT(lsDestroy(CLOCK(network)->clock_defn, (void(*)())0));
    FREE(network->CLOCK_SLOT);
    array_free(clock_array);
}

/*
 * When duplicating a network copy the relevant fields for the clocks
 * Note that the network_clock_t strucure iin the network is already allocated
 */
void
network_clock_dup(old, new)
network_t *old, *new;
{
    int i, j;
    char *dummy;
    lsGen gen, gen2;
    clock_setting_t setting;
    sis_clock_t *old_clk, *new_clk;
    clock_edge_t *edge, old_edge, dup_edge, new_edge;

    /*
     * Right now copy the clocking parameters
     */
    setting = clock_get_current_setting(old);
    foreach_clock(old, gen, old_clk){
	new_clk = clock_create(clock_name(old_clk));
	(void)clock_add_to_network(new, new_clk);
	old_edge.clock = old_clk;
	new_edge.clock = new_clk;
	for(j = 0; j <= 1; j++){     /* SPECIFICATION and WORKING */
#ifdef _IBMR2
            clock_set_current_setting(new, (enum clock_setting_enum) j);
            clock_set_current_setting(old, (enum clock_setting_enum) j);
#else
            clock_set_current_setting(new, (clock_setting_t) j);
            clock_set_current_setting(old, (clock_setting_t) j);
#endif
	    clock_set_cycletime(new, clock_get_cycletime(old));
	    for(i = RISE_TRANSITION; i <= FALL_TRANSITION; i++){
		old_edge.transition = new_edge.transition = i;
		clock_dup_param(new_edge, old_edge, CLOCK_NOMINAL_POSITION);
		clock_dup_param(new_edge, old_edge, CLOCK_UPPER_RANGE);
		clock_dup_param(new_edge, old_edge, CLOCK_LOWER_RANGE);
	    }
	}
    }
    /*
     * Now add the dependencies.. Generate old dependencies and figure out the
     * new clocks with those names. Then add dependencies.
     */
    foreach_clock(old, gen, old_clk){
	new_clk = clock_get_by_name(new, clock_name(old_clk));
	assert(new_clk != NIL(sis_clock_t));
	dup_edge.clock = new_clk;
	old_edge.clock = old_clk;
	for(j = 0; j <= 1; j++){     /* SPECIFICATION and WORKING */
#ifdef _IBMR2
            clock_set_current_setting(new, (enum clock_setting_enum) j);
            clock_set_current_setting(old, (enum clock_setting_enum) j);
#else
            clock_set_current_setting(new, (clock_setting_t) j);
            clock_set_current_setting(old, (clock_setting_t) j);
#endif
	    for(i = RISE_TRANSITION; i <= FALL_TRANSITION; i++){
		dup_edge.transition = old_edge.transition = i;
		gen2 = clock_gen_dependency(old_edge);
		while(lsNext(gen2, &dummy, LS_NH) == LS_OK){
		    edge = (clock_edge_t *)dummy;
		    new_edge.transition = edge->transition;
		    new_edge.clock = clock_get_by_name(new, clock_name(edge->clock));
		    assert(new_edge.clock != NIL(sis_clock_t));
		    assert(clock_add_dependency(dup_edge, new_edge));
		}
		(void)lsFinish(gen2);
	    }
	}
    }
    clock_set_current_setting(old, setting);
    clock_set_current_setting(new, setting);
}

/*
 * When creating a new network, this routine initializes the clock field
 */
void
network_clock_alloc(network)
network_t *network;
{
    network_clock_t *clk;

    clk = ALLOC(network_clock_t, 1);
    network->CLOCK_SLOT = (char *)clk;
    CLOCK(network)->clock_defn = lsCreate();
    clock_set_current_setting(network, WORKING);
    clock_set_cycletime(network, CLOCK_NOT_SET);
    clock_set_current_setting(network, SPECIFICATION);
    clock_set_cycletime(network, CLOCK_NOT_SET);
}

/*
 * If edge2 depends on edge1 return TRUE
 */
static int
clock_is_dependent(edge1, edge2)
clock_edge_t edge1, edge2;
{
    lsGen gen;
    int status;
    char *dummy;
    clock_edge_t *new_edge;

    if (edge1.clock == edge2.clock && edge1.transition == edge2.transition)
	return TRUE;

    status = FALSE;
    gen = clock_gen_dependency(edge1);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK){
	new_edge = (clock_edge_t *)dummy;
	if (new_edge->clock == edge2.clock &&
		new_edge->transition == edge2.transition){
	    status = TRUE;
	    break;
	}
    }
    LS_ASSERT(lsFinish(gen));

    return status;
}
static void
clock_add_no_check_dependency(setting, edge1, edge2)
clock_setting_t setting;
clock_edge_t edge1, edge2;
{
    int j = (int)setting;

    (void)lsNewEnd(edge1.clock->dependency[edge1.transition][j],
	    (lsGeneric)&(edge2.clock->edges[edge2.transition]), LS_NH);
    (void)lsNewEnd(edge2.clock->dependency[edge2.transition][j],
	    (lsGeneric)&(edge1.clock->edges[edge1.transition]), LS_NH);
}

/*
 * Add the dependency between specified edges.. Returns 0 if error.
 */
int
clock_add_dependency(edge1, edge2)
clock_edge_t edge1, edge2;
{
    int j;
    char *dummy;
    lsGen gen1, gen2;
    clock_edge_t *new_edge1, *new_edge2;

    if (edge1.clock->network != edge2.clock->network) {  /* Cannot do it */
	return 0;
    }
    if (edge1.clock == edge2.clock && edge1.transition == edge2.transition) {
	/* No dependency between and edge and itself */
	return 1;
    }
    j = clock_get_current_setting_index(edge1.clock->network);

    if (ABS(clock_get_parameter(edge1, CLOCK_NOMINAL_POSITION) -
	    clock_get_parameter(edge2, CLOCK_NOMINAL_POSITION)) > EPS){
	/* Incompatible nominal edge times */
	return 0;
    }

    if (clock_is_dependent(edge1, edge2)){   /* No more work needed */
	return 1;
    }

    clock_add_no_check_dependency(j, edge1, edge2);

    /*
     * Now for the transitive update of the lists... 
     * The current scheme is a bit excessive but so what
     * First add required dependencies between prev members in the list 
     */
    gen1 = clock_gen_dependency(edge1);
    while(lsNext(gen1, &dummy, LS_NH) == LS_OK){
	new_edge1 = (clock_edge_t *)dummy;

	if (new_edge1->clock == edge2.clock &&
		new_edge1->transition == edge2.transition) {
	    /* This was the dependency just added */
	    continue;
	}

	if (!clock_is_dependent(edge2, *new_edge1)){
	    clock_add_no_check_dependency(j, edge2, *new_edge1);
	}
    }
    (void)lsFinish(gen1);
    /*
     * Among members of the other list 
     */
    gen2 = clock_gen_dependency(edge2);
    while(lsNext(gen2, &dummy, LS_NH) == LS_OK){
	new_edge2 = (clock_edge_t *)dummy;

	if (new_edge2->clock == edge1.clock &&
		new_edge2->transition == edge1.transition) {
	    /* This was the dependency just added */
	    continue;
	}

	if (!clock_is_dependent(edge1, *new_edge2)){
	    clock_add_no_check_dependency(j, edge1, *new_edge2);
	}
    }
    (void)lsFinish(gen2);

    /*
     * Make dependencies between the two lists  
     */
    gen2 = clock_gen_dependency(edge2);
    while(lsNext(gen2, &dummy, LS_NH) == LS_OK){
	new_edge2 = (clock_edge_t *)dummy;

	if (new_edge2->clock == edge1.clock &&
		new_edge2->transition == edge1.transition) {
	    /* This was the dependency just added */
	    continue;
	}
	gen1 = clock_gen_dependency(edge1);
	while(lsNext(gen1, &dummy, LS_NH) == LS_OK){
	    new_edge1 = (clock_edge_t *)dummy;
	    if (new_edge1->clock == edge2.clock &&
		    new_edge1->transition == edge2.transition) {
		continue;
	    }
	    if (!clock_is_dependent(*new_edge1, *new_edge2)){
		clock_add_no_check_dependency(j, *new_edge1, *new_edge2);
	    }
	}
	(void)lsFinish(gen1);
    }
    (void)lsFinish(gen2);

    return 1;
}

/*
 * Removes dependencies between two events.
 */
void
clock_remove_dependency(edge1, edge2)
clock_edge_t edge1, edge2;
{
    lsGen gen;
    int delete;
    char *dummy;
    clock_edge_t *new_edge;

    /*
     * Delete dependency of edge2 stored in edge1.. Since clock_is_dependent
     * returns TRUE if edge1 == edge2 we want to check that cond beforehand
     */
    if (edge1.clock == edge2.clock && edge1.transition == edge2.transition){
	return;
    }

    /* Delete edge2 from edge1 */
    delete = 0;
    gen = clock_gen_dependency(edge1);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK){
	new_edge = (clock_edge_t *)dummy;
	if (new_edge->clock == edge2.clock &&
		new_edge->transition == edge2.transition){
	    LS_ASSERT(lsDelBefore(gen, (lsGeneric *)&new_edge));
	    delete++;
	    break;
	}
    }
    (void)lsFinish(gen);

    /* Delete edge1 from edge2 */
    gen = clock_gen_dependency(edge2);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK){
	new_edge = (clock_edge_t *)dummy;
	if (new_edge->clock == edge1.clock &&
		new_edge->transition == edge1.transition){
	    LS_ASSERT(lsDelBefore(gen, (lsGeneric *)&new_edge));
	    delete++;
	    break;
	}
    }

    /* Check if adequate deleteions have been made */
    if (delete != 0 && delete != 2){
	fail("SERIOUS ERROR: asymmetric dependency lists\n");
    }
    (void)lsFinish(gen);
}

/*
 * Returns the generator for the dependent events of edge 
 */
lsGen
clock_gen_dependency(edge)
clock_edge_t edge;
{
    int j;
    if (edge.clock->network == NIL(network_t)){
	fail("Edge not part of network\n");
    }
    j = clock_get_current_setting_index(edge.clock->network);
    return lsStart(edge.clock->dependency[edge.transition][j]);
}

/*
 * Given the name of a clock returns the corresponding clock structure.
 * Returns NIL(sis_clock_t) if none is found
 */
sis_clock_t *
clock_get_by_name(network, clk_name)
network_t *network;
char *clk_name;
{
    lsGen gen;
    char *dummy;
    sis_clock_t *clock;

    gen = lsStart(CLOCK(network)->clock_defn);
    while (lsNext(gen, &dummy, LS_NH) == LS_OK) {
	clock = (sis_clock_t *)dummy;
	if (!strcmp(clock->name, clk_name)){
	    (void)lsFinish(gen);
	    return clock;
	    /* NOTREACHED */
	}
    }
    (void)lsFinish(gen);
    return NIL(sis_clock_t);
}

/*
 * Returns the name of a clock. Do not free the returned string
 */
char *
clock_name(clock)
sis_clock_t *clock;
{
    return clock->name;
}

/*
 * Sets the flag in the data structure specifying the clock data that will
 * be retreived or set.. "mode" can either be SPECIFICATION or WORKING
 */
void
clock_set_current_setting(network, mode)
network_t *network;
clock_setting_t mode;
{
    CLOCK(network)->flag = mode;
}

/*
 * Returns the current value of the clock data flag
 */
clock_setting_t
clock_get_current_setting(network)
network_t *network;
{
    return CLOCK(network)->flag;
}

/*
 * Returns the current value of the clock data flag that can be used
 * to index the data structure ....
 */
int
clock_get_current_setting_index(network)
network_t *network;
{
    clock_setting_t flag = CLOCK(network)->flag;

    if (flag == SPECIFICATION){
	return 0;
    } else if (flag == WORKING) {
	return 1;
    } else {
	/* Should never happen -- return -1 to cause an error on accesing */
	return -1;
    }
}

/*
 * Return the number of clocks in the netwrk
 */
int
network_num_clock(network)
network_t *network;
{
    return lsLength(CLOCK(network)->clock_defn);
}

int
clock_num_dependent_edges(edge)
clock_edge_t edge;
{
    int flag, num;
    network_t *net;

    if ((net = edge.clock->network) == NIL(network_t)){
	return 0;
    }
    flag = clock_get_current_setting_index(net);
    num = lsLength(edge.clock->dependency[edge.transition][flag]);

    return num;
}

/*
 * The user can set parameters for the edges on a clock. The
 * clock must be part of a network for this to work (Since it
 * expects to find the SPECIFICATION/WORKING flag on the network)
 */
int
clock_set_parameter(clock_edge, parameter, value)
clock_edge_t clock_edge;
clock_param_t parameter;
double value;
{
    lsGen gen;
    network_t *network;
    int which_edge, flag;
    clock_edge_t *new_edge;

    if ((network = clock_edge.clock->network) == NIL(network_t)) {
	error_append("Clock not part of network\n");
	return 0;
    }

    flag = clock_get_current_setting_index(network);
    which_edge = clock_edge.transition;

    switch (parameter){
	case CLOCK_NOMINAL_POSITION:
	    clock_edge.clock->value[which_edge][flag].nominal = value;
	    /* 
	     * Also need to shift the dependent events
	     */
	    gen = clock_gen_dependency(clock_edge);
	    while(lsNext(gen, (char **)&new_edge, LS_NH) == LS_OK){
		/*
		 * Cannot use clock_set_parameter()
		 * -- will cause unbounded recursion
		 */
		new_edge->clock->value[new_edge->transition][flag].nominal = value;
	    }
	    (void)lsFinish(gen);
	    break;
	case CLOCK_LOWER_RANGE:
	    clock_edge.clock->value[which_edge][flag].lower_range = value;
	    break;
	case CLOCK_UPPER_RANGE:
	    clock_edge.clock->value[which_edge][flag].upper_range = value;
	    break;
	default:
	    fail("bad enum value in clock_set_parameter");
	    /* NOTREACHED */
    }
    return 1;
}

double
clock_get_parameter(clock_edge, parameter)
clock_edge_t clock_edge;
clock_param_t parameter;
{
    network_t *network;
    int which_edge, flag;
    double t, cycle;

    if ((network = clock_edge.clock->network) == NIL(network_t)) {
	fail("Clock not part of a network\n");
    }

    flag = clock_get_current_setting_index(network);
    cycle = CLOCK(network)->cycle_time[flag];
    which_edge = clock_edge.transition;

    switch (parameter){
	case CLOCK_NOMINAL_POSITION:
	    return clock_edge.clock->value[which_edge][flag].nominal;
	case CLOCK_LOWER_RANGE:
	    return clock_edge.clock->value[which_edge][flag].lower_range;
	case CLOCK_UPPER_RANGE:
	    return clock_edge.clock->value[which_edge][flag].upper_range;
	case CLOCK_ABSOLUTE_VALUE:
	    if (cycle == CLOCK_NOT_SET) {
		return CLOCK_NOT_SET;
	    } else {
		t = clock_edge.clock->value[which_edge][flag].nominal * cycle;
		t /= 100.0;
		return t;
	    }
	default:
	    fail("bad enum value in clock_get_parameter");
	    /* NOTREACHED */
    }
}
#undef EPS /* undefine EPSILION */
#endif /* SIS */
