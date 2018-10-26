

#ifndef PRL_SEQBDD_H
#define PRL_SEQBDD_H

#include "var_set.h"

/* referenced via a pointer only */

typedef struct seq_info_t seq_info_t;			 
typedef struct prl_options_t prl_options_t;

 /* routines that are method dependent; currently we only support product method */

EXTERN void     Prl_ProductBddOrder		    ARGS((seq_info_t *, prl_options_t *));
EXTERN void     Prl_ProductInitSeqInfo 		    ARGS((seq_info_t *, prl_options_t *));
EXTERN void     Prl_ProductFreeSeqInfo 		    ARGS((seq_info_t *, prl_options_t *));
EXTERN bdd_t   *Prl_ProductComputeNextStates 	    ARGS((bdd_t *, seq_info_t *, prl_options_t *));
EXTERN bdd_t   *Prl_ProductReverseImage 	    ARGS((bdd_t *, seq_info_t *, prl_options_t *));
EXTERN array_t *Prl_ProductExtractNetworkInputNames ARGS((seq_info_t *, prl_options_t *));

 /* the corresponding function types */

typedef void      (*Prl_SeqInfoFn)     ARGS((seq_info_t *, prl_options_t *));
typedef bdd_t *   (*Prl_NextStateFn)   ARGS((bdd_t *, seq_info_t *, prl_options_t *));
typedef array_t * (*Prl_NameExtractFn) ARGS((seq_info_t *, prl_options_t *));


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
  char *            method_name;
  range_method_t    type;
  Prl_SeqInfoFn     bdd_order;
  Prl_SeqInfoFn     init_seq_info;
  Prl_SeqInfoFn     free_seq_info;
  Prl_NextStateFn   compute_next_states;
  Prl_NextStateFn   compute_reverse_image;
  Prl_NameExtractFn extract_network_input_names;

 /* Option for env_verify_fsm */
  int stop_if_verify;

 /* Options to Prl_RemoveLatches */
  struct {
    int local_retiming;		/* when on, perform local retiming before removing redundant latches */
    int max_cost;		/* max support size; when exceeded, latch is not removed */
    int max_level;		/* max depth for substitute logic; when exceeded, latch not removed */
    int remove_boot;		/* if set, tries to remove boot latches (latches fed by a constant) */
  } remlatch;
};

typedef struct prl_removedep_struct prl_removedep_t;

struct prl_removedep_struct {
    int perform_check;		/* checks whether the dependencies are not logical */
    int verbosity;		/*  */
    int insert_a_one;		/* if set, inserts a constant one instead of a constant zero */
};

EXTERN int  Prl_ExtractEnvDc  	   ARGS((network_t *, network_t *, prl_options_t *));
EXTERN void Prl_EquivNets     	   ARGS((network_t *, prl_options_t *));
EXTERN void Prl_RemoveLatches 	   ARGS((network_t *, prl_options_t *));
EXTERN int  Prl_LatchOutput   	   ARGS((network_t *, array_t *, int));
EXTERN int  Prl_RemoveDependencies ARGS((network_t *, array_t *, prl_removedep_t *));
EXTERN int  Prl_VerifyEnvFsm	   ARGS((network_t *, network_t *, network_t *, prl_options_t *));


 /* exported from "com_verify.c" */
EXTERN void Prl_StoreAsSingleOutputDcNetwork ARGS((network_t *, network_t *));

#endif /* PRL_SEQBDD_H */
