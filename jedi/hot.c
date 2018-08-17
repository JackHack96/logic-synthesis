
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

int write_one_hot();            /* forward declaration */

write_one_hot(fptr)
FILE *fptr;
{
int i, j, c;

/* print header */
(void)
fprintf(fptr,
".model jedi_output\n");

/* print state table */
(void)
fprintf(fptr,
".start_kiss\n");
(void)
fprintf(fptr,
".i %d\n", ni-1);
(void)
fprintf(fptr,
".o %d\n", no-1);
(void)
fprintf(fptr,
".s %d\n", enumtypes[0].ns);
(void)
fprintf(fptr,
".p %d\n", np);
(void)
fprintf(fptr,
".r %s\n", reset_state);
for (
i = 0;
i<np;
i++) {
for (
j = 0;
j<ni-1; j++) {
(void)
fprintf(fptr,
"%s", inputs[j].entries[i].token);
}
(void)
fprintf(fptr,
" ");
(void)
fprintf(fptr,
"%s", inputs[ni-1].entries[i].token);
(void)
fprintf(fptr,
" ");

(void)
fprintf(fptr,
"%s", outputs[0].entries[i].token);
(void)
fprintf(fptr,
" ");
for (
j = 1;
j<no;
j++) {
(void)
fprintf(fptr,
"%s", outputs[j].entries[i].token);
}
(void)
fprintf(fptr,
"\n");
}
(void)
fprintf(fptr,
".end_kiss\n");

/* print encodings */
for (
i = 0;
i<enumtypes[0].
ns;
i++) {
(void)
fprintf(fptr,
".code %s ", enumtypes[0].symbols[i].token);
for (
j = 0;
j<enumtypes[0].
ns;
j++) {
if (i == j) {
(void)
fprintf(fptr,
"1");
} else {
(void)
fprintf(fptr,
"0");
}
}
(void)
fprintf(fptr,
"\n");
}

(void)
fprintf(fptr,
".end\n");
} /* end of write_one_hot */
