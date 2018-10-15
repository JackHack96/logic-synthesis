/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/speed/weight_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:51 $
 *
 */
#include "sis.h"
#include "speed_int.h"
#include <math.h>

#define SCALE 100.0
#define SCALE_2 10000

st_table *
speed_compute_weight(network, speed_param, array)
network_t *network;
speed_global_t *speed_param;
array_t *array; /* List of nodes whose delays are to be printed */
{
    st_table *table, *crit_table, *weight_table;
    st_generator *stgen;
    node_t *np;
    lsGen gen;
    int weight, level, temp, a_weight, min_area, max_area;
    double min_time, max_time, t_range, a_range, t_weight, tmp_weight;
    delay_time_t *temp_weight, *t;
    array_t *w_array, *level_array;
    char *dummy, *dummy1;
    int i, max_level;

    /* Initializations */
    table = st_init_table(st_ptrcmp, st_ptrhash);
    weight_table = st_init_table(st_ptrcmp, st_ptrhash);
    min_time = POS_LARGE;
    max_time = NEG_LARGE;
    min_area = POS_LARGE;
    max_area = NEG_LARGE;

    /* Store the level of the critical signal in the table */
    crit_table = speed_levelize_crit(network, speed_param, &max_level);

    /* Build an array such that the entry holds the
    numer of crit signals at that level */
    w_array = array_alloc(node_t *, 0);
    level_array = array_alloc(int, 0);
    for (i = 0; i <= max_level; i++){
         array_insert (int, level_array, i, 0);
    }
    foreach_node(network, gen, np) {
        if (np->type == INTERNAL) {
            if (st_lookup(crit_table, (char *)np, &dummy) ) {
                level = (int) dummy;
                temp = array_fetch(int, level_array, level);
                temp ++;
                array_insert(int, level_array, level, temp);
            }
        }
    }

    foreach_node(network, gen, np) {
        if (np->type == INTERNAL) {
            if (speed_critical(np, speed_param) ) {
                if (speed_weight(np, speed_param, &t_weight, &a_weight) ){
		    temp_weight = ALLOC(delay_time_t, 1);
		    temp_weight->rise = t_weight;
		    temp_weight->fall = (double)a_weight;
		    (void) st_insert(weight_table, (char *)np,
			(char *)temp_weight);

		    max_time = MAX(max_time, t_weight);
		    min_time = MIN(min_time, t_weight);
		    max_area = MAX(max_area, a_weight);
		    min_area = MIN(min_area, a_weight);

		    (void) array_insert_last(node_t *, w_array, np);
		} else{
                    (void) st_insert(table, (char *)np,
			(char *)(MAXWEIGHT * 100));
		}
            }
        }
    }

    /*
     * Combine the weight values and after
     * scaling them to be in the same range 
     */
    a_range = (double)max_area - (double)min_area;
    a_range = (a_range == 0 ? 1 : a_range);
    t_range = max_time - min_time;
    t_range = (t_range == 0 ? 1 : t_range);

    for (i = 0; i < array_n(w_array); i++){
	np = array_fetch(node_t *, w_array, i);
	if (st_lookup(weight_table, (char *)np, &dummy1) ){
	    t = (delay_time_t *)dummy1;
	    t->rise = ((MAXWEIGHT * (t->rise - min_time )) / t_range) +1 ; 
	    t->fall = (MAXWEIGHT * (t->fall - min_area)) / a_range; 
/*
 * Divide the weight by the number of critical signals 
 * at that level
 */
	    (void) st_lookup(crit_table, (char *)np, &dummy) ;
	    level = (int) dummy;
	    temp = array_fetch(int, level_array, level);
	    tmp_weight = t->rise + speed_param->coeff * t->fall;
	    weight = (int)ceil((tmp_weight / (double) temp));
	    (void) st_insert(table, (char *)np, (char *)weight);
	}
    }

    if (array != NIL(array_t)) {
	for (i = 0; i < array_n(array); i++) {
	    np = array_fetch(node_t *, array, i);
	    if (st_lookup(table, (char *)np, &dummy) ){
		weight = (int)dummy;
		(void) st_lookup(crit_table, (char *)np, &dummy) ;
		level = (int) dummy;
		temp = array_fetch(int, level_array, level);
		if (st_lookup(weight_table, (char *)np, &dummy1) ){
		    t = (delay_time_t *)dummy1;
		    if (speed_param->debug){
			(void) fprintf(sisout, "%-10s Weight(t=%6.4f ,a=%6.4f)/%2d = %-6d\n",
			node_name(np), t->rise, t->fall, temp, weight);
		    }
		} else {
		   (void) fprintf(sisout, "%-10s Weight( NEEDS TO BE AVOIDED  )   = %-6d\n",
		   node_name(np), weight);
		}
	    }
	}
    }

    array_free(level_array);
    array_free(w_array);
    st_free_table(crit_table);
    st_foreach_item(weight_table, stgen, &dummy1, &dummy){
	FREE(dummy);
    }
    st_free_table(weight_table);

    return table;
}
