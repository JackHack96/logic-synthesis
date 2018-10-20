/**CFile****************************************************************

  FileName    [lxu.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The entrance into the fast extract module.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxu.c,v 1.9 2004/03/10 03:23:20 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h" 
#include "mvc.h"
#include "lxu.h"

#include "time.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*===== lxuCreate.c ====================================================*/
extern Lxu_Matrix * Lxu_CreateMatrix( Lxu_Data_t * pData );
extern void         Lxu_CreateCovers( Lxu_Matrix * p, Lxu_Data_t * pData );

/* Forward decl */

Lxu_Single *Lxu_SelectRandomSingle ( Lxu_Matrix * p );
Lxu_Double *Lxu_SelectRandomDouble ( Lxu_Matrix * p );

Lxu_Single *Lxu_SelectRandomSingleUnweighted ( Lxu_Matrix * p );
Lxu_Double *Lxu_SelectRandomDoubleUnweighted ( Lxu_Matrix * p );

static int s_MemoryTotal;
static int s_MemoryPeak;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs fast_extract on a set of covers.]

  Description [All the covers are given in the array p->ppCovers. 
  The resulting covers are returned in the array p->ppCoversNew.
  The number of entries in both cover arrays is equal to the number
  of all values in the current nodes plus the max expected number 
  of extracted nodes (p->nValuesCi + p->nValuesInt + p->nValuesExtMax). 
  The first p->nValuesCi entries, corresponding to the CI nodes, are NULL.
  The next p->nValuesInt entries, corresponding to the int nodes, are covers
  from which the divisors are extracted. The last p->nValuesExtMax entries  
  are reserved for the new covers to be extracted. The number of extracted 
  covers is returned. This number does not exceed the given number (p->nNodesExt). 
  Two other things are important for the correct operation of this procedure:
  (1) The input covers should be SCC-free. (2) The literal array (pCover->pLits) 
  is allocated in each cover. The i-th entry in the literal array of a cover
  is the number of the cover in the array p->ppCovers, which represents this
  literal (variable value) in the given cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lxu_FastExtract( Lxu_Data_t * pData )
{
	Lxu_Matrix * p;
	weightType Weight1, Weight2;
	unsigned seed = (unsigned) time(NULL);
	unsigned start_time = (unsigned) time(NULL);

	FILE *hpf = fopen("hp.log", "w");
	assert (hpf != NULL);

    s_MemoryTotal = 0;
    s_MemoryPeak  = 0;

	printf ("LXU: seed is %u\n", seed);
	srand(seed);

	assert ((pData->fMvNetwork == 0) && "Lxu works only with binary networks");

	// Initialize geometry engine
	Lxu_EuclidStart(pData->alpha, pData->beta, pData->interval);

	// create the matrix
	p = Lxu_CreateMatrix( pData );

	// initial placement and computing of physical weights of divisors
	if (pData->beta != 0) {
		Lxu_EuclidPlaceAllAndComputeWeights(p);
	}

	p->allPlaced = 1;

	printf ("LXU: After initial placement and weight computation (%lu)\n", (time(NULL) - start_time)); 

	Lxu_HeapDoubleCheck( p->pHeapDouble );

	/******************************************************
	 *    Random extractions
	 *

	if (1) 
	{

		FILE *log = fopen("weights.log", "w");
		
		do 
		{
			weightType ws, wd;

			Lxu_Single *s = Lxu_SelectRandomSingleUnweighted ( p );
			Lxu_Double *d = Lxu_SelectRandomDoubleUnweighted ( p );

			// Lxu_Single *s = Lxu_SelectRandomSingle ( p );
			// Lxu_Double *d = Lxu_SelectRandomDouble ( p );

			ws = (s != 0) ? s->Weight : -1;
			wd = (d != 0) ? d->Weight : -1;

			if ((ws < 0) && (wd < 0))
				break;

			fprintf (log, "%g\t\t%s\n", ws > wd ? ws : wd, ws > wd ? "single" : "double");

			if (ws > wd) 
			{
				Lxu_UpdateSingle( p, s );
			}	
			else
			{
				Lxu_UpdateDouble( p, d );
			}

		} while ( ++pData->nNodesNew < pData->nNodesExt );


		fclose(log);
	}
	else 

	******************************************************/
	
	if ( !pData->fUseCompl )
    {
		int c = 0;
        pData->nNodesNew = 0;
	    do
        {
			fprintf (hpf, "%lu\t\t ", time(NULL) - start_time);

		    Weight1 = Lxu_HeapSingleReadMaxWeight( p->pHeapSingle );
		    Weight2 = Lxu_HeapDoubleReadMaxWeight( p->pHeapDouble );

            if ( pData->fVerbose )
                printf( "Best double = %g. Best single = %g.\n", Weight2, Weight1 );

            if ( Weight1 >= Weight2 )
            {
				Lxu_Single *s = Lxu_HeapSingleReadMax( p->pHeapSingle );

                if ( (Weight1 > 0.001) || (Weight1 == 0.001 && pData->fUse0) ) {
					fprintf (hpf, "%d\tsingle \t%.3f ", pData->nNodesNew, Weight1);
			        Lxu_UpdateSingle( p, s );
				} else
                    break;
            }
            else
            {
				Lxu_Double *d = Lxu_HeapDoubleGetMax( p->pHeapDouble );

                if ( (Weight2 > 0.001) || (Weight2 == 0.001 && pData->fUse0) ) {
					fprintf (hpf, "%d\tdouble \t%.3f ", pData->nNodesNew, Weight2);
			        Lxu_UpdateDouble( p, d );
				} else
                    break;
            }

			// fprintf (hpf, "\t %.3f ", Lxu_ComputeHPWL(p));
			fprintf (hpf, "\n");

			if ((pData->beta != 0) && (++c == pData->interval)) 
			{
				Lxu_EuclidPlaceAllAndComputeWeights(p);
				c = 0;
				fprintf (hpf, "## Replacing\t\t %.3f\n", Lxu_ComputeHPWL(p));
				fflush(hpf);
			}
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }
    else
    { 
		int c = 0;
		Lxu_Single * pSingle;
		Lxu_Double * pDouble;
		weightType Weight3;

		assert (0 && "Complements not handled properly");

        pData->nNodesNew = 0;
	    do
        {
		    Weight1 = Lxu_HeapSingleReadMaxWeight( p->pHeapSingle );
		    Weight2 = Lxu_HeapDoubleReadMaxWeight( p->pHeapDouble );

			Lxu_HeapDoubleCheck( p->pHeapDouble );

            // select the best single and double
            Weight3 = Lxu_Select( p, &pSingle, &pDouble );
            if ( pData->fVerbose )
                printf( "Best double = %g. Best single = %g. Best complement = %g.\n", 
                    Weight2, Weight1, Weight3 );

            if ( (Weight3 > 0) || (Weight3 == 0 && pData->fUse0) )
                Lxu_Update( p, pSingle, pDouble );
            else
                break;

			Lxu_HeapDoubleCheck( p->pHeapDouble );

			if ((pData->beta != 0) && (++c == pData->interval))
			{
				Lxu_EuclidPlaceAllAndComputeWeights(p);
				c = 0;
			}

        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }

	/*
	
	if (pData->beta != 0) {
		printf ("Euclid: Last GASP\n");
		Lxu_EuclidPlaceAllAndComputeWeights(p);
	}

	*/

	fprintf (hpf, "\n");
	fclose (hpf);
	
	// Done with geometry engine
	Lxu_EuclidStop();

    if ( pData->fVerbose )
        printf( "Total single = %3d. Total double = %3d. Total compl = %3d.\n", p->nDivs1, p->nDivs2, p->nDivs3 );

    // create the new covers
    if ( pData->nNodesNew )
        Lxu_CreateCovers( p, pData );
	Lxu_MatrixDelete( p );
//    printf( "Memory usage after delocation:   Total = %d. Peak = %d.\n", s_MemoryTotal, s_MemoryPeak );
    return pData->nNodesNew;
}


/**Function*************************************************************

  Synopsis    [Unmarks the cubes in the ring.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixRingCubesUnmark( Lxu_Matrix * p )
{
	Lxu_Cube * pCube, * pCube2;
    // unmark the cubes
    Lxu_MatrixForEachCubeInRingSafe( p, pCube, pCube2 )
        pCube->pOrder = NULL;
    Lxu_MatrixRingCubesReset( p );
}


/**Function*************************************************************

  Synopsis    [Unmarks the vars in the ring.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixRingVarsUnmark( Lxu_Matrix * p )
{
	Lxu_Var * pVar, * pVar2;
    // unmark the vars
    Lxu_MatrixForEachVarInRingSafe( p, pVar, pVar2 )
        pVar->pOrder = NULL;
    Lxu_MatrixRingVarsReset( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Lxu_MemFetch( Lxu_Matrix * p, int nBytes )
{
    s_MemoryTotal += nBytes;
    if ( s_MemoryPeak < s_MemoryTotal )
        s_MemoryPeak = s_MemoryTotal;
    return Extra_MmFixedEntryFetch( p->pMemMan );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MemRecycle( Lxu_Matrix * p, char * pItem, int nBytes )
{
    s_MemoryTotal -= nBytes;
    Extra_MmFixedEntryRecycle( p->pMemMan, pItem );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Lxu_Single *Lxu_SelectRandomSingle ( Lxu_Matrix * p )
{
	Lxu_Single *s = 0;
	weightType sum = 0, chosen;
   
	for (s = p->lSingles.pHead; s != 0; s = s->pNext)
	{
		if (s->Weight > 0.0)
			sum += s->Weight;
	}

//	printf ("Single: Total sum is %g\n", sum);

	chosen = sum * (1.0 * rand() / RAND_MAX);

//	printf ("Single: Chosen is %g\n", chosen);

	for (s = p->lSingles.pHead; s != 0; s = s->pNext)
	{
		if (s->Weight > 0.0)
		{
			if ((chosen - s->Weight) < 0.0)
				break;

			chosen -= s->Weight;
		}
	}	

	return s;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Lxu_Double *Lxu_SelectRandomDouble ( Lxu_Matrix * p )
{
	Lxu_Double *s = 0;
	weightType sum = 0, chosen;
	int i;
   
	for ( i = 0; i < p->nTableSize; i++ )
	{
		for ( s = p->pTable[i].pHead; s != 0; s = s->pNext )
		{
			if (s->Weight > 0.0)
				sum += s->Weight;
		}
	}

//	printf ("Double: Total sum is %g\n", sum);

	chosen = sum * (1.0 * rand() / RAND_MAX);

//	printf ("Double: Chosen is %g\n", chosen);

	for ( i = 0; i < p->nTableSize; i++ )
	{
		for ( s = p->pTable[i].pHead; s != 0; s = s->pNext )
		{
			if (s->Weight > 0.0)
			{
				if ((chosen - s->Weight) < 0.0)
					goto done;

				chosen -= s->Weight;
			}
		}
	}

done:

	return s;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Lxu_Single *Lxu_SelectRandomSingleUnweighted ( Lxu_Matrix * p )
{
	Lxu_Single *s = 0;
	int chosen, i;

	if ( p->lSingles.nItems == 0 )
		return 0;

	chosen = rand() % p->lSingles.nItems;
	i = 0;
	
	for (s = p->lSingles.pHead; s != 0; s = s->pNext)
	{
		if ( i == chosen )
			break;
		i++;
	}

	return s;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Lxu_Double *Lxu_SelectRandomDoubleUnweighted ( Lxu_Matrix * p )
{
	Lxu_Double *s = 0;
	int i, total, chosen, j;

	total = 0;
   
	for ( i = 0; i < p->nTableSize; i++ )
	{
		for ( s = p->pTable[i].pHead; s != 0; s = s->pNext )
		{
			total += p->pTable[i].nItems;
		}
	}

	if ( total == 0 )
		return 0;

	chosen = rand() % total;
	j = 0;

	for ( i = 0; i < p->nTableSize; i++ )
	{
		if ( j + p->pTable[i].nItems > chosen )
		{
			for ( s = p->pTable[i].pHead; s != 0; s = s->pNext )
			{
				if (j == chosen)
					goto done;
				++j;
			}
		}
		else
		{
			j += p->pTable[i].nItems;
		}
	}

done:

	return s;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


