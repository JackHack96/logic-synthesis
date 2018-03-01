/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/log_sel.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:09 $
 *
 */
#include "nova.h"

/**************************************************************************
*                Chooses the first symbleme to encode                     *
*                                                                         *
**************************************************************************/

sel_first(net,symblemes,num)
CONSTRAINT *net;
SYMBLEME *symblemes;
int num;

{
    CONSTRAINT *constrptr1,*constrptr2;
    int *candidates;
    int i,selected,maxcount;

    /* allocates memory for candidates */
    if ((candidates = (int *) calloc((unsigned)(num+1),sizeof(int))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for candidates in sel_first\n");
        exit(-1);
    }
   
    /* searches the deepest constraint */
    for (i = num-1; i>=0; i--) {
        for (constrptr1 = levelarray[i]; constrptr1 != (CONSTRAINT *) 0; 
                                          constrptr1 = constrptr1->levelnext) {
            if (constrptr1->attribute == NOTUSED) {
                break;
            }
        }
        if (constrptr1 != (CONSTRAINT *) 0) { 
            break;
        }
    }
  
    /* if the search for a constraint doesn't return a pointer to nil */
    if ( (constrptr1 != (CONSTRAINT *) 0) && ( i>= 0 ) ) {

        for (i=0; i<num; i++) { 
	    candidates[i] = 0;
        }

        /* gives each symbleme a score equal to 10 for every constraint of 
           the deepest level + the weights of such constraints */
        for (constrptr2 = constrptr1; constrptr2 != (CONSTRAINT *) 0; 
                                       constrptr2 = constrptr2->levelnext) {
            if (constrptr2->attribute == NOTUSED) {
                for (i=0; i<num; i++)
                    if (constrptr2->relation[i] == ONE) {
                        candidates[i] = candidates[i] + 10 + constrptr2->weight;
                    }
            }
        }

        /* gives each symbleme belonging to a constraint originated from an  
           output an additional score of 10 */
        for (constrptr1 = net; constrptr1 != (CONSTRAINT *) 0; 
                            constrptr1 = constrptr1->next) {
            if (constrptr1->source_type != 0) {
                for (i=0; i<num; i++) {
                    if (constrptr1->relation[i] == ONE) {
                        candidates[i] = candidates[i] + 10;
                    }
                }
            }
        }

        /* chooses the element with the largest score */
        maxcount = -1;
        for (i=0; i<num; i++) {
            if (candidates[i] >= maxcount) {
                maxcount = candidates[i];
                selected = i;
            }
        }
  
        /* prints the candidates 
        printf("\nCandidates = ");
        for (i=0; i<num; i++) {
            printf("%d ", candidates[i]);
        }*/

    } else { 

          /* no constraints : searches the first not coded symbleme */
          /*printf("\nSel_first: no constraints - search the first not coded symbleme\n");*/
          for (selected = 0;
               (selected < num) && (symblemes[selected].code_status == FIXED);
               selected++); 

    }

    return(selected);

}




/**************************************************************************
*     chooses a coded element as a propagation source for new codes       *
*                                                                         *
**************************************************************************/


sel_propagate(net,symblemes,num,code_length)
CONSTRAINT *net;
SYMBLEME *symblemes;
int num;
int code_length;

{
    CONSTRAINT *constrptr;
    CONSTRLINK *link;
    int i,j,k,score,maxscore,propagate;

    /* open problem: when more than a cluster of codes has been assigned to
       some vertices , from which one new codes should be propagated ? */

    maxscore = 0;
    propagate = -1;

    /* scans all the symblemes already coded */
    for (i=0; i<num; i++) {

        if (symblemes[i].code_status == FIXED) {

            /* if the code of the i-th symbleme is hamming-distant one from the 
               j-th free vertex - i.e. there are free positions close to i */ 
            for (j=0; j<power(2,code_length); j++) {

                if ( (hypervertices[j].ass_status == FALSE) &&
                     (hamm_dist(symblemes[i].code,hypervertices[j].code,
                                                 code_length) == 1) ) {

                    /* if the i-th symbleme belongs to a constraint not 
                       already used containing symblemes not already coded */
                    for (constrptr = net; constrptr != (CONSTRAINT *) 0; 
                                           constrptr = constrptr->next) {

                        if ( (constrptr->attribute == NOTUSED) && 
                                (constrptr->relation[i]== ONE)   ) {

                            /* k is a symbleme not already coded */
                            for (k=0; k<num; k++) {

                                if ( (constrptr->relation[k] == ONE) && 
                                     (symblemes[k].code_status == NOTCODED) ) {

                                    /* scans the notused constraints involving i
                                    to compute its score - chooses the "i-th"
                                    symbleme with the largest score */
                                    score = 0;

                                    for (link = symblemes[i].constraints;
                                         link != (CONSTRLINK *) 0;
                                         link = link->next) {

                                        if (link->constraint->attribute == NOTUSED) {

                                            score = score + 
                                             (link->constraint->level+1) * 
                                             link->constraint->weight;

                                        }

                                    }

                                    if (score > maxscore) {
                                        maxscore = score;
                                        propagate = i;
                                    }

                                }

                            }     

                        }     
                    }     
                }            

            }      

        }    

    }       

    return(propagate);

}    



/**************************************************************************
* Chooses a new symbleme to code (not constraint-related to the symblemes *
* already coded)                                                          *
*                                                                         *
**************************************************************************/


sel_next(net,symblemes,num)
CONSTRAINT *net;
SYMBLEME *symblemes;
int num;

{
    CONSTRAINT *constrptr1,*constrptr2;
    int *candidates;
    int i,j,jump,selected,maxcount;

    /* allocate memory for candidates */
    if ((candidates = (int *) calloc((unsigned)(num+1),sizeof(int))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for candidates in sel_next\n");
        exit(-1);
    }

    /* search the deepest constraint */
    jump = 0;
    for (i = num-1; i >= 0; i--) {
        for (constrptr1 = levelarray[i]; constrptr1 != (CONSTRAINT *) 0; 
                                          constrptr1 = constrptr1->levelnext) {
            if ( constrptr1->attribute == NOTUSED ) {
                for (j=0; j<num; j++) {
                    if ( (constrptr1->relation[j] == ONE) &&
                         (symblemes[j].code_status == NOTCODED ) ) {
                        jump = 1;
                        break; 
                    }
                }
            }
            if ( constrptr1 != (CONSTRAINT *) 0 && jump == 1) break;
        }
        if ( constrptr1 != (CONSTRAINT *) 0 && jump == 1) break;
    }

    /* if the search for a constraint doesn't return a pointer to nil */
    if ( (constrptr1 != (CONSTRAINT *) 0) && (i >= 0) ) {

        for (i=0; i<num; i++) candidates[i] = 0;

        /* gives each symbleme a score equal to 10 for every constraint of the 
           deepest level + the weights of such constraints */
        for (constrptr2 = constrptr1; constrptr2 != (CONSTRAINT *) 0; 
                                       constrptr2 = constrptr2->levelnext) {

            if (constrptr2->attribute == NOTUSED) {

                for (i = 0; i < num; i++) {

                    if ( constrptr2->relation[i] == ONE && 
                         symblemes[i].code_status == NOTCODED ) {

                        candidates[i] = candidates[i] + 10 + constrptr2->weight;

                    }

                }

            }

        }

        /* each element belonging to a constraint originated from an 
           output is given an additional score of 10 */
        for (constrptr1 = net; constrptr1 != (CONSTRAINT *) 0; 
                                constrptr1 = constrptr1->next ) {
            if (constrptr1->source_type != 0) {
                for (i=0; i<num; i++) {
                    if ( constrptr1->relation[i] == ONE &&
                         symblemes[i].code_status == NOTCODED ) {
                        candidates[i] = candidates[i] + 10;
                    }
                }
            }
        }

        /* the element with the maximum score is chosen */
        maxcount = -1;
        for (i=0; i<num; i++) {
            if (candidates[i] >= maxcount) {
                maxcount = candidates[i];
                selected = i;
            }
        }

        /* prints the candidates */
        /*printf ("\nCandidates = ");
        for (i=0; i<num; i++)
            printf("%d", candidates[i]);
            printf("\n");*/

    } else { 

          /* no constraints: search the first not coded symbleme */
          /*printf("\nSel_next: no constraints - search the first not coded symbleme\n");*/
          for (selected = 0; 
               (selected < num) && (symblemes[selected].code_status == FIXED);
               selected++);

    }

    return(selected);
            
}




