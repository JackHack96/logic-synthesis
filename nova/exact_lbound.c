
/*******************************************************************************
*             NECESSARY CONDITIONS ON THE HYPERCUBE DIMENSION                  *
* (to allow a quick rejection of dimensions unfeasible for a given problem     *
* instance).                                                                   *
* The following necessary conditions that need to be satisfied by a solution   *
* have been found so far :                                                     *
* 1) first counting criterion - the k-face-poset should have at least as many  *
* faces of a given cardinality as the instance poset has constraints of a      *
* a given cardinality                                                          *
* 2) second counting criterion - a constraint cannot have more fathers than    *
* the face of minimum dimension (that can be assigned to it in the current     *
* hypercube) has minimal including neighbours in the current hypercube         *
* 3) third counting criterion - the current hypercube must have at least as    *
* many free positions (#vertices - net_num) as the minimum number of imaginary *
* states to introduce ( plus a corollary on the feasibility of a densest       *
* packing )                                                                    *
*******************************************************************************/

#include "inc/nova.h"

least_dimcube(net_num)
int net_num;

{

int          i, cube_dim, constr_num, fathers_num, violations_num, neighbours_num,
             immag_num, prov, work, iter_num, horiz_steps, free_pos, *level_count;
BOOLEAN dim_notfound, packing_cond();
CONSTRAINT_E *constrptr_e;
FATHERS_LINK *father_scanner;

typedef struct int_list {
    int             value;
    int             workarea;
    struct int_list *next;
}            INT_LIST;

INT_LIST *immag_list, *new_int, *int_scanner, *int_ptr, *bound_ptr;

/*                  FIST COUNTING CRITERION
   the number of faces of dimension i of a cube of dimension cube_dim
   must be >= than the number of constraints of level i               */

if (VERBOSE) printf("\nFirst counting criterion\n");

if ((
level_count = (int *) calloc((unsigned) graph_depth, sizeof(int))
) == (int *) 0) {
if (VERBOSE)
printf("Insufficient memory for level_count in least_dimcube");
exit(-1);
}

/* level_count[i] tells how many constraints of level i there are */
for (
i = 0;
i<graph_depth;
i++) {
constr_num = 0;
for (
constrptr_e = graph_levels[i];
constrptr_e != (CONSTRAINT_E *) 0;
constrptr_e = constrptr_e->next
) {
constr_num++;
}
level_count[i] =
constr_num;
/*printf("level_count[i] = %d\n", level_count[i]);*/
}


dim_notfound = TRUE;
cube_dim     = 1;

while (dim_notfound == TRUE) {
i = 0;
while ((i < graph_depth-1) &&
(
comb_num(cube_dim, cube_dim
-i)*power(2, cube_dim-i)
>= level_count[i])) {
/*printf("i = %d\n",i);
printf("faces = %d\n",comb_num(cube_dim,cube_dim-i) * power(2,cube_dim-i));
printf("constraints = %d\n",level_count[i]);*/
i++;
}
if (i == graph_depth-1) {
dim_notfound = FALSE;
} else {
cube_dim++;
/*printf("cube_dim = %d\n",cube_dim);*/
}
}

/* special check if there is space for the complete constraint :
   either the dimension of the cube = height of the constraint poset
   or there are no constraints of one level < than the complete one */
if ( !((cube_dim == graph_depth-1) ||
(level_count[graph_depth-2] == 0))) {
/* increase cube_dim to accomodate the complete constraint */
cube_dim++;
}

if (VERBOSE) printf("cube_dim after first nec. cond. = %d\n", cube_dim);



/*                  SECOND COUNTING CRITERION
   a constraint cannot have more fathers than the face of minimum
   dimension (that can be assigned to it in the current hypercube)
   has minimal including neighbours in the current hypercube        */

if (VERBOSE) printf("Second counting criterion\n");

violations_num = 1;
while (violations_num > 0 ) {
violations_num = 0;
for (
i = 0;
i < graph_depth-1; i++) {
for (
constrptr_e = graph_levels[i];
constrptr_e != (CONSTRAINT_E *) 0;
constrptr_e = constrptr_e->next
) {
neighbours_num = cube_dim - mylog2(minpow2(constrptr_e->card));
/*printf("neighbours_num = %d  ", neighbours_num);*/
fathers_num    = 0;
for (
father_scanner = constrptr_e->up_ptr;
father_scanner != (FATHERS_LINK *) 0;
father_scanner = father_scanner->next
) {
fathers_num++;
}
/*printf("fathers_num = %d  \n", fathers_num);*/
if ( neighbours_num < fathers_num) {
violations_num++;
break;
}
}
if (violations_num > 0) {
cube_dim++;
break;
}
}
}
if (VERBOSE) printf("cube_dim after second nec. cond. = %d\n", cube_dim);



/*                    THIRD COUNTING CRITERION
   If there are constraints whose cardinality is not a power of 2 ,
   the faces assigned to them will have some frozen vertices , as if
   the initial problem have more than net_num states , ie. some
   "imaginary" states , too . How many imaginary states should
   we introduce to satisfy the constraints ?
   There are two facts to take into account :
   1) for a constraint of cardinality "c" we need to introduce
                        minpow2(c) - c
      imaginary states;
   2) at most "cube_dim" constraints can share the same imaginary state.
   Using the two given facts we can compute the minimum # of imaginary
   states, i.e. the densiest packing of imaginary states .
   Since a closed formula for it is ackward, we give a dynamic scheme
   to compute it.
   The bottomline is : the current hypercube must have enough free
   positions (#vertices - net_num) to fit at least the densiest packing.
   A corollary ot this criterion is the packing condition that checks
   whether introducing imaginary vertices, in a hypercube with no more
   free positions, creates completed constraints sharing intersections
   whose cardinality is not a power of 2 (an unfeasible situation)    */

if (VERBOSE) printf("Third counting criterion\n");

/* build immag_list, the list of the # of imaginary states introduced
   by each constraint whose cardinality is not a power of 2            */

immag_list = (INT_LIST *) 0;

for (
i = graph_depth - 2;
i >= 0; i--) {
for (
constrptr_e = graph_levels[i];
constrptr_e != (CONSTRAINT_E *) 0;
constrptr_e = constrptr_e->next
) {
immag_num = minpow2(constrptr_e->card) - constrptr_e->card;
if ( immag_num > 0) {
if ((
new_int = (INT_LIST *) calloc((unsigned) 1, sizeof(INT_LIST))
) == ( INT_LIST *) 0) {
if (VERBOSE)
printf("Insufficient memory for new_int in least_dimcube");
exit(-1);
}
new_int->
value = immag_num;
new_int->
next       = immag_list;
immag_list = new_int;
}
}
}

/*printf("immag_list : ");
for (int_scanner = immag_list; int_scanner != (INT_LIST *) 0;
               int_scanner = int_scanner->next) {
    printf("%d ", int_scanner->value);
}
printf("\n");*/

/* immag_list is sorted (in non-decreasing order) -
   a simple bubble sort is used                   */

bound_ptr = (INT_LIST *) 0;
work      = 1;
while ( work > 0 ) {
work = 0;
for (
int_scanner = immag_list;
int_scanner != bound_ptr &&
int_scanner->next != (INT_LIST *) 0;
int_scanner = int_scanner->next
) {
if (int_scanner->value < (int_scanner->next)->value) {
prov = int_scanner->value;
int_scanner->
value = (int_scanner->next)->value;
(int_scanner->next)->
value   = prov;
int_ptr = int_scanner;
work    = 1;
}
}
bound_ptr = int_ptr;
}

/*printf("immag_list : ");
for (int_scanner = immag_list; int_scanner != (INT_LIST *) 0;
               int_scanner = int_scanner->next) {
    printf("%d ", int_scanner->value);
}
printf("\n");*/

/* simulate on immag_list (in the work area) the identification of
   imaginary states belonging to different constraints -
   at each step, one subtracts 1 to the first "cube_dim" non-zero
   elements of immag_list -
   the number of steps needed to reduce to zero all elements of
   immag_list is the minimum number of imaginary states
   to introduce, i.e. the densiest packing -
   if the current hypercube doesn't have at least as many free
   positions (#vertices - net_num) as the minimum number of imaginary
   states to introduce, "cube_dim" is incremented and the necessary
   condition is verified again, until for a suitable "cube_dim" it holds */

while (1) {

for (
int_scanner = immag_list;
int_scanner != (INT_LIST *) 0;
int_scanner = int_scanner->next
) {
int_scanner->
workarea = int_scanner->value;
}

iter_num = 0;
work     = 1;
while (work > 0) {
work        = 0;
horiz_steps = cube_dim;
int_scanner = immag_list;
while (horiz_steps > 0 && int_scanner != (INT_LIST *) 0) {
if (int_scanner->workarea > 0) {
(int_scanner->workarea)--;
work = 1;
horiz_steps--;
}
int_scanner = int_scanner->next;
}
if (work > 0) iter_num++;

/*printf("immag_list : ");
for (int_scanner = immag_list; int_scanner != (INT_LIST *) 0;
               int_scanner = int_scanner->next) {
    printf("%d ", int_scanner->workarea);
}
printf("\n");*/

}
/*printf("iter_num = %d\n", iter_num);*/
free_pos = power(2, cube_dim) - net_num;
if (free_pos >= iter_num) {       /* 3rd count. criterion satisfied */

/* the packing condition checks whether introducing imaginary
   vertices, in a hypercube with no more free positions, creates
   completed constraints sharing intersections whose cardinality
   is not a power of 2 (an unfeasible situation)                */
if (free_pos == iter_num) {          /* check packing condition */

if (
packing_cond(iter_num, net_num
)) { /* packing cond. sat. */
break;

} else {                      /* packing condition violated */
cube_dim++;
}

} else {        /* free_pos > iter_num - no other check is made */
break;
}

} else {         /* free_pos < iter_num - 3rd count. crit. violated */
cube_dim++;
}
}

if (VERBOSE) printf("cube_dim after third nec. cond. = %d\n", cube_dim);


return(cube_dim);

}



BOOLEAN packing_cond(iter_num, net_num)
        int iter_num;
        int net_num;

{

    int          i, card_inter, diff, new_card, card_c1c2();
    CONSTRAINT_E *constrptr_e, *constrptr1_e, *constrptr2_e;

    /* check all couples of constraints whose cardinality is a not a power 
       of 2 -
       if the constraints of one of them have the same cardinality and
          (cardinality of their intersection + the difference to the
           next power of 2) is not a power of 2 and
           all imaginary vertices are used for these constraints (i.e.
           "iter_num" is equal to "diff")
         return FALSE
         otherwise return TRUE                                               */
    for (i = graph_depth - 2; i >= 0; i--) {
        for (constrptr_e = graph_levels[i]; constrptr_e != (CONSTRAINT_E *) 0;
             constrptr_e = constrptr_e->next) {
            if (minpow2(constrptr_e->card) - constrptr_e->card != 0) {
                constrptr1_e      = constrptr_e;
                /*printf("constrptr1_e = %s\n", constrptr1_e->relation);*/
                for (constrptr2_e = constrptr1_e->next;
                     constrptr2_e != (CONSTRAINT_E *) 0;
                     constrptr2_e = constrptr2_e->next) {
                    diff = minpow2(constrptr2_e->card) - constrptr2_e->card;
                    if (diff != 0 &&
                        constrptr1_e->card == constrptr2_e->card &&
                        iter_num == diff) {
                        /*printf("constrptr2_e = %s\n", constrptr2_e->relation);*/
                        card_inter =
                                card_c1c2(constrptr1_e->relation, constrptr2_e->relation, net_num);
                        /*printf("card_inter = %d\n", card_inter);*/
                        new_card = card_inter + diff;
                        /*printf("card_inter + diff = %d\n", new_card);*/
                        if (minpow2(new_card) - new_card != 0) {
                            return (FALSE);
                        }
                    }
                }
            }
        }
    }

    return (TRUE);

}
