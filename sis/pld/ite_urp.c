#include "sis.h"
#include "pld_int.h" 
#include "ite_int.h"

/* int ACT_ITE_FIND_KERNEL; */
st_table *ite_end_table;
static sm_matrix *build_F();
static ite_vertex* urp_F();
static int good_bin_var();
static int find_var();
static void update_F();
extern my_sm_copy_row(), my_sm_print();

/* We need this to initialise the 0 and 1 ite's and store them 
 * in a table so they can be easily accessed
   Note: node must be in the network (the only use of network is to get the fanin nodes
   from the fanin names: so the restriction can be removed in future, if needed)*/ 

make_ite(node)
node_t *node;
{
    sm_matrix *F;
    ite_vertex *ite, *vertex;
    node_function_t node_fun;

    assert(node->type != PRIMARY_INPUT && node->type != PRIMARY_OUTPUT);
    /* handle trivial cases */
    /*----------------------*/
    node_fun = node_function(node);
    switch(node_fun) {
      case NODE_0: 
        ite = ite_alloc();
        ite->value = 0;
        ACT_ITE_ite(node) = ite;
        return;
      case NODE_1:
        ite = ite_alloc();
        ite->value = 1;
        ACT_ITE_ite(node) = ite;
        return;
    }
        
    /* ACT_ITE_FIND_KERNEL = 1; */
    /* Init a global lookup table for terminal vertices*/
    ite_end_table = st_init_table(st_ptrcmp, st_ptrhash);
    vertex = ite_alloc();
    vertex->value = 0;
    (void) st_insert(ite_end_table, (char *)0, (char *)vertex);
    vertex = ite_alloc();
    vertex->value = 1;
    (void) st_insert(ite_end_table, (char *)1, (char *)vertex);
/* initialize Matrix for node*/
    F = build_F(node);
    if(ACT_ITE_DEBUG > 2) my_sm_print(F);
/* construct act for node*/
    /* ite = act_ite_build(F, node); */
    ite = urp_F(F, node); 
/* Free the st_table now*/
    st_free_table(ite_end_table);
    ACT_ITE_ite(node) = ite;
    if(ACT_ITE_DEBUG > 2) {
	ite_print_dag(ite); 
    }
/* Free up the space*/
    sm_free_space(F);
}



/* Constructs the cover of the node. We use the sm package because we 
   need to solve a column covering problem at the unate leaf*/
/* no changes needed */
static
sm_matrix *
build_F(node)
node_t *node;
{
    int i, j;
    node_cube_t cube;
    node_literal_t literal;
    sm_matrix *F;
    node_t *fanin;
    sm_col *p_col;

    F = sm_alloc();
    
    for(i = node_num_cube(node)-1; i >= 0; i--) {
	cube = node_get_cube(node, i);
	foreach_fanin(node, j, fanin) {
	    literal = node_get_literal(cube, j);
	    switch(literal) {
	      case ONE:
		 sm_insert(F, i, j)->user_word = (char *)1;
		break;
	      case ZERO:
		 sm_insert(F, i, j)->user_word = (char *)0;
		break;
	      case TWO:
		 sm_insert(F, i, j)->user_word = (char *)2;
		break;
	      default:
		(void)fprintf(misout, "error in F construction \n");
		break;
	    }
	}
    }
/* STuff the fanin names in the user word for each column*/
    foreach_fanin(node, j, fanin) {
	p_col = sm_get_col(F, j);
	p_col->user_word = node_long_name(fanin);
    }
    return F;
}



/* Recurse !! using Unate recursive paradigm(modified)*/
static
ite_vertex *
urp_F(F, node)
sm_matrix *F;
node_t *node;
{
    int b_col, ex, a_col_special, b_col_special;
    ite_vertex *ite_THEN, *ite_ELSE, *ite_IF;
    ite_vertex *ite_ELSE_IF, *ite_ELSE_THEN, *ite_ELSE_ELSE;
    ite_vertex  *my_shannon_ite(), *ite, *unate_ite(), *ite_1, *ite_0;
    sm_matrix *F_IF, *F_ELSE_THEN, *F_ELSE_ELSE, *F_new_IF, *act_sm_complement_special_case();
    sm_element *p;
    char *b_name;
    int value, special_case;
    node_t *fanin;
   
    /* if there are no cubes in the matrix F, return 0 */
    /*-------------------------------------------------*/
    if (F->nrows == 0) {
        assert(st_lookup(ite_end_table, (char *)0, (char **) &ite_0));
        return ite_0;
    }
    

    ex = 0;
    b_col = good_bin_var(F, &ex);   
    if(ACT_ITE_DEBUG > 2) {
	(void)fprintf(misout, "The most binate col is %d\n", b_col);
    }
    if(b_col != -1) {
        b_name = sm_get_col(F, b_col)->user_word;
        fanin = network_find_node(node->network, b_name);
        assert(fanin != NIL (node_t));
	if(!ex) {
	    ite_split_F(F, b_col, &F_IF, &F_ELSE_THEN, &F_ELSE_ELSE);
	    if(ACT_ITE_DEBUG > 2){
	        (void)fprintf(misout, "factored wrt %d'\n", b_col);
	        my_sm_print(F_IF);
	    }
            /* check if F_IF is of the form a' or a' b'. If so, put a + b (a) as the IF child,
               and swap the THEN and ELSE children (done slightly later)                     */
            /*-------------------------------------------------------------------------------*/
            if (special_case = act_mux_inputs_special_case(F_IF, &a_col_special, &b_col_special)) {
                F_new_IF = act_sm_complement_special_case(F_IF, special_case, a_col_special, b_col_special);
                sm_free_space(F_IF);
                F_IF = F_new_IF;
            }
	    ite_IF = urp_F(F_IF, node);
	    if(ACT_ITE_DEBUG > 2){
	        (void)fprintf(misout, "factored wrt %d \n", b_col);
	        my_sm_print(F_ELSE_THEN);
	    }
	    ite_ELSE_THEN = urp_F(F_ELSE_THEN, node);
	    if(ACT_ITE_DEBUG > 2){
	        (void)fprintf(misout, "factored wrt %d'\n", b_col);
	        my_sm_print(F_ELSE_ELSE);
	    }
	    ite_ELSE_ELSE = urp_F(F_ELSE_ELSE, node);
            ite_ELSE_IF = ite_literal(b_col, b_name, fanin);
	    ite_ELSE = my_shannon_ite(ite_ELSE_IF, ite_ELSE_THEN, ite_ELSE_ELSE);

            /* if there are no rows in the IF part, can remove extra vertices */
            /*----------------------------------------------------------------*/
            if (F_IF->nrows == 0) {
                ite = ite_ELSE;
            } else {
                assert(st_lookup(ite_end_table, (char *) 1, (char **) &ite_1));
                if (special_case) {
                    ite = my_shannon_ite(ite_IF, ite_ELSE, ite_1);
                } else {
                    ite = my_shannon_ite(ite_IF, ite_1, ite_ELSE);
                }
            }
	    sm_free_space(F_IF);
	    sm_free_space(F_ELSE_THEN);
	    sm_free_space(F_ELSE_ELSE);

	} else {

            /* Common variable to all cubes */
            /*------------------------------*/
	    p = sm_find(F, 0, b_col);
	    value = (int )p->user_word;
	    if((value != 1) && (value != 0)) {
		(void)fprintf(misout, "ERROR in Common Variable\n");
                exit(1);
	    }
            assert(st_lookup(ite_end_table, (char *)0, (char **) &ite_0));
            update_F(F, b_col);
            if(ACT_ITE_DEBUG > 2){
                my_sm_print(F);
            }
            ite_IF = ite_literal(b_col, b_name, fanin);
	    if(value == 1){
                ite_THEN = urp_F(F, node);
                ite = my_shannon_ite(ite_IF, ite_THEN, ite_0);
            } 
	    if(value == 0) {
		ite_ELSE = urp_F(F, node);
                ite = my_shannon_ite(ite_IF, ite_0, ite_ELSE); /* saving an inverter */
	    } 	    
	}
    } 
    else 
        /* unate cover */
        /*-------------*/
        {
            ite = unate_ite(F, node);
        }
    return ite;
	
}


/* Choose good variable for co-factoring*/
static int   
good_bin_var(A, ex)
sm_matrix *A;
int *ex;

{
    int *o, *z, i, var;
    sm_col *pcol;
    sm_element *p;
    int unate;

    o = ALLOC(int, A->ncols);
    z = ALLOC(int, A->ncols);
    for( i =0; i< A->ncols; i++) {
	o[i] = 0;
	z[i] = 0;
    }
  
    /* find number of times each variable appears in
       uncomplemented (o[]) and complemented (z[]) phase */
    /*---------------------------------------------------*/

    i = 0;
    sm_foreach_col(A, pcol){ 
	sm_foreach_col_element(pcol, p) {
	    switch((int )p->user_word) {
	      case 1:
		o[i]++;
		break;
	      case 2:
		break;
	      case 0:
		z[i]++;
		break;
	      default:
		(void)fprintf(misout, "Err in sm->user_word\n");
		break;
	    }
	}
	i++;
    }

    /* check if a unate cover */
    /*------------------------*/
    unate = 1;
    for (i = 0; i < A->ncols; i++) {
        if (o[i] == 0 || z[i] == 0) continue;
        unate = 0;
        break;
    }
    if (unate) {
        FREE(o);
        FREE(z);
        return (-1);
    }

    /* look for a common variable in same phase in all the cubes */
    /*-----------------------------------------------------------*/
    for (i = 0; i < A->ncols; i++) {
        if(o[i] == A->nrows){ 
	    *ex = 1;
	    FREE(o);
	    FREE(z);	
	    return i;
	}
	if (z[i] == A->nrows){
	    *ex = 1;
	    FREE(o);
	    FREE(z);	
	    return i;
	}
    }
    /* find the most binate variable */
    /*-------------------------------*/
    var  = find_var(o, z, A->ncols);
    FREE(o);
    FREE(z);	
    return var; 
}
	

    
static int 
find_var(o, z, size)
int *o, *z, size;
{
    int i, b_b= 0, b_d= MAXINT, var;
    
    var = -1;
    for(i = 0; i < size; i++) {
	if((o[i] > 0) && (z[i] > 0)){
	    if(b_b < o[i]+ z[i]){
		b_b = o[i] + z[i];
		b_d = abs(o[i]-z[i]);
		var = i;
	    } else if(b_b == o[i]+ z[i]) {
		if(abs(o[i]-z[i]) < b_d) {
		    b_d = abs(o[i]-z[i]);
		    var = i;
		}
	    }
	}	
    }
    return var;
}
				

/* Divide matrix into 3 co-factored matrices - THENELSE is the matrix
   with cubes that have 2 in the b_col, THEN cubes have 1, ELSE have 0 */
void
ite_split_F(F, b_col, F_THENELSE, F_THEN, F_ELSE)
sm_matrix *F, **F_THENELSE, **F_THEN, **F_ELSE;
int b_col;
{
    sm_matrix *Fthenelse, *Fthen, *Felse;
    sm_row  *F_row;
    sm_col *pcol;
    sm_element *p; 
    
    Fthen = sm_alloc();
    Felse = sm_alloc();
    Fthenelse = sm_alloc();
    pcol = sm_get_col(F, b_col);
    sm_foreach_col_element(pcol, p) {
	F_row = sm_get_row(F, p->row_num);
	switch((int )p->user_word) {
	  case 0:
	    my_sm_copy_row(F_row, Felse, b_col, F);
	    break;
	  case 1:
	    my_sm_copy_row(F_row, Fthen, b_col, F);
	    break;
	  case 2:
	    my_sm_copy_row(F_row, Fthenelse, b_col, F);
	    break;
	  default:
	    (void)fprintf(misout, "error in duplicating row\n");
	    break;
	}
    }
    *F_THENELSE = Fthenelse;
    *F_ELSE = Felse;
    *F_THEN = Fthen;
}

/*
my_sm_copy_row(F_row, F, col,Fp)
sm_row *F_row;
sm_matrix *F, *Fp;
int  col; 
{
    int j;
    int row;

    row = F->nrows;
    for(j = 0; j< F_row->length; j++){
	if(j != col) {
	    sm_insert(F, row, j)->user_word = 
	                    sm_row_find(F_row, j)->user_word;
	    	} else if(j == col) {
	     sm_insert(F, row, j)->user_word = (char *)2;
	}
	sm_get_col(F, j)->user_word = sm_get_col(Fp, j)->user_word;
    }
}

*/
			     
/* For debug */
/*
my_sm_print(F)
sm_matrix *F;
{
    sm_row *row;
    sm_element *p;

    (void)fprintf(misout, "Printing F matrix\n");
    sm_foreach_row(F, row) {
	sm_foreach_row_element(row, p){
	    (void)fprintf(misout, "%d", (int *)p->user_word);
	}
	(void)fprintf(misout, "\n");
    }
}

*/

/* Set co-factored variable to 2*/
static void
update_F(F, b_col)
sm_matrix *F;
int b_col;
{
    sm_col *p_col;
    sm_element *p;
    
    p_col = sm_get_col(F, b_col);
    sm_foreach_col_element(p_col, p){
	p->user_word = (char *)2;
    }
}

/*-------------------------------------------------------------------------------
  Given F, checks if F implements function of the form a'b' or a'. Returns 2 if
  former, 1 if latter, 0 otherwise. Returns column numbers of a and b 
  (whichever applicable) in a_col and b_col.
--------------------------------------------------------------------------------*/
int
act_mux_inputs_special_case(F, a_col, b_col)
  sm_matrix *F;
  int *a_col, *b_col;
{

  sm_element *p;
  int count_neg;

  if (F->nrows != 1) return 0;
  
  count_neg = 0;
  sm_foreach_row_element(F->first_row, p) {
      switch ((int) p->user_word) {
        case 0: 
          count_neg++;
          if (count_neg > 2) return 0;
          if (count_neg == 1) *a_col = p->col_num;
          else *b_col = p->col_num;
          break;
        case 1:
          return 0;
        case 2:
          break;
        default:
          (void)fprintf(misout, "error in row\n");
          break;
      }
  }
  return count_neg;
}

/*----------------------------------------------------------------------------
  If F = a' b', obtain the matrix for a + b. If F = a', obtain matrix for a.
------------------------------------------------------------------------------*/
sm_matrix *
act_sm_complement_special_case(F, special_case, a_col, b_col) 
  sm_matrix *F;
  int special_case, a_col, b_col;
{
  sm_matrix *not_F;
  sm_row *r;
  sm_element *not_p;
  int row, j;

  not_F = sm_alloc();
  row = not_F->nrows;
  r = F->first_row;
  for (j = 0; j< r->length; j++){
      not_p = sm_insert(not_F, row, j);
      if (j != a_col) {
          not_p->user_word = (char *) 2;
      } else {
          not_p->user_word = (char *) 1;
      }
      sm_get_col(not_F, j)->user_word = sm_get_col(F, j)->user_word;
  }
  if (special_case == 1) return not_F;

  /* special case = 2, insert one more row */
  /*---------------------------------------*/
  row = not_F->nrows;
  for (j = 0; j< r->length; j++){
      not_p = sm_insert(not_F, row, j);
      if (j != b_col) {
          not_p->user_word = (char *) 2;
      } else {
          not_p->user_word = (char *) 1;
      }
  }
  return not_F;
}

          
