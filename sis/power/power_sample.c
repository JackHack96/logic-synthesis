/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/power/power_sample.c,v $
 * $Author: pchong $
 * $Revision: 1.3 $
 * $Date: 2004/03/16 19:11:05 $
 *
 */
/*---------------------------------------------------------------------------
|   The routines for power estimation using simulation are stored here:
|
|       power_sample_estimate()
|       power_simulation_estimate()
|
| Copyright (c) 1991 - Abhijit Ghosh. University of California, Berkeley
|
| Jose' Monteiro, MIT, Jun/93            jcm@rle-vlsi.mit.edu
+--------------------------------------------------------------------------*/

#include "sis.h"
#include "power_int.h"

#define RAND_RANGE 0x7fffffff


static double *power_get_PIs_prob();
static double  power_sample_do_estimate();
static double  calculate_power();
/* These routines were copied from ../sim/interpret.c, also static there */
static void    gen_random_pattern();
static void    simulate_network_word();
static long    simulate_network_word_rec();
static long    simulate_node_word();


/* Simple interface for power estimate using sampling */
int power_sample_estimate(network, type, delay, num_samples, total_power)
network_t *network;
int type;
int delay;
int num_samples;
double *total_power;
{
    int status;

    if(network == NIL(network_t)){
        fprintf(siserr, "Power estimation routine called with NIL network.\n");
        return 1;
    }

    /* Default values for global variables */
    power_setSize = 1;
    power_delta = DEFAULT_PS_MAX_ALLOWED_ERROR;
    power_verbose = 0;
    power_cap_in_latch = CAP_IN_LATCH;      /* Default I/O latch capacitances */
    power_cap_out_latch = CAP_OUT_LATCH;

    status = power_simulation_estimate(network, type, delay, APPROXIMATION_PS,
                                       num_samples, num_samples+1, NIL(FILE),
                                       FACTORED_FORM, DEFAULT_MAX_INPUT_SIZING,
                                       total_power);
    return status;
}


/* Interface with all options - calling routine must define GLOBAL variables:
   power_setSize, power_delta, power_verbose, power_cap_in/out_latch */
int power_simulation_estimate(network, type, delay, psProbOption, num_samples,
                              sample_gap, info_file, sop_or_fact, input_sizing,
                              total_power)
network_t *network;
int type;
int delay;
int psProbOption;
int num_samples;
int sample_gap;
FILE *info_file;
int sop_or_fact;
int input_sizing;
double *total_power;
{
    st_table *info_table;
    array_t *psOrder;
    network_t *symbolic;
    node_info_t *info;
    node_t *node;
    double *probPI,
           *psLineProb;
    int status;
    st_generator *gen;

    /* Calculate node capacitances */
    info_table = power_get_node_info(network, info_file, sop_or_fact,
                                     input_sizing);
    if (info_file) {
	fclose(info_file);
    }

    /* Calculate node delays */
    switch(delay){
      case 0:
        st_foreach_item(info_table, gen, (char **) &node, (char **) &info){
            if(info->delay == -1)      /* Otherwise already specified */
                info->delay = 0;
        }
        break;
      case 1:
        st_foreach_item(info_table, gen, (char **) &node, (char **) &info){
            if(info->delay == -1)      /* Otherwise already specified */
                info->delay = 1;
        }
        break;
      default:
        status = delay_trace(network, DELAY_MODEL_MAPPED);
        if(!status){
            fprintf(siserr, "Something's wrong! No library loaded?!...\n"); 
            return 1;
        }
        st_foreach_item(info_table, gen, (char **) &node, (char **) &info){
            if(info->delay == -1)      /* Otherwise already specified */
                info->delay = power_get_mapped_delay(node);
        }
    }

    /* First create the symbolic network */
    symbolic = power_symbolic_simulate(network, info_table);

    switch(type){
      case COMBINATIONAL:
        break;            /* Nothing to be done */
#ifdef SIS
      case PIPELINE:
        if(power_add_pipeline_logic(network, symbolic))
            return 1;
        break;
      case SEQUENTIAL:       /* For sampling, only PSlines probability */
        if(power_add_fsm_state_logic(network, symbolic))
            return 1;
        switch(psProbOption){
          case EXACT_PS:
          case STATELINE_PS:
            fprintf(siserr, "Warning, this version doesn't support exact state probabilities in sample mode.\nThe approximate method will be used.\n");
          case APPROXIMATION_PS:     /* Line prob. will be changed inside */
            if(power_setSize != 1){
                fprintf(siserr, "Warning, this version doesn't support correlation between PS lines in sample\nmode. The approximate method will be used with set size 1.\n");
                power_setSize = 1;
            }
            psLineProb = power_direct_PS_lines_prob(network, info_table,
                                                    &psOrder, symbolic);
            FREE(psLineProb);
            array_free(psOrder);
            break;
          case UNIFORM_PS:
            break;
          default:
            fprintf(siserr, "Internal error!!! Unknown option in power_simulation_estimate!\n");
            return 1;
        }
        break;
#endif /* SIS */
      case DYNAMIC:
        fprintf(siserr, "Error! Sorry, this version does not support dynamic circuits in sample mode.\n");
        return 1;
      default:
        fprintf(siserr, "Internal error!!! Unknown type in power_simulation_estimate!\n");
        return 1;
    }

    probPI = power_get_PIs_prob(symbolic, info_table);

    /* Now simulate network to estimate power */
    *total_power = power_sample_do_estimate(symbolic, num_samples, sample_gap,
                                            probPI);

    FREE(probPI);
    st_foreach_item(info_table, gen, (char **) &node, (char **) &info){
        FREE(info);
    }
    st_free_table(info_table);

    network_free(symbolic);

    return 0;
}


static double power_sample_do_estimate(network, num_iter, num_gap, probPI)
network_t *network;
int num_iter, num_gap;
double *probPI;
{
    double total_power;
    long mask,
         *pi_values,
         *po_values;
    int i, j, k,
        n_pi,
        n_po,
        seed,
        *n_ones,
        num_since_last = 0;

    n_pi = network_num_pi(network);
    n_po = network_num_po(network);
    pi_values = ALLOC(long, n_pi);
    po_values = ALLOC(long, n_po);
    n_ones = ALLOC(int, n_po);
    for(i = 0; i < n_po; i++)
        n_ones[i] = 0;

    for(i = 0; i < num_iter; i++, num_since_last++){
        seed = (int)time(0);
        gen_random_pattern(n_pi, pi_values, seed, probPI);
        simulate_network_word(network, pi_values, po_values);

        for(j = 0; j < n_po; j++){
            mask = 1;
            for(k = 0; k < sizeof(long) * 8; k++){
                n_ones[j] += ((po_values[j] & mask) != 0);
                mask <<= 1;
            }
        }

        if(num_since_last == num_gap){
            num_since_last = 0;
            fprintf(sisout, "Iteration %d :: Power %f\n",
                    (i+1) * sizeof(long) * 8,
                    calculate_power(network, n_ones, i+1));
        }
    }

    total_power = calculate_power(network, n_ones, i);
    FREE(pi_values);
    FREE(po_values);
    FREE(n_ones);

    return total_power;
}

static double calculate_power(network, n_ones, i)
network_t *network;
int *n_ones;
int i;
{
    node_t *po;
    power_info_t *power_info;
    int i_po;
    double power = 0.0;
    lsGen gen;

    i *= sizeof(long) * 8;
    i_po = 0;

    foreach_primary_output(network, gen, po){
        po = po->fanin[0]->copy;          /* Link to the original network */
        assert(st_lookup(power_info_table, (char*) po, (char**) &power_info));
        power_info->switching_prob = ((double) n_ones[i_po]) / i;
        power += ((((double) n_ones[i_po]) / i) * power_info->cap_factor);
        i_po++;
    }

    return power * CAPACITANCE * 250.0;
}


static double *power_get_PIs_prob(symbolic, info_table)
network_t *symbolic;
st_table *info_table;
{
    node_t *pi;
    node_info_t *info;
    double *probPI;
    int i;
    lsGen gen;

    probPI = ALLOC(double, network_num_pi(symbolic));

    i = 0;
    foreach_primary_input(symbolic, gen, pi){
        pi = pi->copy;                      /* Link to the original network */
        assert(st_lookup(info_table, (char *) pi, (char **) &info));
        probPI[i++] = info->prob_one;
    }

    return probPI;
}


static void gen_random_pattern(n, pattern, seed, probPI)
int n;
long *pattern;
int seed;
double *probPI;
{
    int i, j;

    srandom(seed);
    for(i = 0; i < n; i++){
        pattern[i] = 0;
        for(j = 0; j < sizeof(long) * 8; j++)
            if(random() < ((int) (probPI[i] * RAND_RANGE)))
                pattern[i] |= 1 << j;
    }
}

static void simulate_network_word(network, pi_values, po_values)
network_t *network;
long *pi_values;
long *po_values;
{
    st_table *table = st_init_table(st_ptrcmp, st_ptrhash);
    lsGen gen;
    node_t *pi,
           *po,
           *input;
    int i;

    i = 0;
    foreach_primary_input(network, gen, pi){
        st_insert(table, (char *) pi, (char *) pi_values[i]);
        i++;
    }
    i = 0;
    foreach_primary_output(network, gen, po){
        input = node_get_fanin(po, 0);
        po_values[i] = simulate_network_word_rec(network, table, input);
        i++;
    }
    st_free_table(table);
}

static long simulate_network_word_rec(network, table, node)
network_t *network;
st_table *table;
node_t *node;
{
    node_t *input;
    long result,
         **inputs;
    int i,
        n_inputs = node_num_fanin(node);
    char *value;

    if(st_lookup(table, (char *) node, &value))
        return (long) value;

    assert(node->type != PRIMARY_INPUT);
    inputs = ALLOC(long *, 4);
    for(i = 0; i < 4; i++)
        inputs[i] = ALLOC(long, n_inputs);
    for(i = 0; i < n_inputs; i++){
        input = node_get_fanin(node, i);
        inputs[ONE][i] = simulate_network_word_rec(network, table, input);
        inputs[ZERO][i] = ~ inputs[ONE][i];
        inputs[TWO][i] = ~ 0l;
    }
    result = simulate_node_word(node, inputs);
    st_insert(table, (char *) node, (char *) result);
    for(i = 0; i < 4; i++)
        FREE(inputs[i]);
    FREE(inputs);

    return result;
}

static long simulate_node_word(node, inputs)
node_t *node;
long **inputs;
{
    register int i, j;
    node_t *fanin;
    node_cube_t cube;
    long result = 0,
         and_result;
    int num_cubes = node_num_cube(node),
        literal;

    for(i = 0; i < num_cubes; i++){
        and_result = ~ 0l;
        cube = node_get_cube(node, i);
        foreach_fanin(node, j, fanin){
            literal = node_get_literal(cube, j);
            and_result &= inputs[literal][j];
        }
        result |= and_result;
    }

    return result;
}
