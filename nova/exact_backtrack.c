#ifndef JEDI_INT_H
#define JEDI_INT_H

#include "inc/nova.h"

/******************************************************************************
 *                         Upper level backtracking                            *
 ******************************************************************************/

/******************************************************************************
 * init_backtrack_up chooses as initial configuration of the dimensions of the *
 * faces the first one in lexicographic order , ( the one with all dimensions  *
 * set to mindim_face )                                                        *
 ******************************************************************************/

BOOLEAN init_backtrack_up() {

    int          i;
    CONSTRAINT_E *constrptr;

    cart_prod   = 1;
    bktup_count = 1;

    if (VERBOSE)
        printf("\nInit_backtrack_up sets curdim of constraints of cat. 1\n");

    for (i = graph_depth - 1; i >= 0; i--) {
        for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
             constrptr = constrptr->next) {
            /* constraints of category 1 get as current face dimension the
               minimum feasible face dimension                                */
            if (constrptr->face_ptr->category == 1) {
                constrptr->face_ptr->curdim_face = constrptr->face_ptr->mindim_face;
                if (VERBOSE) {
                    printf("constraint %s : ", constrptr->relation);
                    printf("new curdim_face = %d\n", constrptr->face_ptr->curdim_face);
                }
                cart_prod *= constrptr->face_ptr->maxdim_face -
                             constrptr->face_ptr->mindim_face + 1;
                if (cart_prod < 0) {
                    fprintf(stderr,
                            "\nSolution space too large: exact search discontinued\n");
                    exit(-1);
                }
            }
        }
    }

    if (VERBOSE)
        printf("\ncart_prod in init_backtrack_up = %d\n", cart_prod);

    return (TRUE);
}

/*******************************************************************************
 *   backtrack_up finds a new configuration of the dimensions of the faces *
 *   according to the lexicographic order introduced in the cartesian * product
 *of the linearly ordered sets of the integer dimensions that        * each
 *constraint of cardinality 1 can assume (cardinality of each such      * set =
 *maxdim_face - mindim_face for that constraint)                       *
 *******************************************************************************/

BOOLEAN backtrack_up() {

    int          i, search_st;
    CONSTRAINT_E *constrptr, *lex_constrptr;

    if (VERBOSE)
        printf("\nBacktrack_up updates curdim of constraints of cat. 1\n");

    /* A new configuration of the dimensions of the faces is generated
      according to the lexicographic order introduced in the cartesian
      product of the linearly ordered sets of the integer dimensions
      that each constraint of cardinality 1 can assume (cardinality of
      each such set = maxdim_face - mindim_face for that constraint) -
      To find the next configuration in lex order we increase by one
      the right-most value that can be increased and set all values
      further to the right to one                                       */

    if (VERBOSE)
        printf("bktup_count = %d\n", bktup_count);

    if (bktup_count == cart_prod) { /* no more configurations available */
        return (FALSE);
    } else {
        /* we find the right_most value that can be increased -
           lex_constrptr points to it                           */
        for (i    = graph_depth - 1; i >= 0; i--) {
            for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
                 constrptr = constrptr->next) {
                if (constrptr->face_ptr->category == 1) {
                    if (constrptr->face_ptr->curdim_face <
                        constrptr->face_ptr->maxdim_face) {
                        lex_constrptr = constrptr;
                    }
                }
            }
        }
        /* we increase by one the right-most value that can be increased
           and set all values further to the right to one                   */
        search_st = 0;
        for (i    = graph_depth - 1; i >= 0; i--) {
            for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
                 constrptr = constrptr->next) {
                if (constrptr->face_ptr->category == 1) {
                    if (search_st == 0) {
                        if (constrptr == lex_constrptr) {
                            constrptr->face_ptr->curdim_face++;
                            search_st = 1;
                        }
                    }
                    if (constrptr != lex_constrptr && search_st == 1) {
                        constrptr->face_ptr->curdim_face = constrptr->face_ptr->mindim_face;
                    }
                }
            }
        }
        bktup_count++;
        /* bktup_calls (global variable) is set to 0 in faces_alloc */
        bktup_calls++;
        for (i = graph_depth - 1; i >= 0; i--) {
            for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
                 constrptr = constrptr->next) {
                if (constrptr->face_ptr->category == 1) {
                    if (VERBOSE) {
                        printf("constraint %s : ", constrptr->relation);
                        printf("new curdim_face = %d\n", constrptr->face_ptr->curdim_face);
                    }
                }
            }
        }
    }

    return (TRUE);
}

/*****************************************************************************
 *                       LOWER LEVEL BACKTRACKING *
 *******************************************************************************/

BOOLEAN backtrack_down(net_num, dim_cube)int net_num;
                                         int dim_cube;

{

    CODORDER_LINK *firstptr, *curptr, *backtrackptr, *first_to_code(),
                  *insert_in_list();
    CONSTRAINT_E  *next_constr, *next_to_code();
    BOOLEAN face_found, backtrack, assign_face(), toomuch_work();

    if (VERBOSE)
        printf("\n      *********   B A C K T R A C K _ D O W N   *********\n");
    /* clears the faces (to start a new encoding attempt) */
    clear_faces();

    /* the complete cube is assigned to the complete constraint */
    code_complete_constr(dim_cube,
                         graph_levels[graph_depth - 1]->face_ptr->curdim_face);

    /* Selects the first constraint to code and inserts it in the selection
       list - firstptr points to the first linker in the order list   */
    firstptr     = first_to_code(dim_cube);
    curptr       = firstptr;
    backtrackptr = firstptr;

    /* assigns a face to the first selected constraint */
    assign_face_tofirst(curptr, dim_cube);

    /* selects a new constraint to code */
    next_constr = next_to_code(curptr->constraint_e, dim_cube);

    while (next_constr != (CONSTRAINT_E *) 0) {

        /* inserts the constraint pointed to by curptr in the selection list and
           returns a pointer to the last element added to the selection list */
        curptr       = insert_in_list(curptr, next_constr);
        backtrackptr = curptr;

        backtrack = FALSE;

        /* assigns a face to the current constraint */
        face_found = assign_face(curptr, dim_cube, net_num);

        while (!(face_found == TRUE && backtrack == FALSE)) {

            if (face_found == FALSE && backtrack == FALSE) {
                if ((I_HYBRID || IO_HYBRID || IO_VARIANT) && (toomuch_work() == TRUE)) {
                    btkdown_summary();
                    return (FALSE);
                }
                /* a backtracking phase starts */
                if (DEBUG)
                    printf("\nA BACKTRACKING PHASE STARTS\n");
                backtrack = TRUE;
                /* the current face information is cancelled */
                undo_face(backtrackptr);
                backtrackptr = backtrackptr->right;
                if (DEBUG)
                    printf("Backwards to %s\n", backtrackptr->constraint_e->relation);
                if (backtrackptr == firstptr) {
                    if (VERBOSE)
                        printf(
                                "\nBacktrack failed : WRONG CONFIGURATION OR CUBE TOO SMALL\n");
                    btkdown_summary();
                    if (DEBUG)
                        show_faces();
                    return (FALSE);
                } else {
                    face_found = assign_face(backtrackptr, dim_cube, net_num);
                }
            }

            if (face_found == FALSE && backtrack == TRUE) {
                if ((I_HYBRID || IO_HYBRID || IO_VARIANT) && (toomuch_work() == TRUE)) {
                    btkdown_summary();
                    return (FALSE);
                }
                /* the current backtracking phase continues */
                if (DEBUG)
                    printf("\nTHE CURRENT BACKTRACKING PHASE CONTINUES\n");
                undo_face(backtrackptr);
                backtrackptr = backtrackptr->right;
                if (DEBUG)
                    printf("Backwards to %s\n", backtrackptr->constraint_e->relation);
                if (backtrackptr == firstptr) {
                    if (VERBOSE)
                        printf(
                                "\nBacktrack failed : WRONG CONFIGURATION OR CUBE TOO SMALL\n");
                    btkdown_summary();
                    if (DEBUG)
                        show_faces();
                    return (FALSE);
                } else {
                    face_found = assign_face(backtrackptr, dim_cube, net_num);
                }
            }

            if (face_found == TRUE && backtrack == TRUE) {
                if ((I_HYBRID || IO_HYBRID || IO_VARIANT) && (toomuch_work() == TRUE)) {
                    btkdown_summary();
                    return (FALSE);
                }
                /* a backtracking phase ends */
                /* we are recovering from the current backtracking phase */
                /* climbing ahead the current backtracking phase */
                if (DEBUG)
                    printf("\nCLIMBING AHEAD THE CURRENT BACKTRACKING PHASE\n");
                backtrackptr = backtrackptr->left;
                if (DEBUG)
                    printf("Forwards to %s\n", backtrackptr->constraint_e->relation);
                face_found = assign_face(backtrackptr, dim_cube, net_num);
                if (backtrackptr == curptr) {
                    backtrack = FALSE;
                }
            }
        }

        /* selects a new constraint to code */
        next_constr = next_to_code(curptr->constraint_e, dim_cube);
    }

    /* summarizes the work performed by backtrack_down */
    if (VERBOSE)
        printf("\nBACKTRACK_DOWN SUCCESSFULLY COMPLETED\n");
    btkdown_summary();
    if (DEBUG)
        show_faces();
    return (TRUE);
}

/* clears the information of the faces (to start a new encoding attempt) */

clear_faces() {

    CONSTRAINT_E *constrptr;
    int          i;

    for (i = graph_depth - 1; i >= 0; i--) {
        for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
             constrptr = constrptr->next) {
            constrptr->face_ptr->seed_valid   = FALSE;
            constrptr->face_ptr->first_valid  = FALSE;
            constrptr->face_ptr->code_valid   = FALSE;
            constrptr->face_ptr->count_index  = 0;
            constrptr->face_ptr->comb_index   = 0;
            constrptr->face_ptr->lexmap_index = 0;
            constrptr->face_ptr->dim_index    = 0;
            constrptr->face_ptr->tried_codes  = 1;
        }
    }
}

/*******************************************************************************
 *    clears the face information of backtrackptr and the other constraints *
 *    that point to it as their codfather *
 *******************************************************************************/

undo_face(backtrackptr)
CODORDER_LINK *backtrackptr;

{

SONS_LINK    *son_scanner;
CONSTRAINT_E *constrptr;

constrptr = backtrackptr->constraint_e;

if (constrptr->face_ptr->category != 1) {
/* constraints of category 1 are assigned a seed_value only once by
   next_to_code, while other constraints are reassigned a seed_value
   by every call of gen_newcode_catx */
constrptr->face_ptr->
seed_valid = FALSE;
}
constrptr->face_ptr->
first_valid = FALSE;
constrptr->face_ptr->
code_valid = FALSE;
constrptr->face_ptr->
count_index = 0;
constrptr->face_ptr->
comb_index = 0;
constrptr->face_ptr->
lexmap_index = 0;
constrptr->face_ptr->
dim_index = 0;

for (
son_scanner = constrptr->down_ptr;
son_scanner != (SONS_LINK *)0;
son_scanner = son_scanner->next
) {
if (son_scanner->codfather_ptr == constrptr) {
if (DEBUG)
printf("cleared son : %s\n", son_scanner->constraint_e->relation);
son_scanner->constraint_e->face_ptr->
seed_valid = FALSE;
son_scanner->constraint_e->face_ptr->
first_valid = FALSE;
son_scanner->constraint_e->face_ptr->
code_valid = FALSE;
son_scanner->constraint_e->face_ptr->
count_index = 0;
son_scanner->constraint_e->face_ptr->
comb_index = 0;
son_scanner->constraint_e->face_ptr->
lexmap_index = 0;
son_scanner->constraint_e->face_ptr->
dim_index = 0;
son_scanner->
codfather_ptr = (CONSTRAINT_E *) 0;
}
}
}

/*******************************************************************************
 * Selects the first constraint to code and inserts it in the selection list *
 * The selected constraint has to be of category 1 with maximum curdim_face *
 *******************************************************************************/

CODORDER_LINK *first_to_code(dim_cube)int dim_cube;

{
    int           i, max_curdim_face;
    char          *seed, *start_seed();
    CONSTRAINT_E  *constrptr, *prov_constrptr;
    CODORDER_LINK *ord_link;

    if ((ord_link = (CODORDER_LINK *) calloc(
            (unsigned) 1, sizeof(CODORDER_LINK))) == (CODORDER_LINK *) 0) {
        fprintf(stderr, "Insufficient memory for ord_link in first_to_code");
        exit(-1);
    }

    max_curdim_face = -1;
    for (i          = graph_depth - 1; i >= 0; i--) {
        for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
             constrptr = constrptr->next) {
            if (constrptr->face_ptr->category == 1 &&
                constrptr->face_ptr->curdim_face > max_curdim_face) {
                max_curdim_face = constrptr->face_ptr->curdim_face;
                prov_constrptr  = constrptr;
            }
        }
    }
    ord_link->constraint_e = prov_constrptr;
    ord_link->right        = (CODORDER_LINK *) 0;
    ord_link->left         = (CODORDER_LINK *) 0;

    if (DEBUG)
        printf("\nThe first chosen constraint is : %s \n",
               ord_link->constraint_e->relation);

    seed = start_seed(dim_cube, ord_link->constraint_e->face_ptr->curdim_face);
    strcpy(ord_link->constraint_e->face_ptr->seed_value, seed);
    free_mem(seed);
    ord_link->constraint_e->face_ptr->seed_valid = TRUE;
    if (DEBUG)
        printf("The first seed is : %s\n",
               ord_link->constraint_e->face_ptr->seed_value);

    return (ord_link);
}

/*******************************************************************************
 *                    Selects a new constraint to code * Try the selection in
 *the following priority order  :                         * 1) are there
 *constraints of category 1 not already coded with the same       * curdim_face
 *as constrptr and having intersections in common with it ?       * i.e. among
 *the sons of constrptr having fathers of category 1 with the same * curdim_face
 *choose one and among its fathers choose one not already coded   * 2) are there
 *constraints of category 1 not already coded with the same       * curdim_face
 *as constrptr ?                                                  * 3) are there
 *constraints not already coded with the same curdim_face as      * constrptr
 *and having intersections in common with it ?                      * i.e. among
 *the sons of constrptr having fathers with the same curdim_face   * choose one
 *and among its fathers choose one not already coded               * 4) are
 *there constraints not already coded with the same curdim_face as      *
 *  constrptr ? * 5) are there constraints not already coded with a smaller
 *curdim_face ?      *
 *******************************************************************************/

CONSTRAINT_E *next_to_code(constrptr, dim_cube)CONSTRAINT_E *constrptr;
                                               int dim_cube;

{

    int          i, level;
    char         *seed, *start_seed(), *copy_seed();
    SONS_LINK    *son_scanner;
    FATHERS_LINK *ft_scanner;
    CONSTRAINT_E *scanner, *temp_ptr;

    temp_ptr = (CONSTRAINT_E *) 0;

    /* first branch of the selection criterion :
       are there constraints of category 1 not already coded with the same
       curdim_face as constrptr and having intersections in common with it ?
       i.e. among the sons of constrptr having fathers of category 1 with the
       same curdim_face choose one and among its fathers choose one not already
       coded */
    for (son_scanner = constrptr->down_ptr; son_scanner != (SONS_LINK *) 0;
         son_scanner = son_scanner->next) {
        for (ft_scanner = son_scanner->constraint_e->up_ptr;
             ft_scanner != (FATHERS_LINK *) 0; ft_scanner = ft_scanner->next) {
            if (ft_scanner->constraint_e->face_ptr->first_valid == FALSE &&
                (constrptr->face_ptr->curdim_face ==
                 ft_scanner->constraint_e->face_ptr->curdim_face) &&
                ft_scanner->constraint_e->face_ptr->category == 1) {
                if (DEBUG)
                    printf("\nThe next chosen constraint is : %s \n",
                           ft_scanner->constraint_e->relation);
                /* computes seed_value */
                if (constrptr->face_ptr->curdim_face ==
                    ft_scanner->constraint_e->face_ptr->curdim_face) {
                    seed = copy_seed(dim_cube, constrptr->face_ptr->cur_value);
                    strcpy(ft_scanner->constraint_e->face_ptr->seed_value, seed);
                    free_mem(seed);
                    ft_scanner->constraint_e->face_ptr->seed_valid = TRUE;
                } else {
                    seed = start_seed(dim_cube,
                                      ft_scanner->constraint_e->face_ptr->curdim_face);
                    strcpy(ft_scanner->constraint_e->face_ptr->seed_value, seed);
                    free_mem(seed);
                    ft_scanner->constraint_e->face_ptr->seed_valid = TRUE;
                }
                if (DEBUG) {
                    printf("The seed is : %s\n",
                           ft_scanner->constraint_e->face_ptr->seed_value);
                    printf("next_to_code : branch n. 1\n");
                }
                return (ft_scanner->constraint_e);
            }
        }
    }

    /* second branch of the selection criterion :
       are there constraints of category 1 not already coded with the same
       curdim_face as constrptr ? */
    for (i = graph_depth - 1; i >= 0; i--) {
        for (scanner = graph_levels[i]; scanner != (CONSTRAINT_E *) 0;
             scanner = scanner->next) {
            if (scanner->face_ptr->first_valid == FALSE &&
                constrptr->face_ptr->curdim_face == scanner->face_ptr->curdim_face &&
                scanner->face_ptr->category == 1) {
                if (DEBUG)
                    printf("The next chosen constraint is : %s \n", scanner->relation);
                /* computes seed_value */
                seed = start_seed(dim_cube, scanner->face_ptr->curdim_face);
                strcpy(scanner->face_ptr->seed_value, seed);
                free_mem(seed);
                scanner->face_ptr->seed_valid = TRUE;
                if (DEBUG) {
                    printf("The seed is : %s\n", scanner->face_ptr->seed_value);
                    printf("next_to_code : branch n. 2\n");
                }
                return (scanner);
            }
        }
    }

    /* third branch of the selection criterion :
       are there constraints not already coded with the same curdim_face
       as constrptr and having intersections in common with it ?
       i.e. among the sons of constrptr having fathers with the same curdim_face
       choose one and among its fathers choose one not already coded */
    for (son_scanner = constrptr->down_ptr; son_scanner != (SONS_LINK *) 0;
         son_scanner = son_scanner->next) {
        for (ft_scanner = son_scanner->constraint_e->up_ptr;
             ft_scanner != (FATHERS_LINK *) 0; ft_scanner = ft_scanner->next) {
            if (ft_scanner->constraint_e->face_ptr->first_valid == FALSE &&
                (constrptr->face_ptr->curdim_face ==
                 ft_scanner->constraint_e->face_ptr->curdim_face)) {
                if (DEBUG)
                    printf("\nThe next chosen constraint is : %s \n",
                           ft_scanner->constraint_e->relation);
                /* computes seed_value */
                if (constrptr->face_ptr->curdim_face ==
                    ft_scanner->constraint_e->face_ptr->curdim_face) {
                    seed = copy_seed(dim_cube, constrptr->face_ptr->cur_value);
                    strcpy(ft_scanner->constraint_e->face_ptr->seed_value, seed);
                    free_mem(seed);
                    ft_scanner->constraint_e->face_ptr->seed_valid = TRUE;
                } else {
                    seed = start_seed(dim_cube,
                                      ft_scanner->constraint_e->face_ptr->curdim_face);
                    strcpy(ft_scanner->constraint_e->face_ptr->seed_value, seed);
                    free_mem(seed);
                    ft_scanner->constraint_e->face_ptr->seed_valid = TRUE;
                }
                if (DEBUG) {
                    printf("The seed is : %s\n",
                           ft_scanner->constraint_e->face_ptr->seed_value);
                    printf("next_to_code : branch n. 3\n");
                }
                return (ft_scanner->constraint_e);
            }
        }
    }

    /* fourth branch of the selection criterion :
       are there constraints not already coded with the same curdim_face as
       constrptr ? */
    for (i = graph_depth - 1; i >= 0; i--) {
        for (scanner = graph_levels[i]; scanner != (CONSTRAINT_E *) 0;
             scanner = scanner->next) {
            if (scanner->face_ptr->first_valid == FALSE &&
                constrptr->face_ptr->curdim_face == scanner->face_ptr->curdim_face) {
                if (DEBUG)
                    printf("The next chosen constraint is : %s \n", scanner->relation);
                /* computes seed_value */
                seed = start_seed(dim_cube, scanner->face_ptr->curdim_face);
                strcpy(scanner->face_ptr->seed_value, seed);
                free_mem(seed);
                scanner->face_ptr->seed_valid = TRUE;
                if (DEBUG) {
                    printf("The seed is : %s\n", scanner->face_ptr->seed_value);
                    printf("next_to_code : branch n. 4\n");
                }
                return (scanner);
            }
        }
    }

    /* fifth branch of the selection criterion :
       are there constraints not already coded with a smaller
       curdim_face ? */

    /* are there constraints not already coded with a smaller
       curdim_face ? choose the maximum curdim_face */
    level  = -1;
    for (i = graph_depth - 1; i >= 0; i--) {
        for (scanner = graph_levels[i]; scanner != (CONSTRAINT_E *) 0;
             scanner = scanner->next) {
            if (scanner->face_ptr->first_valid == FALSE &&
                scanner->face_ptr->curdim_face > level) {
                level    = scanner->face_ptr->curdim_face;
                temp_ptr = scanner;
            }
        }
    }
    /* is any of them of category 1 ? choose it */
    level  = -1;
    for (i = graph_depth - 1; i >= 0; i--) {
        for (scanner = graph_levels[i]; scanner != (CONSTRAINT_E *) 0;
             scanner = scanner->next) {
            if (scanner->face_ptr->first_valid == FALSE &&
                scanner->face_ptr->category == 1 &&
                scanner->face_ptr->curdim_face == temp_ptr->face_ptr->curdim_face) {
                level = scanner->face_ptr->curdim_face;
                if (DEBUG)
                    printf("The next chosen constraint is : %s \n", scanner->relation);
                /* computes seed_value */
                seed = start_seed(dim_cube, scanner->face_ptr->curdim_face);
                strcpy(scanner->face_ptr->seed_value, seed);
                free_mem(seed);
                scanner->face_ptr->seed_valid = TRUE;
                if (DEBUG) {
                    printf("The seed is : %s\n", scanner->face_ptr->seed_value);
                    printf("next_to_code : branch n. 5.1\n");
                }
                return (scanner);
            }
        }
    }
    /* otherwise choose what pointed to by temptr */
    if (level == -1 && temp_ptr != (CONSTRAINT_E *) 0) {
        if (DEBUG)
            printf("The next chosen constraint is : %s \n", temp_ptr->relation);
        /* computes seed_value */
        seed = start_seed(dim_cube, temp_ptr->face_ptr->curdim_face);
        strcpy(temp_ptr->face_ptr->seed_value, seed);
        free_mem(seed);
        temp_ptr->face_ptr->seed_valid = TRUE;
        if (DEBUG) {
            printf("The seed is : %s\n", temp_ptr->face_ptr->seed_value);
            printf("next_to_code : branch n. 5.2\n");
        }
        return (temp_ptr);
    }

    /* PREVIOUS IMPLEMENTATION OF THE 5TH BRANCH - STARTS
    for (level = constrptr->face_ptr->curdim_face; level >= 0; level--) {
        if (DEBUG) printf("\nencoding level = %d \n", level);
        for (i = graph_depth-1; i >= 0; i--) {
            for (scanner = graph_levels[i]; scanner != (CONSTRAINT_E *) 0;
                                                  scanner = scanner->next ) {
                if (scanner->face_ptr->first_valid == FALSE &&
                    scanner->face_ptr->curdim_face == level ) {
                    if (DEBUG) printf("The next chosen constraint is : %s \n",
                                                     scanner->relation);
                    seed = start_seed(dim_cube,level);
                    strcpy(scanner->face_ptr->seed_value,seed);
                    free_mem(seed);
                    scanner->face_ptr->seed_valid = TRUE;
                    if (DEBUG) { printf("The seed is : %s\n",
    scanner->face_ptr->seed_value); printf("next_to_code : branch n. 5\n"); }
                    return(scanner);
                }
            }
        }
    }
    PREVIOUS IMPLEMENTATION OF THE 5TH BRANCH - ENDS */

    return ((CONSTRAINT_E *) 0);
}

/*******************************************************************************
 *   Inserts the constraint pointed to by curptr in the selection list and *
 *   returns a pointer to the last element added to the selection list
 * ********************************************************************************/

CODORDER_LINK *insert_in_list(curptr, next_constr)CODORDER_LINK *curptr;
                                                  CONSTRAINT_E *next_constr;

{

    CODORDER_LINK *ord_link;

    if ((ord_link = (CODORDER_LINK *) calloc(
            (unsigned) 1, sizeof(CODORDER_LINK))) == (CODORDER_LINK *) 0) {
        fprintf(stderr, "Insufficient memory for ord_link in backtrack");
        exit(-1);
    }
    ord_link->constraint_e = next_constr;
    ord_link->right        = curptr;
    curptr->left           = ord_link;
    ord_link->left         = (CODORDER_LINK *) 0;
    curptr = ord_link;

    return (curptr);
}

/* codes the universe constraint */

code_complete_constr(dim_cube, dim_face
)
int dim_cube;
int dim_face;

{

int  j;
char *cube_code;

if ((
cube_code = (char *) calloc((unsigned) dim_cube + 1, sizeof(char))
) ==
(char *)0) {
fprintf(stderr,
"Insufficient memory for cube_code in start_code");
exit(-1);
}

cube_code[dim_cube] = '\0';

for (
j = 0;
j<dim_face;
j++) {
cube_code[j] = 'x';
}

strcpy(graph_levels[graph_depth - 1]
->face_ptr->seed_value, cube_code);
graph_levels[graph_depth - 1]->face_ptr->
seed_valid = TRUE;
strcpy(graph_levels[graph_depth - 1]
->face_ptr->first_value, cube_code);
graph_levels[graph_depth - 1]->face_ptr->
first_valid = TRUE;
strcpy(graph_levels[graph_depth - 1]
->face_ptr->cur_value, cube_code);
graph_levels[graph_depth - 1]->face_ptr->
code_valid = TRUE;
graph_levels[graph_depth - 1]->face_ptr->
count_index = 1;
graph_levels[graph_depth - 1]->face_ptr->
comb_index = 1;
graph_levels[graph_depth - 1]->face_ptr->
lexmap_index = 1;

free_mem(cube_code);
}

/*******************************************************************************
 * creates a face of dim_cube bits with dim_face x's and the pattern x..x0..0 *
 *******************************************************************************/

char *copy_seed(dim_cube, cur_face)int dim_cube;
                                   char *cur_face;

{

    int  j;
    char *seed_code;

    if ((seed_code = (char *) calloc((unsigned) dim_cube + 1, sizeof(char))) ==
        (char *) 0) {
        fprintf(stderr, "Insufficient memory for seed_code");
        exit(-1);
    }

    seed_code[dim_cube] = '\0';

    for (j = 0; j < dim_cube; j++) {
        seed_code[j] = cur_face[j];
    }

    return (seed_code);
}

/*******************************************************************************
 * creates a face of dim_cube bits with dim_face x's and the pattern x..x0..0 *
 *******************************************************************************/

char *start_seed(dim_cube, dim_face)int dim_cube;
                                    int dim_face;

{

    int  i, j;
    char *seed_code;

    /* dim_face = number of x's in the face of "dim_cube" bits */

    if ((seed_code = (char *) calloc((unsigned) dim_cube + 1, sizeof(char))) ==
        (char *) 0) {
        fprintf(stderr, "Insufficient memory for seed_code");
        exit(-1);
    }

    seed_code[dim_cube] = '\0';

    for (j = 0; j < dim_face; j++) {
        seed_code[j] = 'x';
    }
    for (i = dim_face; i < dim_cube; i++) {
        seed_code[i] = ZERO;
    }

    return (seed_code);
}

/*******************************************************************************
 *           Assigns a face to the first selected constraint *
 *******************************************************************************/

assign_face_tofirst(curptr, dim_cube
)
CODORDER_LINK *curptr;
int           dim_cube;

{

CODORDER_LINK *ord_temptr;
char          *new_code, *gen_newcode_cat1();
int           level;

if (DEBUG)
printf("\n** Monitoring of assign_face_tofirst **\n");

/* generates a new code starting from seed_value */
new_code = gen_newcode_cat1(
        curptr, curptr->constraint_e->face_ptr->seed_value, dim_cube);
if (DEBUG)
printf("\ngen_newcode_cat1 returned = %s\n", new_code);

/* debug print to show the selection list */
level = mylog2(minpow2(curptr->constraint_e->card));

/* for all constraints already coded at the same level */
if (DEBUG) {
printf("\nConstraints at the same level already in ord_link :\n");
for (
ord_temptr = curptr;
ord_temptr != (CODORDER_LINK *)0;
ord_temptr = ord_temptr->right
) {
if (

mylog2 (minpow2(ord_temptr

->constraint_e->card)) == level) {
printf("%s ", ord_temptr->constraint_e->relation);
}
}
printf("\n");
}

if (DEBUG)
printf("** Exit from assign_face_tofirst **\n");
}

/*******************************************************************************
 *              Assigns a face to the current constraint * Given a constraint,
 *it assigns to it a face compatible with the partial      * assignment built up
 *to now; at the same time, it assigns faces to those      * constraints of
 *category 2, that are children of the constraint being         * currently
 *encoded and have another father already encoded.                   * When
 *unable to map a constraint to a face, it informs backtrack_down which   *
 * takes actions. * Faces are generated calling the routines gen_newface_catx,
 *and they are      * verified with help of the routines fathers_codes_ok and
 *code_verify          *
 *******************************************************************************/

BOOLEAN assign_face(curptr, dim_cube, net_num)CODORDER_LINK *curptr;
                                              int dim_cube;
                                              int net_num;

{

    CODORDER_LINK *ord_temptr;
    SONS_LINK     *son_scanner;
    CONSTRAINT_E  *scanner, *constrptr, *inter_ptr, *exist_son();
    char          *new_code, *inter_code, *gen_newcode_cat1(), *gen_newcode_cat2(),
                  *gen_newcode_cat3(), *a_inter_b();
    int           level;
    BOOLEAN ok_to_code;

    if (DEBUG)
        printf("\n** Monitoring of assign_face **\n");

    /* the faces of sons of curptr having it as codfather and already assigned
       are cleared */
    constrptr        = curptr->constraint_e;
    for (son_scanner = constrptr->down_ptr; son_scanner != (SONS_LINK *) 0;
         son_scanner = son_scanner->next) {
        if (son_scanner->codfather_ptr == constrptr) {
            if (DEBUG)
                printf("\ncleared son : %s\n", son_scanner->constraint_e->relation);
            son_scanner->constraint_e->face_ptr->seed_valid   = FALSE;
            son_scanner->constraint_e->face_ptr->first_valid  = FALSE;
            son_scanner->constraint_e->face_ptr->code_valid   = FALSE;
            son_scanner->constraint_e->face_ptr->count_index  = 0;
            son_scanner->constraint_e->face_ptr->comb_index   = 0;
            son_scanner->constraint_e->face_ptr->lexmap_index = 0;
            son_scanner->constraint_e->face_ptr->dim_index    = 0;
            son_scanner->codfather_ptr                        = (CONSTRAINT_E *) 0;
        }
    }

    ok_to_code = FALSE;

    /* generates a new code starting from the seed - according to the
       category of constrptr a different gen_newcode is invoked */
    if (curptr->constraint_e->face_ptr->category == 1) {
        new_code = gen_newcode_cat1(
                curptr, curptr->constraint_e->face_ptr->seed_value, dim_cube);
        if (DEBUG)
            printf("\ngen_newcode_cat1 returned = %s\n", new_code);
    }
    if (curptr->constraint_e->face_ptr->category == 2) {
        new_code = gen_newcode_cat2(curptr, dim_cube);
        if (DEBUG)
            printf("\ngen_newcode_cat2 returned = %s\n", new_code);
    }
    if (curptr->constraint_e->face_ptr->category == 3) {
        new_code = gen_newcode_cat3(curptr, dim_cube);
        if (DEBUG)
            printf("\ngen_newcode_cat3 returned = %s\n", new_code);
    }

    while (new_code != (char *) 0) {

        /* checks if new_code satisfies the inclusion relations with
           respect to the codes already assigned                      */
        if (fathers_codes_ok(curptr->constraint_e, new_code, dim_cube) == 1 &&
            code_verify(curptr->constraint_e, new_code, net_num, dim_cube) == 1) {

            ok_to_code = TRUE;
            if (DEBUG)
                printf("code_verify verified the code\n");

            /* assigns faces to those constraints of category 2, that are
               children of the constraint being currently encoded and have
               another father already encoded                                */
            for (level = graph_depth - 1; level >= 0; level--) {
                for (scanner = graph_levels[level]; scanner != (CONSTRAINT_E *) 0;
                     scanner = scanner->next) {
                    if (scanner->face_ptr->code_valid != FALSE &&
                        scanner != curptr->constraint_e) {

                        /* that have a non-void intersection with the current one */
                        inter_ptr = exist_son(scanner, curptr->constraint_e, dim_cube);

                        if (inter_ptr != (CONSTRAINT_E *) 0) {

                            if (DEBUG) {
                                printf("\nThere are non-empty intersections\n");
                                printf("%s intersects with ", curptr->constraint_e->relation);
                                printf("%s ", scanner->relation);
                                printf("in %s\n", inter_ptr->relation);
                            }
                            inter_code = a_inter_b(scanner->face_ptr->cur_value,
                                                   curptr->constraint_e->face_ptr->cur_value,
                                                   inter_ptr->card, dim_cube);

                            if (inter_ptr->face_ptr->code_valid == FALSE) {
                                /* intersection not already coded */

                                /*printf("It has not been already coded \n");*/

                                /* if possible, a face is assigned to the
                       intersection, otherwise ok_to_code = FALSE and
                       break                                          */
                                if (DEBUG) {
                                    printf("codes = %s ",
                                           curptr->constraint_e->face_ptr->cur_value);
                                    printf("and = %s ", scanner->face_ptr->cur_value);
                                }
                                if (inter_code != (char *) 0) {
                                    if (DEBUG)
                                        printf("intersect in %s (feasible code)\n", inter_code);
                                } else {
                                    if (DEBUG)
                                        printf("intersect in a non-feasible code\n");
                                }

                                if (inter_code != (char *) 0) {

                                    if (fathers_codes_ok(inter_ptr, inter_code, dim_cube) == 1 &&
                                        code_verify(inter_ptr, inter_code, net_num, dim_cube) ==
                                        1) {
                                        if (DEBUG)
                                            printf("code_verify verified the code\n");
                                        ok_to_code = TRUE;
                                        /* a face is assigned to the intersection */
                                        strcpy(inter_ptr->face_ptr->first_value, inter_code);
                                        inter_ptr->face_ptr->first_valid = TRUE;
                                        strcpy(inter_ptr->face_ptr->cur_value, inter_code);
                                        free_mem(inter_code);
                                        inter_ptr->face_ptr->code_valid   = TRUE;
                                        inter_ptr->face_ptr->count_index  = 1;
                                        inter_ptr->face_ptr->comb_index   = 1;
                                        inter_ptr->face_ptr->lexmap_index = 1;
                                        /* sets properly codfather_ptr */
                                        set_codfather(curptr->constraint_e, inter_ptr);

                                    } else {

                                        ok_to_code = FALSE;
                                        free_mem(inter_code);
                                        break;
                                    }
                                } else {

                                    ok_to_code = FALSE;
                                    break;
                                }

                            } else {

                                /* intersection already coded */
                                if (DEBUG)
                                    printf("Its already assigned code is : %s\n",
                                           inter_ptr->face_ptr->cur_value);
                                if (strcmp(inter_code, inter_ptr->face_ptr->cur_value) == 0) {
                                    if (DEBUG)
                                        printf("Codes are compatible\n");
                                    ok_to_code = TRUE;
                                    free_mem(inter_code);

                                } else {
                                    if (DEBUG)
                                        printf("Codes are incompatible\n");
                                    ok_to_code = FALSE;
                                    free_mem(inter_code);
                                    break;
                                }
                            }
                        }
                    }
                }
                if (ok_to_code == FALSE)
                    break;
            }
        }

        if (ok_to_code == FALSE) {

            /* generates a new code starting from the seed - according to the
               category of constrptr a different gen_newcode is invoked */
            if (curptr->constraint_e->face_ptr->category == 1) {
                new_code = gen_newcode_cat1(
                        curptr, curptr->constraint_e->face_ptr->seed_value, dim_cube);
                if (DEBUG)
                    printf("\ngen_newcode_cat1 returned = %s\n", new_code);
            }
            if (curptr->constraint_e->face_ptr->category == 2) {
                new_code = gen_newcode_cat2(curptr, dim_cube);
                if (DEBUG)
                    printf("\ngen_newcode_cat2 returned = %s\n", new_code);
            }
            if (curptr->constraint_e->face_ptr->category == 3) {
                new_code = gen_newcode_cat3(curptr, dim_cube);
                if (DEBUG)
                    printf("\ngen_newcode_cat3 returned = %s\n", new_code);
            }

        } else { /* ok_to_code == TRUE */
            break;
        }
    }

    /* for all constraints already coded in ord_link */
    if (DEBUG) {
        printf("\nConstraints already in ord_link :\n");
        for (ord_temptr = curptr; ord_temptr != (CODORDER_LINK *) 0;
             ord_temptr = ord_temptr->right) {
            printf("%s ", ord_temptr->constraint_e->relation);
        }
        printf("\n");
    }

    if (DEBUG)
        printf("** Exit from assign_face **\n");

    if (new_code != (char *) 0) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}

/*******************************************************************************
 * are the constraints constrptr1 and constrptr2 connected to an intersection *
 * constraint ? * i.e., among the children of constrptr1 is there any who is
 *also a child of   * constrptr2 ? * if yes and the dimension of the
 *intersection of the fathers' faces is <=     * than the current face dimension
 *of the son                        * a pointer to the son is returned ; *
 * otherwise (i.e., if a) yes and the dimen. of the inter. of the fathers' *
 *                        faces is > than the current face dimension of the son
 ** b) or there is no intersection constraint)               * a pointer to nil
 *is returned .                                               * In case b) the
 *reason is that there is more than 1 possible code and we can  * generate these
 *codes only via invoking gen_newcode_cat2 at the upper level,  * while we can
 *code intersections as a byproduct of other coding operations    * only when
 *they can have at most one code ; notice that this was a practical  * design
 *decision due to a later understanding of this circumstance            *
 *******************************************************************************/

CONSTRAINT_E *exist_son(constrptr1, constrptr2,
                        dim_cube)CONSTRAINT_E *constrptr1;
                                 CONSTRAINT_E *constrptr2;
                                 int dim_cube;

{

    SONS_LINK    *son_scanner;
    FATHERS_LINK *ft_scanner;
    int dim_fathers_inter();

    /* are the constraints constrptr1 and constrptr2 connected to an
       intersection constraint ?
       i.e., among the children of constrptr1 is there any who is also
       child of constrptr2 ?                                             */
    for (son_scanner = constrptr1->down_ptr; son_scanner != (SONS_LINK *) 0;
         son_scanner = son_scanner->next) {
        for (ft_scanner = son_scanner->constraint_e->up_ptr;
             ft_scanner != (FATHERS_LINK *) 0; ft_scanner = ft_scanner->next) {
            if ((ft_scanner->constraint_e == constrptr2) &&
                (dim_fathers_inter(constrptr1->face_ptr->cur_value,
                                   constrptr2->face_ptr->cur_value, dim_cube) <=
                 son_scanner->constraint_e->face_ptr->curdim_face)) {
                /*printf("op. # 1 =
                %d\n",dim_fathers_inter(constrptr1->face_ptr->cur_value,constrptr2->face_ptr->cur_value,dim_cube));
                printf("op. # 2 =
                %d\n",son_scanner->constraint_e->face_ptr->curdim_face);*/
                /* if the dimension of the intersection of the fathers'
                   faces is <= than the current face dimension of the son
                   a pointer to the son is returned -
                   if the dimen. of the inter. of the fathers' faces is >
                   than the current face dimension of the son a pointer to
                   nil is returned, because there is more than 1 possible
                   code and we can generate these codes only via invoking
                   gen_newcode_cat2 at the upper level, while we can code
                   intersections as a byproduct of other coding operations
                   only when they can have at most one code -
                   notice that this is an implementation choice due to a
                   later understanding of this situation */
                return (son_scanner->constraint_e);
            }
        }
    }

    return ((CONSTRAINT_E *) 0);
}

/*******************************************************************************
 * called by assign_face : let codfather_ptr of constrptr2 point to constrptr1 *
 *******************************************************************************/

set_codfather(constrptr1, constrptr2
)
CONSTRAINT_E *constrptr1;
CONSTRAINT_E *constrptr2;

{

SONS_LINK *son_scanner;

for (
son_scanner = constrptr1->down_ptr;
son_scanner != (SONS_LINK *)0;
son_scanner = son_scanner->next
) {
if (son_scanner->constraint_e == constrptr2) {
son_scanner->
codfather_ptr = constrptr1;
if (DEBUG)
printf("The codfather of %s is %s\n", constrptr2->relation,
son_scanner->codfather_ptr->relation);
break;
}
}
}

/*******************************************************************************
 * The verifications of a new proposed code carried on by this routine are : *
 * 1) if the new constraint has only one father, the latter's face must include
 ** properly the face proposed for the former                                 *
 * 2) if the new constraint has more than one father : * 2.1) if the dimension
 *of the intersection of the faces assigned           * to the fathers is > than
 *curdim_face of the son :                      * ditto intersection must
 *contain (also not properly) the face           * proposed for the son * 2.2)
 *if the dimension of the intersection of the fathers is ==            * to
 *curdim_face of the son :                                            * the
 *faces assigned to the fathers must intersect in the face           * proposed
 *for the son                                                   *
 *******************************************************************************/

fathers_codes_ok(refptr, code, dim_cube
)
CONSTRAINT_E *refptr;
char         *code;
int          dim_cube;

{

FATHERS_LINK *father_scanner;
char         *ft_inter, *a_inter_b(), *prov_ptr;
BOOLEAN flag;
int          i;

flag = FALSE;

if ((
ft_inter = (char *) calloc((unsigned) dim_cube + 1, sizeof(char))
) ==
(char *)0) {
fprintf(stderr,
"Insufficient memory for ft_inter in fathers_codes_ok");
exit(-1);
}

ft_inter[dim_cube] = '\0';
for (
i = 0;
i<dim_cube;
i++) {
ft_inter[i] = 'x';
}

/* 1) if the new constraint has only one father, the latter's face must
      include properly the face proposed for the former                   */
/*printf("\nThe fathers of constraint %s are :\n", refptr->relation);*/
if (refptr->up_ptr->next == (FATHERS_LINK *)0) {
if (refptr->up_ptr->constraint_e->face_ptr->code_valid != FALSE) {
/*printf("%s \n", refptr->up_ptr->constraint_e->relation);*/
if (
face1_incl_face2(refptr
->up_ptr->constraint_e->face_ptr->cur_value,
code, dim_cube) == 1) {
/*printf("Father satisfied : %s includes %s\n",
      refptr->up_ptr->constraint_e->face_ptr->cur_value,code);*/
flag = TRUE;
free_mem(ft_inter);
return (flag);
}
}

} else {

/* 2) if the new constraint has more than one father :
   (notice that every constraint (except the universe) has either 1 or
    more fathers, and that only constraints with more than 1 father
    reach this point) */
for (
father_scanner = refptr->up_ptr;
father_scanner != (FATHERS_LINK *)0;
father_scanner = father_scanner->next
) {
if (father_scanner->constraint_e->face_ptr->code_valid != FALSE) {
/*printf("%s \n", father_scanner->constraint_e->relation);*/
prov_ptr = ft_inter;
ft_inter = a_inter_b(ft_inter,
                     father_scanner->constraint_e->face_ptr->cur_value,
                     0, dim_cube);
free_mem(prov_ptr);
}
}

/* 2.1) if the dimension of the intersection of the faces assigned
   to the fathers is > than curdim_face of the son :
   ditto intersection must contain (also not properly) the face
   proposed for the son */
if (
dim_face(ft_inter, dim_cube
) > refptr->face_ptr->curdim_face) {
if (
face1_incl2_face2(ft_inter, code, dim_cube
) == 1) {
/*printf(
       "Fathers satisfied : ft_inter = %s includes code = %s \n",
                                      ft_inter, code);
printf("fathers_codes_ok verified the code\n");*/
flag = TRUE;
free_mem(ft_inter);
return (flag);
}
}
/* 2.2) if the dimension of the intersection of the fathers is ==
   to curdim_face of the son :
   the faces assigned to the fathers must intersect in the face
   proposed for the son  */
if (
dim_face(ft_inter, dim_cube
) == refptr->face_ptr->curdim_face) {
if (
strcmp(ft_inter, code
) == 0) {
/*printf(
         "Fathers satisfied : ft_inter = %s and code = %s coincide\n",
                                        ft_inter, code);
  printf("fathers_codes_ok verified the code\n");*/
flag = TRUE;
free_mem(ft_inter);
return (flag);
}
}
}

free_mem(ft_inter);

return (flag);
}

/*******************************************************************************
 *   computes the intersection of the codes assigned to the fathers of refptr *
 *******************************************************************************/

char *fathers_codes_inter(refptr, dim_cube)CONSTRAINT_E *refptr;
                                           int dim_cube;

{

    FATHERS_LINK *father_scanner;
    char         *ft_inter, *prov_ptr, *a_inter_b();
    int          i;

    if ((ft_inter = (char *) calloc((unsigned) dim_cube + 1, sizeof(char))) ==
        (char *) 0) {
        fprintf(stderr, "Insufficient memory for ft_inter in fathers_code_inter");
        exit(-1);
    }

    ft_inter[dim_cube] = '\0';
    for (i = 0; i < dim_cube; i++) {
        ft_inter[i] = 'x';
    }

    if (DEBUG)
        printf("\nThe fathers of constraint %s are :\n", refptr->relation);
    if (refptr->up_ptr->next == (FATHERS_LINK *) 0) {
        if (refptr->up_ptr->constraint_e->face_ptr->code_valid != FALSE) {
            if (DEBUG) {
                printf("%s ", refptr->up_ptr->constraint_e->relation);
                printf("with intersection = %s \n",
                       refptr->up_ptr->constraint_e->face_ptr->cur_value);
            }
            strcpy(ft_inter, refptr->up_ptr->constraint_e->face_ptr->cur_value);
            return (ft_inter);
        }

    } else {

        for (father_scanner = refptr->up_ptr; father_scanner != (FATHERS_LINK *) 0;
             father_scanner = father_scanner->next) {
            if (father_scanner->constraint_e->face_ptr->code_valid != FALSE) {
                if (DEBUG) {
                    printf("%s ", father_scanner->constraint_e->relation);
                    printf("with code %s \n",
                           father_scanner->constraint_e->face_ptr->cur_value);
                }
                prov_ptr = ft_inter;
                ft_inter = a_inter_b(ft_inter,
                                     father_scanner->constraint_e->face_ptr->cur_value,
                                     0, dim_cube);
                free_mem(prov_ptr);
            }
        }

        if (DEBUG)
            printf("and intersection = %s \n", ft_inter);

        return (ft_inter);
    }

    return (0);
}

/*******************************************************************************
 * The verifications of a new proposed code carried on by this routine are : *
 * 1) an assigned face shouldn't coincide with the new one * 2) if an assigned
 *face includes properly the new one, the old constraint     * should be a
 *father of the new one                                         * 3) if the new
 *face includes properly a face already assigned, the new        * constraint
 *should be a father of the old one                              * 4) if an
 *assigned face has a non-empty intersection with the new one (and    * both of
 *them contain properly the intersection), the related constraints  * should
 *have a non_empty intersection                                      * A last
 *check helpful for efficiency, not necessary for correcteness :     * 5) if an
 *already coded constraint (not of cardinality 1) has a nonempty      *
 *    intersection with the current constraint (not cardinality 1) the related *
 *    faces must intersect in a subface large enough to contain the *
 *    intersection of the constraints *
 *******************************************************************************/

code_verify(refptr, code, net_num, dim_cube
)
CONSTRAINT_E *refptr;
char         *code;
int          net_num;
int          dim_cube;

{

CONSTRAINT_E *constrptr;
char         *inter_code, *inter_constr, *a_inter_b(), *c1_inter_c2();
int          i, state_pos1, state_pos2;

for (
i = graph_depth - 1;
i >= 0; i--) {
for (
constrptr = graph_levels[i];
constrptr != (CONSTRAINT_E *)0;
constrptr = constrptr->next
) {
if (constrptr != refptr && constrptr->face_ptr->code_valid != FALSE) {

/* 1) an assigned face must not coincide with the new one */
if (
strcmp(constrptr
->face_ptr->cur_value, code) == 0) {
if (DEBUG)
printf("The code already exists\n");
return (0);
}

/* 2) if an assigned face includes properly the new one the old
constraint must be a father of the new one              */
if (
face1_incl_face2(constrptr
->face_ptr->cur_value, code, dim_cube) ==
1) {
if (
p_incl_s(constrptr
->relation, refptr->relation) == 0) {
if (DEBUG)
printf("%s includes properly %s but %s is not father of %s\n",
constrptr->face_ptr->cur_value, code, constrptr->relation,
refptr->relation);
return (0);
}
}

/* 3) if the new face includes properly a face already assigned
      the new constraint must be a father of the old one      */
if (
face1_incl_face2(code, constrptr
->face_ptr->cur_value, dim_cube) ==
1) {
if (
p_incl_s(refptr
->relation, constrptr->relation) == 0) {
if (DEBUG)
printf("%s includes properly %s but %s is not father of %s\n",
code, constrptr->face_ptr->cur_value, refptr->relation,
constrptr->relation);
return (0);
}
}

/* 4) if an assigned face has a non-empty intersection with
the new one ( and both of them contain the intersection
properly ) the related constraints must have a non_empty
intersection                                            */
inter_code =
a_inter_b(constrptr->face_ptr->cur_value, code, 0, dim_cube);
if (inter_code != (char *)0 &&
face1_incl_face2(constrptr
->face_ptr->cur_value, inter_code,
dim_cube) == 1 &&
face1_incl_face2(code, inter_code, dim_cube
) == 1) {
inter_constr =
c1_inter_c2(constrptr->relation, refptr->relation, net_num);
if (inter_constr == (char *)0) {
if (DEBUG)
printf("%s and %s intersect but %s and %s don't\n",
constrptr->face_ptr->cur_value, code, constrptr->relation,
refptr->relation);
free_mem(inter_code);
return (0);
}
free_mem(inter_constr);
}
free_mem(inter_code);

/* A last check helpful for efficiency, not necessary for
   correcteness :
   5) if an already coded constraint (not of cardinality 1) has
   a nonempty intersection with the current constraint (not of
   of cardinality 1) the related faces must intersect in a
   subface large enough to contain the intersection of the
   constraints                                                */
if (refptr->card != 1 && constrptr->card != 1) {
inter_constr =
c1_inter_c2(constrptr->relation, refptr->relation, net_num);
if (inter_constr != (char *)0) {
inter_code = a_inter_b(constrptr->face_ptr->cur_value, code,
                       card_c1(inter_constr, net_num), dim_cube);
if (inter_code == (char *)0) {
if (DEBUG)
printf("%s & %s intersect but %s and %s don't\n",
constrptr->relation, refptr->relation,
constrptr->face_ptr->cur_value, code);
free_mem(inter_constr);
return (0);
}
free_mem(inter_code);
free_mem(inter_constr);
}
}
}
}
}

if ((IO_HYBRID || IO_VARIANT) && OUT_VERIFY && refptr->card == 1) {
state_pos1 = locate_state(refptr->relation, net_num);
if (DEBUG)
printf("\n\noutput verification of state %d (%s)", state_pos1, code);
for (
constrptr = graph_levels[0];
constrptr != (CONSTRAINT_E *)0;
constrptr = constrptr->next
) {
if (constrptr != refptr && constrptr->face_ptr->code_valid != FALSE) {
state_pos2 = locate_state(constrptr->relation, net_num);
if (DEBUG)
printf("\nversus state %d (%s) : ", state_pos2,
constrptr->face_ptr->cur_value);
/* if order_array(state_pos1,state_pos2) is = 1 then the
code of state_pos2 ought to cover the code of state_pos1 */
if (select_array[state_pos1] == USED &&
order_array[state_pos1][state_pos2] == ONE) {
if (
codei_covers_codej(constrptr
->face_ptr->cur_value, code) == 0) {
if (DEBUG)
printf("%d doesn't cover %d", state_pos2, state_pos1);
return (0);
}
if (DEBUG)
printf("%d covers %d", state_pos2, state_pos1);
}
/* if order_array(state_pos2,state_pos1) is = 1 then the
code of state_pos1 ought to cover the code of state_pos2 */
if (select_array[state_pos2] == USED &&
order_array[state_pos2][state_pos1] == ONE) {
if (
codei_covers_codej(code, constrptr
->face_ptr->cur_value) == 0) {
if (DEBUG)
printf("%d doesn't cover %d", state_pos1, state_pos2);
return (0);
}
if (DEBUG)
printf("%d covers %d", state_pos1, state_pos2);
}
}
}
}

return (1);
}

/*******************************************************************************
 *          updates the counters of the # of codes tried up to now * and prints
 *the # of codes tried in the last  backtrack_down         *
 *                                                                              *
 *  "local_work" counts the codes tried in the last backtrack_down * "cube_work"
 *counts the codes tried in the current cube                     * "total_work"
 *counts the codes tried in overall by exact_code                *
 *******************************************************************************/

btkdown_summary() {

    CONSTRAINT_E *constrptr_e;
    int          local_work;
    int          i;

    local_work = 0;

    for (i = graph_depth - 1; i >= 0; i--) {
        for (constrptr_e = graph_levels[i]; constrptr_e != (CONSTRAINT_E *) 0;
             constrptr_e = constrptr_e->next) {
            local_work = local_work + constrptr_e->face_ptr->tried_codes;
        }
    }

    /* cube_work (global variable) is set to 0 in faces_dim_set */
    cube_work += local_work;
    /* total_work (global variable) is set to 0 in faces_alloc */
    total_work += local_work;

    if (VERBOSE) {
        printf("\nBTKDOWN_SUMMARY : %d\n", local_work);
        printf("local_work = %d\n", local_work);
    }
}

BOOLEAN toomuch_work() {

    CONSTRAINT_E *constrptr_e;
    int          local_work;
    int          i;

    local_work = 0;

    for (i = graph_depth - 1; i >= 0; i--) {
        for (constrptr_e = graph_levels[i]; constrptr_e != (CONSTRAINT_E *) 0;
             constrptr_e = constrptr_e->next) {
            local_work = local_work + constrptr_e->face_ptr->tried_codes;
        }
    }

    if (local_work > MAXWORK) {
        if (VERBOSE)
            printf("\nBacktrack interrupted : SEARCH TOO LONG OR CUBE TOO SMALL\n");
        return (TRUE);
    } else
        return (FALSE);
}

int locate_state(relation, net_num)char *relation;
                                   int net_num;

{

    int i;

    for (i = 0; i < net_num; i++) {
        if (relation[i] == ONE)
            break;
    }

    return (i);
}
#endif