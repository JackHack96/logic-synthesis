/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/jedi/weights.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:08 $
 *
 */
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */
#include "inc/jedi.h"

int     compute_weights();		/* forward declaration */
int     add_input_weights();		/* forward declaration */
int     add_output_weights();		/* forward declaration */
int     add_normal_inputs();		/* forward declaration */
int     add_normal_outputs();		/* forward declaration */
int     add_variation_inputs();		/* forward declaration */
int     add_variation_outputs();	/* forward declaration */
int     normal_input_proximity();	/* forward declaration */
int     normal_output_proximity();	/* forward declaration */

compute_weights()
{
    int i, e;

    if (verboseFlag) {
	(void) fprintf(stderr, "computing weights ...\n");
    }

    /*
     * only compute weights if the variable
     * is not a boolean
     */
    if (weightType == OUTPUT) {
	for (i=0; i<no; i++) {
	    if (!outputs[i].boolean_flag) {
		add_output_weights(i);
	    }
	}

	if (!oneplaneFlag) {
	    for (i=0; i<ni; i++) {
		if (!inputs[i].boolean_flag) {
		    e = inputs[i].enumtype_ptr;
		    if (!enumtypes[e].output_flag) {
			add_input_weights(i);
		    }
		}
	    }
	}
    } else if (weightType == INPUT) {
	for (i=0; i<ni; i++) {
	    if (!inputs[i].boolean_flag) {
		add_input_weights(i);
	    }
	}

	if (!oneplaneFlag) {
	    for (i=0; i<no; i++) {
		if (!outputs[i].boolean_flag) {
		    e = outputs[i].enumtype_ptr;
		    if (!enumtypes[e].input_flag) {
			add_output_weights(i);
		    }
		}
	    }
	}
    } else {
	for (i=0; i<ni; i++) {
	    if (!inputs[i].boolean_flag) {
		add_input_weights(i);
	    }
	}

	for (i=0; i<no; i++) {
	    if (!outputs[i].boolean_flag) {
		add_output_weights(i);
	    }
	}
    }
} /* end of compute_weights */


add_input_weights(i)
int i;
{
    int e;

    e = inputs[i].enumtype_ptr;
    enumtypes[e].input_flag = TRUE;

    if (variationFlag) {
	add_variation_inputs(i);
    } else {
	add_normal_inputs(i);
    }
} /* end of add_input_weights */


add_output_weights(i)
int i;
{
    int e;

    e = outputs[i].enumtype_ptr;
    enumtypes[e].output_flag = TRUE;

    if (variationFlag) {
	add_variation_outputs(i);
    } else {
	add_normal_outputs(i);
    }
} /* end of add_output_weights */


add_normal_inputs(i)
int i;
{
    int x, y;
    int sx, sy;
    int e;
    int we;
    Boolean c, c1, c2;

    e = inputs[i].enumtype_ptr;
    for (x=0; x<np; x++) {
	for (y=0; y<np; y++) {
	    if (x<y) {
		/*
		 * watch out for don't cares
		 */
		c1 = check_dont_care(inputs[i].entries[x].token);
		c2 = check_dont_care(inputs[i].entries[y].token);
		c = (!c1 && !c2);
		if (c) {
		    sx = inputs[i].entries[x].symbol_ptr;
		    sy = inputs[i].entries[y].symbol_ptr;
		    if (sx != sy){
			we = normal_output_proximity(x, y);
			enumtypes[e].links[sx][sy].weight += we;
			enumtypes[e].links[sy][sx].weight += we;
		    }
		}
	    }
	}
    }
} /* end of add_normal_inputs */


add_normal_outputs(i)
int i;
{
    int x, y;
    int sx, sy;
    int e;
    int we;
    Boolean c, c1, c2;

    e = outputs[i].enumtype_ptr;
    for (x=0; x<np; x++) {
	for (y=0; y<np; y++) {
	    if (x<y) {
		/*
		 * watch out for don't cares
		 */
		c1 = check_dont_care(outputs[i].entries[x].token);
		c2 = check_dont_care(outputs[i].entries[y].token);
		c = (!c1 && !c2);
		if (c) {
		    sx = outputs[i].entries[x].symbol_ptr;
		    sy = outputs[i].entries[y].symbol_ptr;
		    if (sx != sy){
		        we = normal_input_proximity(x, y);
			enumtypes[e].links[sx][sy].weight += we;
			enumtypes[e].links[sy][sx].weight += we;
		    }
		}
	    }
	}
    }
} /* end of add_normal_outputs */


int normal_input_proximity(x, y)
int x, y;
{
    int i;
    int prox;
    int pprox;
    Boolean c1, c2, c3;
    int e;

    prox = 0; pprox = 0;
    for (i=0; i<ni; i++) {
	c1 = !strcmp(inputs[i].entries[x].token, inputs[i].entries[y].token);
	c2 = check_dont_care(inputs[i].entries[x].token);
	c3 = check_dont_care(inputs[i].entries[y].token);
	if (c1 && !c2 && !c3) {
	    /*
	     * if two tokens are equal but neither is a dont care
	     */
	    if (inputs[i].boolean_flag) {
		prox ++;
	    } else {
		e = inputs[i].enumtype_ptr;
		prox += enumtypes[e].nb;
	    }
	} else if (c2 && c3) {
	    /*
	     * else if both tokens are dont cares
	     */
	    if (inputs[i].boolean_flag) {
		prox ++;
	    } else {
		e = inputs[i].enumtype_ptr;
		prox += enumtypes[e].nb;
	    }
	} else if (c2 || c3) {
	    /*
	     * else if one token is a dont care but the other is not
	     */
	    if (inputs[i].boolean_flag) {
		pprox ++;
	    } else {
		e = inputs[i].enumtype_ptr;
		pprox += enumtypes[e].nb;
	    }
	}
    }

    return (prox + pprox/2);
} /* end of normal_input_proximity */


int normal_output_proximity(x, y)
int x, y;
{
    int i;
    int prox;
    int pprox;
    Boolean c1, c2, c3;
    int e;

    prox = 0; pprox = 0;
    for (i=0; i<no; i++) {
	c1 = !strcmp(outputs[i].entries[x].token, outputs[i].entries[y].token);
	c2 = check_dont_care(outputs[i].entries[x].token);
	c3 = check_dont_care(outputs[i].entries[y].token);
	if (c1 && !c2 && !c3) {
	    /*
	     * if two tokens are equal but neither is a dont care
	     */
	    if (outputs[i].boolean_flag) {
		prox ++;
	    } else {
		e = outputs[i].enumtype_ptr;
		prox += enumtypes[e].nb;
	    }
	} else if (c2 && c3) {
	    /*
	     * else if both tokens are dont cares
	     */
	    if (outputs[i].boolean_flag) {
		prox ++;
	    } else {
		e = outputs[i].enumtype_ptr;
		prox += enumtypes[e].nb;
	    }
	} else if (c2 || c3) {
	    /*
	     * else if one token is a dont care but the other is not
	     */
	    if (outputs[i].boolean_flag) {
		pprox ++;
	    } else {
		e = outputs[i].enumtype_ptr;
		pprox += enumtypes[e].nb;
	    }
	}
    }

    return (prox + pprox/2);
} /* end of normal_output_proximity */


add_variation_inputs(i)
int i;
{
    int si, sj;
    int e;
    int ie;
    int n;
    int we;
    int nb;
    int nw_si, nw_sj;
    int k, l;

    e = inputs[i].enumtype_ptr;
    for (si=0; si<enumtypes[e].ns; si++) {
	for (sj=0; sj<enumtypes[e].ns; sj++) {
	    if (si<sj) {
		for (n=0; n<no; n++) {
		    if (outputs[n].boolean_flag) {
			/*
			 * Off
			 */
			nw_si = nw_sj = 0;
			for (k=0; k<np; k++) {
			    if (!strcmp(outputs[n].entries[k].token, "1")) {
				if (!strcmp(inputs[i].entries[k].token,
				  enumtypes[e].symbols[si].token)) {
				    nw_si++;
				} else if (!strcmp(inputs[i].entries[k].token,
				  enumtypes[e].symbols[sj].token)) {
				    nw_sj++;
				}
			    }
			}
			we = nw_si * nw_sj;
			enumtypes[e].links[si][sj].weight += we;
			enumtypes[e].links[sj][si].weight += we;
		    } else {
			ie = outputs[n].enumtype_ptr;
			nb = enumtypes[ie].nb;

			/*
			 * for each symbol
			 */
			for (l=0; l<enumtypes[ie].ns; l++) {
			    nw_si = nw_sj = 0;
			    for (k=0; k<np; k++) {
				if (!strcmp(outputs[n].entries[k].token, 
				  enumtypes[ie].symbols[l].token)) {
				    if (!strcmp(inputs[i].entries[k].token,
				      enumtypes[e].symbols[si].token)) {
					nw_si++;
				    } else if 
				      (!strcmp(inputs[i].entries[k].token,
				      enumtypes[e].symbols[sj].token)) {
					nw_sj++;
				    }
				}
			    }
			    we = nw_si * nw_sj * nb;
			    enumtypes[e].links[si][sj].weight += we;
			    enumtypes[e].links[sj][si].weight += we;
			}
		    }
		}
	    }
	}
    }
} /* end of add_variation_inputs */


add_variation_outputs(i)
int i;
{
    int si, sj;
    int e;
    int ie;
    int n;
    int we;
    int nb;
    int nw_si, nw_sj;
    int k, l;

    e = outputs[i].enumtype_ptr;
    for (si=0; si<enumtypes[e].ns; si++) {
	for (sj=0; sj<enumtypes[e].ns; sj++) {
	    if (si<sj) {
		for (n=0; n<ni; n++) {
		    if (inputs[n].boolean_flag) {
			/*
			 * I-on
			 */
			nw_si = nw_sj = 0;
			for (k=0; k<np; k++) {
			    if (!strcmp(inputs[n].entries[k].token, "1")) {
				if (!strcmp(outputs[i].entries[k].token,
				  enumtypes[e].symbols[si].token)) {
				    nw_si++;
				} else if (!strcmp(outputs[i].entries[k].token,
				  enumtypes[e].symbols[sj].token)) {
				    nw_sj++;
				}
			    }
			}
			we = nw_si * nw_sj;
			enumtypes[e].links[si][sj].weight += we;
			enumtypes[e].links[sj][si].weight += we;

			/*
			 * I-off
			 */
			nw_si = nw_sj = 0;
			for (k=0; k<np; k++) {
			    if (!strcmp(inputs[n].entries[k].token, "0")) {
				if (!strcmp(outputs[i].entries[k].token,
				  enumtypes[e].symbols[si].token)) {
				    nw_si++;
				} else if (!strcmp(outputs[i].entries[k].token,
				  enumtypes[e].symbols[sj].token)) {
				    nw_sj++;
				}
			    }
			}
			we = nw_si * nw_sj;
			enumtypes[e].links[si][sj].weight += we;
			enumtypes[e].links[sj][si].weight += we;
		    } else {
			ie = inputs[n].enumtype_ptr;
			nb = enumtypes[ie].nb;

			/*
			 * for each symbol
			 */
			for (l=0; l<enumtypes[ie].ns; l++) {
			    nw_si = nw_sj = 0;
			    for (k=0; k<np; k++) {
				if (!strcmp(inputs[n].entries[k].token, 
				  enumtypes[ie].symbols[l].token)) {
				    if (!strcmp(outputs[i].entries[k].token,
				      enumtypes[e].symbols[si].token)) {
					nw_si++;
				    } else if 
				      (!strcmp(outputs[i].entries[k].token,
				      enumtypes[e].symbols[sj].token)) {
					nw_sj++;
				    }
				}
			    }
			    we = nw_si * nw_sj * nb;
			    enumtypes[e].links[si][sj].weight += we;
			    enumtypes[e].links[sj][si].weight += we;
			}
		    }
		}
	    }
	}
    }
} /* end of add_variation_outputs */
