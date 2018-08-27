extern void io_fprintf_break(FILE *, char *, ...);

extern void io_fputs_break();

extern void io_fputc_break();

extern void io_fput_params();

extern void write_sop();

extern char *io_node_name();

extern char *io_name();

extern void io_write_name();

extern void io_write_node();

extern void io_write_gate();

extern int io_po_fanout_count();

extern int io_rpo_fanout_count();

extern int io_lpo_fanout_count();

extern int io_node_should_be_printed();

extern void read_filename_to_netname();

extern node_t *read_find_or_create_node();

extern node_t *read_slif_find_or_create_node();

extern int read_check_io_list();

extern void read_hack_outputs();

extern void read_cleanup_buffers();

#ifdef SIS
extern void read_delay_cleanup();
extern void read_change_madeup_name();
#endif /* SIS */

extern node_t *read_add_fake_primary_output();

extern void write_blif_for_bdsyn();

extern void write_blif_slif_delay();

extern int read_lineno;
extern char *read_filename;

extern int com_write_slif();

extern int com_plot_blif();

extern char *gettoken();

#define MAX_WORD 1024

typedef struct {
  array_t *actuals; /* Array of nodes: inputs, then outputs */
  char **formals;   /* Array of formals */
  int inputs;       /* How many entries in `actuals' are inputs */
  char *netname;    /* Network to patch this network into */
} patchinfo;

typedef struct {           /* Data field in the models table */
  network_t *network;      /* The network */
  lsList po_list;          /* The po_list for the network */
  lsList latch_order_list; /* Ordering for latches in stg */
  lsList patch_lists;      /* Who depends on me */
  int depends_on;          /* How many I depend on */
  int library;             /* Is this going to be put in library? */
} modelinfo;

extern modelinfo *read_find_or_create_model();

#ifdef SIS
extern int slif_add_to_library();
#endif /* SIS */

extern int read_blif_slif();
