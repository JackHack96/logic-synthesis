
/*******************************************************************************
*   Invokes the minimizer to get the input constraints                         *
*   This version uses the espresso-II version#2.0 multiple-valued minimizer    *
*******************************************************************************/

#include "inc/nova.h"

mini() {

    FILE       *fopen(), *fp;
    INPUTTABLE *new;
    char       string1[MAXSTRING];
    char       string2[MAXSTRING];
    char       string3[MAXSTRING];
    char       string4[MAXSTRING];
    char       command[MAXSTRING];
    int        i, pos;

    for (i = 0; i < statenum; i++) zeroutput[i] = 0;

    if ((fp = fopen(temp1, "w")) == NULL) {
        fprintf(stderr, "fopen: can't create %s (temp1)\n", temp1);
        exit(-1);
    }

    /* determine number and sizes of multiple-valued variables and write the command ".mv ...parameters..." */
    if (TYPEFR) fputs(".type fr\n", fp);
    if (ISYMB) {
        sprintf(string1, ".mv %d %d %d %d %d\n", 3, 0, inputnum, statenum, outputfield + statenum - 1);
        fputs(string1, fp);
    } else {
        sprintf(string1, ".mv %d %d %d %d\n", inputfield + 2, inputfield, statenum, outputfield + statenum - 1);
        fputs(string1, fp);
    }

    /* write the product terms in "temp1" */
    for (new = firstable; new != (INPUTTABLE *) 0; new = new->next) {
        if (ISYMB) {
            mvlab(string2, new->ilab, inputnum);
        } else {
            strcpy(string2, new->input);
        }
        mvlab(string3, new->plab, statenum);
        mvlab(string4, new->nlab, statenum);
        sprintf(string1, "%s | %s | %s %s\n", string2, string3, string4, new->output);
        fputs(string1, fp);

        /* if the proper output part is all zeroes updates the array zeroutput 
	   at the "next-state"-th position                                    */
        if (myindex(new->output, "1") < 0) {
            pos = myindex(string4, "1");
            if (pos >= 0) {
                (zeroutput[pos])++;
            }
        }

    }

    if (VERBOSE) {
        printf("\nzeroutput = ");
        for (i = 0; i < statenum; i++) printf("%d", zeroutput[i]);
        printf("\n");
    }

    fclose(fp);


    if (!ONEHOT) { /* hack to skip minimization with 1-hot encoding */
        if (VERBOSE) printf("\nRunning Espresso\n");
        sprintf(command, "espresso %s > %s", temp1, temp2);
        system(command);
        if (VERBOSE) printf("Espresso terminated\n");


        /* analyses the minimized file */
        analysis(temp2);
        onehot_card();
    } else {
        sprintf(command, "cp %s %s", temp1, temp2);
        system(command);
    }

}


/* converts a symbleme into a multiple-valued part of 0's and 1's */
mvlab(string, label, size
)
char string[MAXSTRING];
int  label;
int  size;

{

int k;

for (
k = 0;
k<size;
k++) {
if (label == -1 || label == k) {
string[k] = ONE;
} else {
string[k] = ZERO;
}
}
string[k] = '\0';

}


/*******************************************************************************
*                  Analyzes the minimized file                                 *
*                                                                              *
*******************************************************************************/

analysis(name)
char *name;

{
FILE       *fp, *fopen();
CONSTRAINT *scanner, *new, *newconstraint();
char       line[MAXLINE], *fgets();
char       binput[MAXSTRING];
char       bprstate[MAXSTRING];
char       bnxstate[MAXSTRING];
char       boutput[MAXSTRING];
int        i, cursor, linelength, cont;

if ((
fp = fopen(name, "r")
) == NULL) {
fprintf(stderr,
"analysis: can't open %s\n", name);
exit(-1);
} else {
while (
fgets(line,MAXLINE, fp) != (char *) 0) {

linelength = strlen(line);

/* skip blank lines , comment lines and command lines */
if ((linelength == 1) || (
myindex(line,
"#") >= 0) ||
(
myindex(line,
".") >= 0)) {
}

else {
if (ISYMB) {
for (
i = 1;
i<=
inputnum;
i++) {
binput[i-1] = line[i];
}
binput[inputnum] = '\0';
cursor = inputnum + 2;

for (
i = cursor;
i<cursor+
statenum;
i++) {
bprstate[i-cursor] = line[i];
}
bprstate[statenum] = '\0';
cursor = i + 1;

for (
i = cursor;
i<cursor+
statenum;
i++) {
bnxstate[i-cursor] = line[i];
}
bnxstate[statenum] = '\0';
cursor = i;

for (
i = cursor;
i<cursor+
outputfield;
i++) {
boutput[i-cursor] = line[i];
}
boutput[outputfield] = '\0';

}
else {
for (
i = 0;
i<inputfield;
i++) {
binput[i] = line[i];
}
binput[inputfield] = '\0';
cursor = i + 1;

for (
i = cursor;
i<cursor+
statenum;
i++) {
bprstate[i-cursor] = line[i];
}
bprstate[statenum] = '\0';
cursor = i + 1;

for (
i = cursor;
i<cursor+
statenum;
i++) {
bnxstate[i-cursor] = line[i];
}
bnxstate[statenum] = '\0';
cursor = i;

for (
i = cursor;
i<cursor+
outputfield;
i++) {
boutput[i-cursor] = line[i];
}
boutput[outputfield] = '\0';

}

/*printf("binput= %s\n", binput);
printf("bprstate= %s\n", bprstate);
printf("bnxstate= %s\n", bnxstate);
printf("boutput= %s\n", boutput);*/

/* deduce the constraints on the symbolic proper inputs */
if (ISYMB) {
cont = 0;
for (
i = 0;
i<inputnum;
i++) {

if (binput[i] == ONE) {
cont++;
} else {
if (binput[i] != ZERO) {
fprintf(stderr,
"Error reading Espresso output file in analysis\n");
fprintf(stderr,
"binput = %s\n", binput);
exit(-1);
}
}
}
/* we found a new meaningful constraint , let's insert it in "inputnet" */
if (cont > 1  &&  cont < inputnum) {
for (
scanner = inputnet;
scanner != (CONSTRAINT *) 0;
scanner = scanner->next
) {
if (
strcmp(binput, scanner
->relation) == 0 ) break;
}
if (scanner == (CONSTRAINT *) 0) {
new = newconstraint(binput, cont, 0);
new->
next     = inputnet;
inputnet = new;
} else {
scanner->weight++;
}
}
}

/* deduce the constraints on the states */
cont = 0;
for (
i = 0;
i<statenum;
i++) {

if (bprstate[i] == ONE) {
cont++;
} else {
if (bprstate[i] != ZERO) {
fprintf(stderr,
"Error reading Espresso output file in analysis\n");
fprintf(stderr,
"bprstate = %s\n", bprstate);
exit(-1);
}
}
}
/* find the next-states associated to the current constraint */
/* we found a new meaningful constraint , let's insert it in "statenet" */
if (cont > 1  &&  cont < statenum) {
for (
scanner = statenet;
scanner != (CONSTRAINT *) 0;
scanner = scanner->next
) {
if (
strcmp(bprstate, scanner
->relation) == 0 ) {
for (
i = 0;
i<statenum;
i++) {
if (bnxstate[i] == ONE) scanner->next_states[i] = ONE;
}
break;
}
}
if (scanner == (CONSTRAINT *) 0) {
new = newconstraint(bprstate, cont, 0);
new->
next     = statenet;
statenet = new;
strcpy(new
->next_states, bnxstate);
} else {
scanner->weight++;
}
}


}

}

/*if (ISYMB){
    shownet(&inputnet,"Inputnet","after analysis");
}

shownet(&statenet,"Statenet","after analysis");*/

fclose(fp);
}


}


onehot_card() {

    FILE *fopen(), *fpin;
    char line[MAXLINE], string[MAXLINE];

    if ((fpin = fopen(temp2, "r")) == NULL) {
        fprintf(stderr, "fopen: can't read file temp2\n");
        exit(-1);
    }
    while (fgets(line, MAXLINE, fpin) != NULL) {
        if (myindex(line, ".p") >= 0) {
            sscanf(line, "%s %d", string, &onehot_products);
            break;
        }
    }
    fclose(fpin);

}
