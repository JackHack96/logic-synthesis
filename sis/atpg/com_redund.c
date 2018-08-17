
#include <setjmp.h>
#include <signal.h>
#include "sis.h"
#include "../include/atpg_int.h"

static bool remove_redundancy();

static bool atpg_application();

static bool quick_atpg_application();

static void save_fault_pattern();

static jmp_buf timeout_env;

static void timeout_handle() {
    longjmp(timeout_env, 1);
}


static void
print_usage() {
    fprintf(sisout, "usage: red_removal [-dhnqrRptvy]\n");
    fprintf(sisout, "-d\tdepth of RTG sequences (default is STG depth)\n");
    fprintf(sisout, "-h\tuse fast SAT; no non-local implications\n");
    fprintf(sisout, "-n\tnumber of sequences to fault simulate at one time\n");
    fprintf(sisout, "\t(default is system word length; n must be less than this length)\n");
    fprintf(sisout, "-r\tno RTG\n");
    fprintf(sisout, "-R\tno random propagation\n");
    fprintf(sisout, "-q\tquick redundancy removal--remove only SNE redundancies\n");
    fprintf(sisout, "-p\tno product machines, i.e. no fault-free propagation or \n\tgood/faulty PMT\n");
    fprintf(sisout, "-t\tperform tech decomp of network\n");
    fprintf(sisout, "-v\tverbosity\n");
    fprintf(sisout, "-y\tlength of random prop sequences (default is 20)\n");
}

int
st_fpcmp(obj1, obj2)
        char *obj1;
        char *obj2;
{
    fault_pattern_t *fp1, *fp2;
    int             diff;

    fp1  = (fault_pattern_t *) obj1;
    fp2  = (fault_pattern_t *) obj2;
    diff = (int) (fp1->node - fp2->node);
    if (diff) {
        return diff;
    }
    diff = (int) (fp1->fanin - fp2->fanin);
    if (diff) {
        return diff;
    }
    diff = fp1->value - fp2->value;
    return diff;
}

int
st_fphash(obj, modulus)
        char *obj;
        int modulus;
{
    unsigned        hash;
    fault_pattern_t *fp;

    fp   = (fault_pattern_t *) obj;
    hash = (unsigned) (fp->node) + (unsigned) (fp->fanin) + (unsigned) (fp->value + 1);
    return hash % modulus;
}

int
com_redundancy_removal(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    long            begin_time;
    network_t       *dc_network;
    int             stg_depth, n_redundant, n_setups, i;
    bool            output_fns_changed, redundancy_found;
    atpg_info_t     *info;
    atpg_ss_info_t  *ss_info, *exdc_ss_info;
    seq_info_t      *seq_info;
    fault_t         *tf;
    atpg_options_t  *atpg_opt;
    fault_pattern_t *info_p;
    sequence_t      *sequence;
    array_t         *vectors;
    unsigned        *vector;
    st_table        *fp_table, *aborted_table, *prev_fault_table;
    st_generator    *sgen;
    lsGen           gen;
    lsList          tested_faults;

    if ((*network) == NIL(network_t)) return 0;
    if (network_num_internal(*network) == 0) return 0;

    n_setups   = 0;
    begin_time = util_cpu_time();
    info       = atpg_info_init(*network);
    info->seq = (network_num_latch(info->network) ? TRUE : FALSE);
    atpg_opt = info->atpg_opt;
    if (!set_atpg_options(info, argc, argv)) {
        print_usage();
        atpg_free_info(info);
        return 1;
    }

    /* This is a hack.  Currently, atpg and hence redundancy removal only
       works if a single initial state is given.  This forces a sequential
       circuit to look combinational, so the assumption is that all states
       are possible initial states.  This means a conservative approximation
       for sequential redundancy removal. */

    if (atpg_opt->force_comb) info->seq = FALSE;
    if (atpg_opt->timeout > 0) {
        (void) signal(SIGALRM, timeout_handle);
        (void) alarm((unsigned int) atpg_opt->timeout);
        if (setjmp(timeout_env) > 0) {
            fprintf(sisout, "timeout occurred after %d seconds\n",
                    atpg_opt->timeout);
            atpg_free_info(info);
            return 1;
        }
    }
    if (atpg_opt->tech_decomp) {
        decomp_tech_network(*network, INFINITY, INFINITY);
    }
    network_sweep(*network);

    ss_info = atpg_sim_sat_info_init(*network, info);
    if (!info->seq) {
        seq_info = NIL(seq_info_t);
    }

    fp_table           = st_init_table(st_fpcmp, st_fphash);
    aborted_table      = st_init_table(st_fpcmp, st_fphash);
    prev_fault_table   = st_init_table(st_fpcmp, st_fphash);
    output_fns_changed = TRUE;
    n_redundant        = -1;
    /* external don't cares */
    if (info->seq || ((dc_network = network_dc_network(*network)) == NULL)) {
        exdc_ss_info = NIL(atpg_ss_info_t);
    } else {
        exdc_ss_info = atpg_sim_sat_info_init(dc_network, info);
        atpg_sim_setup(exdc_ss_info);
        atpg_comb_sim_setup(exdc_ss_info);
        atpg_sat_init(exdc_ss_info);
    }

    do {
        n_redundant++;
        /* setup data structures for one redundancy removal */
        /* Only need to update the output functions if the previous redundancy
         * was an "observability" redundancy.
         */
        if (output_fns_changed) {
            if (n_redundant != 0) {
                atpg_sim_unsetup(ss_info);
            }
            atpg_sim_setup(ss_info);
            if (info->seq) {
                if (n_redundant != 0) {
                    seq_info_free(info, seq_info);
                }
                n_setups += 1;
                seq_info = atpg_seq_info_init(info);
                seq_setup(seq_info, info);
                record_reset_state(seq_info, info);
                assert(calculate_reachable_states(seq_info));
                seq_info->valid_states_network = convert_bdd_to_network(seq_info,
                                                                        seq_info->range_data->total_set);
                stg_depth = array_n(seq_info->reached_sets);
                info->statistics->stg_depth = stg_depth;
                atpg_sat_node_info_setup(seq_info->valid_states_network);
                atpg_setup_seq_info(info, seq_info, stg_depth);
            }
        }

        atpg_gen_faults(info);
        atpg_comb_sim_setup(ss_info);
        if (exdc_ss_info != NIL(atpg_ss_info_t)) {
            atpg_exdc_sim_link(ss_info, ss_info);
        }

        if (atpg_opt->verbosity > 0) {
            fprintf(sisout, "%d total faults\n", lsLength(info->faults));
        }

        /* start off with RTG */
        if (n_redundant == 0) {
            if (atpg_opt->rtg_depth == -1) {
                if (info->seq) atpg_opt->rtg_depth = stg_depth;
                else atpg_opt->rtg_depth = 1;
            }
            if (atpg_opt->quick_redund) {
                tested_faults = lsCreate();
            } else {
                tested_faults = atpg_random_cover(info, ss_info, exdc_ss_info,
                                                  TRUE, NIL(
                int));
            }
            info->statistics->n_RTG_tested = lsLength(tested_faults);
            foreach_fault(tested_faults, gen, tf) {
                save_fault_pattern(fp_table, prev_fault_table, tf);
            }
            if (atpg_opt->verbosity > 0) {
                fprintf(sisout, "%d faults covered by RTG\n",
                        lsLength(tested_faults));
            }
            /* patterns still retained */
            lsDestroy(tested_faults, free_fault);
        }
        if (atpg_opt->quick_redund) {
            redundancy_found = quick_atpg_application(info, ss_info, seq_info,
                                                      exdc_ss_info, fp_table, aborted_table,
                                                      prev_fault_table, &output_fns_changed);
        } else {
            redundancy_found = atpg_application(info, ss_info, seq_info,
                                                exdc_ss_info, fp_table, aborted_table,
                                                prev_fault_table, &output_fns_changed);
        }

        if (network_num_latch(*network) != info->n_latch) {
            info->n_po    = network_num_po(*network);
            info->n_pi    = network_num_pi(*network);
            info->n_latch = network_num_latch(*network);
            if (info->n_latch == 0) {
                seq_info_free(info, seq_info);
                info->seq = FALSE;
            }
            output_fns_changed = TRUE;
            if (atpg_opt->quick_redund) {
                st_foreach_item(fp_table, sgen, (char **) &info_p, NIL(
                char *)) {
                    st_delete(fp_table, (char **) &info_p, NIL(
                    char *));
                    FREE(info_p);
                }
                st_foreach_item(info->sequence_table, sgen, (char **) &sequence,
                                NIL(
                char *)) {
                    st_delete(info->sequence_table, (char **) &sequence,
                              NIL(
                    char *));
                    vectors = sequence->vectors;
                    for (i  = array_n(vectors); i--;) {
                        vector = array_fetch(
                        unsigned *, vectors, i);
                        FREE(vector);
                    }
                    array_free(vectors);
                    FREE(sequence);
                }
            }
        }
        lsDestroy(info->faults, free_fault);
        atpg_comb_sim_unsetup(ss_info);

    } while (redundancy_found == TRUE);

    info->time_info->total_time = (util_cpu_time() - begin_time);

    if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "number of setups: %d\n", n_setups);
        fprintf(sisout, "total time:\t\t\t%.2f\n",
                info->time_info->total_time / 1000.0);
    }

    /* free all aborted faults and fault-pattern pairs */
    st_foreach_item(fp_table, sgen, (char **) &info_p, (char **) &sequence)
    {
        FREE(info_p);
    }
    st_free_table(fp_table);
    st_foreach_item(prev_fault_table, sgen, (char **) &info_p, NIL(
    char *)) {
        FREE(info_p);
    }
    st_free_table(prev_fault_table);
    st_foreach_item(aborted_table, sgen, (char **) &info_p, NIL(
    char *)) {
        FREE(info_p);
    }
    st_free_table(aborted_table);

    print_and_destroy_sequences(info);
    if (exdc_ss_info != NIL(atpg_ss_info_t)) {
        atpg_sim_unsetup(exdc_ss_info);
        atpg_comb_sim_unsetup(exdc_ss_info);
        atpg_sim_free(exdc_ss_info);
        atpg_sat_free(exdc_ss_info);
        FREE(exdc_ss_info);
    }
    atpg_sim_unsetup(ss_info);
    atpg_sim_free(ss_info);
    FREE(ss_info);
    if (info->seq) {
        seq_info_free(info, seq_info);
    }
    atpg_free_info(info);
    sm_cleanup();
    fast_avl_cleanup();
    return 0;
}

static bool
atpg_application(info, ss_info, seq_info, exdc_info, fp_table, aborted_table,
                 prev_fault_table, output_fns_changed)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info;
        seq_info_t *seq_info;
        atpg_ss_info_t *exdc_info;
        st_table *fp_table;
        st_table *aborted_table;
        st_table *prev_fault_table;
        bool *output_fns_changed;
{
    atpg_options_t  *atpg_opt = info->atpg_opt;
    bool            pi_seen_before;
    int             hit, *slot, count, cnt;
    fault_t         *fault, *tf;
    fault_pattern_t fault_info, *new_info;
    st_generator    *sgen;
    lsList          seen_faults, covered, untested_faults;
    lsHandle        handle;
    lsGeneric       data;
    lsGen           gen;
    sequence_t      *test_sequence;

    untested_faults = lsCreate();
    /* reset aborted fault flags */
    st_foreach_item(aborted_table, sgen, (char **) &new_info, NIL(
    char *)) {
        st_insert(aborted_table, (char *) new_info, 0);
    }

    /* retain only previously unseen faults */
    count       = lsLength(info->faults);
    seen_faults = lsCreate();
    atpg_sat_init(ss_info);
    while (count-- && lsFirstItem(info->faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
        fault_info.node  = fault->node;
        fault_info.fanin = fault->fanin;
        fault_info.value = (fault->value == S_A_0) ? 0 : 1;
        lsRemoveItem(handle, &data);
        if (st_lookup(prev_fault_table, (char *) &fault_info, NIL(char *))) {
            lsNewEnd(seen_faults, (lsGeneric) data, 0);
        }
        else {
            lsNewEnd(info->faults, (lsGeneric) data, 0);
        }
    }
    cnt = 0;
    /* deterministic ATPG on previously unseen faults only */
    while (lsFirstItem(info->faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
        fault_info.node  = fault->node;
        fault_info.fanin = fault->fanin;
        fault_info.value = (fault->value == S_A_0) ? 0 : 1;
        if (atpg_opt->verbosity > 0) {
            atpg_print_fault(sisout, fault);
        }

        /* find test using three-step algorithm and fault-free assumption */
        test_sequence = generate_test(fault, info, ss_info, seq_info,
                                      exdc_info, cnt);

        switch (fault->status) {
            case REDUNDANT:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Redundant\n");
                }
                pi_seen_before = remove_redundancy(info, ss_info, fault,
                                                   output_fns_changed);
                lsRemoveItem(handle, &data);
                free_fault((fault_t *) data);
                break;
            case ABORTED:
            case UNTESTED:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Untested\n");
                }
                assert(!st_lookup(aborted_table, (char *) &fault_info, NIL(
                char *)));
                new_info = ALLOC(fault_pattern_t, 1);
                *new_info = fault_info;
                st_insert(aborted_table, (char *) new_info, (char *) 0);
                assert(!st_lookup(prev_fault_table, (char *) &fault_info, NIL(
                char *)));
                new_info = ALLOC(fault_pattern_t, 1);
                *new_info = fault_info;
                st_insert(prev_fault_table, (char *) new_info, 0);
                lsRemoveItem(handle, &data);
                lsNewEnd(untested_faults, (lsGeneric) data, 0);
                break;
            case TESTED: /* tested fault */
                if (ATPG_DEBUG) {
                    assert(atpg_verify_test(ss_info, fault, test_sequence));
                }
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Tested\n");
                }
                cnt++;
                lsRemoveItem(handle, &data);
                test_sequence->n_covers = 1;
                fault->sequence         = test_sequence;
                st_insert(info->sequence_table, (char *) test_sequence, (char *) 0);
                save_fault_pattern(fp_table, prev_fault_table, fault);
                free_fault((fault_t *) data);
                if (cnt == atpg_opt->n_sim_sequences) {
                    covered = atpg_seq_single_fault_simulate(ss_info, exdc_info,
                                                             ss_info->word_vectors, info->faults,
                                                             NIL(sequence_t * ), 0);
                    concat_lists(covered,
                                 atpg_seq_single_fault_simulate(ss_info,
                                                                exdc_info, ss_info->word_vectors,
                                                                untested_faults, NIL(sequence_t * ), 0));
                    extract_test_sequences(info, ss_info, covered,
                                           ss_info->word_vectors,
                                           NIL(
                    int), info->n_real_pi);
                    foreach_fault(covered, gen, tf) {
                        save_fault_pattern(fp_table, prev_fault_table, tf);
                    }
                    lsDestroy(covered, free_fault);
                    cnt = 0;
                    reset_word_vectors(ss_info);
                }
                break;
            default: fail("bad fault->status returned by generate_test");
                break;
        }
        /* pi's cannot be removed */
        if (fault->status == REDUNDANT && !pi_seen_before) {
            lsDestroy(seen_faults, free_fault);
            lsDestroy(untested_faults, free_fault);
            return TRUE;
        }
    }

    /* look at previously-seen faults now */
    concat_lists(info->faults, seen_faults);

    /* get faults using previous patterns */
    atpg_simulate_old_sequences(info, ss_info, exdc_info, fp_table, untested_faults);
    if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "%d faults remaining after using previous tests\n",
                lsLength(info->faults) + lsLength(untested_faults));
    }

    cnt = 0;
    /* now deterministic TPG on remaining faults */
    while (lsFirstItem(info->faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
        fault_info.node  = fault->node;
        fault_info.fanin = fault->fanin;
        fault_info.value = (fault->value == S_A_0) ? 0 : 1;
        hit = st_find(aborted_table, (char *) &fault_info, (char ***) &slot);
        if (hit && !(*slot)) {
            *slot = 1;
            lsRemoveItem(handle, &data);
            lsNewEnd(info->faults, (lsGeneric) data, 0);
            continue;
        }
        if (atpg_opt->verbosity > 0) {
            atpg_print_fault(sisout, fault);
        }
        if (info->seq) {
            if (!seq_info->product_machine_built && atpg_opt->deterministic_prop
                && atpg_opt->build_product_machines) {
                seq_product_setup(info, seq_info, info->network);
                copy_orig_bdds(seq_info);
                atpg_product_setup_seq_info(seq_info);
            }
        }

        /* find test using three-step algorithm and fault-free assumption */
        test_sequence = generate_test(fault, info, ss_info, seq_info,
                                      exdc_info, cnt);

        switch (fault->status) {
            case REDUNDANT:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Redundant\n");
                }
                pi_seen_before = remove_redundancy(info, ss_info, fault,
                                                   output_fns_changed);
                lsRemoveItem(handle, &data);
                free_fault((fault_t *) data);
                break;
            case ABORTED:
            case UNTESTED:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Untested\n");
                }
                if (!st_lookup(aborted_table, (char *) &fault_info, NIL(char *))) {
            new_info = ALLOC(fault_pattern_t, 1);
            *new_info = fault_info;
            st_insert(aborted_table, (char *) new_info, (char *) 1);
        }
                assert(st_lookup(prev_fault_table, (char *) &fault_info, NIL(
                char *)));
                lsRemoveItem(handle, &data);
                lsNewEnd(untested_faults, (lsGeneric) data, 0);
                break;
            case TESTED: /* tested fault */
                if (ATPG_DEBUG) {
                    assert(atpg_verify_test(ss_info, fault, test_sequence));
                }
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Tested\n");
                }
                cnt++;
                lsRemoveItem(handle, &data);
                test_sequence->n_covers = 1;
                fault->sequence         = test_sequence;
                st_insert(info->sequence_table, (char *) test_sequence, (char *) 0);
                save_fault_pattern(fp_table, prev_fault_table, fault);
                free_fault((fault_t *) data);
                if (cnt == atpg_opt->n_sim_sequences) {
                    covered = atpg_seq_single_fault_simulate(ss_info,
                                                             exdc_info, ss_info->word_vectors, info->faults,
                                                             NIL(sequence_t * ), 0);
                    concat_lists(covered,
                                 atpg_seq_single_fault_simulate(ss_info,
                                                                exdc_info, ss_info->word_vectors,
                                                                untested_faults, NIL(sequence_t * ), 0));
                    extract_test_sequences(info, ss_info, covered,
                                           ss_info->word_vectors,
                                           NIL(
                    int), info->n_real_pi);
                    foreach_fault(covered, gen, tf) {
                        save_fault_pattern(fp_table, prev_fault_table, tf);
                    }
                    lsDestroy(covered, free_fault);
                    cnt = 0;
                    reset_word_vectors(ss_info);
                }
                break;
            default: fail("bad fault->status returned by generate_test");
                break;
        }

        /* pi's cannot be removed */
        if (fault->status == REDUNDANT && !pi_seen_before) {
            if (info->seq) {
                if (seq_info->product_machine_built) {
                    seq_info_product_free(seq_info);
                }
            }
            lsDestroy(untested_faults, free_fault);
            return TRUE;
        }
    }

    /* now product machine traversal on remaining untested faults */
    if (info->seq && atpg_opt->build_product_machines) {
        while (lsFirstItem(untested_faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
            if (atpg_opt->verbosity > 0) {
                atpg_print_fault(sisout, fault);
            }
            if (!seq_info->product_machine_built) {
                seq_product_setup(info, seq_info, info->network);
                copy_orig_bdds(seq_info);
                atpg_product_setup_seq_info(seq_info);
            }

            /* find test using PMT */
            test_sequence = generate_test_using_verification(fault, info, ss_info,
                                                             seq_info, cnt);

            switch (fault->status) {
                case REDUNDANT:
                    if (atpg_opt->verbosity > 0) {
                        fprintf(sisout, "Redundant\n");
                    }
                    pi_seen_before = remove_redundancy(info, ss_info, fault,
                                                       output_fns_changed);
                    lsRemoveItem(handle, &data);
                    free_fault((fault_t *) data);
                    break;
                case ABORTED:
                case UNTESTED:
                    if (atpg_opt->verbosity > 0) {
                        fprintf(sisout, "Untested by PMT\n");
                    }
                    assert(st_lookup(aborted_table, (char *) &fault_info, NIL(
                    char *)));
                    assert(st_lookup(prev_fault_table, (char *) &fault_info, NIL(
                    char *)));
                    lsRemoveItem(handle, &data);
                    free_fault((fault_t *) data);
                    break;
                case TESTED: /* tested fault */
                    if (ATPG_DEBUG) {
                        assert(atpg_verify_test(ss_info, fault, test_sequence));
                    }
                    if (atpg_opt->verbosity > 0) {
                        fprintf(sisout, "Tested\n");
                    }
                    cnt++;
                    lsRemoveItem(handle, &data);
                    test_sequence->n_covers = 1;
                    fault->sequence         = test_sequence;
                    st_insert(info->sequence_table, (char *) test_sequence, (char *) 0);
                    save_fault_pattern(fp_table, prev_fault_table, fault);
                    free_fault((fault_t *) data);
                    if (cnt == atpg_opt->n_sim_sequences) {
                        covered = atpg_seq_single_fault_simulate(ss_info,
                                                                 exdc_info, ss_info->word_vectors,
                                                                 untested_faults, NIL(sequence_t * ), 0);
                        extract_test_sequences(info, ss_info, covered,
                                               ss_info->word_vectors,
                                               NIL(
                        int), info->n_real_pi);
                        foreach_fault(covered, gen, tf) {
                            save_fault_pattern(fp_table, prev_fault_table, tf);
                        }
                        lsDestroy(covered, free_fault);
                        cnt = 0;
                        reset_word_vectors(ss_info);
                    }
                    break;
                default: fail("bad fault->status returned by generate_test");
                    break;
            }

            /* pi's cannot be removed */
            if (fault->status == REDUNDANT && !pi_seen_before) {
                lsDestroy(untested_faults, free_fault);
                if (seq_info->product_machine_built) {
                    seq_info_product_free(seq_info);
                }
                return TRUE;
            }
        }
    }
    /* if a redundancy was removed, then sat has already been freed in 
     * the remove_redundancy procedure.
     */
    atpg_sat_free(ss_info);
    lsDestroy(untested_faults, free_fault);
    if (info->seq) {
        if (seq_info->product_machine_built) {
            seq_info_product_free(seq_info);
        }
    }
    return FALSE;
}

static bool
quick_atpg_application(info, ss_info, seq_info, exdc_info, fp_table,
                       aborted_table, prev_fault_table, output_fns_changed)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info;
        seq_info_t *seq_info;
        atpg_ss_info_t *exdc_info;
        st_table *fp_table;
        st_table *aborted_table;
        st_table *prev_fault_table;
        bool *output_fns_changed;
{
    atpg_options_t  *atpg_opt = info->atpg_opt;
    bool            pi_seen_before;
    int             hit, *slot, count, cnt, n_pi_vars;
    fault_t         *fault, *tf;
    fault_pattern_t fault_info, *new_info;
    st_generator    *sgen;
    lsList          seen_faults, covered, untested_faults;
    lsHandle        handle;
    lsGeneric       data;
    lsGen           gen;
    sat_result_t    sat_value;

    untested_faults = lsCreate();
    /* reset aborted fault flags */
    st_foreach_item(aborted_table, sgen, (char **) &new_info, NIL(
    char *)) {
        st_insert(aborted_table, (char *) new_info, 0);
    }

    /* retain only previously unseen faults */
    count       = lsLength(info->faults);
    seen_faults = lsCreate();
    atpg_sat_init(ss_info);
    while (count-- && lsFirstItem(info->faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
        fault_info.node  = fault->node;
        fault_info.fanin = fault->fanin;
        fault_info.value = (fault->value == S_A_0) ? 0 : 1;
        lsRemoveItem(handle, &data);
        if (st_lookup(prev_fault_table, (char *) &fault_info, NIL(char *))) {
            lsNewEnd(seen_faults, (lsGeneric) data, 0);
        }
        else {
            lsNewEnd(info->faults, (lsGeneric) data, 0);
        }
    }
    cnt = 0;
    /* deterministic SNE redundancy identification on previously unseen 
       faults only 
    */
    while (lsFirstItem(info->faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
        fault_info.node  = fault->node;
        fault_info.fanin = fault->fanin;
        fault_info.value = (fault->value == S_A_0) ? 0 : 1;
        if (atpg_opt->verbosity > 0) {
            atpg_print_fault(sisout, fault);
        }
        sat_reset(ss_info->atpg_sat);
        n_pi_vars = atpg_network_fault_clauses(ss_info, exdc_info, seq_info, fault);
        sat_value = sat_solve(ss_info->atpg_sat, atpg_opt->fast_sat,
                              atpg_opt->verbosity);
        switch (sat_value) {
            case SAT_ABSURD:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Redundant\n");
                }
                fault->redund_type = CONTROL;
                pi_seen_before = remove_redundancy(info, ss_info, fault,
                                                   output_fns_changed);
                lsRemoveItem(handle, &data);
                free_fault((fault_t *) data);
                break;
            case SAT_GAVEUP:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Aborted by SAT\n");
                }
                assert(!st_lookup(aborted_table, (char *) &fault_info, NIL(
                char *)));
                new_info = ALLOC(fault_pattern_t, 1);
                *new_info = fault_info;
                st_insert(aborted_table, (char *) new_info, (char *) 0);
                assert(!st_lookup(prev_fault_table, (char *) &fault_info, NIL(
                char *)));
                new_info = ALLOC(fault_pattern_t, 1);
                *new_info = fault_info;
                st_insert(prev_fault_table, (char *) new_info, 0);
                lsRemoveItem(handle, &data);
                lsNewEnd(untested_faults, (lsGeneric) data, 0);
                break;
            case SAT_SOLVED: /* tested fault */
                fault->sequence = derive_comb_test(ss_info, n_pi_vars, cnt);
                lsRemoveItem(handle, &data);
                save_fault_pattern(fp_table, prev_fault_table, fault);
                free_fault((fault_t *) data);
                cnt++;
                st_insert(info->sequence_table, (char *) fault->sequence, (char *) 0);
                if (cnt == atpg_opt->n_sim_sequences) {
                    covered = atpg_comb_single_fault_simulate(ss_info, exdc_info,
                                                              ss_info->word_vectors, info->faults);
                    concat_lists(covered,
                                 atpg_comb_single_fault_simulate(ss_info,
                                                                 exdc_info, ss_info->word_vectors,
                                                                 untested_faults));
                    extract_test_sequences(info, ss_info, covered,
                                           ss_info->word_vectors,
                                           NIL(
                    int), info->n_pi);
                    foreach_fault(covered, gen, tf) {
                        save_fault_pattern(fp_table, prev_fault_table, tf);
                    }
                    lsDestroy(covered, free_fault);
                    cnt = 0;
                    reset_word_vectors(ss_info);
                }
                break;
            default: fail("bad fault->status returned by generate_test");
                break;
        }
        /* pi's cannot be removed */
        if (sat_value == SAT_ABSURD && !pi_seen_before) {
            lsDestroy(seen_faults, free_fault);
            lsDestroy(untested_faults, free_fault);
            return TRUE;
        }
    }

    /* look at previously-seen faults now */
    concat_lists(info->faults, seen_faults);

    /* get faults using previous patterns */
    if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "%d faults remaining\n",
                lsLength(info->faults) + lsLength(untested_faults));
    }
    atpg_comb_simulate_old_sequences(info, ss_info, exdc_info, fp_table,
                                     untested_faults);
    if (atpg_opt->verbosity > 0) {
        fprintf(sisout, "%d faults remaining after using previous tests\n",
                lsLength(info->faults) + lsLength(untested_faults));
    }

    cnt = 0;
    /* now deterministic SNE identification on remaining faults */
    while (lsFirstItem(info->faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
        fault_info.node  = fault->node;
        fault_info.fanin = fault->fanin;
        fault_info.value = (fault->value == S_A_0) ? 0 : 1;
        hit = st_find(aborted_table, (char *) &fault_info, (char ***) &slot);
        if (hit && !(*slot)) {
            *slot = 1;
            lsRemoveItem(handle, &data);
            lsNewEnd(info->faults, (lsGeneric) data, 0);
            continue;
        }
        if (atpg_opt->verbosity > 0) {
            atpg_print_fault(sisout, fault);
        }

        sat_reset(ss_info->atpg_sat);
        n_pi_vars = atpg_network_fault_clauses(ss_info, exdc_info, seq_info, fault);
        sat_value = sat_solve(ss_info->atpg_sat, atpg_opt->fast_sat,
                              atpg_opt->verbosity);
        switch (sat_value) {
            case SAT_ABSURD:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Redundant\n");
                }
                fault->redund_type = CONTROL;
                pi_seen_before = remove_redundancy(info, ss_info, fault,
                                                   output_fns_changed);
                lsRemoveItem(handle, &data);
                free_fault((fault_t *) data);
                break;
            case SAT_GAVEUP:
                if (atpg_opt->verbosity > 0) {
                    fprintf(sisout, "Aborted by SAT\n");
                }
                if (!st_lookup(aborted_table, (char *) &fault_info, NIL(char *))) {
            new_info = ALLOC(fault_pattern_t, 1);
            *new_info = fault_info;
            st_insert(aborted_table, (char *) new_info, (char *) 1);
        }
                assert(st_lookup(prev_fault_table, (char *) &fault_info, NIL(
                char *)));
                lsRemoveItem(handle, &data);
                lsNewEnd(untested_faults, (lsGeneric) data, 0);
                break;
            case SAT_SOLVED: /* tested fault */
                fault->sequence = derive_comb_test(ss_info, n_pi_vars, cnt);
                lsRemoveItem(handle, &data);
                save_fault_pattern(fp_table, prev_fault_table, fault);
                free_fault((fault_t *) data);
                cnt++;
                st_insert(info->sequence_table, (char *) fault->sequence, (char *) 0);
                if (cnt == atpg_opt->n_sim_sequences) {
                    covered = atpg_comb_single_fault_simulate(ss_info, exdc_info,
                                                              ss_info->word_vectors, info->faults);
                    concat_lists(covered,
                                 atpg_comb_single_fault_simulate(ss_info,
                                                                 exdc_info, ss_info->word_vectors,
                                                                 untested_faults));
                    extract_test_sequences(info, ss_info, covered,
                                           ss_info->word_vectors,
                                           NIL(
                    int), info->n_pi);
                    foreach_fault(covered, gen, tf) {
                        save_fault_pattern(fp_table, prev_fault_table, tf);
                    }
                    lsDestroy(covered, free_fault);
                    cnt = 0;
                    reset_word_vectors(ss_info);
                }
                break;
            default: fail("bad fault->status returned by generate_test");
                break;
        }

        /* pi's cannot be removed */
        if (sat_value == SAT_ABSURD && !pi_seen_before) {
            lsDestroy(untested_faults, free_fault);
            return TRUE;
        }
    }

    /* If a redundancy was removed, then sat has already been freed in 
     * the remove_redundancy procedure.
     */
    atpg_sat_free(ss_info);
    lsDestroy(untested_faults, free_fault);
    return FALSE;
}


/* TRUE if pi redundancy had been handled before
 * FALSE otherwise 
 */
static bool
remove_redundancy(info, ss_info, fault, output_fns_changed)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info;
        fault_t *fault;
        bool *output_fns_changed;
{
    int    n_fanout, i;
    node_t *const_node, *fanout, **fanouts;
    lsGen  gen;

    n_fanout = node_num_fanout(fault->node);
    if (node_function(fault->node) == NODE_PI && n_fanout == 0) {
        return TRUE;
    }
    /* free sat here, since it's impossible to free completely after 
     * a redundancy has been removed
     */
    atpg_sat_free(ss_info);

    const_node = (fault->value == S_A_0 ? node_constant(0) : node_constant(1));
    network_add_node(info->network, const_node);
    if (fault->fanin == NIL(node_t)) {
        fanouts = ALLOC(node_t * , n_fanout);
        i       = 0;
        foreach_fanout(fault->node, gen, fanout)
        {
            fanouts[i++] = fanout;
        }
        for (i = 0; i < n_fanout; i++) {
            node_patch_fanin(fanouts[i], fault->node, const_node);
        }
        FREE(fanouts);
    } else {
        node_patch_fanin(fault->node, fault->fanin, const_node);
    }

    network_sweep(info->network);
    if (fault->redund_type == CONTROL) {
        *output_fns_changed = FALSE;
    } else {
        assert(fault->redund_type == OBSERVE);
        *output_fns_changed = TRUE;
    }
    return FALSE;
}

/* matches fault with a sequence that detects it */
static void
save_fault_pattern(fault_pattern_table, prev_fault_table, f)
        st_table *fault_pattern_table;
        st_table *prev_fault_table;
        fault_t *f;
{
    sequence_t      **slot;
    fault_pattern_t fp, *fp_ptr;

    fp.node  = f->node;
    fp.fanin = f->fanin;
    fp.value = (f->value == S_A_0) ? 0 : 1;
    if (!st_find(fault_pattern_table, (char *) &fp, (char ***) &slot)) {
        fp_ptr = ALLOC(fault_pattern_t, 1);
        *fp_ptr = fp;
        st_insert(fault_pattern_table, (char *) fp_ptr, (char *) f->sequence);
    }
        /* update sequence stored with fault */
    else {
        *slot = f->sequence;
    }
    if (!st_lookup(prev_fault_table, (char *) &fp, NIL(char *))) {
        fp_ptr = ALLOC(fault_pattern_t, 1);
        *fp_ptr = fp;
        st_insert(prev_fault_table, (char *) fp_ptr, 0);
    }
}

