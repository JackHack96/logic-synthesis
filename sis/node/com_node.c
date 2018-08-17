
#include "sis.h"
#include "../include/node_int.h"

com_div(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
array_t *node_vec;
node_t  *node1, *node2, *q, *r;

node_vec = com_get_nodes(*network, argc, argv);
if (
array_n(node_vec)
!= 2) {
(void)
fprintf(miserr,
"usage: _div n1 n2 -- test division\n");
return 1;
}

node1 = array_fetch(node_t * , node_vec, 0);
node2 = array_fetch(node_t * , node_vec, 1);

q = node_div(node1, node2, &r);
(void)
fprintf(misout,
"q = ");
node_print_rhs(misout, q
);
(void)
fprintf(misout,
"\nr = ");
node_print_rhs(misout, r
);
(void)
fprintf(misout,
"\n");
node_free(q);
node_free(r);

array_free(node_vec);
return 0;
}

com_sim(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
array_t         *node_vec;
node_t          *node;
node_sim_type_t mode;
int             c, i;

mode = NODE_SIM_ESPRESSO;

util_getopt_reset();

while ((
c = util_getopt(argc, argv, "m:")
) != EOF) {
switch (c) {
case 'm':
if (
strcmp(util_optarg,
"simpcomp") == 0) {
mode = NODE_SIM_SIMPCOMP;
} else if (
strcmp(util_optarg,
"espresso") == 0) {
mode = NODE_SIM_ESPRESSO;
} else if (
strcmp(util_optarg,
"exact") == 0) {
mode = NODE_SIM_EXACT;
} else if (
strcmp(util_optarg,
"exact-lits") == 0) {
mode = NODE_SIM_EXACT_LITS;
} else {
goto
usage;
}
break;
default:
goto
usage;
}
}

node_vec = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
for (
i = 0;
i <
array_n(node_vec);
i++) {
node = array_fetch(node_t * , node_vec, i);
switch (
node_function(node)
) {
case NODE_PI:
case NODE_PO:
break;
default:
(void)
node_simplify_replace(node, NIL(node_t), mode
);
}
}
array_free(node_vec);
return 0;
usage:
(void)
fprintf(miserr,
"_sim [node-list] -- test simplification\n");
(void)
fprintf(miserr,
"    -m simpcomp\t\tselect fast heuristic\n");
(void)
fprintf(miserr,
"    -m espresso\t\tminimize using espresso\n");
(void)
fprintf(miserr,
"    -m exact\t\tminimize using exact (cubes)\n");
(void)
fprintf(miserr,
"    -m exact-lits\tminimize using exact (lits)\n");
return 1;
}

com_cof(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
array_t *node_vec;
node_t  *node1, *node2, *r;

node_vec = com_get_nodes(*network, argc, argv);
if (
array_n(node_vec)
!= 2) {
(void)
fprintf(miserr,
"usage: _cof n1 n2 -- test cofactor\n");
return 1;
}

node1 = array_fetch(node_t * , node_vec, 0);
node2 = array_fetch(node_t * , node_vec, 1);

r = node_cofactor(node1, node2);
(void)
fprintf(misout,
"cof = ");
node_print_rhs(misout, r
);
(void)
fprintf(misout,
"\n");
node_free(r);

array_free(node_vec);
return 0;
}

com_wd(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
array_t *node_vec;
node_t  *node1, *node2;
int     c, use_complement;

use_complement = 0;

util_getopt_reset();

while ((
c = util_getopt(argc, argv, "c")
) != EOF) {
switch (c) {
case 'c':
use_complement = 1;
break;
default:
goto
usage;
}
}

node_vec = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
if (
array_n(node_vec)
!= 2) goto
usage;

node1 = array_fetch(node_t * , node_vec, 0);
node2 = array_fetch(node_t * , node_vec, 1);
(void)
node_substitute(node1, node2, use_complement
);
array_free(node_vec);
return 0;

usage:
(void)
fprintf(miserr,
"usage: wd [-c] n1 n2 -- re-express n1 using n2\n");
(void)
fprintf(miserr,
"    -c\t\talso use the complement of n2\n");
return 1;
}

com_scc(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
array_t *node_vec;
node_t  *node;
int     i;

if (argc < 2) goto
usage;

node_vec = com_get_nodes(*network, argc, argv);
if (
array_n(node_vec)
< 1) goto
usage;
for (
i = 0;
i <
array_n(node_vec);
i++) {
node = array_fetch(node_t * , node_vec, i);
(void)
node_scc(node);
}
array_free(node_vec);
return 0;

usage:
(void)
fprintf(miserr,
"usage: scc n1 n2 ...\n");
return 1;
}

com_invert(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
array_t *node_vec;
node_t  *node;
int     i;

if (argc < 2) goto
usage;

node_vec = com_get_nodes(*network, argc, argv);
if (
array_n(node_vec)
< 1) goto
usage;
for (
i = 0;
i <
array_n(node_vec);
i++) {
node = array_fetch(node_t * , node_vec, i);
(void)
node_invert(node);
}
array_free(node_vec);
return 0;

usage:
(void)
fprintf(miserr,
"usage: invert n1 n2 ...\n");
return 1;
}

init_node() {
    com_add_command("wd", com_wd, 1);
    com_add_command("invert", com_invert, 1);
    com_add_command("_scc", com_scc, 1);
    com_add_command("_sim", com_sim, 0);
    com_add_command("_div", com_div, 0);
    com_add_command("_cof", com_cof, 0);

    define_cube_size(20);
    set_espresso_flags();
    name_mode = LONG_NAME_MODE;
}


end_node() {
    node_discard_all_daemons();
    undefine_cube_size();
}
