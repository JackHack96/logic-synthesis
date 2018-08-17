
#include "sis.h"
#include "../include/com_int.h"

network_t *backup_network;
char      sis_shell_char = '!';      /* can be reset using the "set shell_char" */

static int     autoexec;        /* indicates currently in autoexec */
static jmp_buf env;

static int com_dispatch();

static int apply_alias();

static char *split_line();

static int check_shell_escape();


#ifdef _IBMR2
/* ARGSUSED */
static RETSIGTYPE
sigterm()
{
    (void) signal(SIGINT, SIG_IGN);	/* ignore further ctl-c */
    longjmp(env, 1);
}
#else

/* ARGSUSED */
static int
sigterm(sig, code, scp) {
    (void) signal(SIGINT, SIG_IGN);    /* ignore further ctl-c */
    longjmp(env, 1);
}

#endif


com_execute(network, command
)
network_t **network;
char      *command;
{
int  status, argc;
int  loop;
char *commandp, **argv;

(void) signal(SIGINT, SIG_IGN);
commandp = command;
do {
if (
check_shell_escape(commandp,
&status)) break;
commandp = split_line(commandp, &argc, &argv);
loop     = 0;
status   = apply_alias(network, &argc, &argv, &loop);
if (status == 0) {
status = com_dispatch(network, argc, argv);
}
com_free_argv(argc, argv
);
} while (status == 0 && *commandp != '\0');
return
status;
}

static int
com_dispatch(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int             status;
    char            *value;
    command_descr_t *descr;

    if (argc == 0) {        /* empty command */
        return 0;
    }

    if (!avl_lookup(command_table, argv[0], &value)) {
        (void) fprintf(miserr, "unknown command '%s'\n", argv[0]);
        return 1;
    }

    descr = (command_descr_t *) value;
    if (setjmp(env)) {
        /* return from control-c -- restore the network */
        if (descr->changes_network) {
            *network = backup_network;
            backup_network = NIL(network_t);
        }
        return 1;
    }

    if (descr->changes_network) {
        if (backup_network != NIL(network_t)) {
            network_free(backup_network);
        }
        backup_network = network_dup(*network);
    }

    (void) signal(SIGINT, sigterm);
    status = (*descr->command_fp)(network, argc, argv);

    /* automatic execution of arbitrary command after each command */
    /* usually this is a passive command ... */
    if (status == 0 && !autoexec) {
        if (avl_lookup(flag_table, "autoexec", &value)) {
            autoexec = 1;
            status   = com_execute(network, value);
            autoexec = 0;
        }
    }

    (void) signal(SIGINT, SIG_IGN);
    return status;
}

/*
 * Apply alias.
 * If perform a history substitution in expanding an alias, remove all the
 * orginal trailing arguments.  e.g
 * > alias t rl \!:1
 * > t lion.blif  would otherwise expand to   rl lion.blif lion.blif
 */
static int
apply_alias(network, argcp, argvp, loop)
        network_t **network;
        int *argcp;
        char ***argvp;
        int *loop;
{
    int           i, argc, stopit, added, offset, did_subst, subst, status, newc, j;
    char          *arg, **argv, **newv;
    alias_descr_t *alias;

    argc   = *argcp;
    argv   = *argvp;
    stopit = 0;
    for (; *loop < 20; (*loop)++) {
        if (argc == 0) {
            return 0;
        }
        if (stopit != 0 || avl_lookup(alias_table, argv[0], (char **) &alias) == 0) {
            return 0;
        }
        if (strcmp(argv[0], alias->argv[0]) == 0) {
            stopit = 1;
        }
        FREE(argv[0]);
        added  = alias->argc - 1;

        /* shift all the arguments to the right */
        if (added != 0) {
            argv   = REALLOC(
                    char *, argv, argc + added);
            for (i = argc - 1; i >= 1; i--) {
                argv[i + added] = argv[i];
            }
            for (i = 1; i <= added; i++) {
                argv[i] = NIL(
                        char);
            }
            argc += added;
        }
        subst  = 0;
        for (i = 0, offset = 0; i < alias->argc; i++, offset++) {
            arg    = hist_subst(alias->argv[i], &did_subst);
            if (arg == NIL(char)) {
                *argcp = argc;
                *argvp = argv;
                return (1);
            }
            if (did_subst != 0) {
                subst = 1;
            }
            status = 0;
            do {
                arg = split_line(arg, &newc, &newv);
                /*
                 * If there's a complete `;' terminated command in `arg',
                 * when split_line() returns arg[0] != '\0'.
                 */
                if (arg[0] == '\0') {   /* just a bunch of words */
                    break;
                }
                status = apply_alias(network, &newc, &newv, loop);
                if (status == 0) {
                    status = com_dispatch(network, newc, newv);
                }
                com_free_argv(newc, newv);
            } while (status == 0);
            if (status != 0) {
                *argcp = argc;
                *argvp = argv;
                return (1);
            }
            added  = newc - 1;
            if (added != 0) {
                argv   = REALLOC(
                        char *, argv, argc + added);
                for (j = argc - 1; j > offset; j--) {
                    argv[j + added] = argv[j];
                }
                argc += added;
            }
            for (j = 0; j <= added; j++) {
                argv[j + offset] = newv[j];
            }
            FREE(newv);
            offset += added;
        }
        if (subst == 1) {
            for (i = offset; i < argc; i++) {
                FREE(argv[i]);
            }
            argc = offset;
        }
        *argcp = argc;
        *argvp = argv;
    }

    (void) fprintf(siserr, "error -- alias loop\n");
    return 1;
}

static char *
split_line(command, argc, argv)
        char *command;
        int *argc;
        char ***argv;
{
    register char *p, *start, c;
    register int  i, j;
    register char *new_arg;
    array_t       *argv_array;
    int           single_quote, double_quote;

    argv_array = array_alloc(
            char *, 5);

    p = command;
    for (;;) {
        /* skip leading white space */
        while (isspace(*p)) {
            p++;
        }

        /* skip until end of this token */
        single_quote = double_quote = 0;
        for (start   = p; (c = *p) != '\0'; p++) {
            if (c == ';' || c == '#' || isspace(c)) {
                if (!single_quote && !double_quote) {
                    break;
                }
            }
            if (c == '\'') {
                single_quote = !single_quote;
            }
            if (c == '"') {
                double_quote = !double_quote;
            }
        }
        if (single_quote || double_quote) {
            (void) fprintf(miserr, "ignoring unbalanced quote ...\n");
        }
        if (start == p) break;

        new_arg    = ALLOC(
                char, p - start + 1);
        j          = 0;
        for (i     = 0; i < p - start; i++) {
            c = start[i];
            if (c != '\'' && c != '\"') {
                new_arg[j++] = isspace(c) ? ' ' : start[i];
            }
        }
        new_arg[j] = '\0';
        array_insert_last(
                char *, argv_array, new_arg);
    }

    *argc = array_n(argv_array);
    *argv = array_data(
            char *, argv_array);
    array_free(argv_array);
    if (*p == ';') {
        p++;
    } else if (*p == '#') {
        for (; *p != 0; p++);        /* skip to end of line */
    }
    return p;
}

static int
check_shell_escape(p, status)
        char *p;
        int *status;
{
    char *value;
    while (isspace(*p)) {
        p++;
    }
    if ((value = com_get_flag("shell_char")) != NIL(char)) {
        sis_shell_char = *value;
    }
    if (*p == sis_shell_char) {
        *status = system(p + 1);
        return 1;
    }
    return 0;
}
