/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/comb_objects.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:08 $
 *
 */
#include "inc/nova.h"

/*****************************************************************
*                                                                *
*     Generation of a new face available for a constraint        *
*     There are three different variations of gen_newcode        *
*     according to the category of the constraint encoded        *
*                                                                *
*****************************************************************/

/*****************************************************************
*   gen_newcode for constraints of category 1 :                  *
*   constraints of category 1 have the universe as only father   *
*****************************************************************/

char *gen_newcode_cat1(curptr,seed_code,dim_cube)
CODORDER_LINK *curptr;
char *seed_code;
int dim_cube;

{

    int i;
    int num_x;
    int count_max,comb_max,lex_max;
    char *new_code,*combinations(),*counting();

    /*printf("\nGen_newcode_cat1 :");*/

    num_x = 0;
    for (i=0; i<dim_cube; i++) {
	if (seed_code[i] == 'x') {
            num_x++;
        }
    }

    /* maximum of counting */
    count_max = power(2,dim_cube - num_x); 
    /* maximum of combinations */
    comb_max = comb_num(dim_cube,num_x);
    /* maximum of transformations (new faces) */
    lex_max = count_max * comb_max;

    if (DEBUG) printf("\nlex_max = %d\n", lex_max);

    /* if it is the first attempt to assign a face to the current constraint
       start from a suitable seed_code (modification of a neighboring code)
       and generate a new code calling 'combinations'                        */
    if ( curptr->constraint_e->face_ptr->lexmap_index == 0 ) {
	new_code = combinations(seed_code,dim_cube);
        /*printf("gen_newcode_cat1 returned = %s\n", new_code);*/
        strcpy(curptr->constraint_e->face_ptr->first_value,new_code);
        curptr->constraint_e->face_ptr->first_valid = TRUE;
        strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
        curptr->constraint_e->face_ptr->code_valid = TRUE;
        curptr->constraint_e->face_ptr->count_index = 1;
        curptr->constraint_e->face_ptr->comb_index = 1;
        curptr->constraint_e->face_ptr->lexmap_index = 1;
        (curptr->constraint_e->face_ptr->tried_codes)++;
    } else {
	/* if all possible faces have already been assigned in vain to the
	   current constraint return the null code                           */
        if (curptr->constraint_e->face_ptr->lexmap_index == lex_max) {
	    return( (char *) 0 );
	} else {
	    /* otherwise apply to the current face the transformation
	       1) 'counting' if the current counting cycle is unfinished
	       2) 'combinations' if the current counting cycle is finished
		  and there are still combinations to try                     */
            if (curptr->constraint_e->face_ptr->count_index < count_max) {
	        new_code = counting(curptr->constraint_e->face_ptr->cur_value,
								      dim_cube);
                /*printf("gen_newcode_cat1 returned = %s\n", new_code);*/
                (curptr->constraint_e->face_ptr->count_index)++;
                (curptr->constraint_e->face_ptr->lexmap_index)++;
                (curptr->constraint_e->face_ptr->tried_codes)++;
                strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
                curptr->constraint_e->face_ptr->code_valid = TRUE;
	    } else {
                if (curptr->constraint_e->face_ptr->comb_index < comb_max) {
	            new_code = 
		     combinations(curptr->constraint_e->face_ptr->cur_value,
								      dim_cube);
                    /*printf("gen_newcode_cat1 returned = %s\n", new_code);*/
                    curptr->constraint_e->face_ptr->count_index = 1;
                    (curptr->constraint_e->face_ptr->comb_index)++;
                    (curptr->constraint_e->face_ptr->lexmap_index)++;
                    (curptr->constraint_e->face_ptr->tried_codes)++;
                    strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
                    curptr->constraint_e->face_ptr->code_valid = TRUE;
		}
	    }
	}
    }

    if (DEBUG) printf("lexmap_index = %d\n", curptr->constraint_e->face_ptr->lexmap_index);

    free_mem(new_code);
    return(curptr->constraint_e->face_ptr->cur_value); 

}



/*****************************************************************************
*   gen_newcode for constraints of category 2 :                              *
*   constraints of category 2 should be assigned faces contained in the      *
*   intersection of those already assigned to their fathers -                *
*   the dimension of each face varies from a minimum (face_ptr->curdim_face) *
*   to a maximum (dimension of the intersection of the faces assigned to the *
*   fathers) and this is taken care by the upper level routine               *
*   (gen_newcode_cat2), while at the lower level (newcode_cat2) all possible *
*   faces of a certain (current) dimension are generated                     *
*****************************************************************************/

char *gen_newcode_cat2(curptr,dim_cube)
CODORDER_LINK *curptr;
int dim_cube;

{

    char *newcode_cat2(),*new_code,*fathers_intercode,*fathers_codes_inter();
    int i,num_x,dim_subcube;

    /* num_x is the face dimension of curptr */
    num_x = curptr->constraint_e->face_ptr->curdim_face;

    /* the variables fathers_intercode and dim_subcube should be
       computed once for all at a (more) global level - for reasons of
       easy coding they are recomputed at every call of gen_newcode_cat2 */
    /* intersection of the faces already assigned to the fathers */
    fathers_intercode = fathers_codes_inter(curptr->constraint_e,dim_cube);
    /* dim_subcube is the face dimension of the intersection of the codes of
       the fathers of curptr */
    dim_subcube = 0;
    for (i=0; i<dim_cube; i++) {
	if (fathers_intercode[i] == 'x') {
            dim_subcube++;
        }
    }

    /* at the first call, dim_index is set to the current face dimension */
    if ( curptr->constraint_e->face_ptr->dim_index == 0 ) {
        curptr->constraint_e->face_ptr->dim_index = num_x;
        i = curptr->constraint_e->face_ptr->dim_index;
        new_code = 
                  newcode_cat2(curptr,dim_cube,i,dim_subcube,fathers_intercode);
    } else {
        /* a new code is generated with the current dim_index */
        i = curptr->constraint_e->face_ptr->dim_index;
        new_code = 
                  newcode_cat2(curptr,dim_cube,i,dim_subcube,fathers_intercode);
        if ( new_code == (char *) 0  && 
             curptr->constraint_e->face_ptr->dim_index < dim_subcube ) {
            /* all codes of the current dim_index have been generated -
               dim_index is set to the next value (if it is still within 
               dim_subcube) and a new code is generated -
               notice that the counting and combinations indexes are
               reset because a new generation cycle starts */
            (curptr->constraint_e->face_ptr->dim_index)++;
            curptr->constraint_e->face_ptr->count_index = 0;
            curptr->constraint_e->face_ptr->comb_index = 0;
            curptr->constraint_e->face_ptr->lexmap_index = 0;
            i = curptr->constraint_e->face_ptr->dim_index;
            if (DEBUG) printf("\ni = %d\n", i);
            new_code = 
                  newcode_cat2(curptr,dim_cube,i,dim_subcube,fathers_intercode);
        }
    }

    free_mem(fathers_intercode);

    return(new_code);    

}


char *newcode_cat2(curptr,dim_cube,num_x,dim_subcube,fathers_intercode) 
CODORDER_LINK *curptr;
int dim_cube;
int num_x;               /* num_x is the current face dimension of curptr */
int dim_subcube;         /* dim_subcube is the face dimension of the 
                           intersection of the codes of the fathers of curptr */
char *fathers_intercode; /*=fathers_codes_inter(curptr->constraint_e,dim_cube)*/

{

    int count_max,comb_max,lex_max;
    char *new_code,*new_subcode,*seed,*start_seed(),
	 *insert_code(),*copy_code(),*combinations(),*counting();

    /*printf("\nGen_newcode_cat2 :");*/

    /* if the intersection of the faces already assigned to the fathers is
       empty return the empty face because a mistake has been made in the
       past and is worthless attempting to assign meaningful faces to the son */
    if (fathers_intercode == (char *) 0) {
	return( (char *) 0);
    }

    /* maximum of counting */
    count_max = power(2,dim_subcube - num_x); 
    /* maximum of combinations */
    comb_max = comb_num(dim_subcube,num_x);
    /* maximum of transformations (new faces) */
    lex_max = count_max * comb_max;

    if (DEBUG) printf("\nlex_max = %d\n", lex_max);

    /* if it is the first attempt to assign a face to the current constraint
       start from start_seed (with a subface dimension) and generate a new 
       subcode calling 'combinations'.
       The subcode is 
       1) merged into the code of the father, and 
       2) assigned to seed_value to be the starting point of the next 
       combinatorial transformation                                           */
    if ( curptr->constraint_e->face_ptr->lexmap_index == 0 ) {
	if (dim_subcube > 0) {
	  seed = start_seed(dim_subcube,num_x);
	  new_subcode = combinations(seed,dim_subcube);
	  free_mem(seed);
	  new_code = insert_code(new_subcode,fathers_intercode,dim_cube);
        } else { /* fathers_intercode is already the only possible new_code */
	    new_code = copy_code(fathers_intercode,dim_cube);
	    new_subcode = copy_code(fathers_intercode,dim_cube);
        }
        /*printf("gen_newcode_cat2 returned = %s\n", new_code);*/
        strcpy(curptr->constraint_e->face_ptr->seed_value,new_subcode);
        curptr->constraint_e->face_ptr->seed_valid = TRUE;
        strcpy(curptr->constraint_e->face_ptr->first_value,new_code);
        curptr->constraint_e->face_ptr->first_valid = TRUE;
        strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
        curptr->constraint_e->face_ptr->code_valid = TRUE;
        curptr->constraint_e->face_ptr->count_index = 1;
        curptr->constraint_e->face_ptr->comb_index = 1;
        curptr->constraint_e->face_ptr->lexmap_index = 1;
        (curptr->constraint_e->face_ptr->tried_codes)++;
    } else {
	/* if all possible faces have already been assigned in vain to the
	   current constraint return the null code                           */
        if (curptr->constraint_e->face_ptr->lexmap_index == lex_max) {
	    return( (char *) 0);
	} else {
	    /* otherwise apply to the current face the transformation
	       1) 'counting' if the current counting cycle is unfinished
	       2) 'combinations' if the current counting cycle is finished
		  and there are still combinations to try                     */
            if (curptr->constraint_e->face_ptr->count_index < count_max) {
	        new_subcode = counting(
			curptr->constraint_e->face_ptr->seed_value,dim_subcube);
	        new_code = insert_code(new_subcode,fathers_intercode,dim_cube);
                /*printf("gen_newcode_cat2 returned = %s\n", new_code);*/
                (curptr->constraint_e->face_ptr->count_index)++;
                (curptr->constraint_e->face_ptr->lexmap_index)++;
                (curptr->constraint_e->face_ptr->tried_codes)++;
                strcpy(curptr->constraint_e->face_ptr->seed_value,new_subcode);
                curptr->constraint_e->face_ptr->seed_valid = TRUE;
                strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
                curptr->constraint_e->face_ptr->code_valid = TRUE;
	    } else {
                if (curptr->constraint_e->face_ptr->comb_index < comb_max) {
		    new_subcode = combinations(
	                curptr->constraint_e->face_ptr->seed_value,dim_subcube);
	            new_code = insert_code(new_subcode,fathers_intercode,
								      dim_cube);
                    /*printf("gen_newcode_cat2 returned = %s\n", new_code);*/
                    curptr->constraint_e->face_ptr->count_index = 1;
                    (curptr->constraint_e->face_ptr->comb_index)++;
                    (curptr->constraint_e->face_ptr->lexmap_index)++;
                    (curptr->constraint_e->face_ptr->tried_codes)++;
                    strcpy(curptr->constraint_e->face_ptr->seed_value,new_subcode);
                    curptr->constraint_e->face_ptr->seed_valid = TRUE;
                    strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
                    curptr->constraint_e->face_ptr->code_valid = TRUE;
		}
	    }
	}
    }

    if (DEBUG) printf("lexmap_index = %d\n", curptr->constraint_e->face_ptr->lexmap_index);

    free_mem(new_subcode);
    free_mem(new_code);
    return(curptr->constraint_e->face_ptr->cur_value);

}



/*****************************************************************************
*   gen_newcode for constraints of category 3 :                              *
*   constraints of category 3 should be assigned faces contained in that     *
*   already assigned to their unique father -                                *
*   the dimension of each face varies from a minimum (face_ptr->curdim_face) *
*   to a maximum (dimension of the face assigned to its father - 1)          *
*   and this is taken care by the upper level routine (gen_newcode_cat3),    *
*   while at the lower level (newcode_cat3) all possible faces of a certain  *
*   (current) dimension are generated                                        *
*****************************************************************************/

char *gen_newcode_cat3(curptr,dim_cube)
CODORDER_LINK *curptr;
int dim_cube;

{

    char *newcode_cat3(),*new_code,*father_code;
    int i,num_x,dim_subcube;

    /* num_x is the face dimension of curptr */
    num_x = curptr->constraint_e->face_ptr->curdim_face;

    father_code = 
                curptr->constraint_e->up_ptr->constraint_e->face_ptr->cur_value;
    if (DEBUG) printf("\nfather_code = %s", father_code);

    /* the variable dim_subcube should be computed once for all
       at a (more) global level - for reasons of easy coding it 
       is recomputed at every call of gen_newcode_cat3          */
    /* dim_subcube is the face dimension of the father of curptr */
    dim_subcube = 0;
    for (i=0; i<dim_cube; i++) {
	if (father_code[i] == 'x') {
            dim_subcube++;
        }
    }

    /* at the first call, dim_index is set to the current face dimension */
    if ( curptr->constraint_e->face_ptr->dim_index == 0 ) {
        curptr->constraint_e->face_ptr->dim_index = num_x;
        i = curptr->constraint_e->face_ptr->dim_index;
        new_code = 
                  newcode_cat3(curptr,dim_cube,i,dim_subcube,father_code);
    } else {
        /* a new code is generated with the current dim_index */
        i = curptr->constraint_e->face_ptr->dim_index;
        new_code = 
                  newcode_cat3(curptr,dim_cube,i,dim_subcube,father_code);
        if ( new_code == (char *) 0  && 
             curptr->constraint_e->face_ptr->dim_index < (dim_subcube - 1) ) {
            /* all codes of the current dim_index have been generated -
               dim_index is set to the next value (if it is still within 
               dim_subcube) and a new code is generated -
               notice that the counting and combinations indexes are
               reset because a new generation cycle starts */
            (curptr->constraint_e->face_ptr->dim_index)++;
            curptr->constraint_e->face_ptr->count_index = 0;
            curptr->constraint_e->face_ptr->comb_index = 0;
            curptr->constraint_e->face_ptr->lexmap_index = 0;
            i = curptr->constraint_e->face_ptr->dim_index;
            if (DEBUG) printf("\ni = %d\n", i);
            new_code = 
                  newcode_cat3(curptr,dim_cube,i,dim_subcube,father_code);
        }
    }

    return(new_code);    

}



char *newcode_cat3(curptr,dim_cube,num_x,dim_subcube,father_code)
CODORDER_LINK *curptr;
int dim_cube;
int num_x;               /* num_x is the current face dimension of curptr */
int dim_subcube;         /* dim_subcube is the face dimension of the 
                            father of curptr */
char *father_code;       /* face assigned to the father of curptr */

{

    int count_max,comb_max,lex_max;
    char *new_code,*new_subcode,*seed,*start_seed(),
	 *insert_code(),*combinations(),*counting();

    /*printf("\nGen_newcode_cat3 :");*/

    /* maximum of counting */
    count_max = power(2,dim_subcube - num_x); 
    /* maximum of combinations */
    comb_max = comb_num(dim_subcube,num_x);
    /* maximum of transformations (new faces) */
    lex_max = count_max * comb_max;

    if (DEBUG) printf("\nlex_max = %d\n", lex_max);

    /* if it is the first attempt to assign a face to the current constraint
       start from start_seed (with a subface dimension) and generate a new 
       subcode calling 'combinations'.
       The subcode is 
       1) merged into the code of the father, and 
       2) assigned to seed_value to be the starting point of the next 
       combinatorial transformation                                           */
    if ( curptr->constraint_e->face_ptr->lexmap_index == 0 ) {
	seed = start_seed(dim_subcube,num_x);
	new_subcode = combinations(seed,dim_subcube);
	free_mem(seed);
	new_code = insert_code(new_subcode,father_code,dim_cube);
        /*printf("gen_newcode_cat3 returned = %s\n", new_code);*/
        strcpy(curptr->constraint_e->face_ptr->seed_value,new_subcode);
        curptr->constraint_e->face_ptr->seed_valid = TRUE;
        strcpy(curptr->constraint_e->face_ptr->first_value,new_code);
        curptr->constraint_e->face_ptr->first_valid = TRUE;
        strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
        curptr->constraint_e->face_ptr->code_valid = TRUE;
        curptr->constraint_e->face_ptr->count_index = 1;
        curptr->constraint_e->face_ptr->comb_index = 1;
        curptr->constraint_e->face_ptr->lexmap_index = 1;
        (curptr->constraint_e->face_ptr->tried_codes)++;
    } else {
	/* if all possible faces have already been assigned in vain to the
	   current constraint return the null code                           */
        if (curptr->constraint_e->face_ptr->lexmap_index == lex_max) {
	    if (DEBUG) printf("\n");
	    return( (char *) 0 );
	} else {
	    /* otherwise apply to the current face the transformation
	       1) 'counting' if the current counting cycle is unfinished
	       2) 'combinations' if the current counting cycle is finished
		  and there are still combinations to try                     */
            if (curptr->constraint_e->face_ptr->count_index < count_max) {
	        new_subcode = counting(
			curptr->constraint_e->face_ptr->seed_value,dim_subcube);
	        new_code = insert_code(new_subcode,father_code,dim_cube);
                /*printf("gen_newcode_cat3 returned = %s\n", new_code);*/
                (curptr->constraint_e->face_ptr->count_index)++;
                (curptr->constraint_e->face_ptr->lexmap_index)++;
                (curptr->constraint_e->face_ptr->tried_codes)++;
                strcpy(curptr->constraint_e->face_ptr->seed_value,new_subcode);
                curptr->constraint_e->face_ptr->seed_valid = TRUE;
                strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
                curptr->constraint_e->face_ptr->code_valid = TRUE;
	    } else {
                if (curptr->constraint_e->face_ptr->comb_index < comb_max) {
		    new_subcode = combinations(
	                curptr->constraint_e->face_ptr->seed_value,dim_subcube);
	            new_code = insert_code(new_subcode,father_code,dim_cube);
                    /*printf("gen_newcode_cat3 returned = %s\n", new_code);*/
                    curptr->constraint_e->face_ptr->count_index = 1;
                    (curptr->constraint_e->face_ptr->comb_index)++;
                    (curptr->constraint_e->face_ptr->lexmap_index)++;
                    (curptr->constraint_e->face_ptr->tried_codes)++;
                    strcpy(curptr->constraint_e->face_ptr->seed_value,new_subcode);
                    curptr->constraint_e->face_ptr->seed_valid = TRUE;
                    strcpy(curptr->constraint_e->face_ptr->cur_value,new_code);
                    curptr->constraint_e->face_ptr->code_valid = TRUE;
		}
	    }
	}
    }

    if (DEBUG) printf("lexmap_index = %d\n", curptr->constraint_e->face_ptr->lexmap_index);

    free_mem(new_subcode);
    free_mem(new_code);
    return(curptr->constraint_e->face_ptr->cur_value);

}



char *insert_code(new_subcode,father_code,dim_cube)
char *new_subcode;
char *father_code;
int dim_cube;

{

    int i,j;
    char *new_code;

    if ( (new_code = (char *) calloc((unsigned)dim_cube+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for new_code in insert_code");
        exit(-1);
    }

    j=0;
    for (i=0; i<dim_cube; i++) {
        if (father_code[i] != 'x') {
            new_code[i] = father_code[i];
        } else {
            new_code[i] = new_subcode[j];
            j++;
        }
    }
    new_code[dim_cube] = '\0';

    return(new_code);

}



char *copy_code(code,dim_cube)
char *code;
int dim_cube;

{

    int i;
    char *new_code;

    if ( (new_code = (char *) calloc((unsigned)dim_cube+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for new_code in copy_code");
        exit(-1);
    }

    for (i=0; i<dim_cube; i++) {
        new_code[i] = code[i];
    }
    new_code[dim_cube] = '\0';

    return(new_code);

}




/*******************************************************************************
*                                                                              *
*          Routines generating elementary combinatorial objects                *
*                                                                              *
*******************************************************************************/

/* Comb_num(n,k) computes the binomial coefficient (n,k) */

comb_num(n,k)
int n;
int k;

{

    if ( (n == 0) || (k == 0) || (k == n) ) {
	return(1);
    } else {
        return( comb_num(n-1,k) + comb_num(n-1,k-1) );
    }

}



/* generation of combinations (k subsets out of n sets) 
   in lexicographic order                                */

char *combinations(seed_code,dim_cube)
char *seed_code;
int dim_cube;

{
    int i,j,l,num_x;
    int *object;
    char *output;

    /* subsets of size k */
    num_x = 0;
    for (i=0; i<dim_cube; i++) {
	if (seed_code[i] == 'x') {
            num_x++;
        }
    }

    if ( (output = (char *) calloc((unsigned)dim_cube+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for output");
        exit(-1);
    }

    if ( (object = (int *) calloc((unsigned)num_x+1,sizeof(int))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for object");
        exit(-1);
    }

    object[0] = -1;

    j = 1;
    for (i=0; i<dim_cube; i++) {
	if (seed_code[i] == 'x') {
	    object[j] = i+1;
	    j++;
	}
    }

    /*j = 1;

    while (j != 0) {*/

        /*printf("initial object : ");
        for (l=0; l<=num_x; l++) {
	    printf("object[l] = %d ", object[l]);
	}
	printf("\n");*/

        j = num_x;

	while (object[j] == dim_cube-num_x+j) {
	    j = j-1;
	}

	object[j] = object[j]+1;

	for (i=j+1; i<= num_x; i++) {
	    object[i] = object[i-1] + 1;
	}

    /*}*/

    /*printf("transformed object : ");
    for (l=0; l<=num_x; l++) {
        printf("object[l] = %d ", object[l]);
    }
    printf("\n");*/

    output[dim_cube] = '\0';
    for (l=0; l<dim_cube; l++) {
	output[l] = 'a';
    }
    for (l=1; l<=num_x; l++) {
        output[object[l]-1] = 'x';
    }
    for (i=0; i<dim_cube; i++) { if (seed_code[i] != 'x') {
	    for (j=0; j<dim_cube; j++) {
	        if ( output[j] == 'a') {
	            output[j] = seed_code[i];
		    break;
		}
	    }
	}
    }
    /*printf("\nCombinations returned ");
    for (l=0; l<dim_cube; l++) {
        printf(" %c ", output[l]);
    }
    printf("\n");*/

    free_mem(object);

    return(output);

}



/* Counting in base 2 to generate all n-bit strings */

char *counting(seed_code,dim_cube)
char *seed_code;
int dim_cube;

{

    int i,j,num_x; 
    int *binary;
    char *output;

    num_x = 0;
    for (i=0; i<dim_cube; i++) {
	if (seed_code[i] == 'x') {
            num_x++;
        }
    }

    /* counting in base 2 */
    if ( (binary = (int *) calloc((unsigned)dim_cube-num_x+1,sizeof(int))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for binary");
        exit(-1);
    }

    if ( (output = (char *) calloc((unsigned)dim_cube+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for output");
        exit(-1);
    }

    for (i=dim_cube-num_x-1; i>=0; i--) {
        binary[i] = -1;
    }
    binary[dim_cube-num_x] = 0;

    j = dim_cube-num_x-1;
    for (i=0; i<dim_cube; i++) {
	if (seed_code[i] != 'x') {
            binary[j] = seed_code[i] - '0';
	    j--;
        }
    }

    /*while (binary[dim_cube-num_x] != 1) {*/

	/*printf("old binary : ");
        for (i=dim_cube-num_x; i>=0; i--) {
	    printf("binary[i] = %d ", binary[i]);
	}
	printf("\n");*/
	
	i = 0;
	while (binary[i] == 1) {
	    binary[i] = 0;
	    i++;
	}
	binary[i] = 1;

    /*}*/

    /*printf("tranformed binary : ");
    for (i=dim_cube-num_x; i>=0; i--) {
        printf("binary[i] = %d ", binary[i]);
    }
    printf("\n");*/

    output[dim_cube] = '\0';
    for (i=0; i<dim_cube; i++) {
	output[i] = 'a';
    }
    for (i=0; i<dim_cube; i++) {
	if (seed_code[i] == 'x') {
	    output[i] = seed_code[i];
	}
    }
    for (i=dim_cube-num_x-1; i>=0; i--) {
        for (j=0; j<dim_cube; j++) {
	    if (output[j] == 'a') {
		if (binary[i] == 0) {
	            output[j] = ZERO;
                }
		if (binary[i] == 1) {
	            output[j] = ONE;
                }
		break;
	    }
	}
    }
    /*printf("\nCounting returned ");
    for (i=0; i<dim_cube; i++) {
        printf(" %c ", output[i]);
    }
    printf("\n");*/

    free_mem(binary);

    return(output);

}



/*  Generation of combinations (k subsets) in the minimal change order
    derived from the n-bit binary-reflected Gray code, n>=k>0           */

char *min_combinations(seed_code,dim_cube)
char *seed_code;
int dim_cube;

{
    int i,j,l,t,num_x;
    int *buffer,*stack;
    char *output;

    if (DEBUG) printf("\nEntered into min_combinations\n");

    /* compute the size k (= num_x) of the subsets */
    num_x = 0;
    for (i=0; i<dim_cube; i++) {
	if (seed_code[i] == 'x') {
            num_x++;
        }
    }

    if ( (output = (char *) calloc((unsigned)dim_cube+1,sizeof(char))) == (char *) 0) {
        fprintf(stderr,"Insufficient memory for output");
        exit(-1);
    }

    if ( (buffer = (int *) calloc((unsigned)dim_cube+2,sizeof(int))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for object");
        exit(-1);
    }

    if ( (stack = (int *) calloc((unsigned)dim_cube+2,sizeof(int))) == (int *) 0) {
        fprintf(stderr,"Insufficient memory for stack in min_combinations");
        exit(-1);
    }
	
    for (j=1; j<=num_x; j++) {
	buffer[j] = 1;
	stack[j] = j+1;
    }
    for (j=num_x+1; j<=dim_cube+1; j++) {
	buffer[j] = 0;
	stack[j] = j+1;
    }

    t = num_x;
    stack[1] = num_x+1;
    i = 0;

    while (i != (dim_cube+1)) {
        if (DEBUG) printf("New min_combination : ");
        for (l=1; l<=dim_cube; l++) {
	    if (buffer[l] == 1) output[l-1] = '1'; else output[l-1] = '0';
	    if (DEBUG) printf("%d", buffer[l]);
	}
	output[dim_cube] = '\0';
	if (DEBUG) printf("\n");
	i = stack[1];
	stack[1] = stack[i];
	stack[i] = i+1;
	if (buffer[i] == 1) {
	    if (t!=0) {
		buffer[t] = !buffer[t];
	    } else {
		buffer[i-1] = !buffer[i-1];
	    }
	    t = t+1;
	} else {
            if (t!=1) {
		buffer[t-1] = !buffer[t-1];
	    } else {
		buffer[i-1] = !buffer[i-1];
	    }
	    t = t-1;
	}
	buffer[i] = !buffer[i];
	if (t == (i-1) || t == 0) {
	    t = t+1;
	} else {
	    t = t - buffer[i-1];
	    stack[i-1] = stack[1];
	    if (t==0) {
		stack[1] = i-1;
	    } else {
		stack[1] = t+1;
	    }
	}
    }

    free_mem(buffer);
    free_mem(stack);

    return(output);

}
