

#ifndef PRL_SEQBDD_H
#define PRL_SEQBDD_H

#include "var_set.h"

/* referenced via a pointer only */

typedef struct seq_info_t seq_info_t;
typedef struct prl_options_t prl_options_t;

/* routines that are method dependent; currently we only support product method
 */

extern void Prl_ProductBddOrder(seq_info_t *, prl_options_t *);

extern void Prl_ProductInitSeqInfo(seq_info_t *, prl_options_t *);

extern void Prl_ProductFreeSeqInfo(seq_info_t *, prl_options_t *);

extern bdd_t *Prl_ProductComputeNextStates(bdd_t *, seq_info_t *,
                                           prl_options_t *);

extern bdd_t *Prl_ProductReverseImage(bdd_t *, seq_info_t *, prl_options_t *);

extern array_t *Prl_ProductExtractNetworkInputNames(seq_info_t *,
                                                    prl_options_t *);

/* the corresponding function types */

typedef void (*Prl_SeqInfoFn)(seq_info_t *, prl_options_t *);

typedef bdd_t *(*Prl_NextStateFn)(bdd_t *, seq_info_t *, prl_options_t *);

typedef array_t *(*Prl_NameExtractFn)(seq_info_t *, prl_options_t *);

struct prl_options_t {
  /* global options */
  int verbose;

  /* ordering heuristic: branch & bound depth */
  int ordering_depth;

  /* timing control */
  int timeout;
  long last_time;
  long total_time;

  /* method dependent generic info */
  char *method_name;
  range_method_t type;
  Prl_SeqInfoFn bdd_order;
  Prl_SeqInfoFn init_seq_info;
  Prl_SeqInfoFn free_seq_info;
  Prl_NextStateFn compute_next_states;
  Prl_NextStateFn compute_reverse_image;
  Prl_NameExtractFn extract_network_input_names;

  /* Option for env_verify_fsm */
  int stop_if_verify;

  /* Options to Prl_RemoveLatches */
  struct {
    int local_retiming; /* when on, perform local retiming before removing
                           redundant latches */
    int max_cost;    /* max support size; when exceeded, latch is not removed */
    int max_level;   /* max depth for substitute logic; when exceeded, latch not
                        removed */
    int remove_boot; /* if set, tries to remove boot latches (latches fed by a
                        constant) */
  } remlatch;
};

typedef struct prl_removedep_struct prl_removedep_t;

struct prl_removedep_struct {
  int perform_check; /* checks whether the dependencies are not logical */
  int verbosity;     /*  */
  int insert_a_one; /* if set, inserts a constant one instead of a constant zero
                     */
};

extern int Prl_ExtractEnvDc(network_t *, network_t *, prl_options_t *);

extern void Prl_EquivNets(network_t *, prl_options_t *);

extern void Prl_RemoveLatches(network_t *, prl_options_t *);

extern int Prl_LatchOutput(network_t *, array_t *, int);

extern int Prl_RemoveDependencies(network_t *, array_t *, prl_removedep_t *);

extern int Prl_VerifyEnvFsm(network_t *, network_t *, network_t *,
                            prl_options_t *);

/* exported from "com_verify.c" */
extern void Prl_StoreAsSingleOutputDcNetwork(network_t *, network_t *);

#endif /* PRL_SEQBDD_H */
