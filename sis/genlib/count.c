
/* file @(#)count.c	1.1                      */
/* last modified on 5/29/91 at 12:35:21   */
#include "genlib_int.h"

static int f[40][20][20];


static int 
count_it(value, n)
int *value;
int n;
{
    int i, l, sum, smallest, *value1;

    if (n == 1) {
	sum = value[0];
    } else {
	smallest = value[n-1];
	sum = 0;
	for(i = 0; i < smallest; i++) {
	    value1 = ALLOC(int, n-1);
	    for(l = 0; l < n-1; l++) {
		value1[l] = value[l] - i;
	    }
	    sum += count_it(value1, n-1);
	    FREE(value1);
	}
    }
    return sum;
}

static int
hack_adjust(n, s, p)
int n, s, p;
{
    int sum, temp[100];

    sum = 0;

    switch (s) {
    case 0:
    case 1:
    case 2:
    case 3:
	break;
    case 4:
	/* 1 3, 2 2 --  subtract 1 2 */
	temp[0] = gl_number_of_gates(n-1, p, 2);
	temp[1] = gl_number_of_gates(n-1, p, 1);
	sum += count_it(temp, 2);
	break;

    case 5:
	/* 1 4, 2 3 -- subtract 1 3 */
	temp[0] = gl_number_of_gates(n-1, p, 3);
	temp[1] = gl_number_of_gates(n-1, p, 1);
	sum += count_it(temp, 2);

	/* 1 2 2, 1 1 3 -- subtract 1 1 2 */
	temp[0] = gl_number_of_gates(n-1, p, 2);
	temp[1] = gl_number_of_gates(n-1, p, 1);
	temp[2] = gl_number_of_gates(n-1, p, 1);
	sum += count_it(temp, 3);
	break;

    case 6:
	/* 1 5, 2 4, 3 3 -- subtract 1 4, 2 3, 1 3, add 1 3 */
	temp[0] = gl_number_of_gates(n-1, p, 4);
	temp[1] = gl_number_of_gates(n-1, p, 1);
	sum += count_it(temp, 2);
	temp[0] = gl_number_of_gates(n-1, p, 3);
	temp[1] = gl_number_of_gates(n-1, p, 2);
	sum += count_it(temp, 2);

	/* 2 2 2, 1 2 3, 1 1 4 -- subtract 1 2 2, 1 1 2, 1 1 3, add 1 1 2 */
	temp[0] = gl_number_of_gates(n-1, p, 2);
	temp[1] = gl_number_of_gates(n-1, p, 2);
	temp[2] = gl_number_of_gates(n-1, p, 1);
	sum += count_it(temp, 3);
	temp[0] = gl_number_of_gates(n-1, p, 3);
	temp[1] = gl_number_of_gates(n-1, p, 1);
	temp[2] = gl_number_of_gates(n-1, p, 1);
	sum += count_it(temp, 3);

	/* 1 1 2 2, 1 1 1 3 -- subtract 1 1 1 2 */
	temp[0] = gl_number_of_gates(n-1, p, 2);
	temp[1] = gl_number_of_gates(n-1, p, 1);
	temp[2] = gl_number_of_gates(n-1, p, 1);
	temp[3] = gl_number_of_gates(n-1, p, 1);
	sum += count_it(temp, 4);
	break;

    default:
	(void) fprintf(stderr, "s >= 7 not supported.\n");
	exit(1);
    }
    return sum;
}

int 
gl_number_of_gates(n, s, p)
{
    int i, k, sum, *value, *temp;
    partition_t *part;

    if (f[n][s][p] == 0) {
	if (n == 0) {
	    sum = 1;

	} else {
	    sum = 1;
	    part = gl_init_gen_partition(s); 
	    while (gl_next_partition(part, &value, &k)) {
		temp = ALLOC(int, k);
		for(i = 0; i < k; i++) {
		    temp[i] = gl_number_of_gates(n-1, p, value[i]);
		}
		sum += count_it(temp, k);
		FREE(temp);
	    }
	    gl_free_gen_partition(part);
	    sum -= hack_adjust(n, s, p);
	}
	f[n][s][p] = sum;
    }
    return f[n][s][p];
}
