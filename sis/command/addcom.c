
#include "sis.h"
#include "com_int.h"

avl_tree *command_table;


/* ARGSUSED */
void
com_add_command(name, func_fp, changes)
        char *name;
        PFI func_fp;
        int changes;
{
    char            *key, *value;
    command_descr_t *descr;

    key = name;
    if (avl_delete(command_table, &key, &value)) {
        /* delete existing definition for this command */
        (void) fprintf(miserr, "warning: redefining '%s'\n", name);
        com_command_free(value);
    }

    descr = ALLOC(command_descr_t, 1);
    descr->name            = util_strsav(name);
    descr->command_fp      = func_fp;
    descr->changes_network = changes;
    assert(!avl_insert(command_table, descr->name, (char *) descr));
}


void
com_command_free(value)
        char *value;
{
    command_descr_t *command = (command_descr_t *) value;

    FREE(command->name);        /* same as key */
    FREE(command);
}
