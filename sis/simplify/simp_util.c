#include "sis.h"
#include "simp_int.h"

void copy_dcnetwork();
node_t *find_node_exdc();
void free_dcnetwork_copy();
array_t *order_nodes_elim();
int num_cube_cmp();
static int level_elim_cmp();
int fsize_cmp();

typedef struct node_fsize_t_defn {
    node_t *node;
    int fsize;
} node_fsize_t;


void copy_dcnetwork(network)
network_t *network;
{ 
    node_t *node, *dcnode, *dcfanin;
    node_t *n1, *n2, *n3, *n4;
    node_t *tempnode;
    int i,j,k;
    pset last, setp;
    network_t    *DC_network;
    array_t *dc_list;
  
    if(network == NULL) {
        return;
    }
    DC_network = network_dc_network(network);
    if(DC_network == NULL) {
        return;
    }
    dc_list= network_dfs(DC_network);
    for(i=0 ; i< array_n(dc_list); i++){
        dcnode = array_fetch(node_t *, dc_list, i);
        (void) cspf_alloc(dcnode);
        if (node_function(dcnode) == NODE_PI){
            for(j=0 ; j< network_num_pi(network); j++){
                node= network_get_pi(network, j);
                if (strcmp(dcnode->name, node->name) == 0){
                    CSPF(dcnode)->node = node;
                    break;
                }
            }
            continue;
        }
        if (node_function(dcnode) == NODE_PO){
            dcfanin= node_get_fanin(dcnode, 0);
            CSPF(dcnode)->node= CSPF(dcfanin)->node;
            continue;
        }

        n3= node_constant(0);
        foreach_set(dcnode->F, last, setp) {
            n1= node_constant(1);
            for(k=0 ; k< dcnode->nin; k++){
            if ((j= GETINPUT(setp,k))!= TWO){
            if (j== ONE){
                dcfanin= node_get_fanin(dcnode,k);
                tempnode= CSPF(dcfanin)->node;
                n4= node_literal(tempnode,1);
                n2= node_and(n1, n4);
                node_free(n4);
                node_free(n1);
                n1= n2;
            }else{
                dcfanin= node_get_fanin(dcnode,k);
                tempnode= CSPF(dcfanin)->node;
                n4= node_literal(tempnode,0);
                n2= node_and(n1, n4);
                node_free(n4);
                node_free(n1);
                n1= n2;
            }
            }
            }
            n2= node_or(n1,n3);
            node_free(n1);
            node_free(n3);
            n3= n2;
        }
        CSPF(dcnode)->node = n3;
    }
    array_free(dc_list);
    return;
}

node_t *find_node_exdc(ponode, node_exdc_table)
node_t *ponode; /*primary output of the care network*/
st_table *node_exdc_table;

{
    node_t *dcponode;
    char *dummy;

/* find the corresponding PO in the don't care network from the hash table*/
    if (node_exdc_table == NIL(st_table)) {
        return(node_constant(0));
    }
    if(!st_lookup(node_exdc_table, (char *) ponode, &dummy))
        return(node_constant(0));
    dcponode= (node_t *) dummy;
	if (node_function(CSPF(dcponode)->node) == NODE_PI){
		return(node_literal(CSPF(dcponode)->node, 1));
	}
    return(node_dup(CSPF(dcponode)->node));
}

void free_dcnetwork_copy(network)
network_t *network;
{ 
    node_t *dcnode;
    int i;
    network_t    *DC_network;
    array_t *dc_list;
  
    if(network == NULL) {
        return;
    }
    DC_network = network_dc_network(network);
    if(DC_network == NULL) {
        return;
    }

    dc_list= network_dfs(DC_network);
    for(i=0 ; i< array_n(dc_list); i++){
        dcnode = array_fetch(node_t *, dc_list, i);
        if (node_function(dcnode) == NODE_PO){
            (void) cspf_free(dcnode);
            continue;
        }
        if (node_function(dcnode) == NODE_PI){
            cspf_free(dcnode);
            continue;
        }
        ntbdd_free_at_node(CSPF(dcnode)->node);
        node_free(CSPF(dcnode)->node);
        cspf_free(dcnode);
    }
    array_free(dc_list);
}

array_t *order_nodes_elim(network)
network_t *network;
{
    array_t *odc_order_list, *network_node_list;
    node_t *node;
    int array_size;
    int i; 

    network_node_list= network_dfs_from_input(network);
    array_size= array_n(network_node_list);
    odc_order_list= array_alloc(node_t *, 0);
    for(i= 0; i< array_size; i++){
      node= array_fetch(node_t *, network_node_list, i);
      odc_alloc(node);
      ODC(node)->value= odc_value(node);
      ODC(node)->order= i;
      if (node_function(node) == NODE_PO)
        continue;
      array_insert_last(node_t *, odc_order_list, node);
    }
    (void) find_odc_level(network);
    array_sort(odc_order_list, level_elim_cmp);
    for(i= 0 ; i < array_n(network_node_list) ; i++){
      node= array_fetch(node_t *, network_node_list, i);
      odc_free(node);
    }
	array_free(network_node_list);
    return(odc_order_list);
}

static int level_elim_cmp(obj1, obj2)
char **obj1;
char **obj2;
{
    node_t *node1, *node2;

    node1= *((node_t **)obj1);
    node2= *((node_t **)obj2);

    if (ODC(node1)->level < ODC(node2)->level)
        return(-1);
    if (ODC(node1)->level > ODC(node2)->level)
        return(1);

    if (ODC(node1)->order > ODC(node2)->order)
        return(-1);
    if (ODC(node1)->order < ODC(node2)->order)
        return(1);
    return(0);
}


int num_cube_cmp(obj1, obj2)
char **obj1;
char **obj2;
{
    node_t *node1, *node2;

    node1= *((node_t **)obj1);
    node2= *((node_t **)obj2);

    if (node_num_cube(node1) < node_num_cube(node2))
        return(-1);
    if (node_num_cube(node1) > node_num_cube(node2))
        return(1);
    return(0);
}

array_t *
simp_order(nodevec)
array_t *nodevec;
{
    node_t *cn;
    node_fsize_t *new_entry, *this_entry;
    array_t *list;
    int i, fac_size;

    list = array_alloc(node_fsize_t *, 0);
    for (i = 0; i < array_n(nodevec); i ++) {
	cn = array_fetch(node_t *, nodevec, i);
	fac_size = factor_num_literal(cn);
	new_entry = ALLOC(node_fsize_t, 1);
	new_entry->node = cn;
	new_entry->fsize = fac_size;
	array_insert(node_fsize_t *, list, i, new_entry);
    }

    array_sort(list, fsize_cmp);
    for (i = 0; i < array_n(list); i ++) {
	this_entry = array_fetch(node_fsize_t *, list, i);
	array_insert(node_t *, nodevec, i, this_entry->node);
	FREE(this_entry);
    }

    array_free(list);
    return(nodevec);
}

int fsize_cmp(obj1, obj2)
char **obj1;
char **obj2;
{
    node_fsize_t *fsize_struct1, *fsize_struct2;

    fsize_struct1 = *((node_fsize_t **) obj1);
    fsize_struct2 = *((node_fsize_t **) obj2);

    if (fsize_struct1->fsize < fsize_struct2->fsize)
	return(1);
    else if (fsize_struct1->fsize > fsize_struct2->fsize) 
	return(-1);
    else 
	return(0);
}
