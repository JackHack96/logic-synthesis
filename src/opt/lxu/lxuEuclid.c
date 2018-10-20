#include "lxuInt.h"
#include "lxuEuclidInt.h"

#include "time.h"

const double eps = 0.5;
static int plotLayout (char *plotfile, Lxu_Matrix *p);

// Utility functions

weightType Lxu_EuclidCombine(weightType l, weightType p)
{
	assert ((p!=weightType_POS_INF) && (p!=weightType_NEG_INF));
	return ((es.alpha)*l) + ((es.beta)*p);
}

int coord_comparison(const void *a, const void *b)
{
	// TODO: Costly assertion. Remove.
	assert ((((Lxu_Edge*)a)->x != coordType_POS_INF) && (((Lxu_Edge*)b)->x != coordType_POS_INF));

    if (((Lxu_Edge*)a)->x < ((Lxu_Edge*)b)->x)
        return -1;
	else if (((Lxu_Edge*)a)->x > ((Lxu_Edge*)b)->x)
        return 1;
    else
        return 0;
}

void Lxu_EuclidPrintPoint (const Lxu_Point *p)
{
	printf ("(%g, %g)", p->x, p->y);
}

void Lxu_EuclidSetPoint (Lxu_Point *lv, const Lxu_Point *p)
{
	lv->x = p->x;
	lv->y = p->y;
}

void Lxu_EuclidPrintRect (const Lxu_Rect *r)
{
	printf ("[");
	Lxu_EuclidPrintPoint (&r->tl);
	printf (" ");
	Lxu_EuclidPrintPoint (&r->br);
	printf ("]");
}

void Lxu_EuclidResetRect (Lxu_Rect *r)
{
	r->tl.x = coordType_POS_INF;
}

void Lxu_EuclidSetRect (Lxu_Rect *r, coord tlx, coord tly, coord brx,  coord bry)
{
	r->tl.x = tlx;
	r->tl.y = tly;
	r->br.x = brx;
	r->br.y = bry;
}

coord Lxu_EuclidComputeSemiperimeter (Lxu_Rect *r)
{
	//assert (r->tl.x != coordType_POS_INF);
	assert (Lxu_EuclidGoodRect(r));
	return (r->br.x - r->tl.x) + (r->br.y - r->tl.y);
}

/* Computes the bounding box of the rectangle and the point */

void Lxu_EuclidAddToBoundingBox (Lxu_Rect *net, const Lxu_Point *p)
{
	if (net->tl.x == coordType_POS_INF) {

		net->tl.x = p->x;
		net->br.x = p->x;
		net->tl.y = p->y;
		net->br.y = p->y;

	} else {

		if (p->x < net->tl.x)
			net->tl.x = p->x;

		if (p->x > net->br.x)
			net->br.x = p->x;

		if (p->y < net->tl.y)
			net->tl.y = p->y;

		if (p->y > net->br.y)
			net->br.y = p->y;
	}
}

int Lxu_EuclidGoodRect (Lxu_Rect *r)
{
	// An empty rectangle is NOT a good rectangle
	return (r->tl.x != coordType_POS_INF) && (r->tl.x <= r->br.x) && (r->tl.y <= r->br.y);
}

coord Lxu_ComputeHPWL(Lxu_Matrix *p)
{
	// For each variable (positive and negative literal considered together) go through its
	// fanouts to compute the HPWL of its output net

	Lxu_Lit *pLit;
    Lxu_Var *pVar;
	Lxu_Rect bb;
	coord totalhpwl = 0;
	int pins;
	
	Lxu_MatrixForEachVariable (p, pVar) {

		Lxu_EuclidResetRect (&bb);
		pins = 0;

		// Do for -ve literal
		Lxu_VarForEachLiteral (pVar, pLit) {
			assert (pVar = pLit->pVar);
			Lxu_EuclidAddToBoundingBox(&bb, &pLit->pCube->pVar->position);
			pins++;
		}

		pVar = pVar->pNext;

		// Do for +ve literal
		Lxu_VarForEachLiteral (pVar, pLit) {
			assert (pVar = pLit->pVar);
			Lxu_EuclidAddToBoundingBox(&bb, &pLit->pCube->pVar->position);
			pins++;
		}

		if (pins > 0) totalhpwl += Lxu_EuclidComputeSemiperimeter(&bb);
	}

	return totalhpwl;	
}

// Public functions

void Lxu_EuclidStart(weightType alpha, weightType beta, int interval)
{
	es.alpha = alpha;
	es.beta = beta;
	es.interval = interval;
	Lxu_EuclidSetRect (&es.arena, 0, 0, 100, 100);
	
	if (verbose) printf ("Euclid: Started. alpha = %g, beta = %g, interval = %d\n", es.alpha, es.beta, es.interval);
}

void Lxu_EuclidStop()
{
	if (verbose) printf ("Euclid: Stopped\n");
}

void Lxu_EuclidRandomPlacement (Lxu_Matrix *p, Lxu_Rect *r)
{
	Lxu_Var *pVar, *pOther;

	assert (Lxu_EuclidGoodRect(r));

	srand((unsigned)time( NULL ));

	printf ("Euclid: Random placement invoked!\n");

	Lxu_MatrixForEachVariable (p, pVar) {

		double x = (1.0 * rand() / RAND_MAX);
		double y = (1.0 * rand() / RAND_MAX);

		pVar->position.x = r->tl.x + x * (r->br.x - r->tl.x);
		pVar->position.y = r->tl.y + y * (r->br.y - r->tl.y);
		es.numNodes++;

		pOther = pVar;
		pVar = pVar->pNext;		// Also place the other literal at the same location

		Lxu_EuclidSetPoint (&pVar->position, &pOther->position);
	}

}

void Lxu_EuclidExternalPlacement (Lxu_Matrix *p, Lxu_Rect *r)
{
	FILE *geo;
	Lxu_Var *pVar = NULL;
	int code;
	char temp[1024], satFile[24], plFile[24];
	static int plotcount = 0;

	// Note: following stuff assumes r to be (0,0) - (100,100) 
	assert (Lxu_EuclidGoodRect(r));

	sprintf(satFile, "euclid%03d.sat", plotcount);
	geo = fopen(satFile, "w");

	/* File format for .sat file:
	 * each section is a "var xxxx" followed by a list of inputs vars ended by a "end"
	 */
	
	assert ((geo != NULL) && "Couldn't write to .sat file");

	/* Write out the netlist and intially assign all guys to (-1, -1) which is outside
	 * the placement area. Later on external placer will place all the variables which correspond
	 * to the +ve literals and then we go in and place all the -ve literals at the same
	 * location as the +ve ones. 
	 **/

	Lxu_MatrixForEachVariable (p, pVar) {

		pVar->position.x = -1;	// To track if every guy is read in or not
		pVar->position.y = -1;	

		pVar = pVar->pNext;		// The first var corresponds to the complement. We skip this
								// since complement and node are placed at the same location

		pVar->position.x = -1;
		pVar->position.y = -1;	

		fprintf (geo, "var %d\n", pVar->iVar); 

		/* Computation disabled at ntkLxu */
		/* fprintf (geo, "%s %d\n", pVar->Type?"fixed":"var", pVar->iVar); */

		/* Only one output */
		/* cycle through all inputs */
		{
			int n;
			Lxu_Cube *pCube;

			/* Go through each cube of this variable */

			for (pCube=pVar->pFirst, n=0; n < pVar->nCubes; n++, pCube = pCube->pNext) {
				Lxu_Lit *pLit;

				/* Find each literal in the support of the cube */
				Lxu_CubeForEachLiteral(pCube, pLit) {

					// Whenever neg polarity is involved
					// we want gordian to connected to the positive
					// polarity literal, i.e. to the node
					int node = pLit->pVar->iVar;
					node = ((node % 2) != 0) ? node : (node + 1);
					
					fprintf (geo, " %d", node);
				}

				fprintf (geo, "\t");
			}
		}
		
		fprintf (geo, "\nend\n");
	}

	fclose(geo);

/*
	printf ("Euclid: Calling CAPO: 1");
	fflush(stdout);
	code = system ("translate-to-capo > /dev/null");
	assert ((code == 0) && "translate-to-capo failed"); 

	printf (" 2");
	fflush(stdout);
	code = system ("echo HGraphWPins : foo.nets foo.nodes > foo.aux");
	assert ((code == 0) && "Creation of foo.aux failed"); 

	printf (" 3");
	fflush(stdout);
	code = system ("LayoutGen-Linux.exe -f foo.aux -saveAs bar > layoutgen.log 2>&1");
	assert ((code == 0) && "LayoutGen failed"); 
	
	printf (" 4");
	fflush(stdout);

//	if (p->plot) 
//		sprintf (temp, "MetaPl-Capo8.6_linux.exe -save -f bar.aux -plot bar-%03d > capo.log 2>&1", plotcount);

	sprintf (temp, "MetaPl-Capo8.7_linux.exe -save -f bar.aux > capo.log 2>&1");

	code = system (temp);
	assert ((code == 0) && "Capo Failed"); 
	
	printf (" 5");
	fflush(stdout);
*/

	/* 

	   Interface with external script: mvsis provides a file euclidxxx.sat to the external script by 
           making a system call of the form:

		script euclidxxx

           The script assumes that the .sat file is euclidxxx.sat and reads that in and produces the
           placed info in a file euclidxxx.pl and returns. The nodes in the euclidxxx.pl file should 
           be in the same order as that in euclidxxx.sat. Currently no check is done to verify this.
           capo.sh is an example wrapper that conforms to this interface.

        */   

	sprintf(temp, "%s euclid%03d", p->extPlacerCmd, plotcount);
	code = system (temp);

	if (code != 0)
	{
		fprintf(stderr, "Error invoking external placer %s. Shell returned %d. Aborting.\n", p->extPlacerCmd, code);
		exit(1);
	}

	/* Now we read the placed cell info back and read in the coords */

	{
		const int size = 1024;
		char buffer[1024];
		FILE *loc;
		char name[32];
		double x, y, maxx = INT_MIN, maxy = INT_MIN;
		int res;

		sprintf(plFile, "euclid%03d.pl", plotcount);
		loc = fopen(plFile, "r");

		assert ((loc != NULL) && "Couldn't open .pl file");

		pVar = p->lVars.pHead;

		while (pVar != 0) {

			if (0 == fgets(buffer, size, loc))
				break;

			buffer[strlen(buffer) - 1] = 0;	// kill new line

			if (strcmp(buffer, "UCLA pl 1.0") == 0)
				continue;

			if (buffer[0]=='#')		// comment
				continue;

			if (buffer[0]==0)
				continue;

			res = sscanf(buffer, " %s %lg %lg ", name, &x, &y);

			assert (res == 3);

			//printf("Node %s at (%g, %g)\n", name, x, y);

			assert (pVar != 0);		// The file must not have more nodes than we are looking for

			pVar->position.x = x + 5;		// Center of cell
			pVar->position.y = y + 5;		// Center of cell
			es.numNodes++;

			if (x > maxx) maxx = x;
			if (y > maxy) maxy = y;

			if (x < -eps || y < -eps)
			{
				fprintf (stderr, "\nERROR: cell not in positive quadrant (%g, %g)\n", x, y);
				assert (x >= -eps && "Placement area assumed to be in positive quadrant");
				assert (y >= -eps && "Placement area assumed to be in positive quadrant");
			}

			pVar = pVar->pNext->pNext;		// Next even position 
		}

		fclose(loc);

		// printf (" 6\n");

		/* So now we go ahead and assign each even variable the position of the
		 * odd variable after it
		 */

		maxx += 10;		
		maxy += 10;

		Lxu_MatrixForEachVariable (p, pVar) {

			assert ((pVar->position.x != -1) && "Not all node locations read back in");
			assert ((pVar->position.y != -1) && "Not all node locations read back in");

			pVar->position.x = (100 * pVar->position.x) / maxx;
			pVar->position.y = (100 * pVar->position.y) / maxy;
		
			Lxu_EuclidSetPoint (&(pVar->pNext->position), &pVar->position);

			pVar = pVar->pNext;		// Skip the next guy; we just did it
		}

	}	/* End of read-in block */

#ifdef WIN32
        _unlink( satFile );
        _unlink( plFile );
#else
        unlink( satFile );
        unlink( plFile );
#endif

	/* Debug: plot locations */
	if (p->plot) {
		char buffer[1024];
		sprintf(buffer, "euclid%03d.gpl", plotcount);
		plotLayout (buffer, p);
	}

	plotcount++;		// For unique filenames
}

int plotLayout (char *plotfile, Lxu_Matrix *p)
{
	Lxu_Var *pVar = NULL;
	FILE *pf = fopen(plotfile, "w");

	assert (pf && "Could not open plot file");

	Lxu_MatrixForEachVariable (p, pVar) {
		pVar = pVar->pNext;
		fprintf (pf, "set label \"%d\" at %g, %g\n", pVar->iVar, pVar->position.x + 2, pVar->position.y);
	}

	fprintf(pf, "\n\nplot[:][:] '-' w p\n\n");

	Lxu_MatrixForEachVariable (p, pVar) {
		pVar = pVar->pNext;
		fprintf (pf, "\t%g\t%g\n", pVar->position.x, pVar->position.y);
	}

	fprintf(pf, "EOF\npause -1\n");
	fclose(pf);
	return 0;
}

/* Every divisor stores with it the list of cube pairs that generate it.
 * This function iterates through that list and returns true if the
 * given cube is part of that list
 * (If the given cube is part of that list, that means that if the divisor
 * is used, the given cube will cease to exist)
 */

int isCubeInPairs(Lxu_Cube *pCube, Lxu_Double *pDiv)
{
	Lxu_Pair *pPair;
	Lxu_DoubleForEachPair (pDiv, pPair) {
		if (pPair->pCube1 == pCube || pPair->pCube2 == pCube)
			return 1;
	}
	return 0;
}

void Lxu_EuclidComputeOptimalCostAux(Lxu_Edge temp[], int nn, Lxu_EuclidTuple *a)
{
	int i;

	a->cost = 0;

	qsort (temp, 2*nn, sizeof(Lxu_Edge), coord_comparison);

	for (i=0; i<2*nn; i++) {
		if ((i<nn) && (temp[i].l==0))		// 
			a->cost -= temp[i].x;	
		else if ((i>=nn) && (temp[i].l==1))
			a->cost += temp[i].x;
	}

	a->x  = (temp[nn-1].x + temp[nn].x) / 2;
}

void Lxu_EuclidComputeOptimalCostX(const Lxu_Rect nets[], int nn, Lxu_EuclidTuple *ans)
{
//	Lxu_Edge temp[2*nn]; 
	Lxu_Edge * temp = ALLOC( Lxu_Edge, 2*nn ); // changed by alanmi
	int i;
	
	//Lxu_Edge temp[12] = {{0,1},{4,0},{18,1},{20,0},{12,1},{14,0},{12,1},{20,0},{2,1},{14,0},{0,1},{4,0}};
	//nn = 6;

	for (i=0; i<nn; i++) {
		temp[i*2].x = nets[i].tl.x;
		temp[i*2].l = 1;
		temp[i*2+1].x = nets[i].br.x;
		temp[i*2+1].l = 0;
	}

	Lxu_EuclidComputeOptimalCostAux(temp, nn, ans);

    FREE( temp ); // added by alanmi
}

void Lxu_EuclidComputeOptimalCostY(const Lxu_Rect nets[], int nn, Lxu_EuclidTuple *ans)
{
//	Lxu_Edge temp[2*nn]; 
	Lxu_Edge * temp = ALLOC( Lxu_Edge, 2*nn ); // changed by alanmi
	int i;
	
	for (i=0; i<nn; i++) {
		temp[i*2].x = nets[i].tl.y;
		temp[i*2].l = 1;
		temp[i*2+1].x = nets[i].br.y;
		temp[i*2+1].l = 0;
	}

	Lxu_EuclidComputeOptimalCostAux(temp, nn, ans);
    FREE( temp ); // added by alanmi
}

/* This iterator pattern is used to grab the variables in the support of
 * a double cube divisor
 */

void Lxu_VarIteratorStart(Lxu_Double *pDiv, Lxu_Lit **ppLit1, Lxu_Lit **ppLit2)
{
	*ppLit1 = pDiv->lPairs.pHead->pCube1->lLits.pHead;
	*ppLit2 = pDiv->lPairs.pHead->pCube2->lLits.pHead;
}

Lxu_Var *Lxu_VarIteratorNext(Lxu_Lit **ppLit1, Lxu_Lit **ppLit2)
{
	Lxu_Var *pVar = NULL;

	while (pVar == NULL) {
	
		if ((*ppLit1) && (*ppLit2) )
		{
			if ( (*ppLit1)->iVar == (*ppLit2)->iVar )
			{ // skip the cube free part since nets of the variables
			  // in the base are unaffected
				*ppLit1 = (*ppLit1)->pHNext;
				*ppLit2 = (*ppLit2)->pHNext;
				continue;
			}
			else if ( (*ppLit1)->iVar < (*ppLit2)->iVar )
			{	
				pVar = (*ppLit1)->pVar;
				// move to the next literal in this cube
				*ppLit1 = (*ppLit1)->pHNext;
			}
			else
			{	
				pVar = (*ppLit2)->pVar;
				// move to the next literal in this cube
				*ppLit2 = (*ppLit2)->pHNext;
			}
		}
		else if ( (*ppLit1) && !(*ppLit2) )
		{	
			pVar = (*ppLit1)->pVar;
			// move to the next literal in this cube
			*ppLit1 = (*ppLit1)->pHNext;
		}
		else if ( !(*ppLit1) && (*ppLit2) )
		{	
			pVar = (*ppLit2)->pVar;
			// move to the next literal in this cube
			*ppLit2 = (*ppLit2)->pHNext;
		}
		else
			break;
	}

	return pVar;
}

/* Compute and store the pWeight and optimal position for a 
 * double cube divisor. Does not update heap with new weight.
 */

void LxuEuclidComputePWeightDouble (Lxu_Matrix *p, Lxu_Double *pDiv)
{
	Lxu_Pair *pPair;
	Lxu_Var	*pVar;
	Lxu_Rect nets[LXU_EUCLID_MAX_NETS];	
	Lxu_Rect temp;
	int	nn;				// nn is the number used to index the nets array. 
						// nets[0] is output net. Others are input nets. 
	Lxu_Lit *pLit, *pLit1, *pLit2;
	coord	oldSemiperimeter;
	coord	shrunkSemiperimeter;
	Lxu_EuclidTuple optCostPlacement;
	weightType	cost;

	Lxu_EuclidResetRect (&nets[0]);

	// Compute output net

	Lxu_DoubleForEachPair (pDiv, pPair) {
		assert (pPair->pCube1->pVar == pPair->pCube2->pVar);
		Lxu_EuclidAddToBoundingBox(&nets[0], &(pPair->pCube1->pVar->position));
	}

	shrunkSemiperimeter = Lxu_EuclidComputeSemiperimeter(&nets[0]);
	oldSemiperimeter = 0;		// No output net if divisor not used

	nn = 1;		// 0th is the output net

	Lxu_VarIteratorStart(pDiv, &pLit1, &pLit2);

	while (1) {

		pVar = Lxu_VarIteratorNext (&pLit1, &pLit2);
		if (pVar == NULL)
			break;

		// pVar = variable in the support of the divisor
		
		// Compute bounding box of v assuming it doesn't hit these cubes
		// This is correct even if v continues to go to the node of which 
		// these are cubes, since nothing special should be done in that case
		// (I think :-)
		
		// TODO: Check that we do not do double counting

		Lxu_EuclidResetRect (&nets[nn]);		// Will store shrunken BB
		Lxu_EuclidResetRect (&temp);			// Will store actual BB

		// First we need to add the location where this variable is produced
		// to the bounding box

		Lxu_EuclidAddToBoundingBox(&nets[nn], &pVar->position);
		Lxu_EuclidAddToBoundingBox(&temp, &pVar->position);

		// Now we go through all the cubes where this variable is used
		// except those cubes which are affected by the use of this divisor
		// i.e. the cubes in the [Lxu_Pair] of this divisor

		for (pLit = pVar->lLits.pHead; pLit!=NULL; pLit = pLit->pVNext) {

			// TODO: Make this efficient if needed

			if (!isCubeInPairs(pLit->pCube, pDiv)) {
				Lxu_EuclidAddToBoundingBox(&nets[nn], &(pLit->pCube->pVar->position));
			}

			// Changed in Nov by Sat
			// Lxu_EuclidAddToBoundingBox(&temp, &pVar->position);
			Lxu_EuclidAddToBoundingBox(&temp, &(pLit->pCube->pVar->position));
		}

		// Changed in Nov by Sat
		//oldSemiperimeter += Lxu_EuclidComputeSemiperimeter(&nets[nn]);
		//shrunkSemiperimeter += Lxu_EuclidComputeSemiperimeter(&temp);
		
		shrunkSemiperimeter += Lxu_EuclidComputeSemiperimeter(&nets[nn]);
		oldSemiperimeter += Lxu_EuclidComputeSemiperimeter(&temp);
		
		nn++; 

		/* if the following triggers, increase LXU_EUCLID_MAX_NETS */
		assert (nn < LXU_EUCLID_MAX_NETS);
	}

	// Compute physical weight
	// pweight = reduction in wirelength = oldSemiperimeter - newSemiperimeter

	cost = 0;

	Lxu_EuclidComputeOptimalCostX(nets, nn, &optCostPlacement);
	cost += optCostPlacement.cost;
	pDiv->position.x = optCostPlacement.x;

	Lxu_EuclidComputeOptimalCostY(nets, nn, &optCostPlacement);
	cost += optCostPlacement.cost;
	pDiv->position.y = optCostPlacement.x;

	pDiv->pWeight = oldSemiperimeter - (shrunkSemiperimeter + cost);

}

/* Compute and store the pWeight and optimal position for a 
 * single cube divisor. Does not update heap with new weight.
 */

void LxuEuclidComputePWeightSingle (Lxu_Matrix *p, Lxu_Single *pSingle)
{
	Lxu_Lit *pLit1, *pLit2;
	Lxu_Var *pVar1 = pSingle->pVar1;
	Lxu_Var *pVar2 = pSingle->pVar2;
	Lxu_Rect nets[3];	// Output net and nets for each of the 2 literals
						// nets[0] is output net
	Lxu_Rect temp;
	coord	oldSemiperimeter;
	coord	shrunkSemiperimeter;
	Lxu_EuclidTuple optCostPlacement;
	weightType	cost;

	// Reset the flags

	Lxu_VarForEachLiteral (pVar1, pLit1) {
		pLit1->temp=0;
	}

	Lxu_VarForEachLiteral (pVar2, pLit2) {
		pLit2->temp=0;
	}

	// Now flag the literals which indicate coincidence

	pLit1  = pVar1->lLits.pHead;
	pLit2  = pVar2->lLits.pHead;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->pCube->pVar->iVar == pLit2->pCube->pVar->iVar )
			{ // the variables are the same
				if ( pLit1->iCube == pLit2->iCube )
				{ // the literals are the same

					pLit1->temp = 1;			// Flag the literals 
					pLit2->temp = 1;

					pLit1 = pLit1->pVNext;
					pLit2 = pLit2->pVNext;
				}
				else if ( pLit1->iCube < pLit2->iCube )
					pLit1 = pLit1->pVNext;
				else
					pLit2 = pLit2->pVNext;
			}
			else if ( pLit1->pCube->pVar->iVar < pLit2->pCube->pVar->iVar )
				pLit1 = pLit1->pVNext;
			else
				pLit2 = pLit2->pVNext;
		}
		else if ( pLit1 && !pLit2 )
			pLit1 = pLit1->pVNext;
		else if ( !pLit1 && pLit2 )
			pLit2 = pLit2->pVNext;
		else
			break;
	}

	// Now compute the nets -- only 3 nets here

	Lxu_EuclidResetRect (&nets[0]);		// net for the outputs
	Lxu_EuclidResetRect (&nets[1]);		// net for var1
	Lxu_EuclidResetRect (&nets[2]);		// net for var2

	Lxu_EuclidAddToBoundingBox(&nets[1], &pVar1->position);
	Lxu_EuclidAddToBoundingBox(&nets[2], &pVar2->position);

	// Handle variable 1

	Lxu_EuclidResetRect (&temp);		
	Lxu_EuclidAddToBoundingBox(&temp, &pVar1->position);

	Lxu_VarForEachLiteral (pVar1, pLit1) {
		if (0 == pLit1->temp) {
			// Var1 will have to go here even after division
			Lxu_EuclidAddToBoundingBox(&nets[1], &(pLit1->pCube->pVar->position));
		} else {
			// Var 1 will not have to go here after division. Neither will Var2. But the new net will have to.
			Lxu_EuclidAddToBoundingBox(&nets[0], &(pLit1->pCube->pVar->position));
		}
		Lxu_EuclidAddToBoundingBox(&temp, &(pLit1->pCube->pVar->position));
	}

	oldSemiperimeter = Lxu_EuclidComputeSemiperimeter(&temp);

	// Handle variable 2
	
	Lxu_EuclidResetRect (&temp);		
	Lxu_EuclidAddToBoundingBox(&temp, &pVar2->position);

	Lxu_VarForEachLiteral (pVar2, pLit2) {
		if (0 == pLit2->temp) {
			// Var2 will have to go here even after division
			Lxu_EuclidAddToBoundingBox(&nets[2], &(pLit2->pCube->pVar->position));
		}
		Lxu_EuclidAddToBoundingBox(&temp, &(pLit2->pCube->pVar->position));
	}

	oldSemiperimeter += Lxu_EuclidComputeSemiperimeter(&temp);

	shrunkSemiperimeter = Lxu_EuclidComputeSemiperimeter(&nets[0]) 
							+ Lxu_EuclidComputeSemiperimeter(&nets[1])
							+ Lxu_EuclidComputeSemiperimeter(&nets[2]);

	// Compute physical weight
	// pweight = reduction in wirelength = oldSemiperimeter - newSemiperimeter

	cost = 0;

	Lxu_EuclidComputeOptimalCostX(nets, 3, &optCostPlacement);
	cost += optCostPlacement.cost;
	pSingle->position.x = optCostPlacement.x;

	Lxu_EuclidComputeOptimalCostY(nets, 3, &optCostPlacement);
	cost += optCostPlacement.cost;
	pSingle->position.y = optCostPlacement.x;

	pSingle->pWeight = oldSemiperimeter - (shrunkSemiperimeter + cost);
}

/* Sets up the initial placement and also computes the
 * physical weights of all the divisors
 */

void Lxu_EuclidPlaceAllAndComputeWeights(Lxu_Matrix *p) 
{
	Lxu_Double *pDiv;
	Lxu_Single *pSingle;
	int i;
	int	good = 0, bad = 0, zero = bad;		// Stats

	es.numNodes = 0;		// Keep track of how many guys we actually place; not really used elsewhere

	// Place the nodes
	if (verbose) printf ("Euclid: Placing all nodes %s\n", p->allPlaced ? "" : "for first time");
	
	printf ("Euclid: Total HPWL before placement is %.3f\n", Lxu_ComputeHPWL(p));
	
	//Lxu_EuclidRandomPlacement (p, &es.arena);

	Lxu_EuclidExternalPlacement (p, &es.arena);
	printf ("Euclid: Total HPWL after placement is  %.3f\n", Lxu_ComputeHPWL(p));

	if (verbose) printf ("Euclid: Actually placed %d nodes\n", es.numNodes);

	// Compute weights of the double cube divisors and update heap

	Lxu_HeapDoubleCheck( p->pHeapDouble );
	
	Lxu_MatrixForEachDouble (p, pDiv, i) {

		LxuEuclidComputePWeightDouble (p, pDiv);

		assert (pDiv->pWeight != weightType_NEG_INF);

		if (pDiv->pWeight > 0) 
			good++;
		else if (pDiv->pWeight < 0)
			bad++;
		else 
			zero++;
		
		pDiv->Weight = Lxu_EuclidCombine(pDiv->lWeight, pDiv->pWeight);	

		Lxu_HeapDoubleUpdate( p->pHeapDouble, pDiv ); 
	}

	printf ("Euclid: Double stats: Good: %d\t Bad: %d\t Neutral: %d\n", good, bad, zero);

	good = bad = zero = 0;
	pDiv = 0;					// We don't use this anymore

	Lxu_HeapDoubleCheck( p->pHeapDouble );

	// Compute weights of the single cube divisors and update heap

	Lxu_MatrixForEachSingle (p, pSingle) {

		LxuEuclidComputePWeightSingle (p, pSingle);

		if (pSingle->pWeight > 0) 
			good++;
		else if (pSingle->pWeight < 0)
			bad++;
		else 
			zero++;
	
		pSingle->Weight = Lxu_EuclidCombine(pSingle->lWeight, pSingle->pWeight);	
		Lxu_HeapSingleUpdate( p->pHeapSingle, pSingle ); 
	}

	printf ("Euclid: Single stats: Good: %d\t Bad: %d\t Neutral: %d\n", good, bad, zero);

	Lxu_HeapSingleCheck( p->pHeapSingle );
}

