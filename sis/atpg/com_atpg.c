#include "atpg_int.h"
#include "sis.h"
#include <setjmp.h>
#include <signal.h>

static void print_usage();

static jmp_buf timeout_env;
static void timeout_handle() { longjmp(timeout_env, 1); }

init_atpg() {
  com_add_command("atpg", com_atpg, 1);
  com_add_command("red_removal", com_redundancy_removal, 1);
  com_add_command("short_tests", com_short_tests, 1);
}

end_atpg() {}

static void print_usage() {
  fprintf(sisout, "usage: atpg [-dfFhnrRptvy] [file]\n");
  fprintf(sisout, "-d\tdepth of RTG sequences (default is STG depth)\n");
  fprintf(sisout, "-f\tno fault simulation\n");
  fprintf(sisout, "-F\tno reverse fault simulation\n");
  fprintf(sisout, "-h\tuse fast SAT; no non-local implications\n");
  fprintf(sisout, "-n\tnumber of sequences to fault simulate at one time\n");
  fprintf(
      sisout,
      "\t(default is system word length; n must be less than this length)\n");
  fprintf(sisout, "-r\tno RTG\n");
  fprintf(sisout, "-R\tno random propagation\n");
  fprintf(sisout, "-p\tno product machines, i.e. no fault-free propagation or "
                  "\n\tgood/faulty PMT\n");
  fprintf(sisout, "-t\tperform tech decomp of network\n");
  fprintf(sisout, "-v\tverbosity\n");
  fprintf(sisout, "-y\tlength of random prop sequences (default is 20)\n");
  fprintf(sisout, "file\toutput file for test patterns\n");
}

int com_atpg(network, argc, argv) network_t **network;
int argc;
char **argv;
{
  long begin_time, time, last_time;
  network_t *dc_network;
  int cnt, stg_depth, n_tests, n_det_tests;
  atpg_info_t *info;
  atpg_ss_info_t *ss_info, *exdc_ss_info;
  seq_info_t *seq_info;
  fault_t *fault;
  lsHandle handle;
  lsGeneric data;
  atpg_options_t *atpg_opt;
  sequence_t *test_sequence;
  fault_pattern_t *fault_info;

  if ((*network) == NIL(network_t))
    return 0;
  if (network_num_internal(*network) == 0)
    return 0;

  last_time = begin_time = util_cpu_time();
  info = atpg_info_init(*network);
  info->seq = (network_num_latch(*network) ? TRUE : FALSE);
  atpg_opt = info->atpg_opt;
  if (!set_atpg_options(info, argc, argv)) {
    print_usage();
    atpg_free_info(info);
    return 1;
  }
  if (atpg_opt->timeout > 0) {
    (void)signal(SIGALRM, timeout_handle);
    (void)alarm((unsigned int)atpg_opt->timeout);
    if (setjmp(timeout_env) > 0) {
      fprintf(sisout, "timeout occurred after %d seconds\n", atpg_opt->timeout);
      atpg_free_info(info);
      return 1;
    }
  }
  if (atpg_opt->tech_decomp) {
    decomp_tech_network(*network, INFINITY, INFINITY);
  }

  ss_info = atpg_sim_sat_info_init(*network, info);
  if (info->seq) {
    seq_info = atpg_seq_info_init(info);
  } else {
    seq_info = NIL(seq_info_t);
  }
  atpg_gen_faults(info);
  atpg_sim_setup(ss_info);
  atpg_comb_sim_setup(ss_info);
  atpg_sat_init(ss_info);

  /* external don't cares (only used for combinational circuits) */
  if (info->seq || ((dc_network = network_dc_network(*network)) == NULL)) {
    exdc_ss_info = NIL(atpg_ss_info_t);
  } else {
    exdc_ss_info = atpg_sim_sat_info_init(dc_network, info);
    atpg_sim_setup(exdc_ss_info);
    atpg_comb_sim_setup(exdc_ss_info);
    atpg_exdc_sim_link(exdc_ss_info, ss_info);
    atpg_sat_init(exdc_ss_info);
  }

  info->statistics->initial_faults = lsLength(info->faults);
  if (atpg_opt->verbosity > 0) {
    fprintf(sisout, "%d total faults\n", lsLength(info->faults));
  }

  if (info->seq) {
    seq_setup(seq_info, info);
    if (atpg_opt->build_product_machines) {
      seq_product_setup(info, seq_info, info->network);
      copy_orig_bdds(seq_info);
      atpg_product_setup_seq_info(seq_info);
    }
    record_reset_state(seq_info, info);
  }

  time = util_cpu_time();
  info->time_info->setup = (time - last_time);
  last_time = time;

  if (info->seq) {
    assert(calculate_reachable_states(seq_info));
    seq_info->valid_states_network =
        convert_bdd_to_network(seq_info, seq_info->range_data->total_set);
    time = util_cpu_time();
    info->time_info->traverse_stg = (time - last_time);
    last_time = time;
    stg_depth = array_n(seq_info->reached_sets);
    info->statistics->stg_depth = stg_depth;
    atpg_sat_node_info_setup(seq_info->valid_states_network);
    atpg_setup_seq_info(info, seq_info, stg_depth);
  } else {
    info->time_info->traverse_stg = 0;
    info->statistics->stg_depth = 0;
  }

  /* I think that the rtg_depth should depend on the STG depth, but I'm
   * not sure that this exact length is the best choice...
   */
  n_tests = 0;
  if (atpg_opt->rtg) {
    if (info->seq) {
      if (atpg_opt->rtg_depth == -1)
        atpg_opt->rtg_depth = stg_depth;
    } else {
      atpg_opt->rtg_depth = 1;
    }
    info->tested_faults =
        atpg_random_cover(info, ss_info, exdc_ss_info, TRUE, &n_tests);
    info->statistics->n_RTG_tested = lsLength(info->tested_faults);
  } else {
    info->tested_faults = lsCreate();
  }
  if (atpg_opt->verbosity > 0) {
    fprintf(sisout, "%d faults covered by RTG\n",
            lsLength(info->tested_faults));
  }
  time = util_cpu_time();
  info->time_info->RTG = (time - last_time);
  last_time = time;

  cnt = 0;
  n_det_tests = 0;
  while (lsFirstItem(info->faults, (lsGeneric *)&fault, &handle) == LS_OK) {
    if (atpg_opt->verbosity > 0) {
      atpg_print_fault(sisout, fault);
    }
    /* find test using three-step algorithm and fault-free assumption */
    test_sequence =
        generate_test(fault, info, ss_info, seq_info, exdc_ss_info, cnt);

    switch (fault->status) {
    case REDUNDANT:
      if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "Redundant\n");
      }
      lsNewEnd(info->redundant_faults, (lsGeneric)fault, 0);
      if (atpg_opt->reverse_fault_sim) {
        fault_info = ALLOC(fault_pattern_t, 1);
        fault_info->node = fault->node;
        fault_info->fanin = fault->fanin;
        fault_info->value = (fault->value == S_A_0) ? 0 : 1;
        st_insert(info->redund_table, (char *)fault_info, (char *)0);
      }
      lsRemoveItem(handle, (lsGeneric *)&data);
      break;
    case ABORTED:
      if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "Aborted by SAT\n");
      }
      lsNewEnd(info->untested_faults, (lsGeneric)fault, 0);
      lsRemoveItem(handle, &data);
      break;
    case UNTESTED:
      if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "Untested\n");
      }
      lsNewEnd(info->untested_faults, (lsGeneric)fault, 0);
      lsRemoveItem(handle, &data);
      break;
    case TESTED:
      n_tests++;
      n_det_tests++;
      if (ATPG_DEBUG) {
        assert(atpg_verify_test(ss_info, fault, test_sequence));
      }
      if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "Tested\n");
      }
      if (atpg_opt->fault_simulate) {
        ss_info->alloc_sequences[cnt] = test_sequence;
        cnt++;
      }
      lsNewEnd(info->tested_faults, (lsGeneric)fault, 0);
      lsRemoveItem(handle, &data);
      test_sequence->n_covers = 1;
      test_sequence->index = n_tests;
      fault->sequence = test_sequence;
      st_insert(info->sequence_table, (char *)test_sequence, (char *)0);
      if (atpg_opt->fault_simulate &&
          (cnt == atpg_opt->n_sim_sequences || n_det_tests < 3)) {
        last_time = util_cpu_time();
        fault_simulate(info, ss_info, exdc_ss_info, cnt);
        cnt = 0;
        reset_word_vectors(ss_info);
        time = util_cpu_time();
        info->time_info->fault_simulate += (time - last_time);
      }
      break;
    default:
      fail("bad fault->status returned by generate_test");
      break;
    }
    if (atpg_opt->verbosity > 0) {
      fprintf(sisout, "%d faults remaining\n",
              lsLength(info->faults) + lsLength(info->untested_faults));
    }
  }
  info->statistics->n_untested_by_main_loop = lsLength(info->untested_faults);

  if (info->seq && atpg_opt->build_product_machines &&
      (info->statistics->n_untested_by_main_loop > 0)) {
    last_time = util_cpu_time();
    while (lsFirstItem(info->untested_faults, (lsGeneric *)&fault, &handle) ==
           LS_OK) {
      if (atpg_opt->verbosity > 0) {
        atpg_print_fault(sisout, fault);
      }
      /* find test using good/faulty product machine traversal */
      last_time = util_cpu_time();
      test_sequence =
          generate_test_using_verification(fault, info, ss_info, seq_info, cnt);
      time = util_cpu_time();
      info->time_info->product_machine_verify += (time - last_time);
      switch (fault->status) {
      case REDUNDANT:
        if (atpg_opt->verbosity > 0) {
          fprintf(sisout, "Redundant\n");
        }
        lsNewEnd(info->redundant_faults, (lsGeneric)fault, 0);
        if (atpg_opt->reverse_fault_sim) {
          fault_info = ALLOC(fault_pattern_t, 1);
          fault_info->node = fault->node;
          fault_info->fanin = fault->fanin;
          fault_info->value = (fault->value == S_A_0) ? 0 : 1;
          st_insert(info->redund_table, (char *)fault_info, (char *)0);
        }
        lsRemoveItem(handle, &data);
        break;
      case TESTED:
        n_tests++;
        if (ATPG_DEBUG) {
          assert(atpg_verify_test(ss_info, fault, test_sequence));
        }
        if (atpg_opt->verbosity > 0) {
          fprintf(sisout, "Tested\n");
        }
        if (atpg_opt->fault_simulate) {
          ss_info->alloc_sequences[cnt] = test_sequence;
          cnt++;
        }
        lsNewEnd(info->tested_faults, (lsGeneric)fault, 0);
        lsRemoveItem(handle, &data);
        test_sequence->n_covers = 1;
        test_sequence->index = n_tests;
        fault->sequence = test_sequence;
        st_insert(info->sequence_table, (char *)test_sequence, (char *)0);
        if (atpg_opt->fault_simulate && cnt == atpg_opt->n_sim_sequences) {
          last_time = util_cpu_time();
          fault_simulate(info, ss_info, exdc_ss_info, cnt);
          cnt = 0;
          reset_word_vectors(ss_info);
          time = util_cpu_time();
          info->time_info->fault_simulate += (time - last_time);
        }
        break;
      default:
        fail("bad fault->status returned by PMT");
        break;
      }
      if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "%d faults remaining\n",
                lsLength(info->untested_faults));
      }
    }
  }

  if (atpg_opt->reverse_fault_sim) {
    reverse_fault_simulate(info, ss_info);
  }
  print_and_destroy_sequences(info);
  info->time_info->total_time = (util_cpu_time() - begin_time);
  atpg_print_results(info, seq_info);
  if (exdc_ss_info != NIL(atpg_ss_info_t)) {
    atpg_sim_unsetup(exdc_ss_info);
    atpg_comb_sim_unsetup(exdc_ss_info);
    atpg_sim_free(exdc_ss_info);
    atpg_sat_free(exdc_ss_info);
    FREE(exdc_ss_info);
  }

  atpg_sim_unsetup(ss_info);
  atpg_comb_sim_unsetup(ss_info);
  atpg_sim_free(ss_info);
  atpg_sat_free(ss_info);
  FREE(ss_info);
  if (info->seq) {
    if (atpg_opt->build_product_machines) {
      seq_info_product_free(seq_info);
    }
    seq_info_free(info, seq_info);
  }
  lsDestroy(info->tested_faults, free_fault);
  lsDestroy(info->faults, 0);
  atpg_free_info(info);
  sm_cleanup();
  fast_avl_cleanup();
  return 0;
}
