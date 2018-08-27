
/******************************************************************************
 *                       Approximate encoding algorithm                        *
 *                                                                             *
 ******************************************************************************/

#include "inc/nova.h"

iovariant_code(net, symblemes, net_name, net_num, code_length) CONSTRAINT **net;
SYMBLEME *symblemes;
char *net_name;
int net_num;
int code_length;

{

  int dim_cube, iter_num, i, approx_work, state;
  BOOLEAN code_found, found_coding, backtrack_down();

  CONSTRAINT *update_net(), *last_constr;

  if ((*net) == (CONSTRAINT *)0) {
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

  /* set to PRUNED all face constraints associated to next states */
  cut_nxstconstr(net, net_num);
  /* count the face constraints associated only to outputs */
  iter_num = num_outconstr(net);

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
  if (VERBOSE)
    printf("\nApprox_work = %d\n", approx_work);

  /* set to NOTUSED all face constraints associated to next states */
  restore_nxstconstr(net, net_num);

  /* tries to satisfy the output covering relations */
  if (VERBOSE)
    printf("\n\nSATISFACTION OF OUTPUT COVERING RELATIONS\n\n");
  OUT_VERIFY = TRUE;
  state = maxgain_state();
  while (state != -1) {
    select_array[state] = USED;
    /*printf("state = %d\n", state);
        show_orderarray(net_num);*/

    /* set to TRIED all UNUSED face constraints associated to state */
    tried_nxstconstr(net, state);

    exact_graph(net, net_num);
    faces_alloc(net_num);
    faces_set(dim_cube);
    code_found = backtrack_down(net_num, dim_cube);
    approx_work += cube_work;

    if (code_found == FALSE) {
      select_array[state] = PRUNED;

      /* set to NOTUSED all TRIED face constraints associated to state */
      notused_nxstconstr(net, state);

      if (VERBOSE)
        printf("\nNo code was found\n");
    } else {
      found_coding = TRUE;

      /* set to USED all TRIED face constraints associated to state */
      used_nxstconstr(net, state);

      faces_to_codes(symblemes, net_num);
      if (VERBOSE)
        printf("\nA code was found\n");
    }

    state = maxgain_state();
  }
  if (VERBOSE) {
    printf("\nApprox_work = %d\n", approx_work);
    constr_satisfaction(net, symblemes, dim_cube, "exact");
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

int num_outconstr(net) CONSTRAINT **net;

{

  CONSTRAINT *constrptr;
  int i;

  i = 0;

  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    if (constrptr->attribute == NOTUSED)
      i++;
  }

  return (i);
}

cut_nxstconstr(net, net_num) CONSTRAINT **net;
int net_num;

{

  CONSTRAINT *constrptr;
  int i;

  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    for (i = 0; i < net_num; i++) {
      if (constrptr->next_states[i] == ONE) {
        constrptr->attribute = PRUNED;
        break;
      }
    }
  }
}

restore_nxstconstr(net, net_num) CONSTRAINT **net;
int net_num;

{

  CONSTRAINT *constrptr;
  int i;

  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    for (i = 0; i < net_num; i++) {
      if (constrptr->next_states[i] == ONE) {
        constrptr->attribute = NOTUSED;
        break;
      }
    }
  }
}

tried_nxstconstr(net, next_state) CONSTRAINT **net;
int next_state;

{

  CONSTRAINT *constrptr;

  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    if (constrptr->next_states[next_state] == ONE &&
        constrptr->attribute == NOTUSED) {
      constrptr->attribute = TRIED;
    }
  }
}

notused_nxstconstr(net, next_state) CONSTRAINT **net;
int next_state;

{

  CONSTRAINT *constrptr;

  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    if (constrptr->next_states[next_state] == ONE &&
        constrptr->attribute == TRIED) {
      constrptr->attribute = NOTUSED;
    }
  }
}

used_nxstconstr(net, next_state) CONSTRAINT **net;
int next_state;

{

  CONSTRAINT *constrptr;

  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    if (constrptr->next_states[next_state] == ONE &&
        constrptr->attribute == TRIED) {
      constrptr->attribute = USED;
    }
  }
}
