
/******************************************************************************
 *                       Approximate encoding algorithm                        *
 *                                                                             *
 ******************************************************************************/

#include "inc/nova.h"

iohybrid_code(net, symblemes, net_name, net_num, code_length) CONSTRAINT **net;
SYMBLEME *symblemes;
char *net_name;
int net_num;
int code_length;

{

  int dim_cube, iter_num, i, approx_work, state;
  BOOLEAN code_found, found_coding, backtrack_down();

  CONSTRAINT *update_net(), *last_constr;

  if ((*net) == (CONSTRAINT *)0 || OUT_ONLY) {
    if (VERBOSE)
      printf("SATISFACTION OF OUTPUT RELATIONS WITH SPECIALIZED ALGORITHM\n");
    out_encoder(code_length);
    if (strcmp(net_name, "Inputnet") != 0) {
      exact_rotation();

      exact_output();
    }
    return;
  }

  code_found = FALSE;
  found_coding = FALSE;
  OUT_VERIFY = FALSE;

  dim_cube = mylog2(minpow2(net_num));
  approx_work = 0;

  iter_num = num_constr(net);

  for (i = 0; i < iter_num; i++) {
    last_constr = update_net(net);
    /*printf("last_constr->relation = %s\n", last_constr->relation);*/
    exact_graph(net, net_num);
    faces_alloc(net_num);

    /* decides the categories and bounds of the faces (func. of dim_cube) */
    faces_set(dim_cube);

    /* finds an encoding (if any) for the current configuration */
    code_found = backtrack_down(net_num, dim_cube);

    approx_work += cube_work;

    if (code_found == FALSE) {
      last_constr->attribute = PRUNED;
      if (VERBOSE)
        printf("\nNo code was found\n");
    } else {
      found_coding = TRUE;
      faces_to_codes(symblemes, net_num);
      if (VERBOSE)
        printf("\nA code was found\n");
    }
  }
  if (VERBOSE) {
    printf("\nApprox_work = %d\n", approx_work);
    constr_satisfaction(net, symblemes, dim_cube, "exact");
  }

  /* tries to satisfy the output covering relations */
  if (VERBOSE)
    printf("\n\nSATISFACTION OF OUTPUT COVERING RELATIONS\n\n");
  OUT_VERIFY = TRUE;
  state = maxgain_state();
  while (state != -1) {
    select_array[state] = USED;
    /*printf("state = %d\n", state);
        show_orderarray(net_num);*/

    exact_graph(net, net_num);
    faces_alloc(net_num);
    faces_set(dim_cube);
    code_found = backtrack_down(net_num, dim_cube);
    approx_work += cube_work;

    if (code_found == FALSE) {
      select_array[state] = PRUNED;
      if (VERBOSE)
        printf("\nNo code was found\n");
    } else {
      found_coding = TRUE;
      faces_to_codes(symblemes, net_num);
      if (VERBOSE)
        printf("\nA code was found\n");
    }

    state = maxgain_state();
  }
  if (VERBOSE) {
    printf("\nApprox_work = %d\n", approx_work);
    out_performance("exact");
    weighted_outperf(net_num, "exact");
  }

  if (found_coding == FALSE)
    random_patch(symblemes, net_num);

  /* add a new dimension to the current cube to satisfy more constraints */
  if (code_length > dim_cube) {
    project_code(net, symblemes, net_name, net_num, code_length);
  }

  if (strcmp(net_name, "Inputnet") != 0) {
    exact_rotation();

    exact_output();
  }
}

int maxgain_state() {

  int i, min_gain, max_gain, state;

  state = -1;

  min_gain = 0;
  for (i = 0; i < statenum; i++) {
    if (gain_array[i] < min_gain) {
      min_gain = gain_array[i];
    }
  }
  max_gain = min_gain - 1;

  for (i = 0; i < statenum; i++) {
    if (select_array[i] == NOTUSED && gain_array[i] > max_gain) {
      max_gain = gain_array[i];
      state = i;
    }
  }

  return (state);
}
