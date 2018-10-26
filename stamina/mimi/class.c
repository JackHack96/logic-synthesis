
/* SCCSID %W% */
#include "util.h"
#include "struct.h"
#include "global.h"
#include "merge.h"
#include "stack.h"
#include "max.h"
#include "user.h"

#define next_state n_state->state_index
#define I sizeof(int)
#define SMASK(x) (x & 0x7fff)

IMPLIED **local_imply;
int *local_state;

get_implied()
{
	register i,j,k;

	if (!(local_state = ALLOC(int,num_st)))
		panic("ALLOC in x");

	if (!(local_imply = ALLOC(IMPLIED *,200)))
		panic("ALLOC in x");

	for (i=0; i<num_st; i++)
		states[i]->code = NIL(char);

	for (i=0; i<user.stat.n_max; i++) {		/* Maximal Compatible */
		Find_Implied(max,i);
	}
}


one_implied()
{
	register i,j,k;

	i=user.stat.n_max-1;
	
	Find_Implied(max,i);
}

extern NLIST *_head;
static int sequ, deleted;

Find_Implied(xmax,max_index) 
PRIMES xmax;
{
	register i,l;
	int num_implied;
	S_EDGE *top;
	NLIST *link;

	sequ = deleted = 0;
	for (i=0; i<SMASK(xmax[max_index]->num); i++)
		states[xmax[max_index]->state[i]]->code = (char *) 1;

	for (link=_head; link; link=link->h_next) {
		
		top=(S_EDGE *)link->ptr;
		num_implied = 0;
		while (top) {
			
			if (top->p_state->code) {
					if (top->n_star)
						goto x_con;
					for (l=0; l<num_implied; l++) {
						if (top->next_state == local_state[l])
								goto x_con;
					}
					local_state[num_implied++] = top->next_state;
#ifdef DEBUG
					(void) printf("%s",states[top->next_state]->state_name);
#endif
			}
x_con:
			top=top->v_next;
		}
		if ((num_implied > 1) && !excluded(xmax,max_index,num_implied,sequ)) {
			if (!(local_imply[sequ] =ALLOC(IMPLIED,1)))
				panic("ALLOC in get_implied");
			if (!(local_imply[sequ]->state=ALLOC(int,num_implied)))
				panic("ALLOC in prime");
			MEMCPY(local_imply[sequ]->state,local_state,num_implied*I);
			local_imply[sequ]->num=num_implied;
			sequ++;
		}
	}
	xmax[max_index]->class.many = sequ-deleted;
	for (i=0; i<SMASK(xmax[max_index]->num); i++)
		states[xmax[max_index]->state[i]]->code = NIL(char);

/*
	if (sequ-deleted) {
*/
		{
			register k,m;
			register all;
			IMPLIED *temp_imply;

			all=sequ;
/*
			if (sequ-deleted) {
*/
				for (k=0; k<all; k++) {
					while (!local_imply[k]->num) {
						all--;
						temp_imply = local_imply[k];
						local_imply[k]=local_imply[all];
						local_imply[all] = temp_imply;
						if (k>all-1)
							goto xc24;
					}
				}
/*
			}
*/
		}
xc24:
		sequ -= deleted;
		if (sequ) {
		if (!(xmax[max_index]->class.imply=ALLOC(IMPLIED *,sequ)))
			panic("ALLOC class implied");
		}
	MEMCPY(xmax[max_index]->class.imply,local_imply,sequ*sizeof(IMPLIED *));
/*
	}
*/
}

transitive(xmax,max_index) 
PRIMES xmax;
{
	register i,l;
	int n_imply;

	sequ = deleted = 0;
	n_imply = column_implied(xmax[max_index]->state,xmax[max_index]->num,xmax,max_index,1);

	for (i=0; i<n_imply; i++) {
		n_imply = column_implied(local_imply[i]->state,local_imply[i]->num,xmax,max_index,0);
	}
	
	xmax[max_index]->class.many = n_imply;

	if (!(xmax[max_index]->class.imply=ALLOC(IMPLIED *,n_imply)))
		panic("ALLOC class implied");
	MEMCPY(xmax[max_index]->class.imply,local_imply,n_imply*sizeof(IMPLIED *));

/*
	if (sequ-deleted) {
		{
			register k,m;
			register all;
			IMPLIED *temp_imply;

			all=sequ;
			if (sequ-deleted) {
				for (k=0; k<all; k++) {
					while (!local_imply[k]->num) {
						all--;
						temp_imply = local_imply[k];
						local_imply[k]=local_imply[all];
						local_imply[all] = temp_imply;
						if (k>all-1)
							goto xc24;
					}
				}
			}
		}
xc24:
		sequ -= deleted;
		if (!(xmax[max_index]->class.imply=ALLOC(IMPLIED *,sequ)))
			panic("ALLOC class implied");
	MEMCPY(xmax[max_index]->class.imply,local_imply,sequ*sizeof(IMPLIED *));
	}
*/
}

column_implied(state_set,number,xmax,max_index,reset)
PRIMES xmax;
int *state_set;
{
	register i,l;
	S_EDGE *top;
	NLIST *link;
	int num_implied,met;
	static i_num=0;
	int added;

	if (reset)
		i_num = 0;
	for (i=0; i<number; i++)
		states[state_set[i]]->code = (char *) 1;

	deleted = added = 0;
	for (link=_head; link; link=link->h_next) {
		
		top=(S_EDGE *)link->ptr;
		num_implied = 0;
		met = 0;
		while (top) {
			
			if (top->p_state->code) {
		
				met++;
				if (top->n_star)
					goto x_con;
				for (l=0; l<num_implied; l++) {
					if (top->next_state == local_state[l])
							goto x_con;
				}
				local_state[num_implied++] = top->next_state;
#ifdef DEBUG
				(void) printf("%s",states[top->next_state]->state_name);
#endif
				if (met == number)
					break;
			}
x_con:
			top=top->v_next;
		}
		if ((num_implied > 1) && !excluded(xmax,max_index,num_implied,i_num)) {
			if (!(local_imply[i_num] =ALLOC(IMPLIED,1)))
				panic("ALLOC in get_implied");
			if (!(local_imply[i_num]->state=ALLOC(int,num_implied)))
				panic("ALLOC in prime");
			MEMCPY(local_imply[i_num]->state,local_state,num_implied*I);
			local_imply[i_num]->num=num_implied;
			i_num++;
			added++;
		}
	}
	
	for (i=0; i<number; i++)
		states[state_set[i]]->code = (char *) 0;

/*
	if (added-deleted) {
*/
		{
			register k,m;
			register all;
			IMPLIED *temp_imply;

			all=i_num;
			for (k=0; k<all; k++) {
				while (!local_imply[k]->num) {
					all--;
					temp_imply = local_imply[k];
					local_imply[k]=local_imply[all];
					local_imply[all] = temp_imply;
					if (k>all-1)
						goto xc24;
				}
			}
		}
xc24:
		i;
/*
		sequ -= deleted;
		if (!(xmax[max_index]->class.imply=ALLOC(IMPLIED *,sequ)))
			panic("ALLOC class implied");
	MEMCPY(xmax[max_index]->class.imply,local_imply,sequ*sizeof(IMPLIED *));
	}
*/
	
	i_num -=  deleted;
	return i_num;
}

set_nstate(link,max_index,r_out)
NLIST *link;
char *r_out;
{
	S_EDGE *top;
	int num_implied;
	register i,j,l;
	int exist;
	extern PRIMES prime;

	if (r_out) {
	for (i=0; i<num_po; i++)
		r_out[i] = '-';
	r_out[i] = 0;
	}
	for (i=0; i<SMASK(prime[max_index]->num); i++)
		states[prime[max_index]->state[i]]->code = (char *) 1;
	top=(S_EDGE *)link->ptr;
	num_implied = 0;
	exist=0;
	while (top) {
		if (top->p_state->code) {
			exist=1;
			if (r_out) {
				for (j=0; j<num_po; j++)
				    r_out[j]=(top->output)[j] == '-'?r_out[j]:(top->output)[j];
			}
			if (top->n_star)
				goto x_con;
			for (l=0; l<num_implied; l++) {
				if (top->next_state == local_state[l])
						goto x_con;
			}
			local_state[num_implied++] = top->next_state;
		}
x_con:
		top=top->v_next;
	}
	for (i=0; i<SMASK(prime[max_index]->num); i++)
		states[prime[max_index]->state[i]]->code = NIL(char);

	if (!exist)
		return -1;
	else
		return num_implied;
}

excluded(xmax,maxim,candidate,index)
PRIMES xmax;
{
	register k,l,m;
	int not_match;
	int bound,upbound;

	for (k=0; k <candidate; k++) {
		not_match = 1;
		for (l=0; l<xmax[maxim]->num; l++) {
			if (local_state[k] == xmax[maxim]->state[l]) {
				not_match = 0;
				break;
			}
		}
		if (not_match)
			break;
	}
	if (!not_match)	/* Candidate is included in Maximal Compatible */
		return 1;

	for (k=0; k<index; k++) {
		if (local_imply[k]->num > candidate) {
			bound=candidate;
			upbound=local_imply[k]->num;
		}
		else {
			bound=local_imply[k]->num;
			upbound = candidate;
		}
		for (l=0; l<bound; l++) {
			not_match = 1;
			for (m=0; m<upbound; m++) {
				if (local_imply[k]->num > candidate) {
					if (local_state[l] == local_imply[k]->state[m]) {
						not_match = 0;
						break;
					}
				}
				else {
						if (local_imply[k]->state[l]==local_state[m] ) {
							not_match = 0;
							break;
						}
				}
			}
			if (not_match)
				break;
		}
		if (!not_match) {
			if (bound == candidate)
				return 1;
			else {
				deleted++;
				local_imply[k]->num=0;
			}
		}
	}
	return 0;
}
