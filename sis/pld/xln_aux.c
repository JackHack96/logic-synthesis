
#include "sis.h"
#include "../include/pld_int.h"
#include <math.h>

/* Return the ceiling of log2(n). Use the fact that the floor of the binary
log of n is the highest-order bit of the integer representation */
int
intlog2(n)
        int n;
{
    int i, j, ans;

    if (n <= 0) {
        (void) fprintf(stderr, "Error: can't take the log of %d\n", n);
        fail("");
    }

    /* Through the loop, ans is = floor(log2(n)), i = 2^ans */
    for (ans = 0, i = 1, j = n; (j = j >> 1) != 0; ++ans, (i = i << 1));

    /* if n = i, n = 2^ans, i.e., floor(log2(n)) = ceil(log2(n)).  Otherwise
       bump up ans to make it the ceil */
    if (n > i) ++ans;
    return ans;
}


void xl_binary1(value, length, string)
        int value;
        int length;
        char *string;
{
    int  Position = 0;
    int  remainder;
    int  i;
    char string1[BUFSIZE];

    for (i = 0; i < BUFSIZE; i++) string1[i] = '0';
    while (value != 0) {
        remainder = value % 2;
        value     = (value - remainder) / 2;
        if (remainder == 0) string1[Position++] = '0';
        else string1[Position++] = '1';
    };
    if (Position > length) {
        (void) printf(" The length of string of value '%d' greater than %d\n", value, length);
        exit(1);
    }
    for (i = 0; i < length; i++)
        string[i] = string1[length - 1 - i];

    string[length] = '\0';

};
/*--------------------------------------------------------------
   Given some subset of fanins of node in array Y, return an 
   int array corresponding to the indices of the fanins.
---------------------------------------------------------------*/
int *
xln_array_to_indices(Y, node)
        array_t *Y;
        node_t *node;
{
    int    *lambda_indices;
    int    j, index;
    node_t *fanin;

    lambda_indices = ALLOC(
    int, array_n(Y));
    for (j = 0; j < array_n(Y); j++) {
        fanin = array_fetch(node_t * , Y, j);
        index = node_get_fanin_index(node, fanin);
        lambda_indices[j] = index;
    }
    return lambda_indices;
}
      
