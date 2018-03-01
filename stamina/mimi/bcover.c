/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/mimi/bcover.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:15 $
 *
 */

#include "user.h"
#include "util.h"
#include "struct.h"
#include "global.h"
/*
#include "merge.h"
*/
#include "stack.h"
#include "max.h"
#include "sparse.h"
#include "sparse_int.h"
#include "mincov.h"

#define WEIGHT_DEFAULT	2000

unsigned char *all_states;
int *b_cover;		/* Binate covering solution */
int *c_cover;		/* Binate covering solution */

sm_setup()
{
	register i,j,k;
	int row_index;
	sm_matrix *A;
	sm_element *p;
	sm_row *cover;
	int *weight;
	FILE *b_fp;

	row_index=num_st; 
	if (b_file[0])
		if ((b_fp=fopen(b_file,"w")) == NIL(FILE))
			panic("Cannot open Binate cover file ");
	A=sm_alloc();
	weight = (int *) 0;
	for (i=0; i<p_num; i++) {
		for (j=0; j<SMASK(prime[i]->num); j++) {
			k=prime[i]->state[j];
			p=sm_insert(A,k,i);
			if (b_file[0]) {
				(void) fprintf(b_fp,"%d %d +\n",k,i);
			}
			sm_put(p,0);
		}
	}
	for (i=0; i<p_num; i++) {

		for (j=0; j<prime[i]->class.many; j++) {
			int not_closed;
			
			p=sm_insert(A,row_index,i);
			if (b_file[0]) {
				(void) fprintf(b_fp,"%d %d -\n",row_index,i);
			}
			sm_put(p,-1);
			not_closed=1;
			for (k=0; k<p_num; k++) {
				if (contained(prime[i]->class.imply[j],prime[k])) {
					not_closed=0;
					if (b_file[0]) {
						(void) fprintf(b_fp,"%d %d +\n",row_index,k);
					}
					p=sm_insert(A,row_index,k);
					sm_put(p,0);
				}
			}
			if (not_closed)
				panic("Not closed prime \n");
			row_index++;
		}
	}

	if (b_file[0]) {
		(void) fclose(b_fp);
		exit(0);
	}
	
	cover = sm_minimum_cover(A, weight, 2, 0);

	if (cover) {
		save_cover(cover);
	    sm_row_free(cover);
	} else {
	    (void) fprintf(stderr,"Unfeasible Problem\n");
	}
	sm_free(A);
	sm_cleanup();
}

max_cover(bina)
{
	register i,j,k;
	int row_index;
	sm_matrix *A;
	sm_element *p;
	sm_row *cover;
	int *weight;

	row_index=num_st; 

	if (bina) {
		for (i=0; i<user.stat.n_max; i++) {
			int maxim;
			PRIME *sav_max;

			maxim = 0;
			for (j=i; j<user.stat.n_max; j++) {
				if (max[j]->num > maxim) {
					k = j;
					maxim=max[j]->num;
				}
			}
			sav_max = max[i];
			max[i] = max[k];
			max[k] = sav_max;
		}
	}

	A=sm_alloc();
	for (i=0; i<user.stat.n_max; i++) {
		for (j=0; j<SMASK(max[i]->num); j++) {
			k=max[i]->state[j];
			p=sm_insert(A,k,i);
			sm_put(p,0);
		}
	}
	if (bina) {
		for (i=0; i<user.stat.n_max; i++) {
			for (j=0; j<max[i]->class.many; j++) {
				p=sm_insert(A,row_index,i);
				sm_put(p,-1);
				for (k=0; k<user.stat.n_max; k++) {
					if (contained(max[i]->class.imply[j],max[k])) {
						p=sm_insert(A,row_index,k);
						sm_put(p,0);
					}
				}
				row_index++;
			}
		}
	}
	weight=NIL(int);
	
	cover = sm_minimum_cover(A, weight, 2, 0);
	if (cover) {
		save_cover(cover);
	    sm_row_free(cover);
	} else {
	    (void) fprintf(stderr,"Unfeasible Problem\n");
	}
	sm_free(A);
	sm_cleanup();
	return user.stat.nstates;
}

extern struct isomor *iso;

max_close()
{
	register i,j,k;
	struct sbase *base;

	if (!(base=ALLOC(struct sbase,num_st-1)))
		panic("max_close");
	
	for (i=0; i<num_st; i++) {
		iso[i].list = (int *) 0;
	}

	for (i=0; i<user.stat.n_max; i++) {
		
		for (j=0; j<max[i]->class.many; j++) {
			int not_closed;

			not_closed=1;
			for (k=0; k<user.stat.n_max; k++) {
				if (contained(max[i]->class.imply[j],max[k])) {
					not_closed=0;
					break;
				}
			}
			if (not_closed) {
				not_closed=0;
				for (k=0; k<max[i]->class.imply[j]->num; k++) {
					int iso_id;

					iso_id = max[i]->class.imply[j]->state[k];
					if (iso[iso_id].uvoid) {
						not_closed++;
						iso[iso[iso_id].num].list = (int *) iso_id;
/*
						if (iso[iso[iso_id].num].uvoid)
							panic("UVOID");
*/
					}
					else {
						iso[iso_id].list = (int *) iso_id;
					}
				}
/*
				printf("MAX %d IMPLY %d\n",i,j);
				printf("void state %d for max %d\n",not_closed,i);
*/
				select_one_max(i,max[i]->class.imply[j]->num,base);
				one_implied();
			}
		}
	}
	if (FREE(base))
		panic("lint");
}


select_one_max(maxid,uncovered,base)
struct sbase *base;
{
	register i,j;
	int max_id;
	int newid;
	int *new_state;
	int maxi,itsize;
	int index, point;
	int big;

	big=0;

	max_id=0;
	maxi = num_st;
	for (i=0/*, maxi=0*/; i<user.stat.n_max; i++) {
		point = 0;
		for (j=0; j<max[i]->num; j++) {
			index=max[i]->state[j];
			if (!iso[index].uvoid && iso[index].list) {
				point++;
			}
		}
		if (point == uncovered) {
			if (max[i]->class.many < maxi) {
				maxi=max[i]->class.many;
				max_id=i;
			}
		}
	}	


	/* How many states are covered by that maximal compatibles ? */

	big=uncovered;
	maxi=0;
	point=0;

	for (j=0; j<max[max_id]->num; j++) {
		index=max[max_id]->state[j];
		if (!iso[index].uvoid && iso[index].list) {
			base[point].state=index;
			base[point++].where=j;	
		}
	}
		
	max[user.stat.n_max]->num = max[max_id]->num;
	MEMCPY(max[user.stat.n_max]->state,max[max_id]->state,
		sizeof(int)*max[max_id]->num);

	for (j=0; j<big; j++) {		/* How many in a maximal compatible */

		max[user.stat.n_max]->state[base[j].where]=
			(int ) iso[base[j].state].list;
		iso[base[j].state].list = (int *) 0;
	}
	increase_max_num();
}

save_cover(bsol)
sm_row *bsol;
{
	int new_states;
	sm_element *p;

	new_states = 0;
	p=bsol->first_col;
	while (p) {
		p=p->next_col;
		new_states++;
	}
	if (!(b_cover=ALLOC(int, new_states+1)))
		panic("save_cover");
	new_states = 0;
   	for(p = bsol->first_col; p != 0; p = p->next_col) {
		b_cover[new_states++] = p->col_num;
   	}
	b_cover[new_states] = -1;
	user.stat.nstates=new_states;
	user.stat.rstates=user.stat.nstates+user.stat.ostates;
}

contained(c_imply,prime_id)
PRIME *prime_id;
IMPLIED *c_imply;
{
	register k,l;
	int not_match;

	if (SMASK(prime_id->num) < c_imply->num)
		return 0;
	for (k=0; k <c_imply->num; k++) {
		not_match = 1;
		for (l=0; l<SMASK(prime_id->num); l++) {
		if (prime_id->state[l] == c_imply->state[k]) {
				not_match = 0;
				break;
			}
		}
		if (not_match)
			return 0;
	}
	return 1;
}

#define foreach_prime(v,where)	for (v=0; where[v] != -1; v++)
#define foreach_state(v,where)	for (v=0; v < SMASK(where->num); v++)

/*
shrink_prime()
{
	register i,j,k,m;
	int *s_imp;
	int shrinked,save_state,save_many;
	IMPLIED **save_imply;

	shrinked = 1;

	if (!(s_imp=ALLOC(int, num_st)))
		panic("shrink");

	get_essential_state();

	while (shrinked) {
		shrinked = 0;
		foreach_prime(i,b_cover) {
			get_implied_by_other_prime(b_cover[i],s_imp);

			foreach_state(j,prime[b_cover[i]]) {
				int not_closed;

				if ((all_states[prime[b_cover[i]]->state[j]] == 1)
					|| s_imp[prime[b_cover[i]]->state[j]]) {
					continue;
				}	
				save_state = prime[b_cover[i]]->state[j];
				for (k=j; k<prime[b_cover[i]]->num - 1; k++)
				  prime[b_cover[i]]->state[k]=prime[b_cover[i]]->state[k+1];
				prime[b_cover[i]]->num--;
				save_imply = prime[b_cover[i]]->class.imply;
				save_many = prime[b_cover[i]]->class.many;
				Find_Implied(prime,b_cover[i]);
				not_closed = 0;
				for (k=0; k<prime[b_cover[i]]->class.many; k++) {
					not_closed = 1;
					for (m=0; m<user.stat.nstates; m++) {
						if (i == m)
							continue;
						if (contained(prime[b_cover[i]]->class.imply[k],
								prime[b_cover[m]])) {
							not_closed = 0;
							break;
						}
					}
					if (not_closed) {
						(void) fprintf(stderr,"Not closed\n");
						prime[b_cover[i]]->class.imply = save_imply;
						prime[b_cover[i]]->class.many = save_many;
						prime[b_cover[i]]->state[prime[b_cover[i]]->num++] 
							= save_state;
						break;
					}
				}
				if (!not_closed) {
					all_states[save_state]--;
					shrinked++;
					user.stat.shrink++;
				}
			}
		}
	}
}

*/

shrink_prime()
{
	register i,j,k,m;
	int *s_imp;
	int shrinked, *save_state,save_many;
	IMPLIED **save_imply;

	shrinked = 1;

	if (!(s_imp=ALLOC(int, num_st)))
		panic("shrink");

	if (!(save_state=ALLOC(int, num_st)))
		panic("shrink2");

	get_essential_state();

	while (shrinked) {
		shrinked = 0;
		foreach_prime(i,b_cover) {
			int not_closed;
			int n_shrunk;

			get_implied_by_other_prime(b_cover[i],s_imp);

			n_shrunk = 0;
			foreach_state(j,prime[b_cover[i]]) {

				if ((all_states[prime[b_cover[i]]->state[j]] == 1)
					|| s_imp[prime[b_cover[i]]->state[j]]) {
					continue;
				}	
				save_state[n_shrunk++] = prime[b_cover[i]]->state[j];
				for (k=j; k<prime[b_cover[i]]->num - 1; k++)
				  prime[b_cover[i]]->state[k]=prime[b_cover[i]]->state[k+1];
				prime[b_cover[i]]->num--;
			}
			if (n_shrunk) {
			save_imply = prime[b_cover[i]]->class.imply;
			save_many = prime[b_cover[i]]->class.many;
			Find_Implied(prime,b_cover[i]);
			not_closed = 0;
			for (k=0; k<prime[b_cover[i]]->class.many; k++) {
				not_closed = 1;
				for (m=0; m<user.stat.nstates; m++) {
					if (i == m)
						continue;
					if (contained(prime[b_cover[i]]->class.imply[k],
							prime[b_cover[m]])) {
						not_closed = 0;
						break;
					}
				}
				if (not_closed) {
					prime[b_cover[i]]->class.imply = save_imply;
					prime[b_cover[i]]->class.many = save_many;
					for (j=0; j<n_shrunk; j++)
					prime[b_cover[i]]->state[prime[b_cover[i]]->num++] 
						= save_state[j];
					break;
				}
			}
			if (!not_closed) {
				for (j=0; j<n_shrunk; j++)
					all_states[save_state[j]]--;
				shrinked++;
				user.stat.shrink += n_shrunk;
			}
			}
		}
	}

	shrinked = 1;
	while (shrinked) {
		shrinked = 0;
		for (i=user.stat.nstates-1; i>-1; i--) {
/*
		foreach_prime(i,b_cover) {
*/
			int not_closed;
			int n_shrunk;

			get_implied_by_other_prime(b_cover[i],s_imp);

			n_shrunk = 0;
			foreach_state(j,prime[b_cover[i]]) {

				if ((all_states[prime[b_cover[i]]->state[j]] == 1)
					|| s_imp[prime[b_cover[i]]->state[j]]) {
					continue;
				}	
				save_state[n_shrunk++] = prime[b_cover[i]]->state[j];
				for (k=j; k<prime[b_cover[i]]->num - 1; k++)
				  prime[b_cover[i]]->state[k]=prime[b_cover[i]]->state[k+1];
				prime[b_cover[i]]->num--;
			}
			if (n_shrunk) {
			save_imply = prime[b_cover[i]]->class.imply;
			save_many = prime[b_cover[i]]->class.many;
			Find_Implied(prime,b_cover[i]);
			not_closed = 0;
			for (k=0; k<prime[b_cover[i]]->class.many; k++) {
				not_closed = 1;
				for (m=0; m<user.stat.nstates; m++) {
					if (i == m)
						continue;
					if (contained(prime[b_cover[i]]->class.imply[k],
							prime[b_cover[m]])) {
						not_closed = 0;
						break;
					}
				}
				if (not_closed) {
					prime[b_cover[i]]->class.imply = save_imply;
					prime[b_cover[i]]->class.many = save_many;
					for (j=0; j<n_shrunk; j++)
					prime[b_cover[i]]->state[prime[b_cover[i]]->num++] 
						= save_state[j];
					break;
				}
			}
			if (!not_closed) {
				for (j=0; j<n_shrunk; j++)
					all_states[save_state[j]]--;
				shrinked++;
				user.stat.shrink += n_shrunk;
			}
			}
		}
	}
}

get_essential_state()
{
	register i,j;

	if (!(all_states=ALLOC(unsigned char,num_st)))
		panic("Bcover");

	for (i=0; i<num_st; i++) {
		all_states[i] = 0;
	}
	foreach_prime(i,b_cover) {
		prime[b_cover[i]]->num = SMASK(prime[b_cover[i]]->num);
		foreach_state(j,prime[b_cover[i]]) {
			all_states[prime[b_cover[i]]->state[j]]++;
		}
	}
}

get_implied_by_other_prime(prime_no,x_imp)
int *x_imp;
{
	register i,j,k;
	NLIST *link;
	extern NLIST *_head;
	extern int *local_state;
	IMPLIED flow_imply;
	int s_num;

	for (i=0; i<num_st; i++)
		x_imp[i] = 0;

/*
	for (link=_head; link; link=link->h_next) {

		for (i=0; b_cover[i] != -1; i++) {
			
			if (s_num=set_nstate(link,b_cover[i],(char *) 0)) {
				int count,where;

				if (s_num == -1)
					continue;
				flow_imply.num=s_num;
				flow_imply.state=local_state;
				
					count = 0;
					where = -1;
				if (contained(&flow_imply,prime[prime_no])) {
					for (j=0; b_cover[j] != -1; j++) {
						if (b_cover[j] == prime_no)
							continue;
						if (contained(&flow_imply,prime[b_cover[j]])) {
							if (count++ > 1)
								break;
						}
					}
					if (count < 2) {
						for (k=0; k<flow_imply.num; k++)
							if (!x_imp[flow_imply.state[k]]) {
								x_imp[flow_imply.state[k]]++;
								(void) printf("Don't remove\n");
							}
					}
				}
				
			}
		}
	}
*/

	foreach_prime(i,b_cover) {
		if (b_cover[i] == prime_no)
			continue;
		for (j=0; j<prime[b_cover[i]]->class.many; j++) {
			int essence;
			if (contained(prime[b_cover[i]]->class.imply[j],prime[prime_no])) {
				essence = 1;
				foreach_prime(k,b_cover) {
					if (b_cover[k] == prime_no)
						continue;
					if (contained(prime[b_cover[i]]->class.imply[j],
						prime[b_cover[k]])) {
						essence = 0;
						break;
					}
				}
				if (essence) {
				for (k=0; k<prime[b_cover[i]]->class.imply[j]->num; k++)
					if (!x_imp[prime[b_cover[i]]->class.imply[j]->state[k]]) {
						x_imp[prime[b_cover[i]]->class.imply[j]->state[k]]++;
					}
				}
			}
		}
	}
}
