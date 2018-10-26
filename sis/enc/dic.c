#include "sis.h"
#include "enc_int.h"

dic_family_t *
dic_family_alloc(num, nelem)
int nelem;
{
    dic_family_t *A;
    int setsize;

    setsize = SET_SIZE(nelem);
    A = ALLOC(dic_family_t, 1);
    A->dcapacity = num;
    A->dcount = 0;
    A->dic_size = 2 * setsize;
    A->dset_elem = nelem;
    A->dset_size = setsize;
    A->dic = ALLOC(unsigned int, num * A->dic_size);
    return A;
}

void
dic_family_free(D)
dic_family_t *D;
{
    (void) dic_free(D->dic);
    (void) FREE(D);
}

dic_family_t *
dic_family_copy(D)
dic_family_t *D;
{
    dic_family_t *A;
    pset         p, s, last;

    A = ALLOC(dic_family_t, 1);
    A->dcapacity = D->dcapacity;
    A->dcount = D->dcount;
    A->dic_size = D->dic_size;
    A->dset_elem = D->dset_elem;
    A->dset_size = D->dset_size;
    A->dic = ALLOC(unsigned int, A->dcapacity * A->dic_size);
    last = D->dic + D->dic_size * D->dcount;
    for (p = D->dic, s = A->dic; p < last; p ++, s ++) {
	*s = *p;
    }
    return A;
}

void
dic_family_print(D)
dic_family_t *D;
{
    pset p, last;

    if (D->dcount == 0)
	return;
    last = D->dic + D->dcount * D->dic_size;
    for (p = D->dic; p < last; p += D->dic_size) {
	(void) dic_print(p, D->dset_elem);
    }
    return;
}

/* add a dichotomy to a dichotomy family */
void
dic_family_add(A, s)
dic_family_t *A;
pset         s;
{
    pset p;

    if (A->dcount >= A->dcapacity) {
	A->dcapacity = A->dcapacity + A->dcapacity/2 + 1;
	A->dic = REALLOC(unsigned int, A->dic, A->dcapacity*A->dic_size);
    }
    p = GETDIC(A, A->dcount++);
    INLINEset_copy(lhs_dic(p), lhs_dic(s));
    INLINEset_copy(rhs_dic(p), rhs_dic(s));
}

/* add a dichotomy to a dichotomy family if it doesn't exist */
void
dic_family_add_contain(A, s)
dic_family_t *A;
pset         s;
{
    pset p;
    int i;

    /* do not add if it already exists */
    for (i = 0; i < A->dcount; i ++) {
	if (dicp_equal(s, GETDIC(A, i))) {
	    return;
	}
    }

    if (A->dcount >= A->dcapacity) {
	A->dcapacity = A->dcapacity + A->dcapacity/2 + 1;
	A->dic = REALLOC(unsigned int, A->dic, A->dcapacity*A->dic_size);
    }
    p = GETDIC(A, A->dcount++);
    INLINEset_copy(lhs_dic(p), lhs_dic(s));
    INLINEset_copy(rhs_dic(p), rhs_dic(s));
}

/* add a dichotomy to a dichotomy family if it isn't implied */
void
dic_family_add_irred(A, s)
dic_family_t *A;
pset         s;
{
    pset p;
    int i;

    /* do not add if it already exists */
    for (i = 0; i < A->dcount; i ++) {
	p = GETDIC(A, i);
	if (dicp_implies(s, p)) {
	    return;
	}
	else if (dicp_implies(p, s)) {
	    set_copy(lhs_dic(p), lhs_dic(s));
	    set_copy(rhs_dic(p), rhs_dic(s));
	    return;
	}
    }

    if (A->dcount >= A->dcapacity) {
	A->dcapacity = A->dcapacity + A->dcapacity/2 + 1;
	A->dic = REALLOC(unsigned int, A->dic, A->dcapacity*A->dic_size);
    }
    p = GETDIC(A, A->dcount++);
    INLINEset_copy(lhs_dic(p), lhs_dic(s));
    INLINEset_copy(rhs_dic(p), rhs_dic(s));
}

pset
dic_new(nelem)
int nelem;
{
    pset p;
    int setsize, i;

    setsize = SET_SIZE(nelem);
    p = ALLOC(unsigned int, 2 * setsize);
    i = 2 * setsize - 1;
    do {
	p[i] = 0;
    }
    while(--i >= 0);
    *p = setsize - 1;
    *(p + setsize) = setsize - 1;
    return p;
}

void
dic_free(dic)
pset dic;
{
    FREE(dic);
}

void
dic_print(dic, nelem)
pset dic;
int nelem;
{
    fprintf(sisout, "%s;", pbv1(lhs_dic(dic), nelem));
    fprintf(sisout, "%s\n", pbv1(rhs_dic(dic), nelem));
}

bool
is_prime(dic, nelem)
pset dic;
int  nelem;
{
    pset d;
    bool flag;

    d = dic_new(nelem);
    (void) set_or(lhs_dic(d), lhs_dic(dic), rhs_dic(dic));
    flag = setp_full(lhs_dic(d), nelem);
    (void) dic_free(d);
    return flag;
}

/* is p1 contained in p2 */
bool 
dicp_implies(p1, p2) 
pset p1;
pset p2;
{
    return(setp_implies(lhs_dic(p1), lhs_dic(p2)) && 
	   setp_implies(rhs_dic(p1), rhs_dic(p2)));
}

/* is p1 contained in p2 in either direction */
bool 
dicp_covers(p1, p2) 
pset p1;
pset p2;
{
    return((setp_implies(lhs_dic(p1), lhs_dic(p2)) && 
	    setp_implies(rhs_dic(p1), rhs_dic(p2))) ||
           (setp_implies(lhs_dic(p1), rhs_dic(p2)) && 
	    setp_implies(rhs_dic(p1), lhs_dic(p2))));
}

bool 
dicp_equal(p1, p2) 
pset p1;
pset p2;
{
    return(setp_equal(lhs_dic(p1), lhs_dic(p2)) && 
	   setp_equal(rhs_dic(p1), rhs_dic(p2)));
}

dic_family_t *
dic_cross_product(dlist1, dlist2)
dic_family_t *dlist1;
dic_family_t *dlist2;
{
    dic_family_t *new;
    pset         lastp1, lastp2, p1, p2, merge;
    int          n;

    n = dlist1->dset_elem;
    new = dic_family_alloc(ALLOCSIZE, n);
    lastp1 = dlist1->dic + dlist1->dcount * dlist1->dic_size;
    lastp2 = dlist2->dic + dlist2->dcount * dlist2->dic_size;
    for (p1 = dlist1->dic; p1 < lastp1; p1 += dlist1->dic_size) {
	for (p2 = dlist2->dic; p2 < lastp2; p2 += dlist2->dic_size) {
	    /*
	    if ((merge = dic_merge(p1, p2)) != NULL) {
		if (is_feasible(merge)) {
		    (void) dic_family_add(new, merge);
		    (void) dic_free(merge);
		}
	    }
	    */
	}
    }
    return new;
}
