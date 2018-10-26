

/* March 30, 1992 - Added a heuristic to run merge without lindo.
                  - Added a routine that collapses nodes after merging is done.
                    This was to exploit the case when the network is 4-feasible.
*/

/*
 *  merge.c
 *  *******
 *
 *  Copyright (C) 1989  Yoshihito Nishizaki
 *	University of California, Berkeley
 *
 *  Author:			Advising professor:
 *	Yoshihito Nishizaki	    Alberto Sangiovanni-Vincentelli
 *
 *  --------------------------------------------------------------------------
 *
 *  This file contains routines for merging cells into one unit block
 *	by max cardinality matching method.
 *
 *  functions contained in this file:
 *	merge_node()
 *
 *
 *  Import descriptions:
 *	sis.h		: macros for misII
 *	ULM_int.h	: macros for universal logic module package
 *
 */

#include "sis.h"
#include "pld_int.h"




/*
 *  merge_node()
 *
 *  DESCRIPTION
 *  -----------
 *	Merge two (or more) intermediate nodes in the graph into one ULM node.
 *	Formulate the problem as an integer programming problem
 *	and solve it by Lindo linear programming package.
 *
 *	This problem should be formulated as a max cardinality matching of a
 *	general graph, if the # nodes to be merged is 2.
 *	But for the generality of the routine, we use integer programming.
 *	This does not depend on the number of nodes to be merged.
 *
 *  RETURNS
 *  -------
 *	None
 *
 *  Arguments:
 *  ---------
 *	network		: pointer to a network
 *
 *  Local variables:
 *  ---------------
 *	gen		: generator
 *	cand_node_array	: array of candidate node for merging
 *	fanin_array	: array of pointers to an array of fanin nodes a node
 *	fanin_nodes	: pointer to an array of fanin nodes
 *	fanin1		: pointer to an array of fanin nodes
 *	fanin2		: pointer to an array of fanin nodes
 *	node		: pointer to a node in the network
 *	fanin		: pointer to a fanin node of a node in the network
 *	node1		: pointer to a node in the network
 *	node2		: pointer to a node in the network
 *	intsec		: size of intersection of two sets
 *	union		: size of union of two sets
 *	match1_array	: array of pointers to left side nodes in the mathing
 *	match2_array	: array of pointers to right side nodes in the mathing
 *
 *  Possible errors:
 *  ---------------
 *
 *  SIDE-EFFECTS
 *  ------------
 *	None
 *
 *  GLOBAL VARIABLES
 *  ----------------
 *	None
 *
 */
 static sm_row *xln_merge_find_neighbor_of_row1_with_minimum_neighbors();


extern void merge_node();
void
merge_node(network, MAX_FANIN, MAX_COMMON_FANIN, MAX_UNION_FANIN, filename, verbose, use_lindo,
           match1_array, match2_array)

network_t	*network;
int  		MAX_FANIN, MAX_COMMON_FANIN, MAX_UNION_FANIN;
char 		*filename;
int 		verbose, use_lindo;
array_t	        *match1_array, *match2_array;
{

    lsGen	gen;
    array_t	*cand_node_array;
    array_t	*fanin_array;
    array_t	*fanin_nodes;
    array_t	*fanin1, *fanin2;
    node_t	*node, *fo;
    node_t	*fanin;
    node_t	*node1, *node2;
    int		i, j;
    int		num_intsec;
    int		num_union;
    sm_matrix	*coeff;
    int		col;
    int		num_match;
    char	*merge_file;
    char 	*buf;
    FILE	*mergeFile, *outputFile;
    char 	*lindopathname, *lindo_in, *lindo_out;
    int		sfd;


  /* search for lindo in the path name. If not found, return */
  /*---------------------------------------------------------*/
  if (use_lindo) {
      lindopathname = util_path_search("lindo");
      if (lindopathname == NULL) {
          (void) printf("YOU DO NOT HAVE LINDO INTEGER PROGRAMMING PACKAGE\n");
          (void) printf("\tIN YOUR PATH.\n\n");
          (void) printf("PLEASE OBTAIN LINDO FROM\n\n");
          (void) printf("\tThe Scientific Press, 540 University Ave.\n");
          (void) printf("\tPalo Alto, CA 94301, USA\n");
          (void) printf("\tTEL : (415) 322-5221\n");
          (void) printf("USING A HEURISTIC INSTEAD.....\n");
          use_lindo = 0;
      } else {
          FREE(lindopathname);
      }
  }
    /* Initialization */
    /*----------------*/
    cand_node_array = array_alloc(node_t *, 0);
    fanin_array = array_alloc(array_t *, 0);
    coeff = sm_alloc();

    /* Extract the intermediate nodes with fanins <= MAX_FANIN */
    /*---------------------------------------------------------*/
    foreach_node(network, gen, node) {
	if (node->type != INTERNAL) {
	    continue;
	}
	if (node_num_fanin(node) > MAX_FANIN) {
	    continue;
	}
	array_insert_last(node_t *, cand_node_array, node);
	fanin_nodes = array_alloc(char *, 0);
	array_insert_last(array_t *, fanin_array, fanin_nodes);

	/* Save each fanin of this node and sort them */
	/*--------------------------------------------*/
	foreach_fanin(node, i, fanin) {
	    array_insert_last(char *, fanin_nodes, (char *)fanin);
	}
	array_sort(fanin_nodes, comp_ptr);
    }

    /* Check every mergeable combination of intermediate nodes */
    /*---------------------------------------------------------*/
    for(i = 0, col = 0; i < array_n(cand_node_array); i++) {
	node1 = array_fetch(node_t *, cand_node_array, i);
	fanin1 = array_fetch(array_t *, fanin_array, i);

	for(j = i + 1; j < array_n(cand_node_array); j++) {
	    node2 = array_fetch(node_t *, cand_node_array, j);
	    fanin2 = array_fetch(array_t *, fanin_array, j);

	    count_intsec_union(fanin1, fanin2, &num_intsec, &num_union);
	    if (num_intsec <= MAX_COMMON_FANIN &&
			num_union <= MAX_UNION_FANIN) {
		(void) sm_insert(coeff, i, col);
		(void) sm_insert(coeff, j, col);
		++col;

		/*
		print_fanin(node1);
		print_fanin(node2);
		(void) printf("Node %s and node %s are mergeable\n",
			node_long_name(node1), node_long_name(node2));
		 */
	    }
	}
    }
    if(XLN_DEBUG){
      merge_file = ALLOC(char, 100);
      (void)sprintf(merge_file, "/usr/tmp/xln.merge.merge_matrix.XXXXXX");
      sfd = mkstemp(merge_file);
      FREE(merge_file);
      if(sfd == -1) {
	(void) fprintf(siserr, "cannot open temp file\n");
      }
      else {
	mergeFile = fdopen(sfd, "w");
	if(mergeFile != NULL) {
	  sm_print(mergeFile, coeff);
	  (void) fclose(mergeFile);
	} else {
	  (void) fprintf(siserr, "No space for mergeFile to be written \n");
	}
      }
    }

    outputFile = fopen(filename, "w");

    /* either lindo not available or user did not choose lindo option - use a heuristic to solve
       maximum cardinality matching problem                                                     */
    /*------------------------------------------------------------------------------------------*/
    if (!use_lindo) {
        xln_merge_nodes_without_lindo(coeff, cand_node_array, match1_array, match2_array);
        num_match = array_n(match1_array);

    } else {

        /* Solve max cardinality matching problem by lindo to get the best merge */
        /*-----------------------------------------------------------------------*/
        lindo_in =  formulate_Lindo(coeff);
        lindo_out = ALLOC(char, 1000);
        buf       = ALLOC(char, 500);
        (void) sprintf(lindo_out, "/usr/tmp/merge.lindo.out.XXXXXX");
	sfd = mkstemp(lindo_out);
	if(sfd != -1) {
	  close(sfd);
	  (void) sprintf(buf, "lindo < %s > %s", lindo_in, lindo_out);
	  (void) system(buf);

	  /* Print the results */
	  /*-------------------*/
	  num_match = get_Lindo_result(cand_node_array, coeff,
				       match1_array, match2_array, lindo_out, outputFile);
	}
	FREE(lindo_in);
	FREE(buf);
        FREE(lindo_out);
    }
    if (verbose) (void) printf("Total number of matchings = %d\n\n", num_match);

    (void) fprintf(outputFile, "Merging two CLB's into one CLB\n");
    if (verbose) (void) printf("Merging two CLB's into one CLB\n");
    for(i = 0; i < array_n(match1_array); i++) {
	node1 = array_fetch(node_t *, match1_array, i);
	node2 = array_fetch(node_t *, match2_array, i);
        if (io_po_fanout_count(node1, &fo) == 1) {
            node1 = fo;
        }
        if (io_po_fanout_count(node2, &fo) == 1) {
            node2 = fo;
        }
	(void) fprintf(outputFile, "Merge node %s and %s\n",
			  node_long_name(node1), node_long_name(node2));
	if (verbose) (void) printf("Merge node %s and %s\n",
			  	    node_long_name(node1), node_long_name(node2));
    }
    (void) fprintf(outputFile, "\n# of CLB's = %d\n", network_num_internal(network) - num_match);
    if (verbose) (void) printf("\n# of CLB's = %d\n", network_num_internal(network) - num_match);
    (void) fclose (outputFile);

    /* Termination */
    /*-------------*/
    array_free(cand_node_array);
    for(j=array_n(fanin_array)-1; j>=0; j--){
	array_free(array_fetch(array_t *, fanin_array, j));
    }
    array_free(fanin_array);
    sm_free(coeff);

}


/*----------------------------------------------------------------------------------------------------
  An alternate to lindo option. Uses greedy merging. A node with minimum mergeable nodes is picked
  first. It is combined with a neighbor node that has minimum number of neighbors. The pairs of
  matches are stored in corresponding entries of match1_array and match2_array.
-----------------------------------------------------------------------------------------------------*/
xln_merge_nodes_without_lindo(coeff, cand_node_array, match1_array, match2_array)
  sm_matrix *coeff;
  array_t *cand_node_array, *match1_array, *match2_array;
{
  node_t *n1, *n2;
  sm_row *row1, *row2;

  while (TRUE) {
      row1 = sm_shortest_row(coeff);
      if (row1 == NIL (sm_row)) return;
      n1 = array_fetch(node_t *, cand_node_array, row1->row_num);
      row2 = xln_merge_find_neighbor_of_row1_with_minimum_neighbors(row1, coeff);
      n2 = array_fetch(node_t *, cand_node_array, row2->row_num);
      array_insert_last(node_t *, match1_array, n1);
      array_insert_last(node_t *, match2_array, n2);
      xln_merge_update_neighbor_info(coeff, row1, row2);
  }
}

sm_row *
sm_shortest_row(A)
  sm_matrix *A;
{
  register sm_row *short_row, *prow;
  register int min_length;

  min_length = MAXINT;
  short_row = NIL(sm_row);
  for(prow = A->first_row; prow != 0; prow = prow->next_row) {
      if (prow->length < min_length) {
          min_length = prow->length;
          short_row = prow;
      }
  }
  return short_row;
}

static sm_row *
xln_merge_find_neighbor_of_row1_with_minimum_neighbors(row1, coeff)
  sm_row *row1;
  sm_matrix *coeff;
{
  sm_row *row, *row2;
  sm_element *p;
  int min_neighbr;

  min_neighbr = MAXINT;
  row2 = NIL (sm_row);

  /* each column has two elements */
  /*------------------------------*/
  sm_foreach_row_element(row1, p) {
      if (p->prev_row == NIL (sm_element)) {
          row = sm_get_row(coeff, (p->next_row)->row_num);
      } else {
          row = sm_get_row(coeff, (p->prev_row)->row_num);
      }
      if (row->length < min_neighbr) {
          min_neighbr = row->length;
          row2 = row;
      }
  }
  assert (row2);
  return row2;
}

xln_merge_update_neighbor_info(coeff, row1, row2)
  sm_matrix *coeff;
  sm_row *row1, *row2;
{
  sm_delete_cols_covered_by_row(coeff, row1);
  sm_delete_cols_covered_by_row(coeff, row2);
  sm_delrow(coeff, row1->row_num);
  sm_delrow(coeff, row2->row_num);
}

/*-------------------------------------------------------------------------------------
  There may be cases where after merge, a node can be collapsed into its fanouts without
  destroying the feasibility of the network. Returns the rest of the nodes of the network
  that are not present in match1_array or match2_array.
---------------------------------------------------------------------------------------*/
st_table *
xln_collapse_nodes_after_merge(network, match1_array, match2_array, support, verbose)
  network_t *network;
  array_t *match1_array, *match2_array;
  int support, verbose;
{
  st_table *table;
  array_t *nodevec; /* stores the nodes that are collapsed, so that they can be deleted later */
  int i, OK_to_collapse, is_collapsed;
  node_t *node;
  st_generator *stgen;
  char *dummy, *dummy1;

  table = pld_insert_intermediate_nodes_in_table(network);
  pld_delete_array_nodes_from_table(table, match1_array);
  pld_delete_array_nodes_from_table(table, match2_array);
  nodevec = array_alloc(node_t *, 0);
  st_foreach_item(table, stgen, (char **)&node, &dummy) {
      OK_to_collapse = xln_merge_are_fanouts_in_table(table, node);
      if (OK_to_collapse) {
          is_collapsed = xln_do_trivial_collapse_node_without_moving(node, support);
          if (is_collapsed > 0) {
              array_insert_last(node_t *, nodevec, node);
          }
      }
  }
  for (i = 0; i < array_n(nodevec); i++) {
      node = array_fetch(node_t *, nodevec, i);
      assert(st_delete(table, (char **) &node, &dummy1));
      network_delete_node(network, node);
  }
  if (verbose) {
      (void) printf("deleted %d nodes in the final collapse, final CLBs = %d\n", array_n(nodevec),
                    network_num_internal(network) - array_n(match1_array));
  }
  array_free(nodevec);
  return(table);
}

int
xln_merge_are_fanouts_in_table(table, node)
  st_table *table;
  node_t *node;
{
  lsGen gen;
  node_t *fanout;

  if (node->type == PRIMARY_INPUT || node->type == PRIMARY_OUTPUT) return 0;
  foreach_fanout(node, gen, fanout) {
      if (!st_is_member(table, (char *) fanout)) return 0;
  }
  return 1;
}
