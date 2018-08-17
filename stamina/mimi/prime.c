
/* SCCSID %W% */
#include "user.h"
#include "sis/util/util.h"
#include "struct.h"
#include "global.h"
#include "merge.h"
#include "stack.h"
#include "max.h"

PRIMES     prime;
PRIMES     gen_list;
PRIME      p_new;
int        p_num;        /* Number of Prime */
static int num_literal = 1;

int select_small();

prime_compatible() {
    register i;
    int last_prime;
    int first;

    sort_max();
    if (!(prime = ALLOC(PRIME * , MAX_PRIME)))
        panic("ALLOC in prime");
    gen_list = ALLOC(PRIME * , MAX_PRIME);
    p_new.state = ALLOC(
    int, num_st);
    p_new.num = 0;

    first = 0;
    p_num = 0;
#ifdef DEBUG
    (void) printf("Base %d\n",user.stat.base_max);
#endif
    num_literal = max[0]->num;
    do {
        last_prime = select_big();        /* Select next maximal candidate */

        if ((user.opt.solution == 3) && (first++ < 2))
            prime_from_prime(last_prime);
        else if (user.opt.solution != 3)
            prime_from_prime(last_prime);
    } while (--num_literal > 1);

    if (user.cmd.trans) {
        for (i = 0; i < p_num; i++)
            Find_Implied(prime, i);
    }
    if (user.opt.verbose > 1)
        say_prime();
    FREE(gen_list);
}

prime_from_prime(limit)
                        {
                                register i;

                                for (i=0; i<limit; i++) {
                            if (!(gen_list[i]->class.many))
                                continue;
                            /* Generate prime from Maximal compatibles */
                            enumerate_prime(gen_list[i], &p_new, num_literal - 1, 0, select_small);
                        }
                        }

enumerate_prime(p_from, p_to, iter, start, service
)

int (*service)();

PRIME *p_from;        /* Maximal compatible ID */
PRIME *p_to;
{
register i;

for (
i = start;
i<SMASK(p_from->num)-iter+1; i++) {
/* select current level candidate */
p_to->state[p_to->num]=p_from->state[i];
p_to->num++;
if (iter<2)
(*service)(p_to);
else
enumerate_prime(p_from, p_to, iter
-1, i+1, service);
p_to->num--;
}
}


select_small(p_where)
PRIME *p_where;
{
register k, i,
j;
/*
	if (user.opt.solution == 3) {
		for (j=0; j<user.stat.n_max; j++) {
			if (p_where[p_prime] == max[j])
				break;
		}	
		if ((j >= user.stat.base_max) && (j<user.stat.n_max))
			return 0;
	}
*/
/* If class set contains that of prime it is not prime */

prime[p_num]=
p_where;

if (user.cmd.trans)
transitive(prime, p_num
);
else
Find_Implied(prime, p_num
);

/* I have implied class set in local Imply structure */

if (!

p_exclude()

) {
if (!(prime[p_num]=
ALLOC(PRIME,
1)))
panic("small");
if (!(prime[p_num]->
state = ALLOC(
int,SMASK(p_where->num))))
panic("small 2");
memcpy(prime[p_num]
->state, p_where->state, sizeof(int)*p_where->num);
prime[p_num]->
num = p_where->num;
prime[p_num]->
class = p_where->class;
p_num++;
}
}

p_exclude() {
    register i;

    /* Prime excluding */

    for (i = 0; i < p_num; i++) {
        if (gp_exclude(i, p_num))
            return 1;    /* Candidate is excluded by prime i */
    }
    return 0;            /* Candidate is new prime */
}

gp_exclude(g_prime, candidate
)
/* g_prime: Given prime */
/* candidate: Candidate prime */
{
register i, j, k, l,
    m;
int not_match;
int contained;

/* Check prime contains candidate or not */

for (
k = 0;
k <SMASK(prime[candidate]->num); k++) {
not_match = 1;
for (
l = 0;
l<SMASK(prime[g_prime]->num); l++) {
if (prime[candidate]->state[k] == prime[g_prime]->state[l]) {
not_match = 0;
break;
}
}
if (not_match)
return 0;
}

/* Yes prime contains candidate. Then check class set of candidate
   contains that of given prime */

if (prime[g_prime]->class.many) {
if (!prime[candidate]->class.many)
return 0;
}
else {
return 1;
}

for (
i = 0;
i<prime[g_prime]->class.
many;
i++) {
for (
k = 0;
k<prime[candidate]->class.
many;
k++) {
contained = 0;
if (SMASK(prime[g_prime]->class.imply[i]->num) >
SMASK(prime[candidate]->class.imply[k]->num)) {
continue;
}
for (
l = 0;
l<SMASK(prime[g_prime]->class.imply[i]->num); l++) {
not_match = 1;
for (
m = 0;
m<SMASK(prime[candidate]->class.imply[k]->num); m++) {
if (prime[g_prime]->class.imply[i]->state[l] ==
prime[candidate]->class.imply[k]->state[m]) {
not_match = 0;
break;
}
}
if (not_match)
break;
}
if (!not_match) {
contained = 1;
break;
}
}
if (!contained)    /* Don't exclude */
return 0;
}
return 1;            /* Exclude it */
}

sort_max() {
    register i, j;
    PRIME * save_max;

    for (i = 0; i < user.stat.n_max; i++) {
        for (j = i + 1; j < user.stat.n_max; j++) {
            if (max[i]->num < max[j]->num) {
                save_max = max[i];
                max[i] = max[j];
                max[j] = save_max;
            }
        }
    }
    if (user.cmd.trans)
        for (i = 0; i < user.stat.n_max; i++)
            transitive(max, i);
}

select_big() {
    register i;
    int        index;
    static int max_id    = 0;
    static int exhausted = 0;
    static int selected  = 0;

    if (exhausted)
        return selected;

    for (i = max_id; i < user.stat.n_max; i++) {
        if (max[i]->num == num_literal) {
            prime[p_num++] = max[i];            /* This is prime */
            gen_list[selected++] = max[i];
        } else {
            if (max[i]->num < num_literal) {
                max_id = i;
                break;
            }
        }
    }
    if (i == user.stat.n_max)
        exhausted = 1;

    if (!(prime[p_num] = ALLOC(PRIME, 1)))
        panic("ALLOC in prime");

    if (!(prime[p_num]->state = ALLOC(int, num_literal)))
    panic("ALLOC in prime");

    return selected;
}

write_prime(pnum)
                   {
                           register i;

                   (void) fprintf(stderr, "*** Prime %d  (", pnum);
                           for (i=0; i<SMASK(prime[pnum]->num); i++) {
                       (void) fprintf(stderr, "%s", states[prime[pnum]->state[i]]->state_name);
                       if (i != SMASK(prime[pnum]->num - 1))
                           (void) fprintf(stderr, ",");
                   }
                   (void) fprintf(stderr, ")\n");
                   }

write_imply(pnum)
                   {
                           register i, j;

                   (void) fprintf(stderr, "*** Class set  {");

                           for (i=0; i<prime[pnum]->class.many; i++) {
                       (void) fprintf(stderr, "(");
                       for (j = 0; j < prime[pnum]->class.imply[i]->num; j++) {
                           (void) fprintf(stderr, "%s", states[prime[pnum]->class.imply[i]->state[j]]->state_name);
                           if (j != prime[pnum]->class.imply[i]->num - 1)
                               (void) fprintf(stderr, ",");
                       }
                       (void) fprintf(stderr, ")");
                   }
                   (void) fprintf(stderr, "}\n");
                   }
