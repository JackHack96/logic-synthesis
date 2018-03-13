/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/nova_summ.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
#include "inc/nova.h"

/*******************************************************************************
*                    WRITES THE CONSTRAINTS INTO EXTERNAL FILES                *
*******************************************************************************/

nova_summ()

{

     FILE *fopen(),*fpcons;
     CONSTRAINT *scanner;
     int i,j;
    
     if ( (fpcons = fopen(summ,"w") ) == NULL ) {
         fprintf(stderr,"fopen: can't create summ\n");    
         exit(-1);
     }

     if (I_GREEDY || I_EXACT || I_ANNEAL || I_HYBRID || IO_HYBRID || IO_VARIANT) {
	/*fprintf(fpcons, "onehot_products = %d\n", onehot_products);*/
     }

     if (IO_HYBRID || IO_VARIANT) {

	/*fprintf(fpcons, "symbmin_products = %d\n", symbmin_products);*/

        fprintf(fpcons, "# Cover (bit-wise covering relations among the codes of the states)\n");
        fprintf(fpcons, "# Cover(i,j) = 1 iff the i-th state must be covered by the j-th state\n");
        for ( i = 0; i < statenum; i++) {
           for ( j = 0; j < statenum; j++) {
	      fprintf(fpcons, "%c", order_array[i][j]);
	   }
	   fprintf(fpcons, " wgt:%d", gain_array[i]);
           fprintf(fpcons, "\n");
        }
	fprintf(fpcons, "\n");

     }

     if (I_GREEDY || I_EXACT || I_ANNEAL || I_HYBRID || IO_HYBRID || IO_VARIANT) {

        if (ISYMB) {
	   fprintf(fpcons, "# Face constraints of the symbolic inputs\n");
           for (scanner = inputnet; scanner != (CONSTRAINT *) 0; scanner = scanner->next) {
              fprintf(fpcons, "%s wgt:%d\n", scanner->relation, scanner->weight);
           }
	}
	fprintf(fpcons, "\n# Face constraints of the states\n");
        for (scanner = statenet; scanner != (CONSTRAINT *) 0; scanner = scanner->next) {
           fprintf(fpcons, "%s wgt:%d nxst:%s\n", scanner->relation, scanner->weight, scanner->next_states);
        }

     }

     if (I_GREEDY || RANDOM) {

	if (RANDOM) fprintf(fpcons, "              RANDOM CODES\n");

        if (ISYMB) {
	   fprintf(fpcons, "\nCODES OF THE SYMBOLIC INPUTS");
           for (i = 0; i < inputnum; i++) {
              fprintf(fpcons, "\ninputs[%d]:%s   ", i , inputs[i].name);
	      fprintf(fpcons, "Best code: %s", inputs[i].best_code);
           }
        }
        fprintf(fpcons, "\n\nCODES OF THE STATES");
        for (i = 0; i < statenum; i++) {
           fprintf(fpcons, "\nstates[%d]:%s   ", i , states[i].name);
	   fprintf(fpcons, "Best code: %s", states[i].best_code);
        }
        fprintf(fpcons, "\n");

     }

     if (I_EXACT || I_ANNEAL || I_HYBRID || IO_HYBRID || IO_VARIANT) {

        if (ISYMB) {
	   fprintf(fpcons, "\nCODES OF THE SYMBOLIC INPUTS");
           for (i = 0; i < inputnum; i++) {
              fprintf(fpcons, "\ninputs[%d]:%s   ", i , inputs[i].name);
	      fprintf(fpcons, "Best code: %s", inputs[i].exbest_code);
           }
        }
        fprintf(fpcons, "\n\nCODES OF THE STATES");
        for (i = 0; i < statenum; i++) {
           fprintf(fpcons, "\nstates[%d]:%s   ", i , states[i].name);
	   fprintf(fpcons, "Best code: %s", states[i].exbest_code);
        }
        fprintf(fpcons, "\n");

     }

     fclose(fpcons);

}
