/**CFile****************************************************************

  FileName    [fxu.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The entrance into the fast extract module.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxu.c,v 1.8 2004/02/14 01:56:26 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h" 
#include "mvc.h"
#include "fxu.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*===== fxuCreate.c ====================================================*/
extern Fxu_Matrix * Fxu_CreateMatrix( Fxu_Data_t * pData );
extern void         Fxu_CreateCovers( Fxu_Matrix * p, Fxu_Data_t * pData );

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
int Fxu_FastExtract( Fxu_Data_t * pData )
{
	Fxu_Matrix * p;
    Fxu_Single * pSingle;
    Fxu_Double * pDouble;
	int Weight1, Weight2, Weight3;

    s_MemoryTotal = 0;
    s_MemoryPeak  = 0;

	// create the matrix
	p = Fxu_CreateMatrix( pData );
    if ( p == NULL )
        return -1;
//    if ( pData->fVerbose )
//        printf( "Memory usage after construction: Total = %d. Peak = %d.\n", s_MemoryTotal, s_MemoryPeak );
//Fxu_MatrixPrint( NULL, p );

    if ( pData->fOnlyS )
    {
        pData->nNodesNew = 0;
	    do
        {
		    Weight1 = Fxu_HeapSingleReadMaxWeight( p->pHeapSingle );
            if ( pData->fVerbose )
                printf( "Best single = %3d.\n", Weight1 );
            if ( Weight1 > 0 || Weight1 == 0 && pData->fUse0 )
			    Fxu_UpdateSingle( p );
            else
                break;
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }
    else if ( pData->fOnlyD )
    {
        pData->nNodesNew = 0;
	    do
        {
		    Weight2 = Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble );
            if ( pData->fVerbose )
                printf( "Best double = %3d.\n", Weight2 );
            if ( Weight2 > 0 || Weight2 == 0 && pData->fUse0 )
			    Fxu_UpdateDouble( p );
            else
                break;
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }
    else if ( !pData->fUseCompl )
    {
        pData->nNodesNew = 0;
	    do
        {
		    Weight1 = Fxu_HeapSingleReadMaxWeight( p->pHeapSingle );
		    Weight2 = Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble );

            if ( pData->fVerbose )
                printf( "Best double = %3d. Best single = %3d.\n", Weight2, Weight1 );
//Fxu_Select( p, &pSingle, &pDouble );

            if ( Weight1 >= Weight2 )
            {
                if ( Weight1 > 0 || Weight1 == 0 && pData->fUse0 )
			        Fxu_UpdateSingle( p );
                else
                    break;
            }
            else
            {
                if ( Weight2 > 0 || Weight2 == 0 && pData->fUse0 )
			        Fxu_UpdateDouble( p );
                else
                    break;
            }
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }
    else
    { // use the complement
        pData->nNodesNew = 0;
	    do
        {
		    Weight1 = Fxu_HeapSingleReadMaxWeight( p->pHeapSingle );
		    Weight2 = Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble );

            // select the best single and double
            Weight3 = Fxu_Select( p, &pSingle, &pDouble );
            if ( pData->fVerbose )
                printf( "Best double = %3d. Best single = %3d. Best complement = %3d.\n", 
                    Weight2, Weight1, Weight3 );

            if ( Weight3 > 0 || Weight3 == 0 && pData->fUse0 )
                Fxu_Update( p, pSingle, pDouble );
            else
                break;
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }

    if ( pData->fVerbose )
        printf( "Total single = %3d. Total double = %3d. Total compl = %3d.\n", p->nDivs1, p->nDivs2, p->nDivs3 );

    // create the new covers
    if ( pData->nNodesNew )
        Fxu_CreateCovers( p, pData );
	Fxu_MatrixDelete( p );
//    printf( "Memory usage after delocation:   Total = %d. Peak = %d.\n", s_MemoryTotal, s_MemoryPeak );
    return pData->nNodesNew;
}


/**Function*************************************************************

  Synopsis    [Unmarks the cubes in the ring.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixRingCubesUnmark( Fxu_Matrix * p )
{
	Fxu_Cube * pCube, * pCube2;
    // unmark the cubes
    Fxu_MatrixForEachCubeInRingSafe( p, pCube, pCube2 )
        pCube->pOrder = NULL;
    Fxu_MatrixRingCubesReset( p );
}


/**Function*************************************************************

  Synopsis    [Unmarks the vars in the ring.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixRingVarsUnmark( Fxu_Matrix * p )
{
	Fxu_Var * pVar, * pVar2;
    // unmark the vars
    Fxu_MatrixForEachVarInRingSafe( p, pVar, pVar2 )
        pVar->pOrder = NULL;
    Fxu_MatrixRingVarsReset( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Fxu_MemFetch( Fxu_Matrix * p, int nBytes )
{
    s_MemoryTotal += nBytes;
    if ( s_MemoryPeak < s_MemoryTotal )
        s_MemoryPeak = s_MemoryTotal;
//    return malloc( nBytes );
    return Extra_MmFixedEntryFetch( p->pMemMan );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MemRecycle( Fxu_Matrix * p, char * pItem, int nBytes )
{
    s_MemoryTotal -= nBytes;
//    free( pItem );
    Extra_MmFixedEntryRecycle( p->pMemMan, pItem );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


