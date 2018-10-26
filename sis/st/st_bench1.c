
#include <stdio.h>
#include "array.h"
#include "st.h"
#include "util.h"

#define MAX_WORD	1024

extern long random();

/* ARGSUSED */
main(argc, argv)
char *argv;
{
    array_t *words;
    st_table *table;
    char word[MAX_WORD], *tempi, *tempj;
    register int i, j;
    long time;
#ifdef TEST
    st_generator *gen;
    char *key;
#endif

    /* read the words */
    words = array_alloc(char *, 1000);
    while (gets(word) != NIL(char)) {
	array_insert_last(char *, words, util_strsav(word));
#ifdef TEST
	if (array_n(words) == 25) break;
#endif
    }

    /* scramble them */
    for(i = array_n(words)-1; i >= 1; i--) {
	j = random() % i;
	tempi = array_fetch(char *, words, i);
	tempj = array_fetch(char *, words, j);
	array_insert(char *, words, i, tempj);
	array_insert(char *, words, j, tempi);
    }

#ifdef TEST
    (void) printf("Initial data is\n");
    for(i = array_n(words)-1; i >= 0; i--) {
	(void) printf("%s\n", array_fetch(char *, words, i));
    }
#endif

    /* time putting them into an st tree */
    time = util_cpu_time();
    table = st_init_table(strcmp, st_strhash);
    for(i = array_n(words)-1; i >= 0; i--) {
	(void) st_insert(table, array_fetch(char *, words, i), NIL(char));
    }
    (void) printf("Elapsed time for insert of %d objects was %s\n",
	array_n(words), util_print_time(util_cpu_time() - time));

#ifdef TEST
    (void) printf("st data is\n");
    st_foreach_item(table, gen, &key, NIL(char *)) {
	(void) printf("%s\n", key);
    }
#endif
    return 0;
}
