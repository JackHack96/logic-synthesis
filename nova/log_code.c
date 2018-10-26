

#include "nova.h"

/**************************************************************************
* codes the first element and the other ones related to the constraints   *
* involving it                                                            *
**************************************************************************/


code_first(source,symblemes,num,code_length)
int source;
SYMBLEME *symblemes;
int num;
int code_length;

{
    CONSTRAINT *constrptr;
    int i,j,k,l;
    int mindist,totdist,savevert;

    /*printf("\nCode_first\n");
    printf("==========\n");*/
    
    /* symblemes[source].code gets the zero code */
    newcode(hypervertices[0].code,source,symblemes);
    strcpy(symblemes[source].code,hypervertices[0].code);
    symblemes[source].code_status = FIXED;
    hypervertices[0].ass_status = TRUE;
   
    /* scans the constraints involving "source" starting from the deepest */
    for (i = num-1; i >= 0; i--) {

        for (constrptr = levelarray[i]; constrptr != (CONSTRAINT *) 0; 
                                        constrptr = constrptr->levelnext) {

            if (constrptr->attribute == PRUNED || constrptr->attribute == USED) {
                continue;
            }

            /* if the current constraint involves "source" */
            if (constrptr->relation[source] == ONE) {

                /*printf("\nConstraint currently considered:%s", constrptr->relation);*/

                for (j=0; j<num; j++) {

                    /* j is the symbleme in constrptr to encode */
                    if (constrptr->relation[j] == ONE &&
                        j != source && symblemes[j].code_status == NOTCODED) {

                        /* assigns to the j-th symbleme the free code (k-th 
                           vertex) at the minimum distance from the part of 
                           constrptr already coded */
                        mindist = -1; /* mindist is the minimum distance from a 
                                       free vertex to the part of constrptr 
                                       already coded */

                        for (k=0; k<power(2,code_length); k++) {

                            if (hypervertices[k].ass_status == FALSE) {
                                /* k is a free vertex on the hypercube */
                                totdist = 0;
                                for (l=0; l<num; l++) {

                                    if (constrptr->relation[l] == ONE &&
                                        symblemes[l].code_status != NOTCODED) {

                                        /* l is an already coded symbleme 
                                           belonging to constrptr */
                                        totdist = totdist +
                                         hamm_dist(hypervertices[k].code,
                                          symblemes[l].code,code_length);

                                    }

                                }

                                if ( (totdist<mindist) || (mindist<0) ) {
                                    mindist = totdist;
                                    savevert = k;
                                }

                            }

                        }       

                        newcode(hypervertices[savevert].code,j,symblemes);
                        strcpy(symblemes[j].code,hypervertices[savevert].code);
                        symblemes[j].code_status = FIXED;
                        hypervertices[savevert].ass_status = TRUE;

                    }

                }

                /* deals with the holes if the constraint is not a power of 2 */
                if (constrptr->odd_type == ADMITTED)
                    freeze(constrptr,symblemes,num,code_length);
                constrptr->attribute = USED;

                /* prints the codes */
                /*show_code(symblemes,num);
                printf("\n\n");*/

            }     

        }   

    }    

    /* prints the net */
    /*shownet(&net,net_name,"after code_first");*/

}




/**************************************************************************
*           Propagates from the source a cluster of new codes             *
*                                                                         *
**************************************************************************/

int code_propagate(source,symblemes,num,code_length)
int source;
SYMBLEME *symblemes;
int num;
int code_length;

{
    CONSTRAINT *constrptr;
    int i,j,k,l;
    int mindist,totdist,savevert;
   
    /*printf("\nCode_propagate\n");
    printf("==============\n");*/
    
    /* scans the constraints involving "source" starting from the deepest */
    for (i = num - 1; i >= 0; i--) {

        for (constrptr = levelarray[i]; constrptr != (CONSTRAINT *) 0; 
                                         constrptr = constrptr->levelnext) {

            if ( constrptr->attribute == PRUNED || 
                        constrptr->attribute == USED) {
                continue;
            }

            /* if the current constraint involves "source" */
            if (constrptr->relation[source] == ONE) {

                /*printf("\nConstraint currently considered:%s", constrptr->relation);*/

                for (j=0; j<num; j++) {

                    /* j is the symbleme in constrptr to encode */
                    if (constrptr->relation[j] == ONE && j != source &&
                        symblemes[j].code_status != FIXED) {

                        /* assigns to the j-th symbleme the free code
                           (k vertex) at the minimum distance from the subset 
                           of constrptr already coded */
                        mindist = -1; /* mindist is the minimum distance from a 
                                        free vertex to the subset of constrptr 
                                        already coded */

                        for (k=0; k<power(2,code_length); k++) {

                            if (hypervertices[k].ass_status == FALSE) {

                                /* k is a free vertex on the hypercube */
                                totdist = 0;
                                for (l=0; l<num; l++) {

                                    if (constrptr->relation[l] == ONE &&
                                        symblemes[l].code_status == FIXED) {

                                        /* l is an already coded symbleme 
                                           included in constrptr */
                                        totdist = totdist + hamm_dist(
                                         hypervertices[k].code,
                                         symblemes[l].code,code_length);

                                    } 

                                }

                                if ( (totdist < mindist) || (mindist<0) ) {
                                    mindist = totdist;
                                    savevert = k;
                                }

                            }   

                        }      

			newcode(hypervertices[savevert].code,j,symblemes);
			strcpy(symblemes[j].code,hypervertices[savevert].code);
                        symblemes[j].code_status = FIXED;
                        hypervertices[savevert].ass_status = TRUE;

                    }            

                }            

                /* deals with the holes if the constraint is not a power of 2 */
                if (constrptr->odd_type == ADMITTED) {
                    freeze(constrptr,symblemes,num,code_length);
                }

                constrptr->attribute = USED;

                /* prints the codes */
                /*show_code(symblemes,num);
                printf("\n\n");*/

            }     

        }                 

    }              

    /*shownet(&net,net_name,"after code_propagate");*/                            

}





/**************************************************************************
* codes the source element and the other ones related to the constraints  *
* involving it *
**************************************************************************/

code_next(source,symblemes,num,code_length)
int source;
SYMBLEME *symblemes;
int num;
int code_length;

{
    CONSTRAINT *constrptr;
    int i,j,k,l;
    int mindist,maxdist,totdist,savevert;
 
    /*printf("\nCode_next\n");
    printf("=========\n");*/

    /* assigns to source the free code ( k vertex ) at the maximum distance from
       the vertices already assigned */
    maxdist = 0; /* maxdist is the maximum distance from a free vertex to the 
                    vertices already assigned */
    for (k=0; k<power(2,code_length); k++) {

        if (hypervertices[k].ass_status == FALSE) {

	    /* k is a free vertex on the hypercube */
	    totdist = 0;
            for (l=0; l<num; l++) {

                if (symblemes[l].code_status != NOTCODED) {

	            /* l is an already encoded symbleme included in constrptr */
		    totdist = totdist +
		     hamm_dist(hypervertices[k].code,symblemes[l].code,
                                                     code_length);
                }

            }

            if (totdist > maxdist) {
                maxdist = totdist;
                savevert = k;
            }

        }

    }

    newcode(hypervertices[savevert].code,source,symblemes);
    strcpy(symblemes[source].code,hypervertices[savevert].code);
    symblemes[source].code_status = FIXED;
    hypervertices[savevert].ass_status = TRUE;

    /* prints the codes */
    /*show_code(symblemes,num);
    printf("\n\n");*/

    /* scans the constraints involving "source" starting from the deepest */
    for (i = num-1; i >= 0; i--) {

        for (constrptr = levelarray[i]; constrptr != (CONSTRAINT *) 0;
		                         constrptr = constrptr->levelnext) {

            if (constrptr->attribute == PRUNED || 
					     constrptr->attribute == USED) {
		continue;
            }

            /* if the current constraint involves "source" */
            if (constrptr->relation[source] == ONE) {

                /*printf("\nConstraint currently considered:%s", constrptr->relation);*/

                for (j=0; j<num; j++) {

                    /* j is a symbleme in constrptr to encode */
                    if (constrptr->relation[j] == ONE && 
                         j != source && symblemes[j].code_status != FIXED) {

                        /* assigns to the j-th symbleme the free code 
                           (k vertex) at the minimum distance from the subset  
                           of constrptr already encoded */
                        mindist = -1; /* mindist is the minimum distance from a 
                                         free vertex to the part of constrptr 
				         already encoded */
                        for (k=0; k<power(2,code_length); k++) {

                            if (hypervertices[k].ass_status == FALSE) {

                                /* k is a free vertex on the hypercube */
                                totdist = 0;
                                for (l = 0; l < num; l++) {

                                    if (constrptr->relation[l] == ONE &&
                                        symblemes[l].code_status == FIXED) {

                                        /* l is an already encoded symbleme 
					   included in constrptr */
                                        totdist = totdist +
                                         hamm_dist(hypervertices[k].code,
                                          symblemes[l].code,code_length);

                                    }

                                }

                                if ( (totdist < mindist) || (mindist < 0) ) {
                                    mindist = totdist;
                                    savevert = k;
                                }

                            }

                        }

                        newcode(hypervertices[savevert].code,j,symblemes);
                        strcpy(symblemes[j].code,hypervertices[savevert].code);
                        symblemes[j].code_status = FIXED;
                        hypervertices[savevert].ass_status = TRUE;

                    }    

                }    

                /* deals with the holes if the constraint is not a power of 2 */
                if (constrptr->odd_type == ADMITTED) {
                    freeze(constrptr,symblemes,num,code_length);
		}
                constrptr->attribute = USED;

                /* prints the codes */
                /*show_code(symblemes,num);
                printf("\n\n");*/

            }         

        }        

    }      

    /* prints the net */
    /*shownet(&net,net_name,"after code_next");*/

}



/**************************************************************************
* Fills the field "code" of "vertices" with the boolean coordinates of    *
* that hypervertex on the hypercube of dimension "code_length"            *
**************************************************************************/

codes_box(code_length)
int code_length;

{
    char module;
    int i,j,k,codes_num;

    codes_num = power(2,code_length);
    
    /* clears hypervertices[].code[] before filling them with the right codes */
    for (i=0; i<codes_num; i++) {
        for (j=0; j<code_length; j++) {
            hypervertices[i].code[j] = ZERO;
        }
        hypervertices[i].code[j] = '\0';
    }

    /* hypervertices[i].code[] contains the binary form of i */
    for (i=0; i<codes_num; i++) {

        j = i;
        k = code_length - 1;

        while ( j > 0 ) {

            if ( j % 2 == 0 ) { 
                module = ZERO; 
            } else {
                module = ONE;
            }

            hypervertices[i].code[k] = module;

            j = j / 2;
            k--;
        }

        hypervertices[i].ass_status = FALSE;
    }

    /*printf("\nCodes provided by codes_box\n");
    for (i=0; i<codes_num; i++) {
           printf("i = %d ", i);
           printf("code = %s", hypervertices[i].code);
           printf("\n");
    }*/

}



/************************************************************************
* Freezes the vertices of the hypercube to keep on the same hyperface   *
* symblemes belonging to accepted constraints whose cardinality is not  *
* a power of two                                                        *
************************************************************************/

freeze(constrptr,symblemes,num,code_length)
CONSTRAINT *constrptr;
SYMBLEME *symblemes;
int num;
int code_length;

{
    int i,k,l;

    int mindist,totdist,savevert;

    /*printf("\n\nFreeze:   ");*/
    
    for (i = 0; i < (minpow2(constrptr->card) - constrptr->card); i++) {

	/* assigns to the i-th hole of "constrptr" the free code ( k vertex )
	   at the minimum distance from the subset of constrptr alrady coded */
        mindist = -1; /* mindist is the minimum distance from a free vertex to 
	                 the subset of "constrptr" already coded */
	for (k = 0; k < power(2,code_length); k++) {

	    if (hypervertices[k].ass_status == FALSE) {

		/* k is a free vertex on the hypercube */
		totdist = 0;
		for (l = 0; l < num; l++) {

		    if (constrptr->relation[l] == ONE &&
			symblemes[l].code_status == FIXED ) {

                        /* l is an already coded symbleme included in 
			   constrptr */
                        totdist = totdist +
			 hamm_dist(hypervertices[k].code,symblemes[l].code,
						         code_length);

	            }
                    
		}

		if ( (totdist < mindist) || (mindist < 0) ) {
		    mindist = totdist;
		    savevert = k;
		}

	    }

	}

	/* freezes a vertex iff it is included in the hyperface spanned by
	   "constrptr" */
        /*printf("mindist = %d   ", mindist);
	printf("perimeter = %d\n", perimeter(mylog2(minpow2(constrptr->card))));*/

	if ( mindist <= perimeter(mylog2(minpow2(constrptr->card)))) {
	    hypervertices[savevert].ass_status = TRUE;
	    /*printf("mindist <= perimeter : frozen vertex = %s\n", hypervertices[savevert].code);*/
	} else {
	      /*printf("mindist > perimeter : no vertex frozen\n");*/
	}

    }

}
