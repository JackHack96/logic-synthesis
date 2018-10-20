/**CFile***********************************************************************

  FileName    [random.c]

  PackageName [util]

  Synopsis    [Our own portable random number generator.]

  Copyright   [Copyright (c) 1994-1996 The Regents of the Univ. of California.
  All rights reserved.

  Permission is hereby granted, without written agreement and without license
  or royalty fees, to use, copy, modify, and distribute this software and its
  documentation for any purpose, provided that the above copyright notice and
  the following two paragraphs appear in all copies of this software.

  IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
  CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
  \"AS IS\" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO PROVIDE
  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.]

******************************************************************************/

#include "util.h" 

static char rcsid[] UNUSED = "$Id: random.c,v 1.1.1.1 2003/02/24 22:24:04 wjiang Exp $";

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/* Random generator constants. */
#define MODULUS1 2147483563
#define LEQA1 40014
#define LEQQ1 53668
#define LEQR1 12211
#define MODULUS2 2147483399
#define LEQA2 40692
#define LEQQ2 52774
#define LEQR2 3791
#define STAB_SIZE 64
#define STAB_DIV (1 + (MODULUS1 - 1) / STAB_SIZE)

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/
static long utilRand = 0;
static long utilRand2;
static long shuffleSelect;
static long shuffleTable[STAB_SIZE];

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Initializer for the portable random number generator.]

  Description [Initializer for the portable number generator based on
  ran2 in "Numerical Recipes in C." The input is the seed for the
  generator. If it is negative, its absolute value is taken as seed.
  If it is 0, then 1 is taken as seed. The initialized sets up the two
  recurrences used to generate a long-period stream, and sets up the
  shuffle table.]

  SideEffects [None]

  SeeAlso     [random]

******************************************************************************/
void
util_srandom(seed)
long seed;
{
    int i;

    if (seed < 0)       utilRand = -seed;
    else if (seed == 0) utilRand = 1;
    else                utilRand = seed;
    utilRand2 = utilRand;
    /* Load the shuffle table (after 11 warm-ups). */
    for (i = 0; i < STAB_SIZE + 11; i++) {
	long int w;
	w = utilRand / LEQQ1;
	utilRand = LEQA1 * (utilRand - w * LEQQ1) - w * LEQR1;
	utilRand += (utilRand < 0) * MODULUS1;
	shuffleTable[i % STAB_SIZE] = utilRand;
    }
    shuffleSelect = shuffleTable[1 % STAB_SIZE];
} /* end of util_srandom */

/**Function********************************************************************

  Synopsis    [Portable random number generator.]

  Description [Portable number generator based on ran2 from "Numerical
  Recipes in C." It is a long period (> 2 * 10^18) random number generator
  of L'Ecuyer with Bays-Durham shuffle. Returns a long integer uniformly
  distributed between 0 and 2147483561 (inclusive of the endpoint values).
  The ranom generator can be explicitly initialized by calling
  srandom. If no explicit initialization is performed, then the
  seed 1 is assumed.]

  SideEffects []

  SeeAlso     [srandom]

******************************************************************************/
long
util_random()
{
    int i;	/* index in the shuffle table */
    long int w; /* work variable */

    /* utilRand == 0 if the geneartor has not been initialized yet. */
    if (utilRand == 0) util_srandom(1);

    /* Compute utilRand = (utilRand * LEQA1) % MODULUS1 avoiding
    ** overflows by Schrage's method.
    */
    w          = utilRand / LEQQ1;
    utilRand   = LEQA1 * (utilRand - w * LEQQ1) - w * LEQR1;
    utilRand  += (utilRand < 0) * MODULUS1;

    /* Compute utilRand2 = (utilRand2 * LEQA2) % MODULUS2 avoiding
    ** overflows by Schrage's method.
    */
    w          = utilRand2 / LEQQ2;
    utilRand2  = LEQA2 * (utilRand2 - w * LEQQ2) - w * LEQR2;
    utilRand2 += (utilRand2 < 0) * MODULUS2;

    /* utilRand is shuffled with the Bays-Durham algorithm.
    ** shuffleSelect and utilRand2 are combined to generate the output.
    */

    /* Pick one element from the shuffle table; "i" will be in the range
    ** from 0 to STAB_SIZE-1.
    */
    i = shuffleSelect / STAB_DIV;
    /* Mix the element of the shuffle table with the current iterate of
    ** the second sub-generator, and replace the chosen element of the
    ** shuffle table with the current iterate of the first sub-generator.
    */
    shuffleSelect   = shuffleTable[i] - utilRand2;
    shuffleTable[i] = utilRand;
    shuffleSelect  += (shuffleSelect < 1) * (MODULUS1 - 1);
    /* Since shuffleSelect != 0, and we want to be able to return 0,
    ** here we subtract 1 before returning.
    */
    return(shuffleSelect - 1);

} /* end of util_random */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/



