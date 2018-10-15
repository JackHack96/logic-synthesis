/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/xln_lindo.c,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2005/03/08 05:31:12 $
 *
 */
/*
 *  This file contains routines for formulating a network node merging
 *	problem by interger programming
 *  functions contained in this file:
 *	formulate_Lindo()
 *	get_Lindo_result()
 *
 *  Import descriptions:
 *
 */

#include "sis.h"
#include "pld_int.h"
#include <math.h>

/*
 *  formulate_Lindo()
 *	Formulate an integer programming in LINDO format
 *  Arguments:
 *  ---------
 *	coeff<input>	: coefficient sparse matrix representing which matching
 *			  (column) includes which nodes (rows)
 *  Local variables:
 *  ---------------
 *	numTerm		: counter for the number of terms per line for lindo
 *	termsPerLine	: maximum number of terms per line for lindo
 */

char *
formulate_Lindo(coeff)

sm_matrix	*coeff;


{


int		i;
int		numTerm;
int		termsPerLine = 5;
int		flag;
FILE		*lindoFile;
sm_row		*row;
sm_element	*elem;
char 		*lindo_in;
int		sfd;



lindo_in = ALLOC(char, 1000);
(void)sprintf(lindo_in, "/usr/tmp/merge.lindo_in.XXXXXX");
sfd = mkstemp(lindo_in);
if(sfd == -1) {
    (void) fprintf(siserr, "cannot create temp lindo file\n");
    exit(1);
}
lindoFile = fdopen(sfd,"w");
if(lindoFile == NULL) {
    (void) fprintf(siserr, "No space on disk for ILP formulation\n");
    exit(1);
}

/* Generate objective function : MAX x1 + x2 + ..... + xn */
/*--------------------------------------------------------*/
(void) fprintf(lindoFile,"MAX ");
for (numTerm = 0, i = 0; i < coeff->ncols; ++numTerm, ++i) {

    if (i == 0) {
	(void) fprintf(lindoFile,"x0 ");
    } else {
	if (numTerm < termsPerLine) {
	    (void) fprintf(lindoFile,"+ x%d ", i);
	} else {
	    numTerm = 0;
	    (void) fprintf(lindoFile,"\n + x%d ", i);
	}
    }
}
(void) fprintf(lindoFile,"\n");

/* Generate constraints : xi1 + xi2 + ..... + xim <= 1 */
/*-----------------------------------------------------*/
(void) fprintf(lindoFile,"ST\n");

sm_foreach_row(coeff, row) {

    flag = OFF;
    numTerm = 0;
    sm_foreach_row_element(row, elem) {

	if (flag == OFF) {
	    flag = ON;
	    ++numTerm;
	    (void) fprintf(lindoFile,"x%d ", elem->col_num);
	    continue;
	}
	if (numTerm < termsPerLine) {
	    ++numTerm;
	    (void) fprintf(lindoFile,"+ x%d ", elem->col_num);
	} else {
	    numTerm = 1;
	    (void) fprintf(lindoFile,"\n + x%d ", elem->col_num);
	}
	    
    }
    (void) fprintf(lindoFile,"<= 1\n");
}
(void) fprintf(lindoFile,"END\n");


/* Declaration of integrality */
/*----------------------------*/
for (i = 0; i < coeff->ncols; ++i) {
    (void) fprintf(lindoFile,"INTEGER x%d\n",i);
}

(void) fprintf(lindoFile,"GO\n");
(void) fprintf(lindoFile,"QUIT\n");
(void) fclose(lindoFile);

return lindo_in;
}





/*
 *  get_Lindo_result()
 *	Get lindo results from file and write them to arrays
 */


int
get_Lindo_result(cand_node_array, coeff, match1_array, match2_array, lindo_out, outputFile)

array_t		*cand_node_array;	/* nodes in matching problem */
sm_matrix	*coeff;		/* incidence matrix: row=node, col=matching */
array_t		*match1_array;	/* nodes on one end of matching edge */
array_t		*match2_array;	/* nodes on other end of matching edge */
char		*lindo_out;
FILE		*outputFile;    /* details of the merge results go here */
{

int		i;
int		endFlg;		/* flag, if ON, EOF encountered */
int		infeasibleFlg;	/* flag, if ON, no feasible solution obtained */
char		word[50];	/* storage of string from lindo output file */
int		*LPresult;	/* answer of max matching problem */
int		match_id;	/* sequence number for matching candidate */
float		result;		/* result value from LINDO file */
int		num_match;	/* cardinality of max matching */
sm_col		*col;		/* column of coeff */
node_t		*node;		/* node in the network */
FILE		*lindoFile;	/* output file from Lindo */

/* Initialization */
/*----------------*/
LPresult = ALLOC(int, coeff->ncols);
lindoFile = fopen(lindo_out,"r");

/* Scan each set of solution in the report
 * "REDUCED COST" is the keyword appearing just before the solution set
 * "NO FEASIBLE" is the keyword indicating no feasible solution obtained
 *----------------------------------------------------------------------*/
for (endFlg = infeasibleFlg = OFF;;) {
    for (;;) {
	if (fscanf(lindoFile,"%s",word) == EOF) {
	    endFlg = ON;
	    break;
	}
	if (strcmp(word,"NO") == 0) {
	    (void) fscanf(lindoFile,"%s",word);
	    if (strcmp(word,"FEASIBLE") == 0) {
		endFlg = ON;
		infeasibleFlg = ON;
		break;
	    }
	}
	if (strcmp(word,"REDUCED") == 0) {
	    (void) fscanf(lindoFile,"%s",word);
	    break;
	}
    }
    if (endFlg == ON) {
	break;
    }

/* Now seek the result
 * Note that variable names are changed to upper case letters in the result 
 *	ex) x5 (lindo.in) --> X5 (lindo.out)
 *-----------------------------------------------------------------------*/
    for (; fscanf(lindoFile,"%s",word) != EOF; ) {

/* If "ROW" appears, solution set section ends */
/*---------------------------------------------*/
	if (strcmp(word,"ROW") == 0) {
	    break;
	}
	if (word[0] == 'X') {
	    (void) sscanf(word,"X%d",&match_id);
	    (void) fscanf(lindoFile,"%f",&result);
	    LPresult[match_id] = RINT(result);
	} else {
	    continue;
	}
    }
}
(void) fclose(lindoFile);

/* If no feasible solution, exit */
/*-------------------------------*/
if (infeasibleFlg == ON) {
    (void) (void) printf("NO FEASIBLE SOLUTION OBTAINED\n");
    exit(8);
}

for (num_match = 0, i = 0; i < coeff->ncols; ++i) {
    if (LPresult[i] == 1) {
	++num_match;
	col = sm_get_col(coeff, i);
	node = array_fetch(node_t *, cand_node_array, col->first_row->row_num);
	array_insert_last(node_t *, match1_array, node);
	node = array_fetch(node_t *, cand_node_array, col->last_row->row_num);
	array_insert_last(node_t *, match2_array, node);
    }
}

(void) fprintf(outputFile,"Total number of matching = %d\n\n", num_match);

/* Termination */
/*-------------*/
FREE(LPresult);

return(num_match);

}
