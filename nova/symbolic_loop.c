
#include "inc/nova.h"

/*******************************************************************************
*                        SYMBOLIC MINIMIZATION LOOP                            *
*                                                                              *
*                                                                              *
*   External files used : <> temp6 is the input to espresso (ON1i , OFFi)      *
*                         <> temp7 is the output from espresso (Mi)            *
*                         <> temp81 contains Mi with simplified output part    *
*                         <> temp82 contains ON0j with simplified output part  *
*                            (covers of temp81 and temp82 are intersected)     *
*                         <> temp83 contains the intersection of the covers    *
*                            contained in temp81 and temp82                    *
*                         <> temp9 contains the final minimized cover :        *
*                                  (P = union Mi for i=1,...,q)                *
*                         <> temp10 contains the final irredundant cover       *
*******************************************************************************/


symbolic_loop() {


    FILE           *fopen(), *fpcons, *fp9;
    INPUTTABLE     *new;
    OUTSYMBOL      *temptr, *onjptr, *oniptr, *select_oni();
    ORDER_RELATION *curptr;
    /*ORDER_PATH *cursor;*/
    char           mvline[MAXSTRING];
    char           command[MAXSTRING];
    int            i, j, found;
    int            oni_card, onk, card_gain;
    int relation_is_new();
    int count_prods();
    unsigned       status;


    /* allocates and initializes array_cover */
    array_alloc2();

    /* fills order_array with zeroes */
    for (i = 0; i < statenum; i++) {
        for (j = 0; j < statenum; j++) {
            order_array[i][j] = ZERO;
        }
    }

    /* determines the correct .mv command line */
    if ((fp9 = fopen(temp9, "w")) == NULL) {
        fprintf(stderr, "fopen: can't create temp9\n");
        exit(-1);
    } else {
        if (ISYMB) {
            sprintf(mvline, ".mv %d %d %d %d %d\n", 3, 0, inputnum, statenum, statenum + outputfield - 1);
            fputs(mvline, fp9);
        } else {
            sprintf(mvline, ".mv %d %d %d %d\n", inputfield + 2, inputfield, statenum, statenum + outputfield - 1);
            fputs(mvline, fp9);
        }
    }
    fclose(fp9);


    /* scans the next-state column of the input-table to build
   "outsymbol_list" */
    oni_card = 0;
    for (new = firstable; new != (INPUTTABLE *) 0; new = new->next) {

        /* if the current next state is a don't care state , do nothing
           ( a don't care state may be denoted by a "-" in the
           first position of the string or by the string "ANY" )        */
        if (new->nstate[0] == DASH || myindex(new->nstate, ANYSTATE) >= 0) { ;

        } else {

            /* adds the current next-state symbol to outsymbol_list ,
       unless it is already there */
            found       = 0;
            for (temptr = outsymbol_list; temptr != (OUTSYMBOL *) 0;
                 temptr = temptr->next) {
                if (strcmp(temptr->element, new->nstate) == 0) {
                    found = 1;
                    temptr->card++;
                    break;
                }
            }
            if (found == 0) {
                new_outsymbol(new->nstate, new->nlab);
                oni_card++;
            }

        }
    }

    if (VERBOSE) {
        printf("\nOUTSYMBOL_LIST\n");
        /*printf("Card = %d\n", oni_card);*/
        for (temptr = outsymbol_list; temptr != (OUTSYMBOL *) 0;
             temptr = temptr->next) {
            printf("%s   ", temptr->element);
            printf("%d  ", temptr->nlab);
            printf("card = %d\n", temptr->card);
        }
    }



    /*           ^^^^^^^^^  SYMBOLIC MINIMIZATION LOOP    ^^^^^^^^^        */

    for (onk = 0; onk < oni_card; onk++) {

        /* procedure "select" selects the sets ONi according to a heuristic
       criterion                                                         */
        oniptr = select_oni();
        if (VERBOSE) printf("  Select_oni returned %s\n", oniptr->element);
        select_array[oniptr->nlab] = NOTUSED;


        /* dfs (depth-first search) builds the set of indices
       J = { j : such that there is a path from vi to vj in G(V,A) }    */
        for (i = 0; i < statenum; i++) {
            reached[i] = 0;
        }
        /*printf("dfs called by node = %d", oniptr->nlab);*/
        dfs(oniptr->nlab, oniptr->nlab);

        /*printf("\nPath graph\n");
        for ( i = 0; i < statenum; i++) {
        printf("row = %d: ", i);
            for ( cursor = path_graph[i] ; cursor != (ORDER_PATH *) 0;
                                        cursor = cursor->next  ) {
            printf("%d ", cursor->dest);
            }
        printf("\n");
        }*/


        /*                    Mi = minimize(ON0i,DCi,OFFi)
               out_mini invokes Espresso to minimize (ON0i,DCi,OFFi) where
           OFFi =  Union ( Union(ON0j) ) and j varies in J defined above  .
           At each iteration of the symbolic minimization loop Mi is obtained
           by minimizing ON0i, using a routine that performs multiple-valued-
               input binary-valued output minimization. We invoke the minimization
           routine with the triple (ON0i,DCi,OFFi), where the corresponding
           don't care set DCi includes by construction all the sets ON0j
           for which no path exists in G(V,A) from vi to vj .
           As a result, minimization may be very efficient in reducing the
           cardinality of ON0i because of the advantageous don't care set    */
        card_gain = out_mini(oniptr);
        gain_array[oniptr->nlab] = card_gain;
        /*printf("out_mini = %d\n", card_gain);*/

        /* Comment if card_gain > 0 */
        /* if Mi intersects ONj , (vj,vi) is added to the edge set of the
       graph: A = A U { (vj,vi) such that (Mi intersection ON0j) != 0 }
       (Mi is the minimized cover of ON0i written by espresso in temp6) -
       the intersection is detected by a system call to a modified version
       of espresso called test :
             system("ESPRESSO/test -Dinter Mi On0j")
             note : status = 1  covers are not disjoint
                status = 2  covers are disjoint                          */
        if (OUT_ALL || (!OUT_ALL && card_gain > 0)) {
            for (onjptr = outsymbol_list; onjptr != (OUTSYMBOL *) 0;
                 onjptr = onjptr->next) {
                /*printf("\nonjptr->nlab  = %d  ", onjptr->nlab);
                printf("oniptr->nlab  = %d\n", oniptr->nlab);*/
                if (onjptr->nlab != oniptr->nlab) {

                    /* oncover_inter1 writes in temp81 the cover Mi after
		       substituting string "1" instead of the next-state string  */
                    oncover_inter1();
                    /*printf("\nFile temp81\n");
                    system("cat temp81");*/

                    /* oncover_inter2 writes in temp82 the cover ON0j (j =
		       onjptr->nlab) after substituting string "1" instead of  
		       the next-state string                                     */
                    oncover_inter2(onjptr);
                    /*printf("File temp82\n");
                    system("cat temp82");*/

                    sprintf(command,
                            "espresso -Dintersect %s %s > %s", temp81, temp82, temp83);
                    system(command);
                    status = count_prods(temp83);
                    /*printf("exit code was = %d\n", status);*/
                    if (status == 1) {
                        if (relation_is_new(onjptr->nlab, oniptr->nlab) == 0) {
                            new_order_relation(onjptr->nlab, oniptr->nlab);
                            order_array[oniptr->nlab][onjptr->nlab] = ONE;
                        }
                    }

                }
            }
        }

        if (VERBOSE) {
            printf("\nPartial order graph\n");
            for (i = 0; i < statenum; i++) {
                printf("%d: ", i);
                for (curptr = order_graph[i]; curptr != (ORDER_RELATION *) 0;
                     curptr = curptr->next) {
                    printf("%d ", curptr->index);
                }
                printf("\n");
            }
        }

    }
    if (VERBOSE) {
        printf("\nOrder_array\n");
        for (i = 0; i < statenum; i++) {
            for (j = 0; j < statenum; j++) {
                printf("%c", order_array[i][j]);
            }
            printf(" wgt:%d", gain_array[i]);
            printf("\n");
        }
        printf("\n");
    }
    /*show_implicant();*/

    if ((fpcons = fopen(constraints, "w")) == NULL) {
        fprintf(stderr, "fopen: can't create constraints\n");
        exit(-1);
    }
    for (i      = 0; i < statenum; i++) {
        fprintf(fpcons, "%d: ", i);
        for (curptr = order_graph[i]; curptr != (ORDER_RELATION *) 0;
             curptr = curptr->next) {
            fprintf(fpcons, "%d ", curptr->index);
        }
        fprintf(fpcons, "\n");
    }
    fclose(fpcons);

    /* runs the final cover through Espresso to eliminate redundant terms */
    sprintf(command, "espresso %s > %s", temp9, temp10);
    system(command);
    inputnet = (CONSTRAINT *) 0;
    statenet = (CONSTRAINT *) 0;
    analysis(temp10);

}


/*******************************************************************************
*   Sorts the sets ONi according to a heuristic criterion -                    *
*   Heuristic implemented : to sort the sets ONi in descending order of        *
*   cardinality , so that the largest sets will benefit from larger don't      *
*   care sets                                                                  *
*
*******************************************************************************/

OUTSYMBOL *select_oni() {
    OUTSYMBOL *temptr, *maxptr;
    int       max_card;

    max_card = 0;

    for (temptr = outsymbol_list; temptr != (OUTSYMBOL *) 0;
         temptr = temptr->next) {
        if (temptr->selected == 0 && temptr->card >= max_card) {
            max_card = temptr->card;
            maxptr   = temptr;
        }
    }

    if (VERBOSE) printf("\nMax_card = %d", max_card);

    maxptr->selected = 1;

    return (maxptr);
}


/*******************************************************************************
*  Writes Mi (result of the i-th partial minimization) into the file "temp81"  *
*  after substituting string "1" instead of the next-state string              *
*                                                                              *
*******************************************************************************/

oncover_inter1() {

    FILE *fopen(), *fp7, *fp81;
    char string1[MAXSTRING];
    char string2[MAXSTRING];
    char string3[MAXSTRING];
    char line[MAXLINE];
    char newline[MAXLINE];

    if ((fp81 = fopen(temp81, "w")) == NULL) {
        fprintf(stderr, "fopen: can't open temp81\n");
        exit(-1);
    }

    /* determines number and size of multiple-valued variables and writes in
       "temp81" the command ".mv ...parameters..." */
    if (ISYMB) {
        sprintf(string1, ".mv %d %d %d %d %d\n", 3, 0, inputnum, statenum, 1 + outputfield - 1);
        fputs(string1, fp81);
    } else {
        sprintf(string1, ".mv %d %d %d %d\n", inputfield + 2, inputfield, statenum, 1 + outputfield - 1);
        fputs(string1, fp81);
    }

    /* WRITES THE PRODUCT-TERMS OF Mi IN "temp81" */
    if ((fp7 = fopen(temp7, "r")) == NULL) {
        fprintf(stderr, "fopen: can't read temp7\n");
        exit(-1);
    } else {
        while (fgets(line, MAXLINE, fp7) != (char *) 0) {
            trailblanks(line, strlen(line));
            if (myindex(line, "# espresso") < 0 && myindex(line, ".mv") < 0 &&
                myindex(line, ".p") < 0 && myindex(line, ".e") < 0 &&
                myindex(line, ".type") < 0 && strlen(line) > 1) {
                sscanf(line, "%s %s %s", string1, string2, string3);
                if (string3[0] == '1') {
                    sprintf(newline, "%s %s %s%s \n", string1, string2, "1", string3 + 1);
                    fputs(newline, fp81);
                } else {
                    /*sprintf(newline, "%s %s %s%s \n", string1, string2, "0", cdrstring3);
                    fputs(newline,fp81);*/
                }
            }
        }
        fclose(fp7);
    }

    fclose(fp81);

}


/*******************************************************************************
*  Writes into the file "temp82" the cover ON0j (where j = onjptr->nlab)       *
*  after substituting the string "1" instead of the next-state string          *
*                                                                              *
*******************************************************************************/

oncover_inter2(onjptr)
OUTSYMBOL *onjptr;

{

FILE *fopen(), *fp82, *fp1;

char string1[MAXSTRING];
char string2[MAXSTRING];
char string3[MAXSTRING];
char string4[MAXSTRING];
char carstring3[MAXSTRING];
char labelstring[MAXSTRING];
char line[MAXLINE];
char newline[MAXLINE];

if ((
fp82 = fopen(temp82, "w")
) == NULL) {
fprintf(stderr,
"fopen: can't create temp82\n");
exit(-1);
}

/* determines number and size of multiple-valued variables and writes in
   "temp82" the command ".mv ...parameters..." */
/*
if (ISYMB) {
    sprintf(string1,".mv %d %d %d %d %d\n", 3, 0 , inputnum , statenum, 1+outputfield-1);
    fputs(string1,fp82);
} else {
      sprintf(string1,".mv %d %d %d %d\n", inputfield+2, inputfield , statenum, 1+outputfield-1);
      fputs(string1,fp82);
}
*/

/* WRITES THE PRODUCT-TERMS OF ON0j IN "temp82" */
if ((
fp1 = fopen(temp1, "r")
) == NULL) {
fprintf(stderr,
"fopen: can't read temp1\n");
exit(-1);
} else {
mvlab(labelstring, onjptr
->nlab, statenum);
while (
fgets(line,MAXLINE, fp1) != (char *) 0 ) {
trailblanks(line, strlen(line)
);
if (
myindex(line,
"# espresso") < 0 &&
myindex(line,
".mv") < 0 &&
myindex(line,
".p") < 0 &&
myindex(line,
".e") < 0 &&
myindex(line,
".type") < 0 &&
strlen(line)
> 1 ) {
sscanf(line,
"%s | %s | %s %s", string1, string2, string3, string4);
strcar(carstring3, string3
);
if (
myindex(carstring3, labelstring
) >= 0 ) {
sprintf(newline,
"%s %s %s%s \n", string1, string2, "1", string4);
fputs(newline, fp82
);
}
}
}
fclose(fp1);
}

fclose(fp82);

}



/*******************************************************************************
*   Invokes the minimizer to get the output constraints                        *
*   This version uses the espresso-II version#2.0 multiple-valued minimizer    *
*                                                                              *
*   External files used : <> temp6 is the input to espresso (ON1i , OFFi)      *
*                         <> temp7 is the output from espresso (Mi)            *
*                         <> temp81 contains Mi with simplified output part    *
*                         <> temp82 contains ON0j with simplified output part  *
*                            (covers of temp81 and temp82 are intersected)     *
*                         <> temp9 contains the final minimized cover :        *
*                                  (P = union Mi for i=1,...,q)                *
*                         <> temp10 contains the irredundant cover             *
*******************************************************************************/

int out_mini(oniptr)
        OUTSYMBOL *oniptr;

{

    FILE       *fopen(), *fp6, *fp1;
    OUTSYMBOL  *outptr;
    ORDER_PATH *curptr;
    char       string1[MAXSTRING];
    char       string2[MAXSTRING];
    char       string3[MAXSTRING];
    char       string4[MAXSTRING];
    char       carstring3[MAXSTRING];
    char       labelstring[MAXSTRING];
    char       dcstring[MAXSTRING];
    char       offstring[MAXSTRING];
    char       newstring[MAXSTRING];
    char       line[MAXLINE];
    char       newline[MAXLINE];
    char       command[MAXSTRING];
    int        on_temp6, on_temp7, card_gain;

    /* temp6 is the input to espresso (ON1i , OFFi) */
    if ((fp6 = fopen(temp6, "w")) == NULL) {
        fprintf(stderr, "fopen: can't create temp6\n");
        exit(-1);
    }
    on_temp6 = 0;

    /* determines number and size of multiple-valued variables and writes in 
       "temp6" the command ".mv ...parameters..." */
    if (ISYMB) {
        sprintf(string1, ".type fr\n.mv %d %d %d %d %d\n", 3, 0, inputnum, statenum, 1 + outputfield - 1);
        fputs(string1, fp6);
    } else {
        sprintf(string1, ".type fr\n.mv %d %d %d %d\n", inputfield + 2, inputfield, statenum, 1 + outputfield - 1);
        fputs(string1, fp6);
    }


    /* PROVARE A LEGGERE I PRODOTTI DI ON1j INVECE CHE DI ON0j
       <> ON1i is the i-th slice of the result of the disjoint minimization 
          of the initial cover                                               */

    /* WRITES THE PRODUCT-TERMS OF ON0i IN "temp6" 
       <> ON0i is the i-th slice of the multiple-value interpretation of the 
          initial cover                                                      */
    if ((fp1 = fopen(temp1, "r")) == NULL) {
        fprintf(stderr, "fopen: can't read temp1\n");
        exit(-1);
    } else {
        mvlab(labelstring, oniptr->nlab, statenum);
        while (fgets(line, MAXLINE, fp1) != (char *) 0) {
            trailblanks(line, strlen(line));
            if (myindex(line, "# espresso") < 0 && myindex(line, ".mv") < 0 &&
                myindex(line, ".p") < 0 && myindex(line, ".e") < 0 &&
                myindex(line, ".type") < 0 && strlen(line) > 1) {
                sscanf(line, "%s | %s | %s %s", string1, string2, string3, string4);
                strcar(carstring3, string3);
                if (myindex(carstring3, labelstring) >= 0) {
                    sprintf(newline, " %s %s 1%s \n", string1, string2, string4);
                    fputs(newline, fp6);
                    on_temp6++;
                }
            }
        }
        fclose(fp1);
    }


    /*           APPENDS THE PRODUCT-TERMS OF DCi TO "temp6" 
           DCi =  Union(ON0k) 
                   where k varies in K and
                   K = { k s.t. there is no path from vi to vk in G(V,A) } ) */

    for (outptr = outsymbol_list; outptr != (OUTSYMBOL *) 0;
         outptr = outptr->next) {
        if (outptr->nlab != oniptr->nlab &&
            not_offi(oniptr, outptr) == 1) {
            if ((fp1 = fopen(temp1, "r")) == NULL) {
                fprintf(stderr, "fopen: can't read temp1\n");
                exit(-1);
            } else {
                mvlab(labelstring, outptr->nlab, statenum);
                while (fgets(line, MAXLINE, fp1) != (char *) 0) {
                    trailblanks(line, strlen(line));
                    if (myindex(line, "# espresso") < 0 && myindex(line, ".mv") < 0 &&
                        myindex(line, ".p") < 0 && myindex(line, ".e") < 0 &&
                        myindex(line, ".type") < 0 && strlen(line) > 1) {
                        sscanf(line, "%s | %s | %s %s", string1, string2, string3, string4);
                        strcar(carstring3, string3);
                        if (myindex(carstring3, labelstring) >= 0) {
                            /* we found a product-term in ON0k that has to be added
                              to DCi                                            */
                            strdc(dcstring, oniptr->nlab);
                            sprintf(newstring, " %s %s -%s\n", string1, string2, string4);
                            fputs(newstring, fp6);
                        }
                    }
                }
                fclose(fp1);
            }
        }
    }


    /*           APPENDS THE PRODUCT-TERMS OF OFFi TO "temp6" 
       OFFi =  Union ( Union(ON0j) 
                   where j varies in J and
                     J = { j s.t. there is a path from vi to vj in G(V,A) } ) */
    for (curptr = path_graph[oniptr->nlab]; curptr != (ORDER_PATH *) 0;
         curptr = curptr->next) {
        if ((fp1 = fopen(temp1, "r")) == NULL) {
            fprintf(stderr, "fopen: can't read temp1\n");
            exit(-1);
        } else {
            mvlab(labelstring, curptr->dest, statenum);
            while (fgets(line, MAXLINE, fp1) != (char *) 0) {
                trailblanks(line, strlen(line));
                if (myindex(line, "# espresso") < 0 && myindex(line, ".mv") < 0 &&
                    myindex(line, ".p") < 0 && myindex(line, ".e") < 0 &&
                    myindex(line, ".type") < 0 && strlen(line) > 1) {
                    sscanf(line, "%s | %s | %s %s", string1, string2, string3, string4);
                    strcar(carstring3, string3);
                    sprintf(newstring, " %s %s %s\n", string1, string2, dcstring);
                    if (myindex(carstring3, labelstring) >= 0) {
                        /* we found a product-term in ON0j that has to be added
                           to OFFi                                            */
                        stroff(offstring, oniptr->nlab);
                        sprintf(newstring, " %s %s 0%s\n", string1, string2, string4);
                        fputs(newstring, fp6);
                    }
                }
            }
            fclose(fp1);
        }
    }

    fclose(fp6);

    /*printf("\nInput to espresso\n");
    system("cat temp6");
    printf("on_temp6 = %d\n", on_temp6);*/

    if (VERBOSE) printf("\nRunning Espresso\n");
    sprintf(command, "espresso %s > %s", temp6, temp7);
    system(command);
    if (VERBOSE) printf("Espresso terminated\n\n");

    /*printf("Output from espresso\n");
    system("cat temp7");*/
    on_temp7 = oncounter_temp7();
    /*printf("on_temp7 = %d\n", on_temp7);*/


    /*                       P = P U Mi 
       the minimized final cover is written into the file "temp9"     */
    card_gain = on_temp6 - on_temp7;
    if (OUT_ALL || (!OUT_ALL && card_gain > 0)) final_cover7(oniptr);
    else final_cover6(oniptr);

    return (card_gain);
}


/*******************************************************************************
*  = 1 iff the next-state symbol pointed to by outptr does not belong to the   *
*  covering path of path_graph[oniptr->nlab]                                   *
*******************************************************************************/

not_offi(oniptr, outptr
)
OUTSYMBOL *oniptr;
OUTSYMBOL *outptr;

{
ORDER_PATH *pathptr;

for (
pathptr = path_graph[oniptr->nlab];
pathptr != (ORDER_PATH *) 0;
pathptr = pathptr->next
) {
if (pathptr->dest == outptr->nlab) return(0);
}

return(1);

}


/*******************************************************************************
*       write into the file "temp9" the minimized final cover computed
*
*       incrementally by "out_mini"                                            *
*******************************************************************************/

final_cover6(oniptr)
OUTSYMBOL *oniptr;

{

FILE      *fopen(), *fp6, *fp9;

IMPLICANT *new, *newimplicant();
char      line[MAXLINE], *fgets();
char      string1[MAXLINE];
char      string2[MAXLINE];
char      string3[MAXLINE];
char      onstring[MAXLINE];

if ((
fp9 = fopen(temp9, "a")
) == NULL) {
fprintf(stderr,
"fopen: can't open temp9\n");
exit(-1);
}

if ((
fp6 = fopen(temp6, "r")
) == NULL) {
fprintf(stderr,
"fopen: can't read temp6\n");
exit(-1);
} else {
while (
fgets(line,MAXLINE, fp6) != (char *) 0 ) {
if (
myindex(line,
"# espresso") < 0 &&
myindex(line,
".mv") < 0 &&
myindex(line,
".p") < 0 &&
myindex(line,
".e") < 0 &&
myindex(line,
".type") < 0 ) {
sscanf(line,
"%s %s %s", string1, string2, string3);
if (string3[0] == ONE) {
stron(onstring, oniptr
->nlab);
onstring[oniptr->nlab] = string3[0];
sprintf(line,
"%s %s %s%s\n", string1, string2, onstring, string3+1);
fputs(line, fp9
);
new = newimplicant(line, oniptr->nlab);
new->
next           = implicant_list;
implicant_list = new;
}
}
}
}

fclose(fp6);
fclose(fp9);

}


final_cover7(oniptr)
OUTSYMBOL *oniptr;

{

FILE      *fopen(), *fp7, *fp9;

IMPLICANT *new, *newimplicant();
char      line[MAXLINE], *fgets();
char      string1[MAXLINE];
char      string2[MAXLINE];
char      string3[MAXLINE];
char      onstring[MAXLINE];

if ((
fp9 = fopen(temp9, "a")
) == NULL) {
fprintf(stderr,
"fopen: can't open temp9\n");
exit(-1);
}

if ((
fp7 = fopen(temp7, "r")
) == NULL) {
fprintf(stderr,
"fopen: can't read temp7\n");
exit(-1);
} else {
while (
fgets(line,MAXLINE, fp7) != (char *) 0 ) {
if (
myindex(line,
"# espresso") < 0 &&
myindex(line,
".mv") < 0 &&
myindex(line,
".p") < 0 &&
myindex(line,
".e") < 0 ) {
sscanf(line,
"%s %s %s", string1, string2, string3);
stron(onstring, oniptr
->nlab);
onstring[oniptr->nlab] = string3[0];
sprintf(line,
"%s %s %s%s\n", string1, string2, onstring, string3+1);
fputs(line, fp9
);
new = newimplicant(line, oniptr->nlab);
new->
next           = implicant_list;
implicant_list = new;
}
}
}

fclose(fp7);
fclose(fp9);

}


int oncounter_temp7() {

    FILE *fopen(), *fp7;
    char line[MAXLINE], *fgets();
    char string1[MAXLINE];
    char string2[MAXLINE];
    char string3[MAXLINE];
    int  counter;

    counter = 0;

    if ((fp7 = fopen(temp7, "r")) == NULL) {
        fprintf(stderr, "fopen: can't read temp7\n");
        exit(-1);
    } else {
        while (fgets(line, MAXLINE, fp7) != (char *) 0) {
            if (myindex(line, "# espresso") < 0 && myindex(line, ".mv") < 0 &&
                myindex(line, ".p") < 0 && myindex(line, ".e") < 0) {
                sscanf(line, "%s %s %s", string1, string2, string3);
                if (string3[0] == ONE) counter++;
            }
        }
    }

    fclose(fp7);

    return (counter);

}


int count_prods(name)
        char *name;
{

    FILE *fopen(), *fpin;
    char line[MAXLINE], string[MAXLINE];
    int  count;

    if ((fpin = fopen(name, "r")) == NULL) {
        fprintf(stderr, "fopen: can't read file temp83\n");
        exit(-1);
    }
    while (fgets(line, MAXLINE, fpin) != NULL) {
        if (myindex(line, ".p") >= 0) {
            sscanf(line, "%s %d", string, &count);
            break;
        }
    }
    fclose(fpin);

    if (count == 0) return (2);
    else return (1);

}
