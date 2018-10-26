#include "sis.h"
#include "enc_int.h"

#define MAXLINE 256

/* remove those state variables that do not play any role */
int
filter_cons(file1, file2)
char *file1;
char *file2;
{
    FILE *fp, *fpout;
    char *line, *symbols;
    int  iter, i, n;
    bool *used;

    line = ALLOC(char, MAXLINE+1);
    symbols = ALLOC(char, MAXLINE+1);
    iter = 1;

    if ((fp = fopen(file1, "r")) == NULL) {
        (void) fprintf(siserr, "unable to open constraint file %s\n", file1);
        return FALSE;
    }

    while(fgets(line, MAXLINE, fp) != NIL(char)) {

	if (strlen(line) >= MAXLINE) {
	    (void) fprintf(siserr, 
		"WARNING: Line too long to handle currently : ignored\n");
	    continue;
	}

	/* pick up mv_constraint */
	if (sscanf(line, "input: %s\n", symbols) == 1) {
	    if (iter == 1) {
		n = strlen(symbols);
		used = ALLOC(bool, n);
		for (i = 0; i < n; i ++) {
		    used[i] = FALSE;
		}
	    }
	    else if (n != strlen(symbols)) {
		(void) fprintf(siserr, 
		    "Varying mv-constraint length : aborting\n");
		exit(-1);
	    }

	    for (i = 0; i < n; i ++) {
		if (symbols[i] == '1') {
		    used[i] = TRUE;
		}
		else if (symbols[i] == '-') {
		}
		else if (symbols[i] != '0') {
		    (void) fprintf(siserr, 
			"ERROR: incorrect mv-constraint %s : aborting\n", 
			symbols);
		    exit(-1);
		}
	    }
	}
	iter ++;
    }

    fclose(fp);
    if ((fp = fopen(file1, "r")) == NULL) {
        (void) fprintf(siserr, "unable to open constraint file %s\n", file1);
        return FALSE;
    }
    if ((fpout = fopen(file2, "w")) == NULL) {
        (void) fprintf(siserr, "unable to write constraints file %s\n", file2);
        return FALSE;
    }

    /* rewrite output file without unused state variables */
    while(fgets(line, MAXLINE, fp) != NIL(char)) {

	/* pick up mv_constraint */
	if (sscanf(line, "input: %s\n", symbols) == 1) {
	    (void) fprintf(fpout, "input: ");
	    for (i = 0; i < n; i ++) {
		if (used[i]) {
		    (void) fprintf(fpout, "%c", symbols[i]);
		}
	    }
	    (void) fprintf(fpout, "\n");
	}
    }

    fclose(fp);
    fclose(fpout);
    if (iter != 1) {
	FREE(used);
    }
    FREE(line);
    FREE(symbols);
    return TRUE;
}

/* form seed dichotomies from mv-constraints */
int
read_cons(file, dic_list)
char         *file;
dic_family_t **dic_list;
{
    FILE         *fp;
    dic_family_t *list;
    pset         lhs_set, dic;
    char         *line, *symbols, *rhs;
    int          iter, i, n;

    line = ALLOC(char, MAXLINE+1);
    symbols = ALLOC(char, MAXLINE+1);
    rhs = ALLOC(char, MAXLINE+1);
    *dic_list = NIL(dic_family_t);
    iter = 1;

    if ((fp = fopen(file, "r")) == NULL) {
        (void) fprintf(siserr, "unable to open constraint file %s\n", file);
        return FALSE;
    }

    while(fgets(line, MAXLINE, fp) != NIL(char)) {

	if (strlen(line) >= MAXLINE) {
	    (void) fprintf(siserr, 
		"WARNING: Line too long to handle currently : ignored\n");
	    continue;
	}

	/* pick up mv_constraint */
	if (sscanf(line, "input: %s\n", symbols) == 1) {
	    if (iter == 1) {
		n = strlen(symbols);
		list = dic_family_alloc(ALLOCSIZE, n);
	    }
	    else if (n != strlen(symbols)) {
		(void) fprintf(siserr, 
		    "Varying mv-constraint length : aborting\n");
		exit(-1);
	    }

	    lhs_set = set_new(n);
	    for (i = 0; i < n; i ++) {
		if (symbols[i] == '1') {
		    rhs[i] = 0;
		    (void) set_insert(lhs_set, i);
		}
		else if (symbols[i] == '-') {
		    rhs[i] = 0;
		}
		else if (symbols[i] == '0') {
		    rhs[i] = 1;
		}
		else {
		    (void) fprintf(siserr, 
			"ERROR: incorrect mv-constraint %s : aborting\n", 
			symbols);
		    exit(-1);
		}
	    }

	    /* now get seed dichotomies and form set data structure */
	    for (i = 0; i < n; i ++) {
		if (rhs[i] == 1) {
		    /* add new dichotomy */
		    dic = dic_new(n);
		    (void) set_copy(lhs_dic(dic), lhs_set);
		    (void) set_insert(rhs_dic(dic), i);
		    (void) dic_family_add_irred(list, dic);
		    (void) set_clear(lhs_dic(dic), n);
		    (void) set_copy(rhs_dic(dic), lhs_set);
		    (void) set_insert(lhs_dic(dic), i);
		    (void) dic_family_add_irred(list, dic);
		    (void) dic_free(dic);
		}
	    }
	    (void) set_free(lhs_set);
	    iter ++;
	}
    }

    FREE(line);
    FREE(symbols);
    FREE(rhs);
    fclose(fp);

    *dic_list = list;
    return TRUE;
}

dic_family_t *
gen_uniq(list)
dic_family_t *list;
{
    dic_family_t *new;
    pset         lastp, p, p1, p2, dic;
    int          n, i, j, base1, base2;
    unsigned int val1, val2;
    bool         **implied;

    n = list->dset_elem;
    new = dic_family_copy(list);
    implied = ALLOC(bool *, n);
    for (i = 0; i < n; i ++) {
	implied[i] = ALLOC(bool, n);
	for (j = 0; j < n; j ++) {
	    implied[i][j] = FALSE;
	}
   }
    lastp = list->dic + list->dcount * list->dic_size;
    for (p = list->dic; p < lastp; p += list->dic_size) {
        p1 = lhs_dic(p);
        p2 = rhs_dic(p);
        foreach_set_element(p1, i, val1, base1) {
 	    foreach_set_element(p2, j, val2, base2) {
 	        if (base1 < base2) {
 		    implied[base1][base2] = TRUE;
 	        }
 	        else {
 		    implied[base2][base1] = TRUE;
 	        }
 	    }
        }
    }
    dic = dic_new(n);
    for (i = 0; i < n; i ++) {
	for (j = i + 1; j < n; j ++) {
	    if (!implied[i][j]) {
		(void) set_clear(lhs_dic(dic), n);
		(void) set_clear(rhs_dic(dic), n);
		(void) set_insert(lhs_dic(dic), i);
		(void) set_insert(rhs_dic(dic), j);
		(void) dic_family_add(new, dic);
		(void) set_clear(lhs_dic(dic), n);
		(void) set_clear(rhs_dic(dic), n);
		(void) set_insert(lhs_dic(dic), j);
		(void) set_insert(rhs_dic(dic), i);
		(void) dic_family_add(new, dic);
	    }
	}
    }
    dic_free(dic);
    for (i = 0; i < n; i ++) {
	FREE(implied[i]);
    }
    FREE(implied);
    return new;
}

dic_family_t *
reduce_seeds(list)
dic_family_t *list;
{
     dic_family_t *new1, *new2;
     pset         lastp, p, cp;
     int          n, k, *cnt, max, max_i, i;

     /* first remove implied seeds */
     lastp = list->dic + list->dcount * list->dic_size;
     for (p = list->dic; p < lastp; p += list->dic_size) {
	 SET(p, ACTIVE);
     }
     k = 0;
     for (p = list->dic; p < lastp; p += list->dic_size) {
	 if (TESTP(p, ACTIVE)) {
	     for (cp = p + list->dic_size; cp < lastp; cp += list->dic_size) {
		 if (dicp_implies(cp, p)) {
		     RESET(cp, ACTIVE);
		     k ++;
		 }
	     }
	 }
     }
     n = list->dset_elem;
     new1 = dic_family_alloc(list->dcount - k, n);
     for (p = list->dic; p < lastp; p += list->dic_size) {
	 if (TESTP(p, ACTIVE)) {
	     (void) dic_family_add(new1, p);
	 }
     }

     /* anchor most frequent state at origin */
     cnt = ALLOC(int, n);
     for (i = 0; i < n; i ++) {
	 cnt[i] = 0;
     }
     lastp = new1->dic + new1->dcount * new1->dic_size;
     for (p = new1->dic; p < lastp; p += new1->dic_size) {
	 for (i = 0; i < n; i ++) {
	     if (is_in_set(p, i)) {
		 cnt[i] ++;
	     }
	 }
     }
     max = cnt[0];
     max_i = 0;
     for (i = 1; i < n; i++) {
	 if (cnt[i] > max) {
	     max = cnt[i];
	     max_i = i;
	 }
     }
     new2 = dic_family_alloc(max, n);

     /* retain all seeds either without max_i or in correct lhs or rhs set */
     lastp = new1->dic + new1->dcount * new1->dic_size;
     for (p = new1->dic; p < lastp; p += new1->dic_size) {
	 if (!is_in_set(lhs_dic(p), max_i)) {
	     (void) dic_family_add(new2, p);
	 }
     }
     FREE(cnt);
     (void) dic_family_free(new1);
     return new2;
}
