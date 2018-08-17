
#include "sis.h"
#include "../include/pld_int.h"

/*--------------------------------------------------------------------------
  Returns 1 if node1 is a fanin of node 2. Else returns 0.
---------------------------------------------------------------------------*/
int
is_fanin_of(node1, node2)
        node_t *node1, *node2;
{
    int    i, result;
    node_t *fanin;

    result = 0;
    if (node2->type == PRIMARY_INPUT) return 0;
    foreach_fanin(node2, i, fanin)
    {
        if (fanin == node1) {
            result = 1;
            break;
        }
    }
    return result;
}

COST_STRUCT *
act_allocate_cost_node(node, cost, arrival_time, required_time, slack, is_critical, area_weight,
                       cost_and_arrival_time, act)
        node_t *node;
        int cost, is_critical;
        double arrival_time, required_time, slack, area_weight, cost_and_arrival_time;
        ACT_VERTEX_PTR act;
{
    COST_STRUCT *cost_node;

    cost_node = ALLOC(COST_STRUCT, 1);
    cost_node->node                  = node;
    cost_node->cost                  = cost;
    cost_node->arrival_time          = arrival_time;
    cost_node->required_time         = required_time;
    cost_node->slack                 = slack;
    cost_node->is_critical           = is_critical;
    cost_node->area_weight           = area_weight;
    cost_node->cost_and_arrival_time = cost_and_arrival_time;
    cost_node->act                   = act;

    return cost_node;
}


act_final_check_network(network, cost_table
)
network_t *network;
st_table  *cost_table;
{
lsGen    gen;
node_t   *node;
st_table *table;

enum st_retval act_free_check_table();

table = st_init_table(st_ptrcmp, st_ptrhash);
foreach_node(network, gen, node
) {
act_final_check_node(node, cost_table, table
);
}
st_foreach(table, act_free_check_table, NIL(char)
);
st_free_table(table);
}

act_final_check_node(node, cost_table, table
)
node_t   *node;
st_table *cost_table, *table;
{
COST_STRUCT *cost_node;

if (node->type != INTERNAL) return;

assert (st_lookup(cost_table, node_long_name(node),(char **)

&cost_node));
if (ACT_DEBUG) (void) printf("traversing node %s, cost = %d, pattern_num = %d\n",
node_long_name(node), cost_node
->cost, cost_node->act->pattern_num);
my_traverse_act(cost_node
->act);
act_check(cost_node
->act);
delete_act(cost_node
->act, table);
/* act_traverse_not_initialize_act(cost_node->act);
   my_traverse_act(cost_node->act); */
}

/* ARGSUSED */
enum st_retval
act_free_check_table(key, value, arg)
        char *key, *value, *arg;
{
    return ST_DELETE;

}


ACT_VERTEX_PTR
act_allocate_act_vertex(mark_value)
        int mark_value;
{
    ACT_VERTEX_PTR act;

    act = ALLOC(ACT_VERTEX, 1);
    act->name = NIL(
    char);
    act->id          = 0;
    act->mark        = mark_value;
    act->multiple_fo = 0;
/*  act->multiple_fo_for_mapping = 0; */
    act->cost        = 0;
    act->mapped      = 0;
    act->low         = NIL(ACT_VERTEX);
    act->high        = NIL(ACT_VERTEX);
    return act;
}


double my_max(a, b)
        double a, b;
{
    if (a >= b) return a;
    return b;
}

act_check(vertex)
ACT_VERTEX_PTR vertex;

{
st_table *table;

enum st_retval act_free_check_table();

table = st_init_table(st_ptrcmp, st_ptrhash);
act_check_vertex(vertex, table
);
st_foreach(table, act_free_check_table, NIL(char)
);
st_free_table(table);
}

/*------------------------------------------------------------
   Performs a few checks on the vertices of the act (bdd).
-------------------------------------------------------------*/
act_check_vertex(vertex, table
)
ACT_VERTEX_PTR vertex;
st_table       *table;
{
if (
st_is_member(table,
(char *) vertex)) return;
assert(!
st_insert(table,
(char *) vertex, (char *) vertex));
/* if (ACT_DEBUG)
   (void) printf (" checking vertex %s, value = %d\n", vertex->name, vertex->value); */
assert (vertex
->value == 0 || vertex->value == 1 || vertex->value == 4);
if ((vertex->value != 4) && (vertex->low !=
NIL (ACT_VERTEX)
)) fail(" vertex low not nil");
if ((vertex->value != 4) && (vertex->high !=
NIL (ACT_VERTEX)
)) fail(" vertex high not nil");
if ((vertex->value == 4) && (vertex->low ==
NIL (ACT_VERTEX)
)) fail(" vertex low nil");
if ((vertex->value == 4) && (vertex->high ==
NIL (ACT_VERTEX)
)) fail(" vertex high nil");
if (vertex->value == 4) {
act_check_vertex(vertex
->low, table);
act_check_vertex(vertex
->high, table);
}

}

/*-----------------------------------------------------
  Changes possibly the low or high child (depending on 
  the flag) of the vertex. Decrements the multiple_fo_
  for_mapping flag if possible. This flag is used to 
  distinguish originally multiple_fo vertices. 
  SIDE_EFFECT: May make the  newchild and fanin
  of the node correspond to each other. newchild
  would have to have the same name as the fanin. 
  Stores the info in the global array 
  vertex_name_array.
------------------------------------------------------*/
act_change_vertex_child(vertex, child, flag, node
)
ACT_VERTEX_PTR vertex, child;
int            flag;
node_t         *node;
{
node_t         *fanin;
VERTEX_NODE    *vertex_name_struct;
ACT_VERTEX_PTR newchild;

/* if the child is a 0 or 1 vertex, check if child is a multiple
   fo vertex. If it is not, it is OK. Else, decrease the
   multiple_fo field of child and then make a new vertex
   "newchild", and make it the appropriate child (low or high)
   of the vertex.						*/
/*------------------------------------------------------------*/
if (child->value == 0) {
if (node !=
NIL (node_t)
)

assert (node_num_fanin(node)

== 0);
if (child->multiple_fo_for_mapping == 0) return;
(child->multiple_fo_for_mapping)--;
newchild = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
newchild->
value = 0;
if (flag == LOW) vertex->
low = newchild;
else vertex->
high = newchild;
return;
}
if (child->value == 1) {
if (node !=
NIL (node_t)
)

assert (node_num_fanin(node)

== 0);
if (child->multiple_fo_for_mapping == 0) return;
(child->multiple_fo_for_mapping)--;
newchild = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
newchild->
value = 1;
if (flag == LOW) vertex->
low = newchild;
else vertex->
high = newchild;
return;
}
if (child->low->value == 0 && child->high->value == 1) {
/* this part is to handle cases when there may be more
   than one such vertices (i.e. like child) and then
   there may be more than one parent pointing to the
   terminal vertices 0, 1			     */
/*---------------------------------------------------*/
if (child->multiple_fo_for_mapping == 0) {
if (child->low->multiple_fo_for_mapping != 0) {
(child->low->multiple_fo_for_mapping)--;
newchild = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
newchild->
value = 0;
child->
low = newchild;
}
if (child->high->multiple_fo_for_mapping != 0) {
(child->high->multiple_fo_for_mapping)--;
newchild = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
newchild->
value = 1;
child->
high = newchild;
}
return;
}
(child->multiple_fo_for_mapping)--;
newchild = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
newchild->
value = 4;
newchild->
name = child->name; /* right? */
if (flag == LOW) vertex->
low = newchild;
else vertex->
high = newchild;
act_add_terminal_vertices(newchild);
return;
}
/* the child is non-trivial vertex (i.e. root of another tree)
   have to create a new vertex for it everytime.		 */
/*-------------------------------------------------------------*/
if (child->multiple_fo_for_mapping != 0) (child->multiple_fo_for_mapping)--;
newchild = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
newchild->
value = 4;
if (flag == LOW) vertex->
low = newchild;
else vertex->
high = newchild;
/* make the low and high children of newchild 0 and 1 terminal vertices. */
/*-----------------------------------------------------------------------*/
act_add_terminal_vertices(newchild);

/* newchild corresponds to the fanin of node. Store that info */
/*------------------------------------------------------------*/
assert (node_num_fanin(node)

== 1);
fanin              = node_get_fanin(node, 0); /* node is a node_literal */
vertex_name_struct = act_allocate_vertex_node_struct(newchild, fanin);
array_insert_last(VERTEX_NODE
*, vertex_name_array, vertex_name_struct);
}


VERTEX_NODE *
act_allocate_vertex_node_struct(vertex, node)
        ACT_VERTEX_PTR vertex;
        node_t *node;
{
    VERTEX_NODE * vertex_node_struct;

    vertex_node_struct = ALLOC(VERTEX_NODE, 1);
    vertex_node_struct->vertex = vertex;
    vertex_node_struct->node   = node;
    return vertex_node_struct;
}

/*----------------------------------------------------------
   Creates 0 and 1 terminal vertices and makes them 
   low and high children of the vertex.
-----------------------------------------------------------*/
act_add_terminal_vertices(vertex)
ACT_VERTEX_PTR vertex;
{
ACT_VERTEX_PTR low, high;

low = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
low->
value = 0;
high  = act_allocate_act_vertex(MARK_COMPLEMENT_VALUE);
high->
value = 1;
vertex->
low = low;
vertex->
high = high;
}


/*--------------------------------------------------------------------------------
  Puts the correct node names in the act vertices after partial collapsing of the 
  node. Also updates fields of the cost_struct. Note that cost_struct corresponds
  to the node of the network.
----------------------------------------------------------------------------------*/
act_partial_collapse_update_act_fields(network, dup_fanout, cost_struct
)
network_t   *network;
node_t      *dup_fanout;
COST_STRUCT *cost_struct;

{
node_t *fanout;

fanout = network_find_node(network, node_long_name(dup_fanout));
cost_struct->
node = fanout;
cost_struct->act->
node = fanout;
act_partial_collapse_put_node_names_in_act(cost_struct
->act, network);
}

/*------------------------------------------------------------------------
  Puts correct node names in all the vertices of the act rooted at 
  vertex. It does this by finding the node in the network with the same 
  name as vertex->name and pointing the name field at the name of this node.
  This is correct since the dup_net has the same node names as original
  network.
-------------------------------------------------------------------------*/
act_partial_collapse_put_node_names_in_act(vertex, network
)
ACT_VERTEX_PTR vertex;
network_t      *network;
{
node_t *node;

if (vertex->mark == 0) vertex->
mark = 1;
else vertex->
mark = 0;

if (vertex->value != 4) return;

node = network_find_node(network, vertex->name);
vertex->
name = node_long_name(node);

if (vertex->mark != vertex->low->mark)
act_partial_collapse_put_node_names_in_act(vertex
->low, network);
if (vertex->mark != vertex->high->mark)
act_partial_collapse_put_node_names_in_act(vertex
->high, network);
}


/*--------------------------------------------------------------------------------
  Puts the correct node names in the act vertices after the remapping of the node.
  Also updates fields of the cost_struct. Note that cost_struct corresponds to the 
  node of the network.
----------------------------------------------------------------------------------*/
act_remap_update_act_fields(table1, network1, node, cost_struct
)
st_table    *table1;
network_t   *network1;
node_t      *node;
COST_STRUCT *cost_struct;

{
cost_struct->
node = node;
cost_struct->act->
node = node;
act_remap_put_node_names_in_act(cost_struct
->act, table1, network1);
}

/*------------------------------------------------------------------------
  Puts correct node names in all the vertices of the act rooted at 
  vertex.
-------------------------------------------------------------------------*/
act_remap_put_node_names_in_act(vertex, table1, network1
)
ACT_VERTEX_PTR vertex;
st_table       *table1;
network_t      *network1;
{
node_t *node1, *node;

if (vertex->mark == 0) vertex->
mark = 1;
else vertex->
mark = 0;

if (vertex->value != 4) return;

node1 = network_find_node(network1, vertex->name);

assert (st_lookup(table1,(char *)

node1, (char **) &node));
vertex->
name = node_long_name(node);

if (vertex->mark != vertex->low->mark)
act_remap_put_node_names_in_act(vertex
->low, table1, network1);
if (vertex->mark != vertex->high->mark)
act_remap_put_node_names_in_act(vertex
->high, table1, network1);
}

/****************************************************************************
  puts all the fanins of the node "node" in the order_list, 
  except for the node "n".
*****************************************************************************/

void
act_put_nodes(node, n, order_list)
        node_t *node;
        node_t *n;
        array_t *order_list;
{
    int num_fanin;
    node_t * fanin;
    int i;
    int check;  /* for debugging */

    check     = 0;
    num_fanin = node_num_fanin(node);
    for (i    = num_fanin - 1; i >= 0; i--) {
        fanin = node_get_fanin(node, i);
        if (fanin == n) {
            check = 1;
            continue;
        }
        array_insert_last(node_t * , order_list, fanin);
    }

    /* for debugging */
    /*---------------*/
    if (check == 0) {
        (void) printf("put_order: node %s does not have the required fanin\n",
                      node_long_name(node));
        exit(1);
    }
}

/**********************************************************************
   recursively initialize the required structure of each act vertex.
   also finds out multiple fan out vertices and stores them in the 
   multiple_fo_array.  
************************************************************************/
void
act_initialize_act_area(vertex, multiple_fo_array)
        ACT_VERTEX_PTR vertex;
        array_t *multiple_fo_array;
{
    /* to save repetition, mark the vertex as visited */
    /*------------------------------------------------*/
    if (vertex->mark == 0) vertex->mark = 1;
    else vertex->mark = 0;

    vertex->pattern_num             = -1;
    vertex->cost                    = 0;
    vertex->arrival_time            = (double) 0.0;
    vertex->mapped                  = 0;
    vertex->multiple_fo             = 0;
    vertex->multiple_fo_for_mapping = 0;

    /* do not care for a terminal vertex- nothing is to be done */
    /*----------------------------------------------------------*/
    if (vertex->value != 4) {
        vertex->cost         = 0;
        vertex->arrival_time = (double) (0.0);
        return;
    }

    /* if the left(right) son not visited earlier, initialize it, else
       mark it as a multiple fo vertex */
    /*---------------------------------------------------------------*/

    if (vertex->mark != vertex->low->mark)
        act_initialize_act_area(vertex->low, multiple_fo_array);
    else {
        /* important to check that it is a multiple_fo vertex, else you may be
       adding  a vertex too many times in the multiple_fo array.*/
        /*--------------------------------------------------------------------*/
        if (vertex->low->multiple_fo == 0) {
            array_insert_last(ACT_VERTEX_PTR, multiple_fo_array, vertex->low);
        }
        vertex->low->multiple_fo += 1;
        vertex->low->multiple_fo_for_mapping += 1;
    }

    if (vertex->mark != vertex->high->mark)
        act_initialize_act_area(vertex->high, multiple_fo_array);
    else {
        if (vertex->high->multiple_fo == 0) {
            array_insert_last(ACT_VERTEX_PTR, multiple_fo_array, vertex->high);
        }
        vertex->high->multiple_fo += 1;
        vertex->high->multiple_fo_for_mapping += 1;

    }

}


/* for the purpose of debugging with it first */

void
act_initialize_act_delay(vertex)
        ACT_VERTEX_PTR vertex;
{
    /* to save repetition, mark the vertex as visited */
    /*------------------------------------------------*/
    if (vertex->mark == 0) vertex->mark = 1;
    else vertex->mark = 0;

    vertex->pattern_num             = -1;
    vertex->cost                    = 0;
    vertex->arrival_time            = (double) 0.0; /* so that multiple_fo vertices are taken care of */
    vertex->mapped                  = 0;
    vertex->multiple_fo             = 0;
    vertex->multiple_fo_for_mapping = 0;

    /* do not care for a terminal vertex- nothing is to be done */
    /*----------------------------------------------------------*/
    if (vertex->value != 4) {
        return;
    }
    /* if the left(right) son not visited earlier, initialize it, else
       mark it as a multiple fo vertex */
    /*---------------------------------------------------------------*/

    if (vertex->mark != vertex->low->mark) act_initialize_act_delay(vertex->low);
    else {
        vertex->low->multiple_fo += 1;
        vertex->low->multiple_fo_for_mapping += 1;
    }

    if (vertex->mark != vertex->high->mark) act_initialize_act_delay(vertex->high);
    else {
        vertex->high->multiple_fo += 1;
        vertex->high->multiple_fo_for_mapping += 1;
    }

}

make_multiple_fo_array_delay(vertex, multiple_fo_array
)
ACT_VERTEX_PTR vertex;
array_t        *multiple_fo_array;
{

/* to save repetition, mark the vertex as visited */
/*------------------------------------------------*/
if (vertex->mark == 0) vertex->
mark = 1;
else vertex->
mark = 0;

/* do not care for a terminal vertex- nothing is to be done */
/*----------------------------------------------------------*/
if (vertex->value != 4) return;

if (vertex->multiple_fo != 0)
my_array_insert_unique(vertex, multiple_fo_array
);
if (vertex->mark != vertex->low->mark)
make_multiple_fo_array_delay(vertex
->low, multiple_fo_array);
if (vertex->mark != vertex->high->mark)
make_multiple_fo_array_delay(vertex
->high, multiple_fo_array);
}


/*****************************************************************************
  if vertex is already there in the multiple_fo_array, returns 0.
  Else inserts it in the end of the array and returns 1.
******************************************************************************/
my_array_insert_unique(vertex, multiple_fo_array
)
ACT_VERTEX_PTR vertex;
array_t        *multiple_fo_array;
{
int            i, nsize;
ACT_VERTEX_PTR v;

nsize = array_n(multiple_fo_array);
for (
i = 0;
i<nsize;
i++) {
v = array_fetch(ACT_VERTEX_PTR, multiple_fo_array, i);
if (v == vertex) return 0;
}
array_insert_last(ACT_VERTEX_PTR, multiple_fo_array, vertex
);
return 1;
}


/******************************************************************************************
  Just prints out the BDD (OBDD) starting at the vertex. It first visits the low child and 
  then the right child. Marks all the nodes that are visited so that in future, it does not
  traverse an already traversed (marked) node. 
******************************************************************************************/
void my_traverse_act(vertex)
        ACT_VERTEX_PTR vertex;
{

    if (vertex->mark == 0) vertex->mark = 1;
    else vertex->mark = 0;
    print_vertex(vertex);

    if (vertex->value != 4) return;

    if (vertex->mark == vertex->low->mark) {
        print_vertex(vertex->low);
        if (vertex->mark == vertex->high->mark) {
            print_vertex(vertex->high);
            return;
        }
        my_traverse_act(vertex->high);
        return;
    }
    my_traverse_act(vertex->low);
    if (vertex->mark == vertex->high->mark) {
        print_vertex(vertex->high);
        return;
    }
    my_traverse_act(vertex->high);
    return;

}

static
act_traverse_not_initialize_act(vertex)
ACT_VERTEX_PTR vertex;
{
/* to save repetition, mark the vertex as visited */
/*------------------------------------------------*/
if (vertex->mark == 0) vertex->
mark = 1;
else vertex->
mark = 0;

/* do not care for a terminal vertex- nothing is to be done */
/*----------------------------------------------------------*/
if (vertex->value != 4) {
return;
}

/* if the left(right) son not visited earlier, initialize it, else
   mark it as a multiple fo vertex */
/*---------------------------------------------------------------*/

if (vertex->mark != vertex->low->mark)
act_traverse_not_initialize_act(vertex
->low);
else {
/* important to check that it is a multiple_fo vertex, else you may be
adding  a vertex too many times in the multiple_fo array.*/
/*--------------------------------------------------------------------*/
vertex->low->multiple_fo += 1;
vertex->low->multiple_fo_for_mapping += 1;
}

if (vertex->mark != vertex->high->mark)
act_traverse_not_initialize_act(vertex
->high);
else {
vertex->high->multiple_fo += 1;
vertex->high->multiple_fo_for_mapping += 1;

}

}

/*****************************************************************************************
 Prints information on a vertex of the ACT (OACT) 
*****************************************************************************************/
void
print_vertex(vertex)
        ACT_VERTEX_PTR vertex;
{

    /* terminal vertex */
    /*-----------------*/
    if (vertex->value != 4) {
        (void) printf("value = %d, id = %d, index = %d, multiple_fo_for_mapping = %d\n", vertex->value,
                      vertex->id, vertex->index, vertex->multiple_fo_for_mapping);
        (void) printf("value = %d, id = %d, index = %d\n", vertex->value,
                      vertex->id, vertex->index);
        return;
    }

    /* nonterminal vertex */
    /*--------------------*/
    if (vertex->name != NIL(char))
    (void) printf("name = %s, index = %d, num_fanouts = %d, multiple_fo_for_mapping = %d\n",
                  vertex->name, vertex->index, vertex->multiple_fo + 1, vertex->multiple_fo_for_mapping);
    else
    (void) printf("id = %d, index = %d, num_fanouts = %d, multiple_fo_for_mapping = %d\n",
                  vertex->id, vertex->index, vertex->multiple_fo + 1, vertex->multiple_fo_for_mapping);

    /*     (void) printf ("name = %s, index = %d, num_fanouts = %d\n",
        vertex->name, vertex->index, vertex->multiple_fo + 1);
    else
         (void) printf ("id = %d, index = %d, num_fanouts = %d\n",
        vertex->id, vertex->index, vertex->multiple_fo + 1); */

}






