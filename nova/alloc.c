/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/alloc.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:08 $
 *
 */
/***************************************************************************
*                           ALLOCATES MEMORY                               *
***************************************************************************/


#include "inc/nova.h"

newinputrow (input,pstate,nstate,output)
char *input;
char *pstate;
char *nstate;
char *output;

{
    INPUTTABLE *new;

    if ( (new = (INPUTTABLE *) calloc((unsigned)1,sizeof(INPUTTABLE))) == (INPUTTABLE *) 0) {
        fprintf(stderr,"Insufficient memory for inputtable");
        exit(-1);
    }

 
    if ( (new->input = (char *) calloc((unsigned)strlen(input)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for inputtable->input");
        exit(-1);
    }

    if ( (new->pstate = (char *) calloc((unsigned)strlen(pstate)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for inputtable->pstate");
        exit(-1);
    }

    if ( (new->nstate = (char *) calloc((unsigned)strlen(nstate)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for inputtable->nstate");
        exit(-1);
    }

    if ( (new->output = (char *) calloc((unsigned)strlen(output)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for inputtable->output");
        exit(-1);
    }

    strcpy(new->input,input);
    strcpy(new->pstate,pstate);
    strcpy(new->nstate,nstate);
    strcpy(new->output,output);

    new->acc = 0;

    if (firstable == (INPUTTABLE *) 0) {
         firstable = new;
    } else {
         lastable->next = new;
    }
    lastable = new;


}




CONSTRAINT *newconstraint(relation,card,level)
char *relation;
int card;
int level;

{
    CONSTRAINT *new;

    if ( (new = (CONSTRAINT *) calloc((unsigned)1,sizeof(CONSTRAINT))) == (CONSTRAINT *) 0) {
        fprintf(stderr,"Insufficient memory for constraint");
        exit(-1);
    }

    if ( (new->relation = (char *) calloc((unsigned)strlen(relation)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for constraint->relation");
        exit(-1);
    }
    strcpy(new->relation,relation);

    if ( (new->next_states = (char *) calloc((unsigned)strlen(relation)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for constraint->next_states");
        exit(-1);
    }

    new->face = "";

    new->weight = 1;
    new->level = level;
    new->newlevel = level;
    new->card = card;

    new->attribute   = NOTUSED;
    new->odd_type    = NOTADMITTED;
    new->source_type = INPUT;

    new->levelnext = (CONSTRAINT *) 0;
    new->cardnext  = (CONSTRAINT *) 0;
    new->next      = (CONSTRAINT *) 0;

    return (new);

}



newcode(code,which_element,symbleme_type)
char *code;
int which_element;
SYMBLEME *symbleme_type;

{
    CODE *new;

    if ( (new = (CODE *) calloc((unsigned)1,sizeof(CODE))) == (CODE *) 0) {
        fprintf(stderr,"Insufficient memory for code");
        exit(-1);
    }

    if ( (new->value = (char *) calloc((unsigned)strlen(code)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for code->value");
        exit(-1);
    }

    new->value[strlen(code)] = '\0';

    strcpy(new->value,code);

    new->next = symbleme_type[which_element].vertices;

    symbleme_type[which_element].vertices = new;


}



CONSTRAINT_E *newconstraint_e(relation,card)
char *relation;
int card;

{
    CONSTRAINT_E *new;

    if ( (new = (CONSTRAINT_E *) calloc((unsigned)1,sizeof(CONSTRAINT_E))) == (CONSTRAINT_E *) 0) {
        fprintf(stderr,"Insufficient memory for constraint");
        exit(-1);
    }

    if ( (new->relation = (char *) calloc((unsigned)strlen(relation)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for constraint->relation");
        exit(-1);
    }
    strcpy(new->relation,relation);

    new->weight = 1;
    new->card = card;

    new->up_ptr    = (FATHERS_LINK *) 0;
    new->down_ptr  = (SONS_LINK *) 0;
    new->next      = (CONSTRAINT_E *) 0;
    new->face_ptr  = (FACE *) 0;

    return (new);

}



array_alloc()

{
    int i,max_codelength,num_codes;

    if (ISYMB) {
        symblemenum = max(statenum,inputnum);
    } else {
        symblemenum = statenum;
    }

    if ( (states = (SYMBLEME *) calloc((unsigned)statenum,sizeof(SYMBLEME))) == (SYMBLEME *) 0) {
        fprintf(stderr,"Insufficient memory for states");
        exit(-1);
    }

    for (i=0; i<statenum; i++) {
         if ( (states[i].name = (char *) calloc((unsigned)MAXSTRING,sizeof(char))) == (char *) 0) {
             fprintf(stderr,"Insufficient memory for states[].name");
             exit(-1);
         }
    }

    for (i=0; i<statenum; i++) {
         if ( (states[i].code = (char *) calloc((unsigned)st_codelength+1,sizeof(char))) == (char *) 0) {
             fprintf(stderr,"Insufficient memory for states[].code");
             exit(-1);
         }
    }
    
    for (i=0; i<statenum; i++) {
         if ( (states[i].best_code = (char *) calloc((unsigned)st_codelength+1,sizeof(char))) == (char *) 0) {
             fprintf(stderr,"Insufficient memory for states[].best_code");
             exit(-1);
         }
    }

    for (i=0; i<statenum; i++) {
         if ( (states[i].exact_code = (char *) calloc((unsigned)statenum+1,sizeof(char))) == (char *) 0) {
             fprintf(stderr,"Insufficient memory for states[].exact_code");
             exit(-1);
         }
    }

    for (i=0; i<statenum; i++) {
         if ( (states[i].exbest_code = (char *) calloc((unsigned)statenum+1,sizeof(char))) == (char *) 0) {
             fprintf(stderr,"Insufficient memory for states[].exbest_code");
             exit(-1);
         }
    }


    if (ISYMB) {

        if ( (inputs = (SYMBLEME *) calloc((unsigned)inputnum,sizeof(SYMBLEME))) == (SYMBLEME *) 0) {
            fprintf(stderr,"Insufficient memory for inputs");
            exit(-1);
        }

        for (i=0; i<inputnum; i++) {
             if ( (inputs[i].name = (char *) calloc((unsigned)MAXSTRING,sizeof(char))) == (char *) 0) {
                 fprintf(stderr,"Insufficient memory for inputs[].name");
                 exit(-1);
             }
        }

        for (i=0; i<inputnum; i++) {
             if ( (inputs[i].code = (char *) calloc((unsigned)inp_codelength+1,sizeof(char))) == (char *) 0) {
                 fprintf(stderr,"Insufficient memory for inputs[].code");
                 exit(-1);
             }
        }

        for (i=0; i<inputnum; i++) {
             if ( (inputs[i].best_code = (char *) calloc((unsigned)inp_codelength+1,sizeof(char))) == (char *) 0) {
                 fprintf(stderr,"Insufficient memory for inputs[].best_code");
                 exit(-1);
             }
        }

        for (i=0; i<inputnum; i++) {
             if ( (inputs[i].exact_code = (char *) calloc((unsigned)inputnum+1,sizeof(char))) == (char *) 0) {
                 fprintf(stderr,"Insufficient memory for inputs[].exact_code");
                 exit(-1);
             }
        }

        for (i=0; i<inputnum; i++) {
             if ( (inputs[i].exbest_code = (char *) calloc((unsigned)inputnum+1,sizeof(char))) == (char *) 0) {
                 fprintf(stderr,"Insufficient memory for inputs[].exbest_code");
                 exit(-1);
             }
        }

    }

 
    if ( (levelarray = (CONSTRAINT **) calloc((unsigned)symblemenum,sizeof(CONSTRAINT *))) == (CONSTRAINT **) 0) {
        fprintf(stderr,"Insufficient memory for levelarray");
        exit(-1);
    }

    if ( (cardarray = (CONSTRAINT **) calloc((unsigned)symblemenum,sizeof(CONSTRAINT *))) == (CONSTRAINT **) 0) {
        fprintf(stderr,"Insufficient memory for cardarray");
        exit(-1);
    }

    graph_depth = mylog2(minpow2(symblemenum)) + 2;

    if ( (graph_levels = (CONSTRAINT_E **) calloc((unsigned)graph_depth,sizeof(CONSTRAINT_E *))) == (CONSTRAINT_E **) 0) {
        fprintf(stderr,"Insufficient memory for graph_levels");
        exit(-1);
    }


    if (ISYMB) max_codelength = max(st_codelength,inp_codelength);
     else max_codelength = st_codelength;

    num_codes = power(2,max_codelength);

    if ( (hypervertices = (HYPERVERTEX *) calloc((unsigned)num_codes,sizeof(HYPERVERTEX))) == (HYPERVERTEX *) 0) {
        fprintf(stderr,"Insufficient memory for hypervertices");
        exit(-1);
    }

    for (i=0; i<num_codes; i++) {
         if ( (hypervertices[i].code = (char *) calloc((unsigned)max_codelength+1,sizeof(char))) == (char *) 0) {
             fprintf(stderr,"Insufficient memory for hypervertices[].code");
             exit(-1);
         }
    }


    if ( (zeroutput = (int *) calloc((unsigned)statenum,sizeof(int))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for zeroutput");
        exit(-1);
    }

}


/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
/*           MEMORY ALLOCATION FOR THE SYMBOLIC MINIMIZATION LOOP           */
/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

new_outsymbol(element,lab)
char *element;
int lab;

{
    OUTSYMBOL *new;

    if ( (new = (OUTSYMBOL *) calloc((unsigned)1,sizeof(OUTSYMBOL))) == (OUTSYMBOL *) 0) {
        fprintf(stderr,"Insufficient memory for outsymbol");
        exit(-1);
    }

    if ( (new->element = (char *) calloc((unsigned)strlen(element)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for outsymbol->element");
        exit(-1);
    }

    new->element[strlen(element)] = '\0';

    strcpy(new->element,element);

    new->nlab = lab;

    new->card = 1;

    new->selected = 0;

    new->next = outsymbol_list;

    outsymbol_list = new;


}



new_order_relation(row,column)
int row;
int column;

{
    ORDER_RELATION *new;

    if ( (new = (ORDER_RELATION *) calloc((unsigned)1,sizeof(ORDER_RELATION))) == (ORDER_RELATION *) 0) {
        fprintf(stderr,"Insufficient memory for order_relation");
        exit(-1);
    }

    new->index = column;

    new->next = order_graph[row];

    order_graph[row] = new;

}



new_order_path(row,column)
int row;
int column;

{
    ORDER_PATH *new;

    if ( (new = (ORDER_PATH *) calloc((unsigned)1,sizeof(ORDER_PATH))) == (ORDER_PATH *) 0) {
        fprintf(stderr,"Insufficient memory for order_path");
        exit(-1);
    }

    new->dest = column;

    new->next = path_graph[row];

    path_graph[row] = new;

}



IMPLICANT *newimplicant(row,state)
char *row;
int state;

{
    IMPLICANT *new;

    if ( (new = (IMPLICANT *) calloc((unsigned)1,sizeof(IMPLICANT))) == (IMPLICANT *) 0) {
        fprintf(stderr,"Insufficient memory for implicant");
        exit(-1);
    }

    if ( (new->row = (char *) calloc((unsigned)strlen(row)+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for implicant->row");
        exit(-1);
    }
    strcpy(new->row,row);

    new->state = state;
    new->attribute = NOTUSED;

    new->next = (IMPLICANT *) 0;

    return (new);

}



/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */


array_alloc2()

{

    int i;

    if ( (order_graph = (ORDER_RELATION **) calloc((unsigned)statenum,sizeof(ORDER_RELATION *))) == (ORDER_RELATION **) 0) {
        fprintf(stderr,"Insufficient memory for order_graph");
        exit(-1);
    }

    for ( i = 0; i < statenum; i++ ) {
	order_graph[i] = ( ORDER_RELATION *) 0;
    }

    if ( (path_graph = (ORDER_PATH **) calloc((unsigned)statenum,sizeof(ORDER_PATH *))) == (ORDER_PATH **) 0) {
        fprintf(stderr,"Insufficient memory for order_graph");
        exit(-1);
    }

    for ( i = 0; i < statenum; i++ ) {
	path_graph[i] = ( ORDER_PATH *) 0;
    }

    if ( (reached = (int *) calloc((unsigned)statenum,sizeof(int ))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for array 'reached'");
        exit(-1);
    }
    for ( i = 0; i < statenum; i++ ) {
	reached[i] = 0;
    }

    if ( (order_array = (char **) calloc((unsigned)statenum,sizeof(char *))) == (char **) 0) { fprintf(stderr,"Insufficient memory for 'order_array'"); exit(-1); }
    for ( i = 0; i < statenum; i++ ) {
        if ( (order_array[i] = (char *) calloc((unsigned)statenum+1,sizeof(char))) == (char *) 0) {
            fprintf(stderr,"Insufficient memory for order_array[i]");
            exit(-1);
        }
        order_array[i][statenum] = '\0';
    }

    if ( (gain_array = (int *) calloc((unsigned)statenum,sizeof(int ))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for gain_array");
        exit(-1);
    }
    for ( i = 0; i < statenum; i++ ) {
	gain_array[i] = 0;
    }

    if ( (select_array = (int *) calloc((unsigned)statenum,sizeof(int ))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for select_array");
        exit(-1);
    }
    for ( i = 0; i < statenum; i++ ) {
	select_array[i] = PRUNED;
    }

}
