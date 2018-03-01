/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/stamina/mimi/iso.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:15 $
 *
 */
#include "user.h"
#include "util.h"
#include "struct.h"
#include "global.h"
#include "max.h"
#include "stack.h"

static int *new_state;
static append=0;
static struct sbase *base;
static itsize;
struct isomor *iso;

iso_find()
{
	register i,j;
	int id;
	int *temp;

	if (!(iso=ALLOC(struct isomor,num_st)))
		panic("iso");

	for (i=0; i<num_st; i++) {
		int yy;

		/*xx=*/yy=0;
		iso[i].uvoid=0;
		if (!(iso[i].list=ALLOC(int,num_st)))
			panic("iso_find3");

		for (j=0; j<num_st; j++) {
			if (i != j) {
				if (i >  j)
					id = xstack[(num_st-j-2)*(num_st-j-1)/2-i+num_st-1].status;
				else
					id = xstack[(num_st-i-2)*(num_st-i-1)/2-j+num_st-1].status;
				if (id & COMPATIBLE) {
					iso[i].list[yy++]=j;
				}
/*
				if (id & DEFINIT_COMPATIBLE)
					xx++;
*/
			}
		}
		iso[i].num=yy;
/*
		if (xx == (num_st -1))
			printf("###All compatible states %s\n",states[i]->state_name);
*/
	}

	if (!(temp=ALLOC(int,num_st)))
		panic("iso_find0");

	for (i=0; i<num_st; i++) {
		int numiso;

		if (!iso[i].uvoid && iso[i].num) {
			numiso=0;
#ifdef DEBUG
			(void) printf("%s : ",states[i]->state_name);
#endif
			for (j=i+1; j<num_st; j++) {
				int k;

				if (!iso[j].uvoid) {
					if (iso[i].num == iso[j].num)  {
						iso[j].uvoid=1;
						for (k=0; k<iso[i].num; k++)
							if (iso[i].list[k] != iso[j].list[k]) {
								iso[j].uvoid=0;
								break;
							}
						if (iso[j].uvoid) {
							temp[numiso++]=j;
							iso[j].num = i;
#ifdef DEBUG
							(void) printf("%s ",states[j]->state_name);
#endif
						}
					}
				}
			}
#ifdef DEBUG
			(void) printf("\n");
#endif
			if (numiso) {
				temp[numiso++]=i;
				iso[i].num=numiso;
				MEMCPY(iso[i].list,temp,numiso*sizeof(int));
			}
			else
				iso[i].num=0;
		}
	}

	user.stat.n_iso=0;
	for (i=0; i<num_st; i++)
		if (!iso[i].uvoid && iso[i].num) {
				user.stat.n_iso += iso[i].num;
		}
	if (FREE(temp))
		panic("lint");
}

iso_generate()
{
	int i,j;
	int index, point;

	user.stat.base_max = user.stat.n_max;
	base=ALLOC(struct sbase,num_st-1);
	if (!(new_state = ALLOC(int,num_st)) || !base)
		panic("alloc321");
	for (i=0; i<user.stat.base_max; i++) {
		point = 0;
		for (j=0; j<max[i]->num; j++) {
			index=max[i]->state[j];
			if (!iso[index].uvoid && iso[index].num) {
				base[point].state=index;
				base[point++].where=j;	
#ifdef DEBUG
				(void) printf("%s %d\n",states[index]->state_name,j);
#endif
			}
		}
		if (point) {
			itsize = /* sizeof(int)**/ max[i]->num /*+10*/;
			MEMCPY(new_state,max[i]->state,sizeof(int)*max[i]->num);
			make_max(--point);
			user.stat.n_max--;
		}
	}
	if (FREE(base))
		panic("lint");
}


iso_close(num_of_iso)
{
	register i,j;
	int max_id;
	int maxi,mini;
	int weight,max_weight;
	int index, point;
	int big,uncovered;

	if (!(base=ALLOC(struct sbase,num_st-1)))
		panic("iso_close");
	user.stat.base_max = user.stat.n_max;
	
	big=uncovered=0;

	for (i=0; i<num_st; i++)
		if (!iso[i].uvoid && iso[i].num) {
			uncovered += iso[i].num - 1;
		}
			
	append=0;
	while (uncovered > 0) {
		max_id=0;
		max_weight = 0;
		for (i=0, maxi=0; i<user.stat.base_max; i++) {
			int small_num;

			point = 0;
			weight = 0;
			for (j=0; j<max[i]->num; j++) {
				index=max[i]->state[j];
				if (!iso[index].uvoid && iso[index].num) {
					point++;
					weight += iso[index].num;
				}
			}
			if (!point)
				continue;
			if (point > maxi) {
				max_weight = weight;
				maxi=point;
				max_id=i;
				small_num = max[i]->num;
			}
			else
				if (point == maxi) {
/*
					if (max[max_id]->class.many > max[i]->class.many) {
						max_weight = weight;
						maxi=point;
						max_id=i;
						small_num = max[i]->num;
					}
					else
						if (max[max_id]->class.many == max[i]->class.many)
							if (max[i]->num < small_num) {
								max_weight = weight;
								maxi=point;
								max_id = i;
								small_num = max[i]->num;
							}
*/
					if (weight > max_weight) {
						max_weight = weight;
						maxi=point;
						max_id = i;
						small_num = max[i]->num;
					}
					else {
						if (weight == max_weight) {
					if (max[max_id]->class.many > max[i]->class.many) {
						max_weight = weight;
						maxi=point;
						max_id=i;
						small_num = max[i]->num;
					}
					else
						if (max[max_id]->class.many == max[i]->class.many)
							if (max[i]->class.many) {
							if (max[i]->num < small_num) {
								max_weight = weight;
								maxi=point;
								max_id = i;
								small_num = max[i]->num;
							}
							}
							else {
							if (max[i]->num > small_num) {
								max_weight = weight;
								maxi=point;
								max_id = i;
								small_num = max[i]->num;
							}
							}
						}
					}
				}
		}	

		/* How many states are covered by that maximal compatibles ? */

		mini=num_st;
		big=maxi;
		maxi=0;
		point=0;

		for (j=0; j<max[max_id]->num; j++) {
			index=max[max_id]->state[j];
			if (!iso[index].uvoid && iso[index].num) {
				base[point].state=index;
				base[point++].where=j;	
				if (iso[index].num < mini)
					mini=iso[index].num;
				if (iso[index].num > maxi)
					maxi=iso[index].num;
			}
		}

		for (i=0; i<maxi-1; i++) {
			MEMCPY(max[user.stat.n_max]->state,max[max_id]->state,
				sizeof(int)*max[max_id]->num);
			max[user.stat.n_max]->num = max[max_id]->num;

			for (j=0; j<big; j++) {		/* How many in a maximal compatible */
				int iso_id;

				if ((iso[base[j].state].num - 1) < i) {
					iso_id=i % (iso[base[j].state].num); /* 0; */
/*
					iso[base[j].state].num=0;
*/
				}
				else {
					iso_id=i;
					if (i < (iso[base[j].state].num - 1))
						uncovered--;
				}
/*
				printf("%d ",iso_id);
*/
				max[user.stat.n_max]->state[base[j].where]=
					iso[base[j].state].list[iso_id];
			}
			increase_max_num();
			one_implied();
/*
			printf("\n");
*/
		}
		for (j=0; j<big; j++)
			iso[base[j].state].num=0;
	}
	if (FREE(base))
		panic("lint");
	max_close();
}

make_max(step)
{
	int i,newid;

	for (i=0; i<iso[base[step].state].num; i++) {
		new_state[base[step].where]=iso[base[step].state].list[i];
		if (step)
			make_max(step-1);
		else {
			MEMCPY(max[user.stat.n_max]->state,new_state,sizeof(int)*itsize);
			max[user.stat.n_max]->num = itsize;
			increase_max_num();
		}
	}
}
