
#include "com_int.h"
#include "sis.h"

static int end_loop();

int com_source(network_t **network, int argc, char **argv) {
    int           c, echo, prompt, silent, status, interactive, quit_count, lp_count = -1;
    int           lp_lits, lp_time, lp_file_index, did_subst;
    char          *prompt_string, *real_filename, line[MAX_STR], *command;
    delay_model_t model;
    FILE          *fp;

    model       = DELAY_MODEL_UNIT;
    interactive = silent = prompt = echo = lp_lits = lp_time = 0;

    util_getopt_reset();

    while ((c = util_getopt(argc, argv, "ipsxltm:")) != EOF) {
        switch (c) {
            case 'i': /* a hack to distinguish EOF from stdin */
                interactive = 1;
                break;
            case 'p':prompt = 1;
                break;
            case 's':silent = 1;
                break;
            case 'x':echo = 1;
                break;
            case 'l':lp_lits = 1;
                break;
            case 't':lp_time = 1;
                break;
            case 'm':model = delay_get_model_from_name(util_optarg);
                if (model == DELAY_MODEL_UNKNOWN) {
                    (void) fprintf(siserr, "Unknown delay model %s\n", util_optarg);
                    goto usage;
                }
                break;
            default:goto usage;
        }
    }

    /* Since the source command can accept arguments we will ignore this check */
    /*    if (argc - util_optind != 1) goto usage; */

    lp_file_index = util_optind;
    do {
        lp_count++; /* increment the loop counter */

        fp = com_open_file(argv[lp_file_index], "r", &real_filename, silent);
        if (fp == NULL) {
            FREE(real_filename);
            return !silent; /* error return if not silent */
        }

        quit_count = 0;
        do {
            if (prompt) {
                prompt_string = com_get_flag("prompt");
#ifdef SIS
                if (prompt_string == NIL(char))
                    prompt_string = "sis> ";
#else
                if (prompt_string ==

                    NIL(char)

                )
                  prompt_string = "misII> ";
#endif
                /*	    (void) fprintf(stdout, "%s", prompt_string); */
            } else {
                prompt_string = NIL(char);
            }

            /* clear errors -- e.g., EOF reached from stdin */
            clearerr(fp);

            /* read another command line */
            if (fgets_filec(line, MAX_STR, fp, prompt_string) == NULL) {
                if (interactive) {
                    if (quit_count++ < 5) {
                        (void) fprintf(miserr, "\nUse \"quit\" to leave sis.\n");
                        continue;
                    }
                    status = -1; /* fake a 'quit' */
                } else {
                    status = 0; /* successful end of 'source' ; loop? */
                }
                break;
            }
            quit_count = 0;

            if (echo) {
                (void) fprintf(misout, "%s", line);
            }
            command = hist_subst(line, &did_subst);
            if (command == NIL(char)) {
                status = 1;
                break;
            }
            if (did_subst) {
                if (interactive) {
                    (void) fprintf(stdout, "%s\n", command);
                }
            }
            if (command != line) {
                (void) strcpy(line, command);
            }
            if (interactive && *line != '\0') {
                array_insert_last(char *, command_hist, util_strsav(line));

                if (sishist != NIL(FILE)) {
                    (void) fprintf(sishist, "%s\n", line);
                    (void) fflush(sishist);
                }
            }

            status = com_execute(network, line);
        } while (status == 0);

        if (fp != stdin) {
            if (status > 0) {
                (void) fprintf(miserr, "aborting 'source %s'\n", real_filename);
            }
            (void) fclose(fp);
        }
        FREE(real_filename);

    } while ((status == 0) && (lp_lits || lp_time) &&
             ((lp_count <= 0) || !end_loop(network, lp_lits, lp_time, model)));

    return status;

    usage:
    (void) fprintf(miserr, "source [-psx] filename\n");
    (void) fprintf(miserr, "\t-p supply prompt before reading each line\n");
    (void) fprintf(miserr, "\t-s silently ignore nonexistant file\n");
    (void) fprintf(miserr, "\t-x echo each line as it is executed\n");
    (void) fprintf(miserr, "\t-l loop while literal count decreases\n");
    (void) fprintf(miserr, "\t-t loop white the arrival time decreases\n");
    (void) fprintf(miserr,
                   "\t-m model : delay model to measure arrival time with\n");
    return 1;
}

#define SCR_EQ(a, b) (ABS((a) - (b)) < 0.001) /* Floating point equality */

/* ARGSUSED */
static int end_loop(network_t **network, int lp_lits, int lp_time,
                    delay_model_t model) {
    static int       prev_count    = -1;
    static double    prev_delay    = -1.0;
    static network_t *prev_network = NIL(network_t);
    int              count         = 0;
    double           delay;
    node_t           *node;
    lsGen            gen;

    foreach_node(*network, gen, node) {
        if (node->type == INTERNAL)
            count += factor_num_literal(node);
    }
    if (lp_lits) {
        if (prev_count < 0) {
            prev_network = network_dup(*network);
            prev_count   = count;
            return 0;
        } else if (count < prev_count) {
            network_free(prev_network);
            prev_network = network_dup(*network);
            prev_count   = count;
            return 0;
        } else {
            network_free(*network);
            *network = prev_network;
            prev_network = NIL(network_t);
            prev_count   = -1;
            return 1;
        }
    } else if (lp_time) {
        (void) delay_trace(*network, model);
        (void) delay_latest_output(*network, &delay);
        if (prev_delay < 0) {
            prev_network = network_dup(*network);
            prev_delay   = delay;
            prev_count   = count;
            return 0;
        } else if ((delay < prev_delay) ||
                   (SCR_EQ(delay, prev_delay) && (count < prev_count))) {
            network_free(prev_network);
            prev_network = network_dup(*network);
            prev_delay   = delay;
            prev_count   = count;
            return 0;
        } else {
            network_free(*network);
            *network = prev_network;
            prev_network = NIL(network_t);
            prev_delay   = -1;
            prev_count   = -1;
            return 1;
        }
    }
    return 1;
}

#undef SCR_EQ
