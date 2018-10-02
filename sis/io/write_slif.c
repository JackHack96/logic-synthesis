
#include "io_int.h"
#include "sis.h"

/*
 * Simply modified write_blif to output using slif keywords.
 */

static void write_slif_node();

static void write_slif_delay();

static int write_slif_ignore_node();

#ifdef SIS

static void write_slif_latches();

#endif /* SIS */

int write_slif(FILE *fp, network_t *network, int short_flag, int netlist_flag, int delay_flag) {
    lsGen    gen;
    node_t   *p;
    st_table *ignore_table;

    io_fprintf_break(fp, ".model %s;\n.inputs", network_name(network));
    /*
     * Do not print out latch outputs (print clocks as just inputs)
     */
    foreach_primary_input(network, gen, p) {
#ifdef SIS
        if (latch_from_node(p) == NIL(latch_t)) {
            io_fputc_break(fp, ' ');
            io_write_name(fp, p, short_flag);
        }
#else
        io_fputc_break(fp, ' ');
        io_write_name(fp, p, short_flag);
#endif /* SIS */
    }

    io_fputs_break(fp, ";\n.outputs");
    /*
     * Don't print out control outputs and latch_inputs
     */
    foreach_primary_output(network, gen, p) {
#ifdef SIS
        if (network_is_real_po(network, p) != 0) {
            io_fputc_break(fp, ' ');
            io_write_name(fp, p, short_flag);
        }
#else
        io_fputc_break(fp, ' ');
        io_write_name(fp, p, short_flag);
#endif /* SIS */
    }
    io_fputs_break(fp, ";\n");

    if (delay_flag != 0) {
        write_slif_delay(fp, network);
    }

    /*
     * We should also not write out inverters in the case when the
     * node or its fanin has a name that has an "'" at the end.
     * The inverter will be inferred by the read_slif() routines
     * We will also swap names in the case the inverter has the
     * uninverted name "foo" and its fanin is named "foo'"
     * Also constant nodes with names "0" and "1" cause problems in
     * reading in eqn strings of the type "0 = 0;"
     */
    ignore_table = st_init_table(st_ptrcmp, st_ptrhash);
    if (short_flag == 0) {
        foreach_node(network, gen, p) {
            if (write_slif_ignore_node(p)) {
                (void) st_insert(ignore_table, (char *) p, NIL(char));
            }
        }
    }

#ifdef SIS

    write_slif_latches(fp, network, short_flag, netlist_flag);

    foreach_node(network, gen, p) {
        if (!st_is_member(ignore_table, (char *) p)) {
            if (netlist_flag != 0 && lib_gate_of(p) != NIL(lib_gate_t)) {
                if (io_lpo_fanout_count(p, NIL(node_t *)) == 0) {
                    write_slif_node(fp, p, netlist_flag, short_flag, NIL(node_t));
                }
            } else {
                write_slif_node(fp, p, netlist_flag, short_flag, NIL(node_t));
            }
        }
    }

#else

    foreach_node(network, gen, p) {
      if (!st_is_member(ignore_table, (char *)p)) {
        write_slif_node(fp, p, netlist_flag, short_flag);
      }
    }

#endif /* SIS */

    io_fprintf_break(fp, ".endmodel %s;\n", network_name(network));
    st_free_table(ignore_table);
}

#ifdef SIS

static void write_slif_node(FILE *fp, node_t *node, int netlist_flag, int short_flag, node_t *latch_in)

#else

static void write_slif_node(fp, node, netlist_flag, short_flag) FILE *fp;
node_t *node;
int netlist_flag, short_flag;

#endif /* SIS */
{
    int        i;
    node_t     *fanin;
    char       *name;
    lib_gate_t *gate;

    if (io_node_should_be_printed(node) == 0) {
        return;
    }
    gate = lib_gate_of(node);

    if (netlist_flag == 0 || gate == NIL(lib_gate_t)) {
        write_sop(fp, node, short_flag, 1);
    } else {
        name = lib_gate_name(gate);
        io_fprintf_break(fp, ".call %s %s (", name, name);
#ifdef SIS
        foreach_fanin(node, i, fanin) {
            if (lib_gate_latch_pin(gate) != i) {
                if (i > 0) {
                    io_fputs_break(fp, ", ");
                }
                io_fputs_break(fp, io_name(fanin, short_flag));
            }
        }
        if (latch_in != NIL(node_t)) {
            node = latch_get_output(latch_from_node(latch_in));
        }
#else
        foreach_fanin(node, i, fanin) {
          if (i > 0) {
            io_fputs_break(fp, ", ");
          }
          io_fputs_break(fp, io_name(fanin, short_flag));
        }
#endif /* SIS */
        io_fprintf_break(fp, " ; ; %s);\n", io_name(node, short_flag));
    }
}

static void write_slif_delay(FILE *fp, network_t *network) { write_blif_slif_delay(fp, network, 1); }

#ifdef SIS

static void write_slif_latches(FILE *fp, network_t *network, int netlist_flag, int short_flag) {
    lsGen   gen;
    latch_t *latch;
    node_t  *latchin, *node, *control;

    foreach_latch(network, gen, latch) {
        if (netlist_flag != 0 && latch_get_gate(latch) != NIL(lib_gate_t)) {
            latchin = latch_get_input(latch);
            node    = node_get_fanin(latchin, 0);
            write_slif_node(fp, node, 1, short_flag, latchin);
        } else {
            io_write_name(fp, latch_get_output(latch), short_flag);
            io_fputs_break(fp, " = @D(");
            io_write_name(fp, latch_get_input(latch), short_flag);
            io_fputs_break(fp, ", ");
            control = latch_get_control(latch);
            if (control == NULL) {
                io_fputs_break(fp, "NIL");
            } else {
                io_write_name(fp, control, short_flag);
            }
            io_fputs_break(fp, ");\n");
        }
    }
}

#endif /* SIS */

/*
 * Return 1 if we do want this inverter to be printed
 */
static int write_slif_ignore_node(node_t *node) {
    node_t *fanin;
    int    len1, len2;
    char   *name1, *name2, *sav1, *sav2;
    int    ignore = FALSE;

    if (node->type != INTERNAL) {
        return ignore;
    } else if (node_function(node) == NODE_INV) {
        name1 = node_long_name(node);
        len1  = strlen(name1);
        fanin = node_get_fanin(node, 0);
        name2 = node_long_name(fanin);
        len2  = strlen(name2);

        if (name1[len1 - 1] == '\047') {
            name1[len1 - 1] = '\0';
            if (strcmp(name1, name2) == 0) {
                ignore = TRUE;
            }
            name1[len1 - 1] = '\047';
        } else if (name2[len2 - 1] == '\047') {
            name2[len2 - 1] = '\0';
            if (strcmp(name1, name2) == 0) {
                ignore = TRUE;
            }
            name2[len2 - 1] = '\047';
            if (ignore) {
                /*
                 * Keep the name with the "'" at the inverting node --- First give the
                 * node a name that will not clash with the manes being added
                 */
                sav1 = util_strsav(name1);
                sav2 = util_strsav(name2);
                network_change_node_name(node_network(node), node,
                                         util_strsav("#temp"));
                network_change_node_name(node_network(node), fanin, sav1);
                network_change_node_name(node_network(node), node, sav2);
            }
        }
    } else if (node_function(node) == NODE_0 &&
               (strcmp(node_long_name(node), "0") == 0)) {
        ignore = TRUE;
    } else if (node_function(node) == NODE_1 &&
               (strcmp(node_long_name(node), "1") == 0)) {
        ignore = TRUE;
    }
    return ignore;
}

#if 0

char *
replace_chars_in(char* node_name)
{
   /* MisII allows some charcaters in its variable names
    * that are not allowed in SLIF.
    * This routine scans variable names and replaces those characters
    * by other ones. It currently replaces 
    *      '-' by ':',
    *      '(' by '[' and 
    *      ')' by ']'.
    */
   static char *buffer = NULL;
   int t1 = 0;

   if (buffer == NULL)
   {
      buffer = ALLOC(char, strlen(node_name) + 2);
   }
   else if (strlen(buffer) < strlen(node_name))
   {
      if (buffer != NULL) FREE(buffer);
      buffer = ALLOC(char, strlen(node_name) + 2);
   }

   while (node_name[t1] != '\0')
   {
      if (node_name[t1] == '-')
      {
         buffer[t1] = ':';
      }
      else if (node_name[t1] == ')')
      {
         buffer[t1] = ']';
      }
      else if (node_name[t1] == '(')
      {
         buffer[t1] = '[';
      }
      else
      {
         buffer[t1] = node_name[t1];
      }
      t1++;
    }
    buffer[t1] = '\0';

    return(buffer);
}

void
io_write_slif_name(FILE* fp, node_t* node, int short_flag)
{
    node_t *po;

    if (node->type == INTERNAL && io_po_fanout_count(node, &po) == 1) {
    io_fputs_break(fp, short_flag ? po->short_name : replace_chars_in(po->name));
    } else {
    io_fputs_break(fp, short_flag ? node->short_name : replace_chars_in(node->name));
    }
}

int
own_strcmp(p,q)
char *p, *q;
{
   if ((p == NULL) || (q == NULL)) return -1;

   return(strcmp(p,q));
}

#endif /* 0 */
