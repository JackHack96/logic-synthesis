
/* SCCSID %W% */
#include "user.h"
#include "sis/util/util.h"
#include "struct.h"
#include "global.h"
#include "merge.h"
#include "stack.h"
#include "max.h"
#include "espresso/inc/espresso.h"

P_EDGE       **r_states;
static char  *r_out;
extern int   *b_cover;
extern int   *c_cover;
extern NLIST *_head;

map() {
    register i;

    user.stat.shrink = 0;
    if (user.cmd.shrink)
        shrink_prime();

    user.stat.map_alternative = 0;
    user.stat.map_total       = 0;

    if (!(r_states = ALLOC(P_EDGE * , user.stat.nstates)))
        panic("map x");
    for (i = 0; i < user.stat.nstates; i++)
        r_states[i] = NIL(P_EDGE);
    form_new_flow_table();
    heuristics();
    if (/*(user.stat.product > 10000) ||*/ (user.cmd.merge))
        merge_product();
    write_new_machine();
}

form_new_flow_table() {
    register i, j;
    int     s_num;
    int     s_ptr;
    IMPLIED flow_imply;
    int     *new_ptr;
    NLIST   *link;
    int     *store;

    if (!(r_out = ALLOC(char, num_po + 1)))
    panic("r_out");
    user.stat.product = 0;
    if (!(store = ALLOC(int, user.stat.nstates)))
    panic("Form_flow");
    for (link = _head; link; link = link->h_next) {
        P_EDGE *start;

        start = NIL(P_EDGE);
        for (i = 0; b_cover[i] != -1; i++) {    /* For all closed cover */
            P_EDGE *n_edge;

            s_ptr = 0;
            if (s_num = set_nstate(link, b_cover[i], r_out)) {
                if (s_num == -1)
                    continue;
                flow_imply.num   = s_num;
                flow_imply.state = local_state;
                for (j = 0; b_cover[j] != -1; j++) {
                    if (contained(&flow_imply, prime[b_cover[j]])) {
                        store[s_ptr++] = j; /* b_cover[j];	*/
                    }
                }
                if (!s_ptr) {
                    for (j = 0; j < user.stat.ostates; j++)
                        if (c_cover[j] == flow_imply.state[0]) {
                            store[0] = user.stat.nstates + j;
                            s_ptr = 1;
                            break;
                        }
                    if (!s_ptr)
                        panic("Mapping");
                }
                if (!(new_ptr = ALLOC(int, s_ptr)))
                panic("new_ptr");
                MEMCPY(new_ptr, store, sizeof(int) * s_ptr);
            }
            n_edge    = ALLOC(P_EDGE, 1);
            user.stat.product++;
            n_edge->input  = link->name;
            n_edge->output = r_out;
            if (!(r_out = ALLOC(char, num_po + 1)))
            panic("r_out");
            n_edge->n_state = (STATE *) new_ptr;
            n_edge->p_state = (STATE *) &r_states[i];
            n_edge->p_star = s_ptr;
            if (s_num)
                n_edge->n_star = 0;
            else
                n_edge->n_star = 1;
            n_edge->h_next = n_edge->v_next = NIL(P_EDGE);

            if (start) {
                P_EDGE *v_next;

                v_next = start;
                while (v_next->v_next) {
                    v_next = v_next->v_next;
                }
                v_next->v_next = n_edge;
            } else
                start = n_edge;

            if (r_states[i]) {
                P_EDGE *h_next;

                h_next = r_states[i];
                while (h_next->h_next)
                    h_next = h_next->h_next;
                h_next->h_next = n_edge;
            } else {
                r_states[i] = n_edge;
            }
        }
        link->ptr = (int *) start;
    }

    flow_imply.num = 1;
    flow_imply.state = local_state;

    for (i = 0; i < user.stat.ostates; i++) {
        EDGE *eedge;

        eedge = states[c_cover[i]]->edge;
        while (eedge) {
            user.stat.product++;
            if (!eedge->n_star) {
                local_state[0] = eedge->n_state->state_index;
                s_ptr  = 0;
                for (j = 0; b_cover[j] != -1; j++) {
                    if (contained(&flow_imply, prime[b_cover[j]])) {
                        store[s_ptr++] = j;
                    }
                }
                if (s_ptr) {
                    if (s_ptr == 1)
                        eedge->n_state = (STATE *) store[0];
                    else {
                        if (!(new_ptr = ALLOC(int, s_ptr)))
                        panic("new_ptr");
                        MEMCPY(new_ptr, store, sizeof(int) * s_ptr);
                        eedge->n_state = (STATE *) new_ptr;
                    }
                } else {
                    for (j = 0; j < user.stat.ostates; j++) {
                        if (eedge->n_state->state_index == c_cover[j]) {
                            eedge->n_state = (STATE *) (user.stat.nstates + j);
                            s_ptr = 1;
                            break;
                        }
                    }
                    if (!s_ptr)
                        panic("map incomp");
                }
                eedge->p_star = s_ptr;
            }
            eedge = eedge->next;
        }
    }
}

int *vtbl_state;
int *htbl_state;

heuristics() {
    int   i, j, index;
    NLIST *link;
    float quality;
    int   max_weight, total_weight;
    int   h_index;

    vtbl_state = ALLOC(
    int, user.stat.rstates);
    htbl_state = ALLOC(
    int, user.stat.rstates);

    max_weight = total_weight = 0;
    for (link = _head; link; link = link->h_next) {
        P_EDGE *v_next;

        v_next = (P_EDGE *) link->ptr;

        while (v_next) {
            int maxi, pres;

            for (i = 0; i < user.stat.nstates; i++)
                if (v_next->p_state == (STATE *) &r_states[i]) {
                    pres = i;
                    break;
                }

            if (v_next->p_star > 1) {
                user.stat.map_alternative++;
                user.stat.map_total += v_next->p_star;
                if (user.opt.hmap) {

                    column_sum(v_next->output, link, 1, 0);
                    row_sum(v_next, pres);
                    maxi = -1;
/*
					index=((int *)(v_next->n_state))[0];
*/
                    for (i = 0; i < v_next->p_star; i++) {
                        int num, my_weight;

                        num = ((int *) (v_next->n_state))[i];

                        if (user.opt.hmap == 4) {
                            my_weight = htbl_state[num];
                            if (maxi < my_weight) {
                                maxi  = my_weight;
                                index = num;
                            }
/*
							else 
								if (maxi == my_weight) {
									if ((vtbl_state[num]*htbl_state[num]) >
										(vtbl_state[index] * htbl_state[index]))
										index = num;
								}
*/
                        } else {
                            my_weight = htbl_state[num] + vtbl_state[num];
                            total_weight += my_weight;
                            if (maxi < my_weight) {
                                maxi = my_weight;
                                if (max_weight < maxi)
                                    max_weight = maxi;
                                index = num;
                            }
                        }
                    }

                    if (user.opt.hmap == 4) {
                        h_index = index;
                        maxi    = -1;
                        for (i = 0; i < v_next->p_star; i++) {
                            int num, my_weight;

                            num = ((int *) (v_next->n_state))[i];
                            my_weight      = vtbl_state[num] * htbl_state[num];
                            total_weight += my_weight;
                            if (max_weight < my_weight)
                                max_weight = my_weight;
                            my_weight      = vtbl_state[num];
                            if (maxi < my_weight) {
                                maxi  = my_weight;
/*
								if (max_weight < maxi)
									max_weight = maxi;
*/
                                index = num;
                            }
/*
							else
								if (maxi == my_weight) {
									if ((vtbl_state[num]*htbl_state[num]) >
										(vtbl_state[index] * htbl_state[index]))
										index = num;
								}
*/

                        }
                        if (h_index != index) {
                            if ((vtbl_state[h_index] * htbl_state[h_index]) >
                                (vtbl_state[index] * htbl_state[index]))
                                index = h_index;
                        }
                    }
                    ((int *) (v_next->n_state))[0] = index;
                }
                v_next->p_star = 1;
            }
            v_next = v_next->v_next;
        }
    }

    for (i = 0; i < user.stat.ostates; i++) {
        EDGE *eedge;

        eedge = states[c_cover[i]]->edge;
        while (eedge) {
            int maxi;

            if (eedge->p_star > 1) {
                user.stat.map_alternative++;
                user.stat.map_total += eedge->p_star;
                if (user.opt.hmap) {
                    incomp_column_sum(eedge);
                    incomp_row_sum(c_cover[i], eedge);
                    maxi = -1;
                    for (j = 0; j < eedge->p_star; j++) {
                        int num, my_weight;

                        num = ((int *) (eedge->n_state))[j];
                        if (user.opt.hmap == 4) {
                            my_weight = htbl_state[num];
                            if (maxi < my_weight) {
                                maxi  = my_weight;
                                index = num;
                            }
                        } else {
                            my_weight = htbl_state[num] + vtbl_state[num];
                            total_weight += my_weight;
                            if (maxi < my_weight) {
                                maxi  = my_weight;
                                index = num;
                                if (max_weight < maxi)
                                    max_weight = maxi;
                            }
                        }

                    }
                    if (user.opt.hmap == 4) {

                        h_index = index;
                        maxi    = -1;
                        for (j = 0; j < eedge->p_star; j++) {
                            int num, my_weight;

                            num = ((int *) (eedge->n_state))[j];

                            my_weight      = vtbl_state[num] * htbl_state[num];
                            total_weight += my_weight;
                            if (max_weight < my_weight)
                                max_weight = my_weight;
                            my_weight = vtbl_state[num];

                            if (maxi < my_weight) {
                                maxi  = my_weight;
                                index = num;
/*
								if (max_weight < maxi)
									max_weight = maxi;
*/
                            }
                        }
                        if (h_index != index) {
                            if ((vtbl_state[h_index] * htbl_state[h_index]) >
                                (vtbl_state[index] * htbl_state[index]))
                                index = h_index;
                        }
                    }
                    eedge->n_state = (STATE *) index;
                } else
                    eedge->n_state = (STATE *) ((int *) (eedge->n_state))[0];
                eedge->p_star = 1;
            }
            eedge = eedge->next;
        }
    }

    if (total_weight)
        user.stat.quality =
                (float) (max_weight * user.stat.map_total) / (float) (total_weight);
}

merge_product() {
    FILE  *fp;
    char  *line;
    int   i, j, index;
    NLIST *link;

    for (i = 0; i < user.stat.nstates; i++) {

        P_EDGE *h_next;

        h_next = (P_EDGE *) r_states[i];

        while (h_next) {
            P_EDGE *h_last;
            P_EDGE *h_prev;

            h_last = (P_EDGE *) r_states[i];
            h_prev = (P_EDGE *) r_states[i];

            while (h_last) {
                int position, count;

                if (h_next == h_last) {
                    h_last = h_last->h_next;
                    h_prev = h_next;
                    continue;
                }

                if ((((int *) (h_next->n_state))[0] ==
                     ((int *) (h_last->n_state))[0]) &&
                    !conflict(h_next->output, h_last->output)) {
                    position = 0;
                    count    = 0;
                    for (j   = 0; j < num_pi; j++) {
                        switch (h_last->input[j] - h_next->input[j]) {
                            case 0: break;
                            case 1:
                            case -1:
                                if (count) {
                                    j = num_pi + 1;
                                    count = 0;
                                } else {
                                    count++;
                                    position = j;
                                }
                                break;
                            default: j = num_pi;
                                count = 0;
                                break;
                        }
                    }
                    if (count) {

                        user.stat.product--;
                        h_next->input = ALLOC(
                        char, num_pi + 1);
                        (void) strncpy(h_next->input, h_last->input, num_pi + 1);
                        (void) strncpy(h_next->output, r_out, num_po);

                        h_next->input[position] = '-';

                        if (h_last == r_states[i]) {
                            r_states[i] = h_last->h_next;
                        } else
                            h_prev->h_next = h_last->h_next;
/*
						FREE(h_last);
*/
                        h_last->p_star = 2;
                        h_last = (P_EDGE *) r_states[i];
                        h_prev = (P_EDGE *) r_states[i];
                    } else {
                        h_prev = h_last;
                        h_last = h_last->h_next;
                    }
                } else {
                    h_prev = h_last;
                    h_last = h_last->h_next;
                }

            }
            h_next = h_next->h_next;
        }
    }
}


write_new_machine() {
    FILE  *fp;
    int   i, j, index;
    NLIST *link;

/*
	if (!(line=ALLOC(char,num_pi+num_po+user.stat.rstates+20)))
		panic("write");
*/

    if (user.oname)
        fp = fopen(user.oname, "w");
    else
        fp = stdout;
    (void) fprintf(fp, "# FSM Reduction and mapping (C) Univ. of Colorado VLSI\n");
    (void) fprintf(fp, ".i %d\n", num_pi);
    (void) fprintf(fp, ".o %d\n", num_po);
    (void) fprintf(fp, ".p %d\n", user.stat.product);
    (void) fprintf(fp, ".s %d\n", user.stat.rstates);

    if (user.stat.reset) {
        index = -1;
        for (i = 0; i < user.stat.nstates; i++) {
            for (j = 0; j < SMASK(prime[b_cover[i]]->num); j++) {
                if (!prime[b_cover[i]]->state[j]) {
                    index = i;
                    break;
                }
            }
            if (index > -1)
                break;
        }
        if (index < 0) {
            for (i = 0; i < user.stat.ostates; i++)
                if (!c_cover[i]) {
                    index = user.stat.nstates + i;
                }
        }
        if (index < 0)
            panic("reset");
        else {
            (void) fprintf(fp, ".r S%d\n", index);
        }
    }

    for (link = _head; link; link = link->h_next) {
        P_EDGE *v_next;

        v_next = (P_EDGE *) link->ptr;

        while (v_next) {
            int maxi, pres;

            for (i = 0; i < user.stat.nstates; i++)
                if (v_next->p_state == (STATE *) &r_states[i]) {
                    pres = i;
                    break;
                }

            if (v_next->p_star > 1) {
                v_next = v_next->v_next;
                continue;
            }
            if (v_next->n_star)
                (void) fprintf(fp, "%s S%d * %s\n", v_next->input, pres,
                               v_next->output);
            else
                (void) fprintf(fp, "%s S%d S%d %s\n", v_next->input, pres,
                               ((int *) (v_next->n_state))[0], v_next->output);
            v_next = v_next->v_next;
        }
    }

    for (i = 0; i < user.stat.ostates; i++) {
        EDGE *eedge;

        eedge = states[c_cover[i]]->edge;
        while (eedge) {

            if (eedge->p_star > 1) {
                eedge = eedge->next;
                continue;
            }

            if (eedge->n_star)
                (void) fprintf(fp, "%s S%d * %s\n", eedge->input,
                               user.stat.nstates + i, eedge->output);
            else
                (void) fprintf(fp, "%s S%d S%d %s\n", eedge->input,
                               user.stat.nstates + i, (int) eedge->n_state, eedge->output);
            eedge = eedge->next;
        }
    }

    (void) fprintf(fp, ".e\n");

    (void) fclose(fp);
}

incomp_row_sum(wstate, product
)
EDGE *product;
{
int  i, w;
EDGE *h_next;

for (
i = 0;
i<user.stat.
rstates;
i++) {
htbl_state[i]=0;
}

h_next = states[wstate]->edge;
while (h_next) {
if (!h_next->n_star /* && (h_next != product) */ ) {
switch (user.opt.hmap) {
case 1:
case 2:
w = 1;
break;
case 3:
case 4:
w = literal_intersect(h_next->input, product->input, num_pi,
                      '1');
w +=
literal_intersect(h_next
->output, product->output, num_po,
'1');
break;
}
if (h_next->p_star > 1)
for (
i = 0;
i<h_next->
p_star;
i++)
htbl_state[((int *)(h_next->n_state))[i]] +=
w;
else
/*
				htbl_state[((int *)(h_next->n_state))[0]]++;
*/
htbl_state[(int)h_next->n_state] +=
w;
}
h_next = h_next->next;
}
}

incomp_column_sum(eedge)
EDGE *eedge;
{
int   i, j;
NLIST *link;

for (
i = 0;
i<user.stat.
rstates;
i++) {
vtbl_state[i]=0;
}
for (
i = 0;
i<user.stat.
ostates;
i++) {
EDGE *tedge;

if (c_cover[i] == eedge->p_state->state_index)
continue;
tedge = states[c_cover[i]]->edge;
while (tedge) {
int w;

if (!tedge->n_star)
if (
w = intersection(eedge->input, tedge->input, num_pi)
) {
switch (user.opt.hmap) {
case 1:
w = 1;
break;
case 2:
break;
case 3:
case 4:
w = literal_intersect(eedge->input, tedge->input,
                      num_pi, '-');
w +=
literal_intersect(eedge
->output, tedge->output,
num_po, '1');
break;
}
if (eedge->p_star>1) {
for (
j = 0;
j<eedge->
p_star;
j++)
vtbl_state[((int *)(eedge->n_state))[j]] +=
w;
}
else
/*
						vtbl_state[((int *)(eedge->n_state))[0]] += w;
*/
vtbl_state[(int )eedge->n_state] +=
w;
}
tedge = tedge->next;
}
}
for (
link = _head;
link;
link = link->h_next
) {
int w;

if (
w = intersection(link->name, eedge->input, num_pi)
) {
if (user.opt.hmap > 2)
w = literal_intersect(link->name, eedge->input, num_pi, '-');
column_sum(eedge
->output, link, 0, w);
}
}
}

cube_weight(cube, factor
)
char *cube;
{
register i,
weight;

weight = 1;
for (
i = 0;
i<num_pi;
i++) {
if (cube[i] == '-')
if (factor)
weight += 1;
else
weight *= 2;
}
return
weight;
}

column_sum(output, link, flag, column_weight
)
char  *output;
NLIST *link;
{
register i,
       j;
P_EDGE *v_next;

if (flag) {
for (
i = 0;
i<user.stat.
rstates;
i++) {
vtbl_state[i]=0;
}
}
switch (user.opt.hmap) {
case 1:
column_weight = 1;
break;
case 2:
if (flag)
column_weight = cube_weight(link->name, 0);
break;
case 3:
case 4:
if (flag)
column_weight = cube_weight(link->name, 1);
break;
}
v_next = (P_EDGE *) link->ptr;

while (v_next) {
if (!v_next->n_star)
for (
j = 0;
j<v_next->
p_star;
j++) {
vtbl_state[((int *)(v_next->n_state))[j]] +=
column_weight;
if (user.opt.hmap > 2)
vtbl_state[((int *)(v_next->n_state))[j]] +=
literal_intersect(output, v_next
->output, num_po, '1');
}
v_next = v_next->v_next;
}
if (flag)
for (
j = 0;
j<user.stat.
ostates;
j++) {
EDGE *eedge;

eedge = states[c_cover[j]]->edge;
while (eedge) {
int w;

if (!eedge->n_star) {
if (eedge->p_star)
if (
w = intersection(link->name, eedge->input, num_pi)
) {
switch (user.opt.hmap) {
case 1:
w = 1;
break;
case 3:
case 4:
w = literal_intersect(output, eedge->output,
                      num_po, '1');
w +=
literal_intersect(link
->name,
eedge->input, num_pi, '-');
break;
default:
break;
}
if (eedge->p_star > 1)
for (
i = 0;
i<eedge->
p_star;
i++) {
vtbl_state[((int *)(eedge->n_state))[i]]+=
w;
}
else
vtbl_state[(int)eedge->n_state] +=
w;
}
}
eedge = eedge->next;
}
}
}

row_sum(product, wstate
)
P_EDGE *product;
{
int    i;
P_EDGE *h_next;

for (
i = 0;
i<user.stat.
rstates;
i++) {
htbl_state[i]=0;
}

h_next = r_states[wstate]->h_next;
while (h_next) {
int w;

if (!h_next->n_star /*&& (h_next != product)*/) {
switch (user.opt.hmap) {
case 1:
case 2:
w = 1;
break;
case 3:
case 4:
w = literal_intersect(h_next->input, product->input, num_pi, '1');
w+=
literal_intersect(h_next
->output, product->output, num_po, '1');
break;
}
for (
i = 0;
i<h_next->
p_star;
i++)
htbl_state[((int *)(h_next->n_state))[i]] +=
w;
}
h_next = h_next->h_next;
}
}

#ifdef DEBUG
dump_new_table()
{
    register i,j;
    NLIST *link;

    for (i=0; i<user.stat.nstates; i++) {
        printf("*** state%d \n",i);
        for (link=_head; link; link=link->h_next) {
            P_EDGE *v_next;

            printf("Input %s : ",link->name);
            v_next=(P_EDGE *)link->ptr;
            while (v_next) {
                if (v_next->p_state==(STATE *)(&r_states[i])) {
                    for (j=0; j<v_next->p_star; j++) {
                        printf("state%d ",((int *)(v_next->n_state))[j]);
                    }
                    printf("\n");
                }
                v_next=v_next->v_next;
            }
        }
    }
}
#endif

conflict(str1, str2
)
char str1[], str2[];
{
register i;

for (
i = 0;
i<num_po;
i++) {
switch ((str1[i] - str2[i])) {
case 0:
r_out[i] = str1[i];
break;
case 1:
case -1:
return 1;
default:
r_out[i] = (str1[i] == '-') ? str2[i] : str1[i];
break;
}
}
return 0;
}

literal_intersect(str1, str2, width, xchar
)
char *str1, *str2;
char xchar;
{
register i;
int      count;

count = 0;
for (
i = 0;
i<width;
i++) {
switch ((str1[i] - str2[i])) {
case 0:
if (str1[i] == xchar)
count++;
break;
case 1:
case -1:
break;
default:
break;
/*
			return 0;
*/
}
}
return
count;
}
