/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/show.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
/*****************************************************************************
*                     Shows the net of the constraints                       *
*                                                                            *
*****************************************************************************/

#include "nova.h"

shownet(net,net_name,header)
CONSTRAINT **net;
char *net_name;
char *header;

{
    CONSTRAINT *scanner;

    if ( header != (char *) 0) printf("\n%s %s\n", net_name, header); 

    for (scanner = (*net); scanner != (CONSTRAINT *) 0; scanner = scanner->next) {
        printf("%s wgt:%d lev:%d nxst:%s ", scanner->relation, scanner->weight, scanner->level, scanner->next_states);

        switch(scanner->attribute) {
            case NOTUSED : printf("NOTUSED\n");
                           break;
            case PRUNED  : printf("PRUNED-\n");
                           break;
            case USED    : printf("USED---\n");
                           break;
            case TRIED   : printf("TRIED--\n");
			   break;
            default      : printf("illegal: %d\n", scanner->attribute);
        }

        /*switch(scanner->odd_type) {
            case ADMITTED     : printf("ADMITTED ");
                                break;
            case NOTADMITTED  : printf("-------- ");
                                break;
            case EXAMINED     : printf("EXAMINED ");
                                break;
            default           : printf("illegal: %d", scanner->odd_type);
        }

        switch(scanner->source_type) {
            case INPUT   : printf("INPUT-\n");
                           break;
            case OUTPUT  : printf("OUTPUT\n");
                           break;
            case MIXED   : printf("MIXED-\n");
                           break;
            default      : printf("illegal: %d\n", scanner->source_type);
        }*/

    }

}





/*****************************************************************************
*                             Shows the codes                                *
*                                                                            *
*****************************************************************************/


show_code(symblemes,num)
SYMBLEME *symblemes;
int num;

{
    int i;

    for (i = 0; i < num; i++) {
	printf(".code %s", symblemes[i].name);
	if (symblemes[i].code_status == FIXED || I_ANNEAL == TRUE) {
		printf(" %s\n", symblemes[i].code);
	}
    }

}



show_exactcode(symblemes,num)
SYMBLEME *symblemes;
int num;

{

    int i;

    for (i = 0; i < num; i++) {
	printf(".code %s", symblemes[i].name);
        printf("  %s\n", symblemes[i].exact_code);
    }

}



show_bestcode(symblemes,num)
SYMBLEME *symblemes;
int num;

{

    int i;

    for (i = 0; i < num; i++) {
        printf(".code %s", symblemes[i].name);
	printf(" %s\n", symblemes[i].best_code);
    }

}



show_exbestcode(symblemes,num)
SYMBLEME *symblemes;
int num;

{

    int i;

    for (i = 0; i < num; i++) {
        printf(".code %s", symblemes[i].name);
	printf(" %s\n", symblemes[i].exbest_code);
    }

}



/*******************************************************************************
*       Shows the graph of the constraints (built for the exact algorithm)     *
*******************************************************************************/

show_graph()

{

    int i;
    CONSTRAINT_E *showptr;

    for (i = graph_depth-1; i >= 0; i--) {
	printf("\nlevel of the graph = %d\n", i);
	for (showptr = graph_levels[i]; showptr != (CONSTRAINT_E *) 0;
				         showptr = showptr->next ) {
            printf("%s  ", showptr->relation);
        }
    }
    printf("\n");

}



/*****************************************************************************
*                   Shows the sons of all constraints                        *
*****************************************************************************/

show_sons()

{

    CONSTRAINT_E *upconstr;
    SONS_LINK *son_scanner;
    int i;

    for (i = graph_depth-1; i >= 0; i--) {
	for (upconstr = graph_levels[i]; upconstr != (CONSTRAINT_E *) 0;
				         upconstr = upconstr->next ) {
            printf("The sons of constraint %s are :\n", upconstr->relation);
	    for (son_scanner = upconstr->down_ptr; son_scanner != (SONS_LINK *) 0;
				             son_scanner = son_scanner->next ) {
                printf("%s \n", son_scanner->constraint_e->relation);
                /*printf("%s \n", son_scanner->codfather_ptr->relation);*/
            }
        }    
    }
    
}

	

/*****************************************************************************
*                   Shows the fathers of all constraints                     *
*****************************************************************************/

show_fathers()

{

    CONSTRAINT_E *downconstr;
    FATHERS_LINK *father_scanner;
    int i;

    for (i = graph_depth-1; i >= 0; i--) {
	for (downconstr = graph_levels[i]; downconstr != (CONSTRAINT_E *) 0;
				         downconstr = downconstr->next ) {
            printf("The fathers of constraint %s are :\n", downconstr->relation);
	    for (father_scanner = downconstr->up_ptr; father_scanner != (FATHERS_LINK *) 0;
				            father_scanner = father_scanner->next ) {
                printf("%s \n", father_scanner->constraint_e->relation);
            }
        }    
    }
    
}


/*****************************************************************************
*              Shows the faces assigned to the constraints                   *
*****************************************************************************/

show_faces()

{

    CONSTRAINT_E *constrptr;
    int i;

    printf("\nFINAL CODES\n");
    for (i = graph_depth-1; i >= 0; i--) {
	for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
				         constrptr = constrptr->next ) {
            if ( constrptr->face_ptr != (FACE *) 0 ) {
		printf("constraint = %s ", constrptr->relation);
                printf("face = ");
                /*if (constrptr->face_ptr->code_valid == TRUE) {*/
                    printf(" %s ", constrptr->face_ptr->cur_value);
                /*}*/
                printf("tried_codes = %d \n", constrptr->face_ptr->tried_codes);
	    }
        }    
    }
    
}



/*****************************************************************************
*         Shows the dimensions of the faces assigned to the constraints      *
*****************************************************************************/

show_dimfaces()

{

    CONSTRAINT_E *constrptr;
    int i;

    for (i = graph_depth-1; i >= 0; i--) {
	for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
				                 constrptr = constrptr->next ) {
            if (constrptr->face_ptr->category == 0) {
		printf("constraint %s is of cat. %d ",constrptr->relation,
				                 constrptr->face_ptr->category);
                printf("curdim_face = %d\n", constrptr->face_ptr->curdim_face);
            }
            if (constrptr->face_ptr->category == 1) {
		printf("constraint %s is of cat. %d ",constrptr->relation,
				                 constrptr->face_ptr->category);
                printf("mindim_face = %d ", constrptr->face_ptr->mindim_face);
                printf("maxdim_face = %d\n", constrptr->face_ptr->maxdim_face);
            }
            if (constrptr->face_ptr->category == 2) {
		printf("constraint %s is of cat. %d ",constrptr->relation,
				                 constrptr->face_ptr->category);
                printf("mindim_face = %d\n", constrptr->face_ptr->mindim_face);
            }
            if (constrptr->face_ptr->category == 3) {
		printf("constraint %s is of cat. %d ",constrptr->relation,
				                 constrptr->face_ptr->category);
                printf("mindim_face = %d\n", constrptr->face_ptr->mindim_face);
            }
        }
    }

}



show_implicant()

{

    IMPLICANT *scanner;

    printf("\nImplicants produced by symbolic minimization\n");
    for (scanner = implicant_list; scanner != (IMPLICANT *) 0; scanner = scanner->next) {
        printf("State = %d ", scanner->state);
        switch(scanner->attribute) {
            case NOTUSED : printf("NOTUSED ");
                           break;
            case PRUNED  : printf("PRUNED- ");
                           break;
            case USED    : printf("USED--- ");
                           break;
            default      : printf("illegal: %d", scanner->attribute);
        }
        printf("%s", scanner->row);
   }

}



show_orderarray(net_num)
int net_num;

{

    int i,j;

    printf("\nOrder_array\n");
    for (i = 0; i < net_num; i++) {
	for (j = 0; j < net_num; j++) printf("%c", order_array[i][j]);
        switch(select_array[i]) {
            case NOTUSED : printf(" NOTUSED\n");
                           break;
            case PRUNED  : printf(" PRUNED-\n");
                           break;
            case USED    : printf(" USED---\n");
                           break;
            default      : printf(" illegal: %d\n", select_array[i]);
        }
   }

}
