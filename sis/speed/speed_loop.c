
#include "speed_int.h"
#include "sis.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

/* Exported interface to the looping function */
void speed_loop_interface(network_t **network, double thresh, double coeff, int dist, delay_model_t model, int flag) {
    speed_global_t speed_param;

    (void) speed_fill_options(&speed_param, 0, NIL(char *));
    speed_param.thresh = thresh;
    speed_param.coeff  = coeff;
    speed_param.dist   = dist;
    speed_param.model  = model;
    speed_param.trace  = flag;

    speed_set_delay_data(&speed_param, 0 /* No library acceleration */);
    speed_up_loop(network, &speed_param);

    return;
}

/* Routines used to save the optimized network when CPUlimit is exceeded */

static network_t *saved_network;

/* Handle for the SIGXCPU exception */
void sp_dump_saved_network(int sig, int code, struct sigcontext *context) {
    char name[32], command[256];

    if (saved_network != NIL(network_t)) {
        (void) sprintf(command, "wl -n /usr/tmp/%s.blif.%d",
                       network_name(saved_network), getpid());
        com_execute(&saved_network, command);
        gethostname(name, 32);
        (void) fprintf(sisout, "Saving optimized network in %s on %s\n", command + 6,
                       name);
    } else {
        (void) fprintf(sisout, "Nothing needs to be saved\n");
    }

#if !defined(__hpux)
    /* Should exit */
    if (sig == SIGXCPU) {
        exit(1);
    }
#endif
}

/* Basic routine for performance optimization !!! */

void speed_up_loop(network_t **network, speed_global_t *speed_param) {
    int        i, status;
    network_t  *dup_cur_net, *cur_net;
    array_t    *array_of_methods = speed_param->local_trans;
    sp_xform_t *ti_model;
    double     best, cur, best_area, cur_area;
    char       *best_name, *cur_name;
    node_t     *best_node, *cur_node; /* critical output node */

    /* Check for trivial networks */
    if (network_num_pi(*network) == 0)
        return;

    /*****************  BEGIN MACRO **********************/
#define SWAP_AND_LOOP_AGAIN                                                    \
  cur = best;                                                                  \
  cur_area = best_area;                                                        \
  FREE(cur_name);                                                              \
  cur_name = best_name;                                                        \
  if (saved_network != NIL(network_t))                                         \
    network_free(saved_network);                                               \
  network_free(*network);                                                      \
  saved_network = speed_network_dup(cur_net);                                  \
  *network = cur_net;                                                          \
  if (speed_param->req_times_set && cur > NSP_EPSILON) {                       \
    if (speed_param->interactive) {                                            \
      (void)fprintf(sisout, "Timing constraints are met\n");                   \
    }                                                                          \
    goto free_saved_network;                                                   \
  }                                                                            \
  goto loop;
    /*****************  END MACRO **********************/

    saved_network = NIL(network_t);
    if (speed_param->trace) {
        if (speed_param->new_mode) {
            (void) fprintf(
                    sisout, "distance = %-2d, Selection = %s, %sAGG-B%s, Transforms: ",
                    speed_param->dist, SP_METHOD(speed_param->region_flag),
                    (speed_param->del_crit_cubes ? "" : "NON"),
                    (speed_param->transform_flag == BEST_BANG_FOR_BUCK ? "/C" : ""));
            for (i = 0; i < array_n(array_of_methods); i++) {
                ti_model = array_fetch(sp_xform_t *, array_of_methods, i);
                if (ti_model->on_flag)
                    (void) fprintf(sisout, " \"%s\"", ti_model->name);
            }
            (void) fprintf(sisout, "\n");
        } else {
            (void) fprintf(sisout, "distance = %-2d  threshold = %3.1f\n",
                           speed_param->dist, speed_param->thresh);
        }
    }
    /* Set the flag based on whether required times are set for PO nodes */

    assert(delay_trace(*network, speed_param->model));
    SP_GET_PERFORMANCE(speed_param, *network, cur_node, cur, cur_name, cur_area);
    if (speed_param->req_times_set && cur > NSP_EPSILON) {
        if (speed_param->interactive) {
            (void) fprintf(sisout, "Timing constraints are met\n");
        }
        return;
    }

#if !defined(__hpux)
    /* Insert a handler for the CPULIMIT */
    if (signal(SIGXCPU, sp_dump_saved_network) != 0) {
        (void) fprintf(sisout, "Unable to start SIGXCPU handler");
    }

    /* Another handler for the signal SIGUSR1 to dump intermediate networks */
    if (signal(SIGUSR1, sp_dump_saved_network) != 0) {
        (void) fprintf(sisout, "Unable to start SIGUSR1 handler");
    }

#endif
    loop:
    cur_net = network_dup(*network);
    if (speed_param->red_removal && speed_param->model != DELAY_MODEL_MAPPED) {
        (void) fprintf(sisout, "Be patient, running red removal ...\n");

        (void) com_redundancy_removal(&cur_net, 0, NIL(char *));

        /* Get a true estimate of the delay --- after red_removal */
        assert(delay_trace(cur_net, speed_param->model));
        SP_GET_PERFORMANCE(speed_param, *network, cur_node, cur, cur_name,
                           cur_area);
    }

    if (speed_param->new_mode) {
        status = new_speed(cur_net, speed_param);
    } else {
        speed_up_network(cur_net, speed_param);
    }

    assert(delay_trace(cur_net, speed_param->model));
    SP_GET_PERFORMANCE(speed_param, cur_net, best_node, best, best_name,
                       best_area);

    if (SP_IMPROVED(speed_param, best, cur)) {
        if (speed_param->trace) {
            SP_PRINT(speed_param, cur, best, cur_area, best_area, cur_name,
                     best_name);
        }
        SWAP_AND_LOOP_AGAIN;
    } else if (lib_network_is_mapped(cur_net)) {
        /* Carry out an area recovery step to perturb the network*/
        (void) nsp_downsize_non_crit_gates(cur_net, speed_param->model);
        SP_GET_PERFORMANCE(speed_param, cur_net, best_node, best, best_name,
                           best_area);
        if (SP_IMPROVED(speed_param, best, cur)) {
            (void) fprintf(sisout, "Downsizing transformed netw !!!\n");
            SP_PRINT(speed_param, cur, best, cur_area, best_area, cur_name,
                     best_name);
            SWAP_AND_LOOP_AGAIN;
        } else {
            /* Try to downsize the original network. Not the transformed one */
            dup_cur_net = network_dup(*network);
            delay_trace(dup_cur_net, speed_param->model);
            (void) nsp_downsize_non_crit_gates(dup_cur_net, speed_param->model);
            SP_GET_PERFORMANCE(speed_param, dup_cur_net, best_node, best, best_name,
                               best_area);
            if (SP_IMPROVED(speed_param, best, cur)) {
                network_free(cur_net);
                cur_net = dup_cur_net;
                (void) fprintf(sisout, "Downsizing saved netw !!!\n");
                SP_PRINT(speed_param, cur, best, cur_area, best_area, cur_name,
                         best_name);
                SWAP_AND_LOOP_AGAIN;
            } else {
                network_free(dup_cur_net);
            }
        }
    }

    /*
     * At this stage try once more to speed up
     */
    if (speed_param->new_mode) {
        if (status < 0) {
            /* No change was made to the network so no point in repeating */
            goto exit_after_print;
        } else {
            /* The earlier iteration changed network, give it another try */
            (void) new_speed(cur_net, speed_param);
        }
    } else {
        speed_up_network(cur_net, speed_param);
    }
    assert(delay_trace(cur_net, speed_param->model));
    FREE(best_name);
    SP_GET_PERFORMANCE(speed_param, cur_net, best_node, best, best_name,
                       best_area);

    if (SP_IMPROVED(speed_param, best, cur)) {
        if (speed_param->trace) {
            SP_PRINT(speed_param, cur, best, cur_area, best_area, cur_name,
                     best_name);
        }
        SWAP_AND_LOOP_AGAIN;
    }

    exit_after_print:
    if (speed_param->trace) {
        SP_PRINT(speed_param, cur, cur, cur_area, cur_area, cur_name, cur_name);
    }

    FREE(cur_name);
    FREE(best_name);
    network_free(cur_net);

    free_saved_network:
#if !defined(__hpux)
    /* terminate the special handler for handling cpulimit violations */
    if (signal(SIGXCPU, SIG_DFL) != 0) {
        (void) fprintf(sisout, "Unable to restore default handler");
    }
#endif

    if (saved_network != NIL(network_t)) {
        network_free(saved_network);
        saved_network = NIL(network_t);
    }
}

/*
 * For the original speedup (-f option), the script tries
 * the different values for the distance
 */
void speed_up_script(network_t **network, speed_global_t *speed_param) {
    if (speed_param->new_mode) {
        speed_param->dist = DEFAULT_SPEED_DIST;
        speed_up_loop(network, speed_param);
    } else {
        speed_param->dist = DEFAULT_SPEED_DIST;
        speed_up_loop(network, speed_param);
        speed_param->dist += 1;
        speed_up_loop(network, speed_param);
        speed_param->dist += 1;
        speed_up_loop(network, speed_param);
        speed_param->dist -= 1;
        speed_up_loop(network, speed_param);
        speed_param->dist -= 1;
        speed_up_loop(network, speed_param);
    }
}
