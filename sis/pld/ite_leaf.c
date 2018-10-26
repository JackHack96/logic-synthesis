#include "sis.h"
#include "pld_int.h" /* uncommented Feb 9, 1992 */
#include "ite_int.h"

static check_universal();
static void unate_split_F();

/* Here we have obtained a unate leaf and wish to construct its bdd*/
ite_vertex *
unate_ite(F, node)
sm_matrix *F;
node_t *node;
{
    ite_vertex *ite, *ite_for_cube_F(), *unate_ite(), *my_or_ite_F();
    int i, j;
    sm_matrix *M;
    sm_col  *pcol; 
    sm_row *cover;
    sm_element *p;
    array_t *array, *array_ite;
    int *phase;
    int *weights; /* -ve phase variables get slightly lower weight */
    int is_neg_phase, pos_weight;
    int orig_col_num, all_twos;

    /* Check whether the matrix has more than one cube and contains
       the universal cube. This helps to prune the tree!            */
    /*--------------------------------------------------------------*/
    if(check_universal(F)) {
	assert(st_lookup(ite_end_table, (char *)1, (char **) &ite));
        return ite;
    } 

    /* If matrix has only one row, construct ite so that the AND of the 
       variables is realised.                                          */
    /*-----------------------------------------------------------------*/
    if (F->nrows == 1){
	ite = ite_for_cube_F(F, node);
        return ite;
    } 
    /* if the matrix F corresponds to cubes with single literal each,
       construct ite with special attention to variable ordering      */
    /*----------------------------------------------------------------*/
    ite = ite_check_for_single_literal_cubes(F, node->network);
    if (ite != NIL (ite_vertex)) return ite;

    /* Construct the M matrix which is used for covering purposes
       Its number of columns may be different from F's number of
       columns, since there may be a column of all 2's in F. Then,
       M's columns get shifted to left as compared to F.         */	
    /*-----------------------------------------------------------*/
    is_neg_phase = 0; /* see if some variable in -ve phase */
    phase = ALLOC(int, F->ncols);
    M = sm_alloc();
    j = -1;
    sm_foreach_col(F,pcol) {
        all_twos = 1;
        phase[pcol->col_num] = 2; /* initialize */
        sm_foreach_col_element(pcol, p){
            if(p->user_word != (char *)2) {
                if (all_twos != 0) {
                    all_twos = 0;
                    j++;
                }
                (void) sm_insert(M, p->row_num, j);
                sm_get_col(M, j)->user_word = (char *) p->col_num;
                phase[p->col_num] = (int )p->user_word;
                if (phase[p->col_num] == 0) is_neg_phase = 1;
            }
        }
    }


    /* solve covering problem for M*/	
    /* assigning weights - in general, give slightly lower weights to the
       -ve phase variables, so that they may occur higher in the order.
       "Slight" difference, so that it is not possible to reduce the 
       weight of the cover by removing one positive guy and putting more 
       than one negative guys. . We want the
       cardinality of the cover to be minimum, with the ties being broken
       in favour of -ve wt. variables.
       If all variables of positive phase, pass NIL (int).                */
    /*--------------------------------------------------------------------*/
    if (is_neg_phase) {
        weights = ALLOC(int, M->ncols);
        pos_weight = M->ncols + 1;
        for (i = 0, j = 0; i < F->ncols; i++) {
            switch(phase[i]) {
              case 0:
                weights[j] = M->ncols;
                j++;
                break;
              case 1:  
                weights[j] = pos_weight;
                j++;
                break;
              case 2:
                break;
            }
        }
        assert(j == M->ncols);
        cover =  sm_minimum_cover(M, weights, 0, 0);
        FREE(weights);
    } else {
        cover = sm_minimum_cover(M, NIL(int), 0, 0);
    }
    /*
      sm_foreach_row_element(cover, p){
        p->user_word = (char *)phase[p->col_num];
    }
    */
    /* change the column and user_word information of the elements of cover
       to point to the columns of F (from M).                              */
    /*---------------------------------------------------------------------*/
    sm_foreach_row_element(cover, p) {
        orig_col_num = (int) (sm_get_col(M, p->col_num)->user_word);
        p->user_word = (char *) phase[orig_col_num]; /* as earlier */
        p->col_num = orig_col_num;
    }
    sm_free_space(M);
    FREE(phase);
    array = array_alloc(sm_matrix *, 0);
    /* construct sub matrices as a result of the covering solution*/	
    unate_split_F(F, cover, array);
    /* Recursively solve each of the sub matrices*/	
    array_ite = array_alloc(ite_vertex *, 0);
    for( i = 0; i < array_n(array); i++ ) {
        ite = unate_ite(array_fetch(sm_matrix *, array, i), node);
        array_insert_last(ite_vertex *, array_ite, ite);
    }
    if(array_n(array_ite) != cover->length) {
        (void)fprintf(misout, 
                      "error array length (of ites) and minucov solution do not match \n");
        exit(1);
    }
    ite = my_or_ite_F(array_ite, cover, array, node->network);  
    /* Free up the area alloced for array and the Fi s.*/    
    for(i = 0; i< array_n(array); i++ ){
        sm_free_space(array_fetch(sm_matrix *, array, i));
    }
    array_free(array);
    array_free(array_ite);
    sm_row_free(cover);
    return ite;
}


/* checks for a row of 2's. returns 1 if true else 0*/ 
static int 
check_universal(F)
sm_matrix *F;
{
    sm_row *row;
    sm_element *p;
    int var;
    
    sm_foreach_row(F, row) {
	var = 1;
	sm_foreach_row_element(row, p){
	    if(p->user_word != (char *)2) {
		var = 0;
	    }
	}
	if (var) return 1;
    }
    return 0;
}






/* constructs a set of sub matrices from F such that */
static void
unate_split_F(F, cover, array)
sm_matrix *F;
sm_row *cover;
array_t *array;
{
    sm_element *p, *q;
    sm_col *col;
    sm_matrix *Fi;
    sm_row *row, *F_row, *my_row_dup();
    int col_num;

/* Initialise row->user_word == NUll to prevent duplication of cubes
 * if 2 elements in the cover contain the same cube! Need for optimisation*/

    sm_foreach_row(F, row) {
	row->user_word = (char *)0;
    }
    sm_foreach_row_element(cover, q) {
	col_num = q->col_num;
	col = sm_get_col(F, col_num);
	Fi = sm_alloc();
	sm_foreach_col_element(col, p) {
	    F_row = sm_get_row(F, p->row_num);
	    if((p->user_word != (char *)2)&&(F_row->user_word == (char *)0)) { 
		my_sm_copy_row(F_row, Fi, col_num, F);
		if(ACT_ITE_DEBUG > 2) {
		    my_sm_print(Fi); 
		}
		F_row->user_word = (char *)1;
	    }
	}
	array_insert_last(sm_matrix *, array, Fi);
    }
}

