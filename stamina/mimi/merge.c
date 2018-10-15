/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/mimi/merge.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:15 $
 *
 */
/* SCCSID %W%  */
#include "user.h"
#include "util.h"
#include "struct.h"
#include "global.h"
#include "merge.h"
#include "stack.h"

#define next_state_is_not_star !(edge1->n_star || edge2->n_star)
#define next_state n_state->state_index
#define implied_is_incompatible ptr1->status == INCOMPATIBLE
#define stack_status xstack[i].status
#define num_of_compatible j

merge()
{
	register i,j,id;
	int changed;
	int merge_entry;	

	merge_entry=(num_st-1)*num_st/2;

	for (i=0; i<num_st; i++) {
		(void) input_intersection(states[i],states[i]);
	}

	alloc_stack(merge_entry);
	id = 0;
	for (i=num_st-2; i>-1; i--)
		for (j=num_st-1; j>i; j--) {

			init_stack(id);
#ifdef DEBUG
	(void) printf("### %s %s ",states[i]->state_name,states[j]->state_name);
#endif
			switch (input_intersection(states[i],states[j])) {
			case INCOMPATIBLE:
				close_stack(id,INCOMPATIBLE);
				break;
			case DEFINIT_COMPATIBLE:
				close_stack(id,DEFINIT_COMPATIBLE);
				break;
			case CONDITIONAL_COMPATIBLE:
				pack_stack(id);
				break;
			default:
				panic("Coding Error");
				break;
			}
			id++;
		}
	
	do {
		STACK *ptr1;

		changed = 0;
		num_of_compatible = 0;
		for (i=0; i<merge_entry; i++) {
			if (stack_status == CONDITIONAL_COMPATIBLE) {
				open_stack(i);
				while (ptr1 = pop()) {
					if (implied_is_incompatible) {
						free_stack(i,INCOMPATIBLE);
						changed = 1;
						break;
					}
				}
			}
			if (stack_status & COMPATIBLE)
				num_of_compatible++;
		}
	} while (changed);
	
	user.stat.n_compatible = num_of_compatible;
/*
	printf("N EDGE %d\n",num_of_compatible);
*/

	id=0;
	changed=0;

	for (i=num_st-2; i>-1; i--) {		/* Column indexing */
		for (j=num_st-1; j>i; j--) { 	/* Row indexing */

		   if (xstack[id].status & COMPATIBLE) {
				states[i]->assigned=states[j]->assigned=1;
				if (user.opt.verbose > 10) 
			(void) fprintf(stderr,"Compatible pair %d : %s %s : %x\n",changed++,
				states[i]->state_name,states[j]->state_name,xstack[id].status);
		   }
		   id++;
		}
	}

	if (!user.stat.n_compatible)	{
		flushout();	
		if (user.opt.verbose)
			fprintf(stderr,"No compatible states\n");
		exit(0);
	}
#ifdef DEBUG
	if (user.opt.verbose > 3)
		(void) printf("Number of Compatible pairs %d\n",user.stat.n_compatible);
	for (i=0; i<num_st; i++) {
		(void) printf("\n%s : ",states[i]->state_name);
		for (j=0; j<num_st; j++) {
			if (i != j) {
				if (i>j)
					id = (num_st-j-2)*(num_st-j-1)/2-i+num_st-1;
				else
					id = (num_st-i-2)*(num_st-i-1)/2-j+num_st-1;
				if (xstack[id].status & COMPATIBLE)
					(void) printf("%s, ",states[j]->state_name);
			}
		}
	}
#endif
}

input_intersection(state1,state2)
STATE *state1, *state2;
{
	EDGE *edge1, *edge2;
	char *input1, *input2;
	int self;
	int pushed;

	pushed = 0;
	edge1 = state1->edge;
	if (state1 == state2)
		self = 1;
	else
		self = 0;
	while (edge1 != NIL(EDGE)) {
		if (self) 
			edge2 = edge1->next;
		else
			edge2 = state2->edge;
		while (edge2 != NIL(EDGE)) {

			if (intersection(edge1->input,edge2->input,num_pi)) {

				if (!intersection(edge1->output, edge2->output,num_po)) {
					if (self) {
						panic("Conflict Input intersection");
					}
					else {
					/* Incompatible pair of states */
						return INCOMPATIBLE;
					}
				}
				else {
					if (self) {
						if ((edge2->next_state != edge1->next_state) ||
							(strcmp(edge1->output,edge2->output))) {
							(void) fprintf(stderr,"### Input Intersection %s\n",
								state1->state_name);
							(void) fprintf(stderr,"state %s %s\n",
								states[edge1->next_state]->state_name,
								states[edge2->next_state]->state_name);
							panic("Invalid specification");
						}
/*
						else {
					printf("Duplicated input %s\n",edge1->p_state->state_name);
					printf("input %s %s\n",edge1->input,edge2->input);
					printf("next_state %s\n",edge2->n_state->state_name);
						}
*/
					}
					else {
						/* Conditionally compatible pair of states */
						if (next_state_is_not_star)	
							if (edge1->next_state != edge2->next_state) {
								pushed = 1;
								push(edge1->next_state,edge2->next_state);
							}
					}
				}
			}
			edge2 = edge2->next;
		}
		edge1 = edge1->next;
	}
	if (pushed)
		return CONDITIONAL_COMPATIBLE;
	else
		return DEFINIT_COMPATIBLE;
}

intersection(cube1,cube2,bit)
char cube1[], cube2[];
{
	register i;
	int inter_minterm;

	inter_minterm = 1;

	for (i=0; i< bit; i++) {
		switch ( cube1[i]-cube2[i]) {
		case 0:		/* Same input */
			if (cube1[i] == '-')
				inter_minterm *= 2;
			break;
		case -1:	/* Conflict input bit */
		case 1:
			return 0;
		default:        /* One has don't care condition */
			break;
		}
	}
	return inter_minterm;
}

xintersection(cube1,cube2,bit,xnode)
NODE *xnode;
char cube1[], cube2[];
{
	register i,add;

	add=0;
	for (i=0; i< bit; i++) {
		if (xnode->on_off[i]) {
			switch ( cube1[i]-cube2[i]) {
			case 0:		/* Same input */
				break;
			case -1:	/* Conflict input bit */
			case 1:
				return -1;
			default:        /* One has don't care condition */
				add++;
				break;
			}
		}
	}
	return add;
}

/*
int
power(base, exp)
int	base, exp;
{
	int	i, power;
	power = 1;
	i = 1;
	while(i <= exp)
	{
		power = power*base;
		i++;
	}
	
	return(power);
}
*/
