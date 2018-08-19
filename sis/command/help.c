
#include "sis.h"
#include "../include/com_int.h"


static char *
command_alias_help(command)
        char *command;
{
    char          *value;
    alias_descr_t *alias;

    if (!avl_lookup(alias_table, command, &value)) {
        return command;
    }
    alias = (alias_descr_t *) value;
    return alias->argv[0];
}

/* ARGSUSED */
com_help(network, argc, argv)
network_t **network;
int       argc;
char      **argv;
{
int           c, i, all;
char          *geom = NULL;
char          *key;
avl_generator *gen;
char          buffer[1024];
char          *command;
char          *lib_name;
FILE          *gfp;

util_getopt_reset();

all = 0;
while ((
c = util_getopt(argc, argv, "ag:")
) != EOF) {
switch (c) {
case 'a':
all = 1;
break;
case 'g':
geom = util_optarg;
break;
default: goto
usage;
}
}

if (!all &&

com_graphics_enabled()

) {
gfp = com_graphics_open("help", "help", "new");
if (geom)
fprintf(gfp,
".geometry\t%s\n", geom);
if (argc - util_optind == 1) {
command = command_alias_help(argv[util_optind]);
} else {
command = "help";
}
fprintf(gfp,
".topic\t%s\n", command);
com_graphics_close (gfp);
} else if (argc - util_optind == 0) {
i = 0;
avl_foreach_item(command_table, gen, AVL_FORWARD,
                 &key,

                 NIL(
                         char *)

                ) {
if ((key[0] == '_') == all) {
(void)
fprintf(sisout,
"%-15s", key);
if ((++i%5) == 0) (void)
fprintf(sisout,
"\n");
}
}
if ((i%5) != 0) (void)
fprintf(sisout,
"\n");
} else if (argc - util_optind == 1) {
command  = command_alias_help(argv[util_optind]);
lib_name = sis_library();
#if defined(__CYGWIN__)
(void) sprintf(buffer, "less %s/help/%s.fmt", lib_name, command);
#else
(void)
sprintf(buffer,
"more %s/help/%s.fmt", lib_name, command);
#endif
(void)
system(buffer);
FREE(lib_name);
} else {
goto
usage;
}

return 0;

usage:
(void)
fprintf(siserr,
"usage: help [-a] [command]\n");
return 1;
}
