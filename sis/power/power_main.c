
/*---------------------------------------------------------------------------
|   The main routines for power estimation are stored here:
|
|       power_main_driver()
|       power_estimate()
|
|       power_command_line_interface()
|       power_get_node_info()
|       power_get_mapped_delay()
|
|   The remaining are auxiliary routines to determine delay and capacitance
| at each node in the network and define switching at the primary inputs.
|
| Copyright (c) 1991 - Abhijit Ghosh. University of California, Berkeley
|
| Jose' Monteiro, MIT, Jan/93            jcm@rle-vlsi.mit.edu
|  - Modified to be included in SIS (instead of Flames)
|  - Added power for primary inputs
|  - Added power_estimate routine
|  - Added pipelines
|  - Added state probabilities
|  - New interface
+--------------------------------------------------------------------------*/

#include "sis.h"
#define MAIN
#include "power_int.h"
#undef MAIN

static void power_fill_info_table_from_file();
static int  load_sum_of_products();
static int  load_factored_form();
static void power_usage();


int power_command_line_interface(network, argc, argv)
network_t *network;
int argc;
char **argv;
{
    FILE *info_file;
    int i,
        status,
        mode,
        delay,
        type,
        psProbOption,
        sop_or_fact,
        input_sizing,
        num_samples,
        sample_gap;
    double total_power;

    /* Default values */
    mode = BDD_MODE;
    delay = 0;                 /* Default is zero delay model */
    type = COMBINATIONAL;
    psProbOption = APPROXIMATION_PS;
    power_setSize = 1;
    power_delta = DEFAULT_PS_MAX_ALLOWED_ERROR;
    num_samples = 100;
    info_file = NIL(FILE);

    sop_or_fact = FACTORED_FORM;
    input_sizing = DEFAULT_MAX_INPUT_SIZING;
    sample_gap = INFINITY;       /* Don't show intermediate value in sampling */
    power_verbose = 0;
    power_cap_in_latch = CAP_IN_LATCH;      /* Default I/O latch capacitances */
    power_cap_out_latch = CAP_OUT_LATCH;

    /* Check arguments */
    for(i = 1; i != argc; )
        switch(argv[i++][1]){
          case 'm':
            switch(argv[i][0]){
              case 'b':
              case 'B':
                mode = BDD_MODE;
                break;
              case 's':
              case 'S':
                mode = SAMPLE_MODE;
                break;
              default:
                fprintf(siserr,
                        "Invalid mode: %s. Only BDD or SAMPLE.\n",
                        argv[i]);
                return 1;
            }
            i++;
            continue;
          case 'd':
            switch(argv[i][0]){
              case 'z':
              case 'Z':
                delay = 0;
                break;
              case 'u':
              case 'U':
                delay = 1;
                break;
              case 'g':
              case 'G':
                delay = 2;
                break;
              default:
                fprintf(siserr,
                        "Invalid delay: %s. One of ZERO, UNIT or GENERAL.\n",
                        argv[i]);
                return 1;
            }
            i++;
            continue;
          case 't':
            switch(argv[i][0]){
              case 'c':
              case 'C':
                type = COMBINATIONAL;
                break;
#ifdef SIS
              case 's':
              case 'S':
                if(network_num_latch(network))
                    type = SEQUENTIAL;
                else
                    fprintf(siserr, "Warning, non sequential network. Combinational power will be reported.\n");
                break;
              case 'p':
              case 'P':
                if(network_num_latch(network))
                    type = PIPELINE;
                else
                    fprintf(siserr, "Warning, non sequential network. Combinational power will be reported.\n");
                break;
#endif
              case 'd':
              case 'D':
                type = DYNAMIC;
                break;
              default:
                fprintf(siserr, "Invalid type: %s. One of COMBINATIONAL",
                        argv[i]);
#ifdef SIS
                fprintf(siserr, ", SEQUENTIAL, PIPELINE");
#endif
                fprintf(siserr, " or DYNAMIC.\n");
                return 1;
            }
            i++;
            continue;
#ifdef SIS
          case 's':          /* Only used for sequential type circuits */
            switch(argv[i][0]){
              case 'a':
              case 'A':
                psProbOption = APPROXIMATION_PS;
                break;
              case 'e':
              case 'E':
                if(network_num_latch(network) > 16){
                    fprintf(siserr, "Too many states! Maximum number of latches for exact method is 16.\n");
                    return 1;
                }
                psProbOption = EXACT_PS;
                break;
/*            case 'l':                 * Not very interesting - remove
              case 'L':
                if(network_num_latch(network) > 16){
                    fprintf(siserr, "Too many states! Maximum number of latches for exact method is 16.\n");
                    return 1;
                }
                psProbOption = STATELINE_PS;
                break;
*/            case 'u':
              case 'U':
                psProbOption = UNIFORM_PS;
                break;
              default:
                fprintf(siserr,
                        "Invalid PS prob calculation: %s. One of APPROXIMATION, EXACT or UNIFORM.\n",
                        argv[i]);
                return 1;
            }
            i++;
            continue;
          case 'a': /* Only for APPROXIMATION_PS, correlation between PS lines*/
            power_setSize = atoi(argv[i]);
            if((power_setSize < 1) || (power_setSize > 16)){
                fprintf(siserr, "Invalid PS lines set size: %d. Limits are 1 to 16.\n", power_setSize);
                return 1;
            }
            i++;
            continue;
          case 'e':    /* Only for APPROX_PS, error allowed for PS lines prob */
            power_delta = atof(argv[i]);
            if(power_delta <= 0){
                fprintf(siserr, "Invalid error: %f. Must be a positive value.\n", power_delta);
                return 1;
            }
            i++;
            continue;
#endif
          case 'n':                        /* Only used for sampling mode */
            num_samples = atoi(argv[i]);
            i++;
            continue;
          case 'f':
            info_file = fopen(argv[i], "r");
            if(info_file == NIL(FILE)){
                fprintf(siserr, "Cannot open information file: %s\n", argv[i]);
                return 1;
            }
            i++;
            continue;
          case 'h':
            power_usage();
            return 0;
/* Undocumented options: might be useful for someone else, so leave them here */
#ifdef SIS
          case 'R': /* Sets latch capacitances to 0, only comb power reported */
            power_cap_in_latch = power_cap_out_latch = 0;
            continue;
#endif
          case 'M':    /* Until how many inputs we assume transistor resizing */
            input_sizing = atoi(argv[i]);
            i++;
            continue;
          case 'S':
            sop_or_fact = SUM_OF_PRODUCTS;
            continue;
          case 'N':
            sample_gap = atoi(argv[i]);
            i++;
            continue;
          case 'V':
            power_verbose = atoi(argv[i]);
            i++;
            continue;
          default:            /* Came here => Bad option */
            fprintf(siserr, "Option %s not understood\n", argv[i-1]);
            power_usage();
            return 1;
        }

    if(mode == BDD_MODE)
        status = power_main_driver(network, type, delay, psProbOption,
                                   info_file, sop_or_fact, input_sizing,
                                   &total_power);
    else
        status = power_simulation_estimate(network, type, delay, psProbOption,
                                           num_samples, sample_gap, info_file,
                                           sop_or_fact, input_sizing,
                                           &total_power);
    if(status)
        return 1;

    /* Print result */
    switch(type){
      case COMBINATIONAL:
        fprintf(sisout, "Combinational power estimation, ");
        break;
#ifdef SIS
      case SEQUENTIAL:
        fprintf(sisout, "Sequential power estimation");
        switch(psProbOption){
          case APPROXIMATION_PS:
            if(power_setSize != 1){
                fprintf(sisout, " (setsize = %d", power_setSize);
                if(power_delta != DEFAULT_PS_MAX_ALLOWED_ERROR)
                    fprintf(sisout, ", maxerror = %f),\n", power_delta);
                else
                    fprintf(sisout, "), ");
            }
            else
                if(power_delta != DEFAULT_PS_MAX_ALLOWED_ERROR)
                    fprintf(sisout, " (maxerror = %f), ", power_delta);
                else
                    fprintf(sisout, ", ");
            break;
          case EXACT_PS:
            fprintf(sisout, " (Exact method), ");
            break;
          case UNIFORM_PS:
            fprintf(sisout, " (Uniform method), ");
        }
        break;
      case PIPELINE:
        fprintf(sisout, "Pipeline power estimation, ");
        break;
#endif
      case DYNAMIC:
        fprintf(sisout, "Dynamic power estimation.\n");
    }
    if(type != DYNAMIC)
        switch(delay){
          case 0:
            fprintf(sisout, "with Zero delay model.\n");
            break;
          case 1:
            fprintf(sisout, "with Unit delay model.\n");
            break;
          default:
            fprintf(sisout, "with General delay model.\n");
        }
    if(mode == SAMPLE_MODE)
        fprintf(sisout, "Using Sampling, #vectors = %d\n",
                num_samples * sizeof(long) * 8);

    fprintf(sisout, "Network: %s, Power = %.1f uW assuming 20 MHz clock and Vdd = 5V\n",
            network_name(network), total_power);

    return 0;
}


/* Simple interface (Note: corresponding sample routine is in power_sample.c) */
int power_estimate(network, type, delay, total_power)
network_t *network;
int type;
int delay;
double *total_power;
{
    int status;

    if(network == NIL(network_t)){
        fprintf(siserr, "Power estimation routine called with NULL network.\n");
        return 1;
    }

    /* Default values for global variables */
    power_setSize = 1;
    power_delta = DEFAULT_PS_MAX_ALLOWED_ERROR;
    power_verbose = 0;
    power_cap_in_latch = CAP_IN_LATCH;      /* Default I/O latch capacitances */
    power_cap_out_latch = CAP_OUT_LATCH;

    status = power_main_driver(network, type, delay, APPROXIMATION_PS,
                               NIL(FILE), FACTORED_FORM,
                               DEFAULT_MAX_INPUT_SIZING, total_power);
    return status;
}


/* Interface with all options - calling routine must define GLOBAL variables:
   power_setSize, power_delta, power_verbose, power_cap_in/out_latch */
int power_main_driver(network, type, delay, psProbOption, info_file,
                      sop_or_fact, input_sizing, total_power)
network_t *network;
int type;
int psProbOption;
FILE *info_file;
int sop_or_fact;
int input_sizing;
double *total_power;
{
    st_table *info_table;
    node_t *node;
    node_info_t *info;
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

    /* Now we have info needed, call the appropriate routine */
    switch(type){
      case COMBINATIONAL:
        switch(delay){
          case 0:
            status = power_comb_static_zero(network, info_table, total_power);
            break;
          default:
            status = power_comb_static_arbit(network, info_table, total_power);
        }
        break;
#ifdef SIS
      case SEQUENTIAL:
        status = power_seq_static_arbit(network, info_table, psProbOption,
                                        total_power);
        break;
      case PIPELINE:
        status = power_pipe_arbit(network, info_table, total_power);
        break;
#endif
      case DYNAMIC:
        status = power_dynamic(network, info_table, total_power);
        break;
      default:
        fprintf(siserr, "Internal error!!! Unknown mode in power_main_driver!\n");
        return 1;
    }

    st_foreach_item(info_table, gen, (char **) &node, (char **) &info){
        FREE(info);
    }
    st_free_table(info_table);

    return status;
}


st_table *power_get_node_info(network, file, sop_or_fact, input_sizing)
network_t *network;
FILE *file;
int sop_or_fact;
int input_sizing;
{
    st_table *info_table;
    node_t *node;
    power_info_t *power_info;
    node_info_t *node_info;
    lsGen gen;

    if(power_info_table != NIL(st_table))
        power_free_info();

    power_info_table = st_init_table(st_ptrcmp, st_ptrhash);

    info_table = st_init_table(st_ptrcmp, st_ptrhash);

    power_fill_info_table_from_file(info_table, network, file, sop_or_fact,
                                    input_sizing);

    foreach_node(network, gen, node){

        if(st_lookup(power_info_table, (char *) node, (char **) &power_info))
            continue;                /* Info already specified from the file */

        power_info = ALLOC(power_info_t, 1);
        node_info = ALLOC(node_info_t, 1);

        if(node_function(node) == NODE_PI){
            if(network_is_real_pi(network, node))
                power_info->cap_factor = 0;
            else       /* Latch output */
                power_info->cap_factor = power_cap_out_latch;
            if(sop_or_fact == SUM_OF_PRODUCTS)
                power_info->cap_factor +=
                    load_sum_of_products(network, node, input_sizing);
            else
                power_info->cap_factor +=
                    load_factored_form(network, node, input_sizing);
            node_info->delay = -1;                 /* Mark it not initialized */
            node_info->prob_one = 0.5;
        }
        else if(node_function(node) == NODE_PO){  /* These won't be used */
            power_info->cap_factor = 0;
            node_info->delay = -1;            /* Mark it not initialized */
            node_info->prob_one = 0.0;
        }
        else{  /* This is an internal node */
            if(sop_or_fact == SUM_OF_PRODUCTS)
                power_info->cap_factor = /* Output load + Internal dissipation*/
                    load_sum_of_products(network, node, input_sizing) +
                    node_num_literal(node) / 2;
            else
                power_info->cap_factor =
                    load_factored_form(network, node, input_sizing) +
                    factor_num_literal(node) / 2;
            node_info->delay = -1;                 /* Mark it not initialized */
            node_info->prob_one = 0.0;                       /* Won't be used */
        }
        power_info->switching_prob = 0.0;
        st_insert(power_info_table, (char *) node, (char *) power_info);
        st_insert(info_table, (char *) node, (char *) node_info);
    }

    return info_table;
}


static void power_fill_info_table_from_file(info_table, network, file,
                                           sop_or_fact, input_sizing)
st_table *info_table;
network_t *network;
FILE *file;
int sop_or_fact;
int input_sizing;
{
    char buffer[100];
    power_info_t *power_info;
    node_info_t *node_info;
    node_t *node;

    if(file == NIL(FILE)) /* Nothing to read */
        return;

    while(1){
        if(fscanf(file, "%s", buffer) != EOF){
          TOP:
            if(!strcmp(buffer, "name")){ /* New gate record beginning */
                power_info = ALLOC(power_info_t, 1);
                node_info = ALLOC(node_info_t, 1);
                /* Put in the default values in the info structure */
                if(fscanf(file, "%s", buffer) == EOF){
                    fprintf(siserr, "Sudden end of file in input\n");
                    FREE(power_info);
                    FREE(node_info);
                    return;
                }
                node = network_find_node(network, buffer);
                if(node == NIL(node_t)){
                    fprintf(siserr, "Cannot find node %s in network\n", buffer);
                    /* Scan in but ignore rest of this record */
                    while(fscanf(file, "%s", buffer) != EOF){
                        if(!strcmp(buffer, "name"))
                            break; /* Got to a new record */
                    }
                    if(!strcmp(buffer, "name"))
                        goto TOP;
                    else
                        goto BRKPT;
                }
                power_info->switching_prob = 0.0;
                st_insert(power_info_table, (char *) node, (char *) power_info);
                st_insert(info_table, (char *) node, (char *) node_info);
                if(node_function(node) == NODE_PI){
                    if(network_is_real_pi(network, node))
                        power_info->cap_factor = 0;
                    else       /* Latch output */
                        power_info->cap_factor = power_cap_out_latch;
                    if(sop_or_fact == SUM_OF_PRODUCTS)
                        power_info->cap_factor +=
                            load_sum_of_products(network, node, input_sizing);
                    else
                        power_info->cap_factor +=
                            load_factored_form(network, node, input_sizing);
                    node_info->delay = -1;         /* Mark it not initialized */
                    node_info->prob_one = 0.0;
                }
                else if(node_function(node) == NODE_PO){     /* Won't be used */
                    power_info->cap_factor = 0;
                    node_info->delay = 0;
                    node_info->prob_one = 0.0;
                }
                else{     /* This is an internal node */
                    if(sop_or_fact == SUM_OF_PRODUCTS)
                        power_info->cap_factor =
                            load_sum_of_products(network, node, input_sizing) +
                            node_num_literal(node) / 2;
                    else
                        power_info->cap_factor =
                            load_factored_form(network, node, input_sizing) +
                            factor_num_literal(node) / 2;
                    node_info->delay = -1;         /* Mark not it initialized */
                    node_info->prob_one = 0.0;               /* Won't be used */
                }
            } /* End if(!strcmp(buffer, "name")) */

            /* Now we have the default values in there. Should read in
               non-default values, provided in file */

            if(!strcmp(buffer, "cap_factor")){
                if(fscanf(file, "%s", buffer) == EOF){
                    fprintf(siserr, "Sudden end in information file\n");
                    return;
                }
                power_info->cap_factor = atoi(buffer);
                continue;
            }
            if(!strcmp(buffer, "delay")){
                if(fscanf(file, "%s", buffer) == EOF){
                    fprintf(siserr, "Sudden end in information file\n");
                    return;
                }
                node_info->delay = atoi(buffer);
                continue;
            }
            if(!strcmp(buffer, "p0")){
                if(fscanf(file, "%s", buffer) == EOF){
                    fprintf(siserr, "Sudden end in information file\n");
                    return;
                }
                node_info->prob_one = 1.0 - atof(buffer);
                continue;
            }
            if(!strcmp(buffer, "p1")){
                if(fscanf(file, "%s", buffer) == EOF){
                    fprintf(siserr, "Sudden end in information file\n");
                    return;
                }
                node_info->prob_one = atof(buffer);
                continue;
            }
        } /* Ends fscanf main loop */
        else{
          BRKPT:
            break; /* Get out of the loop */
        }
    } /* End while(1) */
}


/*
*  Assume that a delay trace has been performed.
*  Obtain the delay values of the nodes.
*  Multiply by 5 to return an integer value.
*/
int power_get_mapped_delay(node)
node_t *node;
{
    node_t *fanin;
    delay_time_t delay;
    double max,
           act_delay;
    int i,
        int_delay;

    max = -1.0;
    foreach_fanin(node, i, fanin){
        delay = delay_arrival_time(fanin);
        if(delay.rise >= delay.fall)
            if(delay.rise > max)
                max = delay.rise;
        else
            if(delay.fall > max)
                max = delay.fall;
    }

    delay = delay_arrival_time(node);
    if(delay.rise >= delay.fall)
        act_delay = delay.rise - max;
    else
        act_delay = delay.fall - max;

    int_delay = (int)((act_delay + 0.1) / 0.2);

    return int_delay;
}


static int load_sum_of_products(network, node, input_sizing)
network_t *network;
node_t *node;
int input_sizing;
{
    node_t *fanout,
           *fanin;
    power_info_t *power_info;
    int i,
        total = 0,
        num_fanin,
        *literal_count;
    lsGen gen;

    foreach_fanout(node, gen, fanout){
        if(node_function(fanout) == NODE_PO){
            if(network_is_real_po(network, fanout)){
                if(st_lookup(power_info_table, (char *) fanout,
                             (char **) &power_info))
                    /* Yes the information for this output has been specified */
                    total += power_info->cap_factor;
            }
            else            /* It is a latch input */
                total += power_cap_in_latch;
        }
        else{
            literal_count = node_literal_count(fanout);

            num_fanin = node_num_fanin(fanout);
            if(num_fanin > input_sizing)
                num_fanin = input_sizing;

            foreach_fanin(fanout, i, fanin){
                if(fanin == node)
                    break;
            }
            total += num_fanin * (literal_count[2*i] + literal_count[2*i + 1]);
            FREE(literal_count);
        }/*Number of inputs to the node determines the size of the transistors*/
    }    /* times the number of times this input is used (+ and - phase) */

    return total;
}


static int load_factored_form(network, node, input_sizing)
network_t *network;
node_t *node;
int input_sizing;
{
    node_t *fanout;
    power_info_t *power_info;
    int total = 0,
        num_fanin;
    lsGen gen;

    foreach_fanout(node, gen, fanout){
        if(node_function(fanout) == NODE_PO){
            if(network_is_real_po(network, fanout)){
                if(st_lookup(power_info_table, (char *) fanout,
                             (char **) &power_info))
                    /* Yes the information for this output has been specified */
                    total += power_info->cap_factor;
            }
            else            /* It is a latch input */
                total += power_cap_in_latch;
        }
        else{
            num_fanin = node_num_fanin(fanout);
            if(num_fanin > input_sizing)
                num_fanin = input_sizing;

            total += num_fanin * factor_num_used(fanout, node);

        }/*Number of inputs to the node determines the size of the transistors*/
    }      /* times the number of times this input is used (+ and - phase) */

    return total;
}


static void power_usage()
{
    fprintf(siserr, "Power estimation program for CMOS Circuits\n");
#ifdef SIS
    fprintf(siserr, "Usage: power_estimate [-m c] [-d c] [-t c] [-s c] [-a n] [-e f] [-n n] [-f file]\n");
#else
    fprintf(siserr, "Usage: power_estimate [-m c] [-d n] [-t c] [-n n] [-f file]\n");
#endif
    fprintf(siserr, "-m c: estimation mode, c either SAMPLING or BDD (default).\n");
    fprintf(siserr, "-d c: delay model, c one of ZERO (default), UNIT or GENERAL (from library).\n");
#ifdef SIS
    fprintf(siserr, "-t c: estimation type, c one of COMBINATIONAL (default), SEQUENTIAL,\n      PIPELINE or DYNAMIC (for dynamic domino circuits).\n");
    fprintf(siserr, "-s c: PS lines probability calculation method, c one of APPROXIMATION (default),\n      EXACT or UNIFORM (0.5 is used). Only used for SEQUENTIAL type.\n");
    fprintf(siserr, "-a n: number of PS lines to be correlated (default 1). Only used for\n      the APPROXIMATION method.\n");
    fprintf(siserr, "-e f: maximum error allowed for PS lines probabilities (default 0.01). Only\n      used for the APPROXIMATION method.\n");
#else
    fprintf(siserr, "-t c: estimation type, c one of COMBINATIONAL (default) or DYNAMIC.\n");
#endif
    fprintf(siserr, "-n n: number of sets of %d input vectors to be simulated (default 100). Only\n      used for SAMPLING mode.\n", sizeof(long) * 8);
    fprintf(siserr, "-f file: specify file with input probabilities, node capacitance and delay.\n");
}
