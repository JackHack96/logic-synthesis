#ifdef SIS
#include "sis.h"
#include "astg_int.h"
#include "si_int.h"
#include <math.h>

#define ZERO_CODE "0"
#define ONE_CODE "1"

static network_t *stg_to_net();
static int extract_and_solve_cons();
static void encode_stg();

static char file[] = "/tmp/SISXXXXXX";

/* Tracey's single transition time (STT) state assignment algorithm */
network_t *
astg_encode(stg, astg, heuristic)
graph_t *stg;
astg_graph *astg;
int heuristic;
{
  network_t *network;
  int initial;
  lsGen gen;
  vertex_t *state;
  FILE *fp;

  if (stg == NIL(graph_t)) {
    return NIL(network_t);
  }

  switch(stg_get_num_states(stg)) {
  case 0:
    fail("STG with no states?");
    /* NOTREACHED */
    break; 
  case 1:
    break;
  case 2:
    initial = 1;
    stg_foreach_state(stg, gen, state) {
      if (initial) {
		stg_set_state_encoding(state, ZERO_CODE);
		initial = 0;
      } else {
		stg_set_state_encoding(state, ONE_CODE);
		  }
    }
    break;
  default:
    if (!extract_and_solve_cons(stg, heuristic)) return NIL(network_t);
    break;
  }

  return NIL(network_t);
 /* return stg_to_net(stg, astg); */
}


static void
dic_add (l1, l2, r1, r2, n, dic, list)
int l1, l2, r1, r2, n;
pset dic;
dic_family_t *list;
{
	set_clear (lhs_dic(dic), n);
	if (l1 >= 0) {
		set_insert (lhs_dic(dic), l1);
	}
	if (l2 >= 0) {
		set_insert (lhs_dic(dic), l2);
	}

	set_clear (rhs_dic(dic), n);
	if (r1 >= 0) {
		set_insert (rhs_dic(dic), r1);
	}
	if (r2 >= 0) {
		set_insert (rhs_dic(dic), r2);
	}
	dic_family_add_irred(list, dic);
}

/* extract dichotomoy constraints from an STG */
static int
extract_and_solve_cons(stg, heuristic)
graph_t *stg;
int heuristic;
{
  edge_t *edge;
  vertex_t *s_i, *s_j, *to_state, *from_state, *s_m, *s_k;
  lsGen gen;
  st_generator *sgen1, *sgen2, sgen3;
  char *input_label;
  int x, y, n, j, m, i, k;
  st_table *input_label_table; /* hashed by edge labels */
  st_table *next_state_table; /* hashed by next state pointers */
  array_t *dic_array, *dic_array1; /* states with the same next states */
  st_table *index;
  dic_family_t *seed_list, *dic_list, *prime_list;
  pset dic;
  sm_matrix *table;
  sm_row *cover;

  index = st_init_table(st_ptrcmp, st_ptrhash);
  x = 0;
  stg_foreach_state(stg, gen, s_j) {
    st_insert (index, (char *) s_j, (char *) x);
    x++;
  }
  input_label_table = st_init_table(strcmp, st_strhash);
  stg_foreach_transition(stg, gen, edge) {
    input_label = stg_edge_input_string(edge);
    to_state = stg_edge_to_state(edge);
    from_state = stg_edge_from_state(edge);
    if (!st_lookup(input_label_table, input_label, 
		(char**) &next_state_table)) {

      dic_array = array_alloc(vertex_t *, 0);
      array_insert_last(vertex_t *, dic_array, from_state);

      next_state_table = st_init_table(st_ptrcmp, st_ptrhash);
      assert (! st_insert(next_state_table, (char *) to_state, (char *) dic_array));

      st_insert(input_label_table, input_label, (char*) next_state_table);

    } else {
      if (! st_lookup(next_state_table, (char *) to_state, 
		(char**) &dic_array)) {

		dic_array = array_alloc(vertex_t *, 0);
		array_insert_last(vertex_t *, dic_array, from_state);
		assert(! st_insert(next_state_table, (char *) to_state, (char *) dic_array));

      } else {
		array_insert_last(vertex_t *, dic_array, from_state);
      }
    }
  }
  
  /* fill out the seed data structure; terminology from Unger's book */
  n = stg_get_num_states(stg);
  dic = dic_new(n);
  seed_list = dic_family_alloc(ALLOCSIZE, n);
  st_foreach_item(input_label_table, sgen1, &input_label, 
	(char**) &next_state_table) {
      if (g_debug > 1) {
		  (void)fprintf(sisout, "input %s: ", input_label);
      }
      if (st_count (next_state_table) < 2) {
		  if (g_debug > 1) {
			(void)fprintf(sisout, "skipped (only one entry)\n");
		  }
		  continue;
      }
      st_foreach_item(next_state_table, sgen2, (char**) &s_j, 
		(char **) &dic_array) {
		if (g_debug > 1) {
			(void)fprintf(sisout, "[%s] ", stg_get_state_name(s_j));
		}
		for (x = array_n(dic_array) - 1; x >= 0; x--) {
		  s_i = array_fetch(vertex_t *, dic_array, x);
		  if (g_debug > 1) {
			  (void)fprintf(sisout, "%s ", stg_get_state_name(s_i));
		  }
		}
		st_lookup (index, (char *) s_j, (char **) &j);
		sgen3 = *sgen2;
		/* this ensures j != m */
		while (st_gen (sgen2, (char **) &s_m, (char **) &dic_array1)) {
			st_lookup (index, (char *) s_m, (char **) &m);
			for (x = array_n(dic_array) - 1; x >= 0; x--) {
				s_i = array_fetch(vertex_t *, dic_array, x);
				st_lookup (index, (char *) s_i, (char **) &i);
				for (y = array_n(dic_array1) - 1; y >= 0; y--) {
				  s_k = array_fetch(vertex_t *, dic_array1, y);
				  st_lookup (index, (char *) s_k, (char **) &k);
				  /* now test i != j or k != m */
				  if (i != j || k != m) {
					/* [j, i; m] */
					dic_add (j, i, m, -1, n, dic, seed_list);

					/* [j, i; k] */
					dic_add (j, i, -1, k, n, dic, seed_list);

					/* [j; m, k] */
					dic_add (j, -1, m, k, n, dic, seed_list);

					/* [i; m, k] */
					dic_add (-1, i, m, k, n, dic, seed_list);

					/* [m, k; j] */
					dic_add (m, k, j, -1, n, dic, seed_list);

					/* [m, k; i] */
					dic_add (m, k, -1, i, n, dic, seed_list);

					/* [m; j, i] */
					dic_add (m, -1, j, i, n, dic, seed_list);

					/* [k; j, i] */
					dic_add (-1, k, j, i, n, dic, seed_list);
				  }
				}
			}
		}
		*sgen2 = sgen3;
		array_free(dic_array);
		if (g_debug > 1) {
			(void)fprintf(sisout, ";");
		}
      }
      if (g_debug > 1) {
		  (void)fprintf(sisout, "\n");
      }
      st_free_table(next_state_table);
  }
  dic_free(dic);
  st_free_table(input_label_table);
  if (g_debug > 2) {
	(void)fprintf(sisout, "There are %d initial seeds\n", seed_list->dcount);
	dic_family_print (seed_list);
  }

  /* now solve them */
  dic_list = reduce_seeds(gen_uniq(seed_list));
  if (g_debug > 2) {
	(void)fprintf(sisout, "There are %d reduced seeds\n", dic_list->dcount);
	dic_family_print (dic_list);
  }

  prime_list = gen_eqn(dic_list, LIMIT);
  table = dic_to_sm(prime_list, dic_list);
  cover = sm_minimum_cover(table, NIL(int), heuristic, 0);
  if (g_debug > 2) {
	print_min_cover(table, cover, prime_list);
  }

  encode_stg(stg, cover, table, prime_list);
 
  (void) dic_family_free(seed_list);
  (void) dic_family_free(dic_list);
  (void) dic_family_free(prime_list);
  sm_row_free(cover);
  sm_free(table);
  return 1;
}

static void
encode_stg(stg, cover, table, prime_list)
graph_t *stg;
sm_row *cover;
sm_matrix *table;
dic_family_t *prime_list;
{
  int i, j, state_bits;
  char **codes;
  sm_element *elem;
  sm_col *col;
  pset p;
  vertex_t *state;
  lsGen gen;
  int num_states = stg_get_num_states(stg);

  assert(prime_list->dset_elem == num_states);
  
  state_bits =  0;
  sm_foreach_row_element(cover, elem) {
    state_bits ++;
  }

  codes = ALLOC(char *, num_states);
  for (i = 0; i < num_states; i++) {
    codes[i] = ALLOC(char, state_bits+1);
    codes[i][state_bits] = '\0';
  }
  
  j = 0;
  sm_foreach_row_element(cover, elem) {
    col = sm_get_col(table, elem->col_num);
    p = sm_get(pset, col);
    for (i = 0; i < num_states; i++) {
      codes[i][j] = is_in_set(p, i) ? '1' : '0';
    }
    j++;
  }
  
  i = 0;
  stg_foreach_state(stg, gen, state) {
    if (g_debug) (void)fprintf(sisout, "state %s = %s\n", 
			       stg_get_state_name(state), codes[i]);
    stg_set_state_encoding(state, codes[i]);
    FREE(codes[i]);
    i++;
  }
  FREE(codes);
}


/* map an encoded stg into an unminimized network */
static network_t *
stg_to_net(stg, astg)
graph_t *stg;
astg_graph *astg;
{
  network_t *network;
  FILE *fp;
  
  lsGen gen;
  vertex_t *start;
  edge_t *e;
  astg_generator agen;
  astg_signal *signal;
  int i, state_bits;
  char **state_vars = NIL(char*);
  char buf[80];
  int sfd;

  sfd = mkstemp(file);
  if (sfd == -1) {
    (void)fprintf(siserr, "stg_to_net: can't create temporary file");
    return NIL(network_t);
  }
  fp = fdopen(sfd, "w+");
  unlink(file);

  if (fp == 0) {
    (void)fprintf(siserr, "stg_to_net: can't open a temporary file");
    return NIL(network_t);
  }

  start = stg_get_start(stg);
  if (stg_get_state_encoding(start) == NIL(char)) {
    if (stg_get_num_states(stg) != 1) return NIL(network_t);
    state_bits = 0;
  } else {
    state_bits = strlen(stg_get_state_encoding(start));
    state_vars = ALLOC(char *, state_bits);
    for (i = 0; i < state_bits; i++) {
      (void)sprintf(buf, "_x%d", i);
      state_vars[i] = util_strsav (buf);
    }
  }
  (void) fprintf(fp, ".type fr\n");
  (void) fprintf(fp, ".i %d\n", stg_get_num_inputs(stg) + state_bits);
  (void) fprintf(fp, ".o %d\n", stg_get_num_outputs(stg) + state_bits);
  (void) fprintf(fp, ".ilb");
  if (astg != NIL(astg_graph)) {
    astg_foreach_signal(astg, agen, signal) {
      (void) fprintf(fp, " %s", astg_signal_name(signal));
    }
    for (i = 0; i < state_bits; i++) {
      (void) fprintf(fp, " %s", state_vars[i]);
    }
    (void) fprintf(fp, "\n.ob");
    for (i = 0; i < state_bits; i++) {
      (void) fprintf(fp, " %s_out", state_vars[i]);
    }
    astg_foreach_signal(astg, agen, signal) {
      if (astg_is_noninput(signal)) {
	(void) fprintf(fp, " %s_out", astg_signal_name(signal));
      }
    }
    (void) fprintf(fp, "\n");
  }
  
  stg_foreach_transition(stg, gen, e) {
    if (state_bits == 0) {
      (void) fprintf(fp, "%s %s\n", stg_edge_input_string(e), 
		     stg_edge_output_string(e));
    } else {
      (void) fprintf(fp, "%s %s %s %s\n", stg_edge_input_string(e),
		     stg_get_state_encoding(stg_edge_from_state(e)),
		     stg_get_state_encoding(stg_edge_to_state(e)),
		     stg_edge_output_string(e));
    }
  }
  (void) fprintf(fp, ".e\n");
  fflush(fp);
  rewind(fp);

  /* sis_read_pla needs this */
  read_register_filename(file);
  network = sis_read_pla(fp, /* single output */ 1);
  read_register_filename(NIL(char));
  
  fclose(fp);
  if (state_vars != NIL(char *)) {
    for (i = 0 ; i < state_bits; i++) {
      FREE(state_vars[i]);
    }
    FREE(state_vars);
  }
  return network;
}
#endif /* SIS */

