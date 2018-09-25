/* -------------------------------------------------------------------------- *\
   astg_core.h -- asynchronous signal transition graph core declarations.

   This is the "core" of the ASTG package, the lowest level data structures
   and functions which the rest of the ASTG package is based on.  Although
   SIS supports multilevel combinational networks, it unfortunately does
   not support a multilevel package implementation.  So the "core" is sort
   of a package within a package.  See "astg_int.h" for the interface to
   the core package.

   A signal transition graph (STG) is an interpreted Petri Net, which is a
   bipartite graph with a marking.  Transitions are modelled with type 0
   vertices and places are type 1 vertices.
\* -------------------------------------------------------------------------- */

#ifndef ASTG_CORE_H
#define ASTG_CORE_H

#include "astg_int.h"
#include "sis.h"

#define ASTG_NAME_LEN 128

#define dbg(level, action)                                                     \
  if (astg_debug_flag >= (level))                                              \
  action
#define msg printf

#define io_status(source) ((source)->errflag)

#define astg_plx(p) (&(p)->type.place)
#define astg_trx(t) (&(t)->type.trans)

/* ------------------------- astg_ba: array of bits ------------------------- */

typedef struct astg_ba_rec {
  int n_elem;
  int n_word;
  unsigned *bit_array;
} astg_ba_rec;

void astg_ba_set(astg_ba_rec *, int);

void astg_ba_clr(astg_ba_rec *, int);

astg_bool astg_ba_get(astg_ba_rec *, int);

int astg_ba_cmp(astg_ba_rec *, astg_ba_rec *);

astg_ba_rec *astg_ba_init(astg_ba_rec *, int);

astg_ba_rec *astg_ba_new(int);

astg_ba_rec *astg_ba_dup(astg_ba_rec *);

void astg_ba_dispose(astg_ba_rec *);

/* -------------------- DLL : doubly-linked list macros --------------------- */

#define DLL_PREPEND(item, head, next, prev)                                    \
  (item)->next = (head);                                                       \
  (item)->prev = NULL;                                                         \
  if ((head) != NULL)                                                          \
    (head)->prev = (item);                                                     \
  (head) = (item);

#define DLL_INSERT(item, head, next, prev, after)                              \
  if (after != NULL) {                                                         \
    (item)->next = (after)->next;                                              \
    (item)->prev = (after);                                                    \
    if ((after)->next != NULL)                                                 \
      (after)->next->prev = (item);                                            \
    (after)->next = (item);                                                    \
  } else {                                                                     \
    DLL_PREPEND(item, head, next, prev);                                       \
  }

#define DLL_REMOVE(item, head, next, prev)                                     \
  if ((item)->next != NULL)                                                    \
    (item)->next->prev = (item)->prev;                                         \
  if ((item)->prev != NULL)                                                    \
    (item)->prev->next = (item)->next;                                         \
  else                                                                         \
    (head) = (item)->next;

#define get_bit(i, mask) (((i) & (mask)) ? 1 : 0)
#define set_bit(i, mask) ((i) |= (mask))
#define clr_bit(i, mask) ((i) &= ~(mask))
#define val_bit(i, mask, f) ((i) = (f) ? ((i) | (mask)) : ((i) & ~(mask)))

struct astg_signal {
  char *name;                /* name of signal: a, b, etc.		*/
  astg_signal *next;         /* linked list of signals		*/
  astg_signal *prev;         /* doubly linked, for deletion		*/
  astg_signal *dup;          /* used while duplicating		*/
  int id;                    /* for building lock matrix		*/
  astg_signal_enum sig_type; /* input, output, internal		*/
  astg_scode state_bit;      /* for state coding			*/
  astg_bool all_in_one_smc;  /* all trans found in some smc		*/
  astg_trans *t_list;        /* List of transitions of this signal.	*/
  astg_bool sm_checked;      /* astg_check_sm: signal was checked	*/
  int n_trans;               /* set_opp: trans for this sig		*/
  int n_found;               /* set_opp: trans found so far		*/
  astg_bool mark;            /* set_opp: all trans found		*/
  astg_trans *last_trans;    /* set_opp: prev. trans found		*/
  astg_bool can_elim;        /* contract: signal can be eliminated?	*/

  array_t *lg_edges;     /* Lock graph edges adjacency list.	*/
  int lg_weight;         /* Weight for finding shortest path.	*/
  astg_bool unprocessed; /* Flag for shortest path alg.		*/
  astg_signal *lg_from;  /* To find shortest path, trace these.	*/
  lsHandle lg_queue;     /* Handle in priority queue.		*/
};

typedef union astg_v_alg { /* algorithm-specific vertex fields	*/

  struct {            /* astg_simple_cycles			*/
    astg_edge *trail; /* edge from this vertex in cycle	*/
  } sc;

  struct {        /* astg_strong_comp			*/
    int comp_num; /* numbering for the components		*/
  } scc;

  struct {     /* astg_top_sort			*/
    int index; /* topological index for this vertex	*/
  } ts;

  struct {        /* astg_connected_comp			*/
    int comp_num; /* numbering for the components		*/
  } cc;

  struct {           /* astg_dup				*/
    astg_vertex *eq; /* equivalent vertex in new graph	*/
  } dup;

  struct {               /* astg_maxflow				*/
    astg_edge *pathedge; /* edge to use for augmenting path	*/
  } mf;

  struct {             /* astg_shortest_path			*/
    astg_vertex *from; /* how shortest path got to here	*/
    lsHandle util_p;   /* direct access to priority queue	*/
  } sp;

} astg_v_alg;

struct astg_place_info {
  int flow_id;                 /* index of place during token flow	*/
  astg_bool user_named : 1;    /* user explicitly named this place?	*/
  astg_bool initial_token : 1; /* initial token marking		*/
  astg_scode cset_label;       /* For identifying concurreny.		*/
  astg_trans *token_from;      /* Where current token came from.	*/
};

struct astg_trans_info {
  char *alpha_name;           /* Name using only alpha and _ chars.	*/
  astg_signal *sig_p;         /* signal this is for			*/
  astg_trans_enum trans_type; /* pos_x, neg_x, toggle, dummy		*/
  int copy_n;                 /* for duplicate transitions		*/
  astg_trans *parent;         /* vertex in parent stg (for subgraphs)	*/
  astg_trans *next_t;         /* link in pos_t/neg_t list		*/
  astg_trans *opp_trans;      /* opposite transition in this stg	*/
  float delay;                /* time to fire once enabled		*/
  astg_bool can_be_low : 1;   /* sig can be low after this trans?	*/
  astg_bool can_be_high : 1;  /* sig can be high after this?		*/
};

typedef union astg_weight {
  long i;
  float f;
} astg_weight;

struct astg_vertex {
  char *name;                /* name of the vertex			*/
  astg_vertex *parent;       /* vertex in parent stg (for subgraphs)	*/
  astg_vertex *next;         /* link through all vertices in graph   */
  astg_vertex *prev;         /* doubly linked for easy deletion	*/
  astg_graph *stg;           /* graph this vertex is in		*/
  astg_edge *out_edges;      /* list of outgoing edges		*/
  astg_edge *in_edges;       /* list of incoming edges		*/
  astg_bool active : 1;      /* marks active path during DFS		*/
  astg_bool unprocessed : 1; /* marks processed vertices             */
  astg_bool subset : 1;      /* for marking a subset for processing	*/
  astg_bool on_path : 1;     /* for reporting paths and cycles	*/
  astg_bool adjusting : 1;   /* cpm: vertex is being adjusted	*/
  astg_bool forward : 1;     /* maxflow: is flow increasing here?	*/
  astg_bool pkg_mem : 1;     /* memory allocated by graph package	*/
  astg_bool flag0 : 1;       /* Replace these generic flags.		*/
  astg_bool flag1 : 1;
  astg_bool flag2 : 1;
  astg_bool flag3 : 1;
  astg_bool useful : 1;   /* vertex used during token flow?	*/
  astg_bool selected : 1; /* transition is in selection set?	*/
  astg_bool hilited : 1;  /* transition has been hilighted	*/
  float x, y;             /* location of this transition		*/
  astg_v_alg alg;         /* algorithm-specific vertex fields	*/
  astg_weight weight1;    /* weight for maxflow, cpm, etc.	*/
  astg_weight weight2;    /* weight for maxflow, cpm, etc.	*/
  void *userdata;         /* whatever you want to do with this	*/

  astg_vertex_enum vtype; /* Which union below is applicable.	*/
  union {                 /* Type-specific vertex information.	*/
    struct astg_place_info place;
    struct astg_trans_info trans;
  } type;
};

struct astg_edge {
  astg_vertex *tail;     /* tail (source) end of this edge	*/
  astg_edge *prev_out;   /* prev out edge for tail vertex	*/
  astg_edge *next_out;   /* next out edge for tail vertex	*/
  astg_vertex *head;     /* head (target) end of this edge	*/
  astg_edge *prev_in;    /* prev in edge for head vertex		*/
  astg_edge *next_in;    /* next in edge for head vertex		*/
  astg_bool cutset : 1;  /* maxflow: edge is in cutset		*/
  astg_bool pkg_mem : 1; /* memory allocated by graph package	*/
  astg_bool flag0 : 1;
  astg_bool flag1 : 1;
  astg_weight weight1; /* weights for basic graph algorithms	*/
  astg_weight weight2; /* maxflow, cpm, shortest path, etc.	*/
  void *userdata;      /* whatever you want to do with this	*/

  char *guard_eqn;        /* text description of guard		*/
  int guard_n;            /* guard node name suffix		*/
  node_t *guard;          /* only for output edges of places	*/
  array_t *spline_points; /* for displaying the edge		*/
  unsigned selected : 1;  /* edge has been selected		*/
  unsigned hilighted : 1; /* edge has been highlighted		*/
};

struct astg_marking {         /* Defines a single marking of the net.	*/
  astg_scode state_code;      /* Vector of state bit values.		*/
  astg_scode enabled;         /* Vector of enabled signals.		*/
  astg_ba_rec *marked_places; /* Places identified using flow_id.	*/
  astg_bool is_dummy;         /* Marking has enabled dummy trans?	*/
};

struct astg_state {
  array_t *states; /* state is just an array of markings	*/
  astg_graph *stg; /* parent STG for this state info.	*/
};

typedef struct astg_flow {  /* Information from net token flow.	*/
  long change_count;        /* Change count for current flow states.*/
  astg_retval status;       /* Status from last token flow.		*/
  st_table *state_list;     /* Reached in token flow, astg_state*.	*/
  astg_scode initial_state; /* For saving initial state code.	*/
  astg_scode flip_phase;    /* Used by flow to find new phase adj.	*/
  astg_scode phase_adj;     /* Correction for state code phase.	*/
  int in_width;             /* Input width for token flow.		*/
  int out_width;            /* Output width for token flow.		*/
} astg_flow;

struct astg_graph {
  astg_vertex *vertices;     /* linked list of vertices              */
  astg_vertex *vtail;        /* tail end of vertex list		*/
  long change_count;         /* incremented on every change to graph	*/
  int (*f)();                /* user callback for graph algorithms	*/
  void *f_data;              /* user data for callbacks		*/
  char *name;                /* graph name				*/
  astg_graph *parent;        /* for subgraphs			*/
  int n_vertex;              /* number of vertices                   */
  int n_edge;                /* number of edges                      */
  astg_vertex *path_start;   /* For reporting paths in graph.	*/
  int next_place;            /* generating unique place names	*/
  int next_sig;              /* generating unique signal names	*/
  int guard_n;               /* for generating guard names		*/
  network_t *guards;         /* astg_bool conditions on place edges	*/
  lsList comments;           /* save entire-line comments		*/
  char *filename;            /* file this came from (if any)		*/
  long file_count;           /* initial change count for the STG	*/
  int n_sig;                 /* total number of signals		*/
  int n_out;                 /* number of output signals		*/
  astg_signal *sig_list;     /* head of signal DLL list		*/
  char *sel_name;            /* name of current selection set	*/
  astg_bool has_marking;     /* set if marking already		*/
  array_t *sm_comp;          /* state machine components		*/
  long smc_count;            /* change count for current smc		*/
  array_t *mg_comp;          /* marked graph components		*/
  long mgc_count;            /* change count for current mgc		*/
  astg_flow flow_info;       /* exhaustive token flow for net	*/
  void *slots[ASTG_N_SLOTS]; /* Array of slots for data.	*/
};

typedef struct astg_daemon_t {
  struct astg_daemon_t *next; /* Singly-linked list of daemons.	*/
  astg_daemon_enum type;      /* Which type of daemon this is.	*/
  astg_daemon daemon;         /* The daemon to call.			*/
} astg_daemon_t;

typedef struct astg_flow_t { /* Info for generating markings.	*/
  astg_retval status;        /* status code, 0=ok, see astg.h	*/
  astg_bool force_safe;      /* ASTG_TRUE=avoid unsafe markings	*/
  astg_graph *stg;           /* graph to generate reachability graph	*/
  astg_scode state_code;     /* used during guard evaluation		*/
  astg_marking *marking;     /* current marking of the flow		*/
} astg_flow_t;

typedef struct io_t {
  astg_bool from_file; /* reading from file or string?		*/
  astg_bool errflag;   /* used by io_error			*/
  char *inbuf;         /* input buffer for stream		*/
  int in_len;          /* length of input buffer		*/
  int line_n;          /* which line				*/
  char *next_c;        /* current character pointer		*/
  char *buffer;        /* for reading from a file		*/
  int buflen;          /* length of input buffer		*/
  FILE *stream;        /* stream if from file			*/
  char *s;             /* current input buffer			*/
  char *p;             /* current position in input		*/
  int save_one;        /* single-character undo        	*/
} io_t;

typedef astg_bool (*astg_pi_fcn)(char *, astg_graph *, void *);

char *astg_make_name(char *, astg_trans_enum, int);

astg_signal *astg_new_sig(astg_graph *, astg_signal_enum);

astg_signal *astg_dup_signals(astg_signal *);

astg_bool astg_sim_node(node_t *, astg_pi_fcn, astg_graph *, void *);

int astg_register(astg_graph *);

void astg_write_marking(astg_graph *, FILE *);

astg_retval get_sm_comp(astg_graph *, astg_bool);

astg_retval get_mg_comp(astg_graph *, astg_bool);

astg_retval astg_usc(astg_graph *);

astg_retval astg_check_sm(astg_graph *);

void astg_lock_graph_shortest_path(astg_graph *, astg_signal *);

int astg_lock_graph_components(astg_graph *, int (*)(array_t *, int, void *),
                               void *);

astg_bool astg_is_rel(astg_trans *, astg_trans *);

void astg_set_marking(astg_graph *, astg_marking *);

astg_bool astg_set_guard(astg_edge *, char *, io_t *);

astg_scode astg_marking_enabled(astg_marking *);

void astg_make_place_name(astg_graph *, astg_place *);

void io_open(io_t *, FILE *, char *);

static void io_unget(int, io_t *);

int io_get(io_t *);

static int io_getc(io_t *);

int io_error(io_t *, char *);

astg_trans *find_trans_by_name(io_t *, astg_graph *, astg_bool);

void astg_read_marking(io_t *, astg_graph *);

void astg_do_daemons(astg_graph *, astg_graph *, astg_daemon_enum);

void astg_discard_daemons();

array_t *astg_check_new_state(astg_flow_t *, astg_scode);

void astg_flow_setup(astg_flow_t *, astg_graph *);

#endif /* ASTG_CORE_H */
