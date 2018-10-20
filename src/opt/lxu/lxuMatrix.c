/**CFile****************************************************************

  FileName    [lxuMatrix.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to manipulate the sparse matrix.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuMatrix.c,v 1.2 2004/01/30 00:18:49 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern unsigned int Cudd_Prime( unsigned int p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Matrix * Lxu_MatrixAllocate()
{
	Lxu_Matrix * p;
	p = ALLOC( Lxu_Matrix, 1 );
	memset( p, 0, sizeof(Lxu_Matrix) );
	p->nTableSize = Cudd_Prime(10000);
	p->pTable = ALLOC( Lxu_ListDouble, p->nTableSize );
	memset( p->pTable, 0, sizeof(Lxu_ListDouble) * p->nTableSize );
#ifndef USE_SYSTEM_MEMORY_MANAGEMENT
    {
        // get the largest size in bytes for the following structures:
        // Lxu_Cube, Lxu_Var, Lxu_Lit, Lxu_Pair, Lxu_Double, Lxu_Single
        // (currently, Lxu_Var, Lxu_Pair, Lxu_Double take 10 machine words)
        int nSizeMax, nSizeCur;
        nSizeMax = -1;
        nSizeCur = sizeof(Lxu_Cube);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Lxu_Var);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Lxu_Lit);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Lxu_Pair);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Lxu_Double);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Lxu_Single);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
    	p->pMemMan  = Extra_MmFixedStart( nSizeMax, 5000, 1000 ); 
    }
#endif
	p->pHeapDouble = Lxu_HeapDoubleStart();
	p->pHeapSingle = Lxu_HeapSingleStart();
	return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixDelete( Lxu_Matrix * p )
{
	Lxu_HeapDoubleCheck( p->pHeapDouble );
	Lxu_HeapDoubleStop( p->pHeapDouble );
    Lxu_HeapSingleStop( p->pHeapSingle );

	// delete other things
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
	// this code is not needed when the custom memory manager is used
    {
	    Lxu_Cube * pCube, * pCube2;
	    Lxu_Var * pVar, * pVar2;
	    Lxu_Lit * pLit, * pLit2;
	    Lxu_Double * pDiv, * pDiv2;
	    Lxu_Single * pSingle, * pSingle2;
	    Lxu_Pair * pPair, * pPair2;
        int i;
	    // delete the divisors
	    Lxu_MatrixForEachDoubleSafe( p, pDiv, pDiv2, i )
	    {
		    Lxu_DoubleForEachPairSafe( pDiv, pPair, pPair2 )
			    MEM_FREE_LXU( p, Lxu_Pair, 1, pPair );
   		    MEM_FREE_LXU( p, Lxu_Double, 1, pDiv );
	    }
	    Lxu_MatrixForEachSingleSafe( p, pSingle, pSingle2 )
   		    MEM_FREE_LXU( p, Lxu_Single, 1, pSingle );
	    // delete the entries
	    Lxu_MatrixForEachCube( p, pCube )
		    Lxu_CubeForEachLiteralSafe( pCube, pLit, pLit2 )
			    MEM_FREE_LXU( p, Lxu_Lit, 1, pLit );
	    // delete the cubes
	    Lxu_MatrixForEachCubeSafe( p, pCube, pCube2 )
		    MEM_FREE_LXU( p, Lxu_Cube, 1, pCube );
	    // delete the vars
	    Lxu_MatrixForEachVariableSafe( p, pVar, pVar2 )
		    MEM_FREE_LXU( p, Lxu_Var, 1, pVar );
    }
#else
	Extra_MmFixedStop( p->pMemMan, 0 );
#endif

	FREE( p->pppPairs );
	FREE( p->ppPairs );
    FREE( p->pPairsTemp );
	FREE( p->pTable );
	FREE( p->ppVars );
	FREE( p->pVarsTemp );
	FREE( p );
}



/**Function*************************************************************

  Synopsis    [Adds a variable to the matrix.]

  Description [This procedure always adds variables at the end of the matrix.
  It assigns the var's node and number. It adds the var to the linked list of
  all variables and to the table of all nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Var * Lxu_MatrixAddVar( Lxu_Matrix * p )
{
	Lxu_Var * pVar;
	pVar = MEM_ALLOC_LXU( p, Lxu_Var, 1 );
	memset( pVar, 0, sizeof(Lxu_Var) );
	pVar->iVar = p->lVars.nItems;
    p->ppVars[pVar->iVar] = pVar;
	Lxu_ListMatrixAddVariable( p, pVar );

	return pVar;
}

/**Function*************************************************************

  Synopsis    [Adds a literal to the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Cube * Lxu_MatrixAddCube( Lxu_Matrix * p, Lxu_Var * pVar, int iCube )
{
	Lxu_Cube * pCube;
	pCube = MEM_ALLOC_LXU( p, Lxu_Cube, 1 );
	memset( pCube, 0, sizeof(Lxu_Cube) );
	pCube->pVar  = pVar;
	pCube->iCube = iCube;
	Lxu_ListMatrixAddCube( p, pCube );
	return pCube;
}

/**Function*************************************************************

  Synopsis    [Adds a literal to the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixAddLiteral( Lxu_Matrix * p, Lxu_Cube * pCube, Lxu_Var * pVar )
{
	Lxu_Lit * pLit;
	pLit = MEM_ALLOC_LXU( p, Lxu_Lit, 1 );
	memset( pLit, 0, sizeof(Lxu_Lit) );
	// insert the literal into two linked lists
	Lxu_ListCubeAddLiteral( pCube, pLit );
	Lxu_ListVarAddLiteral( pVar, pLit );
	// set the back pointers
	pLit->pCube = pCube;
	pLit->pVar  = pVar;
	pLit->iCube = pCube->iCube;
	pLit->iVar  = pVar->iVar;
	// increment the literal counter
	p->nEntries++;
}

/**Function*************************************************************

  Synopsis    [Deletes the divisor from the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixDelDivisor( Lxu_Matrix * p, Lxu_Double * pDiv )
{
	// delete divisor from the table
	Lxu_ListTableDelDivisor( p, pDiv );
	// recycle the divisor
	MEM_FREE_LXU( p, Lxu_Double, 1, pDiv );
}

/**Function*************************************************************

  Synopsis    [Deletes the literal fromthe matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixDelLiteral( Lxu_Matrix * p, Lxu_Lit * pLit )
{
    // delete the literal
	Lxu_ListCubeDelLiteral( pLit->pCube, pLit );
	Lxu_ListVarDelLiteral( pLit->pVar, pLit );
	MEM_FREE_LXU( p, Lxu_Lit, 1, pLit );
	// increment the literal counter
	p->nEntries--;
}


/**Function*************************************************************

  Synopsis    [Creates and adds a single cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixAddSingle( Lxu_Matrix * p, Lxu_Var * pVar1, Lxu_Var * pVar2, weightType lWeight )
{
    Lxu_Single * pSingle;
    assert( pVar1->iVar < pVar2->iVar );
	pSingle = MEM_ALLOC_LXU( p, Lxu_Single, 1 );
	memset( pSingle, 0, sizeof(Lxu_Single) );
    pSingle->Num = p->lSingles.nItems;
    pSingle->lWeight = lWeight;
    pSingle->pWeight = 0;
    pSingle->Weight = 0;

    pSingle->HNum = 0;
    pSingle->pVar1 = pVar1;
    pSingle->pVar2 = pVar2;
    Lxu_ListMatrixAddSingle( p, pSingle );

 
	// Initially physical weight is computed later by the Lxu_EuclidPlaceAllAndComputeWeights(p) 
	// routine called from lxu main, but later on  as new singles are added they are given the correct weights
	if (1==p->allPlaced) {
		LxuEuclidComputePWeightSingle (p, pSingle);
	} else {
		pSingle->pWeight = 0;
	}
	
	pSingle->Weight = Lxu_EuclidCombine(pSingle->lWeight, pSingle->pWeight);
	
    // add to the heap
    Lxu_HeapSingleInsert( p->pHeapSingle, pSingle );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixAddDivisor( Lxu_Matrix * p, Lxu_Cube * pCube1, Lxu_Cube * pCube2 )
{
	Lxu_Pair * pPair;
	Lxu_Double * pDiv;
    int nBase, nLits1, nLits2;
    int fFound;
	unsigned Key;

    // canonicize the pair
    Lxu_PairCanonicize( &pCube1, &pCube2 );

    // compute the hash key
    if ( p->fMvNetwork )
    {   // in case of MV network, if all the values in the cube-free divisor
        // belong to the same MV variable, this cube pair is not a divisor
        Key = Lxu_PairHashKeyMv( p, pCube1, pCube2, &nBase, &nLits1, &nLits2 );
        if ( Key == 0 )
            return;
    }
    else
        Key = Lxu_PairHashKey( p, pCube1, pCube2, &nBase, &nLits1, &nLits2 );

    // create the cube pair
    pPair = Lxu_PairAlloc( p, pCube1, pCube2 );
    pPair->nBase  = nBase;
    pPair->nLits1 = nLits1;
    pPair->nLits2 = nLits2;

    // check if the divisor for this pair already exists
    fFound = 0;
    Key %= p->nTableSize;
	Lxu_TableForEachDouble( p, Key, pDiv )
		if ( Lxu_PairCompare( pPair, pDiv->lPairs.pTail ) ) // they are equal
        {
            fFound = 1;
            break;
        }

    if ( !fFound )
    {   // create the new divisor
	    pDiv = MEM_ALLOC_LXU( p, Lxu_Double, 1 );
	    memset( pDiv, 0, sizeof(Lxu_Double) );
	    pDiv->Key = Key;
	    // set the number of this divisor
	    pDiv->Num = p->nDivsTotal++; // p->nDivs;
	    // insert the divisor in the table
	    Lxu_ListTableAddDivisor( p, pDiv );
	    // set the initial cost of the divisor
	    pDiv->lWeight -= pPair->nLits1 + pPair->nLits2;
		pDiv->pWeight = 0;
    }

	// link the pair to the cubes
	Lxu_PairAdd( pPair );
	// connect the pair and the divisor
	pPair->pDiv = pDiv;
	Lxu_ListDoubleAddPairLast( pDiv, pPair );	
    // update the max number of pairs in a divisor
    if ( p->nPairsMax < pDiv->lPairs.nItems )
        p->nPairsMax = pDiv->lPairs.nItems;
	// update the divisor's weight
	pDiv->lWeight += pPair->nLits1 + pPair->nLits2 - 1 + pPair->nBase;

	// For the first time we do not compute the physical weight incrementally
	// but do it once the entire matrix is computed. After that when new divisors
	// are added we compute their weights incrementally

	if (1==p->allPlaced) {
		LxuEuclidComputePWeightDouble (p, pDiv);
	} else {
		pDiv->pWeight = 0;
	}
	
	pDiv->Weight = Lxu_EuclidCombine(pDiv->lWeight, pDiv->pWeight);
    if ( fFound ) // update the divisor in the heap
        Lxu_HeapDoubleUpdate( p->pHeapDouble, pDiv );
    else  // add the new divisor to the heap
        Lxu_HeapDoubleInsert( p->pHeapDouble, pDiv );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


