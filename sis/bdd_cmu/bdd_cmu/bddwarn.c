/* BDD error and argument checking routines */

#include <stdio.h>
#include <stdarg.h>

#include "bddint.h"

extern void exit(int);

/* cmu_bdd_warning(message) prints a warning and returns. */

void cmu_bdd_warning(char *message) {
    fprintf(stderr, "BDD library: warning: %s\n", message);
}

/* cmu_bdd_fatal(message) prints an error message and exits. */

void cmu_bdd_fatal(char *message) {
    fprintf(stderr, "BDD library: error: %s\n", message);
    exit(1);
    /* NOTREACHED */
}

int bdd_check_arguments(int count, ...) {
    int     all_valid;
    va_list ap;
    bdd     f;

    va_start(ap, count);
    all_valid = 1;
    while (count) {
        f = va_arg(ap, bdd);
        {
            BDD_SETUP(f);
            if (!f)
                all_valid = 0;
            else if (BDD_REFS(f) == 0)
                cmu_bdd_fatal("bdd_check_arguments: argument has zero references");
        }
        --count;
    }
    return (all_valid);
}

void bdd_check_array(bdd *fs) {
    while (*fs) {
        bdd_check_arguments(1, *fs);
        ++fs;
    }
}
