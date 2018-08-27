/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_map_par.c,v $
 * $Author: pchong $
 * $Revision: 1.3 $
 * $Date: 2005/03/08 01:07:23 $
 *
 */

/* March 30, 1992 - changed weight to be int * in partition_network() */

/*  This file contains routines for partitioning cells into unit blocks.
 *  This corresponds to a technology mapping in the conventional LSI.
 *  functions contained in this file:
 *	merge()
 *  Import descriptions:
 *	sis.h		: macros for misII
 *	ULM_int.h	: macros for universal logic module package
 */

#include "pld_int.h"
#include "sis.h"

extern sm_row *sm_mat_bin_minimum_cover();
extern sm_row *sm_mat_minimum_cover();

mf_graph_t *pld_nishiza_graph;       /* a graph (flow network) */
network_t *pld_nishiza_network;      /* network we are working on */
nodeindex_t *pld_nishiza_hash_table; /* hash table for node <--> index */
int pld_nishiza_g_flag;              /* if ON, found a new matching */

/* added by Rajeev Dec. 27, 1989 */
int ULM_INPUT_SIZE;
array_t *solution_array; /* matrix with the selected columns */

void print_suppressed_node()

{
  lsGen gen;           /* generators */
  node_t *node;        /* a node in the network */
  char name1[BUFSIZE]; /* name buffer */
  char name2[BUFSIZE]; /* name buffer */

  (void)printf("\tSuppressed nodes : ");
  foreach_node(pld_nishiza_network, gen, node) {
    if (node->type == PRIMARY_OUTPUT) {
      continue;
    }
    /* Watch for the Vdd and GND node */
    if (node->type == INTERNAL && node_num_fanin(node) == 0) {
      continue;
    }
    (void)sprintf(name1, "%s_top", node_long_name(node));
    (void)sprintf(name2, "%s_bottom", node_long_name(node));
    if (get_maxflow_edge(pld_nishiza_graph, name2, name1)->capacity ==
        (int)MAXINT) {
      (void)printf("%s ", node_long_name(node));
    }
  }
  (void)printf("\n");
}

/*
 *  construct_maxflow_network()
 *	Construct a network for maxflow problem to get s-t connectivity
 */

void construct_maxflow_network()

{

  lsGen gen, gen2;     /* generators */
  node_t *node;        /* a node in the network */
  node_t *fanout;      /* a fanout node of a node in the network */
  char name1[BUFSIZE]; /* name buffer */
  char name2[BUFSIZE]; /* name buffer */

  /* Create a dummy node for a source */
  /*----------------------------------*/
  mf_read_node(pld_nishiza_graph, SOURCE_NAME, 1);

  foreach_node(pld_nishiza_network, gen, node) {

    if (node->type == PRIMARY_OUTPUT) {
      continue;
    }
    /* Watch for the Vdd and GND node */
    if (node->type == INTERNAL && node_num_fanin(node) == 0) {
      continue;
    }

    /* Create both top and bottom nodes and add an edge between them */
    /*---------------------------------------------------------------*/
    (void)sprintf(name1, "%s_top", node_long_name(node));
    mf_read_node(pld_nishiza_graph, name1, 0);
    (void)sprintf(name2, "%s_bottom", node_long_name(node));
    mf_read_node(pld_nishiza_graph, name2, 0);

    mf_read_edge(pld_nishiza_graph, name2, name1, 1);

    /* If this is PI, add an edge from source to the bottom node */
    /*-----------------------------------------------------------*/
    if (node->type == PRIMARY_INPUT) {
      mf_read_edge(pld_nishiza_graph, SOURCE_NAME, name2, (int)MAXINT);
    }
  }

  /* Add edges between intermediate nodes */
  /*--------------------------------------*/
  foreach_node(pld_nishiza_network, gen, node) {
    if (node->type == PRIMARY_OUTPUT) {
      continue;
    }
    /* Watch for the Vdd and GND node */
    if (node->type == INTERNAL && node_num_fanin(node) == 0) {
      continue;
    }
    (void)sprintf(name1, "%s_top", node_long_name(node));
    foreach_fanout(node, gen2, fanout) {
      if (fanout->type == PRIMARY_OUTPUT) {
        continue;
      }
      (void)sprintf(name2, "%s_bottom", node_long_name(fanout));
      mf_read_edge(pld_nishiza_graph, name1, name2, (int)MAXINT);
    }
  }
}

/*
 *  form_suppress_matrix()
 *	Form a sparse matrix which represents a relationship between separating
 *	sets and their covering nodes.
 *	Each row corresponds to a separating set and each column a node.
 */

void form_suppress_matrix(matrix, sepset_array)

    sm_matrix *matrix;
array_t *sepset_array;

{
  int i, j;
  array_t *sepset_node_array;
  int idx;
  node_t *node;

  for (i = 0; i < array_n(sepset_array); ++i) {
    sepset_node_array = array_fetch(array_t *, sepset_array, i);
    for (j = 0; j < array_n(sepset_node_array); ++j) {
      node = array_fetch(node_t *, sepset_node_array, j);
      idx = nodeindex_indexof(pld_nishiza_hash_table, node);
      (void)sm_insert(matrix, i, idx);
    }
  }
}

/*
 *  squeeze_matching()
 *	Generate one matching which has the sink node as an output
 *	by changing the combination of suppressed node.
 *	Used when the generation of matching in generate_matching fails.
 */

int squeeze_matching(row)

    sm_row *row;

{
  sm_element *element;
  node_t *node;
  int maxflow;

  /* Suppress the nodes corresponding to the column cover */
  for (element = row->first_col; element != NULL; element = element->next_col) {
    node = nodeindex_nodeof(pld_nishiza_hash_table, element->col_num);
    change_edge_capacity(pld_nishiza_graph, node, (int)MAXINT);
  }
  if (XLN_DEBUG) {
    print_suppressed_node();
  }
  maxflow = get_maxflow(pld_nishiza_graph);

  /* If found a separating set, mark the flag and return */
  if (maxflow <= ULM_INPUT_SIZE) {
    pld_nishiza_g_flag = ON;
    return (1);
  }

  /* Release the nodes corresponding to the column cover and try next */
  for (element = row->first_col; element != NULL; element = element->next_col) {
    node = nodeindex_nodeof(pld_nishiza_hash_table, element->col_num);
    change_edge_capacity(pld_nishiza_graph, node, 1);
  }
  return (0);
}

/*
 *  initialize_edge_capacity()
 *	Initialize the capacity of edges
 */

void initialize_edge_capacity()

{
  node_t *node;
  lsGen gen;

  foreach_node(pld_nishiza_network, gen, node) {
    if (node->type == PRIMARY_OUTPUT) {
      continue;
    }
    /* Watch for the Vdd and GND node */
    if (node->type == INTERNAL && node_num_fanin(node) == 0) {
      continue;
    }
    change_edge_capacity(pld_nishiza_graph, node, 1);
  }
}

/*
 *  generate_matching()
 *	Generate (all) matching(s) which has the sink node as an output
 */

void generate_matching(sink_array, sepset_array, count)

    array_t *sink_array; /* array of sink nodes for this matching,
                          * simply copying the current sink node of the
                          * network */
array_t *sepset_array;   /* array of pointers to arrays of separating
                          * sets for the matchings obtained */
int count;

{

  int i;
  node_t *node;
  mf_cutset_t *cutset; /* cutset of the network */
  array_t *sepset_node_array;
  /* array of pointers to arrays of separating
   * sets for the matchings obtained */
  array_t *from_array; /* array of graph nodes (from) in the cutset */
  array_t *to_array;   /* array of graph nodes (to) in the cutset */
  array_t *flow_array; /* array of flows for edges in the cutset */
  int maxflow;
  sm_matrix *suppress_matrix;
  char sup_file[1000];
  FILE *supFile;
  int sfd;

  for (;;) {
    if (XLN_DEBUG) {
      print_suppressed_node();
    }
    maxflow = get_maxflow(pld_nishiza_graph);

    /* If a finite maxflow was not obtained, try a squeeze operation */
    /*---------------------------------------------------------------*/
    if (maxflow > ULM_INPUT_SIZE) {
      if (XLN_DEBUG) {
        (void)printf("Squeezing operation ...\n");
      }
      initialize_edge_capacity();
      suppress_matrix = sm_alloc();
      form_suppress_matrix(suppress_matrix, sepset_array);
      if (XLN_DEBUG) {
        (void)sprintf(sup_file, "/usr/tmp/xln.merge.sup_matrix.XXXXXX");
        sfd = mkstemp(sup_file);
        if (sfd != -1) {
          supFile = fdopen(sfd, "w");
          if (supFile != NULL) {
            sm_print(supFile, suppress_matrix);
            (void)fclose(supFile);
          }
          close(sfd);
        }
      }
      pld_nishiza_g_flag = OFF;
      (void)sm_mat_minimum_cover(suppress_matrix, NULL, 0, 0, 0, 4096,
                                 squeeze_matching);
      sm_free(suppress_matrix);
      if (pld_nishiza_g_flag == OFF) {
        if (XLN_DEBUG) {
          (void)printf("No more finite maxflows\n\n");
        }
        return;
      }
    }

    /* Record the current sink */
    /*-------------------------*/
    node = graph2network_node(pld_nishiza_network,
                              mf_get_sink_node(pld_nishiza_graph));
    array_insert_last(node_t *, sink_array, node);

    /* Record the current separating set */
    /*-----------------------------------*/
    sepset_node_array = array_alloc(node_t *, 0);
    array_insert_last(array_t *, sepset_array, sepset_node_array);
    cutset =
        mf_get_cutset(pld_nishiza_graph, &from_array, &to_array, &flow_array);
    if (XLN_DEBUG) {
      (void)printf("Found a separating set (%d) : ", count);
    }
    count++;
    for (i = 0; i < array_n(flow_array); i++) {
      if (array_fetch(int, flow_array, i) == 1) {
        node = graph2network_node(
            pld_nishiza_network,
            mf_get_node(pld_nishiza_graph, array_fetch(char *, from_array, i)));
        array_insert_last(node_t *, sepset_node_array, node);
        if (XLN_DEBUG) {
          (void)printf("%s ", node_long_name(node));
        }
      }
    }
    if (XLN_DEBUG) {
      (void)printf("\n");
    }
    array_free(from_array);
    array_free(to_array);
    array_free(flow_array);
    mf_free_cutset(cutset);

    /* Suppress the mincut found just now */
    /*------------------------------------*/
    node = array_fetch(node_t *, sepset_node_array, 0);
    change_edge_capacity(pld_nishiza_graph, node, (int)MAXINT);
  }
}

/*
 *  DFS_covered_node()
 *	DFS to get all the intermediate nodes covered by the matching
 */

void DFS_covered_node(node, node_array, flag)

    node_t *node;    /* current node */
array_t *node_array; /* array of nodes covered by this matching */
int *flag;           /* array of flags, if ON, threaded by DFS */

{

  node_t *fanin;
  int i;
  int idx; /* index of node */

  /* Set the flag at ON and store the node in the array */
  /*----------------------------------------------------*/
  idx = nodeindex_indexof(pld_nishiza_hash_table, node);
  flag[idx] = ON;
  array_insert_last(node_t *, node_array, node);

  foreach_fanin(node, i, fanin) {
    idx = nodeindex_indexof(pld_nishiza_hash_table, fanin);
    if (flag[idx] == OFF) {
      DFS_covered_node(fanin, node_array, flag);
    }
  }
}

/*
 *  form_binate_matrix()
 *	Form a sparse matrix for binate covering problem
 */

void form_binate_matrix(network, matrix, weight, sink_array, sepset_array,
                        pbasic_num_rows)

    network_t *network;
sm_matrix *matrix;     /* sparse matrix for binate covering problem */
int *weight;           /* array of weight of each column of matrix */
array_t *sink_array;   /* array of sink nodes for all the matchings */
array_t *sepset_array; /* array of pointers to arrays of separating
                        * set for all matchings */
int *pbasic_num_rows;

{

  node_t *node; /* a node */
  node_t *sink; /* a sink */
  node_t *sink_scan, *po;
  array_t *sepset_node_array;
  /* array of nodes in separating set */
  array_t *node_array; /* array of nodes covered in the matching */
  int row_index;       /* row index of a sparse matrix */
  int i, j, k;
  int num_node; /* # of PI's and internal nodes */
  int idx;      /* index of a node */
  int *flag;    /* flags for DFS, if ON, threaded by DFS */
  lsGen gen;

  /* Initialization */
  /*----------------*/
  num_node = network_num_pi(pld_nishiza_network) +
             network_num_internal(pld_nishiza_network);
  flag = ALLOC(int, num_node);
  row_index = num_node - 1;

  /* a node that fans out to a PO must be output of a match */
  /*--------------------------------------------------------*/
  foreach_primary_output(network, gen, po) {
    /* If this node fans out to PO, should be implemented by some cover */
    /*------------------------------------------------------------------*/
    sink = node_get_fanin(po, 0);
    if ((sink->type == INTERNAL) && (node_num_fanin(sink) == 0))
      continue;
    ++row_index;
    for (k = 0; k < array_n(sink_array); k++) {
      sink_scan = array_fetch(node_t *, sink_array, k);
      if (sink_scan == sink) {
        (void)sm_insert(matrix, row_index, 2 * k);
      }
    }
  }

  *pbasic_num_rows = row_index + 1;
  for (i = 0; i < array_n(sink_array); i++) {

    sink = array_fetch(node_t *, sink_array, i);
    sepset_node_array = array_fetch(array_t *, sepset_array, i);

    /* Initialize DFS flag */
    /*---------------------*/
    for (j = 0; j < num_node; ++j) {
      flag[j] = OFF;
    }
    for (j = 0; j < array_n(sepset_node_array); j++) {
      node = array_fetch(node_t *, sepset_node_array, j);
      idx = nodeindex_indexof(pld_nishiza_hash_table, node);
      flag[idx] = ON;
    }

    /* DFS to get all the intermediate nodes covered by current matching */
    /*-------------------------------------------------------------------*/
    node_array = array_alloc(node_t *, 0);
    DFS_covered_node(sink, node_array, flag);
    if (XLN_DEBUG) {
      (void)printf("Cover(%d) = %s(%d) | ", i, node_long_name(sink),
                   nodeindex_indexof(pld_nishiza_hash_table, sink));
      print_array(node_array, pld_nishiza_hash_table);
    }

    /* Insert an element for each node covered by this matching */
    /*----------------------------------------------------------*/
    for (j = 0; j < array_n(node_array); j++) {
      node = array_fetch(node_t *, node_array, j);
      idx = nodeindex_indexof(pld_nishiza_hash_table, node);

      /* Watch for the Vdd and GND node */
      if (node->type == INTERNAL && node_num_fanin(node) == 0) {
        continue;
      }

      (void)sm_insert(matrix, idx, 2 * i);
    }
    array_free(node_array);

    /* Each input of this matching should be an output of some other */
    /*---------------------------------------------------------------*/
    for (j = 0; j < array_n(sepset_node_array); j++) {
      node = array_fetch(node_t *, sepset_node_array, j);
      if (node->type == PRIMARY_INPUT) {
        continue;
      }
      ++row_index;

      for (k = 0; k < array_n(sink_array); k++) {
        sink_scan = array_fetch(node_t *, sink_array, k);
        if (sink_scan == node) {
          (void)sm_insert(matrix, row_index, 2 * k);
        }
      }

      /* If we do not pick up this matching, no problem */
      /*------------------------------------------------*/
      (void)sm_insert(matrix, row_index, 2 * i + 1);
    }

    /* Set weight */
    /*------------*/
    weight[2 * i] = 1;
    weight[2 * i + 1] = 0;
  }

  /* Termination */
  /*-------------*/
  FREE(flag);
}

/*
 *  partial_collapse_node()
 *	Collapse the given node until it reaches the separating set
 */

node_t *partial_collapse_node(sink, sepset_node_array)

    node_t *sink; /* node to be collapsed */
array_t *sepset_node_array;
/* array of nodes in separating set */

{

  int i, j;
  int flag1; /* if ON, found a node which needs collapse */
  int flag2; /* if ON, this fanin is in separating set */
  node_t *sink_save;
  node_t *node;
  node_t *fanin;

  /* Collapse the nodes */
  /*--------------------*/
  for (;;) {
    sink_save = node_dup(sink);
    flag1 = OFF;
    if (XLN_DEBUG) {
      (void)printf("( ");
    }
    foreach_fanin(sink_save, i, fanin) {
      if (XLN_DEBUG) {
        (void)printf("%s ", node_long_name(fanin));
      }
      /* Check if this fanin node is in separating set */
      /*-----------------------------------------------*/
      for (flag2 = OFF, j = 0; j < array_n(sepset_node_array); j++) {
        node = array_fetch(node_t *, sepset_node_array, j);
        if (strcmp(node_long_name(fanin), node_long_name(node)) == 0) {
          flag2 = ON;
          break;
        }
      }
      if (flag2 == OFF) {
        flag1 = ON;
        (void)node_collapse(sink, fanin);
      }
    }
    if (XLN_DEBUG) {
      (void)printf(") --> ");
    }
    node_free(sink_save);
    if (flag1 == OFF) {
      break;
    }
  }
  if (XLN_DEBUG) {
    (void)printf("end");
  }
  return (sink);
}

/*
 *  partial_collapse()
 *	Collapse some of the network and reorganize it according to the result
 *	of the technology mapping
 */

network_t *partial_collapse(sink_array, sepset_array)

    array_t *sink_array; /* array of sink nodes for all the matchings
                          * in the solution */
array_t *sepset_array;   /* array of pointers to arrays of separating
                          * sets for all the matchings in the solution */

{

  array_t *sepset_node_array;
  /* array of nodes in the separating set */
  array_t *collapsed_node_array;
  /* array of collapsed nodes */
  array_t *del_node_array;     /* array of nodes to be deleted */
  /* array_t	*dfs_array; */ /* array of sorted nodes */
  int i;
  /* int      j; */
  /* int		flag; */
  node_t *sink;
  node_t *col_node; /* collapsed node */

  /* Initialization */
  /*----------------*/
  collapsed_node_array = array_alloc(node_t *, 0);
  del_node_array = array_alloc(node_t *, 0);

  /* For each sink node in the solution, collapse the node */
  /*-------------------------------------------------------*/
  for (i = 0; i < array_n(sink_array); i++) {
    sink = array_fetch(node_t *, sink_array, i);
    if (XLN_DEBUG) {
      (void)printf("Node %s : ", node_long_name(sink));
    }
    sepset_node_array = array_fetch(array_t *, sepset_array, i);

    col_node = partial_collapse_node(node_dup(sink), sepset_node_array);
    if (XLN_DEBUG) {
      (void)printf("\n");
    }
    array_insert_last(node_t *, collapsed_node_array, col_node);
  }

  /* Replace the sink nodes with the collapsed nodes */
  /*-------------------------------------------------*/
  for (i = 0; i < array_n(sink_array); i++) {
    sink = array_fetch(node_t *, sink_array, i);
    col_node = array_fetch(node_t *, collapsed_node_array, i);
    node_replace(sink, col_node);
  }

  /* Remove the nodes that are not sinks of the matching */
  /*-----------------------------------------------------*/
  /*
  dfs_array = network_dfs_from_input(pld_nishiza_network);
  for (i = 0; i < array_n(dfs_array); ++i) {

      node = array_fetch(node_t *, dfs_array, i);
      if (node->type != INTERNAL) {
          continue;
      }
      for(flag = OFF, j = 0; j < array_n(sink_array); j++) {
          sink = array_fetch(node_t *, sink_array, j);
          if (sink == node) {
              flag = ON;
              break;
          }
      }
      if (flag == OFF) {
          array_insert_last(node_t *, del_node_array, node);
      }
  }
  for (i = 0; i < array_n(del_node_array); ++i) {
      node = array_fetch(node_t *, del_node_array, i);
      network_delete_node(pld_nishiza_network, node);
  }
  */
  /* Termination */
  /*-------------*/
  array_free(collapsed_node_array);
  /*
  array_free(dfs_array);
  */
  array_free(del_node_array);
}

/*
 extern void partition_network();
 *  partition_network()
 *	Partition the given network into universal logic modules
 */

void partition_network(network_in, n, bincov_heuristics)

    network_t *network_in;
int n;
int bincov_heuristics;

{

  lsGen gen, gen2; /* generator */
  node_t *node;
  node_t *fanout;
  array_t *match_sink_array;
  /* array of sink nodes for all the matchings
   * obtained so far */
  array_t *match_sepset_array;
  /* array of pointers to arrays of separating
   * sets for all the matchings obtained so far */
  array_t *sink_array;   /* array of sink nodes for the matching
                          * for the current sink */
  array_t *sepset_array; /* array of pointers to arrays of separating
                          * sets for matchings for the current sink */
  array_t *sol_sink_array;
  /* array of sink nodes for all the matchings
   * in the solution */
  array_t *sol_sepset_array;
  /* array of pointers to arrays of separating
   * sets for all the matchings in the solution */
  char buf[BUFSIZE];
  sm_row *solution;    /* solution of the binate covering problem */
  sm_element *element; /* element of sparse matrix */
  sm_matrix *matrix;   /* sparse matrix for binate covering */
  int idx2;
  int j;
  int *weight; /* weight array for binate covering problem */
  int basic_num_rows;
  node_t *sink, *pre_sink;
  array_t *fetched_sepset;
  FILE *binFile;
  char bin_file[100];
  sm_row *sm_mat_bin_minimum_cover_my();
  int sfd;

  ULM_INPUT_SIZE = n;

  foreach_node(network_in, gen, node) {
    if (node_num_fanin(node) > ULM_INPUT_SIZE) {
      (void)printf("Error: The network has a node with > %d fanins\n",
                   ULM_INPUT_SIZE);
      (void)printf("Run xl_imp (xl_ao) -n %d to decompose these nodes\n",
                   ULM_INPUT_SIZE);
      (void)lsFinish(gen);
      return;
    }
  }

  /* Initialization */
  /*----------------*/
  pld_nishiza_network = network_in;

  match_sink_array = array_alloc(node_t *, 0);
  match_sepset_array = array_alloc(array_t *, 0);
  sol_sink_array = array_alloc(node_t *, 0);
  sol_sepset_array = array_alloc(array_t *, 0);
  pld_nishiza_graph = mf_alloc_graph();
  pld_nishiza_hash_table = nodeindex_alloc();
  matrix = sm_alloc();

  /* Form a hash table from a node pointer to a row index */
  /*------------------------------------------------------*/
  foreach_node(pld_nishiza_network, gen, node) {
    if (node->type == PRIMARY_OUTPUT) {
      continue;
    }
    (void)nodeindex_insert(pld_nishiza_hash_table, node);
  }

  construct_maxflow_network();

  /* Obtain matchings for each intermediate nodes */
  /*----------------------------------------------*/
  foreach_node(pld_nishiza_network, gen, node) {
    if (node->type != INTERNAL) {
      continue;
    }
    /* Watch for the Vdd and GND node */
    if (node_num_fanin(node) == 0) {
      (void)printf("%s has 0 fanin. Skipping.\n\n", node_long_name(node));
      continue;
    }
    sink_array = array_alloc(node_t *, 0);
    sepset_array = array_alloc(array_t *, 0);
    (void)sprintf(buf, "%s_bottom", node_long_name(node));
    mf_change_node_type(pld_nishiza_graph, mf_get_node(pld_nishiza_graph, buf),
                        2);
    if (XLN_DEBUG) {
      (void)printf("Generate matching for %s", node_long_name(node));
    }
    foreach_fanout(node, gen2, fanout) {
      if (fanout->type == PRIMARY_OUTPUT) {
        if (XLN_DEBUG) {
          (void)printf(" (PO: %s)", node_long_name(fanout));
        }
        (void)lsFinish(gen2);
        break;
      }
    }
    if (XLN_DEBUG) {
      (void)printf("\n");
    }
    generate_matching(sink_array, sepset_array, array_n(match_sink_array));
    initialize_edge_capacity();

    mf_change_node_type(pld_nishiza_graph, mf_get_node(pld_nishiza_graph, buf),
                        0);
    array_append(match_sink_array, sink_array);
    array_append(match_sepset_array, sepset_array);

    array_free(sink_array);
    array_free(sepset_array);
  }

  weight = ALLOC(int, 2 * array_n(match_sink_array));
  form_binate_matrix(network_in, matrix, weight, match_sink_array,
                     match_sepset_array, &basic_num_rows);
  if (XLN_DEBUG) {
    (void)sprintf(bin_file, "/usr/tmp/xln_merge.bin_matrix.XXXXXX");
    sfd = mkstemp(bin_file);
    if (sfd != -1) {
      binFile = fdopen(sfd, "w");
      if (binFile != NULL) {
        sm_print(binFile, matrix);
        (void)fclose(binFile);
      }
      close(sfd);
    }
  }
  if (bincov_heuristics != 4) {
    solution =
        sm_mat_bin_minimum_cover_greedy(matrix, weight, bincov_heuristics);
  } else {
    solution = sm_mat_bin_minimum_cover_my(matrix, weight, bincov_heuristics,
                                           basic_num_rows);
  }
  FREE(weight);
  /* Extract the matchings and separating sets belonging to the solution */
  /*---------------------------------------------------------------------*/
  if (XLN_DEBUG) {
    (void)printf("\nCover solution\n");
  }
  for (pre_sink = NULL, element = solution->first_col; element != NULL;
       element = element->next_col) {
    if (element->col_num % 2 == 1) {
      if (XLN_DEBUG) {
        (void)printf("Cover(%d)'\n", (element->col_num - 1) / 2);
      }
      continue;
    }
    idx2 = element->col_num / 2;
    sink = array_fetch(node_t *, match_sink_array, idx2);

    /* If this matching implements the same node again, discard it */
    /*-------------------------------------------------------------*/
    if (pre_sink == sink) {
      continue;
    }
    fetched_sepset = array_fetch(array_t *, match_sepset_array, idx2);
    if (XLN_DEBUG) {
      (void)printf("Cover(%d) = %s(%d) | ", idx2, node_long_name(sink),
                   nodeindex_indexof(pld_nishiza_hash_table, sink));
      print_array(fetched_sepset, pld_nishiza_hash_table);
    }

    array_insert_last(node_t *, sol_sink_array, sink);
    array_insert_last(array_t *, sol_sepset_array, fetched_sepset);
    pre_sink = sink;
  }
  if (XLN_DEBUG) {
    (void)printf("\nCollapsing the nodes ...\n");
  }
  partial_collapse(sol_sink_array, sol_sepset_array);

  /* Termination */
  /*-------------*/
  array_free(match_sink_array);
  array_free(sol_sink_array);
  for (j = array_n(match_sepset_array) - 1; j >= 0; j--) {
    array_free(array_fetch(array_t *, match_sepset_array, j));
  }
  array_free(match_sepset_array);
  array_free(sol_sepset_array);
  mf_free_graph(pld_nishiza_graph);
  nodeindex_free(pld_nishiza_hash_table);
  sm_free(matrix);
  sm_row_free(solution);

  /* to delete those nodes that do not fanout anywhere */
  /*---------------------------------------------------*/
  (void)network_sweep(pld_nishiza_network);
  if (network_check(pld_nishiza_network))
    ;
  else
    (void)printf(" %s\n", error_string());
}

sm_row *sm_mat_bin_minimum_cover_greedy(matrix, weight,
                                        bincov_heuristics) sm_matrix *matrix;
int *weight;
int bincov_heuristics;

{
  sm_row *solution;
  sm_row *sm_generate_row();
  sm_matrix *matrix_dup;

  if (bincov_heuristics <= 1)
    return sm_mat_bin_minimum_cover(matrix, weight, bincov_heuristics, 0, 0,
                                    6176, NULL);

  /* if bincov_heuristic is 3 (default) => if num_cols <= 40 => go for the exact
     solution. else apply greedy heuristic.(-h 2) */
  /*------------------------------------------------------------------------------------*/
  if ((bincov_heuristics == 3) && (matrix->ncols <= 40))
    return sm_mat_bin_minimum_cover(matrix, weight, 0, 0, 0, 6176, NULL);

  /* Else apply a simple heuristic for the cover.
     Find the column that covers the maximum number of rows,
     select it in the cover, change the matrix,ie,
     delete the rows that are covered by this column,
     delete the column corresponding to the matchbar. */
  /*--------------------------------------------------*/

  solution_array = array_alloc(int, 0);
  matrix_dup = sm_dup(matrix);

  /* select columns; selected column nums returned in solution_array*/
  /* ---------------------------------------------------------------*/
  select_column_and_update_matrix(matrix_dup);
  solution = sm_generate_row(solution_array);

  sm_free(matrix_dup);
  array_free(solution_array);

  return solution;
}

/****************************************************************************
  Get the column with the maximum number of elements in the matrix.
******************************************************************************/

sm_col *get_column_with_max_elements(matrix) sm_matrix *matrix;
{
  sm_col *pcol;
  sm_col *best_col;
  int MAX_ELEMENTS;
  int colnum;
  int COLUMNS_REMAIN; /* for the condition when only match bar columns remain */

  MAX_ELEMENTS = 0;
  COLUMNS_REMAIN = 0;

  for (pcol = matrix->first_col; pcol != 0; pcol = pcol->next_col) {
    colnum = pcol->col_num;
    if (colnum % 2 == 1)
      continue;
    COLUMNS_REMAIN = 1;
    if (pcol->length < MAX_ELEMENTS)
      continue;
    best_col = pcol;
    MAX_ELEMENTS = pcol->length;
  }
  if (COLUMNS_REMAIN == 0) {
    (void)printf(
        "get_column_with_max_elements(): no valid columns remain ->error\n");
    exit(1);
  }

  if (XLN_DEBUG)
    (void)printf("max_elements in %d (%d)\n", best_col->col_num,
                 best_col->length);
  return best_col;
}

/************************************************************************************
   It selects one column at a time, deletes the covered rows and then
recursively calls itself. Finally a covre of the matrix is obtained. The matrix
changes everytime. Final answer is in the array solution_array. Contains the
column numbers of all the selected columns in the cover.
************************************************************************************/
select_column_and_update_matrix(matrix) sm_matrix *matrix;
{

  array_t *row_array; /* matrix for the rows every time */
  sm_col *best_col, *next_best_col;
  sm_col *get_column_with_max_elements();
  sm_row *prow;
  int best_colnum;
  int i;
  int num_covered_rows;
  int DELETE_FLAG; /* flag for checking if the best_col's complement needs
                      deletion*/

  if (XLN_DEBUG)
    (void)printf("select_col_and_up.. num_rows = %d, num_columns = %d\n",
                 matrix->nrows, matrix->ncols);

  if (matrix->ncols == 0 || matrix->nrows == 0)
    return;

  DELETE_FLAG = 0;
  row_array = array_alloc(sm_row *, 0);

  best_col = get_column_with_max_elements(matrix);
  best_colnum = best_col->col_num;

  /* the complement of the best_col, if present */
  /*--------------------------------------------*/
  next_best_col = best_col->next_col;
  if (next_best_col == 0)
    ;
  else {
    if (next_best_col->col_num == best_colnum + 1)
      DELETE_FLAG = 1;
  }

  if (XLN_DEBUG)
    (void)printf("best column = %d\n", best_colnum);

  /* remember the column number of the best column */
  /*-----------------------------------------------*/
  array_insert(int, solution_array, array_n(solution_array), best_colnum);

  /* find the rows covered by the best_col */
  /*---------------------------------------*/
  for (prow = matrix->first_row; prow != 0; prow = prow->next_row) {
    if (sm_find(matrix, prow->row_num, best_col->col_num) == 0)
      continue;
    array_insert(sm_row *, row_array, array_n(row_array), prow);
    if (XLN_DEBUG)
      (void)printf("inserting row %d in row_array\n", prow->row_num);
  }
  num_covered_rows = array_n(row_array);

  /* delete the column and its complement, if there in the matrix */
  /* actually no need to delete the column but still deleting.    */
  /*--------------------------------------------------------------*/
  sm_delcol(matrix, best_colnum);
  if (DELETE_FLAG)
    sm_delcol(matrix, next_best_col->col_num);

  /* delete the covered rows from the matrix. */
  /*------------------------------------------*/
  for (i = 0; i < num_covered_rows; i++) {
    prow = array_fetch(sm_row *, row_array, i);
    sm_delrow(matrix, prow->row_num);
  }
  array_free(row_array);

  if (XLN_DEBUG)
    sm_print(stdout, matrix);

  select_column_and_update_matrix(matrix);
}

/**********************************************************************************
  From the solution array, generate the sm_row which has an entry for each match
  in the binate cover.
***********************************************************************************/
sm_row *sm_generate_row(solution_array) array_t *solution_array;
{
  sm_row *solution; /* answer to the binate covering problem */
  int i, colnum;
  int num_solution; /* number of matches found by the heuristic */

  solution = sm_row_alloc();

  num_solution = array_n(solution_array);
  for (i = 0; i < num_solution; i++) {
    colnum = array_fetch(int, solution_array, i);
    (void)sm_row_insert(solution, colnum);
  }
  return solution;
}

/*---------------------------------------------------------------------------
  Form a matrix (matrix_basic) that is all matches + the first rows upto the
  basic_num_rows. Solve a unate covering problem on this matrix. All the
  columns selected are deleted, so are their complements. All the rows of
  matrix (original one) that are covered by these columns are also deleted.
  What to do on this small matrix?
----------------------------------------------------------------------------*/
sm_row *sm_mat_bin_minimum_cover_my(matrix, weight, bincov_heuristic,
                                    basic_num_rows) sm_matrix *matrix;
int *weight, bincov_heuristic, basic_num_rows;

{
  sm_matrix *matrix_basic, *matrix_nonbasic;
  sm_row *row, *row_solution_basic, *row_solution_nonbasic;
  sm_col *col, *odd_col;
  sm_element *p;
  array_t *col_array, *row_array;
  st_table *table;
  int i;

  /*
  if (bincov_heuristic != 4) {
      row = sm_mat_bin_minimum_cover_greedy(matrix, weight, bincov_heuristic);
      return row;
  }
  */ /* commented because want to try right now my heuristic */
  /* forming matrix_basic */
  /*----------------------*/
  matrix_basic = sm_alloc();
  matrix_nonbasic = sm_alloc();

  /* first add basic rows */
  /*----------------------*/
  for (row = matrix->first_row; row != 0; row = row->next_row) {
    if (row->row_num < basic_num_rows) {
      sm_copy_row(matrix_basic, row->row_num, row);
    } else {
      sm_copy_row(matrix_nonbasic, row->row_num, row);
    }
  }
  /* now delete odd columns */
  /*------------------------*/
  col_array = array_alloc(sm_col *, 0);
  for (col = matrix_basic->first_col; col != 0; col = col->next_col) {
    if (col->col_num % 2 == 1) {
      array_insert_last(sm_col *, col_array, col);
    }
  }
  for (i = 0; i < array_n(col_array); i++) {
    col = array_fetch(sm_col *, col_array, i);
    sm_delcol(matrix_basic, col->col_num);
  }
  array_free(col_array);

  /* solve a unate covering problem on matrix_basic - use a different flag here
   */
  /*----------------------------------------------------------------------------*/
  row_solution_basic =
      sm_minimum_cover(matrix_basic, NIL(int), bincov_heuristic, XLN_DEBUG);

  /* check that the elements are from even columns of matrix */
  /*---------------------------------------------------------*/
  assert(xln_seq_check_row_elements_even(row_solution_basic));

  /* form matrix for the rest of the operations */
  /*--------------------------------------------*/
  row_array = array_alloc(sm_row *, 0);
  for (row = matrix_nonbasic->first_row; row != 0; row = row->next_row) {
    if (sm_row_intersects(row, row_solution_basic)) {
      array_insert_last(sm_row *, row_array, row);
    }
  }
  for (i = 0; i < array_n(row_array); i++) {
    row = array_fetch(sm_row *, row_array, i);
    sm_delrow(matrix_nonbasic, row->row_num);
  }
  array_free(row_array);

  /* now delete columns that are in row_solution_basic and their complements */
  /*-------------------------------------------------------------------------*/
  col_array = array_alloc(sm_col *, 0);
  sm_foreach_row_element(row_solution_basic, p) {
    assert(col = sm_get_col(matrix, p->col_num));
    assert(col->col_num % 2 == 0);
    array_insert_last(sm_col *, col_array, col);
    odd_col = col->next_col;
    if (odd_col && (odd_col->col_num == col->col_num + 1)) {
      array_insert_last(sm_col *, col_array, col->next_col);
    }
  }
  sm_delete_cols_in_array(matrix_nonbasic, col_array);
  array_free(col_array);

  /* select greedily the columns from matrix_nonbasic */
  /*--------------------------------------------------*/
  table = st_init_table(st_numcmp, st_numhash);
  row_solution_nonbasic = sm_row_alloc();
  xln_seq_table_of_rows_covered_by_odd_columns(matrix_nonbasic, table);
  sm_mat_bin_minimum_cover_nonbasic(matrix_nonbasic, row_solution_nonbasic,
                                    table);
  st_free_table(table);

  /* combine the basic solution with the nonbasic solution */
  /*-------------------------------------------------------*/
  sm_foreach_row_element(row_solution_nonbasic, p) {
    sm_row_insert(row_solution_basic, p->col_num);
  }

  sm_row_free(row_solution_nonbasic);
  sm_free(matrix_basic);
  sm_free(matrix_nonbasic);

  return row_solution_basic;
}

/*------------------------------------------------------------------
  Returns 1 if all the elements of row have even column numbers.
-------------------------------------------------------------------*/
int xln_seq_check_row_elements_even(row) sm_row *row;
{
  sm_element *p;

  sm_foreach_row_element(row, p) {
    if (p->col_num % 2 == 1)
      return 0;
  }
  return 1;
}

/*--------------------------------------------------------------------
  If all the rows of the matrix can be covered by odd columns of the
  matrix, insert all the odd columns in the solution. Else
  find greedily an even column (with max score), add it to the current
  solution, delete the rows it covers and delete it too, along
  with its odd column. Recursively calls itself.
---------------------------------------------------------------------*/
sm_mat_bin_minimum_cover_nonbasic(matrix, row_solution,
                                  table) sm_matrix *matrix;
sm_row *row_solution;
st_table *table; /* initially stores the rows that have not been covered -
                  assumes that even columns presented in matrix are
                  not selected */
{
  sm_col *best_col, *sm_get_even_column_with_max_score(), *col, *odd_col;
  sm_element *p;
  char *value;

  /* check if all the rows of matrix can be covered by odd columns */
  /*---------------------------------------------------------------*/
  if (matrix->nrows == st_count(table)) {
    /* insert all the odd-column entries in the solution */
    /*---------------------------------------------------*/
    sm_foreach_col(matrix, col) {
      if (col->col_num % 2 == 1) {
        sm_row_insert(row_solution, col->col_num);
      }
    }
    return;
  }

  best_col = sm_get_even_column_with_max_score(matrix);
  if (!best_col)
    return;
  sm_row_insert(row_solution, best_col->col_num);
  odd_col = best_col->next_col;
  if (odd_col && (odd_col->col_num == best_col->col_num + 1)) {
    /* delete the rows from the table that were covered by odd_column */
    /*----------------------------------------------------------------*/
    sm_foreach_col_element(odd_col, p) {
      assert(st_delete_int(table, &(p->row_num), &value));
    }
  }
  /* delete the rows that this column covers */
  /*-----------------------------------------*/
  sm_delete_rows_covered_by_col(matrix, best_col);

  sm_delcol(matrix, best_col->col_num);
  if (odd_col && (odd_col->col_num == best_col->col_num + 1)) {
    sm_delcol(matrix, odd_col->col_num);
  }
  sm_mat_bin_minimum_cover_nonbasic(matrix, row_solution, table);
}

/*-----------------------------------------------------------------------
  Score of each even column is num_elements - num_elements of the next
  column. Return the column with max score.
-----------------------------------------------------------------------*/
sm_col *sm_get_even_column_with_max_score(matrix) sm_matrix *matrix;
{
  sm_col *pcol, *next_col, *best_col;
  int max_score, score;
  int colnum;

  best_col = NIL(sm_col);
  max_score = -MAXINT;

  for (pcol = matrix->first_col; pcol != 0; pcol = pcol->next_col) {
    colnum = pcol->col_num;
    if (colnum % 2 == 1)
      continue;
    /* next_col may not be the next_col of the original matrix */
    /*---------------------------------------------------------*/
    next_col = pcol->next_col;
    if (next_col && (next_col->col_num == colnum + 1)) {
      score = pcol->length - next_col->length;
    } else {
      score = pcol->length;
    }
    if (score < max_score)
      continue;
    best_col = pcol;
    max_score = score;
  }
  if (XLN_DEBUG) {
    (void)printf("max score of %d (%d)\n", best_col->col_num, max_score);
  }
  return best_col;
}

/*-------------------------------------------------------------
  Fills table with row numbers of all the rows covered by the
  odd numbered columns of matrix.
---------------------------------------------------------------*/
xln_seq_table_of_rows_covered_by_odd_columns(matrix, table) sm_matrix *matrix;
st_table *table;
{
  sm_col *col;
  sm_element *p;

  sm_foreach_col(matrix, col) {
    if (col->col_num % 2 == 1) {
      sm_foreach_col_element(col, p) {
        assert(!st_insert(table, (char *)p->row_num, (char *)p->row_num));
      }
    }
  }
}
