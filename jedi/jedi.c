
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

FILE *infp;            /* input file pointer */
FILE *outfp;            /* output file pointer */

main(argc, argv
)
int  argc;
char **argv;
{
/*
 * parse options
 */
parse_options(argc, argv
);

/*
 * read input from stdin
 */
if (kissFlag) {
read_kiss(infp, outfp
);
} else {
read_input(infp, outfp
);
}

/*
 * check if one hot
 */
if (hotFlag) {
write_one_hot(outfp);
(void) exit(0);
}

/*
 * compute weights
 */
if (!sequentialFlag && !srandomFlag && !drandomFlag) {
compute_weights();
}

/*
 * solve the embedding problem
 */
if (srandomFlag || drandomFlag) {
random_embedding();
} else if (clusterFlag) {
cluster_embedding();
} else if (!sequentialFlag) {
opt_embedding();
}

if (kissFlag && !plaFlag) {
write_blif(outfp);
} else if (kissFlag && plaFlag) {
write_output(outfp);
} else {
write_output(outfp);
}

(void) exit(0);
} /* end of main */
