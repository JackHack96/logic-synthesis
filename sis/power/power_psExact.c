
/*---------------------------------------------------------------------------
|      Calculates state probabilities of a FSM. Also calculates PS lines
| probabilities from state probabilities.
|
|        power_PS_lines_from_state()
|        power_exact_state_prob()
|
| Jose' Monteiro, MIT, Oct/93            jcm@rle-vlsi.mit.edu
+--------------------------------------------------------------------------*/

#ifdef SIS

#include "power_int.h"
#include "sis.h"
#include "spMatrix.h"

static double *power_state_prob();
static st_table *generatePSLIndexTable();
static void update_PSLines_prob();

void power_PS_lines_from_state(network, info_table) network_t *network;
st_table *info_table;
{
  st_table *stateIndex, *stateLineIndex;
  graph_t *stg;
  vertex_t *state;
  double *stateProb, *psLinesProb;
  int i, j, nPSLines;
  char *encoding, *key, *value;
  st_generator *sgen;
  lsGen gen;

  stg = stg_extract(network, 0 /* Use initial state */);
  if (stg == NIL(graph_t))
    return;

  stateProb = power_state_prob(network, stg, info_table, &stateIndex);
  st_foreach_item(stateIndex, sgen, &key, &value) { FREE(key); }
  st_free_table(stateIndex);

  /* Get number of present state lines */
  nPSLines = network_num_latch(network);
  psLinesProb = ALLOC(double, nPSLines);
  for (i = 0; i < nPSLines; i++)
    psLinesProb[i] = 0.0;

  i = 0;
  stg_foreach_state(stg, gen, state) {
    encoding = stg_get_state_encoding(state);
    for (j = 0; j < nPSLines; j++)
      if (encoding[j] == '1')
        psLinesProb[j] += stateProb[i];
      else
        assert(encoding[j] == '0');
    i++;
  }

  if (power_verbose > 40)
    for (i = 0; i < nPSLines; i++)
      fprintf(sisout, "Index = %d\tProb = %f\n", i, psLinesProb[i]);

  stateLineIndex = generatePSLIndexTable(network); /* Need latch order */
  update_PSLines_prob(network, info_table, psLinesProb, stateLineIndex);

  FREE(stateProb);
  FREE(psLinesProb);
  st_free_table(stateLineIndex);
  stg_free(stg);
  network_set_stg(network, NIL(graph_t));
}

double *power_exact_state_prob(network, info_table, stateIndex,
                               stateLineIndex) network_t *network;
st_table *info_table;
st_table **stateIndex;
st_table **stateLineIndex;
{
  graph_t *stg;
  double *stateProb;

  stg = stg_extract(network, 0 /* Use initial state */);
  if (stg == NIL(graph_t))
    return NIL(double);

  stateProb = power_state_prob(network, stg, info_table, stateIndex);

  stg_free(stg);
  network_set_stg(network, NIL(graph_t));

  *stateLineIndex = generatePSLIndexTable(network); /* Need latch order */

  return stateProb;
}

static double *power_state_prob(network, stg, info_table,
                                stateIndex) network_t *network;
graph_t *stg;
st_table *info_table;
st_table **stateIndex;
{
  vertex_t *state, *fromState;
  node_t *pi;
  edge_t *transition;
  node_info_t *info;
  char *matrix, *transitionInput;
  int i, row, column, matrixSize, spError;
  double *PIProb, transitionProb, *matrixEntry, *rhs, *solution;
  lsGen genPI, genState, genTransition;

  PIProb = ALLOC(double, network_num_pi(network));
  i = 0;
  foreach_primary_input(network, genPI, pi) {
    assert(st_lookup(info_table, (char *)pi, (char **)&info));
    PIProb[i] = info->prob_one;
    i++;
  }

  *stateIndex = st_init_table(strcmp, st_strhash);
  i = 0;
  stg_foreach_state(stg, genState, state) {
    st_insert(*stateIndex, util_strsav(stg_get_state_encoding(state)),
              (char *)i++);
  }

  matrixSize = i;
  matrix = spCreate(matrixSize, 0 /* Real */, &spError);
  assert(spError == spOKAY);

  row = 1;
  stg_foreach_state(stg, genState, state) {
    if (row == matrixSize) /* Don't fill last row */
      break;

    foreach_state_inedge(state, genTransition, transition) {

      transitionInput = stg_edge_input_string(transition);
      transitionProb = 1.0;
      for (i = 0; transitionInput[i] != '\0'; i++)
        switch (transitionInput[i]) {
        case '0':
          transitionProb *= (1 - PIProb[i]);
          continue;
        case '1':
          transitionProb *= PIProb[i];
        }

      fromState = stg_edge_from_state(transition);
      assert(st_lookup(*stateIndex, stg_get_state_encoding(fromState),
                       (char **)&column));

      matrixEntry = spGetElement(matrix, row, column + 1);
      spADD_REAL_ELEMENT(matrixEntry, transitionProb);
    }
    row++;
  }

  /* Subtract 1 from elements of the diagonal, except last row */
  for (i = 1; i < matrixSize; i++) {
    matrixEntry = spGetElement(matrix, i, i);
    spADD_REAL_ELEMENT(matrixEntry, -1.0);
  }

  /* Last line in the matrix is sum of all probabilities */
  for (i = 1; i <= matrixSize; i++) {
    matrixEntry = spGetElement(matrix, matrixSize, i);
    spADD_REAL_ELEMENT(matrixEntry, 1.0);
  }

  /* Build right hand side (rhs) */
  rhs = ALLOC(double, matrixSize);
  for (i = 0; i < (matrixSize - 1); i++)
    rhs[i] = 0.0;
  rhs[matrixSize - 1] = 1.0;

  spError = spOrderAndFactor(matrix, rhs, 0.01 /*pivot selection*/,
                             0.0 /*smaller than any element in the diagonal*/,
                             0 /*don't restrict to diagonal pivoting*/);
  assert(spError == spOKAY);
  /*
  spPrint(matrix, 0, 1, 1);
  printf("spCondition = %f\n", spCondition(matrix, spNorm(matrix), &spError));
  assert(spError == spOKAY);
  */

  solution = ALLOC(double, matrixSize);
  spSolve(matrix, rhs, solution);

  if (power_verbose > 40)
    for (i = 0; i < matrixSize; i++)
      fprintf(sisout, "i = %d\t\tsolution = %f\n", i, solution[i]);

  FREE(PIProb);
  FREE(rhs);
  spDestroy(matrix);

  return solution;
}

static st_table *generatePSLIndexTable(network) network_t *network;
{
  st_table *indexTable;
  latch_t *latch;
  int i = 0;
  lsGen gen;

  indexTable = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_latch(network, gen, latch) {
    st_insert(indexTable, (char *)latch_get_output(latch), (char *)i++);
  }

  return indexTable;
}

static void update_PSLines_prob(network, info_table, psLineProb,
                                psIndex) network_t *network;
st_table *info_table;
double *psLineProb;
st_table *psIndex;
{
  node_t *pi;
  node_info_t *info;
  int index;
  lsGen gen;

  foreach_primary_input(network, gen, pi) {
    if (network_is_real_pi(network, pi))
      continue;
    assert(st_lookup(info_table, (char *)pi, (char **)&info));
    assert(st_lookup(psIndex, (char *)pi, (char **)&index));
    info->prob_one = psLineProb[index];
  }
}

#endif /* SIS */
