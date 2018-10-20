/**CFile****************************************************************

  FileName    [resFanin.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resFanin.c,v 1.1 2005/01/23 06:59:47 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Fpga_Cut_t * Fpga_NodeDropFanin( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Cut_t * pCut, Fpga_Node_t * pFanin );
static Fpga_Cut_t * Fpga_NodeReplaceFaninByOne( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes );
static Fpga_Cut_t * Fpga_NodeReplaceFaninByTwo( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes );
static Fpga_Cut_t * Fpga_NodeReplaceFaninByThree( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes );
static Fpga_Cut_t * Fpga_NodeCompressFanins( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCut );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure tries to move one fanin of the node using candidates.]

  Description [Returns the cut set.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Res_NodeReplaceOneFanin( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes, int fTryTwo )
{
    Fpga_Cut_t * pCutList = NULL, * pCuts, * pTemp;

    assert( pNode->pCutBest->nLeaves <= p->pMan->pLutLib->LutMax );

    // try dropping this fanin
    if ( pNode->pCutBest->nLeaves > 2 )
    if ( pCuts = Fpga_NodeDropFanin( p, pNode, pNode->pCutBest, pFanin ) )
        return pCuts;

    // try replacing fanin by one node
    pCuts = Fpga_NodeReplaceFaninByOne( p, pNode, pFanin, vNodes );
    pCutList = Fpga_CutListAppend( pCutList, pCuts );
    if ( !fTryTwo )
        return pCutList;

    // it may happen that an inferior one-node change prevents a better two-node change
    // mark the nodes used in one-node changes
    for ( pTemp = pCutList; pTemp; pTemp = pTemp->pNext )
        pTemp->ppLeaves[pTemp->nLeaves-1]->fUsed = 1;

    // try replacing fanin by two nodes
    pCuts    = Fpga_NodeReplaceFaninByTwo( p, pNode, pFanin, vNodes );
    pCutList = Fpga_CutListAppend( pCutList, pCuts );

    // unmark the nodes used in one-node changes
    for ( pTemp = pCutList; pTemp; pTemp = pTemp->pNext )
        pTemp->ppLeaves[pTemp->nLeaves-1]->fUsed = 0;
    if ( pCutList )
        return pCutList;
/*
    // try three node combinations
    if ( pNode->pCutBest->nLeaves + 1 <= p->pMan->pLutLib->LutMax )
        pCutList = Fpga_NodeReplaceFaninByThree( p, pNode, pFanin, vNodes );
*/
    return pCutList;
}

/**Function*************************************************************

  Synopsis    [Tries to drop the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_NodeDropFanin( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Cut_t * pCut, Fpga_Node_t * pFanin )
{
    Fpga_Node_t * ppNodes[10];
    int i, iNode;
    // reduce the fanins
    for ( i = iNode = 0; i < pCut->nLeaves; i++ )
        if ( pCut->ppLeaves[i] != pFanin )
            ppNodes[iNode++] = pCut->ppLeaves[i];
    assert( iNode == pCut->nLeaves - 1 );
    return Res_CheckCut( p, pPivot, ppNodes, pCut->nLeaves - 1, 1 );
}

/**Function*************************************************************

  Synopsis    [Tries to drop the fanin and add one extra node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_NodeReplaceFaninByOne( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes )
{
    Fpga_Node_t * ppNodes[10];
    Fpga_Cut_t * pCutNew, * pCutTemp, * pCutList;
    int i, k, nLeaves;

    // reduce the fanins
    nLeaves = pNode->pCutBest->nLeaves;
    for ( i = k = 0; i < nLeaves; i++ )
        if ( pNode->pCutBest->ppLeaves[i] != pFanin )
            ppNodes[k++] = pNode->pCutBest->ppLeaves[i];
    assert( k == nLeaves - 1 );

    // try adding one more node
    pCutList = NULL;
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        ppNodes[nLeaves-1] = vNodes->pArray[i];
        if ( pCutNew = Res_CheckCut( p, pNode, ppNodes, nLeaves, 1 ) )
        {
            pCutNew = Fpga_NodeCompressFanins( p, pNode, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Fpga_CutFree( p->pMan, pCutTemp );
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
Fpga_Cut_t * Fpga_NodeReplaceFaninByTwo( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes )
{
    Fpga_Node_t * ppNodes[10];
    Fpga_Cut_t * pCutNew, * pCutTemp, * pCutList;
    int i, j, k, nLeaves;

    // reduce the fanins
    nLeaves = pNode->pCutBest->nLeaves;
    for ( i = k = 0; i < nLeaves; i++ )
        if ( pNode->pCutBest->ppLeaves[i] != pFanin )
            ppNodes[k++] = pNode->pCutBest->ppLeaves[i];
    assert( k == nLeaves - 1 );

    // try adding two more fanins
    pCutList = NULL;
    for ( i = 0; i < vNodes->nSize; i++ )
    if ( !vNodes->pArray[i]->fUsed )
    for ( j = i+1; j < vNodes->nSize; j++ )
    if ( !vNodes->pArray[j]->fUsed )
    {
        ppNodes[nLeaves-1] = vNodes->pArray[i];
        ppNodes[nLeaves  ] = vNodes->pArray[j];
        if ( pCutNew = Res_CheckCut( p, pNode, ppNodes, nLeaves+1, 1 ) )
        {
            pCutNew = Fpga_NodeCompressFanins( p, pNode, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Fpga_CutFree( p->pMan, pCutTemp );
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
Fpga_Cut_t * Fpga_NodeReplaceFaninByThree( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes )
{
    Fpga_Node_t * ppNodes[10];
    Fpga_Cut_t * pCutNew, * pCutTemp, * pCutList;
    int i, j, k, nLeaves;
    int nSizeLimit; // the maximum number of nodes to consider

    if ( !p->fDoingArea )
        nSizeLimit = 40;
    else
        nSizeLimit = 50;

    // reduce the fanins
    nLeaves = pNode->pCutBest->nLeaves;
    for ( i = k = 0; i < nLeaves; i++ )
        if ( pNode->pCutBest->ppLeaves[i] != pFanin )
            ppNodes[k++] = pNode->pCutBest->ppLeaves[i];
    assert( k == nLeaves - 1 );

    // try adding two more fanins
    pCutList = NULL;
    nSizeLimit = FPGA_MIN( nSizeLimit, vNodes->nSize );
    for ( i = 0; i < nSizeLimit; i++ )
    for ( j = i+1; j < nSizeLimit; j++ )
    for ( k = j+1; k < nSizeLimit; k++ )
    {
        ppNodes[nLeaves-1] = vNodes->pArray[i];
        ppNodes[nLeaves  ] = vNodes->pArray[j];
        ppNodes[nLeaves+1] = vNodes->pArray[k];
        if ( pCutNew = Res_CheckCut( p, pNode, ppNodes, nLeaves+2, 1 ) )
        {
            pCutNew = Fpga_NodeCompressFanins( p, pNode, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Fpga_CutFree( p->pMan, pCutTemp );
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
Fpga_Cut_t * Fpga_NodeCompressFanins( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCut )
{
    Fpga_Cut_t * pCutNew, * pCutTemp;
    int Perm[10], i;

    // do not try removing for small cuts (???)
    if ( pCut->nLeaves == 2 )
        return pCut;

    // derive the order, in which the fanins should be removed
    Res_NodeOrderFanins( p, pCut, Perm );
    // try removing the fanins
    pCutNew = NULL;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        if ( pCutNew = Fpga_NodeDropFanin( p, pNode, pCut, pCut->ppLeaves[Perm[i]] ) )
        {
            pCutNew = Fpga_NodeCompressFanins( p, pNode, pCutTemp = pCutNew );
            if ( pCutNew != pCutTemp )
                Fpga_CutFree( p->pMan, pCutTemp );
            return pCutNew;
        }
    }
    return pCut;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


