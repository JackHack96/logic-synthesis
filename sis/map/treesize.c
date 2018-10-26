
/* file @(#)treesize.c	1.2 */
/* last modified on 5/1/91 at 15:52:02 */
#include "sis.h"
#include "map_int.h"


typedef struct count_struct count_t;
struct count_struct {
    int leafs;		/* counter */
    int nodes;		/* counter */
    int freq;		/* frequency of this count */
    node_t *root_node;
    count_t *next;
};


static int
compare_count(x, y)
count_t *x, *y;
{
    return y->nodes - x->nodes;
}


/* sort_count_list(count_t *list, int compare(count_t *, count_t *)) */
#define TYPE count_t
#define SORT sort_count_list_helper
#define SORT1 sort_count_list
#include "lsort.h"


/* uniq_count_list(count_t *list, int compare(count_t *, count_t *), free) */
#define TYPE count_t
#define UNIQ uniq_count_list
#define FREQ freq
#include "luniq.h"


static void
find_tree_size(node, leaf_table, visited, tree_root)
node_t *node;
st_table *leaf_table;
st_table *visited;
node_t *tree_root;
{
    int i;
    count_t *count;
    node_t *fanin;

    if (node->type == PRIMARY_INPUT || node_num_fanout(node) > 1) {
	/* advance the leaf counter for 'tree_root' */
	if (st_lookup(leaf_table, (char *) tree_root, (char **) &count)) {
	    count->leafs++;
	} else {
	    count = ALLOC(count_t, 1);
	    count->nodes = 0;
	    count->leafs = 1;
	    count->root_node = tree_root;
	    (void) st_insert(leaf_table, (char *) tree_root, (char *) count);
	}

	tree_root = node;	/* start a new tree here */
    } 

    if (! st_lookup(visited, (char *) node, NIL(char *))) {
	(void) st_insert(visited, (char *) node, NIL(char));

	/* advance the node counter for 'tree_root' */
	if (st_lookup(leaf_table, (char *) tree_root, (char **) &count)) {
	    count->nodes++;
	} else {
	    count = ALLOC(count_t, 1);
	    count->nodes = 1;
	    count->leafs = 0;
	    count->root_node = tree_root;
	    (void) st_insert(leaf_table, (char *) tree_root, (char *) count);
	}

	foreach_fanin(node, i, fanin) {
	    find_tree_size(fanin, leaf_table, visited, tree_root);
	}
    }
}


void
map_print_tree_size(fp, network)
FILE *fp;
network_t *network;
{
#ifdef _IBMR2
extern void free();
#endif
  count_t *count, *count_list, *count_next;
  node_t *node, *fanin;
  st_table *leaf_table, *visited;
  st_generator *stgen;
  lsGen gen;

  leaf_table = st_init_table(st_ptrcmp, st_ptrhash);

  visited = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_primary_output(network, gen, node) {
    fanin = node_get_fanin(node, 0);
    find_tree_size(fanin, leaf_table, visited, node);
  }
  st_free_table(visited);

  count_list = NIL(count_t);
  st_foreach_item(leaf_table, stgen, (char **) &node, (char **) &count) {
    count->next = count_list;
    count_list = count;
    /*(void) printf("%-10s : %d\n", node->name, count->nodes);*/
  }
  st_free_table(leaf_table);

  count_list = sort_count_list(count_list, compare_count);
  count_list = uniq_count_list(count_list, compare_count, free);

  (void) fprintf(fp, "Distribution of Tree Sizes\n");
  (void) fprintf(fp, "--------------------------\n");
  for(count = count_list; count != NIL(count_t); count = count_next) {
    count_next = count->next;
    (void) fprintf(fp,
		   "    nodes=%3d   leaf=%3d  freq=%3d  (root node = %s)\n", 
		   count->nodes + count->leafs, count->leafs, 
		   count->freq, count->root_node->name);
    FREE(count);
  }
}
