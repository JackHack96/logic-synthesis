/**CFile****************************************************************

  FileName    [resUpdate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resUpdate.c,v 1.1 2005/01/23 06:59:49 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Res_NodeUpdateArrivalTimeOne( Res_Man_t * p, Fpga_Node_t * pNode );
static void Res_NodeUpdateRequiredTimeOne( Res_Man_t * p, Fpga_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Updates the arrival times after the node has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeUpdateArrivalTimeSlow( Res_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_NodeVec_t * vNodes;
    Fpga_Cut_t * pCut;
    float tRequiredGlobal;
    int i;

    // collect the nodes affected by the change
    vNodes = Fpga_MappingDfsCuts( p->pMan );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pCut = vNodes->pArray[i]->pCutBest;
        pCut->tArrival = Fpga_TimeCutComputeArrival( p->pMan, pCut );
    }
    Fpga_NodeVecFree( vNodes );

    if ( !p->fDoingArea )
    {
        // update the global required times
        tRequiredGlobal = Fpga_TimeComputeArrivalMax( p->pMan );
        assert( tRequiredGlobal <= p->pMan->fRequiredGlo );
        if ( tRequiredGlobal < p->pMan->fRequiredGlo - p->pMan->fEpsilon )
            Fpga_TimeComputeRequiredGlobal( p->pMan );
    }
}

/**Function*************************************************************

  Synopsis    [Updates the required times after the node has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeUpdateRequiredTimeSlow( Res_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_NodeVec_t * vNodes;
    // collect nodes reachable from node in the DFS order through the best cuts
    vNodes = Fpga_MappingDfsCuts( p->pMan );
    Fpga_TimePropagateRequired( p->pMan, vNodes );
    Fpga_NodeVecFree( vNodes );
}


#if 0

/**Function*************************************************************

  Synopsis    [This procedure incrementally updates arrival times.]

  Description [Assuming that the node's arrival time has changed, 
  this procedure propagates the changes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeUpdateArrivalTime( Res_Man_t * p, Fpga_Node_t * pPivot )
{
    float tRequiredGlobal;
    int i, k;

    // if the arrival time did not change, return
    if ( pPivot->pCutOld->tArrival == pPivot->pCutBest->tArrival )
        return;

    // clean the levelized structure
    for ( i = 0; i < p->vLevels->nSize; i++ ) 
        p->pvStack[i]->nSize = 0;

    // start with the entry corresponding to this node
    assert( p->pvStack[pPivot->Level]->nSize == 0 );
    Fpga_NodeVecPush( p->pvStack[pPivot->Level], pPivot );

    for ( i = 0; i < p->vLevels->nSize; i++ ) // iterate through the levels
        for ( k = 0; i < p->pvStack[i]->nSize; k++ ) // iterate through the nodes on this level
            Res_NodeUpdateArrivalTimeOne( p, p->pvStack[i]->pArray[k] );

    // update the global required times
    if ( p->pvStack[p->vLevels->nSize-1]->nSize > 0 )
    {
        tRequiredGlobal = Fpga_TimeComputeArrivalMax( p->pMan );
        assert( tRequiredGlobal <= p->pMan->fRequiredGlo );
        if ( tRequiredGlobal < p->pMan->fRequiredGlo - p->pMan->fEpsilon )
            Fpga_TimeComputeRequiredGlobal( p->pMan );
    }
}

/**Function*************************************************************

  Synopsis    [Propagates changes to one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeUpdateArrivalTimeOne( Res_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_Node_t * pFanout;
    float tArrivalOld;
    int i;

    // iterate through the fanouts
    for ( i = 0; i < pNode->vFanouts->nSize; i++ )
    {
        // get the fanout
        pFanout = pNode->vFanouts->pArray[i];
        assert( pFanout->Level > pNode->Level );
        // save the current arrival time of the fanout
        tArrivalOld = pFanout->pCutBest->tArrival;
        // compute the new arrival time
        pFanout->pCutBest->tArrival = Fpga_TimeCutComputeArrival( p->pMan, pFanout->pCutBest );
        // if the arrival time did not change, continue to the next fanout
        if ( pFanout->pCutBest->tArrival > tArrivalOld - p->pMan->fEpsilon && 
             pFanout->pCutBest->tArrival < tArrivalOld + p->pMan->fEpsilon )
            continue;
        // otherwise, place the fanout on the stack if it is not yet there
        Fpga_NodeVecPushUnique( p->pvStack[pFanout->Level], pFanout );
    }
}


/**Function*************************************************************

  Synopsis    [This procedure incrementally updates required times.]

  Description [Assuming that the node's fanins' required time has changed, 
  this procedure propagates the changes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeUpdateRequiredTime( Res_Man_t * p, Fpga_Node_t * pPivot )
{
    Fpga_Node_t * pNode;
    int i, k;

    // start the update list with the old and new fanins of the node
    assert( p->pvStack[pPivot->Level]->nSize == 0 );
    assert( pPivot->pCutBest != pPivot->pCutOld );

    // clean the levelized structure
    for ( i = 0; i < p->vLevels->nSize; i++ ) 
        p->pvStack[i]->nSize = 0;

    // the old fanins
    for ( i = 0; i < pPivot->pCutOld->nLeaves; i++ )
    {
        pNode = pPivot->pCutOld->ppLeaves[i];
        Fpga_NodeVecPushUnique( p->pvStack[pNode->Level], pNode );
    }
    // the new fanins
    for ( i = 0; i < pPivot->pCutBest->nLeaves; i++ )
    {
        pNode = pPivot->pCutBest->ppLeaves[i];
        Fpga_NodeVecPushUnique( p->pvStack[pNode->Level], pNode );
    }

    for ( i = p->vLevels->nSize-1; i >= 0; i-- ) // iterate through the levels
        for ( k = 0; i < p->pvStack[i]->nSize; k++ ) // iterate through the nodes on this level
            Res_NodeUpdateRequiredTimeOne( p, p->pvStack[i]->pArray[k] );
}

/**Function*************************************************************

  Synopsis    [Propagates changes to one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeUpdateRequiredTimeOne( Res_Man_t * p, Fpga_Node_t * pNode )
{
    Fpga_Node_t * pFanin;
    float fRequiredOld;
    int i;

    // iterate through the fanouts
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
    {
        // get the fanin
        pFanin = pNode->pCutBest->ppLeaves[i];
        assert( pFanin->Level < pNode->Level );
        // remember the old required time of the fanin
        fRequiredOld = pFanin->tRequired;
        // compute the new required time of the fanin
        pFanin->tRequired = pNode->tRequired - p->pMan->pLutLib->pLutDelays[pNode->pCutBest->nLeaves];
        // if the required time does not change, continue 
        if ( pFanin->tRequired > fRequiredOld - p->pMan->fEpsilon && 
             pFanin->tRequired < fRequiredOld + p->pMan->fEpsilon )
            continue;
        // otherwise, place the fanout on the stack if it is not yet there
        Fpga_NodeVecPushUnique( p->pvStack[pFanin->Level], pFanin );
    }
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


