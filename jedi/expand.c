
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */
#include "inc/jedi.h"

int expand_code();            /* forward declaration */

expand_code() {
    int i, j, c;

    for (i = 0; i < ne; i++) {
        for (j = 0; j < enumtypes[i].ns; j++) {
            c = enumtypes[i].symbols[j].code_ptr;
            expand_actual_code(i, c);
        }
    }
}

expand_actual_code(i, c
)
int i;        /* enumtype */
int c;        /* code */
{
FILE *fp;
int  k, nc, nb;
int  diff, j;
char line[BUFSIZE];
char p1, p2;

nc = enumtypes[i].nc;
nb = enumtypes[i].nb;

fp = my_fopen("temp1", "w");
(void)
fprintf(fp,
".i %d\n", nb);
(void)
fprintf(fp,
".o 1\n");
(void)
fprintf(fp,
"%s 1\n", enumtypes[i].codes[c].bit_vector);
for (
k = 0;
k<nc;
k++) {
if (!enumtypes[i].codes[k].assigned) {
(void)
fprintf(fp,
"%s -\n", enumtypes[i].codes[k].bit_vector);
}
}
(void)
fclose(fp);
(void) system("espresso < temp1 > temp2");
fp = my_fopen("temp2", "r");
(void)
fgets(line, BUFSIZE, fp);
(void)
fgets(line, BUFSIZE, fp);
(void)
fgets(line, BUFSIZE, fp);
(void)
fgets(line, BUFSIZE, fp);
for (
j = 0;
j<nb;
j++) {
enumtypes[i].codes[c].bit_vector[j] = line[j];
}
(void)
fclose(fp);
(void) system("rm -f temp1 temp2");

/* now remove unused codes */
for (
k = 0;
k<nc;
k++) {
if (!enumtypes[i].codes[k].assigned) {
diff = 0;
for (
j = 0;
j<nb;
j++) {
p1 = enumtypes[i].codes[k].bit_vector[j];
p2 = line[j];
if (p1 != p2 && p1 != '-' && p2 != '-') diff++;
}
if (diff == 0) enumtypes[i].codes[k].
assigned = TRUE;
}
}
}
