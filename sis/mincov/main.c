
#include "util.h"
#include "sparse.h"
#include "mincov.h"


/*
 *  open_file -- open a file, or fail with an error message and exit
 *  Allows '-' as a synonym for standard input 
 */
static FILE *
open_file(filename, mode)
        char *filename;
        char *mode;
{
    FILE *fp;

    if (strcmp(filename, "-") == 0) {
        return mode[0] == 'r' ? stdin : stdout;
    } else if ((fp = fopen(filename, mode)) == NULL) {
        perror(filename);
        exit(1);
    }
    return fp;
}


static void
usage(prog)
        char *prog;
{
    (void) fprintf(stderr, "usage: %s [-ch] [-v #]\n", prog);
    (void) fprintf(stderr, "   -c\t\tread espresso 'compressed' pi table\n");
    (void) fprintf(stderr, "   -h\t\theuristic covering\n");
    (void) fprintf(stderr, "   -v n\t\tset verbose level to 'n' (e.g., 5)\n");
    exit(2);
}


int
main(argc, argv)
        int argc;
        char **argv;
{
    FILE      *fp;
    int       opt, c, verbose, heuristic, compressed, ok;
    char      *file;
    sm_matrix *A;
    sm_row    *cover;

#if defined(vax) || defined(sun)
    char *prog;

    prog = util_path_search(argv[0]);
    if (prog == NIL(char)) {
    (void) fprintf(stderr, "Cannot find current executable\n");
    exit(1);
    }
    util_restart(prog, "mincov.chkpt", 3600);
#endif

#if defined(bsd4_2) || defined(sun)
    setlinebuf(stdout);
#endif

    opt        = 0;
    verbose    = 0;
    compressed = 0;
    while ((c  = util_getopt(argc, argv, "cho:v:")) != EOF) {
        switch (c) {
            case 'c': compressed = 1;
                break;
            case 'h': heuristic = 1;
                break;
            case 'o': opt = atoi(util_optarg);
                break;
            case 'v': verbose = atoi(util_optarg);
                break;
            default: usage(argv[0]);
                break;
        }
    }

    if (argc - util_optind == 0) {
        file = "-";
    } else if (argc - util_optind == 1) {
        file = argv[util_optind];
    } else {
        usage(argv[0]);
    }

    fp = open_file(file, "r");
    if (compressed) {
        ok = sm_read_compressed(fp, &A);
    } else {
        ok = sm_read(fp, &A);
    }
    if (!ok) {
        (void) fprintf(stderr, "Error reading matrix\n");
        exit(1);
    }

    switch (opt) {
        case 0: if (A->nrows < 25) sm_print(fp, A);
            cover = sm_minimum_cover(A, NIL(
            int), heuristic, verbose);
            (void) printf("Solution is ");
            sm_row_print(stdout, cover);
            (void) printf("\n");

            sm_free(A);
            sm_row_free(cover);
            break;

        case 1:        /* convert old espresso format */
            sm_write(stdout, A);
            break;

        default: usage(argv[0]);
    }

    sm_cleanup();
    exit(0);
}
