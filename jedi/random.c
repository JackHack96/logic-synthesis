
/*
 * Symbolic encoding program for compiling a symbolic
 * description into a binary representation.  Target
 * is multi-level logic synthesis
 *
 * History:
 *
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#include "inc/jedi.h"

random_embedding() {
    int i, j;
    int random_code;
    long time();

    /*
     * generate random seed
     */
    if (drandomFlag) {
        srandom((int) time(NIL(long)));
    }

    /*
     * for each enumerated type
     */
    for (i = 0; i < ne; i++) {
        /*
         * erase previous assignments
         */
        for (j = 0; j < enumtypes[i].nc; j++) {
            enumtypes[i].codes[j].assigned   = FALSE;
            enumtypes[i].codes[j].symbol_ptr = 0;
        }

        /*
         * reassign using random encoding
         */
        for (j = 0; j < enumtypes[i].ns; j++) {
            random_code = random() % enumtypes[i].nc;
            while (enumtypes[i].codes[random_code].assigned) {
                random_code = random() % enumtypes[i].nc;
            }
            enumtypes[i].codes[random_code].assigned   = TRUE;
            enumtypes[i].codes[random_code].symbol_ptr = j;
            enumtypes[i].symbols[j].code_ptr           = random_code;
        }
    }
} /* end of random_embedding */
