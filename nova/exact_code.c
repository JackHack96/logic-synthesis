
/******************************************************************************
*                       Exact encoding algorithm                              *
*                                                                             *
******************************************************************************/


#include "inc/nova.h"

exact_code(net_num)
int net_num;

{

int dim_cube, least_cube;
BOOLEAN code_found, outer_loop, init_backtrack_up(), backtrack_up(),
        backtrack_down();

if (VERBOSE) { printf("\n\nExact_code\n"); printf("==========\n"); }

faces_alloc(net_num);

code_found = FALSE;
least_cube = least_dimcube(net_num);
if (VERBOSE) printf("least_cube = %d\n", least_cube);

/* decides the categories and bounds of the faces (func. of dim_cube) -
   the dimension of the hypercube (it might be increased) is returned */
least_cube = faces_dim_set(least_cube);

for (
dim_cube = least_cube;
dim_cube <=
net_num;
dim_cube++) {
if (VERBOSE) printf("\nCURRENT dim_cube = %d\n", dim_cube);

/* finds a starting feasible configuration of the dimensions of faces */
outer_loop = init_backtrack_up();

while (code_found == FALSE && outer_loop == TRUE) {

/* finds an encoding (if any) for the current configuration */
code_found = backtrack_down(net_num, dim_cube);

/*if no code was found sets a new feasible configuration (if any) */
if (code_found == FALSE) {
outer_loop = backtrack_up();
}

}

cube_summary();

if (code_found == TRUE) {
if (VERBOSE)
printf("\nA code was found for dim_cube = %d\n", dim_cube);
break;
}

/* maxdim of constraints of category 0 & 1 are updated because
   dim_cube is increased by 1 in the coming iteration         */
faces_maxdim_set(dim_cube
+ 1);

}

if (code_found == FALSE) {
/*something went wrong*/
if (VERBOSE)
printf("SOMETHING WENT WRONG : no code found in the maximum cube\n");
exit(-1);
}

exact_summary();

}


/*******************************************************************************
* allocates memory for the faces and connects them to the graph_levels array   *
*******************************************************************************/

faces_alloc(net_num)
int net_num;

{

FACE         *new_face;
CONSTRAINT_E *constrptr_e;
int          i;

/* total_work (global variable) counts the # of codes tried in overall
   by exact_code */
total_work  = 0;
/* bktup_calls (global variable) counts the # of face configurations
   tried in all */
bktup_calls = 1;

/* set to true to start next_to_code on constraints of category 1 */
SEL_CAT1 = TRUE;

/* it allocates the FACE data structure */
for (
i = graph_depth - 1;
i >= 0; i--) {
for (
constrptr_e = graph_levels[i];
constrptr_e != (CONSTRAINT_E *) 0;
constrptr_e = constrptr_e->next
) {
if ((
new_face = (FACE *) calloc((unsigned) 1, sizeof(FACE))
) == ( FACE *) 0) {
fprintf(stderr,
"Insufficient memory for face in faces_alloc");
exit(-1);
}
constrptr_e->
face_ptr = new_face;

if ((constrptr_e->face_ptr->
seed_value = (char *) calloc((unsigned) net_num + 1, sizeof(char))
) == ( char *) 0) {
fprintf(stderr,
"Insufficient memory for seed_value in faces_alloc");
exit(-1);
}
if ((constrptr_e->face_ptr->
first_value = (char *) calloc((unsigned) net_num + 1, sizeof(char))
) == ( char *) 0) {
fprintf(stderr,
"Insufficient memory for first_value in faces_alloc");
exit(-1);
}
if ((constrptr_e->face_ptr->
cur_value = (char *) calloc((unsigned) net_num + 1, sizeof(char))
) == ( char *) 0) {
fprintf(stderr,
"Insufficient memory for cur_value in faces_alloc");
exit(-1);
}

constrptr_e->face_ptr->
code_valid = FALSE;
constrptr_e->face_ptr->
tried_codes = 1;
}
}

}



/*******************************************************************************
*      Sets the categories & mindim,curdim and maxdim of the faces that can    *
*      be assigned to the constraints (function of dim_cube) -                 *
*      it returns the dimension of the hypercube : it may be increased         *
*      with respect to its input value as a consequence of the fact that       *
*      the dimensions of the faces of some constraints may need to be          *
*      increased to be able to contain other constraints at the same level     *
*      (i.e. sons at the same level)                                           *
*******************************************************************************/

int faces_dim_set(dim_cube)
        int dim_cube;

{

    FATHERS_LINK *ft_scanner;
    CONSTRAINT_E *constrptr, *supremum_ptr;
    int          i, fathers_num, new_dimcube;

    /* cube_work (global variable) counts the # of codes tried in the current 
       cube */
    cube_work = 0;

    /*                        PART A
	The constraints are classified in categories :
	1) the complete constraint is of category 0;
	2) constraints whose unique father is the complete constraint are
	   of category 1;
	3) constraints whose unique father is not the complete constraint 
	   are of category 3;
        4) constraints with at least two fathers are of category 2 -

	The dimensions of the faces of the constraints are set on the basis
	of the minimum feasible dimension and of current hypercube dimension */

    if (VERBOSE) printf("\nFaces_dim_set - Part A :\n");

    /* takes care of the complete constraint */
    supremum_ptr = graph_levels[graph_depth - 1];
    supremum_ptr->face_ptr->category = 0;
    if (VERBOSE)
        printf("constraint %s is of cat. %d ", supremum_ptr->relation,
               supremum_ptr->face_ptr->category);
    supremum_ptr->face_ptr->mindim_face = dim_cube;
    supremum_ptr->face_ptr->maxdim_face = dim_cube;
    supremum_ptr->face_ptr->curdim_face = dim_cube;
    if (VERBOSE)
        printf("curdim_face = %d\n", supremum_ptr->face_ptr->curdim_face);

    for (i = graph_depth - 2; i >= 0; i--) {
        for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
             constrptr = constrptr->next) {

            fathers_num     = 0;
            for (ft_scanner = constrptr->up_ptr;
                 ft_scanner != (FATHERS_LINK *) 0; ft_scanner = ft_scanner->next) {
                fathers_num++;
            }

            /* if the constraint has only one father */
            if (fathers_num == 1) {
                /* if the father is the complete constraint */
                if (constrptr->up_ptr->constraint_e == supremum_ptr) {
                    constrptr->face_ptr->category = 1;
                    if (VERBOSE)
                        printf("constraint %s is of cat. %d ", constrptr->relation,
                               constrptr->face_ptr->category);
                    if (constrptr->card == 1) {
                        /* the dimension of faces of constraints of category 1 
                           and cardinality 1 is nailed down to 0 */
                        constrptr->face_ptr->mindim_face =
                                mylog2(minpow2(constrptr->card));
                        if (VERBOSE)
                            printf("mindim_face = %d ",
                                   constrptr->face_ptr->mindim_face);
                        constrptr->face_ptr->maxdim_face =
                                constrptr->face_ptr->mindim_face;
                        if (VERBOSE)
                            printf("maxdim_face = %d \n",
                                   constrptr->face_ptr->maxdim_face);
                    } else {
                        constrptr->face_ptr->mindim_face =
                                mylog2(minpow2(constrptr->card));
                        if (VERBOSE)
                            printf("mindim_face = %d ",
                                   constrptr->face_ptr->mindim_face);
                        constrptr->face_ptr->maxdim_face = dim_cube - 1;
                        if (VERBOSE)
                            printf("maxdim_face = %d \n",
                                   constrptr->face_ptr->maxdim_face);
                    }
                } else {
                    constrptr->face_ptr->category = 3;
                    if (VERBOSE)
                        printf("constraint %s is of cat. %d ", constrptr->relation,
                               constrptr->face_ptr->category);
                    constrptr->face_ptr->mindim_face =
                            mylog2(minpow2(constrptr->card));
                    constrptr->face_ptr->curdim_face =
                            constrptr->face_ptr->mindim_face;
                    if (VERBOSE)
                        printf("mindim_face = %d\n",
                               constrptr->face_ptr->mindim_face);
                }
            }

            /* if the constraint has at least two fathers */
            if (fathers_num > 1) {
                constrptr->face_ptr->category = 2;
                if (VERBOSE)
                    printf("constraint %s is of cat. %d ", constrptr->relation,
                           constrptr->face_ptr->category);
                constrptr->face_ptr->mindim_face =
                        mylog2(minpow2(constrptr->card));
                constrptr->face_ptr->curdim_face =
                        constrptr->face_ptr->mindim_face;
                if (VERBOSE)
                    printf("mindim_face = %d\n",
                           constrptr->face_ptr->mindim_face);
            }

        }
    }


    /*                        PART B
       mindim_face & curdim_face of all constraints are recomputed by 
       max_tree (called on the complete constraint) -
       it must be done because the dimensions of the faces of some
       constraints may need to be increased to be able to contain other 
       constraints at the same level (i.e. sons at the same level) -
       this process may end up increasing the dimension of the complete
       constraint, i.e. increasing the dimension of the hypercube -
       this plays the role of another necessary condition (besides those
       implemented in least_dimcube) that needs to be satisfied by the
       hypercube dimension to allow a feasible embedding               */
    new_dimcube = max_tree(graph_levels[graph_depth - 1]);

    /* FORZATURA DELLA DIMENSIONE PER PROVA */
    /*new_dimcube = 6;*/
    /* FORZATURA DELLA DIMENSIONE PER PROVA - FINE */

    /* if the hypercube dimension has been increased, also maxdim_face of
       constraints of category 0 & 1 need to be updated                   */
    if (new_dimcube > dim_cube) {
        if (VERBOSE)printf("\nThe cube dimension went up to %d\n", new_dimcube);
        faces_maxdim_set(new_dimcube);
    }

    if (VERBOSE) {
        printf("\nFaces_dim_set - Part B :\n");
        show_dimfaces();
    }

    /* FORZATURA DELLE FACCE PER PROVA */
    /*new_dimcube = 6;
    if (VERBOSE) printf("\nThe cube dimension went up to %d\n", new_dimcube);
    for (i = graph_depth-1; i >= 0; i--) {
	for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
				                 constrptr = constrptr->next ) {
            if (strcmp(constrptr->relation,"11111111") == 0) {
                constrptr->face_ptr->mindim_face = 6;
                constrptr->face_ptr->curdim_face = 6;
                constrptr->face_ptr->maxdim_face = 6;
            }
            if (strcmp(constrptr->relation,"01001010") == 0) {
                constrptr->face_ptr->mindim_face = 4;
                constrptr->face_ptr->maxdim_face = 4;
            }
            if (strcmp(constrptr->relation,"00110010") == 0) {
                constrptr->face_ptr->mindim_face = 3;
                constrptr->face_ptr->maxdim_face = 3;
            }
            if (strcmp(constrptr->relation,"01001100") == 0) {
                constrptr->face_ptr->mindim_face = 4;
                constrptr->face_ptr->maxdim_face = 4;
            }
            if (strcmp(constrptr->relation,"00110100") == 0) {
                constrptr->face_ptr->mindim_face = 4;
                constrptr->face_ptr->maxdim_face = 4;
            }
            if (strcmp(constrptr->relation,"11010100") == 0) {
                constrptr->face_ptr->mindim_face = 4;
                constrptr->face_ptr->maxdim_face = 4;
            }
            if (strcmp(constrptr->relation,"10101010") == 0) {
                constrptr->face_ptr->mindim_face = 5;
                constrptr->face_ptr->maxdim_face = 5;
            }
            if (strcmp(constrptr->relation,"00000110") == 0) {
                constrptr->face_ptr->mindim_face = 3;
                constrptr->face_ptr->maxdim_face = 3;
            }
        }
    }
    if (VERBOSE) printf("\nFaces_dim_set - Part C :\n");
    show_dimfaces();*/
    /* FORZATURA DELLE FACCE PER PROVA -  FINE */

    return (new_dimcube);

}



/*******************************************************************************
* recursive procedure to compute the mindim_face of root_ptr to accomodate the *
* the subdag rooted here -                                                     *
* if mindim_face of root_ptr is bigger than the minimum dimension of the       *
* subgraph rooted here return mindim_face of root_ptr (i.e., it can accomodate *
* the subgraph rooted here),                                                   *
* otherwise update mindim_face & curdim_face of face_ptr to the minimum        *
* dimension of the subgraph + 1 and return it (i.e., it needs a dimension      *
* larger by one to accomodate the subgraph rooted here)                        *
*******************************************************************************/

int max_tree(root_ptr)
        CONSTRAINT_E *root_ptr;

{

    SONS_LINK *son_scanner;
    int       local_max;
    int       parz_max;

    /*printf("max_tree called by %s\n", root_ptr->relation);*/

    if (root_ptr->down_ptr == (SONS_LINK *) 0) {
        /*printf("max_tree evaluates to %d\n", root_ptr->face_ptr->mindim_face);*/
        return (root_ptr->face_ptr->mindim_face);
    } else {
        local_max        = -1;
        for (son_scanner = root_ptr->down_ptr; son_scanner != (SONS_LINK *) 0;
             son_scanner = son_scanner->next) {
            /* computes the max of max_tree of the sons of root_ptr */
            parz_max = max_tree(son_scanner->constraint_e);
            if (local_max < parz_max) {
                local_max = parz_max;
            }
        }
        /*printf("local_max = %d\n", local_max);*/
        /* if mindim_face of root_ptr is bigger than the minimum dimension of
           the subgraph rooted here return mindim_face of root_ptr (i.e., it
           can accomodate the subgraph rooted here),
           otherwise update mindim_face & curdim_face of face_ptr to the
           minimum dimension of the subgraph + 1 and return it (i.e., it
           needs a dimension larger by one to accomodate the subgraph rooted
           here) */
        if (root_ptr->face_ptr->mindim_face >
            (local_max)) {
            /*printf("max_tree evaluates to %d\n", 
		                             root_ptr->face_ptr->mindim_face);*/
            return (root_ptr->face_ptr->mindim_face);
        } else {
            root_ptr->face_ptr->mindim_face =
                    local_max + 1;
            root_ptr->face_ptr->curdim_face =
                    local_max + 1;
            /*printf("max_tree evaluates to %d\n", local_max +1);*/
            return (local_max + 1);
        }
    }
}


/*******************************************************************************
*       updates the maxdim of the faces of constraints of category 0 & 1       *
*       (function of dim_cube)                                                 *
*******************************************************************************/

faces_maxdim_set(dim_cube)
int dim_cube;

{

CONSTRAINT_E *constrptr;
int          i;

/* cube_work (global variable) counts the # of codes tried in the current
   cube */
cube_work = 0;

/*printf("\nFace_maxdim_set results :\n");*/

/* maxdim of constraints of category 0 & 1 are updated (only for constraints
   of category 0 & 1 maxdim matters at the upper level) */
for (
i = graph_depth - 1;
i >= 0; i--) {
for (
constrptr = graph_levels[i];
constrptr != (CONSTRAINT_E *) 0;
constrptr = constrptr->next
) {
if (constrptr->face_ptr->category == 0) {
/*printf("constraint %s is of cat. %d ",constrptr->relation,
                   constrptr->face_ptr->category);*/
constrptr->face_ptr->
mindim_face = dim_cube;
constrptr->face_ptr->
maxdim_face = dim_cube;
constrptr->face_ptr->
curdim_face = dim_cube;
/*printf("curdim_face = %d\n",
        constrptr->face_ptr->curdim_face);*/
}
if (constrptr->face_ptr->category == 1 && constrptr->card != 1) {
/*printf("constraint %s is of cat. %d ",constrptr->relation,
                   constrptr->face_ptr->category);*/
constrptr->face_ptr->
maxdim_face = dim_cube - 1;
/*printf("maxdim_face = %d \n",
        constrptr->face_ptr->maxdim_face);*/
}
}
}

}


/*******************************************************************************
*              prints a summary of the work done in the current cube           *
*                                                                              *
*  cube_work counts the # of codes tried in the current cube                   *
*******************************************************************************/

cube_summary() {

    if (VERBOSE) {
        printf("\nCUBE_SUMMARY : \n");
        printf("cube_work = %d\n", cube_work);
    }

}


/*******************************************************************************
*                      prints a summary of exact_code                          *
*                                                                              *
*  bktup_calls counts the # of face configurations tried in all                *
*  total_work  counts the # of codes tried in overall by exact_code            *
*******************************************************************************/

exact_summary() {

    if (VERBOSE) {
        printf("\nEXACT_SUMMARY : \n");
        printf("bktup_calls = %d\n", bktup_calls);
        printf("total_work = %d\n", total_work);
    }

}
