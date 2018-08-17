
#include "sis.h"
#include "../include/com_int.h"

avl_tree *flag_table;


/* ARGSUSED */
com_set_variable(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
char          *flag_value, *key, *value, *set_cmd;
avl_generator *gen;

if (argc == 0 || argc > 3) {
(void)
fprintf(miserr,
"usage: set [name] [value]\n");
return 1;

} else if (argc == 1) {
avl_foreach_item(flag_table, gen, AVL_FORWARD,
&key, &value) {
(void)
fprintf(misout,
"%s\t%s\n", key, value);
}
return 0;

} else {
key = argv[1];
if (
avl_delete(flag_table,
&key, &value)) {
FREE(key);
FREE(value);
}

flag_value = argc == 2 ? util_strsav("") : util_strsav(argv[2]);

(void)
avl_insert(flag_table, util_strsav(argv[1]), flag_value
);

if (

com_graphics_enabled()

) {
/* Let graphical front-end know about new value. */
set_cmd = ALLOC(
char,
strlen(argv[1])
+
strlen(flag_value)
+2);

strcat(strcat(strcpy(set_cmd, argv[1]),

"\t"), flag_value);
com_graphics_exec ("sis", "sis", "set", set_cmd);
FREE (set_cmd);
}

if ((
strcmp(argv[1],
"sisout") == 0) ||
(
strcmp(argv[1],
"misout") == 0)) {
if (misout != stdout) (void)
fclose(misout);
if (
strcmp(flag_value,
"") == 0)
flag_value = "-";
misout     = com_open_file(flag_value, "w", NIL(
char *), 0);
if (misout == NULL)
misout = stdout;
}
if ((
strcmp(argv[1],
"siserr") == 0) ||
(
strcmp(argv[1],
"miserr") == 0)) {
if (miserr != stderr) (void)
fclose(miserr);
if (
strcmp(flag_value,
"") == 0)
flag_value = "-";
miserr     = com_open_file(flag_value, "w", NIL(
char *), 0);
if (miserr == NULL)
miserr = stderr;
}
if (
strcmp(argv[1],
"history") == 0) {
if (sishist !=
NIL(FILE)
) (void)
fclose(sishist);
if (
strcmp(flag_value,
"") == 0) {
sishist = NIL(FILE);
} else {
sishist = com_open_file(flag_value, "w", NIL(
char *), 0);
if (sishist == NULL)
sishist = NIL(FILE);
}
}
return 0;
}
}

/* ARGSUSED */
com_unset_variable(network, argc, argv
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
"usage: unset val1 val2 ...\n");
return 1;
}

for (
i = 1;
i<argc;
i++) {
key = argv[i];
if (
avl_delete(flag_table,
&key, &value)) {
FREE(key);
FREE(value);
}
}
return 0;
}

char *
com_get_flag(flag)
        char *flag;
{
    char *value;

    if (avl_lookup(flag_table, flag, &value)) {
        return value;
    } else {
        return NIL(
        char);
    }
}
