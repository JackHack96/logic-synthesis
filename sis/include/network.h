#ifndef NETWORK_H
#define NETWORK_H

#include "st.h"
#include "graph.h"
#include "astg.h"
#include "node.h"
#include "espresso.h"

#define lsForeachItem(list, gen, data)                                         \
  for (gen = lsStart(list);                                                    \
       (lsNext(gen, (lsGeneric *)&data, LS_NH) == LS_OK) ||                    \
       ((void)lsFinish(gen), 0);)

#define LS_ASSERT(fct) assert((fct) == LS_OK)

typedef struct network_struct network_t;
struct network_struct {
    char      *net_name;
    st_table  *name_table;
    st_table  *short_name_table;
    lsList    nodes;          /* list of all nodes */
    lsList    pi;             /* list of just primary inputs */
    lsList    po;             /* list of just primary outputs */
    network_t *original;   /* UNUSED: pointer to original network */
    double    area;           /* HACK: support area keyword */
    int       area_given;        /* HACK: support area keyword */
    lsList    bdd_list;       /* list of bdd managers */
    network_t *dc_network; /* external don't care network */
#ifdef SIS
    st_table *latch_table; /*      sequential support      */
    lsList   latch;          /*      sequential support      */
    graph_t  *stg;          /*      sequential support      */
    char     *clock;           /*      sequential support      */
#endif                   /* SIS */
    char *default_delay;   /* Stores default delay info */
#ifdef SIS
    astg_t *astg; /* Asynch. Signal Transition Graph.	*/
#endif
};

#define foreach_node(network, gen, p) lsForeachItem((network)->nodes, gen, p)

#define foreach_primary_input(network, gen, p)                                 \
  lsForeachItem((network)->pi, gen, p)

#define foreach_primary_output(network, gen, p)                                \
  lsForeachItem((network)->po, gen, p)

#ifdef SIS
#define foreach_latch(network, gen, l) lsForeachItem((network)->latch, gen, l)
#endif /* SIS */

extern network_t *network_alloc(void);

extern void network_free(network_t *);

extern network_t *network_dup(network_t *);

extern network_t *network_create_from_node(node_t *);

extern network_t *network_from_nodevec(array_t *);

extern char *network_name(network_t *);

extern void network_set_name(network_t *, char *);

extern int network_num_pi(network_t *);

extern int network_num_po(network_t *);

extern int network_num_internal(network_t *);

extern node_t *network_get_pi(network_t *, int);

extern node_t *network_get_po(network_t *, int);

extern void network_add_node(network_t *, node_t *);

extern void network_add_primary_input(network_t *, node_t *);

extern node_t *network_add_primary_output(network_t *, node_t *);

extern void network_delete_node(network_t *, node_t *);

extern void network_delete_node_gen(network_t *, lsGen);

extern node_t *network_find_node(network_t *, char *);

extern void network_change_node_type(network_t *, node_t *,
                                     enum node_type_enum);

extern void network_change_node_name(network_t *, node_t *, char *);

extern int network_check(network_t *);

extern int network_is_acyclic(network_t *);

extern int network_collapse(network_t *);

extern int network_sweep(network_t *);

extern int network_csweep(network_t *);

extern int network_cleanup(network_t *);

extern int network_ccleanup(network_t *);

extern network_t *network_espresso(network_t *);

extern array_t *network_dfs(network_t *);

#ifdef SIS

extern array_t *network_special_dfs(network_t *);

#endif /* SIS */

extern array_t *network_dfs_from_input(network_t *);

extern array_t *network_tfi(node_t *, int);

extern array_t *network_tfo(node_t *, int);

extern int network_append(network_t *, network_t *);

extern pPLA espresso_read_pla(FILE *);

extern void discard_pla(pPLA);

extern pPLA network_to_pla(network_t *);

extern network_t *pla_to_network(pPLA);

extern network_t *pla_to_network_single(pPLA);

extern network_t *pla_to_dcnetwork_single(pPLA);

extern void network_reset_long_name(network_t *);

extern void network_reset_short_name(network_t *);

extern void network_swap_names(network_t *, node_t *, node_t *);

#ifdef SIS

extern void copy_latch_info(lsList, lsList, st_table *);

/* network_create_latch, network_delete_latch declared in latch.h */
extern node_t *network_latch_end(node_t *);

extern void network_disconnect(node_t *, node_t *, node_t **, node_t **);

extern void network_connect(node_t *, node_t *);

extern st_table *snetwork_tfi_po(network_t *);

#define network_num_latch(n) lsLength((n)->latch)

extern graph_t *network_stg(network_t *);

extern void network_set_stg(network_t *, graph_t *);

extern int network_stg_check(network_t *);

extern int network_is_real_po(network_t *, node_t *);

extern int network_is_real_pi(network_t *, node_t *);

extern int network_is_control(network_t *, node_t *);

extern node_t *network_get_control(network_t *, node_t *);

extern node_t *network_add_fake_primary_output(network_t *, node_t *);

extern void network_replace_io_fake_names(network_t *);

#endif /* SIS */

/* network_bdd_list, network_add_bdd declared in bdd.h	*/

extern int network_verify(network_t *, network_t *, int);

extern int net_verify_with_dc(network_t *, network_t *, int, int);

extern network_t *network_dc_network(network_t *);

extern st_table *attach_dcnetwork_to_network(network_t *);

extern node_t *find_ex_dc(node_t *, st_table *);

extern network_t *or_net_dcnet(network_t *);

int init_network(void);

int end_network(void);

#endif