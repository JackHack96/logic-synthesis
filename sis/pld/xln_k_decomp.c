/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_k_decomp.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:56 $
 *
 */
/* changed Feb 20, 1991 - - to accomodate delay routine, made these changes.
   had to change some other stuff, like init_intersection was written,... */

#include "sis.h"
#include "pld_int.h"
#include <math.h>

struct my_cube {
   int *variable; /* maximum cardinality of LAMBDA-set */
 };

static node_t **ALPHA;  /* for storing ALPHA functions */
static node_t *G_node; /* used for finding G_node */

static array_t *karp_decomp_internal();   /* added Feb 25, 1991 */
network_t *xln_k_decomp_node_with_network(); /* added Feb 25, 1991 */
array_t *xln_k_decomp_node_with_array(); /* added Feb 25, 1991 */
extern int split_node(); /* Narendra's routine added Feb 25, 1991 */

static void karp_decomp_node_internal();
static void form_incompatibility_graph();
static void form_u_intersection();
static void get_lambda_cube();
static int val_literal();
static int** init_incompatible();
static int** init_intersection();
static void alloc_my_cube();
static void end_incompatible();
static void end_intersection();
static void free_my_cube();
static void make_incompatible();
static void generate_all_minterms();
static void generate_minterm_with_index();
static void tree_init();
static void form_compatibility_classes();
static void mark_roots_of_trees();
static void get_combination();
static void mark_all_inputs();
static int is_fanin_in_lambda();
static int marked();
static int find_alphas_and_G(); /* returns number of alpha nodes */
static node_t *product_node_for_minterm();
static node_t *find_G();
static void free_tree();
static enum st_retval k_decomp_free_table_entry();
static void replace_node_decomposed();

void
karp_decomp_network(network, SUPPORT, one_node, user_node)
  network_t *network;
  int SUPPORT;
  int one_node;
  node_t *user_node;
{

  node_t *node, *node_simpl;
  array_t *nodevec;
  int i;
  int num_nodes;
  int **intersection;
  int *lambda_indices;
  int NUM_NODES;

  /* NUM_NODES = (int)(pow((double)2, (double)SUPPORT)); */
  NUM_NODES = 1 << SUPPORT;
  intersection = init_intersection();
  lambda_indices = ALLOC(int, SUPPORT);
  if (one_node) {   
      nodevec = array_alloc(node_t *, 0);
      array_insert_last(node_t *, nodevec, user_node);
  }
  else {
      nodevec = network_dfs_from_input(network);
  }
  num_nodes = array_n(nodevec);
  /* simplify so that while taking complement, the support remains same for
     complemented node */
  /*---------------------------------------------------------------------*/
  for (i = 0; i < num_nodes; i++) {
     node = array_fetch(node_t *, nodevec, i);
     if (node->type != INTERNAL) continue;
     node_simpl = node_simplify(node, (node_t *) NULL, NODE_SIM_ESPRESSO);
     node_replace(node, node_simpl);
  }
  for (i = 0; i < num_nodes; i++) {
     node = array_fetch(node_t *, nodevec, i);
     if (node->type != INTERNAL) continue;
     karp_decomp_node_internal(node, intersection, lambda_indices, NUM_NODES, SUPPORT);
  }
  (void) network_sweep(network);

  /* freeing */
  /*---------*/ 
  array_free(nodevec);
  end_intersection(intersection);
  FREE(lambda_indices);  
}

/*----------------------------------------------------------------------------------------
  For the node, generates a lambda set, and then calls the karp_decomp_internal routine.
  If the node cannot be decomposed, calls the split routine. Recursively decomposes till
  feasible node generated.
------------------------------------------------------------------------------------------*/
static void
karp_decomp_node_internal(node, intersection, lambda_indices, NUM_NODES, SUPPORT)
  node_t *node;
  int **intersection;
  int *lambda_indices;
  int NUM_NODES, SUPPORT;
{
  int j, num;
  node_t *new_node;
  array_t *newvec;

  while (1) {
      /* if feasible, return - note this can't be put outside "while" */
      /*--------------------------------------------------------------*/
      if (node_num_fanin (node) <= SUPPORT) return;
  
      /* decompose node using lambda indices as bound set */
      /*--------------------------------------------------*/
      get_combination(node, 0, lambda_indices, SUPPORT);         
      newvec = karp_decomp_internal(node, intersection, lambda_indices, 
                                    NUM_NODES, SUPPORT);
      /* checking if decomposition possible */
      /*------------------------------------*/
      if (newvec != NIL(array_t)) {
          num = array_n(newvec);
          for (j = 1; j < num; j++) {
              new_node = array_fetch(node_t *, newvec, j);
              network_add_node(node->network, new_node);
          }
          new_node = array_fetch(node_t *, newvec, 0);
          node_replace(node, new_node);
          array_free(newvec);
      }
      /* decomposition not possible, split the node */
      /*--------------------------------------------*/
      else {
          (void) split_node(node->network, node, SUPPORT);
          return;
      }
  }
}

/**************************************************************
  decomposes a node in the network such that the bound set
  LAMBDA has a cardinality of SUPPORT. Based on karp's method of
  decomposition. However, to speed up things, presently we do 
  not go for the best decomposition. We just go for "a" 
  decomposition. 
**************************************************************/
static array_t *
karp_decomp_internal(node, intersection, lambda_indices, NUM_NODES, SUPPORT)
node_t *node;
int **intersection;
int *lambda_indices;
int NUM_NODES, SUPPORT;

{

  node_t *node_inv;
  int i;
  st_table *root_table;
  st_table *lambda_table;
  tree_node **treenode;
  int **incompatible;  /* stores whether minterms i & j are incompatible */
  int CLASS_NUM,  NUM_ALPHAS;
  array_t *nodevec;

  incompatible = init_incompatible(NUM_NODES);

 /* to store the info. about the root of the trees. CLASS_NUM tells the 
   class_num of each tree (but is valid only at the end of mark_root */

  root_table = st_init_table(st_ptrcmp, st_ptrhash);
  lambda_table = st_init_table(st_ptrcmp, st_ptrhash); 
  CLASS_NUM = 0;
  treenode = ALLOC (tree_node *, NUM_NODES);
  tree_init(treenode, NUM_NODES);

  node_inv = node_not(node);

  form_incompatibility_graph(node, node_inv, intersection, incompatible, 
                             lambda_table, lambda_indices, NUM_NODES, SUPPORT);
  form_compatibility_classes(treenode, incompatible, NUM_NODES);           
  mark_roots_of_trees(treenode, root_table, NUM_NODES, &CLASS_NUM);
  NUM_ALPHAS = intlog2(CLASS_NUM);
  /* check when the node cannot be decomposed */
  /*------------------------------------------*/
  if (NUM_ALPHAS == SUPPORT) {
      node_free(node_inv);
      free_tree(treenode, root_table, lambda_table, NUM_NODES);
      end_incompatible(incompatible, NUM_NODES);  
      return NIL (array_t);
  }
  find_alphas_and_G(node, lambda_table, treenode, lambda_indices, 
                    NUM_NODES, SUPPORT, CLASS_NUM, NUM_ALPHAS);
  nodevec = array_alloc(node_t *, 0);
  array_insert_last(node_t *, nodevec, G_node);
  for (i = 0; i < NUM_ALPHAS; i++) {
      array_insert_last(node_t *, nodevec, ALPHA[i]);
  }
  node_free(node_inv);
  FREE(ALPHA);
  /* frees the root_table, lambda table also */
  /*-----------------------------------------*/
  free_tree(treenode, root_table, lambda_table, NUM_NODES);
  end_incompatible(incompatible, NUM_NODES);  
  return nodevec;
}
  
static void
form_incompatibility_graph(node, node_inv, intersection, incompatible, lambda_table, lambda_indices, NUM_NODES, SUPPORT)
node_t *node;
node_t *node_inv;
int **intersection,  **incompatible;
st_table *lambda_table;
int *lambda_indices;
int NUM_NODES, SUPPORT;

{
   int node_numcubes;
   int i;
   node_cube_t cube_n;

   /* if the input in is in lambda part, mark it 1, else mark it 0 */
   /* assume static (global) storage for the marking arrays */
   /*---------------------------------------------------------------*/
   mark_all_inputs(node, lambda_table, lambda_indices, SUPPORT);
   node_numcubes = node_num_cube(node);
   for (i = node_numcubes - 1; i >= 0; i--) {
       cube_n = node_get_cube(node, i);
       form_u_intersection(node, cube_n, node_inv, intersection, incompatible, 
                           lambda_table, lambda_indices, NUM_NODES, SUPPORT);
   }
 }

/********************************************************************
   for a cube of the node, form its u-intersection with every
   cube of node_inv.
*********************************************************************/

static void
form_u_intersection(node, cube, node_inv, intersection, incompatible, 
                    lambda_table, lambda_indices, NUM_NODES, SUPPORT)
node_t *node;
node_cube_t cube;
node_t *node_inv;
int **intersection;
int **incompatible;
st_table *lambda_table;
int *lambda_indices;
int NUM_NODES, SUPPORT;
{
   int NULL_INT;
   int node_inv_numcubes;
   int i;
   int j;
   node_t *fanin;
   node_t *fanin_inv;
   node_literal_t literal;
   int lit_val;
   node_literal_t literal_inv;
   int lit_inv_val;
   struct my_cube lambda_cube;
   struct my_cube lambda_cube_inv;
   node_cube_t cube_inv;

   node_inv_numcubes = node_num_cube(node_inv);
   
   for (i = node_inv_numcubes -1; i >= 0; i--) {
        cube_inv = node_get_cube(node_inv, i); /* m_lambda, m_u */
        NULL_INT = FALSE;
        foreach_fanin(node, j, fanin) {
          if (marked(fanin, lambda_table)) continue; /* ignore lambda fanins */
          literal = node_get_literal(cube, j);
	  lit_val = val_literal(literal);

          fanin_inv = node_get_fanin(node_inv, j);
          if (marked(fanin_inv, lambda_table)) {
            (void) fprintf(sisout, "Fatal error: cube ordering erros\n");
	    exit(1);
	  }
          literal_inv = node_get_literal(cube_inv, j);
	  lit_inv_val = val_literal(literal_inv);
          if (intersection[lit_val][lit_inv_val] == -1) {
              NULL_INT = TRUE;
	      break;
	  }
	}
        if (NULL_INT) continue;
        /* found a non-empty intersection of cube with cube_inv */
	/* form the incompatibity matrix from the lambda parts */
	/*-----------------------------------------------------*/        
	alloc_my_cube(&lambda_cube, SUPPORT);
	alloc_my_cube(&lambda_cube_inv, SUPPORT);
	get_lambda_cube(cube, &lambda_cube, lambda_indices, SUPPORT);
	get_lambda_cube(cube_inv, &lambda_cube_inv, lambda_indices, SUPPORT);
	make_incompatible(lambda_cube, lambda_cube_inv, incompatible, NUM_NODES, SUPPORT);
	free_my_cube(&lambda_cube);
	free_my_cube(&lambda_cube_inv);
  }
}
	 
/*------------------------------------------------------------------
  Get the literals of the cube corresponding to the lambda_indices.
  Return it in lambda_cube. The order of literals is important. So, 
  the 0th literal (variable[0]) is the one corresponding to fanin with
  index lambda_indices[0], and so on...
--------------------------------------------------------------------*/
static void
get_lambda_cube(cube, lambda_cube, lambda_indices, SUPPORT)
  node_cube_t cube;
  struct my_cube *lambda_cube;
  int *lambda_indices;
  int SUPPORT;
{
   int j;
   node_literal_t literal;

   for (j = 0; j < SUPPORT; j++) {
      literal = node_get_literal(cube, lambda_indices[j]);
      lambda_cube->variable[j] = val_literal(literal);
  }
}

/*-----------------------------------------------------------------
  If complemented => return 0, uncomplemented => return 1, else 2.
------------------------------------------------------------------*/

static
int val_literal(literal)
    node_literal_t literal;
{    
    int value;

    switch(literal) {
         case ZERO : /* complemented */
	    value = 0;
	    break;
         case ONE : /* uncomplemented */
	    value = 1;
	    break;
	 case TWO : /* does not appear */
	    value = 2; 
	    break;
    }
    return value;
}


static int**
init_incompatible(NUM_NODES)
int NUM_NODES;
{
 int i;
 int j;
 int **incompatible;

 incompatible = ALLOC(int *, NUM_NODES);
 for (i = 0; i < NUM_NODES; i++) {
     incompatible[i] = ALLOC(int, NUM_NODES);
 }
 
 for (i =0 ; i < NUM_NODES; i++) {
   for (j = 0; j < NUM_NODES; j++) {
     incompatible[i][j] = FALSE;
   }
 } 
 return incompatible;
}

static void
end_incompatible(incompatible, NUM_NODES)
int **incompatible;
int NUM_NODES;
{
 int i;

 for (i = 0; i < NUM_NODES; i++) {
   FREE(incompatible[i]);
 } 
 FREE(incompatible);
}

/*---------------------------------------------------------------------
  Generate all the minterms in a and b my_cubes, and make them 
  incompatible.
----------------------------------------------------------------------*/
static void
make_incompatible(a, b, incompatible, NUM_NODES, SUPPORT)
struct my_cube a, b;
int **incompatible;
int NUM_NODES, SUPPORT;
{

  int i;
  int j;
  int *min_a;
  int *min_b;
  int num_min_a = 0;
  int num_min_b = 0;

  min_a = ALLOC(int, NUM_NODES);
  min_b = ALLOC(int, NUM_NODES);

  generate_all_minterms(min_a, &num_min_a, a, SUPPORT);
  generate_all_minterms(min_b, &num_min_b, b, SUPPORT);
  for (i = 0; i < num_min_a; i++) {
    for (j = 0; j < num_min_b; j++) {
      incompatible[min_a[i]][min_b[j]] = incompatible[min_b[j]][min_a[i]]
      = TRUE;
      
    }
  }
  FREE(min_a);
  FREE(min_b);
}

/*--------------------------------------------------------------------------
  returns in min_arr all the minterms in the cube cube. Returns the number
  of these minterms in num_min.
----------------------------------------------------------------------------*/
static 
void
generate_all_minterms(min_arr, num_min, cube, support)
  int min_arr[];
  int *num_min;
  struct my_cube cube;
  int support;
{

  generate_minterm_with_index(min_arr, num_min, cube, support, 0, 0);
}

static
void 
generate_minterm_with_index(min_arr, num_min, cube, support, value, i)
  int min_arr[];
  int *num_min;
  struct my_cube cube;
  int support;
  int value;
  int i;
{
   if (i == support - 1) {
     switch (cube.variable[i]) {
        case 0: 
                min_arr[*num_min] = 2*value; *num_min = *num_min + 1; 
	        return;
        case 1: 
	        
	        min_arr[*num_min] = 2*value + 1; *num_min = *num_min + 1; 
	        return;
	case 2: 
	        
	        min_arr[*num_min] = 2*value; *num_min = *num_min + 1; 
	        min_arr[*num_min] = 2*value + 1;*num_min = *num_min + 1; 
	        return;
     };
   }
   switch (cube.variable[i]) {
      case 0 : generate_minterm_with_index(min_arr, num_min, cube, support, 2*value, i+1);
	       return;
      case 1 : generate_minterm_with_index(min_arr, num_min, cube, support, 2*value + 1, i+1);
 	       return;
      case 2 : generate_minterm_with_index(min_arr, num_min, cube, support, 2*value, i+1);
               generate_minterm_with_index(min_arr, num_min, cube, support, 2*value + 1, i+1);
	       return;
   }

}

/*------------------------------------------------------------------
  Each treenode[i] corresponds to a minterm.
-------------------------------------------------------------------*/
static  void
tree_init(treenode, NUM_NODES)
tree_node **treenode;
int NUM_NODES;
{

 int i;
 
 for (i = 0; i < NUM_NODES; i++) {
   treenode[i] = ALLOC(tree_node, 1);
   treenode[i]->parent = treenode[i];
   treenode[i]->index = i;
   treenode[i]->num_child = 0;
   treenode[i]->class_num = -1;

 }


}

/*---------------------------------------------------------------------
  Put all the minterms that are compatible in one equivalence class.
  The data structure for equivalence class is a tree. Initially, all 
  minterms are a tree each. Then, we merge the trees corresponding to
  the two compatible minterms: the tree with more children becomes the
  parent tree.
-----------------------------------------------------------------------*/
static void
form_compatibility_classes(treenode, incompatible, NUM_NODES)
tree_node **treenode;
int **incompatible;
int NUM_NODES;
{

  int i;
  int j;
  struct tree_node *root1;
  struct tree_node *root2;

  for (i = 0; i< NUM_NODES; i++) {
    for (j = i+1; j < NUM_NODES; j++) {
      if (incompatible[i][j] == FALSE ) /* i and j are compatible */
	{
	  root1 = find_tree(treenode[i]);
	  root2 = find_tree(treenode[j]);
	  if (root1->index == root2->index) continue;
          /* merge the trees corresponding to the minterms */
          /*-----------------------------------------------*/
	  unionop(root1, root2);

	}
    }
  }
}

/*----------------------------------------------------------------
  Stores the root of all the compatibility classes in root_table.
  Counts the number of equivalence (compatibility) classes. 
  The final value is stored in CLASS_NUM (initially 0). Also
  assigns class number to each minterm.
-----------------------------------------------------------------*/
static void
mark_roots_of_trees(treenode, root_table, NUM_NODES, CLASS_NUM) 
tree_node **treenode;
st_table *root_table;
int NUM_NODES, *CLASS_NUM;

{

  int i;
  tree_node *root;
  char *dummy;
  int value;

 for (i = 0; i <  NUM_NODES; i++) {
  root = find_tree(treenode[i]);
  treenode[i]->parent = root;  
  if (st_is_member(root_table, (char *)root->index)) {
     (void) st_lookup(root_table, (char *)root->index, &dummy);
     value = (int) dummy;
     treenode[i]->class_num = value;
   }
  else {
     (void) st_insert(root_table, (char*)root->index, (char *)(*CLASS_NUM));  
     treenode[i]->class_num = (*CLASS_NUM);
     *CLASS_NUM = (*CLASS_NUM) + 1;

   }
  
 }
}

static void
get_combination(node, comb_num, lambda_indices, SUPPORT)
node_t *node;
int comb_num;
int *lambda_indices;
int SUPPORT;
 
{
  int num_fanin;
  /* adhoc routine - can only return a few combinations*/  
  int i;
  num_fanin = node_num_fanin(node);
  for (i = 0; i < SUPPORT; i++) 
    lambda_indices[i] = (comb_num + i) % num_fanin;

}

/* marks all the inputs of the node as 1 if it corresponds to lambda part,
   else 0 */

static
void
mark_all_inputs(node, lambda_table, lambda_indices, SUPPORT)
node_t *node;
st_table *lambda_table;  
int *lambda_indices, SUPPORT;
{

  int islambda_fanin;
  int j;
  node_t *fanin;

  foreach_fanin(node, j, fanin) {
      islambda_fanin =  is_fanin_in_lambda(node, fanin, lambda_indices, SUPPORT);
      (void) 	st_insert(lambda_table, (char *)fanin, (char *)islambda_fanin);
    }
}

/*---------------------------------------------------------------
  Returns 1 if the fanin is in lambda_part. Else returns 0.
----------------------------------------------------------------*/
static
int is_fanin_in_lambda(node, fanin, lambda_indices, SUPPORT)
node_t *fanin ;
node_t *node;
int *lambda_indices, SUPPORT;
{

    int i;
    node_t *fanin_my;

    for (i = 0; i < SUPPORT; i++) {
      fanin_my = node_get_fanin(node, lambda_indices[i]);
      if (fanin_my == fanin) return 1;
    }
    return 0;
}

/*---------------------------------------------------------------
  Returns 1 if the fanin is in lambda_table (i.e., it is in 
  lambda_set.
-----------------------------------------------------------------*/
static
int marked(fanin, lambda_table)
node_t *fanin;
st_table *lambda_table;
{
   char *dummy;  
   int found;


   found = st_lookup(lambda_table, (char *)fanin, &dummy);
   if (!found) {
     (void) fprintf(sisout,  " Fatal error: marked(fanin) : error in finding node\n", 
	     node_name(fanin));
     exit(1);
   }
   return (int)dummy;
}

/*-----------------------------------------------------------------------
  f = g(alpha1, alpha2, ...., alphat, ....);
  Number of ALPHA functions is log2 of number of compatibility classes.
  fn_array stores for a minterm binary value of classnum that minterm
  belongs to. 
  From fn_array, the ALPHA functions are constructed by ORing the 
  minterms that result in 1 for that function.
-------------------------------------------------------------------------*/

static int
find_alphas_and_G(node_to_decomp, lambda_table, treenode, lambda_indices, 
                  NUM_NODES, SUPPORT, CLASS_NUM, NUM_ALPHAS)
node_t *node_to_decomp;
st_table *lambda_table;
tree_node **treenode;
int *lambda_indices;
int NUM_NODES, SUPPORT, CLASS_NUM, NUM_ALPHAS;
{

 char **fn_array;
 node_t *fn, *fn_simplified;
 node_t *fn_node1;
 node_t *node_for_minterm;
 int minterm;
 int fn_num;
 int FLAG;

 fn_array = ALLOC(char *, NUM_NODES);

 if (CLASS_NUM == 0) {
    (void) fprintf(sisout, "Fatal error: no class?\n");
    exit(1);
 }
 if (CLASS_NUM == 1) {
    (void) fprintf(sisout, "Fatal error: strange!!\n");
    exit(1);
 }

 ALPHA = ALLOC(node_t *, NUM_ALPHAS);
 /* for each minterm (in lambda_space), fn_array stores the binary. rep.
    of the class number */
 for (minterm = 0; minterm < NUM_NODES; minterm++) {
   fn_array[minterm] = ALLOC(char, NUM_ALPHAS + 1);
   xl_binary1(treenode[minterm]->class_num, NUM_ALPHAS, fn_array[minterm]);
 }
 /* (void) printf( "NUM_ALPHAS = %d\n", NUM_ALPHAS); */

 for (fn_num = 0; fn_num < NUM_ALPHAS; fn_num++) {
   fn = node_constant(0);
   FLAG = 0;
   for (minterm =0; minterm < NUM_NODES; minterm++) {
      if (fn_array[minterm][fn_num] == '0') continue;
      FLAG = 1;
      node_for_minterm = product_node_for_minterm(minterm, node_to_decomp, 
                                                  lambda_indices, SUPPORT);
      fn_node1 = node_or(node_for_minterm, fn);
      node_free(fn);
      node_free(node_for_minterm);
      fn = fn_node1;
   };
   if (FLAG) {
          if (XLN_DEBUG > 1) {
              (void) printf("fn before simplify = ");
              node_print_rhs(sisout, fn);
          }
	  fn_simplified = node_simplify(fn, (node_t *) NULL, NODE_SIM_ESPRESSO);
          ALPHA[fn_num] = fn_simplified;
          node_free(fn); 
   }
   else ALPHA[fn_num] = fn;
 }
 G_node =  find_G(node_to_decomp, lambda_table, lambda_indices, 
                  treenode, NUM_NODES, SUPPORT, NUM_ALPHAS);

 if (XLN_DEBUG > 1) {
     (void) printf("G_node = ");
     node_print_rhs(sisout, G_node);
 }
 for (minterm = 0; minterm < NUM_NODES; minterm++) {
     FREE(fn_array[minterm]);
 }
 FREE(fn_array);
}

/*-----------------------------------------------------------------------------
  Form a node that corresponds to the minterm of the node (the minterm is in
  lambda_space).
------------------------------------------------------------------------------*/
static 
node_t *
product_node_for_minterm(minterm, node_to_decomp, lambda_indices, SUPPORT)
int minterm;
node_t *node_to_decomp;
int *lambda_indices, SUPPORT;
{

  node_t *node;
  node_t *fn;
  node_t *fn1, *literal_fn, *fanin;
  char char_minterm[BUFSIZE];
  int length, i;
  int phase;

  /* get a binary representation of minterm */
  fn = node_constant(1);
  xl_binary1(minterm, SUPPORT, char_minterm);
  length = strlen (char_minterm);
  if (length != SUPPORT) {
    (void) fprintf(sisout,  "Fatal error: product_node_for_minterm(): length not SUPPORT\n");
    exit(1);
  }
  for (i = 0; i < length; i++) {
     fanin = node_get_fanin(node_to_decomp, lambda_indices[i]); 
     if (char_minterm[i] == '0') phase = 0;
     else phase = 1;
     literal_fn = node_literal(fanin, phase);
     fn1 = node_and (literal_fn, fn);
     node_free(fn);
     node_free(literal_fn);
     fn = fn1;
  }
  node = fn;
  return node;
}

/*------------------------------------------------------------------------
  Find the corresponding function G of the initial node to be decomposed.
--------------------------------------------------------------------------*/
static
node_t *
find_G( node_to_decomp, lambda_table, lambda_indices, treenode, NUM_NODES, 
       SUPPORT,  NUM_ALPHAS)
node_t *node_to_decomp;
st_table *lambda_table; 
int *lambda_indices;
tree_node **treenode;
int NUM_NODES, SUPPORT,  NUM_ALPHAS;

{
  char encoded_char[BUFSIZE];
  node_t *Gnode;  /* to be returned */
  node_t *fanin;
  node_t *product_fn, *product_fn1, *literal_fn;
  node_t *nodeG, *nodeG1, *nodeG2;
  node_t *u_fn, *u_fn1, *cube_fn, *cube_fn1;
  int i, j, k, l;
  int num_min;
  int *min_arr, minterm;
  int lit_val, phase;
  node_cube_t cube;
  struct my_cube lambda_cube;
  node_literal_t literal;

  min_arr = ALLOC(int, NUM_NODES);
  nodeG = node_constant(0);
  i = node_num_cube(node_to_decomp);
  if (i == 0) {
    (void) fprintf(sisout,  "Fatal error:  node %s has no cubes??\n", 
            node_name(node_to_decomp));
    exit(1);
  }
  for (i = node_num_cube(node_to_decomp) - 1 ; i>= 0; i--) {
     cube = node_get_cube(node_to_decomp, i);
     alloc_my_cube(&lambda_cube, SUPPORT);
     get_lambda_cube(cube, &lambda_cube, lambda_indices, SUPPORT);
     num_min = 0;
     generate_all_minterms(min_arr, &num_min, lambda_cube, SUPPORT);

     /* compute fn. corresponding to the u_part only once for the
     cube */
     /*---------------------------------------------------------*/
     u_fn = node_constant(1);
     foreach_fanin(node_to_decomp, j, fanin) {
        if (marked(fanin, lambda_table)) continue;
        literal = node_get_literal(cube, j);
	lit_val = val_literal(literal);
	if (lit_val != 2) {
           literal_fn = node_literal(fanin, lit_val);
	   u_fn1 = node_and(literal_fn, u_fn);
	   node_free(literal_fn);
	   node_free(u_fn);
           u_fn = u_fn1;
        }
     }                

    /* for each minterm in the lambda_space, compute the 
     alpha_functions; OR them together, then AND with u_fn */
    /*-----------------------------------------------------*/

     cube_fn = node_constant(0);

     for (k = 0; k < num_min; k++) {
        product_fn = node_constant(1);
        minterm = min_arr[k];
	xl_binary1(treenode[minterm]->class_num, NUM_ALPHAS, 
                encoded_char);
 	for (l = 0; l < NUM_ALPHAS; l++) {
           if (encoded_char[l] == '0') phase = 0;
           else phase = 1;
           literal_fn = node_literal(ALPHA[l], phase);
	   product_fn1 = node_and (literal_fn, product_fn);
	   node_free(literal_fn);
	   node_free(product_fn);
	   product_fn = product_fn1;
        }
        cube_fn1 = node_or(cube_fn, product_fn);
        node_free(cube_fn);
	node_free(product_fn);
        cube_fn = cube_fn1;
    }
    
    nodeG1 = node_and(cube_fn, u_fn);
    node_free(cube_fn);
    node_free(u_fn);

    nodeG2 = node_or(nodeG1, nodeG);
    node_free(nodeG1);
    node_free(nodeG);
    nodeG = nodeG2;   
    free_my_cube(&lambda_cube);
 } /* for i */
 
  Gnode = node_simplify(nodeG, (node_t *) NULL, NODE_SIM_ESPRESSO);
  node_free(nodeG);
  FREE(min_arr);
  return Gnode;
} 
	    
/* frees the storage allocated for the nodes treenode[] after a node of the 
   network has been decomposed  */

/*ARGSUSED*/
static void
free_tree(treenode, root_table, lambda_table, NUM_NODES)
tree_node **treenode;
st_table *root_table, *lambda_table;
int NUM_NODES;
{

  int i;
  char *arg;

  for (i = 0; i < NUM_NODES; i++) {
    FREE(treenode[i]);
  }
  FREE(treenode);
  st_foreach(root_table, k_decomp_free_table_entry, arg); 
  st_free_table(root_table);
  st_foreach(lambda_table, k_decomp_free_table_entry, arg); 
  st_free_table(lambda_table);

}

/*ARGSUSED*/
static
enum st_retval 
k_decomp_free_table_entry(key, value, arg)
  char *key, *value, *arg;
{
  return ST_DELETE;
}

/*-------------------------------------------------------------------
  The decomposed node is replaced by the set of ALPHA nodes and
  G node. The network changes as a result.
--------------------------------------------------------------------*/
static void
replace_node_decomposed(network, node_to_replace,  NUM_ALPHAS)
network_t *network;
node_t *node_to_replace;
int  NUM_ALPHAS;
{

  int i; 
  
  /* add nodes corresponding to all ALPHA nodes and G node */
  /*-------------------------------------------------------*/
  for (i = 0; i < NUM_ALPHAS; i++) {
      network_add_node(network,  ALPHA[i]);
  };
  FREE(ALPHA);
  node_replace(node_to_replace, G_node);

}

static void
alloc_my_cube(cube, support)
  struct my_cube *cube;
  int  support;
{
    cube->variable = ALLOC(int, support);
}

static void
free_my_cube(cube)
 struct my_cube *cube;

{
   FREE(cube->variable);
}

static int** 
init_intersection()
{
  int **intersection;
  int i;

  /* initializing intersection function */
  /*------------------------------------*/
  intersection = ALLOC(int *, 3);
  for (i = 0; i < 3; i++) {
 	intersection[i] = ALLOC(int, 3);
  }
  intersection[0][1] = -1;
  intersection[1][0] = -1;
  intersection[0][0] = intersection[0][2] = intersection[2][0] = 0;
  intersection[1][1] = intersection[1][2] = intersection[2][1] = 1;
  intersection[2][2] = 2;
  return intersection;
}

static void
end_intersection(intersection)
  int **intersection;
{
  int i;
  for (i = 0; i < 3; i++){
      FREE(intersection[i]);
  }
  FREE(intersection);
}

/*-------------------------------------------------------------------------
   Given lambda indices as the bound set and SUPPORT being the 
   support of lambda indices, decompose the user_node.
   Not recursive. Just does it once. Returns a network with the
   decomposition. user_node is not changed if it does not belong to
   a network, else it may be simplified.
--------------------------------------------------------------------------*/
network_t *
xln_k_decomp_node_with_network(user_node, lambda_indices, SUPPORT)
  node_t *user_node;
  int *lambda_indices, SUPPORT;
{
  network_t *network; /* created out of user_node */
  node_t *node, *node_inv, *node_simpl;
  lsGen gen;
  int NUM_NODES, CLASS_NUM, NUM_ALPHAS;
  int **intersection, **incompatible;
  tree_node **treenode;
  st_table *root_table, *lambda_table;
  int new_support;

  node_simpl = node_simplify(user_node, (node_t *) NULL, NODE_SIM_ESPRESSO);

  /* Added July 1 '93 */
  new_support = xln_modify_lambda_indices(user_node, node_simpl, lambda_indices, SUPPORT); 
  if (new_support <= 1) {
      node_free(node_simpl);
      return NIL (network_t);
  }
  SUPPORT = new_support;

  network = network_create_from_node(node_simpl);
  if (node_network(user_node) != NIL (network_t)) {
      node_replace(user_node, node_simpl);
  } else {
      node_free(node_simpl);
  }
  /* NUM_NODES = (int)(pow((double)2, (double)SUPPORT)); */
  NUM_NODES = 1 << SUPPORT;
  intersection = init_intersection();
  incompatible = init_incompatible(NUM_NODES);
  
  /* to store the info. about the root of the trees. CLASS_NUM tells the 
     class_num of each tree (but is valid only at the end of mark_root */
  
  root_table = st_init_table(st_ptrcmp, st_ptrhash);
  lambda_table = st_init_table(st_ptrcmp, st_ptrhash); 
  CLASS_NUM = 0;
  treenode = ALLOC (tree_node *, NUM_NODES);
  tree_init(treenode, NUM_NODES);

  /* get the internal node of the network- this would be decomposed */
  /*----------------------------------------------------------------*/
  foreach_node(network, gen, node) {
      if (node->type == INTERNAL) break;
  }
  node_inv = node_not(node);

  form_incompatibility_graph(node, node_inv, intersection, incompatible, 
                             lambda_table, lambda_indices, NUM_NODES, SUPPORT);
  form_compatibility_classes(treenode, incompatible, NUM_NODES);           
  mark_roots_of_trees(treenode, root_table, NUM_NODES, &CLASS_NUM);
  NUM_ALPHAS = intlog2(CLASS_NUM);
  if (NUM_ALPHAS == SUPPORT) {
      node_free(node_inv);
      free_tree(treenode, root_table, lambda_table, NUM_NODES);
      end_incompatible(incompatible, NUM_NODES);  
      return NIL (network_t);
  }
  find_alphas_and_G(node, lambda_table, treenode, lambda_indices, 
                                 NUM_NODES, SUPPORT, CLASS_NUM, NUM_ALPHAS);

  /* node is changed, ALPHA freed */
  /*------------------------------*/ 
  replace_node_decomposed(network, node,  NUM_ALPHAS);
  node_free(node_inv);
  /* frees the root_table, lambda table also */
  /*-----------------------------------------*/
  free_tree(treenode, root_table, lambda_table, NUM_NODES); 
  end_incompatible(incompatible, NUM_NODES);    
  end_intersection(intersection);
  (void) network_sweep(network); /* node simplify may be simplifying some nodes */
  return network;
}

/*--------------------------------------------------------------------
  Same as above, except that it takes in a node of the network and 
  returns its decomposition in this array of nodes. Original node and
  network remain unchanged. Slightly different from 
  karp_decomp_internal. If NUM_ALPHAS( number of alpha functions) is 
  greater than bound_alphas, return NIL. If a valid decomposition is 
  found, the first node in the array corresponds to the original node.
----------------------------------------------------------------------*/
array_t *
xln_k_decomp_node_with_array(node, lambda_indices, SUPPORT, bound_alphas)
  node_t *node;
  int *lambda_indices, SUPPORT, bound_alphas;
{
  node_t *node_inv, *node_simpl;
  int NUM_NODES, CLASS_NUM, NUM_ALPHAS;
  int **intersection, **incompatible;
  tree_node **treenode;
  st_table *root_table, *lambda_table;
  int i;
  array_t *nodevec;  
  array_t *check_vec, *xln_checking_lambda_indices_create();
  int new_support;

  node_simpl = node_simplify(node, (node_t *) NULL, NODE_SIM_ESPRESSO);
  if (node_num_fanin(node) != node_num_fanin(node_simpl)) {
      (void) printf("warning---- node was not expressed on minimal support\n");
      (void) printf("warning---- changing node %s\n", node_long_name(node));
  }
  new_support = xln_modify_lambda_indices(node, node_simpl, lambda_indices, SUPPORT); 
  if (new_support != SUPPORT) {
      (void) printf("warning---- a fanin of the node deleted after simplifying\n");      
      (void) printf("warning---- new lambda set is of cardinality %d (not %d),\n",
                    new_support, SUPPORT);      
      (void) printf("old node = ");
      node_print(stdout, node);
      (void) printf("\n, simplified node = ");
      node_print(stdout, node_simpl);      
      (void) printf("\n");      
      if (new_support <= 1) {
          (void) printf("the lambda set is no good\n");
          node_free(node_simpl);
          return (NIL (array_t));
      }
      SUPPORT = new_support;
  }
  node_replace(node, node_simpl);
  check_vec = xln_checking_lambda_indices_create(node, lambda_indices, SUPPORT);

  node_inv = node_not(node);
  xln_checking_lambda_indices(node_inv, lambda_indices, check_vec);
  array_free(check_vec);

  /* NUM_NODES = (int)(pow((double)2, (double)SUPPORT)); */
  NUM_NODES = 1 << SUPPORT;
  intersection = init_intersection();
  incompatible = init_incompatible(NUM_NODES);
  
  /* to store the info. about the root of the trees. CLASS_NUM tells the 
     class_num of each tree (but is valid only at the end of mark_root */
  
  root_table = st_init_table(st_ptrcmp, st_ptrhash);
  lambda_table = st_init_table(st_ptrcmp, st_ptrhash); 
  CLASS_NUM = 0;
  treenode = ALLOC (tree_node *, NUM_NODES);
  tree_init(treenode, NUM_NODES);

  form_incompatibility_graph(node, node_inv, intersection, incompatible, 
                             lambda_table, lambda_indices, NUM_NODES, SUPPORT);
  form_compatibility_classes(treenode, incompatible, NUM_NODES);           
  /* printing */
  if (XLN_DEBUG > 2) {
      xln_print_incompatible_matrix(incompatible, NUM_NODES);
  }

  mark_roots_of_trees(treenode, root_table, NUM_NODES, &CLASS_NUM);
  NUM_ALPHAS = intlog2(CLASS_NUM);
  if (XLN_DEBUG) {
      (void) printf("number of equivalence classes = %d\n", CLASS_NUM);
  }
  
  /* added May 29, 91 for generating functions 
     only if this condition is met 
     check when the node cannot be decomposed */
  /*------------------------------------------*/
  if ((NUM_ALPHAS > bound_alphas) || (NUM_ALPHAS == SUPPORT)) {
      node_free(node_inv);
      free_tree(treenode, root_table, lambda_table, NUM_NODES);
      end_incompatible(incompatible, NUM_NODES);  
      return NIL (array_t);
  }
  find_alphas_and_G(node, lambda_table, treenode, lambda_indices, 
                                 NUM_NODES, SUPPORT, CLASS_NUM, NUM_ALPHAS);

  nodevec = array_alloc(node_t *, 0);
  array_insert_last(node_t *, nodevec, G_node);
  for (i = 0; i < NUM_ALPHAS; i++) {
      array_insert_last(node_t *, nodevec, ALPHA[i]);

  }
  node_free(node_inv);
  FREE(ALPHA);
  /* frees the root_table, lambda table also */
  /*-----------------------------------------*/
  free_tree(treenode, root_table, lambda_table, NUM_NODES); 
  end_incompatible(incompatible, NUM_NODES);    
  end_intersection(intersection);
  return nodevec;
}

array_t *
xln_checking_lambda_indices_create(node, lambda_indices, SUPPORT)
  node_t *node;
  int *lambda_indices;
  int SUPPORT;
{
  array_t *check_vec;
  int i;
  node_t *fanin;

  check_vec = array_alloc(node_t *, 0);
  for (i = 0; i < SUPPORT; i++) {
      fanin = node_get_fanin(node, lambda_indices[i]);
      array_insert_last(node_t *, check_vec, fanin);
  }
  return check_vec;
}

xln_checking_lambda_indices(node, lambda_indices, check_vec)
  node_t *node;
  int *lambda_indices;
  array_t *check_vec;
{
  int i;
  node_t *fanin;
  
  for (i = 0; i < array_n(check_vec); i++) {
      fanin = array_fetch(node_t *, check_vec, i);
      assert(lambda_indices[i] == node_get_fanin_index(node, fanin));
  }
}

/*--------------------------------------------------------------------
  Given a node and a simplified node, generates new lambda_indices 
  to take care of any fanins that have been deleted and were there in 
  lambda_indices.
---------------------------------------------------------------------*/
int
xln_modify_lambda_indices(node, node_simpl, lambda_indices, SUPPORT)
  node_t *node, *node_simpl;
  int *lambda_indices, SUPPORT;
{
  int i, j;
  node_t *fanin;

  for (i = 0, j =0; i < SUPPORT; i++) {
      fanin = node_get_fanin(node, lambda_indices[i]);
      if (node_get_fanin_index(node_simpl, fanin) >= 0) {
          lambda_indices[j++] = lambda_indices[i];
      }
  }
  return j;
}


xln_print_incompatible_matrix(incompatible, NUM_NODES)
  int **incompatible;
  int NUM_NODES;
{
  int i, j;

  for (i = 0; i< NUM_NODES; i++) {
      for (j = i+1; j < NUM_NODES; j++) {
          (void) printf("%d ", incompatible[i][j]);
      }
      (void) printf("\n");
  }
}
