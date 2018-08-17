
#include "reductio.h"

mxcomptbl()

/* We expand the compatibles found by coloring ( i.e. the sets of nodes
with the same color ) until they become maximal */

{
    int  i, j, k;
    int  flag;
    pset tmp;

    if (debug > 0) {
        fprintf(stderr, "incompatibility graph\n");
        for (i = 0; i < ns; i++) {
            fprintf(stderr, "%s: ", slab[i]);
            for (j = 0; j < ns; j++) {
                if (is_in_set(GETSET(incograph, i), j)) {
                    fprintf(stderr, "%s ", slab[j]);
                }
            }
            fprintf(stderr, "\n");
        }
    }
/* clears maxcompatibles */
    if (maxcompatibles) sf_free(maxcompatibles);
    tmp            = set_new(ns);
    maxcompatibles = sf_new(0, ns);
    for (i = 0; i < ns; i++) sf_insert_set(maxcompatibles, color[i] - 1, tmp);
    set_free(tmp);

/* loop on the color array */
    for (i = 0; i < ns; i++) set_insert(GETSET(maxcompatibles, color[i] - 1), i);

/* loop on the maxcompatibles matrix */
    for (i = 0; i < colornum; i++) /* i is the color */
    {
        for (j = 0; j < ns; j++) /* j is the state */
        {
            if (!is_in_set(GETSET(maxcompatibles, i), j)) {
                k    = 0; /* k is a node adjacent to j in incograph */
                flag = 1;
                while (k < ns & flag == 1) {
                    if (is_in_set(GETSET(incograph, j), k) &&
                        is_in_set(GETSET(maxcompatibles, i), k))
                        flag = 0;
                    k++;
                }
                if (flag == 1) set_insert(GETSET(maxcompatibles, i), j);
            }
        }
    }

    if (debug > 0) {
        fprintf(stderr, "max compatibles\n");
        for (i = 0; i < colornum; i++) /* i is the color */
        {
            fprintf(stderr, "%d: ", i);
            for (j = 0; j < ns; j++) /* j is the state */
            {
                if (is_in_set(GETSET(maxcompatibles, i), j)) {
                    fprintf(stderr, "%s ", slab[j]);
                }
            }
            fprintf(stderr, "\n");
        }
    }
}
