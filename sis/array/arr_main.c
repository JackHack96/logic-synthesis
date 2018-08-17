#include <stdio.h>
#include "../include/util.h"
#include "../include/array.h"

#define IN_ORDER    1
#define REVERSE_ORDER    2
#define RANDOM        3
#define RANDOM_RANGE    4

//extern long random();

//extern void srandom();

static int count;            /* global: count # compares */

static int compare(char **a, char **b) {
    count++;
    return *(int *) a - *(int *) b;
}

static void print(char *s, array_t *a) {
    int i;

    (void) printf("%s\n", s);
    for (i = 0; i < array_n(a); i++) {
        (void) printf(" %d", array_fetch(int, a, i));
    }
    (void) printf("\n");
}

void usage(char *prog) {
    (void) fprintf(stderr, "%s: check out the array package\n", prog);
    (void) fprintf(stderr, "\t-o\tinitial data in order\n");
    (void) fprintf(stderr, "\t-r\tinitial data in reverse order\n");
    (void) fprintf(stderr, "\t-n #\tnumber of values to sort\n");
    (void) fprintf(stderr, "\t-b #\tmaximum value for the random values\n");
    exit(2);
}

int main(int argc, char *argv[]) {
    long    time;
    array_t *a, *a1;
    int     *b, i, n, type, c, range, value;

    type      = RANDOM_RANGE;
    range     = 10;
    n         = 15;
    while ((c = util_getopt(argc, argv, "b:orn:")) != EOF) {
        switch (c) {
            case 'o': type = IN_ORDER;
                break;
            case 'r': type = REVERSE_ORDER;
                break;
            case 'n': n = atoi(util_optarg);
                break;
            case 'b': type = RANDOM_RANGE;
                range      = atoi(util_optarg);
                break;
            default: usage(argv[0]);
                break;
        }
    }

    if (util_optind != argc) {
        usage(argv[0]);
    }

    /*
     *  create the input-data
     */
    srandom(1);
    time   = util_cpu_time();
    a      = array_alloc(int, 0);
    for (i = 0; i < n; i++) {
        if (type == IN_ORDER) {
            value = i;
        } else if (type == REVERSE_ORDER) {
            value = n - i;
        } else if (type == RANDOM_RANGE) {
            value = random() % range;
        } else {
            value = random();
        }
        array_insert(int, a, i, value);
        if (n < 20) (void) printf(" %d", value);
    }
    (void) printf("\nfill: %d objects, time was %s\n",
                  array_n(a), util_print_time(util_cpu_time() - time));
    if (n < 20) print("unsorted list", a);


    /* 
     *  time a fill using normal subscripted arrays 
     */
    srandom(1);
    time   = util_cpu_time();
    b      = ALLOC(int, n);
    for (i = 0; i < n; i++) {
        if (type == IN_ORDER) {
            value = i;
        } else if (type == REVERSE_ORDER) {
            value = n - i;
        } else if (type == RANDOM_RANGE) {
            value = random() % range;
        } else {
            value = random();
        }
        b[i] = value;
    }
    (void) printf("fill (fast): %d objects, time was %s\n",
                  array_n(a), util_print_time(util_cpu_time() - time));

    /*
     *  now a quick check of append() and insert_last()
     */
    a1 = array_alloc(int, 5);
    array_insert_last(int, a1, 2);
    array_insert_last(int, a1, 1);
    array_insert_last(int, a1, 0);
    if (n < 20) print("unsorted list1", a1);
    array_append(a, a1);
    (void) printf("after join: %d objects, time was %s\n",
                  array_n(a), util_print_time(util_cpu_time() - time));
    if (n < 20) print("unsorted list (after join)", a);
    array_free(a1);



    /*
     *  Test the sorter
     */
    count = 0;
    time  = util_cpu_time();
    array_sort(a, compare);
    (void) printf("sort: %d objects, %d compares, time was %s\n",
                  array_n(a), count, util_print_time(util_cpu_time() - time));
    if (n < 20) print("sorted list", a);


    /*
     *  Try the uniq
     */
    count = 0;
    time  = util_cpu_time();
    array_uniq(a, compare, (void (*)()) 0);
    (void) printf("uniq: %d objects, %d compares, time was %s\n",
                  array_n(a), count, util_print_time(util_cpu_time() - time));
    if (n < 20) print("uniq list", a);

    return 0;
}
