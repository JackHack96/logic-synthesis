/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/log_output.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:09 $
 *
 */
#include "nova.h"

/******************************************************************************
*                  Last small business before leaving NOVA                    *
******************************************************************************/


log_output()

{
    lsummary();

     if (VERBOSE) printf("\nTHE END of NOVA\n");
}




/**************************************************************************
*              Prints the most interesting results                        *
**************************************************************************/

lsummary()

{

    if (VERBOSE) printf("\n\n\n      SUMMARY\n\n");
    fprintf(stdout,"# Prod.-terms of minimized MV cover = %d\n", onehot_products);
    fprintf(stdout,"# Prod.-terms of encoded cover = %d\n", best_products);
    /*printf("worst_products = %d\n", worst_products);*/
    fprintf(stdout,"# Pla area = %d\n", best_size);
    printf("#\n# .start_codes\n");
    if (ISYMB) show_bestcode(inputs,inputnum);
    show_bestcode(states,statenum);
    printf("# .end_codes\n");

    if (VERBOSE) {
        if (I_GREEDY) {
           printf("\nSatisfaction of the input constraints intersection lattice\n");
           if (ISYMB) {
	       printf("INPUTS :");
	       constr_satisfaction(&inputnet,inputs,inp_codelength,"best");
               printf("\n");
           }
           printf("STATES :");
           constr_satisfaction(&statenet,states,st_codelength,"best");
           printf("\n");
           inputnet = (CONSTRAINT *) 0;
           statenet = (CONSTRAINT *) 0;
           analysis(temp2);
        }
        printf("\nSatisfaction of the input constraints\n");
        if (ISYMB) {
	    printf("INPUTS :");
	    constr_satisfaction(&inputnet,inputs,inp_codelength,"best");
            printf("\n");
        }
        printf("STATES :");
        constr_satisfaction(&statenet,states,st_codelength,"best");
    }

}



int inclusion(string1,string2,length)
char *string1;
char *string2;
int length;

{
    int i,value;

    value = 0;

    for (i = 0; i < length; i++) {
        
        if ( (string2[i] == '*') || (string1[i] == string2[i]) ) {
    	
	    value = 1;
	        
        } else {
	      value = 0;
	      break;
	}
    	
    }
    
    return(value);
    
}



/**************************************************************************
*             Assigns random codes to symblemes and minimizes             *
**************************************************************************/

randomization()

{

    int i;
    int size_summ = 0;
    int random_size;

    if (VERBOSE) printf("\n\nRANDOM ENCODINGS\n");

    /* if the value of "rand_trials" is still -1 at this point it means that the
       user didn't specify his own value of "rand_trials" and relies on the 
       values assigned by default : "statenum" if proper inputs are not 
       symbolic , "statenum" + "inputnum" if the proper inputs are symbolic */ 
    if ( rand_trials == -1) { 

        rand_trials = statenum;

        if (ISYMB) {
	    rand_trials = rand_trials + inputnum;
        }

    }

    for (i=1; i<= rand_trials; i++) {

	if (VERBOSE) printf("\nRand_trials # %d\n", i);    
	random_code(&random_size);
	size_summ = size_summ + random_size;

    }

    fprintf(stdout,"# Prod.-terms of minimized MV cover = %d\n", onehot_products);
    fprintf(stdout,"# Prod.-terms of best encoded cover = %d\n", best_products);
    fprintf(stdout,"# Best pla area = %d\n", best_size);
    fprintf(stdout,"# Average pla area= %d\n", size_summ / rand_trials);
    if (VERBOSE) printf("\nBest random codes:\n");
    printf("#\n# .start_codes\n");
    if (ISYMB) show_bestcode(inputs,inputnum);
    show_bestcode(states,statenum);
    printf("# .end_codes\n");

}




random_code(rand_size)
int *rand_size;

{

    long time();
    int i,new_code,increment,range,current_size,seed,trandom();

    seed = (int) time(0);
    srand(seed);

    /* random codes the inputs */
    if (ISYMB) {
        range = power(2,inp_codelength);
        new_code = trandom(range);
        increment = trandom(range)*2 + 1;
	for (i = 0; i < inputnum; i++) {
	    itob(inputs[i].code,new_code,inp_codelength);
	    new_code = (new_code + increment) % range;
	}
    }

    /* random codes the states */
    range = power(2,st_codelength);
    new_code = trandom(range);
    increment = trandom(range)*2 + 1;
    for (i = 0; i < statenum; i++) {
        itob(states[i].code,new_code,st_codelength);
	new_code = (new_code + increment) % range;
    }

    coded_mini();

    current_size = size();
    *rand_size = current_size;

    if ( best_size == 0 || current_size < best_size ) {
	best_products = min_products;
	best_size = current_size;
	savecover();
    }

}



/************************************************************************
*         Tries the best complementation and then minimizes             *
*                                                                       *
************************************************************************/


rotation()

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
            compl_states(i);

            coded_mini();

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
		savecover();
	    }

            if (VERBOSE) {
	        printf("\nbest_products = %d", best_products);
	        printf("\nbest_size = %d\n", best_size);
	    }
	    /*printf("\nbest codes for the states found so far");
	    show_bestcode(states,statenum);*/

	}

    } else {

	  compl_states(zerostate());

          coded_mini();

          current_size = size();

          /* global optimality */
          if ( best_size == 0 || best_size > current_size ) {
              best_products = min_products;
              best_size = current_size;
              savecover();
	  }

	  if (VERBOSE) printf("best_size = %d\n", best_size);
	  /*printf("\nbest codes for the states found so far");
	  show_bestcode(states,statenum);*/

    }

    if (VERBOSE) printf("\nENDS THE COMPLEMENTATION PART\n");

}




/******************************************************************************
*   Saves the best code found so far in the bestcode field of the symblemes   *
******************************************************************************/

savecover()

{

    char command[MAXSTRING];
    int i;

    /* saves the input codes */
    if (ISYMB) {
	for ( i = 0; i < inputnum; i++ ) {
	    strcpy(inputs[i].best_code,inputs[i].code);
	    strcpy(inputs[i].exbest_code,inputs[i].code);
	}
    }

    /* saves the state codes */
    for ( i = 0; i < statenum; i++ ) {
	strcpy(states[i].best_code,states[i].code);
	strcpy(states[i].exbest_code,states[i].code);
    }

    /* saves the unminimized cover in temp33 - to fix a bug, july 1994 */
    sprintf(command,"cp %s %s", temp3, temp33);
    system(command);

    /* saves the minimized cover in temp5 */
    sprintf(command,"cp %s %s", temp4, temp5);
    system(command);

}

/******************************************************************************
*         Finds zerostate , i.e. the state that appears more                  *
*         often as a next state with a zero proper output                     * 
******************************************************************************/


zerostate()

{

    int i,zero,maxi;

    if (IO_HYBRID || IO_VARIANT || USER || ONEHOT) return(-1);

    zero = -1;
    maxi = -1;

    if (ZERO_FL) {
        for (i=0; i<statenum; i++) {
            if (strcmp(states[i].name,zero_state) == 0) {
                zero = i;
            }
        }
	if (zero == -1) {
	    fprintf(stderr,"\nWARNING:\n");
	    fprintf(stderr,"Wrong specification of -z option (no such state)\n");
}
    } else {
        for (i = 0; i < statenum; i++) {
	    if (zeroutput[i] != 0 && zeroutput[i] > maxi) {
	        maxi = zeroutput[i];
	        zero = i;
	    }
        }
    }

    if (VERBOSE) {
        if (zero == -1) {
	    printf("\nThe zerostate is unspecified\n");
        } else {
	    printf("\nThe zerostate is %s (%d)\n" , states[zero].name , zero);
        }
    }

    return(zero);

}



/******************************************************************************
*                  Complements the codes of the states                        * 
******************************************************************************/

compl_states(null_code)
int null_code;

{

    int state,bit;

    if ( null_code == -1 ) {
	if (VERBOSE) printf("\nUnspecified argument to compl_state\n");
	return;
    }


    /*printf("Codes of the states before the complementation");
    show_code(states,statenum);*/

    for ( bit = 0; bit < strlen(states[0].code); bit++ ) {
	if ( states[null_code].code[bit] == ONE) {
	    /* complements this bit */
	    for ( state = 0; state < statenum; state++ ) {
		if ( states[state].code[bit] == ONE ) {
		    states[state].code[bit] = ZERO;
		} else {
		      states[state].code[bit] = ONE;
		}
	    }
	}
    }

    /*printf("\nCodes of the states after the complementation");
    show_code(states,statenum);*/

}
