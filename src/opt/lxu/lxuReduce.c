/**CFile****************************************************************

  FileName    [lxuReduce.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to reduce the number of considered cube pairs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuReduce.c,v 1.2 2004/01/30 00:18:51 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"
#include "mvc.h"
#include "lxu.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Precomputes the pairs to use for creating two-cube divisors.]

  Description [This procedure takes the matrix with variables and cubes 
  allocated (p), the original covers of the nodes (i-sets) and their number 
  (ppCovers,nCovers). The maximum number of pairs to compute and the total 
  number of pairs in existence. This procedure adds to the storage of
  divisors exactly the given number of pairs (nPairsMax) while taking
  first those pairs that have the smallest number of literals in their
  cube-free form.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lxu_PreprocessCubePairs( Lxu_Matrix * p, Mvc_Cover_t * ppCovers[], 
     int nCovers, int nPairsTotal, int nPairsMax )
{
    unsigned char * pnLitsDiff;   // storage for the counters of diff literals
    int * pnPairCounters;         // the counters of pairs for each number of diff literals
    Lxu_Cube * pCubeFirst, * pCubeLast;
    Lxu_Cube * pCube1, * pCube2;
    Lxu_Var * pVar;
    int nCubes, nBitsMax, nSum;
    int CutOffNum, CutOffQuant;
    int iPair, iQuant, k, c;
    int clk = clock();

    assert( nPairsMax < nPairsTotal );

    // allocate storage for counter of diffs
    pnLitsDiff = ALLOC( unsigned char, nPairsTotal );
    // go through the covers and precompute the distances between the pairs
    iPair    =  0;
    nBitsMax = -1;
    for ( c = 0; c < nCovers; c++ )
        if ( ppCovers[c] )
        {
            // precompute the differences
            Mvc_CoverCountCubePairDiffs( ppCovers[c], pnLitsDiff + iPair );
            // update the counter
            nCubes = Mvc_CoverReadCubeNum( ppCovers[c] );
            iPair += nCubes * (nCubes - 1) / 2;
            // get the max number of bits
            if ( nBitsMax < ppCovers[c]->nBits )
                nBitsMax = ppCovers[c]->nBits;
        }
    assert( iPair == nPairsTotal );

    // allocate storage for counters of cube pairs by difference
    pnPairCounters = ALLOC( int, 2 * nBitsMax );
    memset( pnPairCounters, 0, sizeof(int) * 2 * nBitsMax );
    // count the number of different pairs
    for ( k = 0; k < nPairsTotal; k++ )
        pnPairCounters[ pnLitsDiff[k] ]++;
    // determine what pairs to take starting from the lower
    // so that there would be exactly pPairsMax pairs
    assert( pnPairCounters[0] == 0 ); // otherwise, covers are not dup-free
    assert( pnPairCounters[1] == 0 ); // otherwise, covers are not SCC-free
    nSum = 0;
    for ( k = 0; k < 2 * nBitsMax; k++ )
    {
        nSum += pnPairCounters[k];
        if ( nSum >= nPairsMax )
        {
            CutOffNum   = k;
            CutOffQuant = pnPairCounters[k] - (nSum - nPairsMax);
            break;
        }
    }
    FREE( pnPairCounters );

    // set to 0 all the pairs above the cut-off number and quantity
    iQuant = 0;
    iPair  = 0;
    for ( k = 0; k < nPairsTotal; k++ )
        if ( pnLitsDiff[k] > CutOffNum ) 
            pnLitsDiff[k] = 0;
        else if ( pnLitsDiff[k] == CutOffNum )
        {
            if ( iQuant++ >= CutOffQuant )
                pnLitsDiff[k] = 0;
            else
                iPair++;
        }
        else
            iPair++;
    assert( iPair == nPairsMax );

    // collect the corresponding pairs and add the divisors
    iPair = 0;
    for ( c = 0; c < nCovers; c++ )
        if ( ppCovers[c] )
        {
            // get the new var in the matrix
            pVar = p->ppVars[c];

            // get the first cube
            pCubeFirst = pVar->pFirst;
            // get the last cube
            pCubeLast = pCubeFirst;
            for ( k = 0; k < pVar->nCubes; k++ )
                pCubeLast = pCubeLast->pNext;
            assert( pCubeLast == NULL || pCubeLast->pVar != pVar );

            // go through the cube pairs
		    for ( pCube1 = pCubeFirst; pCube1 != pCubeLast; pCube1 = pCube1->pNext )
			    for ( pCube2 = pCube1->pNext; pCube2 != pCubeLast; pCube2 = pCube2->pNext )
                    if ( pnLitsDiff[iPair++] )
                    {   // create the divisors for this pair
	                    Lxu_MatrixAddDivisor( p, pCube1, pCube2 );
                    }
        }
    assert( iPair == nPairsTotal );
    FREE( pnLitsDiff );
PRT( "Preprocess", clock() - clk );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


