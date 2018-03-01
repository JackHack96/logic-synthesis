/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/exact_output.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:09 $
 *
 */
#include "nova.h"

/******************************************************************************
*                  Last small business before leaving NOVA                    *
******************************************************************************/


exact_output()

{
    ex_summary();

    if (VERBOSE) printf("\nTHE END of NOVA\n");
}




/**************************************************************************
*              Prints the most interesting results                        *
**************************************************************************/

ex_summary()

{
    if (VERBOSE) printf("\n\n\n      SUMMARY\n\n");
    fprintf(stdout,"# Prod.-terms of minimized MV cover = %d\n", onehot_products);
    if (IO_HYBRID || IO_VARIANT) fprintf(stdout,"# Prod.-terms of symbolic cover = %d\n", symbmin_card());
    /*printf("worst_products = %d\n", worst_products);*/
    fprintf(stdout,"# Prod.-terms of encoded cover = %d\n", best_products);
    fprintf(stdout,"# Pla area = %d\n", best_size);
    printf("#\n# .start_codes\n");
    if (ISYMB) show_exbestcode(inputs,inputnum);
    show_exbestcode(states,statenum);
    printf("# .end_codes\n");
    if (VERBOSE) {
        if (I_HYBRID || IO_HYBRID || IO_VARIANT || USER || ONEHOT) {
            if (ISYMB) {
	        printf("\nINPUTS :");
	        constr_satisfaction(&inputnet,inputs,inp_codelength,"exact");
            }
	    printf("\nSTATES :");
	    constr_satisfaction(&statenet,states,st_codelength,"exact");
        }
        if (IO_HYBRID || IO_VARIANT) { 
	    out_performance("exbest"); 
	    weighted_outperf(statenum,"exbest"); 
        }
    }

}



/*******************************************************************************
*   Verifies if the exact encoding algorithm satisfied all constraints         *
*   (as it should!) - if it didn't , it stops the execution signalling         *
*   the event (it means that there is a bug in the program)                    *
*******************************************************************************/

int exact_check(net,symblemes,net_num)
CONSTRAINT **net;
SYMBLEME *symblemes;
int net_num;

{

    CONSTRAINT_E *constrptr_e;
    CONSTRAINT *constrptr;
    char *face;
    char bit1;
    BOOLEAN check;
    int i,j,code_lgt;

    check = TRUE;

    /* copy the codes of the exact algorithm in symblemes[].exact_code */
    i = net_num-1;
    for (constrptr_e = graph_levels[0]; constrptr_e != (CONSTRAINT_E *) 0;
				         constrptr_e = constrptr_e->next ) {
        if ( constrptr_e->face_ptr != (FACE *) 0 ) {
            strcpy(symblemes[i].exact_code,constrptr_e->face_ptr->cur_value);
	    i--;
        }
    }    
    
    /* empty net : do nothing */
    if ((*net) == (CONSTRAINT *) 0) {
	return;
    }

    code_lgt = strlen(symblemes[0].exact_code);

    if ((face = (char *) calloc((unsigned)code_lgt+1,sizeof(char))) == (char *) 0) {
	fprintf(stderr,"\nInsufficient memory for 'face' in exact_check\n");
	exit(-1);
    }
    face[code_lgt] = '\0';

    /* scans all constraints */
    for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
					   constrptr = constrptr->next) {

        /*printf("The constraint is %s  ", constrptr->relation);*/

        /* scans all boolean coordinates */
        for (i = 0; i < code_lgt; i++) {

            /* stores in bit1 the "i-th" bit of the best_code of the "j-th" 
	       symbleme ( which belongs to the current constraint )        */ 
	    for ( j = 0; j < net_num; j++) {
		if ( constrptr->relation[j] == ONE ) {
	            bit1 = symblemes[j].exact_code[i]; 
		}
	    }

            /* if there are two symblemes in the current constraint different 
	       on the i-th boolean coordinate face[i] gets a * (don't care)  
	       otherwise face[i] gets the common coordinate value           */ 
	    for ( j = 0; j < net_num; j++) {
		if ( constrptr->relation[j] == ONE ) {
		    if ( bit1 != symblemes[j].exact_code[i]) {
		        face[i] = '*'; 
			break;
		    } else {
			  face[i] = bit1;
		    }
                }    
	    }

	}     

        /*printf("the spanned face is %s\n", face);*/

	/* if the exact_code of any other symbleme ( i.e. a symbleme not 
	   contained in constraint ) is covered by face , the current
	   constraint has not been satisfied                             */
        for (j = 0; j < net_num; j++) {
            if ( constrptr->relation[j] == ZERO ) {
		if (inclusion(symblemes[j].exact_code,face,code_lgt) == 1) {
		    fprintf(stderr,"the symbleme %s (%d)  is covered by face\n", symblemes[j].name, j);
		    fprintf(stderr,"UNSATISFIED\n");
	            fprintf(stderr,"\nFATAL ERROR in the verification of the exact encoding\n");
		    check = FALSE;
	            exit(-1);
		    break;
		}
	    }
	}

	/* no other constraint covered by face */
	if (j == net_num) {
	    /*printf("SATISFIED\n");*/
	}

    }

    if (VERBOSE && check == TRUE) {
	printf("\nThe exact encoding algorithm satisfied all constraints\n");
    } 

}



/************************************************************************
*         Tries the best complementation and then minimizes             *
*                                                                       *
************************************************************************/


exact_rotation()

{

    int i,current_size;

    if (VERBOSE) printf("\n\nSTARTS THE COMPLEMENTATION PART\n");

    worst_products = 0;
    first_size = best_size = 0;

    if (COMPLEMENT) {

	/* complements the codes of the states and then minimizes */
	for (i = 0; i < statenum; i++) {

	    if (VERBOSE)printf("\nThe null code is assigned to the state %s\n", 
							     states[i].name);
            excompl_states(i);

            exact_mini();

            current_size = size();

	    if (min_products > worst_products) {
		worst_products = min_products;
	    }

	    /* local optimality */
	    if ( first_size == 0 || first_size > current_size ) {
		first_size = current_size;
	    }

	    /* global optimality */
	    if ( best_size == 0 || best_size > first_size ) {
		best_products = min_products;
		best_size = first_size;
		save_excover();
	    }

	    if (VERBOSE) { 
	        printf("\nbest_products = %d", best_products);
	        printf("\nbest_size = %d\n", best_size);
	        /*printf("\nbest codes for the states found so far");
	        show_exbestcode(states,statenum);*/
    
	        /* analyzes the correlation between the output
	           covering relations and rotations                 */
	        if (IO_HYBRID || IO_VARIANT) out_performance("exact");
	    }

	}

    } else {

	  excompl_states(zerostate());

          exact_mini();

          current_size = size();

          /* global optimality */
          if ( best_size == 0 || best_size > current_size ) {
              best_products = min_products;
              best_size = current_size;
              save_excover();
	  }

	  if (VERBOSE) printf("best_size = %d\n", best_size);
	  /*printf("\nbest codes for the states found so far");
	  show_exbestcode(states,statenum);*/

    }

    if (VERBOSE) printf("\nENDS THE COMPLEMENTATION PART\n");

}




/******************************************************************************
*   Saves the best code found so far in the bestcode field of the symblemes   *
******************************************************************************/

save_excover()

{

    char command[MAXSTRING];
    int i;

    /* saves the input codes */
    if (ISYMB) {
	for ( i = 0; i < inputnum; i++ ) {
	    strcpy(inputs[i].exbest_code,inputs[i].exact_code);
}
    }

    /* saves the state codes */
    for ( i = 0; i < statenum; i++ ) {
	strcpy(states[i].exbest_code,states[i].exact_code);
    }

    /* saves the unminimized cover in temp33 - to fix a bug, july 1994 */
    sprintf(command,"cp %s %s", temp3, temp33);
    system(command);

    /* saves the minimized cover in temp5 */
    sprintf(command,"cp %s %s", temp4, temp5);
    system(command);

}



/******************************************************************************
*                  Complements the codes of the states                        * 
******************************************************************************/

excompl_states(null_code)
int null_code;

{

    int state,bit;

    if ( null_code == -1 ) {
	if (VERBOSE) printf("\nUnspecified argument to excompl_states\n");
	return;
    }


    /*printf("Codes of the states before the complementation");
    show_exactcode(states,statenum);*/

    for ( bit = 0; bit < strlen(states[0].exact_code); bit++ ) {
	if ( states[null_code].exact_code[bit] == ONE) {
	    /* complements this bit */
	    for ( state = 0; state < statenum; state++ ) {
		if ( states[state].exact_code[bit] == ONE ) {
		    states[state].exact_code[bit] = ZERO;
		} else {
		      states[state].exact_code[bit] = ONE;
		}
	    }
	}
    }

    /*printf("\nCodes of the states after the complementation");
    show_exactcode(states,statenum);*/

}
