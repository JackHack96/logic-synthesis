
#include "sis.h"
#include "pld_int.h"

/*************************************************************************
   Assumes that the delay file looks like:

   4
   1 2.3
   2 3.0
   3 3.9
   4 5.4

   This means that total of 4 delay values are specified (first line).
   Each subsequent line gives the delay value for number of fanouts.
   e.g. delay for fanout of 3 is 3.9 (ns).

  The procedure returns an array of delay values. The first entry in the 
  array is 0.0 (corr. to fanout of 0).
***************************************************************************/
array_t *
act_read_delay(filename)
        char *filename;
{
    array_t *delay_values; /* stores at location i */
    FILE    *delayfile;
    int     num_spec;   /* number of fanout values specified in the file */
    int     num_fanout; /* signifies that the word following the current */
    /* word is delay for the num_fanout fanouts      */
    double  delay_num;   /* delay value for some specified fanout       */

    delayfile = fopen(filename, "r");
    fscanf(delayfile, "%d", &num_spec);
    if (num_spec < 1) fail("Error(act_read_delay): first word in the file should be > 0");
    delay_values = array_alloc(
    double, num_spec + 1);
    array_insert(
    double, delay_values, 0, 0.0);
    while (fscanf(delayfile, "%d", &num_fanout) != EOF) {
        if (num_fanout < 1) fail("Error(act_read_delay): number of fanouts should be > 0");
        (void) fscanf(delayfile, "%f", &delay_num);
        if (delay_num < 0.0) fail("Error(act_read_delay): number of fanouts should be > 0");
        array_insert(
        double, delay_values, num_fanout, delay_num);
    }
    (void) fclose(delayfile);
    if (ACT_DEBUG) print_delay_values(delay_values);
    return delay_values;
}

print_delay_values(delay_values)
array_t *delay_values;
{
int    i, nsize;
double delaynum;

nsize = array_n(delay_values);
(void) printf("printing actel delay info for number of fanouts...\n");
for (
i = 1;
i<nsize;
i++) {
delaynum = array_fetch(
double, delay_values, i);
(void) printf(" delay[%d] = %f\n", i, delaynum);
}
(void) printf("\n");
}
