/**CFile****************************************************************

  FileName    [mntkUpdate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkUpdate.c,v 1.1 2005/02/28 05:34:32 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mntk_NodeUpdateArrivalTimeOne( Mntk_Man_t * p, Mntk_Node_t * pNode );
static void Mntk_NodeUpdateRequiredTimeOne( Mntk_Man_t * p, Mntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Updates the arrival times after the node has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeUpdateArrivalTimeSlow( Mntk_Node_t * pRoot )
{
    Mntk_Man_t * p = pRoot->p;
    Mntk_ComputeArrivalAll( p, 0 );
    // update the global required times
    if ( !(p->pPseudo->tArrival.Worst < p->fRequiredGlo + p->fEpsilon &&
           p->pPseudo->tArrival.Worst > p->fRequiredGlo - p->fEpsilon) )
    {
//        printf( "Maximum arrival time goes down from %5.2f to %5.2f (gain = %5.2f).\n", 
//            p->fRequiredGlo, p->pPseudo->tArrival.Worst, p->fRequiredGlo - p->pPseudo->tArrival.Worst );
        Mntk_TimeComputeRequiredGlobal( p );
    }
}

/**Function*************************************************************

  Synopsis    [Updates the required times after the node has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeUpdateRequiredTimeSlow( Mntk_Node_t * pNode )
{
    Mntk_TimeComputeRequiredGlobal( pNode->p );
}

/**Function*************************************************************

  Synopsis    [This procedure incrementally updates arrival times.]

  Description [Assuming that the node's arrival time has changed, 
  this procedure propagates the arrival times in the area of change.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeUpdateArrivalTime( Mntk_Node_t * pPivot )
{
    Mntk_Man_t * p = pPivot->p;
    Mntk_Node_t * pNode;
    int i, k;

    // put the fanouts of the node on the stack
    Mntk_NodeVecClear( p->vWinVisit );
    for ( k = 0; k < pPivot->vFanouts->nSize; k++ )
        Mntk_NodeVecPushUniqueOrder( p->vWinVisit, pPivot->vFanouts->pArray[k], 1 );

    // iterate through the nodes on the stack
    for ( i = 0; i < p->vWinVisit->nSize; i++ )
    {
        // get the node from the stack
        pNode = p->vWinVisit->pArray[i];
        // compute its arrival time
        Mntk_NodeComputeArrival( pNode );
        // if the node's arrival time did not change, skip it
        if ( Mntk_TimesAreEqual( &pNode->tPrevious, &pNode->tArrival, p->fEpsilon ) )
            continue;
        // iterate through the fanouts of the node and put them on the stack
        if ( pNode->vFanouts )
            for ( k = 0; k < pNode->vFanouts->nSize; k++ )
                Mntk_NodeVecPushUniqueOrder( p->vWinVisit, pNode->vFanouts->pArray[k], 1 );
    }
    assert( p->pPseudo->tArrival.Worst < p->fRequiredGlo + p->fEpsilon );

    // if the pseudo-node is on the stack and its arrival time has changed, update other nodes
    if ( p->pPseudo->tArrival.Worst < p->fRequiredGlo - p->fEpsilon )
    {
//        printf( "Maximum arrival time goes down from %5.2f to %5.2f (gain = %5.2f).\n", 
//            p->fRequiredGlo, p->pPseudo->tArrival.Worst, p->fRequiredGlo - p->pPseudo->tArrival.Worst );
        Mntk_TimeComputeRequiredGlobal( p );
    }
}

/**Function*************************************************************

  Synopsis    [This procedure incrementally updates required times.]

  Description [Assuming that the node's fanins' required time has changed, 
  this procedure propagates the changes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeUpdateRequiredTime( Mntk_Node_t * pNodeOld, Mntk_Node_t * pNodeNew )
{
    Mntk_Man_t * p = pNodeOld->p;
    Mntk_Node_t * pNode;
    int i, k;

    // put the fanins of both nodes on the stack to update their required times
    // put the fanins of the node on the stack
    Mntk_NodeVecClear( p->vWinVisit );
    for ( k = 0; k < pNodeOld->nLeaves; k++ )
        Mntk_NodeVecPushUniqueOrder( p->vWinVisit, Mntk_Regular(pNodeOld->ppLeaves[k]), 0 );
    for ( k = 0; k < pNodeNew->nLeaves; k++ )
        Mntk_NodeVecPushUniqueOrder( p->vWinVisit, Mntk_Regular(pNodeNew->ppLeaves[k]), 0 );

    // iterate the nodes on the stack
    for ( i = 0; i < p->vWinVisit->nSize; i++ )
    {
        // get the node from the stack
        pNode = p->vWinVisit->pArray[i];
        // compute its required time
        Mntk_NodeComputeRequired( pNode );
        // if the node's required time did not change, skip it
        if ( Mntk_TimesAreEqual( &pNode->tPrevious, &pNode->tRequired[1], p->fEpsilon ) )
            continue;
        // iterate through the fanins of the node and put them on the stack
        for ( k = 0; k < pNode->nLeaves; k++ )
            Mntk_NodeVecPushUniqueOrder( p->vWinVisit, Mntk_Regular(pNode->ppLeaves[k]), 0 );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


