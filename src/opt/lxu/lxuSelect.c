/**CFile****************************************************************

  FileName    [lxuSelect.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to select the best divisor/complement pair.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuSelect.c,v 1.2 2004/01/30 00:18:51 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_SIZE_LOOKAHEAD      20

static int Lxu_MatrixFindComplement( Lxu_Matrix * p, int iVar );

static Lxu_Double * Lxu_MatrixFindComplementSingle( Lxu_Matrix * p, Lxu_Single * pSingle );
static Lxu_Single * Lxu_MatrixFindComplementDouble2( Lxu_Matrix * p, Lxu_Double * pDouble );
static Lxu_Double * Lxu_MatrixFindComplementDouble4( Lxu_Matrix * p, Lxu_Double * pDouble );

Lxu_Double * Lxu_MatrixFindDouble( Lxu_Matrix * p, 
     int piVarsC1[], int piVarsC2[], int nVarsC1, int nVarsC2 );
void Lxu_MatrixGetDoubleVars( Lxu_Matrix * p, Lxu_Double * pDouble, 
    int piVarsC1[], int piVarsC2[], int * pnVarsC1, int * pnVarsC2 );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Selects the best pair (Single,Double) and returns their weight.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
weightType Lxu_Select( Lxu_Matrix * p, Lxu_Single ** ppSingle, Lxu_Double ** ppDouble )
{
    // the top entries
    Lxu_Single * pSingles[MAX_SIZE_LOOKAHEAD];
    Lxu_Double * pDoubles[MAX_SIZE_LOOKAHEAD];
    // the complements
    Lxu_Double * pSCompl[MAX_SIZE_LOOKAHEAD];
    Lxu_Single * pDComplS[MAX_SIZE_LOOKAHEAD];
    Lxu_Double * pDComplD[MAX_SIZE_LOOKAHEAD];
    Lxu_Pair * pPair;
    int nSingles;
    int nDoubles;
    int i;
    weightType WeightBest;
    weightType WeightCur, lWeightCur, pWeightCur;
    int iNum, fBestS;

    // collect the top entries from the queues
    for ( nSingles = 0; nSingles < MAX_SIZE_LOOKAHEAD; nSingles++ )
    {
        pSingles[nSingles] = Lxu_HeapSingleGetMax( p->pHeapSingle );
        if ( pSingles[nSingles] == NULL )
            break;
    }
    // put them back into the queue
    for ( i = 0; i < nSingles; i++ )
        if ( pSingles[i] )
            Lxu_HeapSingleInsert( p->pHeapSingle, pSingles[i] );
        
    // the same for doubles
    // collect the top entries from the queues
    for ( nDoubles = 0; nDoubles < MAX_SIZE_LOOKAHEAD; nDoubles++ )
    {
        pDoubles[nDoubles] = Lxu_HeapDoubleGetMax( p->pHeapDouble );
        if ( pDoubles[nDoubles] == NULL )
            break;
    }
    // put them back into the queue
    for ( i = 0; i < nDoubles; i++ )
        if ( pDoubles[i] )
            Lxu_HeapDoubleInsert( p->pHeapDouble, pDoubles[i] );

    // for each single, find the complement double (if any)
    for ( i = 0; i < nSingles; i++ )
        if ( pSingles[i] )
            pSCompl[i] = Lxu_MatrixFindComplementSingle( p, pSingles[i] );

    // for each double, find the complement single or double (if any)
    for ( i = 0; i < nDoubles; i++ )
        if ( pDoubles[i] )
        {
            pPair = pDoubles[i]->lPairs.pHead;
            if ( pPair->nLits1 == 1 && pPair->nLits2 == 1 )
            {
                pDComplS[i] = Lxu_MatrixFindComplementDouble2( p, pDoubles[i] );
                pDComplD[i] = NULL;
            }
//            else if ( pPair->nLits1 == 2 && pPair->nLits2 == 2 )
//            {
//                pDComplS[i] = NULL;
//                pDComplD[i] = Lxu_MatrixFindComplementDouble4( p, pDoubles[i] );
//            }
            else
            {
                pDComplS[i] = NULL;
                pDComplD[i] = NULL;
            }
        }

	// The weights in the following calculation are only the
	// logical weights unless indicated otherwise

    // select the best pair
    WeightBest = weightType_NEG_INF;

    for ( i = 0; i < nSingles; i++ )
    {
        lWeightCur = pSingles[i]->lWeight;
        pWeightCur = pSingles[i]->pWeight;

        if ( pSCompl[i] )
        {
            // add the weight of the double
            lWeightCur += pSCompl[i]->lWeight;
            // there is no need to implement this double, so...
            pPair      = pSCompl[i]->lPairs.pHead;
            lWeightCur += pPair->nLits1 + pPair->nLits2;

			//TODO: Determine if this necessitates a change to pWeightCur
        }

		WeightCur = Lxu_EuclidCombine(lWeightCur, pWeightCur);
		
        if ( WeightBest < WeightCur )
        {
            WeightBest = WeightCur;
            *ppSingle = pSingles[i];
            *ppDouble = pSCompl[i];
            fBestS = 1;
            iNum = i;
        }
    }
    for ( i = 0; i < nDoubles; i++ )
    {
        lWeightCur = pDoubles[i]->lWeight;
        pWeightCur = pDoubles[i]->pWeight;
        if ( pDComplS[i] )
        {
            // add the weight of the single
            lWeightCur += pDComplS[i]->lWeight;
            // there is no need to implement this double, so...
            pPair      = pDoubles[i]->lPairs.pHead;
            lWeightCur += pPair->nLits1 + pPair->nLits2;
			
			//TODO: Determine if this necessitates a change to pWeightCur
        }

		WeightCur = Lxu_EuclidCombine(lWeightCur, pWeightCur);

        if ( WeightBest < WeightCur )
        {
            WeightBest = WeightCur;
            *ppSingle = pDComplS[i];
            *ppDouble = pDoubles[i];
            fBestS = 0;
            iNum = i;
        }
    }

/*  NOTE: if uncommenting fix the printfs to match weightType
 *  Sat 6/21/2003
    // print the statistics
    printf( "\n" );
    for ( i = 0; i < nSingles; i++ )
    {
        printf( "Single #%d: Weight = %3d. ", i, pSingles[i]->Weight );
        printf( "Compl: " );
        if ( pSCompl[i] == NULL )
            printf( "None." );
        else
            printf( "D  Weight = %3d  Sum = %3d", 
                pSCompl[i]->Weight, pSCompl[i]->Weight + pSingles[i]->Weight );
        printf( "\n" );
    }
    printf( "\n" );
    for ( i = 0; i < nDoubles; i++ )
    {
        printf( "Double #%d: Weight = %3d. ", i, pDoubles[i]->Weight );
        printf( "Compl: " );
        if ( pDComplS[i] == NULL && pDComplD[i] == NULL )
            printf( "None." );
        else if ( pDComplS[i] )
            printf( "S  Weight = %3d  Sum = %3d", 
                pDComplS[i]->Weight, pDComplS[i]->Weight + pDoubles[i]->Weight );
        else if ( pDComplD[i] )
            printf( "D  Weight = %3d  Sum = %3d", 
                pDComplD[i]->Weight, pDComplD[i]->Weight + pDoubles[i]->Weight );
        printf( "\n" );
    }
    if ( WeightBest == -1 )
        printf( "Selected NONE\n" );
    else
    {
        printf( "Selected = %s.  ", fBestS? "S": "D" );
        printf( "Number = %d.  ", iNum );
        printf( "Weight = %d.\n", WeightBest );
    }
    printf( "\n" );
*/
	if (WeightBest == weightType_NEG_INF)
		WeightBest = -1;

    return WeightBest;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Double * Lxu_MatrixFindComplementSingle( Lxu_Matrix * p, Lxu_Single * pSingle )
{
    int * pValue2Node = p->pValue2Node;
    int iVar1,  iVar2;
    int iVar1C, iVar2C;
    // get the variables of this single div
    iVar1  = pSingle->pVar1->iVar;
    iVar2  = pSingle->pVar2->iVar;
    iVar1C = Lxu_MatrixFindComplement( p, iVar1 );
    iVar2C = Lxu_MatrixFindComplement( p, iVar2 );
    if ( iVar1C == -1 || iVar2C == -1 )
        return NULL;
    assert( iVar1C < iVar2C );
    return Lxu_MatrixFindDouble( p, &iVar1C, &iVar2C, 1, 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Single * Lxu_MatrixFindComplementDouble2( Lxu_Matrix * p, Lxu_Double * pDouble )
{
    int * pValue2Node = p->pValue2Node;
    int piVarsC1[10], piVarsC2[10];
    int nVarsC1, nVarsC2;
    int iVar1,  iVar2, iVarTemp;
    int iVar1C, iVar2C;
    Lxu_Single * pSingle;

    // get the variables of this double div
    Lxu_MatrixGetDoubleVars( p, pDouble, piVarsC1, piVarsC2, &nVarsC1, &nVarsC2 );
    assert( nVarsC1 == 1 );
    assert( nVarsC2 == 1 );
    iVar1 = piVarsC1[0];
    iVar2 = piVarsC2[0];
    assert( iVar1 < iVar2 );

    iVar1C = Lxu_MatrixFindComplement( p, iVar1 );
    iVar2C = Lxu_MatrixFindComplement( p, iVar2 );
    if ( iVar1C == -1 || iVar2C == -1 )
        return NULL;
 
    // go through the queque and find this one
//    assert( iVar1C < iVar2C );
    if ( iVar1C > iVar2C )
    {
        iVarTemp = iVar1C;
        iVar1C = iVar2C;
        iVar2C = iVarTemp;
    }

    Lxu_MatrixForEachSingle( p, pSingle )
        if ( pSingle->pVar1->iVar == iVar1C && pSingle->pVar2->iVar == iVar2C )
            return pSingle;
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Double * Lxu_MatrixFindComplementDouble4( Lxu_Matrix * p, Lxu_Double * pDouble )
{
    int * pValue2Node = p->pValue2Node;
    int piVarsC1[10], piVarsC2[10];
    int nVarsC1, nVarsC2;
    int iVar11,  iVar12,  iVar21,  iVar22;
    int iVar11C, iVar12C, iVar21C, iVar22C;
    int RetValue;

    // get the variables of this double div
    Lxu_MatrixGetDoubleVars( p, pDouble, piVarsC1, piVarsC2, &nVarsC1, &nVarsC2 );
    assert( nVarsC1 == 2 && nVarsC2 == 2 );

    iVar11 = piVarsC1[0];
    iVar12 = piVarsC1[1];
    iVar21 = piVarsC2[0];
    iVar22 = piVarsC2[1];
    assert( iVar11 < iVar21 );

    iVar11C = Lxu_MatrixFindComplement( p, iVar11 );
    iVar12C = Lxu_MatrixFindComplement( p, iVar12 );
    iVar21C = Lxu_MatrixFindComplement( p, iVar21 );
    iVar22C = Lxu_MatrixFindComplement( p, iVar22 );
    if ( iVar11C == -1 || iVar12C == -1 || iVar21C == -1 || iVar22C == -1 )
        return NULL;
    if ( iVar11C != iVar21 || iVar12C != iVar22 || 
         iVar21C != iVar11 || iVar22C != iVar12 )
         return NULL;

    // a'b' + ab   =>  a'b  + ab'
    // a'b  + ab'  =>  a'b' + ab
    // swap the second pair in each cube
    RetValue    = piVarsC1[1];
    piVarsC1[1] = piVarsC2[1];
    piVarsC2[1] = RetValue;

    return Lxu_MatrixFindDouble( p, piVarsC1, piVarsC2, 2, 2 );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lxu_MatrixFindComplement( Lxu_Matrix * p, int iVar )
{
    int * pValue2Node = p->pValue2Node;
    int iVarC;
    int iNode;
    int Beg, End;

    // get the nodes
    iNode = pValue2Node[iVar];
    // get the first node with the same var
    for ( Beg = iVar; Beg >= 0; Beg-- )
        if ( pValue2Node[Beg] != iNode )
        {
            Beg++;
            break;
        }
    // get the last node with the same var
    for ( End = iVar;          ; End++ )
        if ( pValue2Node[End] != iNode )
        {
            End--;
            break;
        }

    // if one of the vars is not binary, quit
    if ( End - Beg > 1 )
        return -1;

    // get the complements
    if ( iVar == Beg )
        iVarC = End;
    else if ( iVar == End ) 
        iVarC = Beg;
    else
    {
        assert( 0 );
    }
    return iVarC;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixGetDoubleVars( Lxu_Matrix * p, Lxu_Double * pDouble, 
    int piVarsC1[], int piVarsC2[], int * pnVarsC1, int * pnVarsC2 )
{
    Lxu_Pair * pPair;
	Lxu_Lit * pLit1, * pLit2;
    int nLits1, nLits2;

    // get the first pair
    pPair = pDouble->lPairs.pHead;
    // init the parameters
	nLits1 = 0;
	nLits2 = 0;
	pLit1 = pPair->pCube1->lLits.pHead;
	pLit2 = pPair->pCube2->lLits.pHead;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->iVar == pLit2->iVar )
			{ // ensure cube-free
				pLit1 = pLit1->pHNext;
				pLit2 = pLit2->pHNext;
			}
			else if ( pLit1->iVar < pLit2->iVar )
            {
                piVarsC1[nLits1++] = pLit1->iVar;
 				pLit1 = pLit1->pHNext;
           }
			else
            {
                piVarsC2[nLits2++] = pLit2->iVar;
				pLit2 = pLit2->pHNext;
            }
		}
		else if ( pLit1 && !pLit2 )
        {
            piVarsC1[nLits1++] = pLit1->iVar;
    		pLit1 = pLit1->pHNext;
        }
		else if ( !pLit1 && pLit2 )
        {
            piVarsC2[nLits2++] = pLit2->iVar;
			pLit2 = pLit2->pHNext;
        }
		else
			break;
	}
    *pnVarsC1 = nLits1;
    *pnVarsC2 = nLits2;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Double * Lxu_MatrixFindDouble( Lxu_Matrix * p, 
     int piVarsC1[], int piVarsC2[], int nVarsC1, int nVarsC2 )
{
    int piVarsC1_[100], piVarsC2_[100];
    int nVarsC1_, nVarsC2_, i;
    Lxu_Double * pDouble;
    Lxu_Pair * pPair;
    unsigned Key;

    assert( nVarsC1 > 0 );
    assert( nVarsC2 > 0 );
    assert( piVarsC1[0] < piVarsC2[0] );

    // get the hash key
    Key = Lxu_PairHashKeyArray( p, piVarsC1, piVarsC2, nVarsC1, nVarsC2 );
    
    // check if the divisor for this pair already exists
    Key %= p->nTableSize;
	Lxu_TableForEachDouble( p, Key, pDouble )
    {
        pPair = pDouble->lPairs.pHead;
        if ( pPair->nLits1 != nVarsC1 )
            continue;
        if ( pPair->nLits2 != nVarsC2 )
            continue;
        // get the cubes of this divisor
        Lxu_MatrixGetDoubleVars( p, pDouble, piVarsC1_, piVarsC2_, &nVarsC1_, &nVarsC2_ );
        // compare lits of the first cube
        for ( i = 0; i < nVarsC1; i++ )
            if ( piVarsC1[i] != piVarsC1_[i] )
                break;
        if ( i != nVarsC1 )
            continue;
        // compare lits of the second cube
        for ( i = 0; i < nVarsC2; i++ )
            if ( piVarsC2[i] != piVarsC2_[i] )
                break;
        if ( i != nVarsC2 )
            continue;
        return pDouble;
    }
    return NULL;
}





/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
weightType Lxu_SelectSCD( Lxu_Matrix * p, weightType WeightLimit, Lxu_Var ** ppVar1, Lxu_Var ** ppVar2 )
{
    int * pValue2Node = p->pValue2Node;
    Lxu_Var * pVar1;
    Lxu_Var * pVar2, * pVarTemp;
    Lxu_Lit * pLitV, * pLitH;
    int Coin;
    int CounterAll;
    int CounterTest;
    weightType WeightCur;
    weightType WeightBest;

    CounterAll = 0;
    CounterTest = 0;

    WeightBest = -10;
    
    // iterate through the columns in the matrix
    Lxu_MatrixForEachVariable( p, pVar1 )
    {
        // start collecting the affected vars
        Lxu_MatrixRingVarsStart( p );

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
                if ( pValue2Node[pVar1->iVar] == pValue2Node[pVar2->iVar] )
                    continue;
                // collect the var
                Lxu_MatrixRingVarsAdd( p, pVar2 );
            }
        }
        // stop collecting the selected vars
        Lxu_MatrixRingVarsStop( p );

        // iterate through the selected vars
        Lxu_MatrixForEachVarInRing( p, pVar2 )
        {
            CounterTest++;

            // count the coincidence
            Coin = Lxu_SingleCountCoincidence( p, pVar1, pVar2 );
            assert( Coin > 0 );

            // get the new weight
            WeightCur = Coin - 2;

            // compare the weights
            if ( WeightBest < WeightCur )
            {
                WeightBest = WeightCur;
                *ppVar1 = pVar1;
                *ppVar2 = pVar2;
            }
        }
        // unmark the vars
        Lxu_MatrixForEachVarInRingSafe( p, pVar2, pVarTemp )
            pVar2->pOrder = NULL;
        Lxu_MatrixRingVarsReset( p );
    }

//    if ( WeightBest == WeightLimit )
//        return -1;
    return WeightBest;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


