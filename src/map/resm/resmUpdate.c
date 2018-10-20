/**CFile****************************************************************

  FileName    [resmUpdate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmUpdate.c,v 1.1 2005/01/23 06:59:53 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Resm_NodeUpdateArrivalTimeOne( Resm_Man_t * p, Resm_Node_t * pNode );
static void Resm_NodeUpdateRequiredTimeOne( Resm_Man_t * p, Resm_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Updates the arrival times after the node has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeUpdateArrivalTimeSlow( Resm_Node_t * pRoot )
{
    Resm_Man_t * p = pRoot->p;
    Resm_ComputeArrivalAll( p, 0 );
    // update the global required times
    if ( !(p->pPseudo->tArrival.Worst < p->fRequiredGlo + p->fEpsilon &&
           p->pPseudo->tArrival.Worst > p->fRequiredGlo - p->fEpsilon) )
    {
//        printf( "Maximum arrival time goes down from %5.2f to %5.2f (gain = %5.2f).\n", 
//            p->fRequiredGlo, p->pPseudo->tArrival.Worst, p->fRequiredGlo - p->pPseudo->tArrival.Worst );
        Resm_TimeComputeRequiredGlobal( p );
    }
}

/**Function*************************************************************

  Synopsis    [Updates the required times after the node has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeUpdateRequiredTimeSlow( Resm_Node_t * pNode )
{
    Resm_TimeComputeRequiredGlobal( pNode->p );
}

/**Function*************************************************************

  Synopsis    [This procedure incrementally updates arrival times.]

  Description [Assuming that the node's arrival time has changed, 
  this procedure propagates the arrival times in the area of change.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeUpdateArrivalTime( Resm_Node_t * pPivot )
{
    Resm_Man_t * p = pPivot->p;
    Resm_Node_t * pNode;
    int i, k;

    // put the fanouts of the node on the stack
    Resm_NodeVecClear( p->vWinVisit );
    for ( k = 0; k < pPivot->vFanouts->nSize; k++ )
        Resm_NodeVecPushUniqueOrder( p->vWinVisit, pPivot->vFanouts->pArray[k], 1 );

    // iterate through the nodes on the stack
    for ( i = 0; i < p->vWinVisit->nSize; i++ )
    {
        // get the node from the stack
        pNode = p->vWinVisit->pArray[i];
        // compute its arrival time
        Resm_NodeComputeArrival( pNode );
        // if the node's arrival time did not change, skip it
        if ( Resm_TimesAreEqual( &pNode->tPrevious, &pNode->tArrival, p->fEpsilon ) )
            continue;
        // iterate through the fanouts of the node and put them on the stack
        if ( pNode->vFanouts )
            for ( k = 0; k < pNode->vFanouts->nSize; k++ )
                Resm_NodeVecPushUniqueOrder( p->vWinVisit, pNode->vFanouts->pArray[k], 1 );
    }
    assert( p->pPseudo->tArrival.Worst < p->fRequiredGlo + p->fEpsilon );

    // if the pseudo-node is on the stack and its arrival time has changed, update other nodes
    if ( p->pPseudo->tArrival.Worst < p->fRequiredGlo - p->fEpsilon )
    {
//        printf( "Maximum arrival time goes down from %5.2f to %5.2f (gain = %5.2f).\n", 
//            p->fRequiredGlo, p->pPseudo->tArrival.Worst, p->fRequiredGlo - p->pPseudo->tArrival.Worst );
        Resm_TimeComputeRequiredGlobal( p );
    }
}

/**Function*************************************************************

  Synopsis    [This procedure incrementally updates required times.]

  Description [Assuming that the node's fanins' required time has changed, 
  this procedure propagates the changes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeUpdateRequiredTime( Resm_Node_t * pNodeOld, Resm_Node_t * pNodeNew )
{
    Resm_Man_t * p = pNodeOld->p;
    Resm_Node_t * pNode;
    int i, k;

    // put the fanins of both nodes on the stack to update their required times
    // put the fanins of the node on the stack
    Resm_NodeVecClear( p->vWinVisit );
    for ( k = 0; k < pNodeOld->nLeaves; k++ )
        Resm_NodeVecPushUniqueOrder( p->vWinVisit, Resm_Regular(pNodeOld->ppLeaves[k]), 0 );
    for ( k = 0; k < pNodeNew->nLeaves; k++ )
        Resm_NodeVecPushUniqueOrder( p->vWinVisit, Resm_Regular(pNodeNew->ppLeaves[k]), 0 );

    // iterate the nodes on the stack
    for ( i = 0; i < p->vWinVisit->nSize; i++ )
    {
        // get the node from the stack
        pNode = p->vWinVisit->pArray[i];
        // compute its required time
        Resm_NodeComputeRequired( pNode );
        // if the node's required time did not change, skip it
        if ( Resm_TimesAreEqual( &pNode->tPrevious, &pNode->tRequired[1], p->fEpsilon ) )
            continue;
        // iterate through the fanins of the node and put them on the stack
        for ( k = 0; k < pNode->nLeaves; k++ )
            Resm_NodeVecPushUniqueOrder( p->vWinVisit, Resm_Regular(pNode->ppLeaves[k]), 0 );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


