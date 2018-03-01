/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/log_approx.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:09 $
 *
 */
/****************************************************************************
* Ve]rtices-wise [b]ottom-[u]p [c]onstraint-[c]lustered [heu]ristic [code]  *
*****************************************************************************/

#include "nova.h"

log_approx(net,symblemes,num,code_length)
CONSTRAINT *net;
SYMBLEME *symblemes;
int num;
int code_length;

{
    int i,sel_symbleme;
    BOOLEAN cycle_in_progress;

    codes_box(code_length);

    /* selects the first symbleme to encode */
    sel_symbleme = sel_first(net,symblemes,num);

    /*printf("\nThe first selected symbleme is %s (%d)\n",
			  symblemes[sel_symbleme].name , sel_symbleme);*/

    /* codes the symbleme returned by sel_first */
    code_first(sel_symbleme,symblemes,num,code_length);


    /* loops while there are symblemes not yet encoded */

    cycle_in_progress = FALSE;

    /* checks if there are symblemes not already encoded */
    for (i=0; i<num; i++) {
        if (symblemes[i].code_status == NOTCODED ) {
            cycle_in_progress = TRUE;
            break;
        }
    }

    while ( cycle_in_progress == TRUE ) {

        /* returns an already coded symbleme from whose code a new
           cluster of symblemes can be propagated on the hypercube */
        sel_symbleme = sel_propagate(net,symblemes,num,code_length);

	/*if (sel_symbleme != -1) {
            printf("\nThe propagation symbleme is %s (%d)\n", 
	        symblemes[sel_symbleme].name , sel_symbleme);
	} else {
              printf("\nNo propagation symbleme was found\n"); 
	}*/
            
        if ( sel_symbleme != -1 ) {  /* there is a propagation symbleme */

            /* codes a new cluster of symblemes starting from the source of the 
               propagation */
            code_propagate(sel_symbleme,symblemes,num,code_length);

        } else {

              /* selects a new symbleme to code ( no propagation possible
                 from the ones already coded ) */
              sel_symbleme = sel_next(net,symblemes,num);

              /* prints the selected element 
              printf("\nThe next symbleme is %s (%d)\n",  
	       symblemes[sel_symbleme].name , sel_symbleme);*/

              /* codes the symbleme selected by sel_next and the other ones
                 connected to it via constraints                            */
              code_next(sel_symbleme,symblemes,num,code_length);

        }

        cycle_in_progress = FALSE;

        /* checks if there are symblemes not already encoded */
        for (i=0; i<num; i++) {
            if (symblemes[i].code_status == NOTCODED) {
                cycle_in_progress = TRUE;
                break;
            }
        }

    }

}
