/**CFile****************************************************************

  FileName    [resLogic.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resLogic.c,v 1.1 2005/01/23 06:59:48 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Res_NodeSupportContainment( Res_Man_t * p, Fpga_Node_t * pRoot, Fpga_Node_t * pNode );

static int  Res_CollectRelatedLogicInternal( Res_Man_t * p, Fpga_Node_t * pNode, float DelayTop, float DelayBot, int fUseTfiOnly, int fCollectUsed, int Limit );
static int  Res_CollectTfi_rec( Fpga_NodeVec_t * vVisited, Fpga_Node_t * pNode, float DelayBot );
static void Res_CollectTfo( Res_Man_t * p, Fpga_NodeVec_t * vQueue, Fpga_Node_t * pPivot, float DelayTop );
static void Res_MarkTfo_rec( Fpga_NodeVec_t * vVisited, Fpga_Node_t * pNode, float DelayTop );
static void Res_CollectBalanceArrays( Res_Man_t * p, int nLimit );
static void Res_CollectVerifyArrays( Res_Man_t * p );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the set of nodes that can be candidates.]

  Description [This procedure computes the set of nodes in the extended TFI
  cone of the given node that can be considered as candidates in resynthesis.
  The number of nodes is limited by the given number (nLimit). The nodes
  are selected to be (a) already used in the mapping, (b) their level does
  not exceed the level of the given node. Support filtering and simulation
  info filtering will be used later.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Res_CollectRelatedLogic( Res_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_Node_t * pTemp;
    Fpga_NodeVec_t * vNodes;
    int LevelLimit, iLevel, i;
    float DelayUnit;
    int fUseTfiOnly;   // set to 1 to use only TFI nodes
    int fCollectUsed;  // set to 1 to collect only used nodes
    int nLevelStart;   // the logic level at which we start collecting candidates
    int nLevelStop;    // the logic level to stop collecting candidates
    int nLimit;        // the max number of candidates

    // set the parameters
    DelayUnit = p->pMan->pLutLib->pLutDelays[2];
    if ( !p->fDoingArea )
    {
        nLevelStart  = 3;   
        nLevelStop   = 10;  

        fUseTfiOnly  = 0; 
        fCollectUsed = 0;
        nLimit       = 50; 
    }
    else
    {
        nLevelStart  = 1;   
        nLevelStop   = 8;  

        fUseTfiOnly  = 0; 
        fCollectUsed = 1;
        nLimit       = 30; 
    }

    // mark the transitive fanin
    if ( fUseTfiOnly )
//        Fpga_MappedMark_rec( pNode );
        Fpga_MappingMark_rec( pNode );

    // mark the node and fanins as used
    assert( pNode->nRefs > 0 );
    pNode->fUsed = 1;
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        pNode->pCutBest->ppLeaves[i]->fUsed = 1;

    // start the node array
    vNodes = Fpga_NodeVecAlloc( nLimit );
    // go through the levels below the given node
    // select the used nodes whose support is contained in the support of the node
    LevelLimit = FPGA_MAX( ((int)pNode->Level) - nLevelStop, 0 );
    assert( LevelLimit >= 0 );
    for ( iLevel = pNode->Level - nLevelStart; iLevel >= LevelLimit; iLevel-- )
        for ( pTemp = p->vLevels->pArray[iLevel]; pTemp; pTemp = pTemp->pLevel )
            if ( (!fCollectUsed || pTemp->nRefs) && 
                 (!fUseTfiOnly || pNode->fMark0) && !pTemp->fUsed && 
                 pTemp->pCutBest->tArrival + DelayUnit <= pNode->tRequired && 
                 Res_NodeSupportContainment( p, pNode, pTemp ) )
            {
                Fpga_NodeVecPush( vNodes, pTemp );
                if ( vNodes->nSize >= nLimit )
                    goto END;
            }
END:
    // unmakr the transitive fanin
    if ( fUseTfiOnly )
//        Fpga_MappedUnmark_rec( pNode );
        Fpga_MappingUnmark_rec( pNode );

    // unmark the node and fanins
    pNode->fUsed = 0;
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        pNode->pCutBest->ppLeaves[i]->fUsed = 0;

    if ( vNodes->nSize == 0 )
    {
        Fpga_NodeVecFree( vNodes );
        return NULL;
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Checks if Node's real support is contained in Root's support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_NodeSupportContainment( Res_Man_t * p, Fpga_Node_t * pRoot, Fpga_Node_t * pNode )
{
    unsigned * pSuppRoot, * pSuppNode;
    int i;

    pSuppRoot = p->pSuppF[pRoot->NumA];
    pSuppNode = p->pSuppF[pNode->NumA];

    // pSuppNode => pSuppRoot    or    pSuppNode & !pSuppRoot == 0
    for ( i = 0; i < p->nSuppWords; i++ )
        if ( pSuppNode[i] & ~pSuppRoot[i] )
            return 0;
    return 1;
}






/**Function*************************************************************

  Synopsis    [Top-level procedure to compute the candidates for the given node.]

  Description [This procedure returns three arrays of nodes:
  Array p->vWinCands contains the internal nodes of the neighborhood.
  Array p->vWinNodes contains the AND nodes used to set up the SAT problem.
  Array p->vWinTotal contains the all the nodes, including the PI of the neighborhood.]
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_CollectRelatedLogic2( Res_Man_t * p, Fpga_Node_t * pNode )
{
    float DelayUnit;   // the time unit
    float DelayTop;    // the arrival time to start collecting candidates
    float DelayBot;    // the arrival time to stop collecting candidates
    int fUseTfiOnly;   // set to 1 to use only TFI nodes
    int fCollectUsed;  // set to 1 to collect only nodes used in the mapping
    int nLimit;        // the max number of candidates
    int fFinalRun;     // is set to 1 if the window cannot be extended
    int nIter;

    // set the parameters
    DelayUnit = p->pMan->pLutLib->pLutDelays[2];
    if ( !p->fDoingArea )
    {
        DelayTop     = pNode->pCutBest->tArrival - 1 * DelayUnit;
        DelayBot     = pNode->pCutBest->tArrival - 8 * DelayUnit;

        fUseTfiOnly  = 0; 
        fCollectUsed = 0;
        nLimit       = 50; 
    }
    else
    {
        DelayTop     = pNode->tRequired          - 1 * DelayUnit;
        DelayBot     = pNode->pCutBest->tArrival -10 * DelayUnit;

        fUseTfiOnly  = 0; 
        fCollectUsed = 1;
        nLimit       = 30; 
    }

    // increase the window size to include more nodes, if possible
    nIter = 0;
    do
    {
        nIter++;
        fFinalRun = Res_CollectRelatedLogicInternal( p, pNode, DelayTop, DelayBot, fUseTfiOnly, fCollectUsed, nLimit );
        DelayBot -= DelayUnit;
    }
    while ( Fpga_NodeVecReadSize(p->vWinCands) < nLimit && !fFinalRun );

    // reduce the window size to simplify the problem, if needed
    while ( Fpga_NodeVecReadSize(p->vWinTotal) > 100 )
    {
        DelayBot += DelayUnit;
        Res_CollectRelatedLogicInternal( p, pNode, DelayTop, DelayBot, fUseTfiOnly, fCollectUsed, nLimit );
    }

    // if the number of nodes is more than the limit, move them to the fanin array
    Res_CollectBalanceArrays( p, nLimit );
    Res_CollectVerifyArrays( p );
}

/**Function*************************************************************

  Synopsis    [Collects the cones for the given delay margins.]

  Description [Returns 1 if it is known that the run is final.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_CollectRelatedLogicInternal( Res_Man_t * p, Fpga_Node_t * pNode, 
    float DelayTop, float DelayBot, int fUseTfiOnly, int fCollectUsed, int Limit )
{
    Fpga_Node_t * pCand;
    int i, fFinalRun;

    // the node should be references and not a PI
    assert( pNode->nRefs > 0 );
    assert( Fpga_NodeIsAnd(pNode) > 0 );

    // clean the arrays
    Fpga_NodeVecClear( p->vWinTotal  );  // the nodes
    Fpga_NodeVecClear( p->vWinCands  );  // candidate nodes
    Fpga_NodeVecClear( p->vWinNodes  );  // AND-nodes used for SAT

    // collect and mark TFI nodes whose arrival is between DelayTop and DelayBot
    // all the nodes are collected in p->vWinTotal
    // the nodes inside the area of interest have delays [DelayTop,DelayBot]
    // the nodes outside the area of interest have delays below DelayBot
    fFinalRun = Res_CollectTfi_rec( p->vWinTotal, pNode, DelayBot );
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        fFinalRun *= Res_CollectTfi_rec( p->vWinTotal, pNode->pCutBest->ppLeaves[i], DelayBot );
    assert( p->vWinTotal->pArray[0] == pNode );

    // sort in the increasing order by the arrival times
    Fpga_SortNodesByArrivalTimes( p->vWinTotal );

    // collect the transitive fanout of these nodes up to DelayTop
    if ( !fUseTfiOnly )
        Res_CollectTfo( p, p->vWinTotal, pNode, DelayTop );


    // collect the candidates (whose delay is below DelayTop and who are not fanins)
    // also, sort all the collected nodes into two types: internal nodes and fanins
    // mark the node and fanins
    pNode->fUsed = 1;
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        pNode->pCutBest->ppLeaves[i]->fUsed = 1;
    // process all nodes
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
    {
        pCand = p->vWinTotal->pArray[i];
        pCand->fMark0 = 0;
        // the node is a fanin if it is a PI or its delay is below
        if ( Fpga_NodeIsAnd(pCand) && pCand->pCutBest->tArrival >= DelayBot )
            Fpga_NodeVecPush( p->vWinNodes, pCand );
        // save the candidate
        if ( !pCand->fUsed && pCand->pCutBest->tArrival <= DelayTop && (pCand->nRefs || !fCollectUsed) )
            Fpga_NodeVecPush( p->vWinCands, pCand );
    }
    // unmark the node and fanins
    pNode->fUsed = 0;
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        pNode->pCutBest->ppLeaves[i]->fUsed = 0;

    return fFinalRun;
}

/**Function*************************************************************

  Synopsis    [Collects and marks all the relevant nodes in the TFI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_CollectTfi_rec( Fpga_NodeVec_t * vVisited, Fpga_Node_t * pNode, float DelayBot )
{
    // skip the collected node
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return 1;
    // collect the node
    pNode->fMark0 = 1;
    Fpga_NodeVecPush( vVisited, pNode );
    // do not pursue the node if it is a PI or if it arrives too early
    if ( !Fpga_NodeIsAnd(pNode) || pNode->pCutBest->tArrival < DelayBot )
        return !Fpga_NodeIsAnd(pNode);
    // collect the fanins
    return Res_CollectTfi_rec( vVisited, Fpga_Regular(pNode->p1), DelayBot ) *
           Res_CollectTfi_rec( vVisited, Fpga_Regular(pNode->p2), DelayBot );
}


/**Function*************************************************************

  Synopsis    [Add the nodes reachable in the TFO up to the given level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_CollectTfo( Res_Man_t * p, Fpga_NodeVec_t * vQueue, Fpga_Node_t * pPivot, float DelayTop )
{
    Fpga_Node_t * pNode, * pNode1, * pNode2, * pFanout;
    int i;
    // mark the TFO of the node
    Fpga_NodeVecClear( p->vWinVisit );
    Res_MarkTfo_rec( p->vWinVisit, pPivot, DelayTop );

    // add the involved fanout nodes
    for ( i = 0; i < vQueue->nSize; i++ ) 
    {
        pNode = vQueue->pArray[i];
        // go through the fanouts
        Fpga_NodeForEachFanout( pNode, pFanout )
        {
            // skip the fanout if
            // - it is already collected (fMark0)
            // - it is in the TFO of the node (fMark1)
            // - its arrival time is above the threshold
            if ( pFanout->fMark0 || pFanout->fMark1 || pFanout->pCutBest->tArrival > DelayTop )
                continue;
            pNode1 = Fpga_Regular(pFanout->p1);
            pNode2 = Fpga_Regular(pFanout->p2);
            if ( pNode1->fMark0 && pNode2->fMark0 )
            {
                pFanout->fMark0 = 1;
                Fpga_NodeVecPushOrder( vQueue, pFanout, 1 );
            }
        }
    }

    // unmark the TFO
    for ( i = 0; i < p->vWinVisit->nSize; i++ )
        p->vWinVisit->pArray[i]->fMark1 = 0;
}

/**Function*************************************************************

  Synopsis    [Mark the TFO of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_MarkTfo_rec( Fpga_NodeVec_t * vVisited, Fpga_Node_t * pNode, float DelayTop )
{
    Fpga_Node_t * pFanout;
    // skip the collected node
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark1 )
        return;
    // collect the node
    pNode->fMark1 = 1;
    Fpga_NodeVecPush( vVisited, pNode );
    // do not pursue the node if it is a PI or if it arrives too early
    if ( pNode->pCutBest->tArrival >= DelayTop )
        return;
    // visit the fanouts
    Fpga_NodeForEachFanout( pNode, pFanout )
        Res_MarkTfo_rec( vVisited, pFanout, DelayTop );
}



/**Function*************************************************************

  Synopsis    [Move the nodes between the arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_CollectBalanceArrays( Res_Man_t * p, int nLimit )
{
/*
    Fpga_Node_t * pCand;
    int i, nShift;

    if ( p->vWinNodes->nSize <= nLimit )
        return;

    // move the first nodes in fanins
    nShift = p->vWinNodes->nSize - nLimit;
    for ( i = 0; i < nShift; i++ )
    {
        pCand = p->vWinNodes->pArray[i];
        Fpga_NodeVecPush( p->vWinFanins, pCand );
    }
    // shift the remaining nodes
    for ( i = 0; i < nLimit; i++ )
        p->vWinNodes->pArray[i] = p->vWinNodes->pArray[nShift + i];
    p->vWinNodes->nSize = nLimit;
*/
}

/**Function*************************************************************

  Synopsis    [Verifies the arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_CollectVerifyArrays( Res_Man_t * p )
{
    Fpga_Node_t * pNode;
    int i;
    // mark all the nodes
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
        p->vWinTotal->pArray[i]->fMark0 = 1;
    // check internal nodes
    for ( i = 0; i < p->vWinNodes->nSize; i++ )
    {
        pNode = p->vWinNodes->pArray[i];
        assert( pNode->fMark0 );
        assert( Fpga_Regular(pNode->p1)->fMark0 );
        assert( Fpga_Regular(pNode->p2)->fMark0 );
    }
    // unmark all the nodes
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
        p->vWinTotal->pArray[i]->fMark0 = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


