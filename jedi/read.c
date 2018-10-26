
/*
 * Symbolic encoding program for compiling a symbolic
 * description into a binary representation.  Target
 * is multi-level logic synthesis
 *
 * History:
 *     3/20/90: recognize ANY or * on input
 *
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#include "jedi.h"

int read_input();                        /* forward declaration */
int read_kiss();                        /* forward declaration */
char *mk_var_name();                        /* forward declaration */

/*
 * global variable
 */
int dummy_count = 0;

read_input(fin, fout)
FILE *fin;
FILE *fout;
{
    Boolean iflag = FALSE;
    Boolean oflag = FALSE;
    Boolean ilbflag = FALSE;
    Boolean obflag = FALSE;
    Boolean itypeflag = FALSE;
    Boolean otypeflag = FALSE;
    Boolean found;
    char tline[BUFSIZE];
    char line[BUFSIZE];
    int i, j;
    int k, l;
    int c;
    int ns, nc, nb;

    ne = np = 0;
    enumtypes = ALLOC(struct Enumtype, 1);
    while (fgets(line, BUFSIZE, fin) != NULL) {
        (void) strcpy(tline, line);
        parse_line(tline);
        if (targc == 0) {
            /* skip empty lines */
        } else if (targv[0][0] == 0) {
            /* skip empty lines */
        } else if (targv[0][0] == '#' ) {
            /* pass on comments */
            (void) fprintf(fout, "%s", line);
        } else if (!strcmp(targv[0], ".i")) {
            if (iflag) {
                (void) fprintf(stderr,
                  "warning: .i has been declared more than once\n");
                continue;
            }

            iflag = TRUE;
            ni = atoi(targv[1]);
            inputs = ALLOC(struct Variable, ni);
            for (i=0; i<ni; i++) {
                inputs[i].name = mk_var_name();                        /* default */
                inputs[i].boolean_flag = FALSE;                        /* undefined */
                inputs[i].enumtype_ptr = 0;                        /* undefined */
                inputs[i].entries = ALLOC(struct Entry, 1);        /* undefined */
            }
        } else if (!strcmp(targv[0], ".o")) {
            if (oflag) {
                (void) fprintf(stderr,
                  "warning: .o has been declared more than once\n");
                continue;
            }

            oflag = TRUE;
            no = atoi(targv[1]);
            outputs = ALLOC(struct Variable, no);
            for (i=0; i<no; i++) {
                outputs[i].name = mk_var_name();                /* default */
                outputs[i].boolean_flag = FALSE;                /* undefined */
                outputs[i].enumtype_ptr = 0;                        /* undefined */
                outputs[i].entries = ALLOC(struct Entry, 1);        /* undefined */
            }
        } else if (!strcmp(targv[0], ".ilb")) {
            if (!iflag) {
                (void) fprintf(stderr,
                  "fatal error: .i has not been defined\n");
                exit(1);
            }

            if (ilbflag) {
                (void) fprintf(stderr,
                  "warning: .ilb has been declared more than once\n");
                continue;
            }

            ilbflag = TRUE;
            for (i=0; i<ni; i++) {
                FREE(inputs[i].name);                        /* free previous ptr */
                inputs[i].name = util_strsav(targv[i+1]);    /* assign new name */
            }
        } else if (!strcmp(targv[0], ".ob")) {
            if (!oflag) {
                (void) fprintf(stderr,
                  "fatal error: .o has not been defined\n");
                exit(1);
            }

            if (obflag) {
                (void) fprintf(stderr,
                  "warning: .ob has been declared more than once\n");
                continue;
            }

            obflag = TRUE;
            for (i=0; i<no; i++) {
                FREE(outputs[i].name);                        /* free previous ptr */
                outputs[i].name = util_strsav(targv[i+1]);    /* assign new name */
            }
        } else if (!strcmp(targv[0], ".itype")) {
            if (!iflag) {
                (void) fprintf(stderr,
                  "fatal error: .i has not been defined\n");
                exit(1);
            }

            if (itypeflag) {
                (void) fprintf(stderr,
                  "warning: .itype has been declared more than once\n");
                continue;
            }

            itypeflag = TRUE;
            for (i=0; i<ni; i++) {
                if (!strcmp(targv[i+1], "Boolean")) {
                    inputs[i].boolean_flag = TRUE;
                } else {
                    found = FALSE;
                    for (j=0; j<ne; j++) {
                        if (!strcmp(targv[i+1], enumtypes[j].name)) {
                            inputs[i].enumtype_ptr = j;
                            found = TRUE;
                            break;
                        }
                    }

                    /*
                     * print an error if no such type defined yet
                     */
                    if (!found) {
                        (void) fprintf(stderr,
                          "fatal error: enum type (%s) has not been defined\n",
                          targv[i+1]);
                        exit(1);
                    }
                }
            }
        } else if (!strcmp(targv[0], ".otype")) {
            if (!oflag) {
                (void) fprintf(stderr,
                  "fatal error: .o has not been defined\n");
                exit(1);
            }

            if (otypeflag) {
                (void) fprintf(stderr,
                  "warning: .otype has been declared more than once\n");
                continue;
            }

            otypeflag = TRUE;
            for (i=0; i<no; i++) {
                if (!strcmp(targv[i+1], "Boolean")) {
                    outputs[i].boolean_flag = TRUE;
                } else {
                    found = FALSE;
                    for (j=0; j<ne; j++) {
                        if (!strcmp(targv[i+1], enumtypes[j].name)) {
                            outputs[i].enumtype_ptr = j;
                            found = TRUE;
                            break;
                        }
                    }

                    /*
                     * print an error if no such type defined yet
                     */
                    if (!found) {
                        (void) fprintf(stderr,
                          "fatal error: enum type (%s) has not been defined\n",
                          targv[i+1]);
                        exit(1);
                    }
                }
            }
        } else if (!strcmp(targv[0], ".enum")) {
            ne++;
            k = ne-1;
            enumtypes = REALLOC(struct Enumtype, enumtypes, ne);
            enumtypes[k].name = util_strsav(targv[1]);
            ns = enumtypes[k].ns = atoi(targv[2]);
            nb = enumtypes[k].nb = atoi(targv[3]);
            if (nb > MAXSPACE) {
                (void) fprintf(stderr, 
                    "panic: type (%s) code length %d exceeds limit of %d.\n",
                    targv[1], nb, MAXSPACE);
            }
            nc = enumtypes[k].nc = pow_of_2(nb);

            enumtypes[k].input_flag = FALSE;
            enumtypes[k].output_flag = FALSE;

            enumtypes[k].dont_care = ALLOC(char, nb + 1);
            for (i=0; i<nb; i++) {
                enumtypes[k].dont_care[i] = '-';
            }
            enumtypes[k].dont_care[nb] = '\0';

            enumtypes[k].codes = ALLOC(struct Code, nc);
            for (i=0; i<nc; i++) {
                enumtypes[k].codes[i].assigned = FALSE;                /* undefined */
                enumtypes[k].codes[i].bit_vector = 
                  int_to_binary(i, nb);
                enumtypes[k].codes[i].symbol_ptr = 0;                /* undefined */
            }

            enumtypes[k].symbols = ALLOC(struct Symbol, ns);
            for (i=0; i<ns; i++) {
                enumtypes[k].symbols[i].token = util_strsav(targv[i+4]);
                enumtypes[k].symbols[i].code_ptr = i;
                /*
                 * assign codes
                 */
                enumtypes[k].codes[i].assigned = TRUE;
                enumtypes[k].codes[i].symbol_ptr = i;
            }

            /*
             * initialize connectivity matrix to be
             * computed when the weights are computed
             */
            enumtypes[k].links = ALLOC(struct Link *, ns);
            for (i=0; i<ns; i++) {
                enumtypes[k].links[i] = 
                  ALLOC(struct Link, ns);
                for (j=0; j<ns; j++) {
                    enumtypes[k].links[i][j].weight = 0;        /* undefined */
                }
            }

            /*
             * compute code distances
             */
            enumtypes[k].distances = ALLOC(int *, nc);
            for (i=0; i<nc; i++) {
                enumtypes[k].distances[i] = ALLOC(int, nc);
            }
            for (i=0; i<nc; i++) {
                for (j=0; j<nc; j++) {
                    enumtypes[k].distances[i][j] = 
                      distance(enumtypes[k].codes[i].bit_vector,
                        enumtypes[k].codes[j].bit_vector, nb);
                }
            }
        } else if (!strcmp(targv[0], ".e")) {
           /* ignore end command */
        } else if (targv[0][0] == '.') {
            /*
             * pass on unrecognized commands
             */
            (void) fprintf(fout, "%s", line);
        } else if (targc == (ni + no)) {
            if (!iflag || !oflag || !itypeflag || !otypeflag) {
                (void) fprintf(stderr,
                  "fatal error: either .i or .o has not been defined\n");
                exit(1);
            }

            np++;
            k = np-1;
            /*
             * parse input plane
             */
            for (i=0; i<ni; i++) {
                inputs[i].entries = 
                  REALLOC(struct Entry, inputs[i].entries, np);
                inputs[i].entries[k].token = util_strsav(targv[i]);
                /*
                 * if not boolean type
                 */
                if (!inputs[i].boolean_flag) {
                    l = inputs[i].entries[k].enumtype_ptr = 
                      inputs[i].enumtype_ptr;
                    for (j=0; j<enumtypes[l].ns; j++) {
                        if (!strcmp(inputs[i].entries[k].token,
                         enumtypes[l].symbols[j].token)) {
                            inputs[i].entries[k].symbol_ptr = j;
                            break;
                        }
                    }
                }
            }

            /*
             * parse output plane
             */
            for (i=0; i<no; i++) {
                outputs[i].entries = 
                  REALLOC(struct Entry, outputs[i].entries, np);
                outputs[i].entries[k].token = util_strsav(targv[i+ni]);
                /*
                 * if not boolean type
                 */
                if (!outputs[i].boolean_flag) {
                    l = outputs[i].entries[k].enumtype_ptr = 
                      outputs[i].enumtype_ptr;
                    for (j=0; j<enumtypes[l].ns; j++) {
                        if (!strcmp(outputs[i].entries[k].token,
                         enumtypes[l].symbols[j].token)) {
                            outputs[i].entries[k].symbol_ptr = j;
                            break;
                        }
                    }
                }
            }
        } else {
            (void) fprintf(stderr, "warning: ignored questionable input ...\n");
            (void) fprintf(stderr, "----> %s", line);
        }
    }

    /*
     * check to make sure a truth table was read in
     */
    if (!iflag || !oflag || !itypeflag || !otypeflag) {
        (void) fprintf(stderr, "jedi: unable to find input truth table\n");
        exit(1);
    }

    /*
     * determine total inputs and total outputs
     */
    tni = 0;
    for (i=0; i<ni; i++) {
        if (inputs[i].boolean_flag) {
            tni++;
        } else {
            c = inputs[i].enumtype_ptr;
            tni += enumtypes[c].nb;
        }
    }
    tno = 0;
    for (i=0; i<no; i++) {
        if (outputs[i].boolean_flag) {
            tno++;
        } else {
            c = outputs[i].enumtype_ptr;
            tno += enumtypes[c].nb;
        }
    }
} /* end of read_input */


read_kiss(fin, fout)
FILE *fin;
FILE *fout;
{
    Boolean iflag = FALSE;
    Boolean oflag = FALSE;
    Boolean sflag = FALSE;
    Boolean ilbflag = FALSE;
    Boolean obflag = FALSE;
    Boolean found;
    char tline[BUFSIZE];
    char line[BUFSIZE];
    int i, j;
    int k, l, m;
    int nc, nb;
    int t;

    np = 0;
    reset_state = NIL(char);
    while (fgets(line, BUFSIZE, fin) != NULL) {
        (void) strcpy(tline, line);
        parse_line(tline);
        if (targc == 0) {
            /* skip */
        } else if (targv[0][0] == 0) {
            /* skip */
        } else if (targv[0][0] == '#') {
            /*
             * pass on comments
             */
            (void) fprintf(fout, "%s", line);
        } else if (!strcmp(targv[0], ".i")) {
            if (iflag) {
                (void) fprintf(stderr,
                  "warning: .i has been declared more than once\n");
                continue;
            }

            iflag = TRUE;
            ni = atoi(targv[1]) + 1;
            inputs = ALLOC(struct Variable, ni);
            for (i=0; i<ni; i++) {
                inputs[i].name = mk_var_name();                        /* default */
                inputs[i].boolean_flag = TRUE;
                inputs[i].enumtype_ptr = 0;
                inputs[i].entries = ALLOC(struct Entry, 1);        /* undefined */
            }
            inputs[ni-1].boolean_flag = FALSE;                /* presentState input */
        } else if (!strcmp(targv[0], ".o")) {
            if (oflag) {
                (void) fprintf(stderr,
                  "warning: .o has been declared more than once\n");
                continue;
            }

            oflag = TRUE;
            no = atoi(targv[1]) + 1;
            outputs = ALLOC(struct Variable, no);
            for (i=0; i<no; i++) {
                outputs[i].name = mk_var_name();                /* default */
                outputs[i].boolean_flag = TRUE;
                outputs[i].enumtype_ptr = 0;
                outputs[i].entries = ALLOC(struct Entry, 1);        /* undefined */
            }
            outputs[0].boolean_flag = FALSE;                /* nextState output */
        } else if (!strcmp(targv[0], ".ilb")) {
            if (!iflag) {
                (void) fprintf(stderr,
                  "fatal error: .i has not been defined\n");
                exit(1);
            }

            if (ilbflag) {
                (void) fprintf(stderr,
                  "warning: .ilb has been declared more than once\n");
                continue;
            }

            ilbflag = TRUE;
            for (i=0; i<ni; i++) {
                FREE(inputs[i].name);                        /* free previous ptr */
                inputs[i].name = util_strsav(targv[i+1]);        /* assign new name */
            }
        } else if (!strcmp(targv[0], ".ob")) {
            if (!oflag) {
                (void) fprintf(stderr,
                  "fatal error: .o has not been defined\n");
                exit(1);
            }

            if (obflag) {
                (void) fprintf(stderr,
                  "warning: .ob has been declared more than once\n");
                continue;
            }

            obflag = TRUE;
            for (i=0; i<no; i++) {
                FREE(outputs[i].name);                        /* free previous ptr */
                outputs[i].name = util_strsav(targv[i+1]);        /* assign new name */
            }
        } else if (!strcmp(targv[0], ".s")) {
            if (sflag) {
                (void) fprintf(stderr,
                  "warning: .s has been declared more than once\n");
                continue;
            }

            sflag = TRUE;
            ne = 1;
            enumtypes = ALLOC(struct Enumtype, 1);
            enumtypes[0].name = "States";
            enumtypes[0].ns = 0;                        /* undefined */
            t = atoi(targv[1]);
	    if (t <= 1) {
		t = 1;
	    } else {
		t = (int) ceil(log((double) t)/ log(2.0));
	    }

            if (bitsFlag) {
                if (code_length < t) {
                    (void) fprintf(stderr, 
                      "warning: code length %d is not long enough: ",
                      code_length);
                    (void) fprintf(stderr, "using default length %d.\n", t);
                    nb = enumtypes[0].nb = t;
                } else {
                    nb = enumtypes[0].nb = code_length;
                }
            } else {
                nb = enumtypes[0].nb = t;
            }

            if (nb > MAXSPACE) {
                (void) fprintf(stderr, 
                    "panic: type (%s) code length %d exceeds limit of %d.\n",
                    "States", nb, MAXSPACE);
            }

            nc = enumtypes[0].nc = pow_of_2(nb);
            enumtypes[0].input_flag = FALSE;
            enumtypes[0].output_flag = FALSE;

            enumtypes[0].dont_care = ALLOC(char, nb + 1);
            for (i=0; i<nb; i++) {
                enumtypes[0].dont_care[i] = '-';
            }
            enumtypes[0].dont_care[nb] = '\0';
        
            enumtypes[0].codes = ALLOC(struct Code, nc);
            for (i=0; i<nc; i++) {
                enumtypes[0].codes[i].assigned = FALSE;                /* undefined */
                enumtypes[0].codes[i].bit_vector = 
                  int_to_binary(i, nb);
                enumtypes[0].codes[i].symbol_ptr = 0;                /* undefined */
            }

            enumtypes[0].symbols = ALLOC(struct Symbol, 1);        /* undefined */
            enumtypes[0].links = ALLOC(struct Link *, 1);        /* undefined */

            /*
             * compute code distances
             */
            enumtypes[0].distances = ALLOC(int *, nc);
            for (i=0; i<nc; i++) {
                enumtypes[0].distances[i] = ALLOC(int, nc);
            }
            for (i=0; i<nc; i++) {
                for (j=0; j<nc; j++) {
                    enumtypes[0].distances[i][j] =
                      distance(enumtypes[0].codes[i].bit_vector,
                        enumtypes[0].codes[j].bit_vector, nb);
                }
            }
        } else if (!strcmp(targv[0], ".r")) {
           reset_state = util_strsav(targv[1]);
        } else if (!strcmp(targv[0], ".e")) {
           /* ignore end command */
        } else if (targv[0][0] == '.') {
            /* skip */
        } else if (targc == 4) {
            if (!iflag || !oflag || !sflag) {
                (void) fprintf(stderr,
                  "fatal error: either .i, .o, or .s has not been defined\n");
                exit(1);
            }

            np++;
            k = np-1;
            l = ni-1;

            /*
             * parse primary inputs
             */
            for (i=0; i<l; i++) {
                inputs[i].entries = 
                  REALLOC(struct Entry, inputs[i].entries, np);
                inputs[i].entries[k].token = ALLOC(char, 2);
                inputs[i].entries[k].token[0] = targv[0][i];
                inputs[i].entries[k].token[1] = '\0';
                inputs[i].entries[k].enumtype_ptr = 0;
            }

            /*
             * parse presentState
             */
            inputs[l].entries = 
              REALLOC(struct Entry, inputs[l].entries, np);
            inputs[l].entries[k].token = util_strsav(targv[1]);
            inputs[l].entries[k].enumtype_ptr = 0;

            if (!check_dont_care(inputs[l].entries[k].token)) {
                found = FALSE;
                for (j=0; j<enumtypes[0].ns; j++) {
                    if (!strcmp(inputs[l].entries[k].token,
                     enumtypes[0].symbols[j].token)) {
                        inputs[l].entries[k].symbol_ptr = j;
                        found = TRUE;
                        break;
                    }
                }
                if (!found) {
                    enumtypes[0].ns++;
                    enumtypes[0].symbols = REALLOC(struct Symbol, 
		      enumtypes[0].symbols, enumtypes[0].ns);
                    m = enumtypes[0].ns-1;

                    inputs[l].entries[k].symbol_ptr = m;
                    enumtypes[0].symbols[m].token =
                      util_strsav(inputs[l].entries[k].token);
                    enumtypes[0].symbols[m].code_ptr = m;

                    /*
                     * assign codes
                     */
                    enumtypes[0].codes[m].assigned = TRUE;
                    enumtypes[0].codes[m].symbol_ptr = m;
                }
            }

            /*
             * parse primary outputs 
             */
            for (i=0; i<no-1; i++) {
                outputs[i+1].entries = 
                  REALLOC(struct Entry, outputs[i+1].entries, np);
                outputs[i+1].entries[k].token = ALLOC(char, 2);
                outputs[i+1].entries[k].token[0] = targv[3][i];
                outputs[i+1].entries[k].token[1] = '\0';
                outputs[i+1].entries[k].enumtype_ptr = 0;
            }

            /*
             * parse nextState
             */
            outputs[0].entries = 
              REALLOC(struct Entry, outputs[0].entries, np);
            outputs[0].entries[k].token = util_strsav(targv[2]);
            outputs[0].entries[k].enumtype_ptr = 0;

            if (!check_dont_care(outputs[0].entries[k].token)) {
		found = FALSE;
		for (j=0; j<enumtypes[0].ns; j++) {
		    if (!strcmp(outputs[0].entries[k].token,
		     enumtypes[0].symbols[j].token)) {
			outputs[0].entries[k].symbol_ptr = j;
			found = TRUE;
			break;
		    }
		}
		if (!found) {
		    enumtypes[0].ns++;
		    enumtypes[0].symbols = REALLOC(struct Symbol, 
		      enumtypes[0].symbols, enumtypes[0].ns);
		    m = enumtypes[0].ns-1;

		    outputs[0].entries[k].symbol_ptr = m;
		    enumtypes[0].symbols[m].token =
		      util_strsav(outputs[0].entries[k].token);
		    enumtypes[0].symbols[m].code_ptr = m;

		    /*
		     * assign codes
		     */
		    enumtypes[0].codes[m].assigned = TRUE;
		    enumtypes[0].codes[m].symbol_ptr = m;
		}
	    }
        } else {
            (void) fprintf(stderr, "warning: ignored incorrect input ...\n");
            (void) fprintf(stderr, "----> %s", line);
        }
    }

    /*
     * check to make sure a truth table was read in
     */
    if (!iflag || !oflag || !sflag) {
        (void) fprintf(stderr, "jedi: unable to find input truth table\n");
        exit(1);
    }

    /*
     * initialize connectivity matrix to be
     * computed when the weights are computed
     */
    enumtypes[0].links = ALLOC(struct Link *, enumtypes[0].ns);
    for (i=0; i<enumtypes[0].ns; i++) {
        enumtypes[0].links[i] = 
          ALLOC(struct Link, enumtypes[0].ns);
    }

    for (i=0; i<enumtypes[0].ns; i++) {
        for (j=0; j<enumtypes[0].ns; j++) {
            enumtypes[0].links[i][j].weight = 0;        /* undefined */
        }
    }

    /*
     * set reset state
     */
    if (reset_state == NIL(char)) {
        reset_state = util_strsav(enumtypes[0].symbols[0].token);
    }

    /*
     * determine total inputs and total outputs
     */
    tni = ni + enumtypes[0].nb - 1;
    tno = no + enumtypes[0].nb - 1;
} /* end of read_kiss */


char *
mk_var_name()
{
    char buffer[BUFSIZE];

    (void) sprintf(buffer, "v.%d", dummy_count++);
    return util_strsav(buffer);
} /* end of mk_var_name */
