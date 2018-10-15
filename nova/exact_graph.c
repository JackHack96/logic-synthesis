/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/exact_graph.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:09 $
 *
 */
/******************************************************************************
*          Builds the graph of the inclusions among the constraints           *
*                                                                             *
******************************************************************************/

#include "nova.h"

exact_graph(net,net_num)
CONSTRAINT **net;
int net_num;

{

    /* graph_depth is set to max(inputnum,statenum) in array_alloc
       and here is reset to the exact depth of the graph of net */
    graph_depth = mylog2(minpow2(net_num)) + 2;

    /* builds the graph of the constraints as given by the problem */
    base_graph(net);

    if (DEBUG) {
        printf("\nGraph of the constraints for the exact algorithm ");
        printf("( %d levels )\n" , mylog2(minpow2(net_num)) +1 );
        printf("\nAfter base_graph :\n");
        printf("---------------- \n");
        show_graph();
    }

    inter_graph(net_num);
    if (DEBUG) {
        printf("\nAfter inter_graph :\n");
        printf("----------------- \n");
        show_graph();
    }

    complete_graph(net_num);
    if (DEBUG) {
        printf("\nAfter complete_graph :\n");
        printf("-------------------- \n");
        show_graph();
    } else {
	if (VERBOSE) {
            printf("\nGraph of the constraints:\n");
            show_graph();
	}
    }

    sons_graph();
    if (DEBUG) {
        printf("\nAfter sons_graph :\n");
        printf("---------------- \n");
        show_sons();
    }

    fathers_graph();
    if (DEBUG) {
        printf("\nAfter fathers_graph :\n");
        printf("------------------- \n");
        show_fathers();
    }
     


}



/******************************************************************************
*                                                                             * 
*         Builds the graph of the constraints as given by the problem         *
*                                                                             *
******************************************************************************/

base_graph(net)
CONSTRAINT **net;

{

    int i,level;
    CONSTRAINT *constrptr;
    CONSTRAINT_E *newconstr,*newconstraint_e();

    /* clears pointers of graph_levels array */
    for (i = 0; i <= graph_depth-1; i++) {
        graph_levels[i] = ( CONSTRAINT_E *) 0;
    }

    /* empty net : do nothing */
    if ((*net) == (CONSTRAINT *) 0) return; 

    /* "constrptr" scans the list of the constraints found by "analysis" */ 
    if (I_EXACT) {
        for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
                                  constrptr = constrptr->next) {
            /* insert the constraints at the various levels of graph_levels */ 
            level = mylog2(minpow2(constrptr->card));
            newconstr = newconstraint_e(constrptr->relation,constrptr->card);
    
            newconstr->next = graph_levels[level];
            graph_levels[level] = newconstr;
        }
    }
    if (I_HYBRID || IO_HYBRID || IO_VARIANT) {
        for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
                                  constrptr = constrptr->next) {
            if (constrptr->attribute == USED) {
                /* insert the constraints at the various levels of graph_levels */ 
                level = mylog2(minpow2(constrptr->card));
                newconstr = newconstraint_e(constrptr->relation,constrptr->card);
        
                newconstr->next = graph_levels[level];
                graph_levels[level] = newconstr;
	    }
        }
    }



}




/*******************************************************************************
*   Finds the intersections between the constraints already present in         *
*   base_graph                                                                 *
*******************************************************************************/


inter_graph(net_num)
int net_num;

{
    CONSTRAINT_E *constrptr1,*constrptr2,*temptr,*new,*newconstraint_e();
    char *newrelation;
    int i,k,j,card,inter_level;
    int countnewconstr;

    countnewconstr = 0;

    if ((newrelation = (char *) calloc((unsigned)net_num+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for newrelation in inter_graph\n");
        exit(-1);
    }
    newrelation[net_num] = '\0';

    for (i = graph_depth-2; i >= 0; i--) {
        /* "constrptr1" points to the first operand of the intersection 
           operation */
	for (constrptr1 = graph_levels[i]; constrptr1 != (CONSTRAINT_E *) 0;
				         constrptr1 = constrptr1->next ) {

            for (k = i; k >= 0; k--) {
                /* "constrptr2" points to the second operand of the intersection 
                   operation */
                for (constrptr2 = graph_levels[k]; constrptr2 != (CONSTRAINT_E *) 0;
                                                constrptr2 = constrptr2->next) {
    
                     /* compute a new constraint as the intersection of two other 
		        ones */
                     card = 0;
                     for (j=0; constrptr1->relation[j] != '\0'; j++) {
                         if (constrptr1->relation[j] == ONE  &&
                             constrptr2->relation[j] == ONE     ) {
                              card++;
                              newrelation[j] = ONE;
                         } else {
                               newrelation[j] = ZERO;
                         }
                     }
    
                     /* discard empty or single intersections */
                     if ( card <= 1 ) {
                         continue;
                     }

                     /* take care of intersections coinciding with already present 
		        elements */
                     inter_level = mylog2(minpow2(card));
	             for (temptr = graph_levels[inter_level]; 
		          temptr != (CONSTRAINT_E *) 0; temptr = temptr->next ) {
                         if (strcmp(newrelation,temptr->relation) == 0) {
                             break;
                         }
                     }
    
                     /* constraint not already found */
                     if (temptr == (CONSTRAINT_E *) 0) {
                         new = newconstraint_e(newrelation,card);
                         new->next = graph_levels[inter_level];
                         graph_levels[inter_level] = new;   
                         countnewconstr++;
                      } 
    
		      if ( countnewconstr > 1000 ) {
		          fprintf(stderr,"\n                  WARNING\n");
		          fprintf(stderr,"** After that lattice added the 1001-th new constraint ,\n");
		          fprintf(stderr,"Nova stopped executing lattice and went ahead with the\n");
		          fprintf(stderr,"constraints that lattice already got ****************\n\n");
		          return;
	             }
                 }
	     }
         }    
    }
}



/*******************************************************************************
*   Adds to the graph of the constraints the following trivial ones :          *
*   1) the constraint including all symbols                                    *
*   2) the constraints including only one symbol at a time                     *
*******************************************************************************/


complete_graph(net_num)
int net_num;

{
    CONSTRAINT_E *new,*newconstraint_e();
    char *newrelation;
    int i,j,level;

    if ((newrelation = (char *) calloc((unsigned)net_num+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for newrelation in inter_graph\n");
        exit(-1);
    }
    newrelation[net_num] = '\0';


    
    /* Adds to the graph of the constraints the following trivial one :      
       the constraint including all symbols                                */

    for (j=0; j < net_num; j++) {
        newrelation[j] = ONE;
    }

    new = newconstraint_e(newrelation,net_num);
    new->next = graph_levels[graph_depth-1];
    graph_levels[graph_depth-1] = new;   

    /* Adds to the graph of the constraints the following trivial ones :       
       the constraints including only one symbol at a time                  */
    level = 0;

    for (i=0; i < net_num; i++) {
        for (j=0; j < net_num; j++) {
	    newrelation[j] = ZERO;
        }
	newrelation[i] = ONE;

        new = newconstraint_e(newrelation,1);
        new->next = graph_levels[level];
        graph_levels[level] = new;   
    }

}



/*****************************************************************************
*                Links the constraints to all their sons                     *
*****************************************************************************/

sons_graph()

{

    CONSTRAINT_E *ftconstr,*sonconstr,*temptr2;
    SONS_LINK *sons_link,*temptr1;
    int i,j;

    for (i = graph_depth-1; i > 0; i--) {
	for (ftconstr = graph_levels[i]; ftconstr != (CONSTRAINT_E *) 0;
				         ftconstr = ftconstr->next ) {
            for (j = i; j >= 0; j--) {
	        for (sonconstr = graph_levels[j]; sonconstr != (CONSTRAINT_E *) 0;
				                 sonconstr = sonconstr->next ) {

		    /* if the constraint pointed to by sonconstr is contained
		       properly in the father it is a potential son           */
	            if (p_incl_s(ftconstr->relation,sonconstr->relation) == 1 &&
		         strcmp(ftconstr->relation,sonconstr->relation) != 0 ) {

			/* if the current potential son is included properly in
			   another son already in the list , don't add it */
		        for (temptr1 = ftconstr->down_ptr; temptr1 != ( SONS_LINK *) 0;
						 temptr1 = temptr1->next ) {
                            
	                    if ( p_incl_s(temptr1->constraint_e->relation,sonconstr->relation) == 1 &&
		                strcmp(temptr1->constraint_e->relation,sonconstr->relation) != 0 ) {
		                break;	        
	               	    }
                        }

			/* if the current potential son is included properly in
			   another constraint at the same level, which in its 
                           turn is included properly in the father, don't add 
                           it */
	                for (temptr2 = graph_levels[j]; temptr2 != (CONSTRAINT_E *) 0;
				                     temptr2 = temptr2->next ) {
	                    if ( p_incl_s(temptr2->relation,sonconstr->relation) == 1 &&
		                 strcmp(temptr2->relation,sonconstr->relation) != 0 &&
	                         p_incl_s(ftconstr->relation,temptr2->relation) == 1 &&
		                 strcmp(ftconstr->relation,temptr2->relation) != 0 ) {
                                break;
                            }
                        }
                        
                        if (temptr1 == (SONS_LINK *) 0 &&
                            temptr2 == (CONSTRAINT_E *) 0) {
                            if ((sons_link = (SONS_LINK *) calloc((unsigned)1,sizeof(SONS_LINK))) == ( SONS_LINK *) 0) {
                                fprintf(stderr,"Insufficient memory for sons_link in sons_graph");
                                exit(-1);
                            }
                            sons_link->constraint_e = sonconstr;
                            sons_link->next = ftconstr->down_ptr;
                            ftconstr->down_ptr = sons_link;
			    sons_link->codfather_ptr = (CONSTRAINT_E *) 0;
			}

                    }
                }
	    }
        }
    }

}



/*****************************************************************************
*               Links the constraints to all their fathers                   *
*****************************************************************************/

fathers_graph()

{

    CONSTRAINT_E *ftconstr,*sonconstr,*temptr2;
    FATHERS_LINK *fathers_link,*temptr1;
    int i,j;

    for (i = 0; i < graph_depth-1; i++) {
	for (sonconstr = graph_levels[i]; sonconstr != (CONSTRAINT_E *) 0;
				         sonconstr = sonconstr->next ) {
            for (j = i; j <= graph_depth-1; j++) {
	        for (ftconstr = graph_levels[j]; ftconstr != (CONSTRAINT_E *) 0;
				                 ftconstr = ftconstr->next ) {

		    /* if the constraint pointed to by ftconstr contains
		       properly the son it is a potential father          */
	            if (p_incl_s(ftconstr->relation,sonconstr->relation) == 1 &&
		         strcmp(ftconstr->relation,sonconstr->relation) != 0 ) {

			/* if the current potential father includes properly 
			   another father already in the list , don't add it */
		        for (temptr1 = sonconstr->up_ptr; temptr1 != ( FATHERS_LINK *) 0;
						 temptr1 = temptr1->next ) {
                            
	                    if (p_incl_s(ftconstr->relation,temptr1->constraint_e->relation) == 1 &&
		              strcmp(ftconstr->relation,temptr1->constraint_e->relation) != 0 ) {
		                break;	        
	               	    }
                        }

			/* if the current potential father includes properly 
			   another constraint at the same level, which in its 
                           turn includes properly the son, don't add it */
	                for (temptr2 = graph_levels[j]; temptr2 != (CONSTRAINT_E *) 0;
				                     temptr2 = temptr2->next ) {
	                    if ( p_incl_s(ftconstr->relation,temptr2->relation) == 1 &&
		                 strcmp(ftconstr->relation,temptr2->relation) != 0 &&
	                         p_incl_s(temptr2->relation,sonconstr->relation) == 1 &&
		                 strcmp(temptr2->relation,sonconstr->relation) != 0 ) {
                                break;
                            }
                        }

                        if (temptr1 == (FATHERS_LINK *) 0 &&
                            temptr2 == (CONSTRAINT_E *) 0   ) {
                            if ((fathers_link = (FATHERS_LINK *) calloc((unsigned)1,sizeof(FATHERS_LINK))) == ( FATHERS_LINK *) 0) {
                                fprintf(stderr,"Insufficient memory for fathers_link in fathers_graph");
                                exit(-1);
                            }
                            fathers_link->constraint_e = ftconstr;
                            fathers_link->next = sonconstr->up_ptr;
                            sonconstr->up_ptr = fathers_link;
			}

                    }
                }
	    }
        }
    }

}
