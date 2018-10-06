
/*---------------------------------------------------------------------------
|      This file contains the "SCAN" operation on the BDD whereby all the
| probabilities are easily obtained using static probabilities of inputs.
|
|        power_calc_func_prob()
|        power_calc_func_prob_w_stateProb()
|        power_calc_func_prob_w_sets()
|
| Copyright (c) 1991 - Abhijit Ghosh. University of California, Berkeley
|
| Jose' Monteiro, MIT, Aug/93            jcm@rle-vlsi.mit.edu
|  - New function that takes into account probabilities of sets of lines
+--------------------------------------------------------------------------*/

#include "power_int.h"
#include "sis.h"

static double power_count_prob();

#ifdef SIS

static double power_count_prob_w_stateProb();

static void evaluate_included_state_prob();

static double power_count_f_prob_w_sets();

static void multiply_PS_prob();

#endif /* SIS */

static st_table   *visited;
static power_pi_t *PIInfo;

/* The following routine calculates the probability of function a (given with
 *  bdd) being one, given the static input zero and one probabilities of its
 *  inputs in the zero and one array
 */
double power_calc_func_prob(bdd_t *f_bdd, power_pi_t *infoPIs) {
    double       result;
    char         *value, *key;
    st_generator *gen;

    assert(f_bdd != NIL(bdd_t));

    visited = st_init_table(st_ptrcmp, st_ptrhash);
    PIInfo  = infoPIs;

    result = power_count_prob(f_bdd);

    st_foreach_item(visited, gen, &key, &value) { FREE(value); }
    st_free_table(visited);

    return result;
}

/* Basically counts the probability of going down each path in the bdd and
 *  sums it up to give the total probability of function being one
 */
static double power_count_prob(bdd_t *f) {
    bdd_t  *child;
    double result, *stored;
    long   topf;

    if (st_lookup(visited, (char *) f->node, (char **) &stored))
        return *stored;

    if (f->node == cmu_bdd_zero(f->mgr))
        return 0.0;
    if (f->node == cmu_bdd_one(f->mgr))
        return 1.0;

    topf = cmu_bdd_if_index(f->mgr, f->node);

    child  = bdd_else(f);
    result = (1.0 - PIInfo[topf].probOne) * power_count_prob(child);
    FREE(child);

    child = bdd_then(f);
    result += PIInfo[topf].probOne * power_count_prob(child);
    FREE(child);

    stored = ALLOC(double, 1);
    *stored = result;
    st_insert(visited, (char *) f->node, (char *) stored);

    return result;
}

#ifdef SIS

/* The following routine calculates the probability of a function (given with
 *  bdd) being one, given the static input zero and one probabilities of its
 *  primary inputs in the zero and one array and the probability of the FSM
 *  being in each state
 */
double power_calc_func_prob_w_stateProb(bdd_t *f_bdd, power_pi_t *infoPIs, double *stateProb, st_table *stateIndex,
                                        int nStateLines) {
    double       result;
    char         *value, *key, *encoding;
    int          i;
    st_generator *gen;

    assert(f_bdd != NIL(bdd_t));

    visited = st_init_table(st_ptrcmp, st_ptrhash);
    PIInfo  = infoPIs;

    encoding = ALLOC(char, nStateLines + 1);
    for (i   = 0; i < nStateLines; i++)
        encoding[i]       = '-';
    encoding[nStateLines] = '\0';

    result = power_count_prob_w_stateProb(f_bdd, stateProb, stateIndex, encoding);

    st_foreach_item(visited, gen, &key, &value) { FREE(value); }
    st_free_table(visited);

    return result;
}

/* Goes down the BDD until a PI is found, thus we have which states are
 *  included in this BDD path (state lines are all before PI lines). At
 *  this point 'power_count_prob' is called and the result returned
 *  multiplied by the sum of the state probabilities.
 */
static double power_count_prob_w_stateProb(bdd_t *f, double *stateProb, st_table *stateIndex, char *encoding) {
    bdd_t  *child;
    double result;
    long   topf;

    if (f->node == cmu_bdd_zero(f->mgr)) {
        FREE(encoding);
        return 0.0;
    }
    if (f->node == cmu_bdd_one(f->mgr)) {
        result = 0.0;
        evaluate_included_state_prob(encoding, stateProb, stateIndex, 0, &result);
        return result;
    }

    topf = cmu_bdd_if_index(f->mgr, f->node);

    if (PIInfo[topf].PSLineIndex == -1) { /* This is a PI */
        result = 0.0;
        evaluate_included_state_prob(encoding, stateProb, stateIndex, 0, &result);
        result *= power_count_prob(f);
        return result;
    }

    encoding[PIInfo[topf].PSLineIndex] = '0';
    child  = bdd_else(f);
    result = power_count_prob_w_stateProb(child, stateProb, stateIndex,
                                          util_strsav(encoding));
    FREE(child);

    encoding[PIInfo[topf].PSLineIndex] = '1';
    child = bdd_then(f);
    result +=
            power_count_prob_w_stateProb(child, stateProb, stateIndex, encoding);
    FREE(child);

    return result;
}

static void evaluate_included_state_prob(char *encoding, double *stateProb, st_table *stateIndexTable, int lineIndex,
                                         double *result) {
    int stateIndex;

    switch (encoding[lineIndex]) {
        case '\0':
            if (st_lookup(stateIndexTable, encoding, (char **) &stateIndex))
                *result += stateProb[stateIndex];
            FREE(encoding);
            return;
        case '0':
        case '1':
            evaluate_included_state_prob(encoding, stateProb, stateIndexTable,
                                         lineIndex + 1, result);
            return;
        case '-':encoding[lineIndex] = '0';
            evaluate_included_state_prob(util_strsav(encoding), stateProb,
                                         stateIndexTable, lineIndex + 1, result);
            encoding[lineIndex] = '1';
            evaluate_included_state_prob(encoding, stateProb, stateIndexTable,
                                         lineIndex + 1, result);
            return;
        default: fail("Bad encoding string!");
    }
}

/* The following routine calculates the probability of a function (given with
 *  bdd) being one, given the static input zero and one probabilities of its
 *  primary inputs in the zero and one array and the probability of the sets
 *  of PS lines
 */
double power_calc_func_prob_w_sets(bdd_t *f_bdd, power_pi_t *infoPIs, double *psProb, int nPSLines) {
    pset         sets;
    double       result;
    char         *value, *key;
    st_generator *gen;

    assert(f_bdd != NIL(bdd_t));

    visited = st_init_table(st_ptrcmp, st_ptrhash);
    PIInfo  = infoPIs;

    sets = set_full(2 * power_setSize);

    result = power_count_f_prob_w_sets(f_bdd, psProb, nPSLines, -1, sets);

    st_foreach_item(visited, gen, &key, &value) { FREE(value); }
    st_free_table(visited);

    return result;
}

/* Goes down the BDD until a PI is found, from which point 'power_count_prob'
 *  is called (state lines are all before PI lines). Each time we enter a new
 *  set (which can be determined by power_setSize) we multiply the current
 *  result by the sum of the probabilities of the combinations included
 *  in the previous set.
 */
static double power_count_f_prob_w_sets(bdd_t *f, double *psProb, int nPSLines, long prevInd, pset sets) {
    bdd_t  *child;
    pset   keepSets, otherSet;
    double result, *stored;
    long   topf;
    int    newSet;

    if (f->node == cmu_bdd_zero(f->mgr)) {
        set_free(sets);
        return 0.0;
    }
    if (f->node == cmu_bdd_one(f->mgr)) {
        result = 1.0;
        multiply_PS_prob(&result, prevInd, psProb, sets);
        set_free(sets);
        return result;
    }

    if (st_lookup(visited, (char *) f->node, (char **) &stored)) {
        result = *stored;
        multiply_PS_prob(&result, prevInd, psProb, sets);
        set_free(sets);
        return result;
    }

    topf = cmu_bdd_if_index(f->mgr, f->node);

    if (topf >= nPSLines) { /* First PI found, all PIs from now on */
        result = power_count_prob(f);
        multiply_PS_prob(&result, prevInd, psProb, sets);
        set_free(sets);
        return result;
    }

    newSet =
            (prevInd >= 0) && ((topf / power_setSize) != (prevInd / power_setSize));

    if (newSet) {
        keepSets = sets;
        sets     = set_full(2 * power_setSize);
    }

    otherSet = set_save(sets);
    set_remove(otherSet, 2 * (topf % power_setSize) + 1);
    child  = bdd_else(f);
    result = power_count_f_prob_w_sets(child, psProb, nPSLines, topf, otherSet);
    FREE(child);

    set_remove(sets, 2 * (topf % power_setSize));
    child = bdd_then(f);
    result += power_count_f_prob_w_sets(child, psProb, nPSLines, topf, sets);
    FREE(child);

    if (!(topf % power_setSize)) { /* First element in a set, can store result */
        stored = ALLOC(double, 1);
        *stored = result;
        st_insert(visited, (char *) f->node, (char *) stored);
    }

    if (newSet) {
        multiply_PS_prob(&result, prevInd, psProb, keepSets);
        set_free(keepSets);
    }

    return result;
}

static void multiply_PS_prob(double *result, long index, double *psProb, pset sets) {
    pset   inclSets;
    double probSum;
    long   setN;
    int    i, setSize;

    setN     = index / power_setSize;
    setSize  = 1 << power_setSize;
    inclSets = power_lines_in_set(sets, setSize);
    probSum  = 0.0;
    for (i   = 0; i < setSize; i++)
        if (is_in_set(inclSets, i))
            probSum += psProb[setN * setSize + i];

    *result *= probSum;

    set_free(inclSets);
}

#endif /* SIS */
