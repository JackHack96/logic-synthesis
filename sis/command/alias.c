
#include "sis.h"
#include "../include/com_int.h"

avl_tree *alias_table;


static void
print_alias(value)
        char *value;
{
    int           i;
    alias_descr_t *alias;

    alias = (alias_descr_t *) value;
    (void) fprintf(misout, "%s\t", alias->name);
    for (i = 0; i < alias->argc; i++) {
        (void) fprintf(misout, " %s", alias->argv[i]);
    }
    (void) fprintf(misout, "\n");
}


/* ARGSUSED */
com_alias(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int           i;
char          *key, *value;
alias_descr_t *alias;
avl_generator *gen;

if (argc == 1) {
avl_foreach_item(alias_table, gen, AVL_FORWARD,
&key, &value) {
print_alias(value);
}
return 0;

} else if (argc == 2) {
if (
avl_lookup(alias_table, argv[1],
&value)) {
print_alias(value);
}
return 0;
}

/* delete any existing alias */
key = argv[1];
if (
avl_delete(alias_table,
&key, &value)) {
com_alias_free(value);
}

alias = ALLOC(alias_descr_t, 1);
alias->
name = util_strsav(argv[1]);
alias->
argc = argc - 2;
alias->
argv = ALLOC(
char *, alias->argc);
for (
i = 2;
i<argc;
i++) {
alias->argv[i-2] =
util_strsav(argv[i]);
}
assert(!
avl_insert(alias_table, alias
->name, (char *) alias));
return 0;
}

/* ARGSUSED */
com_unalias(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int  i;
char *key, *value;

if (argc < 2) {
(void)
fprintf(miserr,
"usage: unalias name1 name2 ...\n");
return 1;
}

for (
i = 1;
i<argc;
i++) {
key = argv[i];
if (
avl_delete(alias_table,
&key, &value)) {
com_alias_free(value);
}
}
return 0;
}

void
com_free_argv(argc, argv)
        int argc;
        char **argv;
{
    int i;

    for (i = 0; i < argc; i++) {
        FREE(argv[i]);
    }
    FREE(argv);
}


void
com_alias_free(value)
        char *value;
{
    alias_descr_t *alias = (alias_descr_t *) value;

    com_free_argv(alias->argc, alias->argv);
    FREE(alias->name);        /* same as key */
    FREE(alias);
}
