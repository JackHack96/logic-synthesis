
#include "sis.h"


static void write_instance_name();

write_bdnet(fp, network
)
FILE      *fp;
network_t *network;
{
int        i;
lsGen      gen;
char       *flag, *path, *viewname;
node_t     *node, *fanin;
lib_gate_t *gate;
#ifdef SIS
int pin_index;
latch_t *latch;
node_t *control;
#endif /* SIS */

if (!
lib_network_is_mapped(network)
) {
error_append("write_bdnet: network not mapped, cannot write netlist\n");
return 0;
}

path     = com_get_flag("OCT-CELL-PATH");
viewname = com_get_flag("OCT-CELL-VIEW");
if (viewname == 0)
viewname = "physical";

(void)
fprintf(fp,
"MODEL \"%s\";\n",
network_name(network)
);
if ((
flag = com_get_flag("OCT-TECHNOLOGY")
) != 0) {
(void)
fprintf(fp,
"    TECHNOLOGY %s;\n", flag);
}
if ((
flag = com_get_flag("OCT-VIEWTYPE")
) != 0) {
(void)
fprintf(fp,
"    VIEWTYPE %s;\n", flag);
}
if ((
flag = com_get_flag("OCT-EDITSTYLE")
) != 0) {
(void)
fprintf(fp,
"    EDITSTYLE %s;\n", flag);
}
(void)
fprintf(fp,
"\n");

(void)
fprintf(fp,
"INPUT");
foreach_primary_input(network, gen, node
) {
#ifdef SIS
if (!network_is_real_pi(network, node)) continue;
#endif /* SIS */
(void)
fprintf(fp,
"\n\t\"%s\"\t:\t\"%s\"", node->name, node->name);
}
(void)
fprintf(fp,
";\n\n");

(void)
fprintf(fp,
"OUTPUT");
foreach_primary_output(network, gen, node
) {
#ifdef SIS
if (!network_is_real_po(network, node)) continue;
#endif /* SIS */
(void)
fprintf(fp,
"\n\t\"%s\"\t:\t\"%s\"",
node->name,
node_get_fanin(node,
0)->name);
}
(void)
fprintf(fp,
";\n\n");

foreach_node(network, gen, node
) {
if (node->type == INTERNAL) {
write_instance_name(fp, path, viewname, node
);
gate = lib_gate_of(node);
#ifdef SIS
if (lib_gate_type(gate) == COMBINATIONAL) {
#endif /* SIS */
foreach_fanin(node, i, fanin
) {
(void)
fprintf(fp,
"\t\"%s\"\t:\t",
lib_gate_pin_name(gate, i, /*inflag*/ 1));
(void)
fprintf(fp,
"\"%s\";\n", fanin->name);
}
(void)
fprintf(fp,
"\t\"%s\"\t:\t",
lib_gate_pin_name(gate,
0, /*inflag*/ 0));
(void)
fprintf(fp,
"\"%s\";\n\n", node->name);
#ifdef SIS
} else {
/* Sequential support added by KJSingh --- 8/24/93 */
pin_index = lib_gate_latch_pin(gate);
foreach_fanin(node, i, fanin) {
    if (i == pin_index) continue;
    (void) fprintf(fp, "\t\"%s\"\t:\t",
           lib_gate_pin_name(gate, i, /*inflag*/ 1));
    (void) fprintf(fp, "\"%s\";\n", fanin->name);
}
/* Also connect up the control input */
latch = latch_from_node(node_get_fanout(node, 0));
if (lib_gate_type(gate) != ASYNCH) {
    if ((control = latch_get_control(latch)) != NIL(node_t)){
    control = node_get_fanin(control, 0);
    (void) fprintf(fp, "\t\"%s\"\t:\t\"%s\";\n",
               lib_gate_pin_name(gate, 0, /*inflag*/ 2),
               control->name);
    } else {
    (void) fprintf(fp, "\t\"%s\"\t:\tUNCONNECTED;\n",
               lib_gate_pin_name(gate, 0, /*inflag*/ 2));
    }
}
/* The output name is the name of the PI corresponding to
   latch o/p */
(void) fprintf(fp, "\t\"%s\"\t:\t",
           lib_gate_pin_name(gate, 0, /*inflag*/ 0));
(void) fprintf(fp, "\"%s\";\n\n",
           node_long_name(latch_get_output(latch)));
}
#endif /* SIS */
}
}

(void)
fprintf(fp,
"ENDMODEL;\n");
return 1;
}


static void
write_instance_name(fp, path, viewname, node)
        FILE *fp;
        char *path;
        char *viewname;
        node_t *node;
{
    char *gate, *view;

    gate = util_strsav(lib_gate_name(lib_gate_of(node)));
    view = strchr(gate, ':');
    if (view == 0) {
        view = viewname;
    } else {
        *view++ = '\0';
    }
    if (path == 0 || gate[0] == '/' || gate[0] == '~') {
        (void) fprintf(fp, "INSTANCE \"%s\":\"%s\"\n", gate, view);
    } else {
        (void) fprintf(fp, "INSTANCE \"%s/%s\":\"%s\"\n", path, gate, view);
    }
    FREE(gate);
}



