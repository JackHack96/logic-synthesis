/**CFile****************************************************************

  FileName    [resmFanin.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmFanin.c,v 1.1 2005/01/23 06:59:51 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Resm_Node_t * Resm_NodeDropFanin( Resm_Node_t * pPivot, Resm_Node_t * pCut, Resm_Node_t * pFanin );
static Resm_Node_t * Resm_NodeReplaceFaninByOne( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes );
static Resm_Node_t * Resm_NodeReplaceFaninByTwo( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes );
static Resm_Node_t * Resm_NodeReplaceFaninByThree( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes );
static Resm_Node_t * Resm_NodeCompressFanins( Resm_Node_t * pPivot, Resm_Node_t * pCut );
static int           Resm_NodeCollectUniqueMinusFanin( Resm_Node_t * pCut, Resm_Node_t * pFanin, Resm_Node_t * pResult[] );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure tries to move one fanin of the node using candidates.]

  Description [Returns the cut set.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeReplaceOneFanin( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes )
{
    Resm_Man_t * p = pPivot->p;
    Resm_Node_t * pCutList = NULL, * pCutsNew, * pCutTemp;
    int fTryTwo = 1;

    assert( pPivot->nLeaves <= p->nVarsMax );

    // try dropping this fanin
    if ( pPivot->nLeaves > 2 )
    if ( pCutList = Resm_NodeDropFanin( pPivot, pPivot, pFanin ) )
        return pCutList;

    // try replacing fanin by one node
    pCutsNew = Resm_NodeReplaceFaninByOne( pPivot, pFanin, vNodes );
    pCutList = Resm_NodeListAppend( pCutList, pCutsNew );
    if ( !fTryTwo || pPivot->nLeaves >= 6 )
        return pCutList;

    // it may happen that an inferior one-node change prevents a better two-node change
    // mark the nodes used in one-node changes
    for ( pCutTemp = pCutList; pCutTemp; pCutTemp = pCutTemp->pNext )
        pCutTemp->ppLeaves[pCutTemp->nLeaves-1]->fUsed = 1;

    // try replacing fanin by two nodes
    pCutsNew = Resm_NodeReplaceFaninByTwo( pPivot, pFanin, vNodes );
    pCutList = Resm_NodeListAppend( pCutList, pCutsNew );

    // unmark the nodes used in one-node changes
    for ( pCutTemp = pCutList; pCutTemp; pCutTemp = pCutTemp->pNext )
        pCutTemp->ppLeaves[pCutTemp->nLeaves-1]->fUsed = 0;
    if ( pCutList || pPivot->nLeaves >= 5 )
        return pCutList;

    // try three node combinations
    if ( p->fUseThree )
        pCutList = Resm_NodeReplaceFaninByThree( pPivot, pFanin, vNodes );
    return pCutList;
}

/**Function*************************************************************

  Synopsis    [Tries to drop the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeDropFanin( Resm_Node_t * pPivot, Resm_Node_t * pCut, Resm_Node_t * pFanin )
{
    Resm_Node_t * ppNodes[10];
    int nInputs;
    nInputs = Resm_NodeCollectUniqueMinusFanin( pCut, pFanin, ppNodes );
    return Resm_CheckCut( pPivot, ppNodes, nInputs, 1 );
}

/**Function*************************************************************

  Synopsis    [Tries to drop the fanin and add one extra node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeReplaceFaninByOne( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes )
{
    Resm_Man_t * p = pPivot->p;
    Resm_Node_t * ppNodes[10];
    Resm_Node_t * pCutNew, * pCutTemp, * pCutList;
    int i, nInputs;

    // reduce the fanins
    nInputs = Resm_NodeCollectUniqueMinusFanin( pPivot, pFanin, ppNodes );
    assert( nInputs < pPivot->nLeaves );

    // try adding one more node
    pCutList = NULL;
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        ppNodes[nInputs] = vNodes->pArray[i];
        if ( pCutNew = Resm_CheckCut( pPivot, ppNodes, nInputs+1, 1 ) )
        {
            pCutNew = Resm_NodeCompressFanins( pPivot, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Resm_NodeFree( pCutTemp );
            // don't collect if it is larger
            if ( pCutNew->nLeaves > p->nVarsMax )
                continue;
            pCutNew->pNext = pCutList;
            pCutList = pCutNew;
            // mark the cut
            pCutNew->nVolume = 1;
        }
    }
    return pCutList;
}

/**Function*************************************************************

  Synopsis    [Tries to drop the fanin and add one extra node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeReplaceFaninByTwo( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes )
{
    Resm_Man_t * p = pPivot->p;
    Resm_Node_t * ppNodes[10];
    Resm_Node_t * pCutNew, * pCutTemp, * pCutList;
    int i, j, nInputs;

    // reduce the fanins
    nInputs = Resm_NodeCollectUniqueMinusFanin( pPivot, pFanin, ppNodes );
    assert( nInputs < pPivot->nLeaves );

    // try adding two more fanins
    pCutList = NULL;
    for ( i = 0; i < vNodes->nSize; i++ )
    if ( !vNodes->pArray[i]->fUsed )
    for ( j = i+1; j < vNodes->nSize; j++ )
    if ( !vNodes->pArray[j]->fUsed )
    {
        ppNodes[nInputs+0] = vNodes->pArray[i];
        ppNodes[nInputs+1] = vNodes->pArray[j];
        if ( pCutNew = Resm_CheckCut( pPivot, ppNodes, nInputs+2, 1 ) )
        {
            pCutNew = Resm_NodeCompressFanins( pPivot, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Resm_NodeFree( pCutTemp );
            // don't collect if it is larger
            if ( pCutNew->nLeaves > p->nVarsMax )
                continue;
            pCutNew->pNext = pCutList;
            pCutList = pCutNew;
            // mark the cut
            pCutNew->nVolume = 2;
        }
    }
    return pCutList;
}


/**Function*************************************************************

  Synopsis    [Tries to drop the fanin and add one extra node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeReplaceFaninByThree( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes )
{
    Resm_Man_t * p = pPivot->p;
    Resm_Node_t * ppNodes[10];
    Resm_Node_t * pCutNew, * pCutTemp, * pCutList;
    int i, j, k, nInputs;
    int nSizeLimit; // the maximum number of nodes to consider

    if ( !p->fDoingArea )
        nSizeLimit = 40;
    else
        nSizeLimit = 50;

    // reduce the fanins
    nInputs = Resm_NodeCollectUniqueMinusFanin( pPivot, pFanin, ppNodes );
    assert( nInputs < pPivot->nLeaves );

    // try adding two more fanins
    pCutList = NULL;
    nSizeLimit = RESM_MIN( nSizeLimit, vNodes->nSize );
    for ( i = 0; i < nSizeLimit; i++ )
    for ( j = i+1; j < nSizeLimit; j++ )
    for ( k = j+1; k < nSizeLimit; k++ )
    {
        ppNodes[nInputs+0] = vNodes->pArray[i];
        ppNodes[nInputs+1] = vNodes->pArray[j];
        ppNodes[nInputs+2] = vNodes->pArray[k];
        if ( pCutNew = Resm_CheckCut( pPivot, ppNodes, nInputs+3, 1 ) )
        {
            pCutNew = Resm_NodeCompressFanins( pPivot, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Resm_NodeFree( pCutTemp );
            // don't collect if it is larger
            if ( pCutNew->nLeaves > p->nVarsMax )
                continue;
            pCutNew->pNext = pCutList;
            pCutList = pCutNew;
            // mark the cut
            pCutNew->nVolume = 3;
        }
    }
    return pCutList;
}

/**Function*************************************************************

  Synopsis    [Tries removing fanins from the cut.]

  Description [Returns the old cut if we could not find a better one. 
  Returns the new cut if it was found. (The old cut is not deleted.)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeCompressFanins( Resm_Node_t * pPivot, Resm_Node_t * pCut )
{
    Resm_Man_t * p = pPivot->p;
    Resm_Node_t * pCutNew, * pCutTemp;
    int Perm[10], i;

    // do not try removing for small cuts (???)
    if ( pCut->nLeaves == 2 )
        return pCut;

    // derive the order, in which the fanins should be removed
    Resm_NodeOrderFanins( pCut, Perm );
    // try removing the fanins
    pCutNew = NULL;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( pCutNew = Resm_NodeDropFanin( pPivot, pCut, pCut->ppLeaves[Perm[i]] ) )
        {
            pCutNew = Resm_NodeCompressFanins( pPivot, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Resm_NodeFree( pCutTemp );
            return pCutNew;
        }
    }
    return pCut;
}


/**Function*************************************************************

  Synopsis    [Collects unique fanins of the node minus the given one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeCollectUniqueMinusFanin( Resm_Node_t * pCut, Resm_Node_t * pFanin, Resm_Node_t * pResult[] )
{
    Resm_Node_t * pLeaf;
    int i, k, nResult;
    // reduce the fanins
    nResult = 0;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pLeaf = Resm_Regular(pCut->ppLeaves[i]);
        if ( pLeaf == pFanin )
            continue;
        // make sure it is unique
        for ( k = 0; k < nResult; k++ )
            if ( pResult[k] == pLeaf )
                break;
        if ( k == nResult )
            pResult[nResult++] = pLeaf;
    }
    return nResult;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


