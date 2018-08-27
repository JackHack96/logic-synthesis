
/*---------------------------------------------------------------------------
|      Calculate PS lines probabilities of a FSM directly (without
| enumerating the states of the FSM).
|
|        power_direct_PS_lines_prob()
|        power_place_PIs_last()
|
| Jose' Monteiro, MIT, Oct/93            jcm@rle-vlsi.mit.edu
+--------------------------------------------------------------------------*/

#ifdef SIS

#include "power_int.h"
#include "sis.h"

#define TINY 1.0e-20;

static array_t *orderPSLines();
static array_t *addOutputLogic();
static node_t **addNewNSSet();
static void update_PSLines_prob();

static double *calculate_line_prob();
static double *power_calc_func_cofact_prob();
static double *power_count_f_cof_probPS();
static double *power_count_f_cof_probPI();
static void multiply_PS_prob();
static void calculate_next_iteration();

static void ludcmp();
static void lubksb();

static double *save_vector();

static st_table *visited;
static power_pi_t *PIInfo;

double *power_direct_PS_lines_prob(network, info_table, symbPSOrder,
                                   symbolic) network_t *network;
st_table *info_table;
array_t **symbPSOrder;
network_t *symbolic;
{
  network_t *nsLogic;
  array_t *psOrder, *poArray, *newNSs;
  node_t *node;
  double *lineProb;
  char name[1000];
  int i;
  lsGen gen;

  /* Get the network corresponding to the NS logic */
  nsLogic = network_dup(network);

  /* Now delete the real PO nodes */
  poArray = array_alloc(node_t *, 0);
  foreach_primary_output(nsLogic, gen, node) {
    array_insert_last(node_t *, poArray, node);
  }
  for (i = 0; i < array_n(poArray); i++) {
    node = array_fetch(node_t *, poArray, i);
    if (network_latch_end(node) == NIL(node_t))
      /* This is a pure PO */
      network_delete_node(nsLogic, node);
  }
  array_free(poArray);

  /* Delete all the nodes that don't go anywhere */
  network_csweep(nsLogic);

  if (power_setSize != 1) {
    /* Generate sets of PS lines */
    psOrder = orderPSLines(nsLogic);
    /* Generate output logic for next state */
    newNSs = addOutputLogic(nsLogic, psOrder);
  } else {
    newNSs = array_alloc(node_t *, 0);
    psOrder = array_alloc(node_t *, 0);
    foreach_primary_output(nsLogic, gen, node) {
      array_insert_last(node_t *, newNSs, node);
      array_insert_last(node_t *, psOrder, network_latch_end(node));
    }
  }

  lineProb = calculate_line_prob(nsLogic, network, info_table, psOrder, newNSs);

  if (power_setSize == 1) {
    update_PSLines_prob(network, info_table, lineProb, psOrder);
    *symbPSOrder = psOrder; /* Return something, won't be used */
  } else {
    *symbPSOrder = array_alloc(node_t *, 0);
    for (i = 0; i < array_n(psOrder); i++) {
      node = array_fetch(node_t *, psOrder, i);
      sprintf(name, "%s_nsl", node->name);
      node = network_find_node(symbolic, name);
      array_insert_last(node_t *, *symbPSOrder, node);
    }
    array_free(psOrder);
  }

  array_free(newNSs);
  network_free(nsLogic);

  return lineProb;
}

static double *calculate_line_prob(nsLogic, network, info_table, psOrder,
                                   newNSs) network_t *nsLogic;
network_t *network;
st_table *info_table;
array_t *psOrder;
array_t *newNSs;
{
  st_table *leaves = st_init_table(st_ptrcmp, st_ptrhash);
  array_t *poArray, *piOrder;
  bdd_manager *manager;
  bdd_t *bdd;
  node_info_t *info;
  node_t *pi, *po, *node;
  double *allpsLinesProb, /* To be returned */
      *psLinesProb,       /* Without the redundant value per set */
      *Y_J_values, *Y, *J;
  int i, j, remain, nSets, psPerSet, nNSLines, n_iter = 0, converged;
  lsGen gen;

  /* Initialize bdd manager for next state logic */
  poArray = array_alloc(node_t *, 0);
  foreach_primary_output(nsLogic, gen, po) {
    array_insert_last(node_t *, poArray, po);
  }
  piOrder = order_nodes(poArray, /* PI's only */ 1);
  if (piOrder == NIL(array_t))
    piOrder = array_alloc(node_t *, 0);
  array_free(poArray);

  power_place_PIs_last(&piOrder, psOrder); /* Also works for power_setSize=1*/

  manager = ntbdd_start_manager(3 * network_num_pi(nsLogic));

  PIInfo = ALLOC(power_pi_t, array_n(piOrder));
  for (i = 0; i < array_n(piOrder); i++) {
    pi = array_fetch(node_t *, piOrder, i);
    st_insert(leaves, (char *)pi, (char *)i);
    if (i < array_n(psOrder)) /* Then it's a PS line */
      continue;
    node = network_find_node(network, pi->name);
    assert(st_lookup(info_table, (char *)node, (char **)&info));
    PIInfo[i].probOne = info->prob_one;
  }
  array_free(piOrder);

  /* Alloc vector of solutions */
  nNSLines = array_n(newNSs);
  psPerSet = 1 << power_setSize;
  nSets = array_n(psOrder) / power_setSize;
  remain = nNSLines - nSets * (psPerSet - 1);
  if (remain) {
    remain++; /* ++ to include the all ones lines */
    allpsLinesProb = ALLOC(double, (nSets + 1) * psPerSet);
    for (i = nSets * psPerSet + remain; i < (nSets + 1) * psPerSet; i++)
      allpsLinesProb[i] = 0.0; /* Won't be used, make things generic */
  } else
    allpsLinesProb = ALLOC(double, nSets *psPerSet);

  /* Initial solution */
  for (i = 0; i < nSets * psPerSet; i++) /* Some waste for setSize = 1 */
    allpsLinesProb[i] = 1.0 / psPerSet;
  for (i = nSets * psPerSet; i < nSets * psPerSet + remain; i++)
    allpsLinesProb[i] = 1.0 / remain;
  psLinesProb = ALLOC(double, nNSLines);
  for (i = 0; i < nSets * (psPerSet - 1); i++)
    psLinesProb[i] = 1.0 / psPerSet;
  for (i = nSets * (psPerSet - 1); i < nNSLines; i++)
    psLinesProb[i] = 1.0 / remain;

  /* Alloc Y vector and J jacobian matrix */
  Y = ALLOC(double, nNSLines);
  J = ALLOC(double, nNSLines *nNSLines);

  /* Format of the Y_J_values array:

                f__ | f   |... nNSLines times | y |
                 psi   psi                       i
                                                                           */
  do {                               /* Iteration loop */
    for (i = 0; i < nNSLines; i++) { /* Build matrices */
      po = array_fetch(node_t *, newNSs, i);
      bdd = ntbdd_node_to_bdd(po, manager, leaves);
      Y_J_values = power_calc_func_cofact_prob(bdd, allpsLinesProb, nNSLines,
                                               array_n(psOrder));
      for (j = 0; j < nNSLines; j++)
        J[i * nNSLines + j] =
            Y_J_values[2 * j] - Y_J_values[2 * j + 1] + (i == j);
      Y[i] = psLinesProb[i] - Y_J_values[2 * nNSLines];
      FREE(Y_J_values);
    }

    converged = TRUE; /* Test for convergence */
    for (i = 0; i < nNSLines; i++)
      if (ABS(Y[i]) > power_delta) {
        converged = FALSE;
        break;
      }

    calculate_next_iteration(allpsLinesProb, &psLinesProb, &Y, J, nNSLines);

    for (i = 0; i < nNSLines; i++) { /* To avoid round-off errors */
      if (psLinesProb[i] < 0.0)
        psLinesProb[i] = 0.0;
      if (psLinesProb[i] > 1.0)
        psLinesProb[i] = 1.0;
    }

    n_iter++;
  } while (!converged);

  if (power_verbose > 25) {
    fprintf(sisout, "Number of Iterations: %d\nLine probabilities:\n", n_iter);
    for (i = 0; i < nNSLines; i++)
      fprintf(sisout, "Index: %d\t\tProb: %.3f\n", i, psLinesProb[i]);
  }

  FREE(Y);
  FREE(J);
  FREE(PIInfo);
  FREE(psLinesProb);
  st_free_table(leaves);
  ntbdd_end_manager(manager);

  return allpsLinesProb;
}

void power_place_PIs_last(piOrder, psOrder) array_t **piOrder;
array_t *psOrder;
{
  st_table *psTable = st_init_table(st_ptrcmp, st_ptrhash);
  array_t *newPIOrder = array_alloc(node_t *, 0);
  node_t *node;
  int i;
  char *dummy = NIL(char);

  for (i = 0; i < array_n(psOrder); i++) {
    node = array_fetch(node_t *, psOrder, i);
    array_insert_last(node_t *, newPIOrder, node);
    st_insert(psTable, (char *)node, dummy);
  }
  for (i = 0; i < array_n(*piOrder); i++) {
    node = array_fetch(node_t *, *piOrder, i);
    if (!st_lookup(psTable, (char *)node, &dummy))
      array_insert_last(node_t *, newPIOrder, node);
  }
  st_free_table(psTable);
  array_free(*piOrder);
  *piOrder = newPIOrder;
}

static double *power_calc_func_cofact_prob(f_bdd, psProb, nNSLines,
                                           nPSLines) bdd_t *f_bdd;
double *psProb;
int nNSLines;
int nPSLines;
{
  double *result;
  int i;
  pset sets;
  char *value, *key;
  st_generator *gen;

  assert(f_bdd != NIL(bdd_t));

  visited = st_init_table(st_ptrcmp, st_ptrhash);

  sets = set_full(2 * power_setSize);

  result =
      power_count_f_cof_probPS(f_bdd, psProb, nNSLines, nPSLines, -1, sets);

  if (result == NIL(double)) { /* Weird case... */
    result = ALLOC(double, 2 * nNSLines + 1);
    for (i = 0; i < 2 * nNSLines + 1; i++)
      result[i] = 0.0;
  }

  st_foreach_item(visited, gen, &key, &value) { FREE(value); }
  st_free_table(visited);

  return result;
}

static double *power_count_f_cof_probPS(f, psProb, nNSLines, nPSLines, prevInd,
                                        sets) bdd_t *f;
double *psProb;
int nNSLines;
int nPSLines;
long prevInd;
pset sets;
{
  bdd_t *child;
  double *result, *result1;
  pset keepSets, otherSet;
  long topf;
  int i, newSet;

  if (f->node == cmu_bdd_zero(f->mgr)) {
    set_free(sets);
    return NIL(double);
  }
  if (f->node == cmu_bdd_one(f->mgr)) {
    result = ALLOC(double, 2 * nNSLines + 1);
    for (i = 0; i < 2 * nNSLines + 1; i++)
      result[i] = 1.0;
    multiply_PS_prob(result, prevInd, psProb, sets, nNSLines);
    set_free(sets);
    return result;
  }

  if (st_lookup(visited, (char *)f->node, (char **)&result)) {
    result1 = save_vector(result, 2 * nNSLines + 1);
    multiply_PS_prob(result1, prevInd, psProb, sets, nNSLines);
    set_free(sets);
    return result1;
  }

  topf = cmu_bdd_if_index(f->mgr, f->node);

  if (topf >= nPSLines) { /* First PI found, all PIs from now on */
    result = power_count_f_cof_probPI(f, nNSLines);
    if ((result != NIL(double)) && (prevInd >= 0))
      multiply_PS_prob(result, prevInd, psProb, sets, nNSLines);
    set_free(sets);
    return result;
  }

  newSet =
      (prevInd != -1) && ((topf / power_setSize) != (prevInd / power_setSize));

  if (newSet) {
    keepSets = sets;
    sets = set_full(2 * power_setSize);
  }

  otherSet = set_save(sets);
  set_remove(otherSet, 2 * (topf % power_setSize) + 1);
  child = bdd_else(f);
  result = power_count_f_cof_probPS(child, psProb, nNSLines, nPSLines, topf,
                                    otherSet);
  FREE(child);

  set_remove(sets, 2 * (topf % power_setSize));
  child = bdd_then(f);
  result1 =
      power_count_f_cof_probPS(child, psProb, nNSLines, nPSLines, topf, sets);
  if (result1 != NIL(double)) {
    if (result == NIL(double))
      result = result1;
    else {
      for (i = 0; i < 2 * nNSLines + 1; i++)
        result[i] += result1[i];
      FREE(result1);
    }
  }
  FREE(child);

  if (!(topf % power_setSize)) /* First element in a set, can store result */
    st_insert(visited, (char *)f->node,
              (char *)save_vector(result, 2 * nNSLines + 1));

  if (newSet) {
    multiply_PS_prob(result, prevInd, psProb, keepSets, nNSLines);
    set_free(keepSets);
  }

  return result;
}

static double *power_count_f_cof_probPI(f, nNSLines) bdd_t *f;
int nNSLines;
{
  bdd_t *child;
  double *result, *result1;
  int i, topf;

  if (st_lookup(visited, (char *)f->node, (char **)&result)) {
    result1 = save_vector(result, 2 * nNSLines + 1);
    return result1;
  }
  if (f->node == cmu_bdd_zero(f->mgr))
    return NIL(double);
  if (f->node == cmu_bdd_one(f->mgr)) {
    result = ALLOC(double, 2 * nNSLines + 1);
    for (i = 0; i < 2 * nNSLines + 1; i++)
      result[i] = 1.0;
    return result;
  }

  topf = cmu_bdd_if_index(f->mgr, f->node);

  child = bdd_else(f);
  result = power_count_f_cof_probPI(child, nNSLines);
  if (result != NIL(double))
    for (i = 0; i < 2 * nNSLines + 1; i++)
      result[i] *= (1.0 - PIInfo[topf].probOne);
  FREE(child);

  child = bdd_then(f);
  result1 = power_count_f_cof_probPI(child, nNSLines);
  if (result1 != NIL(double)) {
    for (i = 0; i < 2 * nNSLines + 1; i++)
      result1[i] *= PIInfo[topf].probOne;

    if (result == NIL(double))
      result = result1;
    else {
      for (i = 0; i < 2 * nNSLines + 1; i++)
        result[i] += result1[i];
      FREE(result1);
    }
  }
  FREE(child);

  st_insert(visited, (char *)f->node,
            (char *)save_vector(result, 2 * nNSLines + 1));

  return result;
}

static void multiply_PS_prob(result, index, psProb, sets,
                             nNSLines) double *result;
long index;
double *psProb;
pset sets;
int nNSLines;
{
  pset inclSets;
  double sumProb;
  long setN;
  int i, thisSet, setSize;

  if (power_setSize == 1) { /* This if is not need, but is much simpler */
    if (is_in_set(sets, 1))
      sumProb = psProb[index];
    else
      sumProb = 1 - psProb[index];
    for (i = 0; i < 2 * nNSLines + 1; i++)
      if (i == 2 * index)
        if (is_in_set(sets, 1))
          result[i++] = 0.0;
        else
          result[++i] = 0.0;
      else
        result[i] *= sumProb;
  } else {
    setN = index / power_setSize;
    setSize = (1 << power_setSize) - 1;
    if ((thisSet = setSize + 1) > (nNSLines + 1 - setN * setSize))
      thisSet = nNSLines + 1 - setN * setSize;
    inclSets = power_lines_in_set(sets, thisSet);

    sumProb = 0.0;
    for (i = 0; i < thisSet; i++)
      if (is_in_set(inclSets, i))
        sumProb += psProb[setN * (setSize + 1) + i];

    for (i = 0; i < 2 * nNSLines; i++)
      if (i / (2 * setSize) != setN)
        result[i] *= sumProb;
    result[i] *= sumProb;

    for (i = 0; i < (thisSet - 1); i++)
      if (!is_in_set(inclSets, i))
        result[2 * (setN * setSize + i) + 1] = 0.0;
    if (!is_in_set(inclSets, thisSet - 1))
      for (i = 0; i < (thisSet - 1); i++)
        result[2 * (setN * setSize + i)] = 0.0;
    set_free(inclSets);
  }
}

static void calculate_next_iteration(allpsLinesProb, psLinesProb, Y, J,
                                     nNSLines) double *allpsLinesProb;
double **psLinesProb;
double **Y;
double *J;
int nNSLines;
{
  int i, j, *indx;
  double sum, *tmp;

  for (i = 0; i < nNSLines; i++) {
    sum = 0.0;
    for (j = 0; j < nNSLines; j++)
      sum += J[i * nNSLines + j] * (*psLinesProb)[j];
    (*Y)[i] = sum - (*Y)[i];
  }

  indx = ALLOC(int, nNSLines);
  ludcmp(J, nNSLines, indx);
  lubksb(J, nNSLines, indx, *Y);
  FREE(indx);

  tmp = *Y;
  *Y = *psLinesProb;
  *psLinesProb = tmp;

  /* Compute probability for remaining element in each set */
  if (power_setSize == 1)
    for (i = 0; i < nNSLines; i++)
      allpsLinesProb[i] = (*psLinesProb)[i];
  else {
    sum = 0.0;
    for (i = 0, j = 0; i < nNSLines; i++, j++) {
      allpsLinesProb[j] = (*psLinesProb)[i];
      sum += (*psLinesProb)[i];
      if (!((i + 1) % ((1 << power_setSize) - 1)) || ((i + 1) == nNSLines)) {
        allpsLinesProb[++j] = 1.0 - sum; /* Last NS in set: 1-P */
        sum = 0.0;
      }
    }
    if (sum != 0.0) /* Case of incomplete last set */
      allpsLinesProb[nNSLines] = 1.0 - sum;
  }
}

/* Routines from "Numerical Recipes in C" */
static void ludcmp(a, n, indx) double *a;
int n;
int *indx;
{
  int i, imax, j, k;
  double big, dum, sum, temp, *vv;

  vv = ALLOC(double, n);
  for (i = 0; i < n; i++) {
    big = 0.0;
    for (j = 0; j < n; j++)
      if ((temp = fabs(a[i * n + j])) > big)
        big = temp;
    if (big == 0.0)
      fail("Singular matrix in routine LUDCMP");
    vv[i] = 1.0 / big;
  }
  for (j = 0; j < n; j++) {
    for (i = 0; i < j; i++) {
      sum = a[i * n + j];
      for (k = 0; k < i; k++)
        sum -= a[i * n + k] * a[k * n + j];
      a[i * n + j] = sum;
    }
    big = 0.0;
    for (i = j; i < n; i++) {
      sum = a[i * n + j];
      for (k = 0; k < j; k++)
        sum -= a[i * n + k] * a[k * n + j];
      a[i * n + j] = sum;
      if ((dum = vv[i] * fabs(sum)) >= big) {
        big = dum;
        imax = i;
      }
    }
    if (j != imax) {
      for (k = 0; k < n; k++) {
        dum = a[imax * n + k];
        a[imax * n + k] = a[j * n + k];
        a[j * n + k] = dum;
      }
      vv[imax] = vv[j];
    }
    indx[j] = imax;
    if (a[j * n + j] == 0.0)
      a[j * n + j] = TINY;
    if (j != n - 1) {
      dum = 1.0 / a[j * n + j];
      for (i = j + 1; i < n; i++)
        a[i * n + j] *= dum;
    }
  }
  FREE(vv);
}

static void lubksb(a, n, indx, b) double *a;
int n;
int *indx;
double b[];
{
  int i, ii = -1, ip, j;
  double sum;

  for (i = 0; i < n; i++) {
    ip = indx[i];
    sum = b[ip];
    b[ip] = b[i];
    if (ii != -1)
      for (j = ii; j < i; j++)
        sum -= a[i * n + j] * b[j];
    else if (sum)
      ii = i;
    b[i] = sum;
  }
  for (i = n - 1; i >= 0; i--) {
    sum = b[i];
    for (j = i + 1; j < n; j++)
      sum -= a[i * n + j] * b[j];
    b[i] = sum / a[i * n + i];
  }
}

static double *save_vector(vector, n) double *vector;
int n;
{
  double *newVector;
  int i;

  newVector = ALLOC(double, n);
  for (i = 0; i < n; i++)
    newVector[i] = vector[i];

  return newVector;
}

static array_t *orderPSLines(nsLogic) network_t *nsLogic;
{
  array_t *psOrder;
  node_t *po;
  lsGen gen;

  /* For now, just list the PIs. Think of a way to get a better ordering */
  psOrder = array_alloc(node_t *, 0);
  foreach_primary_output(nsLogic, gen, po) {
    array_insert_last(node_t *, psOrder, network_latch_end(po));
  }

  return psOrder;
}

static array_t *addOutputLogic(network, oldPSOrder) network_t *network;
array_t *oldPSOrder;
{
  array_t *newNSs;
  node_t **nsSet;
  int nSets, nNewNSs, remain, i, j;

  newNSs = array_alloc(node_t *, 0);
  nSets = array_n(oldPSOrder) / power_setSize;
  nNewNSs = (1 << power_setSize) - 1; /* Last element of set is (1-P) */

  for (i = 0; i < nSets; i++) {
    nsSet = addNewNSSet(network, power_setSize, nNewNSs, oldPSOrder, i);
    for (j = 0; j < nNewNSs; j++)
      array_insert_last(node_t *, newNSs, nsSet[j]);
    FREE(nsSet);
  }
  remain = array_n(oldPSOrder) - power_setSize * nSets;
  if (remain != 0) {
    nsSet = addNewNSSet(network, remain, (1 << remain) - 1, oldPSOrder, i);
    for (j = 0; j < (1 << remain) - 1; j++)
      array_insert_last(node_t *, newNSs, nsSet[j]);
    FREE(nsSet);
  }

  return newNSs;
}

static node_t **addNewNSSet(network, setSize, nNewNSs, oldPSOrder,
                            i) network_t *network;
int setSize;
int nNewNSs;
array_t *oldPSOrder;
int i;
{
  node_t *node, **newNSSet, **toOldNSs;
  pset implic;
  pset_family cover;
  int j, k, mask;

  newNSSet = ALLOC(node_t *, nNewNSs);
  toOldNSs = ALLOC(node_t *, setSize);

  for (j = 0; j < setSize; j++) {
    node = node_alloc();
    network_add_primary_input(network, node);
    toOldNSs[j] = node;
  }
  for (j = 0; j < nNewNSs; j++) {
    node = node_alloc();
    network_add_node(network, node);
    cover = sf_new(1, setSize * 2);
    implic = set_new(setSize * 2);
    mask = 1;
    for (k = 0; k < setSize; k++, mask <<= 1) {
      if (j & mask)
        set_insert(implic, 2 * k + 1);
      else
        set_insert(implic, 2 * k);
    }
    cover = sf_addset(cover, implic);
    set_free(implic);
    node_replace_internal(node, nodevec_dup(toOldNSs, setSize), setSize, cover);
    node_scc(node); /* make it scc-minimal, dup_free, etc. */
    newNSSet[j] = network_add_primary_output(network, node);
  }
  for (j = 0; j < setSize; j++) {
    node = array_fetch(node_t *, oldPSOrder, i * power_setSize + j);
    network_connect(network_latch_end(node), toOldNSs[j]);
  }
  FREE(toOldNSs);

  return newNSSet;
}

static void update_PSLines_prob(network, info_table, psLineProb,
                                psOrder) network_t *network;
st_table *info_table;
double *psLineProb;
array_t *psOrder;
{
  node_t *ps;
  node_info_t *info;
  int i;

  for (i = 0; i < array_n(psOrder); i++) {
    ps = array_fetch(node_t *, psOrder, i);
    ps = network_find_node(network, ps->name);
    assert(st_lookup(info_table, (char *)ps, (char **)&info));
    info->prob_one = psLineProb[i];
  }
}

#endif /* SIS */
