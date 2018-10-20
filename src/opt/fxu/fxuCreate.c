/**CFile****************************************************************

  FileName    [fxuCreate.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Create matrix from covers and covers from matrix.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuCreate.c,v 1.10 2004/10/23 03:20:26 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"
#include "mvc.h"
#include "fxu.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int        Fxu_PreprocessCubePairs( Fxu_Matrix * p, Mvc_Cover_t * ppCovers[], int nCovers, int nPairsTotal, int nPairsMax );

static Fxu_Cube * Fxu_CreateMatrixAddCube( Fxu_Matrix * p, Fxu_Var * pVarNew, Mvc_Cover_t * pMvcCover, Mvc_Cube_t * pMvcCube, int iMvcCube, int * pOrder );
static int        Fxu_CreateMatrixLitCompare( int * ptrX, int * ptrY );
static void       Fxu_CreateCoversNode( Fxu_Matrix * p, Fxu_Data_t * pData, int iNode, Fxu_Cube * pCubeFirst, Fxu_Cube * pCubeNext );
static Fxu_Cube * Fxu_CreateCoversFirstCube( Fxu_Matrix * p, Fxu_Data_t * pData, int iNode );
static int        Fxu_CreateCoversVarCompare( Fxu_Var ** ppV1, Fxu_Var ** ppV2 );

static int * s_pLits;

static int Fxu_CreateSCDs( Fxu_Matrix * p, Fxu_Data_t * pData );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the sparse matrix from the array of Mvc covers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Matrix * Fxu_CreateMatrix( Fxu_Data_t * pData )
{
    Fxu_Matrix * p;
    Fxu_Var * pVar;
    Fxu_Cube * pCubeFirst, * pCubeNew;
    Fxu_Cube * pCube1, * pCube2;
    Mvc_Cover_t * pMvcCover;
    Mvc_Cube_t * pMvcCube;
    int * pOrder, nBitsMax;
    int i, v, iMvcCube;
    int nCubesTotal;
    int nPairsTotal;
    int nPairsStore;
    int nCubes;
    int nCubesMax;
    int iCube, iPair;

    // collect all sorts of statistics
    nCubesTotal = 0;
    nPairsTotal = 0;
    nPairsStore = 0;
    nBitsMax    = -1;
    nCubesMax   = -1;
    for ( i = 0; i < pData->nValuesOld; i++ )
        if ( pMvcCover = pData->ppCovers[i] )
        {
            nCubes       = Mvc_CoverReadCubeNum( pMvcCover );
            nCubesTotal += nCubes;
            nPairsTotal += nCubes * (nCubes - 1) / 2;
            nPairsStore += nCubes * nCubes;
            if ( nBitsMax < pMvcCover->nBits )
                nBitsMax = pMvcCover->nBits;
            if ( nCubesMax < nCubes )
                nCubesMax = nCubes;
        }
//    assert( nBitsMax > 0 );
    if ( nBitsMax <= 0 )
    {
        printf( "The current network does not have SOPs to perform extraction.\n" );
        return NULL;
    }

    if ( nPairsStore > 10000000 )
    {
        printf( "The problem is too large to be solved by \"fxu\" (%d cubes and %d cube pairs)\n", nCubesTotal, nPairsStore );
        return NULL;
    }

    // start the matrix
	p = Fxu_MatrixAllocate();
    p->pValue2Node = pData->pValue2Node;
    p->fMvNetwork  = pData->fMvNetwork;

    // create the column labels 
    p->ppVars = ALLOC( Fxu_Var *, pData->nValuesAlloc );
    for ( i = 0; i < pData->nValuesOld; i++ )
        p->ppVars[i] = Fxu_MatrixAddVar( p );

    // allocate storage for all cube pairs at once
    p->pppPairs = ALLOC( Fxu_Pair **, nCubesTotal + 100 );
    p->ppPairs  = ALLOC( Fxu_Pair *,  nPairsStore + 100 );
    memset( p->ppPairs, 0, sizeof(Fxu_Pair *) * nPairsStore );
    iCube = 0;
    iPair = 0;
    for ( i = 0; i < pData->nValuesOld; i++ )
        if ( pMvcCover = pData->ppCovers[i] )
        {
            // get the number of cubes
            nCubes = Mvc_CoverReadCubeNum( pMvcCover );
            // get the new var in the matrix
            pVar = p->ppVars[i];
            // assign the pair storage
            pVar->nCubes     = nCubes;
            if ( nCubes > 0 )
            {
                pVar->ppPairs    = p->pppPairs + iCube;
                pVar->ppPairs[0] = p->ppPairs  + iPair;
                for ( v = 1; v < nCubes; v++ )
                    pVar->ppPairs[v] = pVar->ppPairs[v-1] + nCubes;
            }
            // update
            iCube += nCubes;
            iPair += nCubes * nCubes;
        }
    assert( iCube == nCubesTotal );
    assert( iPair == nPairsStore );


    // allocate room for the reordered literals
    pOrder = ALLOC( int, nBitsMax );
    // create the rows
    for ( i = 0; i < pData->nValuesOld; i++ )
    {
        // get the corresponding cover
        pMvcCover = pData->ppCovers[i];
        // skip if the cover is not given
        if ( pMvcCover == NULL )
            continue;
        // skip if the cover is empty
        if ( Mvc_CoverReadCubeNum(pMvcCover) == 0 )
            continue;
        assert( pMvcCover->nBits != 0 );

        // get the new var in the matrix
        pVar = p->ppVars[i];

        // here we sort the literals of the cover
        // in the increasing order of the numbers of the corresponding nodes
        // because literals should be added to the matrix in this order
        s_pLits = pMvcCover->pLits;
        for ( v = 0; v < pMvcCover->nBits; v++ )
            pOrder[v] = v;
        qsort( (void *)pOrder, pMvcCover->nBits,
            sizeof(int),(int (*)(const void *, const void *))Fxu_CreateMatrixLitCompare);
        assert( s_pLits[ pOrder[0] ] < s_pLits[ pOrder[pMvcCover->nBits-1] ] );

        // create the corresponding cubes in the matrix
        pCubeFirst = NULL;
        Mvc_CoverForEachCubeWithIndex( pMvcCover, pMvcCube, iMvcCube )
        {
            pCubeNew = Fxu_CreateMatrixAddCube( p, pVar, 
                pMvcCover, pMvcCube, iMvcCube, pOrder );
            if ( pCubeFirst == NULL )
                pCubeFirst = pCubeNew;
            pCubeNew->pFirst = pCubeFirst;
            pCubeNew->iCube  = iMvcCube;
        }
        // set the first cube of this var
        pVar->pFirst = pCubeFirst;
        // create the place to store the cube pairs
//        Fxu_PairAllocStorage( pVar, iMvcCube );

        // create the divisors without preprocessing
        if ( nPairsTotal <= pData->nPairsMax )
        {
		    for ( pCube1 = pCubeFirst; pCube1; pCube1 = pCube1->pNext )
			    for ( pCube2 = pCube1? pCube1->pNext: NULL; pCube2; pCube2 = pCube2->pNext )
				    Fxu_MatrixAddDivisor( p, pCube1, pCube2 );
        }
    }
    FREE( pOrder );

    // consider the case when cube pairs should be preprocessed
    // before adding them to the set of divisors
    if ( nPairsTotal > pData->nPairsMax )
    {
        Fxu_PreprocessCubePairs( p, pData->ppCovers, pData->nValuesOld, 
               nPairsTotal, pData->nPairsMax );
    }

    // add the var pairs to the heap
    Fxu_MatrixComputeSingles( p );

//    Fxu_HeapSinglePrint( stdout, p->pHeapSingle );
//    Fxu_CreateSCDs( p, pData );
//    Fxu_MatrixPrintDivisorProfile( stdout, p );

    // allocate temporary storage for variables
  	p->pVarsTemp  = ALLOC( Fxu_Var *, pData->nValuesAlloc );
    // allocate temporary storage for pairs
//    assert( p->nPairsMax > 0 );
    p->pPairsTemp = ALLOC( Fxu_Pair *, p->nPairsMax * 10 + 100 );


    if ( pData->fVerbose )
    {
        double Density;
        Density = ((double)p->nEntries) / p->lVars.nItems / p->lCubes.nItems;
	    fprintf( stdout, "Matrix: [vars x cubes] = [%d x %d]  ",
            p->lVars.nItems, p->lCubes.nItems );
	    fprintf( stdout, "Lits = %d  Density = %.5f%%\n", 
            p->nEntries, Density );
	    fprintf( stdout, "1-cube divisors = %6d.  ",  p->lSingles.nItems );
	    fprintf( stdout, "2-cube divisors = %6d.  ",  p->nDivsTotal );
	    fprintf( stdout, "Cube pairs = %6d.",    nPairsTotal );
	    fprintf( stdout, "\n" );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Adds one cube with literals to the matrix.]

  Description [Create the cube and literals in the matrix corresponding 
  to the given cube in the Mvc cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Cube * Fxu_CreateMatrixAddCube( Fxu_Matrix * p, Fxu_Var * pVarNew, 
    Mvc_Cover_t * pMvcCover, Mvc_Cube_t * pMvcCube, int iMvcCube, int * pOrder )
{
	Fxu_Cube * pCube;
	Fxu_Var * pVar;
	int i;

	// create the cube
	pCube = Fxu_MatrixAddCube( p, pVarNew, iMvcCube );
    // add literals to the matrix
    for ( i = 0; i < pMvcCover->nBits; i++ )
    {
        // co-singleton transform is done here!
        if ( !Mvc_CubeBitValue( pMvcCube, pOrder[i] ) )
        {
            pVar = p->ppVars[ pMvcCover->pLits[pOrder[i]] ];
		    Fxu_MatrixAddLiteral( p, pCube, pVar );
        }
    }
	return pCube;
}


/**Function*************************************************************

  Synopsis    [Creates the new array of Mvc covers from the sparse matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_CreateCovers( Fxu_Matrix * p, Fxu_Data_t * pData )
{
    Fxu_Cube * pCube, * pCubeFirst, * pCubeNext;
    int iNode, iValue, iValueFirst, nValues;
    int n, v;

    // add the node/value data for the new columns and rows
    for ( n = 0; n < pData->nNodesNew; n++ )
    {
        // get the number of this new node in the node lits
        iNode  = pData->nNodesOld + n;
        // get the first value in the value list
        iValue = pData->pNode2Value[iNode];
        // set the value to node for the values of the new node
        pData->pValue2Node[iValue  ] = iNode;
        pData->pValue2Node[iValue+1] = iNode;
        // set the node to value for the next node
        pData->pNode2Value[iNode+1]  = iValue + 2;
    }

    // write NULL for all CI values
    for ( v = 0; v < pData->nValuesCi; v++ )
        pData->ppCoversNew[v] = NULL;

    // get the first cube of the first internal node 
    pCubeFirst = Fxu_CreateCoversFirstCube( p, pData, pData->nNodesCi );

    // go through the internal nodes
    for ( n = 0; n < pData->nNodesInt; n++ )
    {
        // get the number of this node
        iNode = pData->nNodesCi + n;
        // get the next first cube
        pCubeNext = Fxu_CreateCoversFirstCube(  p, pData, iNode + 1 );

        // check if there any new variables in these cubes
        for ( pCube = pCubeFirst; pCube != pCubeNext; pCube = pCube->pNext )
            if ( pCube->lLits.pTail && pCube->lLits.pTail->iVar >= pData->nValuesOld )
                break;
        if ( pCube == pCubeNext )
        {  // there is no change in this node
            iValueFirst = pData->pNode2Value[iNode];
            nValues     = pData->pNode2Value[iNode + 1] - pData->pNode2Value[iNode];
            for ( v = 0; v < nValues; v++ )
                pData->ppCoversNew[iValueFirst + v] = NULL;
        }
        else
        {   // the node has changed
            Fxu_CreateCoversNode( p, pData, iNode, pCubeFirst, pCubeNext );
        }
        // update the first cube
        pCubeFirst = pCubeNext;
    }

    // add the covers for the extracted nodes
    for ( n = 0; n < pData->nNodesNew; n++ )
    {
        // get the number of this node
        iNode = pData->nNodesOld + n;
        // get the next first cube
        if ( n == pData->nNodesNew - 1 )
            pCubeNext = NULL;
        else
            pCubeNext = Fxu_CreateCoversFirstCube(  p, pData, iNode + 1 );
        // the node should be added
        Fxu_CreateCoversNode( p, pData, iNode, pCubeFirst, pCubeNext );
        // update the first cube
        pCubeFirst = pCubeNext;
    }
}

/**Function*************************************************************

  Synopsis    [Create Mvc covers for one node that has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_CreateCoversNode( Fxu_Matrix * p, Fxu_Data_t * pData, int iNode, Fxu_Cube * pCubeFirst, Fxu_Cube * pCubeNext )
{
    Mvc_Cover_t * pMvcCover;
    Mvc_Cube_t * pMvcCube;
    Fxu_Var * pVar;
    Fxu_Cube * pCube;
    Fxu_Lit * pLit;
    int iNum, iNumC, iNodeVar, iValue, iValueFirst;
    int nValues, c, v;
    bool fPosLitIsOdd;


    // mark and collect all the variables involved with these cubes 
    // between pCubeFirst and pCubeNext
    Fxu_MatrixRingVarsStart( p );
    for ( pCube = pCubeFirst; pCube != pCubeNext; pCube = pCube->pNext )
        for ( pLit = pCube->lLits.pHead; pLit; pLit = pLit->pHNext )
        {
            pVar = pLit->pVar;
            if ( pVar->pOrder == NULL )
            {
                // add this var
                Fxu_MatrixRingVarsAdd( p, pVar );

                // add the related vars
                // get the corresponding node
                iNodeVar = pData->pValue2Node[pVar->iVar];
                // get the number of values of the corresponding node
                iValueFirst = pData->pNode2Value[iNodeVar];
                nValues     = pData->pNode2Value[iNodeVar+1] - pData->pNode2Value[iNodeVar];
                for ( c = 0; c < nValues; c++ )
                {
                    pVar = p->ppVars[iValueFirst + c];
                    if ( pVar->pOrder == NULL )
                        Fxu_MatrixRingVarsAdd( p, pVar );
                }
            }
        }
    Fxu_MatrixRingVarsStop( p );

    // transfer the vars from the list to the array
    p->nVarsTemp = 0;
    Fxu_MatrixForEachVarInRing( p, pVar )
        p->pVarsTemp[ p->nVarsTemp++ ] = pVar;
    Fxu_MatrixRingVarsUnmark( p );

    // sort the variables by their numbers
    qsort( (void *)p->pVarsTemp, p->nVarsTemp, sizeof(Fxu_Var *), 
        (int (*)(const void *, const void *)) Fxu_CreateCoversVarCompare );
    assert( Fxu_CreateCoversVarCompare( p->pVarsTemp, p->pVarsTemp + p->nVarsTemp - 1 ) < 0 );

    // mark the vars with their numbers
    for ( v = 0; v < p->nVarsTemp; v++ )
        p->pVarsTemp[v]->lLits.nItems = v; // hack - reuse lLits.nItems

    // create the covers for this node
    iValueFirst = pData->pNode2Value[iNode];
    nValues     = pData->pNode2Value[iNode+1] - pData->pNode2Value[iNode];

    // get the starting cube of the first cover
    pCube = pCubeFirst;
    for ( v = 0; v < nValues; v++ )
    {
        // get the current value
        iValue = iValueFirst + v;
        // get the variable
        pVar = p->ppVars[iValue];

        // consider default values and constant values
        if ( pVar->pFirst == NULL )
        {
             if ( iValue < pData->nValuesOld && pData->ppCovers[iValue] )
             { // this is the old cover, and it is present
                 // this should be a constant cover
                 assert( Mvc_CoverIsEmpty(pData->ppCovers[iValue]) || 
                         Mvc_CoverIsTautology(pData->ppCovers[iValue]) );
                 // copy the cover (but with the different number of bits
                 pData->ppCoversNew[iValue] = Mvc_CoverAlloc( pData->pManMvc, p->nVarsTemp );
                 // if the original cover is a tau, make this cover a tau too
                 if ( Mvc_CoverIsTautology(pData->ppCovers[iValue]) )
                     Mvc_CoverMakeTautology(pData->ppCoversNew[iValue]);
             }
             else // set the empty cover
                 pData->ppCoversNew[iValue] = NULL;
            continue;
        }

        // start the new Mvc cover
        pMvcCover = Mvc_CoverAlloc( pData->pManMvc, p->nVarsTemp );
        // set the literals according to the selected vars
        Mvc_CoverAllocateArrayLits( pMvcCover );
        for ( c = 0; c < p->nVarsTemp; c++ )
            pMvcCover->pLits[c] = p->pVarsTemp[c]->iVar;

        // determine the polarity of the new literals
        fPosLitIsOdd = -1;
        for ( c = 0; c < p->nVarsTemp; c++ )
            if ( pMvcCover->pLits[c] >= pData->nValuesOld )
            {
                fPosLitIsOdd = ((c & 1) == 0);
                break;
            }

        // add new Mvc cubes to the cover
        for ( c = 0; c < pVar->nCubes; c++ )
        {
            if ( pCube->lLits.nItems )
            { // the cube is not empty
                // create the new Mvc cube
                pMvcCube = Mvc_CubeAlloc( pMvcCover );
                Mvc_CubeBitClean( pMvcCube );
                // insert literals
                for ( pLit = pCube->lLits.pHead; pLit; pLit = pLit->pHNext )
                {
                    iNum = pLit->pVar->lLits.nItems;
                    assert( iNum < p->nVarsTemp );

                    if ( pLit->pVar->iVar < pData->nValuesOld )
                        Mvc_CubeBitInsert( pMvcCube, iNum );
                    else
                    {
//                        Mvc_CubeBitInsert( pMvcCube, iNum - 1 );
                        assert( fPosLitIsOdd != -1 );
                        if ( fPosLitIsOdd )
                        {
                            if ( iNum & 1 )
                                iNumC = iNum - 1;
                            else
                                iNumC = iNum + 1;
                        }
                        else
                        {
                            if ( iNum & 1 )
                                iNumC = iNum + 1;
                            else
                                iNumC = iNum - 1;
                        }
                        Mvc_CubeBitInsert( pMvcCube, iNumC );
                    }
                }
                // here we perform the inverse co-singleton transform
                Mvc_CubeBitNot( pMvcCube );
                // add the cube
                Mvc_CoverAddCubeTail( pMvcCover, pMvcCube );
            }
            // scroll to the new cube
            pCube = pCube->pNext;
        }
        // set the new Mvc cover
        pData->ppCoversNew[iValue] = pMvcCover;
    }
    assert( pCube == pCubeNext );
}


/**Function*************************************************************

  Synopsis    [Adds the var to storage.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Cube * Fxu_CreateCoversFirstCube( Fxu_Matrix * p, Fxu_Data_t * pData, int iNode )
{
    Fxu_Var * pVar;
    int v, iValueFirst, nValues;

    iValueFirst = pData->pNode2Value[iNode];
    nValues     = pData->pNode2Value[iNode + 1] - pData->pNode2Value[iNode];
    // go through variables until the one with the cube
    for ( v = 0; v < nValues; v++ )
    {
        pVar = p->ppVars[iValueFirst + v];
        if ( pVar->pFirst )
            return pVar->pFirst;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Compares the vars by their number.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_CreateCoversVarCompare( Fxu_Var ** ppV1, Fxu_Var ** ppV2 )
{
    int iVar1 = (*ppV1)->iVar;
    int iVar2 = (*ppV2)->iVar;
    if ( iVar1 < iVar2 )
        return -1;
    if ( iVar1 > iVar2 )
        return 1;
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares the vars by their number.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_CreateMatrixLitCompare( int * ptrX, int * ptrY )
{
    return s_pLits[*ptrX] - s_pLits[*ptrY];
} 

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_CreateSCDs( Fxu_Matrix * p, Fxu_Data_t * pData )
{
    Fxu_Var * pVar1;
    Fxu_Var * pVar2, * pVarTemp;
    Fxu_Lit * pLitV, * pLitH;
    int Coin;
    int CounterAll;
    int CounterTest;
    int CounterImp2;
    int CounterImp3;
    int clk;

    CounterAll = 0;
    CounterTest = 0;
    CounterImp2 = 0;
    CounterImp3 = 0;

clk = clock();
    // iterate through the columns in the matrix
    Fxu_MatrixForEachVariable( p, pVar1 )
    {
        // start collecting the affected vars
        Fxu_MatrixRingVarsStart( p );
        // go through all the literals of this variable
        for ( pLitV = pVar1->lLits.pHead; pLitV; pLitV = pLitV->pVNext )
        {
            // for this literal, go through all the horizontal literals
            for ( pLitH = pLitV->pHNext; pLitH; pLitH = pLitH->pHNext )
            {
                // get another variable
                pVar2 = pLitH->pVar;
                CounterAll++;
                // skip the var if it is already used
                if ( pVar2->pOrder )
                    continue;
                // skip the var if it belongs to the same node
                if ( pData->pValue2Node[pVar1->iVar] == pData->pValue2Node[pVar2->iVar] )
                    continue;
                // collect the var
                Fxu_MatrixRingVarsAdd( p, pVar2 );
            }
        }
        // stop collecting the affected vars
        Fxu_MatrixRingVarsStop( p );

        // iterate through the vars
        Fxu_MatrixForEachVarInRing( p, pVar2 )
        {
            CounterTest++;
            // count the coincidence
            Coin = Fxu_SingleCountCoincidence( p, pVar1, pVar2 );
            assert( Coin > 0 );

            if ( Coin > 1 )
                CounterImp2++;
            if ( Coin > 2 )
            {
//                printf( "pair = (%2d, %2d)\n", pVar1->iVar, pVar2->iVar );
                CounterImp3++;
            }
        }
        // unmark the vars
        Fxu_MatrixForEachVarInRingSafe( p, pVar2, pVarTemp )
            pVar2->pOrder = NULL;

        Fxu_MatrixRingVarsReset( p );

    }
    printf( "Total = %5d. Tested = %5d. 2+ = %d. 3+ = %d.\n", 
        CounterAll, CounterTest, CounterImp2, CounterImp3 );

PRT( "Time", clock() - clk );

return 1;
}

