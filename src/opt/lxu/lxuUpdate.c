/**CFile****************************************************************

  FileName    [lxuUpdate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Updating the sparse matrix when divisors are accepted.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuUpdate.c,v 1.4 2004/02/18 01:16:48 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Lxu_UpdateDoublePairs( Lxu_Matrix * p, Lxu_Double * pDouble, Lxu_Var * pVar );
static void Lxu_UpdateMatrixDoubleCreateCubes( Lxu_Matrix * p, Lxu_Cube * pCube1, Lxu_Cube * pCube2, Lxu_Double * pDiv );
static void Lxu_UpdateMatrixDoubleClean( Lxu_Matrix * p, Lxu_Cube * pCubeUse, Lxu_Cube * pCubeRem );
static void Lxu_UpdateMatrixSingleClean( Lxu_Matrix * p, Lxu_Var * pVar1, Lxu_Var * pVar2, Lxu_Var * pVarNew );

static void Lxu_UpdateCreateNewVars( Lxu_Matrix * p, Lxu_Var ** ppVarC, Lxu_Var ** ppVarD, int nCubes );
static int  Lxu_UpdatePairCompare( Lxu_Pair ** ppP1, Lxu_Pair ** ppP2 );
static void Lxu_UpdatePairsSort( Lxu_Matrix * p, Lxu_Double * pDouble );

static void Lxu_UpdateCleanOldDoubles( Lxu_Matrix * p, Lxu_Double * pDiv, Lxu_Cube * pCube );
static void Lxu_UpdateAddNewDoubles( Lxu_Matrix * p, Lxu_Cube * pCube );
static void Lxu_UpdateCleanOldSingles( Lxu_Matrix * p );
static void Lxu_UpdateAddNewSingles( Lxu_Matrix * p, Lxu_Var * pVar );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Updates the matrix after selecting two divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_Update( Lxu_Matrix * p, Lxu_Single * pSingle, Lxu_Double * pDouble )
{
	Lxu_Cube * pCube, * pCubeNew;
	Lxu_Var * pVarC, * pVarD;
	Lxu_Var * pVar1, * pVar2;

    // consider trivial cases
    if ( pSingle == NULL )
    {
        assert( pDouble->Weight == Lxu_HeapDoubleReadMaxWeight( p->pHeapDouble ) );
        Lxu_UpdateDouble( p, pDouble );
        return;
    }
    if ( pDouble == NULL )
    {
        assert( pSingle->Weight == Lxu_HeapSingleReadMaxWeight( p->pHeapSingle ) );
        Lxu_UpdateSingle( p, pSingle );
        return;
    }

	printf ("o");

    // get the variables of the single
    pVar1 = pSingle->pVar1;
    pVar2 = pSingle->pVar2;

    // remove the best double from the heap
    Lxu_HeapDoubleDelete( p->pHeapDouble, pDouble );
	// remove the best divisor from the table
	Lxu_ListTableDelDivisor( p, pDouble );

    // create two new columns (vars)
    Lxu_UpdateCreateNewVars( p, &pVarC, &pVarD, 1 );
    // create one new row (cube)
    pCubeNew = Lxu_MatrixAddCube( p, pVarD, 0 );
    pCubeNew->pFirst = pCubeNew;
    // set the first cube of the positive var
    pVarD->pFirst = pCubeNew;

    // start collecting the affected vars and cubes
    Lxu_MatrixRingCubesStart( p );
    Lxu_MatrixRingVarsStart( p );
    // add the vars
    Lxu_MatrixRingVarsAdd( p, pVar1 );
    Lxu_MatrixRingVarsAdd( p, pVar2 );
    // remove the literals and collect the affected cubes
    // remove the divisors associated with this cube
   	// add to the affected cube the literal corresponding to the new column
	Lxu_UpdateMatrixSingleClean( p, pVar1, pVar2, pVarD );
	// replace each two cubes of the pair by one new cube
	// the new cube contains the base and the new literal
    Lxu_UpdateDoublePairs( p, pDouble, pVarC );
    // stop collecting the affected vars and cubes
    Lxu_MatrixRingCubesStop( p );
    Lxu_MatrixRingVarsStop( p );

    // add the literals to the new cube
    assert( pVar1->iVar < pVar2->iVar );
    assert( Lxu_SingleCountCoincidence( p, pVar1, pVar2 ) == 0 );
    Lxu_MatrixAddLiteral( p, pCubeNew, pVar1 );
    Lxu_MatrixAddLiteral( p, pCubeNew, pVar2 );

    // create new doubles; we cannot add them in the same loop
	// because we first have to create *all* new cubes for each node
    Lxu_MatrixForEachCubeInRing( p, pCube )
   		Lxu_UpdateAddNewDoubles( p, pCube );
    // update the singles after removing some literals
    Lxu_UpdateCleanOldSingles( p );

    // undo the temporary rings with cubes and vars
    Lxu_MatrixRingCubesUnmark( p );
    Lxu_MatrixRingVarsUnmark( p );
    // we should undo the rings before creating new singles

    // create new singles
    Lxu_UpdateAddNewSingles( p, pVarC );
    Lxu_UpdateAddNewSingles( p, pVarD );

	// recycle the divisor
	MEM_FREE_LXU( p, Lxu_Double, 1, pDouble );
	p->nDivs3++;
}

/**Function*************************************************************

  Synopsis    [Updates after accepting single cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateSingle( Lxu_Matrix * p, Lxu_Single * pSingle )
{
	Lxu_Cube * pCube, * pCubeNew;
	Lxu_Var * pVarC, * pVarD;
	Lxu_Var * pVar1, * pVar2;

	// Sat: Refactored: moved to caller
    // read the best divisor from the heap
    //pSingle = Lxu_HeapSingleReadMax( p->pHeapSingle );
	
	
    // get the variables of this single-cube divisor
    pVar1 = pSingle->pVar1;
    pVar2 = pSingle->pVar2;

    // create two new columns (vars)
    Lxu_UpdateCreateNewVars( p, &pVarC, &pVarD, 1 );
    // create one new row (cube)
    pCubeNew = Lxu_MatrixAddCube( p, pVarD, 0 );
    pCubeNew->pFirst = pCubeNew;
    // set the first cube
    pVarD->pFirst = pCubeNew;

	Lxu_EuclidSetPoint(&pVarC->position, &pSingle->position);
	Lxu_EuclidSetPoint(&pVarD->position, &pSingle->position);

    // start collecting the affected vars and cubes
    Lxu_MatrixRingCubesStart( p );
    Lxu_MatrixRingVarsStart( p );
    // add the vars
    Lxu_MatrixRingVarsAdd( p, pVar1 );
    Lxu_MatrixRingVarsAdd( p, pVar2 );
    // remove the literals and collect the affected cubes
    // remove the divisors associated with this cube
   	// add to the affected cube the literal corresponding to the new column
	Lxu_UpdateMatrixSingleClean( p, pVar1, pVar2, pVarD );
    // stop collecting the affected vars and cubes
    Lxu_MatrixRingCubesStop( p );
    Lxu_MatrixRingVarsStop( p );

    // add the literals to the new cube
    assert( pVar1->iVar < pVar2->iVar );
    assert( Lxu_SingleCountCoincidence( p, pVar1, pVar2 ) == 0 );
    Lxu_MatrixAddLiteral( p, pCubeNew, pVar1 );
    Lxu_MatrixAddLiteral( p, pCubeNew, pVar2 );

    // create new doubles; we cannot add them in the same loop
	// because we first have to create *all* new cubes for each node
    Lxu_MatrixForEachCubeInRing( p, pCube )
   		Lxu_UpdateAddNewDoubles( p, pCube );
    // update the singles after removing some literals
    Lxu_UpdateCleanOldSingles( p );
    // we should undo the rings before creating new singles

    // unmark the cubes
    Lxu_MatrixRingCubesUnmark( p );
    Lxu_MatrixRingVarsUnmark( p );

    // create new singles
    Lxu_UpdateAddNewSingles( p, pVarC );
    Lxu_UpdateAddNewSingles( p, pVarD );
	p->nDivs1++;
}

/**Function*************************************************************

  Synopsis    [Updates the matrix after accepting a double cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateDouble( Lxu_Matrix * p, Lxu_Double * pDiv )
{
	Lxu_Cube * pCube, * pCubeNew1, * pCubeNew2;
	Lxu_Var * pVarC, * pVarD;

	// Sat: Refactored: moved to caller
    // remove the best divisor from the heap
    //pDiv = Lxu_HeapDoubleGetMax( p->pHeapDouble );
	
	// remove the best divisor from the table
	Lxu_ListTableDelDivisor( p, pDiv );

    // create two new columns (vars)
    Lxu_UpdateCreateNewVars( p, &pVarC, &pVarD, 2 );

	// Place the new divisor
	Lxu_EuclidSetPoint(&pVarC->position, &pDiv->position);
	Lxu_EuclidSetPoint(&pVarD->position, &pDiv->position);

    // create two new rows (cubes)
    pCubeNew1 = Lxu_MatrixAddCube( p, pVarD, 0 );
    pCubeNew1->pFirst = pCubeNew1;
    pCubeNew2 = Lxu_MatrixAddCube( p, pVarD, 1 );
    pCubeNew2->pFirst = pCubeNew1;
    // set the first cube
    pVarD->pFirst = pCubeNew1;

    // add the literals to the new cubes
    Lxu_UpdateMatrixDoubleCreateCubes( p, pCubeNew1, pCubeNew2, pDiv );

    // start collecting the affected cubes and vars
    Lxu_MatrixRingCubesStart( p );
    Lxu_MatrixRingVarsStart( p );
	// replace each two cubes of the pair by one new cube
	// the new cube contains the base and the new literal

    Lxu_UpdateDoublePairs( p, pDiv, pVarD );

    // stop collecting the affected cubes and vars
    Lxu_MatrixRingCubesStop( p );
    Lxu_MatrixRingVarsStop( p );

    // create new doubles; we cannot add them in the same loop
	// because we first have to create *all* new cubes for each node
    Lxu_MatrixForEachCubeInRing( p, pCube )
   		Lxu_UpdateAddNewDoubles( p, pCube );

    // update the singles after removing some literals
    Lxu_UpdateCleanOldSingles( p );

    // undo the temporary rings with cubes and vars
    Lxu_MatrixRingCubesUnmark( p );
    Lxu_MatrixRingVarsUnmark( p );
    // we should undo the rings before creating new singles

    // create new singles
    Lxu_UpdateAddNewSingles( p, pVarC );
    Lxu_UpdateAddNewSingles( p, pVarD );

    // recycle the divisor
	MEM_FREE_LXU( p, Lxu_Double, 1, pDiv );
	p->nDivs2++;
}

/**Function*************************************************************

  Synopsis    [Update the pairs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateDoublePairs( Lxu_Matrix * p, Lxu_Double * pDouble, Lxu_Var * pVar )
{
    Lxu_Pair * pPair;
    Lxu_Cube * pCubeUse, * pCubeRem;
    int i;

    // collect and sort the pairs
    Lxu_UpdatePairsSort( p, pDouble );
    for ( i = 0; i < p->nPairsTemp; i++ )
    {
        // get the pair
        pPair = p->pPairsTemp[i];
	    // out of the two cubes, select the one which comes earlier
	    pCubeUse = Lxu_PairMinCube( pPair );
	    pCubeRem = Lxu_PairMaxCube( pPair );
        // collect the affected cube
        assert( pCubeUse->pOrder == NULL );
        Lxu_MatrixRingCubesAdd( p, pCubeUse );

	    // remove some literals from pCubeUse and all literals from pCubeRem
	    Lxu_UpdateMatrixDoubleClean( p, pCubeUse, pCubeRem );
	    // add a literal that depends on the new variable
	    Lxu_MatrixAddLiteral( p, pCubeUse, pVar );	
        // check the literal count
        assert( pCubeUse->lLits.nItems == pPair->nBase + 1 );
        assert( pCubeRem->lLits.nItems == 0 );

	    // update the divisors by removing useless pairs
	    Lxu_UpdateCleanOldDoubles( p, pDouble, pCubeUse );
	    Lxu_UpdateCleanOldDoubles( p, pDouble, pCubeRem );
	    // remove the pair
	    MEM_FREE_LXU( p, Lxu_Pair, 1, pPair );
    }
    p->nPairsTemp = 0;
}

/**Function*************************************************************

  Synopsis    [Add two cubes corresponding to the given double-cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateMatrixDoubleCreateCubes( Lxu_Matrix * p, Lxu_Cube * pCube1, Lxu_Cube * pCube2, Lxu_Double * pDiv )
{
	Lxu_Lit * pLit1, * pLit2;
    Lxu_Pair * pPair;
	int nBase, nLits1, nLits2;

	// fill in the SOP and copy the fanins
	nBase = nLits1 = nLits2 = 0;
	pPair = pDiv->lPairs.pHead;
	pLit1 = pPair->pCube1->lLits.pHead;
	pLit2 = pPair->pCube2->lLits.pHead;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->iVar == pLit2->iVar )
			{ // skip the cube free part
				pLit1 = pLit1->pHNext;
				pLit2 = pLit2->pHNext;
                nBase++;
			}
			else if ( pLit1->iVar < pLit2->iVar )
			{	// add literal to the first cube
                Lxu_MatrixAddLiteral( p, pCube1, pLit1->pVar );
				// move to the next literal in this cube
				pLit1 = pLit1->pHNext;
                nLits1++;
			}
			else
			{	// add literal to the second cube
                Lxu_MatrixAddLiteral( p, pCube2, pLit2->pVar );
				// move to the next literal in this cube
				pLit2 = pLit2->pHNext;
                nLits2++;
			}
		}
		else if ( pLit1 && !pLit2 )
		{	// add literal to the first cube
            Lxu_MatrixAddLiteral( p, pCube1, pLit1->pVar );
			// move to the next literal in this cube
			pLit1 = pLit1->pHNext;
            nLits1++;
		}
		else if ( !pLit1 && pLit2 )
		{	// add literal to the second cube
            Lxu_MatrixAddLiteral( p, pCube2, pLit2->pVar );
			// move to the next literal in this cube
			pLit2 = pLit2->pHNext;
            nLits2++;
		}
		else
			break;
	}
	assert( pPair->nLits1 == nLits1 );
	assert( pPair->nLits2 == nLits2 );
	assert( pPair->nBase == nBase );
}


/**Function*************************************************************

  Synopsis    [Create the node equal to double-cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateMatrixDoubleClean( Lxu_Matrix * p, Lxu_Cube * pCubeUse, Lxu_Cube * pCubeRem )
{
	Lxu_Lit * pLit1, * bLit1Next;
	Lxu_Lit * pLit2, * bLit2Next;

	// initialize the starting literals
	pLit1     = pCubeUse->lLits.pHead;
	pLit2     = pCubeRem->lLits.pHead;
	bLit1Next = pLit1? pLit1->pHNext: NULL;
	bLit2Next = pLit2? pLit2->pHNext: NULL;
	// go through the pair and remove the literals in the base
	// from the first cube and all literals from the second cube
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->iVar == pLit2->iVar )
            {  // this literal is present in both cubes - it belongs to the base
                // mark the affected var
                if ( pLit1->pVar->pOrder == NULL )
                    Lxu_MatrixRingVarsAdd( p, pLit1->pVar );
                // leave the base in pCubeUse; delete it from pCubeRem
				Lxu_MatrixDelLiteral( p, pLit2 );
				// step to the next literals
				pLit1     = bLit1Next;
				pLit2     = bLit2Next;
				bLit1Next = pLit1? pLit1->pHNext: NULL;
				bLit2Next = pLit2? pLit2->pHNext: NULL;
			}
			else if ( pLit1->iVar < pLit2->iVar )
			{ // this literal is present in pCubeUse - remove it
                // mark the affected var
                if ( pLit1->pVar->pOrder == NULL )
                    Lxu_MatrixRingVarsAdd( p, pLit1->pVar );
                // delete this literal
                Lxu_MatrixDelLiteral( p, pLit1 );
				// step to the next literals
				pLit1     = bLit1Next;
				bLit1Next = pLit1? pLit1->pHNext: NULL;
			}
			else
			{ // this literal is present in pCubeRem - remove it
                // mark the affected var
                if ( pLit2->pVar->pOrder == NULL )
                    Lxu_MatrixRingVarsAdd( p, pLit2->pVar );
                // delete this literal
				Lxu_MatrixDelLiteral( p, pLit2 );
				// step to the next literals
				pLit2     = bLit2Next;
				bLit2Next = pLit2? pLit2->pHNext: NULL;
			}
		}
		else if ( pLit1 && !pLit2 )
		{ // this literal is present in pCubeUse - leave it
            // mark the affected var
            if ( pLit1->pVar->pOrder == NULL )
                Lxu_MatrixRingVarsAdd( p, pLit1->pVar );
            // delete this literal
			Lxu_MatrixDelLiteral( p, pLit1 );
			// step to the next literals
			pLit1     = bLit1Next;
			bLit1Next = pLit1? pLit1->pHNext: NULL;
		}
		else if ( !pLit1 && pLit2 )
		{ // this literal is present in pCubeRem - remove it
            // mark the affected var
            if ( pLit2->pVar->pOrder == NULL )
                Lxu_MatrixRingVarsAdd( p, pLit2->pVar );
            // delete this literal
			Lxu_MatrixDelLiteral( p, pLit2 );
			// step to the next literals
			pLit2     = bLit2Next;
			bLit2Next = pLit2? pLit2->pHNext: NULL;
		}
		else
			break;
	}
}

/**Function*************************************************************

  Synopsis    [Updates the matrix after selecting a single cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateMatrixSingleClean( Lxu_Matrix * p, Lxu_Var * pVar1, Lxu_Var * pVar2, Lxu_Var * pVarNew )
{
	Lxu_Lit * pLit1, * bLit1Next;
	Lxu_Lit * pLit2, * bLit2Next;

    // initialize the starting literals
	pLit1     = pVar1->lLits.pHead;
	pLit2     = pVar2->lLits.pHead;
	bLit1Next = pLit1? pLit1->pVNext: NULL;
	bLit2Next = pLit2? pLit2->pVNext: NULL;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
    		if ( pLit1->pCube->pVar->iVar == pLit2->pCube->pVar->iVar )
			{ // these literals coincide 
			    if ( pLit1->iCube == pLit2->iCube )
			    { // these literals coincide 
                
                    // collect the affected cube
                    assert( pLit1->pCube->pOrder == NULL );
                    Lxu_MatrixRingCubesAdd( p, pLit1->pCube );

                    // add the literal to this cube corresponding to the new column
		            Lxu_MatrixAddLiteral( p, pLit1->pCube, pVarNew );
                    // clean the old cubes
		            Lxu_UpdateCleanOldDoubles( p, NULL, pLit1->pCube );

				    // remove the literals 
				    Lxu_MatrixDelLiteral( p, pLit1 );
				    Lxu_MatrixDelLiteral( p, pLit2 );

				    // go to the next literals
				    pLit1     = bLit1Next;
				    pLit2     = bLit2Next;
				    bLit1Next = pLit1? pLit1->pVNext: NULL;
				    bLit2Next = pLit2? pLit2->pVNext: NULL;
			    }
			    else if ( pLit1->iCube < pLit2->iCube )
			    {
				    pLit1     = bLit1Next;
				    bLit1Next = pLit1? pLit1->pVNext: NULL;
			    }
			    else
			    {
				    pLit2     = bLit2Next;
				    bLit2Next = pLit2? pLit2->pVNext: NULL;
			    }
            }
			else if ( pLit1->pCube->pVar->iVar < pLit2->pCube->pVar->iVar )
			{
				pLit1     = bLit1Next;
				bLit1Next = pLit1? pLit1->pVNext: NULL;
			}
			else
			{
				pLit2     = bLit2Next;
				bLit2Next = pLit2? pLit2->pVNext: NULL;
			}
		}
		else if ( pLit1 && !pLit2 )
		{
			pLit1     = bLit1Next;
			bLit1Next = pLit1? pLit1->pVNext: NULL;
		}
		else if ( !pLit1 && pLit2 )
		{
			pLit2     = bLit2Next;
			bLit2Next = pLit2? pLit2->pVNext: NULL;
		}
		else
			break;
	}
}

/**Function*************************************************************

  Synopsis    [Sort the pairs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdatePairsSort( Lxu_Matrix * p, Lxu_Double * pDouble )
{
    Lxu_Pair * pPair;
    // order the pairs by the first cube to ensure that 
    // new literals are added to the matrix from top to bottom
    // collect pairs into the array
    p->nPairsTemp = 0;
	Lxu_DoubleForEachPair( pDouble, pPair )
        p->pPairsTemp[ p->nPairsTemp++ ] = pPair;
    if ( p->nPairsTemp > 1 )
    {   // sort
        qsort( (void *)p->pPairsTemp, p->nPairsTemp, sizeof(Lxu_Pair *), 
            (int (*)(const void *, const void *)) Lxu_UpdatePairCompare );
        assert( Lxu_UpdatePairCompare( p->pPairsTemp, p->pPairsTemp + p->nPairsTemp - 1 ) < 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Compares the vars by their number.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lxu_UpdatePairCompare( Lxu_Pair ** ppP1, Lxu_Pair ** ppP2 )
{
    Lxu_Cube * pC1 = (*ppP1)->pCube1;
    Lxu_Cube * pC2 = (*ppP2)->pCube1;
    int iP1CubeMin, iP2CubeMin;
    if ( pC1->pVar->iVar < pC2->pVar->iVar )
        return -1;
    if ( pC1->pVar->iVar > pC2->pVar->iVar )
        return 1;
    iP1CubeMin = Lxu_PairMinCubeInt( *ppP1 );
    iP2CubeMin = Lxu_PairMinCubeInt( *ppP2 );
    if ( iP1CubeMin < iP2CubeMin )
        return -1;
    if ( iP1CubeMin > iP2CubeMin )
        return 1;
    assert( 0 );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Create new variables.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateCreateNewVars( Lxu_Matrix * p, Lxu_Var ** ppVarC, Lxu_Var ** ppVarD, int nCubes )
{
    Lxu_Var * pVarC, * pVarD;

	// add a new column for the complement
	pVarC = Lxu_MatrixAddVar( p );
    pVarC->nCubes = 0;
	// add a new column for the divisor
	pVarD = Lxu_MatrixAddVar( p );
    pVarD->nCubes = nCubes;
    // mark this entry in the Value2Node array
    assert( p->pValue2Node[pVarC->iVar] > 0 );

    p->pValue2Node[pVarD->iVar  ] = p->pValue2Node[pVarC->iVar];
    p->pValue2Node[pVarD->iVar+1] = p->pValue2Node[pVarC->iVar]+1;

    *ppVarC = pVarC;
    *ppVarD = pVarD;

	assert (pVarD->iVar % 2 == 1); // Divisor is odd numbered column in matrix
	assert (pVarC->iVar % 2 == 0); // Complement is even numbered column in matrix
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateCleanOldDoubles( Lxu_Matrix * p, Lxu_Double * pDiv, Lxu_Cube * pCube )
{
    Lxu_Double * pDivCur;
	Lxu_Pair * pPair;
    int i;

    // if the cube is a recently introduced one
    // it does not have pairs allocated
    // in this case, there is nothing to update
    if ( pCube->pVar->ppPairs == NULL )
        return;

	// go through all the pairs of this cube
	Lxu_CubeForEachPair( pCube, pPair, i )
	{
         // get the divisor of this pair
        pDivCur = pPair->pDiv;
		// skip the current divisor
		if ( pDivCur == pDiv )
			continue;
		// remove this pair
	    Lxu_ListDoubleDelPair( pDivCur, pPair );	
		// the divisor may have become useless by now
		if ( pDivCur->lPairs.nItems == 0 )
        {
            assert( pDivCur->lWeight == pPair->nBase - 1 );
            //assert( pDivCur->Weight == pPair->nBase - 1 );
      		Lxu_HeapDoubleDelete( p->pHeapDouble, pDivCur );
			Lxu_MatrixDelDivisor( p, pDivCur );
        }
        else
        {
	        // update the divisor's weight
	        pDivCur->lWeight -= pPair->nLits1 + pPair->nLits2 - 1 + pPair->nBase;
			LxuEuclidComputePWeightDouble (p, pDivCur);
			pDivCur->Weight = Lxu_EuclidCombine(pDivCur->lWeight, pDivCur->pWeight);
      	    Lxu_HeapDoubleUpdate( p->pHeapDouble, pDivCur );
        }
		MEM_FREE_LXU( p, Lxu_Pair, 1, pPair );
	}
	// finally erase all the pair info associated with this cube
	Lxu_PairClearStorage( pCube );
}

/**Function*************************************************************

  Synopsis    [Adds the new divisors that depend on the cube.]

  Description [Go through all the non-empty cubes of this cover 
  (except the given cube) and, for each of them, add the new divisor 
  with the given cube.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateAddNewDoubles( Lxu_Matrix * p, Lxu_Cube * pCube )
{
	Lxu_Cube * pTemp;
    assert( pCube->pOrder );

    // if the cube is a recently introduced one
    // it does not have pairs allocated
    // in this case, there is nothing to update
    if ( pCube->pVar->ppPairs == NULL )
        return;

	for ( pTemp = pCube->pFirst; pTemp->pVar == pCube->pVar; pTemp = pTemp->pNext )
    {
        // do not add pairs with the empty cubes
		if ( pTemp->lLits.nItems == 0 )
            continue;
        // to prevent adding duplicated pairs of the new cubes
        // do not add the pair, if the current cube is marked 
        if ( pTemp->pOrder && pTemp >= pCube )
            continue;
		Lxu_MatrixAddDivisor( p, pTemp, pCube );
    }
}

/**Function*************************************************************

  Synopsis    [Removes old single cube divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateCleanOldSingles( Lxu_Matrix * p )
{ 
    Lxu_Single * pSingle, * pSingle2;
    int WeightNew;

    Lxu_MatrixForEachSingleSafe( p, pSingle, pSingle2 )
    {
        // if at least one of the variables is marked, recalculate
        if ( pSingle->pVar1->pOrder || pSingle->pVar2->pOrder )
        {
            // get the new weight
            WeightNew = -2 + Lxu_SingleCountCoincidence( p, pSingle->pVar1, pSingle->pVar2 );
            if ( WeightNew >= 0 )
            {
                pSingle->lWeight = WeightNew;
				LxuEuclidComputePWeightSingle (p, pSingle);

				pSingle->Weight = Lxu_EuclidCombine(pSingle->lWeight, pSingle->pWeight);
                Lxu_HeapSingleUpdate( p->pHeapSingle, pSingle );
            }
            else
            {
                Lxu_HeapSingleDelete( p->pHeapSingle, pSingle );
                Lxu_ListMatrixDelSingle( p, pSingle );
		        MEM_FREE_LXU( p, Lxu_Single, 1, pSingle );
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Updates the single cube divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_UpdateAddNewSingles( Lxu_Matrix * p, Lxu_Var * pVar )
{
    Lxu_MatrixComputeSinglesOne( p, pVar );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


