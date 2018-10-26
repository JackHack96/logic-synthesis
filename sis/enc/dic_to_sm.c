#include "sis.h"
#include "enc_int.h"

sm_matrix *
dic_to_sm(prime_list, seed_list)
dic_family_t *prime_list;
dic_family_t *seed_list;
{
    sm_matrix  *M;
    sm_col     *col;
    pset       p, lastp, s, lasts;
    int        r, c;
    bool       insert;

    M = sm_alloc();
    lastp = prime_list->dic + prime_list->dcount * prime_list->dic_size;
    lasts = seed_list->dic + seed_list->dcount * seed_list->dic_size;

    for(p = prime_list->dic, c = 0; p < lastp; 
				     p += prime_list->dic_size, c ++) {
	insert = FALSE;
	for(s = seed_list->dic, r =0; s < lasts; 
				       s += seed_list->dic_size, r ++) {
	    if(dicp_covers(s, p)) {
		(void) sm_insert(M, r, c);
		insert = TRUE;
	    }
	}
	if(insert) {
	    col = sm_get_col(M, c);
	   (void) sm_put(col, (char *) p);
	}
    }
    return M;
}

void
print_min_cover(M, cover, prime_list)
sm_matrix    *M;
sm_row       *cover;
dic_family_t *prime_list;
{
    sm_col     *col;
    sm_element *p;
    pset       d;
    int        n;

    n = prime_list->dset_elem;
    fprintf(sisout, "\nMinimum cover is \n");
    sm_foreach_row_element(cover, p) {
	col = sm_get_col(M, p->col_num);
	d = sm_get(pset, col);
	(void) fprintf(sisout, "%s ; ", pbv1(lhs_dic(d), n));
	(void) fprintf(sisout, "%s\n", pbv1(rhs_dic(d), n));
    }
}
