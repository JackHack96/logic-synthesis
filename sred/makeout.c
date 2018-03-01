/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/makeout.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:11 $
 *
 */

#include <stdio.h>
#include "reductio.h"

makeout()

{
	register int state, j;
	NAMETABLE *name;

	fprintf(stdout, ".i %d\n", nis);
	fprintf(stdout, ".o %d\n", nos);
	fprintf(stdout, ".s %d\n", copertura1->count);

	if (reset > 0) {
		fprintf(stdout, ".r s%d\n", reset);
	}

	for (j = 0; j < newnp; j++) {
		fprintf (stdout, "%s ", itable[j].input);

		fprintf (stdout, "%s ", itable[j].pstate);

		fprintf (stdout, "%s ", itable[j].nstate);

		fprintf (stdout, "%s\n", itable[j].output);
	}

	fprintf(stdout, ".e\n");
}

