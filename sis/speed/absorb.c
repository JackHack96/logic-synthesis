/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/speed/absorb.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:50 $
 *
 */
#include "sis.h"
#include "speed_int.h"

/*
 * Collapse the critical nodes in the transitive anin of "f" for a distance
 * "dist" 
 */
void
speed_absorb(f, speed_param)
node_t *f;
speed_global_t *speed_param;
{
    st_table *table; 	/* Temp reference */
    array_t *temp_array;   /* Temp array for bfs implementation */
    node_t *temp, *fi;
    int i, j, k;
    int dist = speed_param->dist;
    int more_to_come, first = 0, last;

    if (f->type != INTERNAL) {
	(void) fprintf(sisout,"Can only absorb internal nodes \n");
	return;
    }

    table =  st_init_table(st_ptrcmp, st_ptrhash);
    temp_array = array_alloc(node_t *, 0);
    array_insert_last(node_t *, temp_array, f); 
    (void) st_insert(table, (char *)f, (char *)(dist+1));

    more_to_come = TRUE;
    for (i = dist; (i > 0) && more_to_come ; i--){
	more_to_come = FALSE;
	last = array_n(temp_array);
	for (j = first; j < last; j++){
	    temp = array_fetch(node_t *, temp_array, j);
	    foreach_fanin(temp, k, fi) {
		if (node_function(fi) == NODE_PI){
		    /* 
		     * Will be fanin to the collapsed node
		     */
		    (void) st_insert(table, (char *)fi, (char *)0);
		} else if (!st_is_member(table, (char *)fi)){
		/* Not yet visited */
		    if (speed_critical(fi, speed_param) ){
		    /*
		     * Add to elements that will be processed 
		     */
			(void) st_insert(table, (char *)fi, (char *)i);
			array_insert_last(node_t *, temp_array, fi);
			more_to_come = TRUE;
		    }
		}
	    }
	}
	first = last;
    }

    /* 
     * collapse all the nodes in temp_array into it 
     */
    speed_absorb_array(f, speed_param, temp_array, table);

    array_free(temp_array);
    st_free_table(table);
    return;
}

void
speed_absorb_array(f, speed_param, temp_array, cache)
node_t *f;
speed_global_t *speed_param;
array_t *temp_array;	/* Nodes to be collapsed into  node "f" */
st_table *cache;	/* Hash table of the nodes in the array -- may be NIL*/
{

    st_table *del_table;
    st_table *table; 	   /* Temp reference */
    array_t *del_array;	   /* Keeps the nodes that are to be deleted */
    node_t *temp, *fi;
    char *dummy;
    node_t *no;
    int i, j;
    int more_to_come, first = 0, last;

    if (cache == NIL(st_table)){
	table = st_init_table(st_ptrcmp, st_ptrhash);
	for (i = 0; i < array_n(temp_array); i++){
	    no = array_fetch(node_t *, temp_array, i);
	    (void)st_insert(table, (char *)no, "");
	}
    } else {
	table = cache;
    }

    /*
     * nodes in the original network that need to be deleted 
     */
    del_table =  st_init_table(st_ptrcmp, st_ptrhash);
    del_array = array_alloc(node_t *, 0);
    foreach_fanin(f, i, no) {
	array_insert_last(node_t *, del_array, no);
    }

    if (speed_param->debug) (void)fprintf(sisout, "Collapsing nodes \n");
    more_to_come = TRUE;
    while(more_to_come) {
	more_to_come = FALSE;
	for (i = 1; i < array_n(temp_array); i++){
	    temp = array_fetch(node_t *, temp_array, i);
	    
	    if (speed_param->debug && node_num_fanin(temp) > 2){
	        (void)fprintf(sisout, "WARN: Collapsing at node %s : %s with mo re than 2 ip\n",
	             node_name(f), node_name(temp));
	    }
	    if (node_collapse(f, temp)) {
		if (speed_param->debug) {
		    node_print(sisout, temp);
		}
	    }
	}
	foreach_fanin(f, i, temp) {
	    if (temp->type != PRIMARY_INPUT &&
		st_lookup(table, (char *)temp, &dummy)){
		more_to_come = TRUE;
	    }
	}
    }
    /*
     * Simplify the node to get rid of the extra literals 
     */

    temp = node_simplify(f, NIL(node_t), NODE_SIM_SIMPCOMP);
    node_replace(f, temp);

    /* Now remove the nodes that don't fanout anywhere */
    more_to_come = TRUE;
    first = 0;
    while (more_to_come) {
	more_to_come = FALSE;
	last = array_n(del_array);
	for (i = first; i < last; i++) {
	    fi = array_fetch(node_t *, del_array, i);
	    if (node_num_fanout(fi) == 0 ) {
		foreach_fanin(fi, j, temp) {
		    if (! st_insert(del_table, (char *)temp, NIL(char)) ){
			array_insert_last (node_t *, del_array, temp);
			more_to_come = TRUE;
		    }
		}
		(void) st_delete(table, (char **)&fi, &dummy);
		network_delete_node(f->network, fi);
	    }
	    (void) st_delete(del_table, (char **)&fi, &dummy);
	}
	first = last;
    }

    /*
     * At this stage the entries in "table" is the set of
     * nodes that will be retained in the original network
     */

    if (cache == NIL(st_table)) st_free_table(table);
    array_free(del_array);
    st_free_table(del_table);

    return;
}
