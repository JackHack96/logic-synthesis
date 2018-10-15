/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/ihybrid_code.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:09 $
 *
 */
/******************************************************************************
*                       Approximate encoding algorithm                        *
*                                                                             *
******************************************************************************/

#include "nova.h"

ihybrid_code(net,symblemes,net_name,net_num,code_length)
CONSTRAINT **net;
SYMBLEME *symblemes;
char *net_name;
int net_num;
int code_length;

{

    int dim_cube,iter_num,i,approx_work;
    BOOLEAN code_found,found_coding,backtrack_down();
    CONSTRAINT *update_net(),*last_constr;

    code_found = FALSE;
    found_coding = FALSE;

    dim_cube = mylog2(minpow2(net_num));
    approx_work = 0;

    iter_num = num_constr(net);

    for (i = 0; i < iter_num; i++) {
        last_constr = update_net(net);
        /*printf("last_constr->relation = %s\n", last_constr->relation);*/
        exact_graph(net,net_num);
        faces_alloc(net_num);
    
        /* decides the categories and bounds of the faces (func. of dim_cube) */
        faces_set(dim_cube);
    
        /* finds an encoding (if any) for the current configuration */ 
        code_found = backtrack_down(net_num,dim_cube);
    
	approx_work += cube_work;
    
        if (code_found == FALSE) {
	    last_constr->attribute = PRUNED;
            if (VERBOSE) printf("\nNo code was found\n");
        } else {
	    found_coding = TRUE;
            faces_to_codes(symblemes,net_num);
            if (VERBOSE) printf("\nA code was found\n");
        }
    }
    if (VERBOSE) {
        printf("\nApprox_work = %d\n", approx_work);
        constr_satisfaction(net,symblemes,dim_cube,"exact");
    }
    if (found_coding == FALSE) random_patch(symblemes,net_num);

    /* add a new dimension to the current cube to satisfy more constraints */
    if (code_length > dim_cube) {
        project_code(net,symblemes,net_name,net_num,code_length);
    }

    if (strcmp(net_name,"Inputnet") != 0) {
        exact_rotation();
        exact_output();
    }

}



project_code(net,symblemes,net_name,net_num,code_length)
CONSTRAINT **net;
SYMBLEME *symblemes;
char *net_name;
int net_num;
int code_length;

{

    CONSTRAINT *constrptr,*inspect_net(),*last_constr;
    int *mask_constr,*mask_move;
    int state_sel(),face_action(),state_pos,action,mindim_cube,i,k;
    BOOLEAN feasible_move();

    if (VERBOSE) printf("\nSTARTS CUBE EXPANSION\n");

    if ( (mask_constr = (int *) calloc((unsigned)net_num,sizeof(int))) == (int *) 0 ) {
	fprintf(stderr,"Insufficient memory for mask_constr in project_code");
    }
    if ( (mask_move = (int *) calloc((unsigned)net_num,sizeof(int))) == (int *) 0 ) {
	fprintf(stderr,"Insufficient memory for mask_move in project_code");
    }

    mindim_cube = mylog2(minpow2(net_num));

    for (i = mindim_cube+1; i <= code_length; i++) {
      if (VERBOSE) printf("\nCURRENT CUBE DIMENSION = %d\n", i);

      elong_code(symblemes,net_num);
  
      /* all PRUNED constraints become NOTUSED */
      for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
                                                    constrptr = constrptr->next) {
          if (constrptr->attribute == PRUNED) {
              constrptr->attribute = NOTUSED;
	    }
      }
  
      /* select the NOTUSED constraint of maximum weight with at least
         one raisable state - NOTUSED constraints with no raisable state
         are PRUNED */
      last_constr = inspect_net(net,symblemes,net_num);
      while (last_constr != (CONSTRAINT *) 0) {
  
          /*printf("last_constr->relation = %s\n", last_constr->relation);*/
          for (k = 0; k < net_num; k++) {
	      if (last_constr->relation[k] == ONE) mask_constr[k] = 1;
	      if (last_constr->relation[k] == ZERO) mask_constr[k] = 0;
	      mask_move[k] = 0;
          }
      
          /* select the raisable state more frequent in NOTUSED constraints */
          state_pos = state_sel(net,symblemes,net_num,mask_constr);
          while (state_pos != -1) {
              /*printf("state_pos = %d\n", state_pos);*/
              /* assign "state_pos" to the new dimension and record the move */
	      mask_constr[state_pos] = 0;
              raise_code(symblemes,state_pos);
	      mask_move[state_pos] = 1;
	      /* check if the move makes NOTUSED an USED constraint 
	         (by invoking feasible_move() inside face_action() -
	         compute the face spanned by the states of last_constr 
	         and decide which action to take according to the new codes */
	      action = 
               face_action(net,symblemes,net_num,last_constr,mask_constr,state_pos);
	      if (action == 1) { /* last_constr USED */
                  last_constr->attribute = USED;
	          /*printf("%s was USED\n", last_constr->relation);*/
	          break;
	      }
	      if (action == 2) { /* last_constr PRUNED - cancell all moves */
                  lower_codes(symblemes,net_num,mask_move);
                  last_constr->attribute = PRUNED;
	          break;
	      }
	      if (action == 3) { /* cancell the last move */
                  lower_code(symblemes,state_pos);
	          mask_move[state_pos] = 0;
	      }
	      if (action == 4) { /* accept the last move */
	      }
	      state_pos = state_sel(net,symblemes,net_num,mask_constr);
          } 
	
          /* verify if some UNSATISFIED constraints were USED as a side-effect */
          moreconstr_used(net,symblemes,net_num);
          /*shownet(net,net_name,"AFTER CONSTRAINT CYCLE");*/
          last_constr = inspect_net(net,symblemes,net_num);
          /*show_exactcode(symblemes,net_num);*/
  
      }
      if (VERBOSE) {
          shownet(net,net_name,"AFTER CUBE EXPANSION");
          show_exactcode(symblemes,net_num);
          constr_satisfaction(net,symblemes,i,"exact");
      }
    }

}



elong_code(symblemes,net_num)
SYMBLEME *symblemes;
int net_num;

{

    int i,old_length;

    old_length = strlen(symblemes[0].exact_code);

    for (i = 0; i < net_num; i++) {
        symblemes[i].exact_code[old_length] = ZERO;
        symblemes[i].exact_code[old_length+1] = '\0';
    }

}



CONSTRAINT *inspect_net(net,symblemes,net_num)
CONSTRAINT **net;
SYMBLEME *symblemes;
int net_num;

{

    CONSTRAINT *constrptr,*inspect_constr;
    int max_weight,length,eligible,i;

    max_weight = 0;
    length = strlen(symblemes[0].exact_code);
    inspect_constr = (CONSTRAINT *) 0;

    for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
                                                  constrptr = constrptr->next) {
        if (constrptr->attribute == NOTUSED) {
	    eligible = 0;
	    for (i = 0; i < net_num; i++) {
		if (constrptr->relation[i] == ONE  && 
				  symblemes[i].exact_code[length - 1] == ZERO) {
		    eligible = 1;
		    break;
                }
	    }
	    if (eligible == 0) {
	        /* NOTE: bug corrected here - test it */
		constrptr->attribute = PRUNED;
	    }
	    if (eligible == 1  &&  constrptr->weight > max_weight) {
	        max_weight = constrptr->weight;
	        inspect_constr = constrptr;
            }
	}
    }

    return(inspect_constr);

}



int state_sel(net,symblemes,net_num,mask)
CONSTRAINT **net;
SYMBLEME *symblemes;
int net_num;
int *mask;

{

    CONSTRAINT *constrptr;
    int state_pos,i_freq,max_freq,length,i;

    length = strlen(symblemes[0].exact_code);

    max_freq = 0;
    state_pos = -1;

    for (i = 0; i < net_num; i++) {
	/* state in the first half of the hypercube */
	if (mask[i] == 1 && symblemes[i].exact_code[length-1] == ZERO) {
	    i_freq = 0;
            for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
                                                  constrptr = constrptr->next) {
               if (constrptr->attribute == NOTUSED && 
                                    constrptr->relation[i] == ONE) {
		   i_freq++;
	       }
            }
	    if (i_freq > max_freq) {
	        max_freq = i_freq; 
		state_pos = i;
            }
	}
    }
    /*mask[state_pos] = 0;*/

    return(state_pos);

}



raise_code(symblemes,state_pos)
SYMBLEME *symblemes;
int state_pos;

{

    int length;

    length = strlen(symblemes[0].exact_code);

    symblemes[state_pos].exact_code[length-1] = ONE;

}



lower_code(symblemes,state_pos)
SYMBLEME *symblemes;
int state_pos;

{

    int length;

    length = strlen(symblemes[0].exact_code);

    symblemes[state_pos].exact_code[length-1] = ZERO;

}



lower_codes(symblemes,net_num,mask_move)
SYMBLEME *symblemes;
int net_num;
int *mask_move;

{

    int i,length;

    length = strlen(symblemes[0].exact_code);

    for (i = 0; i < net_num; i++) {
	if (mask_move[i] == 1) {
            symblemes[i].exact_code[length-1] = ZERO;
	}
    }

}



int face_action(net,symblemes,net_num,constrptr,mask,state_pos)
CONSTRAINT **net;
SYMBLEME *symblemes;
int net_num;
CONSTRAINT *constrptr;
int *mask;
int state_pos;

{

    char *face;
    char bit1;
    BOOLEAN last_state;
    int i,j,satisfied,face_length,new_cube,length;

    satisfied = 0;
    length = strlen(symblemes[0].exact_code);
    last_state = TRUE;
    if (satisfied == 0) {
	for (i = 0; i < net_num; i++) {
	    if (mask[i] == 1 && symblemes[i].exact_code[length-1] == ZERO) {
		last_state = FALSE;
		break;
            }
	}
    }

    if (feasible_move(net,symblemes,net_num,state_pos) == FALSE) {
        if (last_state == TRUE) {
	    /*printf("CANCELL ALL MOVES\n");*/
	    return(2);
        }
        if (last_state == FALSE) {
	    /*printf("CANCELL THE LAST MOVE\n");*/
	    return(3);
        }
    }

    face_length = strlen(symblemes[0].exact_code);
    if ((face = (char *) calloc((unsigned)face_length+1,sizeof(char))) == (char *) 0) {
	fprintf(stderr,"Insufficient memory for face in face_action");
    }

    /* scans all boolean coordinates */
    for (i = 0; i < face_length; i++) {

        /* stores in bit1 the "i-th" bit of the exact_code of the "j-th" 
	   symbleme ( which belongs to constrptr )                       */
        for ( j = 0; j < net_num; j++) {
            if ( constrptr->relation[j] == ONE ) {
	        bit1 = symblemes[j].exact_code[i]; 
            }
	}

        /* if there are two symblemes in constrptr different 
	    on the i-th boolean coordinate face[i] gets a * (don't care)  
	    otherwise face[i] gets the common coordinate value            */
        for ( j = 0; j < net_num; j++) {
            if ( constrptr->relation[j] == ONE ) {
		if ( bit1 != symblemes[j].exact_code[i]) {
		    face[i] = '*'; 
                    break;
		} else {
                    face[i] = bit1;
		}
            }    
	}

    }     
    /*printf("the spanned face is %s\n", face);*/

    /* if the exact_code of any other symbleme ( i.e. a symbleme not 
       contained in constrptr ) is covered by face , constrptr 
       has not been satisfied                                         */
    satisfied = 1;
    /* new_cube set to 1 if the face assigned to constrptr covers wrongly 
       a state coded in the new half of the hypercube                     */
    new_cube = 0; 
    for (j = 0; j < net_num; j++) {
        if ( constrptr->relation[j] == ZERO ) {
            if (inclusion(symblemes[j].exact_code,face,face_length) == 1) {
                /*printf("the symbleme %s (%d)  is covered by face %s\n",symblemes[j].name, j, face);*/
	        satisfied  = 0;
		if (symblemes[j].exact_code[face_length-1] == ONE) new_cube = 1;
	        /*printf("UNSATISFIED\n");*/
            }
	}
    }
    if (satisfied == 1) {
	/*printf("SATISFIED\n");*/
	return(1);
    }
    if (satisfied == 0  &&  last_state == TRUE) {
	/*printf("CANCELL ALL MOVES\n");*/
	return(2);
    }
    if (satisfied == 0  &&  last_state == FALSE  &&  new_cube == 1) {
	/*printf("CANCELL THE LAST MOVE\n");*/
	return(3);
    }
    if (satisfied == 0  &&  last_state == FALSE  &&  new_cube == 0) {
	/*printf("ACCEPT THE LAST MOVE\n");*/
	return(4);
    }

    return(0);

}



BOOLEAN feasible_move(net,symblemes,net_num,state)
CONSTRAINT **net;
SYMBLEME *symblemes;
int net_num;
int state;

{

    CONSTRAINT *constrptr;
    char *face;
    char bit1;
    int i,j,satisfied,face_length;

    /* empty net : do nothing */
    if ((*net) == (CONSTRAINT *) 0) {
	return(FALSE);
    }

    face_length = strlen(symblemes[0].exact_code);
    if ((face = (char *) calloc((unsigned)face_length+1,sizeof(char))) == (char *) 0) {
	fprintf(stderr,"Insufficient memory for face in feasible_move");
    }

    /* scans all constraints */
    for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
					   constrptr = constrptr->next) {

      if (constrptr->attribute == USED) {
        /*printf("The constraint is %s\n", constrptr->relation);*/

        /* scans all boolean coordinates */
        for (i = 0; i < face_length; i++) {

            /* stores in bit1 the "i-th" bit of the best_code of the "j-th" 
	       symbleme ( which belongs to the current constraint )         */
	    for ( j = 0; j < net_num; j++) {
		if ( constrptr->relation[j] == ONE ) {
	            bit1 = symblemes[j].exact_code[i]; 
		}
	    }

            /* if there are two symblemes in the current constraint different 
	       on the i-th boolean coordinate face[i] gets a * (don't care)  
	       otherwise face[i] gets the common coordinate value            */
	    for ( j = 0; j < net_num; j++) {
		if ( constrptr->relation[j] == ONE ) {
		    if ( bit1 != symblemes[j].exact_code[i]) {
		        face[i] = '*'; 
			break;
		    } else {
			  face[i] = bit1;
		    }
                }    
	    }

	}     

        /*printf("the spanned face is %s\n", face);*/

	/* if the best_code of any other symbleme ( i.e. a symbleme not 
	   contained in constraint ) is covered by face , the current
	   constraint has not been satisfied                             */
        for (j = 0; j < net_num; j++) {
            if ( constrptr->relation[j] == ZERO ) {
		if (inclusion(symblemes[j].exact_code,face,face_length) == 1) {
                    /*printf("the symbleme %s (%d)  is covered by face %s\n",symblemes[j].name, j, face);*/
		    satisfied  = 0;
		    /*printf("UNSATISFIED\n");*/
		    break;
		}
	    }
	}
	/* no other constraint covered by face */
	if (j == net_num) {
	    satisfied = 1;
	    /*printf("SATISFIED\n");*/
	}
	if (satisfied == 0) {
	    /*printf("%s would be NOTUSED\n", constrptr->relation);*/
            lower_code(symblemes,state);
	    return(FALSE);
	}
    }

  }

  return(TRUE);

}



moreconstr_used(net,symblemes,net_num)
CONSTRAINT **net;
SYMBLEME *symblemes;
int net_num;

{

    CONSTRAINT *constrptr;
    char *face;
    char bit1;
    int i,j,satisfied,face_length;

    /* empty net : do nothing */
    if ((*net) == (CONSTRAINT *) 0) {
	return;
    }

    face_length = strlen(symblemes[0].exact_code);
    if ((face = (char *) calloc((unsigned)face_length+1,sizeof(char))) == (char *) 0) {
	fprintf(stderr,"Insufficient memory for face in moreconstr_used");
    }

    /* scans all constraints */
    for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
					   constrptr = constrptr->next) {

      if (constrptr->attribute == NOTUSED || constrptr->attribute == PRUNED) {
        /*printf("The constraint is %s\n", constrptr->relation);*/

        /* scans all boolean coordinates */
        for (i = 0; i < face_length; i++) {

            /* stores in bit1 the "i-th" bit of the best_code of the "j-th" 
	       symbleme ( which belongs to the current constraint )         */
	    for ( j = 0; j < net_num; j++) {
		if ( constrptr->relation[j] == ONE ) {
	            bit1 = symblemes[j].exact_code[i]; 
		}
	    }

            /* if there are two symblemes in the current constraint different 
	       on the i-th boolean coordinate face[i] gets a * (don't care)  
	       otherwise face[i] gets the common coordinate value            */
	    for ( j = 0; j < net_num; j++) {
		if ( constrptr->relation[j] == ONE ) {
		    if ( bit1 != symblemes[j].exact_code[i]) {
		        face[i] = '*'; 
			break;
		    } else {
			  face[i] = bit1;
		    }
                }    
	    }

	}     

        /*printf("the spanned face is %s\n", face);*/

	/* if the best_code of any other symbleme ( i.e. a symbleme not 
	   contained in constraint ) is covered by face , the current
	   constraint has not been satisfied                             */
        for (j = 0; j < net_num; j++) {
            if ( constrptr->relation[j] == ZERO ) {
		if (inclusion(symblemes[j].exact_code,face,face_length) == 1) {
                    /*printf("the symbleme %s (%d)  is covered by face %s\n",symblemes[j].name, j, face);*/
		    satisfied  = 0;
		    /*printf("UNSATISFIED\n");*/
		    break;
		}
	    }
	}
	/* no other constraint covered by face */
	if (j == net_num) {
	    satisfied = 1;
	    /*printf("SATISFIED\n");*/
	}
	if (satisfied == 1) {
	    constrptr->attribute = USED;
	    /*printf("%s was USED as a side-effect\n", constrptr->relation);*/
	}
    }

  }

}



/*******************************************************************************
*      Sets the categories & mindim,curdim and maxdim of the faces that can    *
*      be assigned to the constraints (function of dim_cube)                   *
*******************************************************************************/

faces_set(dim_cube)
int dim_cube;

{

    FATHERS_LINK *ft_scanner;
    CONSTRAINT_E *constrptr,*supremum_ptr;
    int i,fathers_num;

    /* cube_work (global variable) counts the # of codes tried in the current 
       cube */
    cube_work = 0;

    /*	The constraints are classified in categories :
	1) the complete constraint is of category 0;
	2) constraints whose unique father is the complete constraint are
	   of category 1;
	3) constraints whose unique father is not the complete constraint 
	   are of category 3;
        4) constraints with at least two fathers are of category 2 -

	The dimensions of the faces of the constraints are set on the basis
	of the minimum feasible dimension and of current hypercube dimension */

    /* takes care of the complete constraint */
    supremum_ptr = graph_levels[graph_depth-1];
    supremum_ptr->face_ptr->category = 0;
    /*printf("constraint %s is of cat. %d ",supremum_ptr->relation,
				            supremum_ptr->face_ptr->category);*/
    supremum_ptr->face_ptr->mindim_face = dim_cube;
    supremum_ptr->face_ptr->maxdim_face = dim_cube;
    supremum_ptr->face_ptr->curdim_face = dim_cube;
    /*printf("curdim_face = %d\n", supremum_ptr->face_ptr->curdim_face);*/

    for (i = graph_depth-2; i >= 0; i--) {
	for (constrptr = graph_levels[i]; constrptr != (CONSTRAINT_E *) 0;
				                 constrptr = constrptr->next ) {

	    fathers_num = 0;
            for (ft_scanner = constrptr->up_ptr; 
	      ft_scanner != (FATHERS_LINK *) 0; ft_scanner = ft_scanner->next) {
	        fathers_num++;
	    }

            /* if the constraint has only one father */
	    if (fathers_num == 1) {
	        /* if the father is the complete constraint */
	        if (constrptr->up_ptr->constraint_e == supremum_ptr) {
                    constrptr->face_ptr->category = 1;
		    /*printf("constraint %s is of cat. %d ",constrptr->relation,
				               constrptr->face_ptr->category);*/
                    constrptr->face_ptr->mindim_face = 
						 mylog2(minpow2(constrptr->card));
		    /*printf("mindim_face = %d ",
				            constrptr->face_ptr->mindim_face);*/
                    constrptr->face_ptr->curdim_face = 
                                               constrptr->face_ptr->mindim_face;
		    /*printf("curdim_face = %d \n",
				            constrptr->face_ptr->curdim_face);*/
                    constrptr->face_ptr->maxdim_face = 
                                               constrptr->face_ptr->mindim_face;
		    /*printf("maxdim_face = %d \n",
				            constrptr->face_ptr->maxdim_face);*/
	        } else {
                    constrptr->face_ptr->category = 3;
		    /*printf("constraint %s is of cat. %d ",constrptr->relation,
				               constrptr->face_ptr->category);*/
                    constrptr->face_ptr->mindim_face = 
		         	 	         mylog2(minpow2(constrptr->card));
                    constrptr->face_ptr->curdim_face = 
                                               constrptr->face_ptr->mindim_face;
		    /*printf("mindim_face = %d\n",
				            constrptr->face_ptr->mindim_face);*/
	        }
	    }

            /* if the constraint has at least two fathers */
	    if (fathers_num > 1) {
                    constrptr->face_ptr->category = 2;
		    /*printf("constraint %s is of cat. %d ",constrptr->relation,
				               constrptr->face_ptr->category);*/
                    constrptr->face_ptr->mindim_face = 
		         	 	         mylog2(minpow2(constrptr->card));
                    constrptr->face_ptr->curdim_face = 
                                               constrptr->face_ptr->mindim_face;
		    /*printf("mindim_face = %d\n",
				            constrptr->face_ptr->mindim_face);*/
	    }

        }
    }

}



int num_constr(net)
CONSTRAINT **net;

{

    CONSTRAINT *constrptr;
    int i;

    i = 0;

    for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
                                                  constrptr = constrptr->next) {
	i++;
    }

    return(i);

}



CONSTRAINT *update_net(net)
CONSTRAINT **net;

{

    CONSTRAINT *constrptr,*update_constr;
    int max_weight;

    max_weight = 0;

    for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
                                                  constrptr = constrptr->next) {
        if (constrptr->attribute == NOTUSED && constrptr->weight > max_weight) {
	    max_weight = constrptr->weight;
	    update_constr = constrptr;
	}
    }
    update_constr->attribute = USED;

    return(update_constr);

}



faces_to_codes(symblemes,net_num)
SYMBLEME *symblemes;
int net_num;

{

    CONSTRAINT_E *constrptr_e;
    int i;

    /* copy the codes of the exact algorithm in symblemes[].exact_code */
    i = net_num-1;
    for (constrptr_e = graph_levels[0]; constrptr_e != (CONSTRAINT_E *) 0;
				         constrptr_e = constrptr_e->next ) {
        if ( constrptr_e->face_ptr != (FACE *) 0 ) {
            strcpy(symblemes[i].exact_code,constrptr_e->face_ptr->cur_value);
	    i--;
        }
    }    
    
}



/*******************************************************************************
*   Verifies which constraints have been satisfied by the encoding algorithm   *
*   and which haven't  -   prints out the related information                  *
*******************************************************************************/

constr_satisfaction(net,symblemes,code_length,label)
CONSTRAINT **net;
SYMBLEME *symblemes;
int code_length;
char *label;

{

    CONSTRAINT *constrptr;
    char *face;
    char bit1;
    int i,j,card,satisfied;
    int sat_measure,unsat_measure;

    /* empty net : do nothing */
    if ((*net) == (CONSTRAINT *) 0) {
	return;
    }

    sat_measure = unsat_measure = 0;

    card = strlen((*net)->relation);

    if ((face = (char *) calloc((unsigned)code_length+1,sizeof(char))) == (char *) 0) {
	fprintf(stderr,"Insufficient memory for face in constr_satisfaction");
	exit(-1);
    }

    /* scans all constraints */
    for (constrptr = (*net); constrptr != (CONSTRAINT *) 0; 
					   constrptr = constrptr->next) {

        /* discards meaningless constraints */
        if (constrptr->card == 1) continue; 

        /*printf("The constraint is %s\n", constrptr->relation);*/

        /* scans all boolean coordinates */
        for (i = 0; i < code_length; i++) {

            /* stores in bit1 the "i-th" bit of the best_code of the "j-th" 
	       symbleme ( which belongs to the current constraint )         */
	    for ( j = 0; j < card; j++) {
		if ( constrptr->relation[j] == ONE ) {
		    if (strcmp(label,"exact") == 0)
	                                      bit1 = symblemes[j].exact_code[i];
		    if (strcmp(label,"best") == 0)
	                                       bit1 = symblemes[j].best_code[i];
		}
	    }

            /* if there are two symblemes in the current constraint different 
	       on the i-th boolean coordinate face[i] gets a * (don't care)  
	       otherwise face[i] gets the common coordinate value            */
	    for ( j = 0; j < card; j++) {
		if ( constrptr->relation[j] == ONE ) {
		    if (strcmp(label,"exact") == 0 ) {
		      if ( bit1 != symblemes[j].exact_code[i]) {
		          face[i] = '*'; 
			  break;
		      } else {
			    face[i] = bit1;
		      }
		    }
		    if (strcmp(label,"best") == 0 ) {
		      if ( bit1 != symblemes[j].best_code[i]) {
		          face[i] = '*'; 
			  break;
		      } else {
			    face[i] = bit1;
		      }
		    }
                }    
	    }

	}     

        /*printf("the spanned face is %s\n", face);*/

	/* if the code of any other symbleme ( i.e. a symbleme not 
	   contained in constraint ) is covered by face , the current
	   constraint has not been satisfied                             */
        for (j = 0; j < card; j++) {
            if ( constrptr->relation[j] == ZERO ) {
		if (strcmp(label,"exact") == 0) {
		  if (inclusion(symblemes[j].exact_code,face,code_length) == 1) {
                      /*printf("the symbleme %s (%d)  is covered by face %s\n",symblemes[j].name, j, face);*/
		      satisfied  = 0;
		      /*printf("UNSATISFIED\n");*/
		      break;
		  }
		}
		if (strcmp(label,"best") == 0) {
		  if (inclusion(symblemes[j].best_code,face,code_length) == 1) {
                      /*printf("the symbleme %s (%d)  is covered by face %s\n",symblemes[j].name, j, face);*/
		      satisfied  = 0;
		      /*printf("UNSATISFIED\n");*/
		      break;
		  }
		}
	    }
	}
	/* no other constraint covered by face */
	if (j == card) {
	    satisfied = 1;
	    /*printf("SATISFIED\n");*/
	}
	if (satisfied == 0) {
	    unsat_measure = unsat_measure + constrptr->weight;
	}
	if (satisfied == 1) {
	    sat_measure = sat_measure + constrptr->weight;
	}

    }

    if (VERBOSE) {
        printf("\nInput constraints satisfaction = %d   ", sat_measure);
        printf("\nInput constraints unsatisfaction = %d\n", unsat_measure);
    }

}



random_patch(symblemes,net_num)
SYMBLEME *symblemes;
int net_num;

{

    long time();
    int code_bits,i,new_code,increment,range,seed,trandom();

    if (VERBOSE) printf("\nI generated a random code\n");

    code_bits = mylog2(net_num);

    seed = (int) time(0);
    srand(seed);

    range = minpow2(net_num);
    new_code = trandom(range);
    increment = trandom(range)*2 + 1;
    for (i = 0; i < net_num; i++) {
        itob(symblemes[i].exact_code,new_code,code_bits);
	new_code = (new_code + increment) % range;
    }

}
